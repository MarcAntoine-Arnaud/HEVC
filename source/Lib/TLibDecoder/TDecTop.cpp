/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TDecTop.cpp
    \brief    decoder class
*/

#include "NALread.h"
#include "TDecTop.h"

//! \ingroup TLibDecoder
//! \{

TDecTop::TDecTop()
: m_SEIs(0)
{
  m_pcPic = 0;
  m_iGopSize      = 0;
  m_bGopSizeSet   = false;
  m_iMaxRefPicNum = 0;
  m_uiValidPS = 0;
#if ENC_DEC_TRACE
  g_hTrace = fopen( "TraceDec.txt", "wb" );
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#endif
  m_bRefreshPending = 0;
  m_uiPOCCDR = 0;
  m_uiPOCRA = MAX_UINT;          
  m_uiPrevPOC               = UInt(-1);
  m_bFirstSliceInPicture    = true;
  m_bFirstSliceInSequence   = true;
  m_vAPS.resize(MAX_NUM_SUPPORTED_APS);
  for(Int i=0; i< MAX_NUM_SUPPORTED_APS; i++)
  {
    m_vAPS[i].reserve(APS_RESERVED_BUFFER_SIZE);
  }
}

TDecTop::~TDecTop()
{
#if ENC_DEC_TRACE
  fclose( g_hTrace );
#endif
}

Void TDecTop::create()
{
  m_cGopDecoder.create();
  m_apcSlicePilot = new TComSlice;
  m_uiSliceIdx = m_uiLastSliceIdx = 0;
}

Void TDecTop::destroy()
{

  if(m_vAPS.size() != 0)
  {
    for(Int i=0; i< m_vAPS.size(); i++)
    {
      std::vector<TComAPS>& vAPS = m_vAPS[i];

      if(vAPS.size() != 0)
      {
        for(Int j=0; j< vAPS.size(); j++)
        {
          freeAPS( &vAPS[j]);
        }
      }
    }
  }
  m_cGopDecoder.destroy();
  
  delete m_apcSlicePilot;
  m_apcSlicePilot = NULL;
  
  m_cSliceDecoder.destroy();
#if G1002_RPS
  m_cRPSList.destroy();
#endif
}

Void TDecTop::init()
{
  // initialize ROM
  initROM();
  m_cGopDecoder.init( &m_cEntropyDecoder, &m_cSbacDecoder, &m_cBinCABAC, &m_cCavlcDecoder, &m_cSliceDecoder, &m_cLoopFilter, &m_cAdaptiveLoopFilter, &m_cSAO);
  m_cSliceDecoder.init( &m_cEntropyDecoder, &m_cCuDecoder );
  m_cEntropyDecoder.init(&m_cPrediction);
}

Void TDecTop::deletePicBuffer ( )
{
  TComList<TComPic*>::iterator  iterPic   = m_cListPic.begin();
  Int iSize = Int( m_cListPic.size() );
  
  for (Int i = 0; i < iSize; i++ )
  {
    TComPic* pcPic = *(iterPic++);
    pcPic->destroy();
    
    delete pcPic;
    pcPic = NULL;
  }
  
  // destroy ALF temporary buffers
  m_cAdaptiveLoopFilter.destroy();

  m_cSAO.destroy();
  
  m_cLoopFilter.        destroy();
  
  // destroy ROM
  destroyROM();
}

Void TDecTop::xUpdateGopSize (TComSlice* pcSlice)
{
  if ( !pcSlice->isIntra() && !m_bGopSizeSet)
  {
    m_iGopSize    = pcSlice->getPOC();
    m_bGopSizeSet = true;
    
    m_cGopDecoder.setGopSize(m_iGopSize);
  }
}

