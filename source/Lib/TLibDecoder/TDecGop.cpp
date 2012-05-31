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

/** \file     TDecGop.cpp
    \brief    GOP decoder class
*/

extern bool g_md5_mismatch; ///< top level flag to signal when there is a decode problem

#include "TDecGop.h"
#include "TDecCAVLC.h"
#include "TDecSbac.h"
#include "TDecBinCoder.h"
#include "TDecBinCoderCABAC.h"
#include "libmd5/MD5.h"
#include "TLibCommon/SEI.h"

#include <time.h>

//! \ingroup TLibDecoder
//! \{
#if HASH_TYPE
static void calcAndPrintHashStatus(TComPicYuv& pic, const SEImessages* seis);
#else
static void calcAndPrintMD5Status(TComPicYuv& pic, const SEImessages* seis);
#endif
// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TDecGop::TDecGop()
{
  m_iGopSize = 0;
  m_dDecTime = 0;
  m_pcSbacDecoders = NULL;
  m_pcBinCABACs = NULL;
}

TDecGop::~TDecGop()
{
  
}

Void TDecGop::create()
{
  
}


Void TDecGop::destroy()
{
#if !AHG6_ALF_OPTION2
  m_alfParamSetPilot.releaseALFParam();
#endif
}

Void TDecGop::init( TDecEntropy*            pcEntropyDecoder, 
                   TDecSbac*               pcSbacDecoder, 
                   TDecBinCABAC*           pcBinCABAC,
                   TDecCavlc*              pcCavlcDecoder, 
                   TDecSlice*              pcSliceDecoder, 
                   TComLoopFilter*         pcLoopFilter, 
                   TComAdaptiveLoopFilter* pcAdaptiveLoopFilter 
                   ,TComSampleAdaptiveOffset* pcSAO
                   )
{
  m_pcEntropyDecoder      = pcEntropyDecoder;
  m_pcSbacDecoder         = pcSbacDecoder;
  m_pcBinCABAC            = pcBinCABAC;
  m_pcCavlcDecoder        = pcCavlcDecoder;
  m_pcSliceDecoder        = pcSliceDecoder;
  m_pcLoopFilter          = pcLoopFilter;
  m_pcAdaptiveLoopFilter  = pcAdaptiveLoopFilter;
  m_pcSAO  = pcSAO;
}


