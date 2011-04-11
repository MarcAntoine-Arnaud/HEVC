/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.   
 *
 * Copyright (c) 2010-2011, ITU/ISO/IEC
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

/** \file     TDecGop.cpp
    \brief    GOP decoder class
*/

#include "TDecGop.h"
#include "TDecCAVLC.h"
#include "TDecSbac.h"
#include "TDecBinCoder.h"
#include "TDecBinCoderCABAC.h"

#include <time.h>

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TDecGop::TDecGop()
{
  m_iGopSize = 0;
  m_dDecTime = 0;
}

TDecGop::~TDecGop()
{
  
}

Void TDecGop::create()
{
  
}


Void TDecGop::destroy()
{

}

Void TDecGop::init( TDecEntropy*            pcEntropyDecoder, 
                   TDecSbac*               pcSbacDecoder, 
                   TDecBinCABAC*           pcBinCABAC,
                   TDecCavlc*              pcCavlcDecoder, 
                   TDecSlice*              pcSliceDecoder, 
                   TComLoopFilter*         pcLoopFilter, 
                   TComAdaptiveLoopFilter* pcAdaptiveLoopFilter 
#if MTK_SAO
                   ,TComSAO*                pcSAO
#endif                   
                   )
{
  m_pcEntropyDecoder      = pcEntropyDecoder;
  m_pcSbacDecoder         = pcSbacDecoder;
  m_pcBinCABAC            = pcBinCABAC;
  m_pcCavlcDecoder        = pcCavlcDecoder;
  m_pcSliceDecoder        = pcSliceDecoder;
  m_pcLoopFilter          = pcLoopFilter;
  m_pcAdaptiveLoopFilter  = pcAdaptiveLoopFilter;
#if MTK_SAO
  m_pcSAO  = pcSAO;
#endif

}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TDecGop::decompressGop (Bool bEos, TComBitstream* pcBitstream, TComPic*& rpcPic, Bool bExecuteDeblockAndAlf)
{
  TComSlice*  pcSlice = rpcPic->getSlice(rpcPic->getCurrSliceIdx());

  //-- For time output for each slice
  long iBeforeTime = clock();
  
  UInt uiStartCUAddr   = pcSlice->getEntropySliceCurStartCUAddr();
#if MTK_NONCROSS_INLOOP_FILTER
  static Bool  bFirst = true;
  static UInt  uiILSliceCount;
  static UInt* puiILSliceStartLCU;
#if MTK_SAO
  SAOParam cSaoParam;
#endif
  if (!bExecuteDeblockAndAlf)
  {
    if(bFirst)
    {
      uiILSliceCount = 0;
      if(!pcSlice->getSPS()->getLFCrossSliceBoundaryFlag())
      {
        puiILSliceStartLCU = new UInt[rpcPic->getNumCUsInFrame() +1];
      }
      bFirst = false;
    }
    
    if(!pcSlice->getSPS()->getLFCrossSliceBoundaryFlag())
    {
      UInt uiSliceStartCuAddr = pcSlice->getSliceCurStartCUAddr();
      if(uiSliceStartCuAddr == uiStartCUAddr)
      {
        puiILSliceStartLCU[uiILSliceCount] = uiSliceStartCuAddr;
        uiILSliceCount++;
      }
    }
#endif //MTK_NONCROSS_INLOOP_FILTER

    UInt iSymbolMode = pcSlice->getSymbolMode();
    if (iSymbolMode)
    {
      m_pcSbacDecoder->init( (TDecBinIf*)m_pcBinCABAC );
      m_pcEntropyDecoder->setEntropyDecoder (m_pcSbacDecoder);
    }
    else
    {
      m_pcEntropyDecoder->setEntropyDecoder (m_pcCavlcDecoder);
    }
    
    m_pcEntropyDecoder->setBitstream      (pcBitstream);
    m_pcEntropyDecoder->resetEntropy      (pcSlice);
    
    if (uiStartCUAddr==0)  // decode ALF params only from first slice header
    {
#if MTK_SAO
      if( rpcPic->getSlice(0)->getSPS()->getUseSAO() )
      {  
        m_pcSAO->allocSaoParam(&cSaoParam);
        m_pcSAO->InitSao(&cSaoParam);
        m_pcEntropyDecoder->decodeSaoParam(&cSaoParam);
      }
#endif

      if ( rpcPic->getSlice(0)->getSPS()->getUseALF() )
      {
#if TSB_ALF_HEADER
        m_pcAdaptiveLoopFilter->setNumCUsInFrame(rpcPic);
#endif
        m_pcAdaptiveLoopFilter->allocALFParam(&m_cAlfParam);
        m_pcEntropyDecoder->decodeAlfParam( &m_cAlfParam );
      }
    }
    
    m_pcSliceDecoder->decompressSlice(pcBitstream, rpcPic);
    
    m_dDecTime += (double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;
  }
  else
  {
    // deblocking filter
    m_pcLoopFilter->setCfg(pcSlice->getLoopFilterDisable(), 0, 0);
    m_pcLoopFilter->loopFilterPic( rpcPic );
#if MTK_SAO
    {
      if( rpcPic->getSlice(0)->getSPS()->getUseSAO())
      {
        m_pcSAO->SAOProcess(rpcPic, &cSaoParam);
      }
    }
#endif
    // adaptive loop filter
    if( pcSlice->getSPS()->getUseALF() )
    {
#if MTK_NONCROSS_INLOOP_FILTER  
      if(pcSlice->getSPS()->getLFCrossSliceBoundaryFlag())
      {
        m_pcAdaptiveLoopFilter->setUseNonCrossAlf(false);
      }
      else
      {
        puiILSliceStartLCU[uiILSliceCount] = rpcPic->getNumCUsInFrame();
        m_pcAdaptiveLoopFilter->setUseNonCrossAlf( (uiILSliceCount > 1) );
        if(m_pcAdaptiveLoopFilter->getUseNonCrossAlf())
        {
          m_pcAdaptiveLoopFilter->setNumSlicesInPic( uiILSliceCount );
          m_pcAdaptiveLoopFilter->createSlice();
          for(UInt i=0; i< uiILSliceCount ; i++)
          {
            (*m_pcAdaptiveLoopFilter)[i].create(rpcPic, i, puiILSliceStartLCU[i], puiILSliceStartLCU[i+1]-1);
          }
        }
      }
#endif
      m_pcAdaptiveLoopFilter->ALFProcess(rpcPic, &m_cAlfParam);
#if MTK_NONCROSS_INLOOP_FILTER
      if(m_pcAdaptiveLoopFilter->getUseNonCrossAlf())
      {
        m_pcAdaptiveLoopFilter->destroySlice();
      }
#endif
      m_pcAdaptiveLoopFilter->freeALFParam(&m_cAlfParam);
    }
    
#if AMVP_BUFFERCOMPRESS
    rpcPic->compressMotion(); 
#endif 
    
    //-- For time output for each slice
    printf("\nPOC %4d ( %c-SLICE, QP%3d ) ",
           pcSlice->getPOC(),
           pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B',
           pcSlice->getSliceQp() );
    
    m_dDecTime += (double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;
    printf ("[DT %6.3f] ", m_dDecTime );
    m_dDecTime  = 0;
    
    for (Int iRefList = 0; iRefList < 2; iRefList++)
    {
      printf ("[L%d ", iRefList);
      for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(RefPicList(iRefList)); iRefIndex++)
      {
        printf ("%d ", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex));
      }
      printf ("] ");
    }
#if DCM_COMB_LIST
    if(pcSlice->getNumRefIdx(REF_PIC_LIST_C)>0 && !pcSlice->getNoBackPredFlag())
    {
      printf ("[LC ");
      for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(REF_PIC_LIST_C); iRefIndex++)
      {
        printf ("%d ", pcSlice->getRefPOC((RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex)));
      }
      printf ("] ");
    }
#endif
#if FIXED_ROUNDING_FRAME_MEMORY
    rpcPic->getPicYuvRec()->xFixedRoundingPic();
#endif 
    rpcPic->setReconMark(true);
#if MTK_NONCROSS_INLOOP_FILTER
    uiILSliceCount = 0;
#endif
  }
}