Void TDecTop::xGetNewPicBuffer ( TComSlice* pcSlice, TComPic*& rpcPic )
{
  xUpdateGopSize(pcSlice);
  
#if G1002_RPS
  m_iMaxRefPicNum = pcSlice->getSPS()->getMaxNumberOfReferencePictures()+pcSlice->getSPS()->getNumReorderFrames() + 1; // +1 to have space for the picture currently being decoded
#else
  m_iMaxRefPicNum = max(m_iMaxRefPicNum, max(max(2, pcSlice->getNumRefIdx(REF_PIC_LIST_0)+1), m_iGopSize/2 + 2 + pcSlice->getNumRefIdx(REF_PIC_LIST_0)));
  
#endif
  if (m_cListPic.size() < (UInt)m_iMaxRefPicNum)
  {
    rpcPic = new TComPic();
    
    rpcPic->create ( pcSlice->getSPS()->getWidth(), pcSlice->getSPS()->getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, true);
    m_cListPic.pushBack( rpcPic );
    
    return;
  }
  
  Bool bBufferIsAvailable = false;
  TComList<TComPic*>::iterator  iterPic   = m_cListPic.begin();
  while (iterPic != m_cListPic.end())
  {
    rpcPic = *(iterPic++);
#if G1002_RPS
    if ( rpcPic->getReconMark() == false && rpcPic->getOutputMark() == false)
    {
      rpcPic->setOutputMark(false);
#else
    if ( rpcPic->getReconMark() == false )
    {
#endif
      bBufferIsAvailable = true;
      break;
    }

#if G1002_RPS
    if ( rpcPic->getSlice( 0 )->isReferenced() == false  && rpcPic->getOutputMark() == false)
    {
      rpcPic->setOutputMark(false);
#else
    if ( rpcPic->getSlice( 0 )->isReferenced() == false )
    {
#endif
      rpcPic->setReconMark( false );
      rpcPic->getPicYuvRec()->setBorderExtension( false );
      bBufferIsAvailable = true;
      break;
    }
  }
  
  if ( !bBufferIsAvailable )
  {
    pcSlice->sortPicList(m_cListPic);
    iterPic = m_cListPic.begin();
    rpcPic = *(iterPic);
    rpcPic->setReconMark(false);
#if G1002_RPS
    rpcPic->setOutputMark(false);
#endif
    
    // mark it should be extended
    rpcPic->getPicYuvRec()->setBorderExtension(false);
  }
}

Void TDecTop::executeDeblockAndAlf(UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, Int& iSkipFrame, Int& iPOCLastDisplay)
{
  if (!m_pcPic)
    /* nothing to deblock */
    return;

  TComPic*&   pcPic         = m_pcPic;

  // Execute Deblock and ALF only + Cleanup

#if G1002_RPS
  m_cGopDecoder.decompressGop(NULL, pcPic, true);
#else
#if REF_SETTING_FOR_LD
  m_cGopDecoder.decompressGop(NULL, pcPic, true, m_cListPic );
#else
  m_cGopDecoder.decompressGop(NULL, pcPic, true);
#endif
#endif

#if !G1002_RPS
  // Apply decoder picture marking at the end of coding
  pcPic->getSlice( 0 )->decodingTLayerSwitchingMarking( m_cListPic );
#endif

  TComSlice::sortPicList( m_cListPic ); // sorting for application output
  ruiPOC              = pcPic->getSlice(m_uiSliceIdx-1)->getPOC();
  rpcListPic          = &m_cListPic;  
  m_cCuDecoder.destroy();        
  m_bFirstSliceInPicture  = true;

  return;
}
#if G1002_RPS
Void TDecTop::xCreateLostPicture(Int iLostPoc) 
{
  printf("\ninserting lost poc : %d\n",iLostPoc);
  TComSlice cFillSlice;
  cFillSlice.setSPS( &m_cSPS );
  cFillSlice.setPPS( &m_cPPS );
  cFillSlice.initSlice();
  TComPic *cFillPic;
  xGetNewPicBuffer(&cFillSlice,cFillPic);
  cFillPic->getSlice(0)->setSPS( &m_cSPS );
  cFillPic->getSlice(0)->setPPS( &m_cPPS );
  cFillPic->getSlice(0)->initSlice();
  

  TComList<TComPic*>::iterator iterPic = m_cListPic.begin();
  Int closestPoc = 1000000;
  while ( iterPic != m_cListPic.end())
  {
     TComPic * rpcPic = *(iterPic++);
     if(abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc)<closestPoc&&abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc)!=0&&rpcPic->getPicSym()->getSlice(0)->getPOC()!=m_apcSlicePilot->getPOC())
     {
        closestPoc=abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc);
     }
  }
  iterPic = m_cListPic.begin();
  while ( iterPic != m_cListPic.end())
  {
     TComPic *rpcPic = *(iterPic++);
     if(abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc)==closestPoc&&rpcPic->getPicSym()->getSlice(0)->getPOC()!=m_apcSlicePilot->getPOC())
     {
       printf("copying picture %d to %d (%d)\n",rpcPic->getPicSym()->getSlice(0)->getPOC() ,iLostPoc,m_apcSlicePilot->getPOC());
       rpcPic->getPicYuvRec()->copyToPic(cFillPic->getPicYuvRec());
       break;
     }
  }
  cFillPic->setCurrSliceIdx(0);
  for(Int i=0; i<cFillPic->getNumCUsInFrame(); i++) 
  {
    cFillPic->getCU(i)->initCU(cFillPic,i);
  }
  cFillPic->getSlice(0)->setReferenced(true);
  cFillPic->getSlice(0)->setPOC(iLostPoc);
  cFillPic->setReconMark(true);
#if G1002_RPS
  cFillPic->setOutputMark(true);