// ====================================================================================================================
// Private member functions
// ====================================================================================================================
#if !AHG6_ALF_OPTION2
Void TDecGop::patchAlfLCUParams(ALFParam*** alfLCUParam, AlfParamSet* alfParamSet, Int firstLCUAddr)
{
  Int numLCUInWidth = alfParamSet->numLCUInWidth;
  Int numLCU        = alfParamSet->numLCU;

  Int rx, ry, pos, posUp;
  std::vector<ALFParam*> storedFilters[NUM_ALF_COMPONENT];
  storedFilters[ALF_Y].clear();
  storedFilters[ALF_Cb].clear();
  storedFilters[ALF_Cr].clear();

  for(Int i=0; i< numLCU; i++)
  {
    rx     = (i+ firstLCUAddr)% numLCUInWidth;
    ry     = (i+ firstLCUAddr)/ numLCUInWidth;
    pos    = (ry*numLCUInWidth) + rx;
    posUp  = pos-numLCUInWidth;

    for(Int compIdx =0; compIdx < NUM_ALF_COMPONENT; compIdx++)
    {
      AlfUnitParam& alfUnitParam = alfParamSet->alfUnitParam[compIdx][i];
      ALFParam&     alfFiltParam = *(alfLCUParam[compIdx][pos]);

      switch( alfUnitParam.mergeType )
      {
      case ALF_MERGE_DISABLED:
        {
          if(alfUnitParam.isEnabled)
          {
            if(alfUnitParam.isNewFilt)
            {
              alfFiltParam = *alfUnitParam.alfFiltParam;
              storedFilters[compIdx].push_back( &alfFiltParam );
            }
            else //stored filter
            {
              alfFiltParam = *(storedFilters[compIdx][alfUnitParam.storedFiltIdx]);
              assert(alfFiltParam.alf_flag == 1);
            }
          }
          else
          {
            alfFiltParam.alf_flag = 0;
          }
        }
        break;
      case ALF_MERGE_UP:
        {
          assert(posUp >= 0);
          alfFiltParam = *(alfLCUParam[compIdx][posUp]);
        }
        break;
      case ALF_MERGE_LEFT:
        {
          assert(pos-1 >= 0);
          alfFiltParam = *(alfLCUParam[compIdx][pos-1]);
        }
        break;
      case ALF_MERGE_FIRST:
        {
          alfFiltParam = *(alfLCUParam[compIdx][firstLCUAddr]);
        }
        break;
      default:
        {
          printf("not a supported ALF merge type\n");
          assert(0);
          exit(-1);
        }
      }
    } //compIdx
  } //i (LCU)
}
#endif
// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TDecGop::decompressGop(TComInputBitstream* pcBitstream, TComPic*& rpcPic, Bool bExecuteDeblockAndAlf)
{
  TComSlice*  pcSlice = rpcPic->getSlice(rpcPic->getCurrSliceIdx());
  // Table of extracted substreams.
  // These must be deallocated AND their internal fifos, too.
  TComInputBitstream **ppcSubstreams = NULL;

  //-- For time output for each slice
  long iBeforeTime = clock();
  
  UInt uiStartCUAddr   = pcSlice->getDependentSliceCurStartCUAddr();
  static Bool  bFirst = true;
  static UInt  uiILSliceCount;
  static UInt* puiILSliceStartLCU;
#if AHG6_ALF_OPTION2
  static std::vector<Bool> sliceAlfEnabled[3];
#else
  static std::vector<AlfCUCtrlInfo> vAlfCUCtrlSlices;
#endif
#if H0391_LF_ACROSS_SLICE_BOUNDARY_CONTROL
  static std::vector<Bool> LFCrossSliceBoundaryFlag;
#endif

  if (!bExecuteDeblockAndAlf)
  {
    if(bFirst)
    {
      uiILSliceCount = 0;
      puiILSliceStartLCU = new UInt[(rpcPic->getNumCUsInFrame()* rpcPic->getNumPartInCU()) +1];
      bFirst = false;
    }

    UInt uiSliceStartCuAddr = pcSlice->getSliceCurStartCUAddr();
    if(uiSliceStartCuAddr == uiStartCUAddr)
    {
      puiILSliceStartLCU[uiILSliceCount] = uiSliceStartCuAddr;
      uiILSliceCount++;
    }

    m_pcSbacDecoder->init( (TDecBinIf*)m_pcBinCABAC );
    m_pcEntropyDecoder->setEntropyDecoder (m_pcSbacDecoder);
    
    UInt uiNumSubstreams = pcSlice->getPPS()->getNumSubstreams();

    //init each couple {EntropyDecoder, Substream}
    UInt *puiSubstreamSizes = pcSlice->getSubstreamSizes();
    ppcSubstreams    = new TComInputBitstream*[uiNumSubstreams];
    m_pcSbacDecoders = new TDecSbac[uiNumSubstreams];
    m_pcBinCABACs    = new TDecBinCABAC[uiNumSubstreams];
    UInt uiBitsRead = pcBitstream->getByteLocation()<<3;
    for ( UInt ui = 0 ; ui < uiNumSubstreams ; ui++ )
    {
      m_pcSbacDecoders[ui].init(&m_pcBinCABACs[ui]);
      UInt uiSubstreamSizeBits = (ui+1 < uiNumSubstreams ? puiSubstreamSizes[ui] : pcBitstream->getNumBitsLeft());
      ppcSubstreams[ui] = pcBitstream->extractSubstream(ui+1 < uiNumSubstreams ? puiSubstreamSizes[ui] : pcBitstream->getNumBitsLeft());
      // update location information from where tile markers were extracted
      {
        UInt uiDestIdx       = 0;
        for (UInt uiSrcIdx = 0; uiSrcIdx<pcBitstream->getTileMarkerLocationCount(); uiSrcIdx++)
        {
          UInt uiLocation = pcBitstream->getTileMarkerLocation(uiSrcIdx);
          if ((uiBitsRead>>3)<=uiLocation  &&  uiLocation<((uiBitsRead+uiSubstreamSizeBits)>>3))
          {
            ppcSubstreams[ui]->setTileMarkerLocation( uiDestIdx, uiLocation - (uiBitsRead>>3) );
            ppcSubstreams[ui]->setTileMarkerLocationCount( uiDestIdx+1 );
            uiDestIdx++;
          }
        }
        ppcSubstreams[ui]->setTileMarkerLocationCount( uiDestIdx );
        uiBitsRead += uiSubstreamSizeBits;
      }
    }

    for ( UInt ui = 0 ; ui+1 < uiNumSubstreams; ui++ )
    {
      m_pcEntropyDecoder->setEntropyDecoder ( &m_pcSbacDecoders[uiNumSubstreams - 1 - ui] );
      m_pcEntropyDecoder->setBitstream      (  ppcSubstreams   [uiNumSubstreams - 1 - ui] );
      m_pcEntropyDecoder->resetEntropy      (pcSlice);
    }

    m_pcEntropyDecoder->setEntropyDecoder ( m_pcSbacDecoder  );
    m_pcEntropyDecoder->setBitstream      ( ppcSubstreams[0] );
    m_pcEntropyDecoder->resetEntropy      (pcSlice);

    if(uiSliceStartCuAddr == uiStartCUAddr)
    {
#if H0391_LF_ACROSS_SLICE_BOUNDARY_CONTROL
      LFCrossSliceBoundaryFlag.push_back( pcSlice->getLFCrossSliceBoundaryFlag());
#endif
      if(pcSlice->getSPS()->getUseALF())
      {
#if AHG6_ALF_OPTION2
        for(Int compIdx=0; compIdx < 3; compIdx++)
        {
          sliceAlfEnabled[compIdx].push_back(  pcSlice->getAlfEnabledFlag(compIdx) );
        }
#else
        if(pcSlice->getAlfEnabledFlag())
        {
          if(pcSlice->getSPS()->getUseALFCoefInSlice())
          {
            Int numSUinLCU    = 1<< (g_uiMaxCUDepth << 1); 
            Int firstLCUAddr   = pcSlice->getSliceCurStartCUAddr() / numSUinLCU;  
            patchAlfLCUParams(m_pcAdaptiveLoopFilter->getAlfLCUParam(), &m_alfParamSetPilot, firstLCUAddr);
          }

          if( !pcSlice->getSPS()->getUseALFCoefInSlice())
          {
          vAlfCUCtrlSlices.push_back(m_cAlfCUCtrlOneSlice);
          }
        }
#endif
      }
    }
#if DEPENDENT_SLICES
    if( pcSlice->getPPS()->getDependentSlicesEnabledFlag() && (!pcSlice->getPPS()->getCabacIndependentFlag()) )
    {
      pcSlice->initCTXMem_dec( 2 );
      for ( UInt st = 0; st < 2; st++ )
      {
        TDecSbac* ctx = NULL;
        ctx = new TDecSbac;
        ctx->init( (TDecBinIf*)m_pcBinCABAC );
        ctx->load( m_pcSbacDecoder );
        pcSlice->setCTXMem_dec( ctx, st );
      }
    }
#endif

    m_pcSbacDecoders[0].load(m_pcSbacDecoder);
    m_pcSliceDecoder->decompressSlice( pcBitstream, ppcSubstreams, rpcPic, m_pcSbacDecoder, m_pcSbacDecoders);
    m_pcEntropyDecoder->setBitstream(  ppcSubstreams[uiNumSubstreams-1] );
    // deallocate all created substreams, including internal buffers.
    for (UInt ui = 0; ui < uiNumSubstreams; ui++)
    {
      ppcSubstreams[ui]->deleteFifo();
      delete ppcSubstreams[ui];
    }
    delete[] ppcSubstreams;
    delete[] m_pcSbacDecoders; m_pcSbacDecoders = NULL;
    delete[] m_pcBinCABACs; m_pcBinCABACs = NULL;

    m_dDecTime += (double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;
  }
  else
  {
    // deblocking filter
#if !TILES_OR_ENTROPY_FIX
    Bool bLFCrossTileBoundary = (pcSlice->getPPS()->getTileBehaviorControlPresentFlag() == 1)?
                                (pcSlice->getPPS()->getLFCrossTileBoundaryFlag()):(pcSlice->getPPS()->getSPS()->getLFCrossTileBoundaryFlag());
#else
    Bool bLFCrossTileBoundary = pcSlice->getPPS()->getLFCrossTileBoundaryFlag();
#endif
    if (pcSlice->getPPS()->getDeblockingFilterControlPresent())
    {
#if DBL_HL_SYNTAX
      if(pcSlice->getPPS()->getLoopFilterOffsetInPPS())
      {
        pcSlice->setLoopFilterDisable(pcSlice->getPPS()->getLoopFilterDisable());
        if (!pcSlice->getLoopFilterDisable())
        {
          pcSlice->setLoopFilterBetaOffset(pcSlice->getPPS()->getLoopFilterBetaOffset());
          pcSlice->setLoopFilterTcOffset(pcSlice->getPPS()->getLoopFilterTcOffset());
        }
      }
#else
      if(pcSlice->getSPS()->getUseDF())
      {
        if(pcSlice->getInheritDblParamFromAPS())
        {
          pcSlice->setLoopFilterDisable(pcSlice->getAPS()->getLoopFilterDisable());
          if (!pcSlice->getLoopFilterDisable())
          {
            pcSlice->setLoopFilterBetaOffset(pcSlice->getAPS()->getLoopFilterBetaOffset());
            pcSlice->setLoopFilterTcOffset(pcSlice->getAPS()->getLoopFilterTcOffset());
          }
        }
      }
#endif
    }
    m_pcLoopFilter->setCfg(pcSlice->getPPS()->getDeblockingFilterControlPresent(), pcSlice->getLoopFilterDisable(), pcSlice->getLoopFilterBetaOffset(), pcSlice->getLoopFilterTcOffset(), bLFCrossTileBoundary);
    m_pcLoopFilter->loopFilterPic( rpcPic );

    pcSlice = rpcPic->getSlice(0);
    if(pcSlice->getSPS()->getUseSAO() || pcSlice->getSPS()->getUseALF())
    {
      Int sliceGranularity = pcSlice->getPPS()->getSliceGranularity();
      puiILSliceStartLCU[uiILSliceCount] = rpcPic->getNumCUsInFrame()* rpcPic->getNumPartInCU();
#if H0391_LF_ACROSS_SLICE_BOUNDARY_CONTROL
      rpcPic->createNonDBFilterInfo(puiILSliceStartLCU, uiILSliceCount,sliceGranularity,&LFCrossSliceBoundaryFlag,rpcPic->getPicSym()->getNumTiles() ,bLFCrossTileBoundary);
#else
      rpcPic->createNonDBFilterInfo(puiILSliceStartLCU, uiILSliceCount,sliceGranularity,pcSlice->getSPS()->getLFCrossSliceBoundaryFlag(),rpcPic->getPicSym()->getNumTiles() ,bLFCrossTileBoundary);
#endif
    }

    if( pcSlice->getSPS()->getUseSAO() )
    {
      if(pcSlice->getSaoEnabledFlag())
      {
#if !SAO_REMOVE_APS
        if (pcSlice->getSaoInterleavingFlag())
#endif
        {
#if !SAO_REMOVE_APS // APS syntax
          pcSlice->getAPS()->setSaoInterleavingFlag(pcSlice->getSaoInterleavingFlag());
          pcSlice->getAPS()->setSaoEnabled(pcSlice->getSaoEnabledFlag());
#endif
          pcSlice->getAPS()->getSaoParam()->bSaoFlag[0] = pcSlice->getSaoEnabledFlag();
          pcSlice->getAPS()->getSaoParam()->bSaoFlag[1] = pcSlice->getSaoEnabledFlagCb();
          pcSlice->getAPS()->getSaoParam()->bSaoFlag[2] = pcSlice->getSaoEnabledFlagCr();
        }
#if SAO_REMOVE_APS // encoder renaming
        m_pcSAO->setSaoLcuBasedOptimization(1);
#else
        m_pcSAO->setSaoInterleavingFlag(pcSlice->getAPS()->getSaoInterleavingFlag());
#endif
        m_pcSAO->createPicSaoInfo(rpcPic, uiILSliceCount);
        m_pcSAO->SAOProcess(rpcPic, pcSlice->getAPS()->getSaoParam());  
        m_pcAdaptiveLoopFilter->PCMLFDisableProcess(rpcPic);
        m_pcSAO->destroyPicSaoInfo();
      }
    }

    // adaptive loop filter
    if( pcSlice->getSPS()->getUseALF() )
    {
#if AHG6_ALF_OPTION2
      m_pcAdaptiveLoopFilter->createPicAlfInfo(rpcPic, uiILSliceCount);
      m_pcAdaptiveLoopFilter->ALFProcess(rpcPic, pcSlice->getAPS()->getAlfParam(), sliceAlfEnabled);
#else
      if( (pcSlice->getSPS()->getUseALFCoefInSlice())?(true):(pcSlice->getAlfEnabledFlag()))
      {

        if(!pcSlice->getSPS()->getUseALFCoefInSlice())
        {
          patchAlfLCUParams(m_pcAdaptiveLoopFilter->getAlfLCUParam(), pcSlice->getAPS()->getAlfParam());
        }
        m_pcAdaptiveLoopFilter->createPicAlfInfo(rpcPic, uiILSliceCount, pcSlice->getSliceQp());
        m_pcAdaptiveLoopFilter->ALFProcess(rpcPic, vAlfCUCtrlSlices, pcSlice->getSPS()->getUseALFCoefInSlice());
#endif
      m_pcAdaptiveLoopFilter->PCMLFDisableProcess(rpcPic);
      m_pcAdaptiveLoopFilter->destroyPicAlfInfo();
#if !AHG6_ALF_OPTION2
      }
      m_pcAdaptiveLoopFilter->resetLCUAlfInfo(); //reset all LCU ALFParam->alf_flag = 0
#endif
    }
    
    if(pcSlice->getSPS()->getUseSAO() || pcSlice->getSPS()->getUseALF())
    {
      rpcPic->destroyNonDBFilterInfo();
    }

    rpcPic->compressMotion(); 
    Char c = (pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B');
    if (!pcSlice->isReferenced()) c += 32;
    
    //-- For time output for each slice
    printf("\nPOC %4d TId: %1d ( %c-SLICE, QP%3d ) ",
          pcSlice->getPOC(),
          pcSlice->getTLayer(),
          c,
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
#if !REMOVE_LC
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
    if (m_pictureDigestEnabled)
    {
#if HASH_TYPE
      calcAndPrintHashStatus(*rpcPic->getPicYuvRec(), rpcPic->getSEIs());
#else
      calcAndPrintMD5Status(*rpcPic->getPicYuvRec(), rpcPic->getSEIs());
#endif
    }

#if FIXED_ROUNDING_FRAME_MEMORY
    rpcPic->getPicYuvRec()->xFixedRoundingPic();
#endif

    rpcPic->setOutputMark(true);
    rpcPic->setReconMark(true);

    rpcPic->setUsedForTMVP( true );

    uiILSliceCount = 0;
#if AHG6_ALF_OPTION2
    for(Int compIdx=0; compIdx < 3; compIdx++)
    {
      sliceAlfEnabled[compIdx].clear();
    }
#else
    vAlfCUCtrlSlices.clear();
#endif
#if H0391_LF_ACROSS_SLICE_BOUNDARY_CONTROL
    LFCrossSliceBoundaryFlag.clear();
#endif
  }
}
#if HASH_TYPE
/**
 * Calculate and print hash for pic, compare to picture_digest SEI if
 * present in seis.  seis may be NULL.  Hash is printed to stdout, in
 * a manner suitable for the status line. Theformat is:
 *  [Hash_type:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx,(yyy)]
 * Where, x..x is the hash
 *        yyy has the following meanings:
 *            OK          - calculated hash matches the SEI message
 *            ***ERROR*** - calculated hash does not match the SEI message
 *            unk         - no SEI message was available for comparison
 */
static void calcAndPrintHashStatus(TComPicYuv& pic, const SEImessages* seis)
{
  /* calculate MD5sum for entire reconstructed picture */
  unsigned char recon_digest[3][16];
  int numChar=0;
  const char* hashType;

  if(seis->picture_digest->method == SEIpictureDigest::MD5)
  {
    hashType = "MD5";
    calcMD5(pic, recon_digest);
    numChar = 16;    
  }
  else if(seis->picture_digest->method == SEIpictureDigest::CRC)
  {
    hashType = "CRC";
    calcCRC(pic, recon_digest);
    numChar = 2;
  }
  else if(seis->picture_digest->method == SEIpictureDigest::CHECKSUM)
  {
    hashType = "Checksum";
    calcChecksum(pic, recon_digest);
    numChar = 4;
  }
  else
  {
    hashType = "\0";
  }

  /* compare digest against received version */
  const char* ok = "(unk)";
  bool mismatch = false;

  if (seis && seis->picture_digest)
  {
   ok = "(OK)";
    for(int yuvIdx = 0; yuvIdx < 3; yuvIdx++)
    {
      for (unsigned i = 0; i < numChar; i++)
      {
        if (recon_digest[yuvIdx][i] != seis->picture_digest->digest[yuvIdx][i])
        {
          ok = "(***ERROR***)";
          mismatch = true;
        }
      }
    }
  }

  printf("[%s:%s,%s] ", hashType, digestToString(recon_digest, numChar), ok);

  if (mismatch)
  {
    g_md5_mismatch = true;
    printf("[rx%s:%s] ", hashType, digestToString(seis->picture_digest->digest, numChar));
  }
}
#else
/**
 * Calculate and print MD5 for pic, compare to picture_digest SEI if
 * present in seis.  seis may be NULL.  MD5 is printed to stdout, in
 * a manner suitable for the status line. Theformat is:
 *  [MD5:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx,(yyy)]
 * Where, x..x is the md5
 *        yyy has the following meanings:
 *            OK          - calculated MD5 matches the SEI message
 *            ***ERROR*** - calculated MD5 does not match the SEI message
 *            unk         - no SEI message was available for comparison
 */
static void calcAndPrintMD5Status(TComPicYuv& pic, const SEImessages* seis)
{
  /* calculate MD5sum for entire reconstructed picture */
  unsigned char recon_digest[16];
  calcMD5(pic, recon_digest);

  /* compare digest against received version */
  const char* md5_ok = "(unk)";
  bool md5_mismatch = false;

  if (seis && seis->picture_digest)
  {
    md5_ok = "(OK)";
    for (unsigned i = 0; i < 16; i++)
    {
      if (recon_digest[i] != seis->picture_digest->digest[i])
      {
        md5_ok = "(***ERROR***)";
        md5_mismatch = true;
      }
    }
  }

  printf("[MD5:%s,%s] ", digestToString(recon_digest), md5_ok);
  if (md5_mismatch)
  {
    g_md5_mismatch = true;
    printf("[rxMD5:%s] ", digestToString(seis->picture_digest->digest));
  }
}
#endif
//! \}