#endif
  if(m_uiPOCRA == MAX_UINT)
  {
    m_uiPOCRA = iLostPoc;
  }
}
#endif
Bool TDecTop::decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay)
{
  TComPic*&   pcPic         = m_pcPic;
  // Initialize entropy decoder
  m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
  m_cEntropyDecoder.setBitstream      (nalu.m_Bitstream);

  switch (nalu.m_UnitType)
  {
    case NAL_UNIT_SPS:
      m_cEntropyDecoder.decodeSPS( &m_cSPS );

#if AMP
      for (Int i = 0; i < m_cSPS.getMaxCUDepth() - 1; i++)
      {
        m_cSPS.setAMPAcc( i, m_cSPS.getUseAMP() );
        //m_cSPS.setAMPAcc( i, 1 );
      }

      for (Int i = m_cSPS.getMaxCUDepth() - 1; i < m_cSPS.getMaxCUDepth(); i++)
      {
        m_cSPS.setAMPAcc( i, 0 );
      }
#endif

      // create ALF temporary buffer
      m_cAdaptiveLoopFilter.create( m_cSPS.getWidth(), m_cSPS.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
      m_cSAO.create( m_cSPS.getWidth(), m_cSPS.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#if !DISABLE_PARALLEL_DECISIONS
      m_cLoopFilter.create( m_cSPS.getWidth(), m_cSPS.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#else
      m_cLoopFilter.        create( g_uiMaxCUDepth );
#endif
      m_uiValidPS |= 1;
      
      return false;

    case NAL_UNIT_PPS:
#if G1002_RPS
      m_cPPS.setRPSList(&m_cRPSList);
#endif
      m_cPPS.setSPS(&m_cSPS);
      m_cEntropyDecoder.decodePPS( &m_cPPS );

      m_uiValidPS |= 2;
      return false;

    case NAL_UNIT_APS:
      {
        TComAPS  cAPS;
        allocAPS(&cAPS);
        decodeAPS(nalu.m_Bitstream, cAPS);
        pushAPS(cAPS);
      }
      return false;

    case NAL_UNIT_SEI:
      m_SEIs = new SEImessages;
      m_cEntropyDecoder.decodeSEI(*m_SEIs);
      return false;

    case NAL_UNIT_CODED_SLICE:
    case NAL_UNIT_CODED_SLICE_IDR:
    case NAL_UNIT_CODED_SLICE_CDR:
    {
      // make sure we already received both parameter sets
      assert( 3 == m_uiValidPS );
#if TILES_DECODER
      m_apcSlicePilot->setSPS(&m_cSPS);
      m_apcSlicePilot->initSlice();
#endif
      if (m_bFirstSliceInPicture)
      {
#if !TILES_DECODER
        m_apcSlicePilot->initSlice();
#endif
        m_uiSliceIdx     = 0;
        m_uiLastSliceIdx = 0;
      }
      m_apcSlicePilot->setSliceIdx(m_uiSliceIdx);

      //  Read slice header
      m_apcSlicePilot->setSPS( &m_cSPS );

      m_apcSlicePilot->setPPS( &m_cPPS );
      m_apcSlicePilot->setSliceIdx(m_uiSliceIdx);
      if (!m_bFirstSliceInPicture)
      {
#if TILES_DECODER || OL_USE_WPP
        m_apcSlicePilot->copySliceInfo( pcPic->getPicSym()->getSlice(m_uiSliceIdx-1) );
#else
        memcpy(m_apcSlicePilot, pcPic->getPicSym()->getSlice(m_uiSliceIdx-1), sizeof(TComSlice));
#endif
      }

      m_apcSlicePilot->setNalUnitType(nalu.m_UnitType);
      m_apcSlicePilot->setReferenced(nalu.m_RefIDC != NAL_REF_IDC_PRIORITY_LOWEST);
#if G1002_RPS
      m_apcSlicePilot->setTLayerInfo(nalu.m_TemporalID);
#endif
      m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot);
#if G220_PURE_VLC_SAO_ALF
      if(m_apcSlicePilot->isNextSlice())
      {
        if(m_cSPS.getUseALF())
        {
#if ALF_SAO_SLICE_FLAGS
          if (m_apcSlicePilot->getAlfEnabledFlag())
#else
          if(m_vAPS[m_apcSlicePilot->getAPSId()].back().getAlfEnabled())
#endif
          {
            m_cGopDecoder.decodeAlfOnOffCtrlParam();
          }
        }
      }
#if (TILES_DECODER || OL_USE_WPP)
      m_cEntropyDecoder.decodeWPPTileInfoToSliceHeader(m_apcSlicePilot);
#endif

#endif

#if !DISABLE_CAVLC
      if ( m_apcSlicePilot->getSymbolMode() )
#endif
      {
        Int numBitsForByteAlignment = nalu.m_Bitstream->getNumBitsUntilByteAligned();
        if ( numBitsForByteAlignment > 0 )
        {
          UInt bitsForByteAlignment;
          nalu.m_Bitstream->read( numBitsForByteAlignment, bitsForByteAlignment );
          assert( bitsForByteAlignment == ( ( 1 << numBitsForByteAlignment ) - 1 ) );
        }
      }

#if !G1002_RPS
      m_apcSlicePilot->setTLayerInfo(nalu.m_TemporalID);
#endif

      if (m_apcSlicePilot->isNextSlice() && m_apcSlicePilot->getPOC()!=m_uiPrevPOC && !m_bFirstSliceInSequence)
      {
        m_uiPrevPOC = m_apcSlicePilot->getPOC();
        return true;
      }
#if SCALING_LIST
#if G174_DF_OFFSET
      if(m_cSPS.getUseSAO() || m_cSPS.getUseALF()|| m_cSPS.getScalingListFlag() || m_cSPS.getUseDF())
#else
      if(m_cSPS.getUseSAO() || m_cSPS.getUseALF()|| m_cSPS.getScalingListFlag())
#endif
#else
      if(m_cSPS.getUseSAO() || m_cSPS.getUseALF())
#endif
      {
        m_apcSlicePilot->setAPS( popAPS(m_apcSlicePilot->getAPSId())  );
      }

      if (m_apcSlicePilot->isNextSlice()) 
        m_uiPrevPOC = m_apcSlicePilot->getPOC();
      m_bFirstSliceInSequence = false;
      if (m_apcSlicePilot->isNextSlice())
      {
        // Skip pictures due to random access
        if (isRandomAccessSkipPicture(iSkipFrame, iPOCLastDisplay))
        {
          return false;
        }
      }
#if G1002_RPS
      //detect lost reference picture and insert copy of earlier frame.
      while(m_apcSlicePilot->checkThatAllRefPicsAreAvailable(m_cListPic, m_apcSlicePilot->getRPS(), true) > 0)
      {
        xCreateLostPicture(m_apcSlicePilot->checkThatAllRefPicsAreAvailable(m_cListPic, m_apcSlicePilot->getRPS(), false)-1);
      }
#endif
      if (m_bFirstSliceInPicture)
      {
        // Buffer initialize for prediction.
        m_cPrediction.initTempBuff();
#if G1002_RPS
        m_apcSlicePilot->applyReferencePictureSet(m_cListPic, m_apcSlicePilot->getRPS());
#endif
        //  Get a new picture buffer
        xGetNewPicBuffer (m_apcSlicePilot, pcPic);
        
        /* transfer any SEI messages that have been received to the picture */
        pcPic->setSEIs(m_SEIs);
        m_SEIs = NULL;

        // Recursive structure
        m_cCuDecoder.create ( g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight );
        m_cCuDecoder.init   ( &m_cEntropyDecoder, &m_cTrQuant, &m_cPrediction );
        m_cTrQuant.init     ( g_uiMaxCUWidth, g_uiMaxCUHeight, m_apcSlicePilot->getSPS()->getMaxTrSize());
        
        m_cSliceDecoder.create( m_apcSlicePilot, m_apcSlicePilot->getSPS()->getWidth(), m_apcSlicePilot->getSPS()->getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
      }

      //  Set picture slice pointer
      TComSlice*  pcSlice = m_apcSlicePilot;
      Bool bNextSlice     = pcSlice->isNextSlice();

#if TILES
      UInt uiCummulativeTileWidth;
      UInt uiCummulativeTileHeight;
      UInt i, j, p;

#if NONCROSS_TILE_IN_LOOP_FILTERING
      //set the TileBoundaryIndependenceIdr
      if(pcSlice->getPPS()->getTileBehaviorControlPresentFlag() == 1)
      {
        pcPic->getPicSym()->setTileBoundaryIndependenceIdr( pcSlice->getPPS()->getTileBoundaryIndependenceIdr() );
      }
      else
      {
        pcPic->getPicSym()->setTileBoundaryIndependenceIdr( pcSlice->getPPS()->getSPS()->getTileBoundaryIndependenceIdr() );
      }
#endif

      if( pcSlice->getPPS()->getColumnRowInfoPresent() == 1 )
      {
#if !NONCROSS_TILE_IN_LOOP_FILTERING
        //set the TileBoundaryIndependenceIdr
        pcPic->getPicSym()->setTileBoundaryIndependenceIdr( pcSlice->getPPS()->getTileBoundaryIndependenceIdr() );
#endif
        //set NumColumnsMins1 and NumRowsMinus1
        pcPic->getPicSym()->setNumColumnsMinus1( pcSlice->getPPS()->getNumColumnsMinus1() );
        pcPic->getPicSym()->setNumRowsMinus1( pcSlice->getPPS()->getNumRowsMinus1() );

        //create the TComTileArray
        pcPic->getPicSym()->xCreateTComTileArray();

        if( pcSlice->getPPS()->getUniformSpacingIdr() == 1)
        {
          //set the width for each tile
          for(j=0; j < pcPic->getPicSym()->getNumRowsMinus1()+1; j++)
          {
            for(p=0; p < pcPic->getPicSym()->getNumColumnsMinus1()+1; p++)
            {
              pcPic->getPicSym()->getTComTile( j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p )->
                setTileWidth( (p+1)*pcPic->getPicSym()->getFrameWidthInCU()/(pcPic->getPicSym()->getNumColumnsMinus1()+1) 
                - (p*pcPic->getPicSym()->getFrameWidthInCU())/(pcPic->getPicSym()->getNumColumnsMinus1()+1) );
            }
          }

          //set the height for each tile
          for(j=0; j < pcPic->getPicSym()->getNumColumnsMinus1()+1; j++)
          {
            for(p=0; p < pcPic->getPicSym()->getNumRowsMinus1()+1; p++)
            {
              pcPic->getPicSym()->getTComTile( p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j )->
                setTileHeight( (p+1)*pcPic->getPicSym()->getFrameHeightInCU()/(pcPic->getPicSym()->getNumRowsMinus1()+1) 
                - (p*pcPic->getPicSym()->getFrameHeightInCU())/(pcPic->getPicSym()->getNumRowsMinus1()+1) );   
            }
          }
        }
        else
        {
          //set the width for each tile
          for(j=0; j < pcSlice->getPPS()->getNumRowsMinus1()+1; j++)
          {
            uiCummulativeTileWidth = 0;
            for(i=0; i < pcSlice->getPPS()->getNumColumnsMinus1(); i++)
            {
              pcPic->getPicSym()->getTComTile(j * (pcSlice->getPPS()->getNumColumnsMinus1()+1) + i)->setTileWidth( pcSlice->getPPS()->getColumnWidth(i) );
              uiCummulativeTileWidth += pcSlice->getPPS()->getColumnWidth(i);
            }
            pcPic->getPicSym()->getTComTile(j * (pcSlice->getPPS()->getNumColumnsMinus1()+1) + i)->setTileWidth( pcPic->getPicSym()->getFrameWidthInCU()-uiCummulativeTileWidth );
          }
  
          //set the height for each tile
          for(j=0; j < pcSlice->getPPS()->getNumColumnsMinus1()+1; j++)
          {
            uiCummulativeTileHeight = 0;
            for(i=0; i < pcSlice->getPPS()->getNumRowsMinus1(); i++)
            { 
              pcPic->getPicSym()->getTComTile(i * (pcSlice->getPPS()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcSlice->getPPS()->getRowHeight(i) );
              uiCummulativeTileHeight += pcSlice->getPPS()->getRowHeight(i);
            }
            pcPic->getPicSym()->getTComTile(i * (pcSlice->getPPS()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcPic->getPicSym()->getFrameHeightInCU()-uiCummulativeTileHeight );
          }
        }
      }
      else
      {
#if !NONCROSS_TILE_IN_LOOP_FILTERING
        //set the TileBoundaryIndependenceIdr
        pcPic->getPicSym()->setTileBoundaryIndependenceIdr( pcSlice->getSPS()->getTileBoundaryIndependenceIdr() );
#endif
        //set NumColumnsMins1 and NumRowsMinus1
        pcPic->getPicSym()->setNumColumnsMinus1( pcSlice->getSPS()->getNumColumnsMinus1() );
        pcPic->getPicSym()->setNumRowsMinus1( pcSlice->getSPS()->getNumRowsMinus1() );

        //create the TComTileArray
        pcPic->getPicSym()->xCreateTComTileArray();

        //automatically set the column and row boundary if UniformSpacingIdr = 1
        if( pcSlice->getSPS()->getUniformSpacingIdr() == 1 )
        {
          //set the width for each tile
          for(j=0; j < pcPic->getPicSym()->getNumRowsMinus1()+1; j++)
          {
            for(p=0; p < pcPic->getPicSym()->getNumColumnsMinus1()+1; p++)
            {
              pcPic->getPicSym()->getTComTile( j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p )->
                setTileWidth( (p+1)*pcPic->getPicSym()->getFrameWidthInCU()/(pcPic->getPicSym()->getNumColumnsMinus1()+1) 
                - (p*pcPic->getPicSym()->getFrameWidthInCU())/(pcPic->getPicSym()->getNumColumnsMinus1()+1) );
            }
          }

          //set the height for each tile
          for(j=0; j < pcPic->getPicSym()->getNumColumnsMinus1()+1; j++)
          {
            for(p=0; p < pcPic->getPicSym()->getNumRowsMinus1()+1; p++)
            {
              pcPic->getPicSym()->getTComTile( p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j )->
                setTileHeight( (p+1)*pcPic->getPicSym()->getFrameHeightInCU()/(pcPic->getPicSym()->getNumRowsMinus1()+1) 
                - (p*pcPic->getPicSym()->getFrameHeightInCU())/(pcPic->getPicSym()->getNumRowsMinus1()+1) );   
            }
          }
        }
        else
        {
          //set the width for each tile
          for(j=0; j < pcSlice->getSPS()->getNumRowsMinus1()+1; j++)
          {
            uiCummulativeTileWidth = 0;
            for(i=0; i < pcSlice->getSPS()->getNumColumnsMinus1(); i++)
            {
              pcPic->getPicSym()->getTComTile(j * (pcSlice->getSPS()->getNumColumnsMinus1()+1) + i)->setTileWidth( pcSlice->getSPS()->getColumnWidth(i) );
              uiCummulativeTileWidth += pcSlice->getSPS()->getColumnWidth(i);
            }
            pcPic->getPicSym()->getTComTile(j * (pcSlice->getSPS()->getNumColumnsMinus1()+1) + i)->setTileWidth( pcPic->getPicSym()->getFrameWidthInCU()-uiCummulativeTileWidth );
          }
  
          //set the height for each tile
          for(j=0; j < pcSlice->getSPS()->getNumColumnsMinus1()+1; j++)
          {
            uiCummulativeTileHeight = 0;
            for(i=0; i < pcSlice->getSPS()->getNumRowsMinus1(); i++)
            { 
              pcPic->getPicSym()->getTComTile(i * (pcSlice->getSPS()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcSlice->getSPS()->getRowHeight(i) );
              uiCummulativeTileHeight += pcSlice->getSPS()->getRowHeight(i);
            }
            pcPic->getPicSym()->getTComTile(i * (pcSlice->getSPS()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcPic->getPicSym()->getFrameHeightInCU()-uiCummulativeTileHeight );
          }
        }
      }

      pcPic->getPicSym()->xInitTiles();

      //generate the Coding Order Map and Inverse Coding Order Map
      UInt uiEncCUAddr;
      for(i=0, uiEncCUAddr=0; i<pcPic->getPicSym()->getNumberOfCUsInFrame(); i++, uiEncCUAddr = pcPic->getPicSym()->xCalculateNxtCUAddr(uiEncCUAddr))
      {
        pcPic->getPicSym()->setCUOrderMap(i, uiEncCUAddr);
        pcPic->getPicSym()->setInverseCUOrderMap(uiEncCUAddr, i);
      }
      pcPic->getPicSym()->setCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());
      pcPic->getPicSym()->setInverseCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());

      //convert the start and end CU addresses of the slice and entropy slice into encoding order
      pcSlice->setEntropySliceCurStartCUAddr( pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getEntropySliceCurStartCUAddr()) );
      pcSlice->setEntropySliceCurEndCUAddr( pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getEntropySliceCurEndCUAddr()) );
      if(pcSlice->isNextSlice())
      {
        pcSlice->setSliceCurStartCUAddr(pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getSliceCurStartCUAddr()));
        pcSlice->setSliceCurEndCUAddr(pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getSliceCurEndCUAddr()));
      }
#endif

      if (m_bFirstSliceInPicture) 
      {
        if(pcPic->getNumAllocatedSlice() != 1)
        {
          pcPic->clearSliceBuffer();
        }
      }
      else
      {
        pcPic->allocateNewSlice();
      }
      assert(pcPic->getNumAllocatedSlice() == (m_uiSliceIdx + 1));
      m_apcSlicePilot = pcPic->getPicSym()->getSlice(m_uiSliceIdx); 
      pcPic->getPicSym()->setSlice(pcSlice, m_uiSliceIdx);

      pcPic->setTLayer(nalu.m_TemporalID);

      if (bNextSlice)
      {
#if G1002_RPS && G1002_CRA_CHECK
        pcSlice->checkCRA(pcSlice->getRPS(), m_uiPOCCDR, m_cListPic); 
#else
        // Do decoding refresh marking if any
        pcSlice->decodingRefreshMarking(m_uiPOCCDR, m_bRefreshPending, m_cListPic);
#endif
        
#if NO_TMVP_MARKING
        if ( !pcSlice->getPPS()->getEnableTMVPFlag() && pcPic->getTLayer() == 0 )
        {
          pcSlice->decodingMarkingForNoTMVP( m_cListPic, pcSlice->getPOC() );
        }
#endif
        
        // Set reference list
        pcSlice->setRefPicList( m_cListPic );
        
        // HierP + GPB case
        if ( m_cSPS.getUseLDC() && pcSlice->isInterB() )
        {
          if(pcSlice->getRefPicListCombinationFlag() && (pcSlice->getNumRefIdx(REF_PIC_LIST_0) > pcSlice->getNumRefIdx(REF_PIC_LIST_1)))
          {
            for (Int iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_1); iRefIdx++)
            {
              pcSlice->setRefPic(pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx), REF_PIC_LIST_1, iRefIdx);
            }
          }
          else
          {
            Int iNumRefIdx = pcSlice->getNumRefIdx(REF_PIC_LIST_0);
            pcSlice->setNumRefIdx( REF_PIC_LIST_1, iNumRefIdx );
            
            for (Int iRefIdx = 0; iRefIdx < iNumRefIdx; iRefIdx++)
            {
              pcSlice->setRefPic(pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx), REF_PIC_LIST_1, iRefIdx);
            }
          }
        }
        
        // For generalized B
        // note: maybe not existed case (always L0 is copied to L1 if L1 is empty)
        if (pcSlice->isInterB() && pcSlice->getNumRefIdx(REF_PIC_LIST_1) == 0)
        {
          Int iNumRefIdx = pcSlice->getNumRefIdx(REF_PIC_LIST_0);
          pcSlice->setNumRefIdx        ( REF_PIC_LIST_1, iNumRefIdx );
          
          for (Int iRefIdx = 0; iRefIdx < iNumRefIdx; iRefIdx++)
          {
            pcSlice->setRefPic(pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx), REF_PIC_LIST_1, iRefIdx);
          }
        }
        if (pcSlice->isInterB())
        {
          Bool bLowDelay = true;
          Int  iCurrPOC  = pcSlice->getPOC();
          Int iRefIdx = 0;

          for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; iRefIdx++)
          {
            if ( pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx)->getPOC() > iCurrPOC )
            {
              bLowDelay = false;
            }
          }
          for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; iRefIdx++)
          {
            if ( pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx)->getPOC() > iCurrPOC )
            {
              bLowDelay = false;
            }
          }

          pcSlice->setCheckLDC(bLowDelay);            
        }
        
        //---------------
        pcSlice->setRefPOCList();
        
        if(!pcSlice->getRefPicListModificationFlagLC())
        {
          pcSlice->generateCombinedList();
        }

#if WP_IMPROVED_SYNTAX
        if( pcSlice->getRefPicListCombinationFlag() && pcSlice->getPPS()->getWPBiPredIdc()==1 && pcSlice->getSliceType()==B_SLICE )
        {
          pcSlice->setWpParamforLC();
        }
#endif
        pcSlice->setNoBackPredFlag( false );
        if ( pcSlice->getSliceType() == B_SLICE && !pcSlice->getRefPicListCombinationFlag())
        {
          if ( pcSlice->getNumRefIdx(RefPicList( 0 ) ) == pcSlice->getNumRefIdx(RefPicList( 1 ) ) )
          {
            pcSlice->setNoBackPredFlag( true );
#if !TILES
            Int i;
#endif
            for ( i=0; i < pcSlice->getNumRefIdx(RefPicList( 1 ) ); i++ )
            {
              if ( pcSlice->getRefPOC(RefPicList(1), i) != pcSlice->getRefPOC(RefPicList(0), i) ) 
              {
                pcSlice->setNoBackPredFlag( false );
                break;
              }
            }
          }
        }
      }
      
      pcPic->setCurrSliceIdx(m_uiSliceIdx);
#if SCALING_LIST
      if(pcSlice->getSPS()->getScalingListFlag())
      {
        if(pcSlice->getAPS()->getScalingListEnabled())
        {
          pcSlice->setScalingList ( &m_scalingList  );
          if(pcSlice->getScalingList()->getUseDefaultOnlyFlag())
          {
            pcSlice->setDefaultScalingList();
          }
          m_cTrQuant.setScalingListDec(pcSlice->getScalingList());
        }
        m_cTrQuant.setUseScalingList(true);
      }
      else
      {
        m_cTrQuant.setFlatScalingList();
        m_cTrQuant.setUseScalingList(false);
      }
#endif

      //  Decode a picture
#if G1002_RPS
      m_cGopDecoder.decompressGop(nalu.m_Bitstream, pcPic, false);
#else
#if REF_SETTING_FOR_LD
      m_cGopDecoder.decompressGop(nalu.m_Bitstream, pcPic, false, m_cListPic );
#else
      m_cGopDecoder.decompressGop(nalu.m_Bitstream, pcPic, false);
#endif
#endif

      m_bFirstSliceInPicture = false;
      m_uiSliceIdx++;
    }
      break;
    default:
      assert (1);
  }

  return false;
}

/** Function for checking if picture should be skipped because of random access
 * \param iSkipFrame skip frame counter
 * \param iPOCLastDisplay POC of last picture displayed
 * \returns true if the picture shold be skipped in the random access.
 * This function checks the skipping of pictures in the case of -s option random access.
 * All pictures prior to the random access point indicated by the counter iSkipFrame are skipped.
 * It also checks the type of Nal unit type at the random access point.
 * If the random access point is CDR, pictures with POC equal to or greater than the CDR POC are decoded.
 * If the random access point is IDR all pictures after the random access point are decoded.
 * If the random access point is not IDR or CDR, a warning is issues, and decoding of pictures with POC 
 * equal to or greater than the random access point POC is attempted. For non IDR/CDR random 
 * access point there is no guarantee that the decoder will not crash.
 */
Bool TDecTop::isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay)
{
  if (iSkipFrame) 
  {
    iSkipFrame--;   // decrement the counter
    return true;
  }
  else if (m_uiPOCRA == MAX_UINT) // start of random access point, m_uiPOCRA has not been set yet.
  {
    if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CDR)
    {
      m_uiPOCRA = m_apcSlicePilot->getPOC(); // set the POC random access since we need to skip the reordered pictures in CDR.
    }
    else if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR)
    {
      m_uiPOCRA = 0; // no need to skip the reordered pictures in IDR, they are decodable.
    }
    else 
    {
      printf("\nUnsafe random access point. Decoder may crash.");
      m_uiPOCRA = m_apcSlicePilot->getPOC(); // set the POC random access skip the reordered pictures and try to decode if possible.  This increases the chances of avoiding a decoder crash.
      //m_uiPOCRA = 0;
    }
  }
  else if (m_apcSlicePilot->getPOC() < m_uiPOCRA)  // skip the reordered pictures if necessary
  {
    iPOCLastDisplay++;
    return true;
  }
  // if we reach here, then the picture is not skipped.
  return false; 
}

/** decode APS syntax elements
 * \param [in] bs bistream pointer
 * \param [in,out] cAPS APS object to store the decoded results
 */
Void TDecTop::decodeAPS(TComInputBitstream* bs, TComAPS& cAPS)
{
#if !G220_PURE_VLC_SAO_ALF
  Int iBitLeft;
#endif
  m_cEntropyDecoder.decodeAPSInitInfo(cAPS);
#if SCALING_LIST
  if(cAPS.getScalingListEnabled())
  {
    m_cEntropyDecoder.decodeScalingList( &m_scalingList );
  }
#endif
#if G174_DF_OFFSET
  if(cAPS.getLoopFilterOffsetInAPS())
  {
    m_cEntropyDecoder.decodeDFParams( &cAPS );    
  }
#endif

  if(cAPS.getSaoEnabled())
  {
    cAPS.getSaoParam()->bSaoFlag[0] = true;
#if !G220_PURE_VLC_SAO_ALF
    //read SAO bitstream length in byte
    UInt uiBsLength = bs->read(APS_BITS_FOR_SAO_BYTE_LENGTH);
    assert(uiBsLength > 0);

    //read byte-alignment bits
    Int numBitsForByteAlignment = bs->getNumBitsUntilByteAligned();
    if ( numBitsForByteAlignment > 0 )
    {
      UInt bitsForByteAlignment;
      bs->read( numBitsForByteAlignment, bitsForByteAlignment );
      assert( bitsForByteAlignment == ( ( 1 << numBitsForByteAlignment ) - 1 ) );
    }

    iBitLeft = bs->getNumBitsLeft();

    if (cAPS.getCABACForAPS())
    {
      m_cSbacDecoder.init((TDecBinIf*)(&m_cBinCABAC));
      m_cEntropyDecoder.setEntropyDecoder(&m_cSbacDecoder);
      m_cEntropyDecoder.setBitstream(bs);
      m_cEntropyDecoder.resetEntropy(cAPS.getCABACinitQP(), cAPS.getCABACinitIDC());
    }
    else
    {
      m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
      m_cEntropyDecoder.setBitstream(bs);
    }
#endif
    m_cEntropyDecoder.decodeSaoParam( cAPS.getSaoParam());
#if !G220_PURE_VLC_SAO_ALF
    iBitLeft = bs->getNumBitsLeft() - (iBitLeft - (uiBsLength << 3));
    assert(iBitLeft >= 0);
    if(iBitLeft) bs->read(iBitLeft); //garbage bits. 
    //else  trailing bits
#endif
  }

  if(cAPS.getAlfEnabled())
  {
    cAPS.getAlfParam()->alf_flag = 1;
#if !G220_PURE_VLC_SAO_ALF
    //read ALF bitstream length in byte
    UInt uiBsLength = bs->read(APS_BITS_FOR_ALF_BYTE_LENGTH);
    assert(uiBsLength > 0);

    //read byte-alignment bits
    Int numBitsForByteAlignment = bs->getNumBitsUntilByteAligned();
    if ( numBitsForByteAlignment > 0 )
    {
      UInt bitsForByteAlignment;
      bs->read( numBitsForByteAlignment, bitsForByteAlignment );
      assert( bitsForByteAlignment == ( ( 1 << numBitsForByteAlignment ) - 1 ) );
    }

    if (cAPS.getCABACForAPS())
    {
      m_cSbacDecoder.init((TDecBinIf*)(&m_cBinCABAC));
      m_cEntropyDecoder.setEntropyDecoder(&m_cSbacDecoder);
      m_cEntropyDecoder.setBitstream(bs);
      m_cEntropyDecoder.resetEntropy(cAPS.getCABACinitQP(), cAPS.getCABACinitIDC());
    }
    else
    {
      m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
      m_cEntropyDecoder.setBitstream(bs);
    }
#endif
    m_cEntropyDecoder.decodeAlfParam( cAPS.getAlfParam());
  }

}

/** Pop APS object pointer from APS container
 * \param [in] APS ID
 * \return APS pointer
 */

TComAPS* TDecTop::popAPS (UInt apsID)
{
  std::vector<TComAPS>& vAPS = m_vAPS[apsID];

  if(vAPS.size() > 1)
  {
    for(Int i=0; i< vAPS.size()-1; i++)
    {
      freeAPS(&vAPS[0]);
    }
    vAPS.erase(vAPS.begin(), vAPS.end()-1);
  }

  assert(vAPS.size() == 1);

  //pop APS link
  return &(vAPS.front());

}

/** Push APS object into APS container
 * \param [in] cAPS APS object to be pushed.
 */

Void TDecTop::pushAPS  (TComAPS& cAPS)
{
  UInt apsID = cAPS.getAPSID();
  assert(apsID < MAX_NUM_SUPPORTED_APS);

  std::vector<TComAPS>& vAPS = m_vAPS[apsID];

  if(vAPS.size() < APS_RESERVED_BUFFER_SIZE)
  {
    vAPS.push_back(cAPS);
  }
  else
  {
    vAPS[APS_RESERVED_BUFFER_SIZE-1] = cAPS;
  }

}


Void TDecTop::allocAPS (TComAPS* pAPS)
{
#if SCALING_LIST
  if(m_cSPS.getScalingListFlag())
  {
    pAPS->createScalingList();
  }
#endif
  if(m_cSPS.getUseSAO())
  {
    pAPS->createSaoParam();
    m_cSAO.allocSaoParam(pAPS->getSaoParam());
  }
  if(m_cSPS.getUseALF())
  {
    pAPS->createAlfParam();
    m_cAdaptiveLoopFilter.allocALFParam(pAPS->getAlfParam());
  }
}
Void TDecTop::freeAPS (TComAPS* pAPS)
{
#if SCALING_LIST
  if(m_cSPS.getScalingListFlag())
  {
    pAPS->destroyScalingList();
  }
#endif
  if(m_cSPS.getUseSAO())
  {
    m_cSAO.freeSaoParam(pAPS->getSaoParam());
    pAPS->destroySaoParam();
  }
  if(m_cSPS.getUseALF())
  {
    m_cAdaptiveLoopFilter.freeALFParam(pAPS->getAlfParam());
    pAPS->destroyAlfParam();
  }
}


//! \}
