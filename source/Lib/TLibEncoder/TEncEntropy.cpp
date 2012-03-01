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

/** \file     TEncEntropy.cpp
    \brief    entropy encoder class
*/

#include "TEncEntropy.h"
#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComAdaptiveLoopFilter.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"

//! \ingroup TLibEncoder
//! \{

Void TEncEntropy::setEntropyCoder ( TEncEntropyIf* e, TComSlice* pcSlice )
{
  m_pcEntropyCoderIf = e;
  m_pcEntropyCoderIf->setSlice ( pcSlice );
}

Void TEncEntropy::encodeSliceHeader ( TComSlice* pcSlice )
{
#if SAO_UNIT_INTERLEAVING
  if (pcSlice->getSPS()->getUseSAO())
  {
    pcSlice->setSaoInterleavingFlag(pcSlice->getAPS()->getSaoInterleavingFlag());
    pcSlice->setSaoEnabledFlag     (pcSlice->getAPS()->getSaoParam()->bSaoFlag[0]);
    if (pcSlice->getAPS()->getSaoInterleavingFlag())
    {
      pcSlice->setSaoEnabledFlagCb   (pcSlice->getAPS()->getSaoParam()->bSaoFlag[1]);
      pcSlice->setSaoEnabledFlagCr   (pcSlice->getAPS()->getSaoParam()->bSaoFlag[2]);
    }
    else
    {
      pcSlice->setSaoEnabledFlagCb   (0);
      pcSlice->setSaoEnabledFlagCr   (0);
    }
  }
#endif

  m_pcEntropyCoderIf->codeSliceHeader( pcSlice );
  return;
}

Void TEncEntropy::encodeSliceHeaderSubstreamTable( TComSlice* pcSlice )
{
  m_pcEntropyCoderIf->codeSliceHeaderSubstreamTable( pcSlice );
}

Void TEncEntropy::encodeTerminatingBit      ( UInt uiIsLast )
{
  m_pcEntropyCoderIf->codeTerminatingBit( uiIsLast );
  
  return;
}

Void TEncEntropy::encodeSliceFinish()
{
  m_pcEntropyCoderIf->codeSliceFinish();
}

#if OL_FLUSH
Void TEncEntropy::encodeFlush()
{
  m_pcEntropyCoderIf->codeFlush();
}
Void TEncEntropy::encodeStart()
{
  m_pcEntropyCoderIf->encodeStart();
}
#endif

Void TEncEntropy::encodeSEI(const SEI& sei)
{
  m_pcEntropyCoderIf->codeSEI(sei);
  return;
}

Void TEncEntropy::encodePPS( TComPPS* pcPPS )
{
  m_pcEntropyCoderIf->codePPS( pcPPS );
  return;
}

Void TEncEntropy::encodeSPS( TComSPS* pcSPS )
{
  m_pcEntropyCoderIf->codeSPS( pcSPS );
  return;
}

Void TEncEntropy::encodeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
#if BURST_IPCM
  if( !bRD )
  {
    if( pcCU->getLastCUSucIPCMFlag() && pcCU->getIPCMFlag(uiAbsPartIdx) )
    {
      return;
    }
  }
#endif
  m_pcEntropyCoderIf->codeSkipFlag( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::codeFiltCountBit(ALFParam* pAlfParam, Int64* ruiRate)
{
  resetEntropy();
  resetBits();
  codeFilt(pAlfParam);
  *ruiRate = getNumberOfWrittenBits();
  resetEntropy();
  resetBits();
}

Void TEncEntropy::codeAuxCountBit(ALFParam* pAlfParam, Int64* ruiRate)
{
  resetEntropy();
  resetBits();
  codeAux(pAlfParam);
  *ruiRate = getNumberOfWrittenBits();
  resetEntropy();
  resetBits();
}

Void TEncEntropy::codeAux(ALFParam* pAlfParam)
{
  //  m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->realfiltNo); 

#if !LCU_SYNTAX_ALF
  m_pcEntropyCoderIf->codeAlfFlag(pAlfParam->alf_pcr_region_flag);
#endif
#if !ALF_SINGLE_FILTER_SHAPE
  m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->filter_shape); 
#endif
  Int noFilters = min(pAlfParam->filters_per_group-1, 2);
  m_pcEntropyCoderIf->codeAlfUvlc(noFilters);

  if(noFilters == 1)
  {
    m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->startSecondFilter);
  }
  else if (noFilters == 2)
  {
#if LCU_SYNTAX_ALF
#if ALF_16_BA_GROUPS
    Int numMergeFlags = 16;
#else
    Int numMergeFlags = 15;
#endif
#else
    Int numMergeFlags = pAlfParam->alf_pcr_region_flag ? 16 : 15;
#endif
    for (Int i=1; i<numMergeFlags; i++) 
    {
      m_pcEntropyCoderIf->codeAlfFlag (pAlfParam->filterPattern[i]);
    }
  }
}

Int TEncEntropy::lengthGolomb(int coeffVal, int k)
{
  int m = 2 << (k - 1);
  int q = coeffVal / m;
  if(coeffVal != 0)
  {
    return(q + 2 + k);
  }
  else
  {
    return(q + 1 + k);
  }
}

Int TEncEntropy::codeFilterCoeff(ALFParam* ALFp)
{
  Int filters_per_group = ALFp->filters_per_group;
  int sqrFiltLength = ALFp->num_coeff;
  int i, k, kMin, kStart, minBits, ind, scanPos, maxScanVal, coeffVal, len = 0,
    *pDepthInt=NULL, kMinTab[MAX_SCAN_VAL], bitsCoeffScan[MAX_SCAN_VAL][MAX_EXP_GOLOMB],
    minKStart, minBitsKStart, bitsKStart;
  
  pDepthInt = pDepthIntTabShapes[ALFp->filter_shape];
  maxScanVal = 0;
#if ALF_SINGLE_FILTER_SHAPE
  int minScanVal = MIN_SCAN_POS_CROSS;
#else
  int minScanVal = ( ALFp->filter_shape==ALF_STAR5x5 ) ? 0 : MIN_SCAN_POS_CROSS;
#endif

  for(i = 0; i < sqrFiltLength; i++)
  {
    maxScanVal = max(maxScanVal, pDepthInt[i]);
  }
  
  // vlc for all
  memset(bitsCoeffScan, 0, MAX_SCAN_VAL * MAX_EXP_GOLOMB * sizeof(int));
  for(ind=0; ind<filters_per_group; ++ind)
  {
    for(i = 0; i < sqrFiltLength; i++)
    {
      scanPos=pDepthInt[i]-1;
      coeffVal=abs(ALFp->coeffmulti[ind][i]);
      for (k=1; k<15; k++)
      {
        bitsCoeffScan[scanPos][k]+=lengthGolomb(coeffVal, k);
      }
    }
  }
  
  minBitsKStart = 0;
  minKStart = -1;
  for(k = 1; k < 8; k++)
  { 
    bitsKStart = 0; 
    kStart = k;
    for(scanPos = minScanVal; scanPos < maxScanVal; scanPos++)
    {
      kMin = kStart; 
      minBits = bitsCoeffScan[scanPos][kMin];
      
      if(bitsCoeffScan[scanPos][kStart+1] < minBits)
      {
        kMin = kStart + 1; 
        minBits = bitsCoeffScan[scanPos][kMin];
      }
      kStart = kMin;
      bitsKStart += minBits;
    }
    if((bitsKStart < minBitsKStart) || (k == 1))
    {
      minBitsKStart = bitsKStart;
      minKStart = k;
    }
  }
  
  kStart = minKStart; 
  for(scanPos = minScanVal; scanPos < maxScanVal; scanPos++)
  {
    kMin = kStart; 
    minBits = bitsCoeffScan[scanPos][kMin];
    
    if(bitsCoeffScan[scanPos][kStart+1] < minBits)
    {
      kMin = kStart + 1; 
      minBits = bitsCoeffScan[scanPos][kMin];
    }
    
    kMinTab[scanPos] = kMin;
    kStart = kMin;
  }
  
  // Coding parameters
  ALFp->minKStart = minKStart;
#if !LCU_SYNTAX_ALF  
  ALFp->maxScanVal = maxScanVal;
#endif
  for(scanPos = minScanVal; scanPos < maxScanVal; scanPos++)
  {
    ALFp->kMinTab[scanPos] = kMinTab[scanPos];
  }

#if LCU_SYNTAX_ALF
  if (ALFp->filters_per_group == 1)
  {
    len += writeFilterCoeffs(sqrFiltLength, filters_per_group, pDepthInt, ALFp->coeffmulti, kTableTabShapes[ALF_CROSS9x7_SQUARE3x3]);
  }
  else
  {
#endif
  len += writeFilterCodingParams(minKStart, minScanVal, maxScanVal, kMinTab);

  // Filter coefficients
  len += writeFilterCoeffs(sqrFiltLength, filters_per_group, pDepthInt, ALFp->coeffmulti, kMinTab);
#if LCU_SYNTAX_ALF
  }
#endif
  
  return len;
}

Int TEncEntropy::writeFilterCodingParams(int minKStart, int minScanVal, int maxScanVal, int kMinTab[])
{
  int scanPos;
  int golombIndexBit;
  int kMin;

  // Golomb parameters
  m_pcEntropyCoderIf->codeAlfUvlc(minKStart - 1);
  
  kMin = minKStart; 
  for(scanPos = minScanVal; scanPos < maxScanVal; scanPos++)
  {
    golombIndexBit = (kMinTab[scanPos] != kMin)? 1: 0;
    
    assert(kMinTab[scanPos] <= kMin + 1);
    
    m_pcEntropyCoderIf->codeAlfFlag(golombIndexBit);
    kMin = kMinTab[scanPos];
  }    
  
  return 0;
}

Int TEncEntropy::writeFilterCoeffs(int sqrFiltLength, int filters_per_group, int pDepthInt[], 
                                   int **FilterCoeff, int kMinTab[])
{
  int ind, scanPos, i;
  
  for(ind = 0; ind < filters_per_group; ++ind)
  {
    for(i = 0; i < sqrFiltLength; i++)
    {
      scanPos = pDepthInt[i] - 1;
#if LCU_SYNTAX_ALF
      Int k = (filters_per_group == 1) ? kMinTab[i] : kMinTab[scanPos];
      golombEncode(FilterCoeff[ind][i], k);
#else
      golombEncode(FilterCoeff[ind][i], kMinTab[scanPos]);
#endif
    }
  }
  return 0;
}

Int TEncEntropy::golombEncode(int coeff, int k)
{
  int q, i;
  int symbol = abs(coeff);
  
  q = symbol >> k;
  
  for (i = 0; i < q; i++)
  {
    m_pcEntropyCoderIf->codeAlfFlag(1);
  }
  m_pcEntropyCoderIf->codeAlfFlag(0);
  // write one zero
  
  for(i = 0; i < k; i++)
  {
    m_pcEntropyCoderIf->codeAlfFlag(symbol & 0x01);
    symbol >>= 1;
  }
  
  if(coeff != 0)
  {
    int sign = (coeff > 0)? 1: 0;
    m_pcEntropyCoderIf->codeAlfFlag(sign);
  }
  return 0;
}

Void TEncEntropy::codeFilt(ALFParam* pAlfParam)
{
  if(pAlfParam->filters_per_group > 1)
  {
    m_pcEntropyCoderIf->codeAlfFlag (pAlfParam->predMethod);
  }
  for(Int ind = 0; ind < pAlfParam->filters_per_group; ++ind)
  {
    m_pcEntropyCoderIf->codeAlfFlag (pAlfParam->nbSPred[ind]);
  }
  codeFilterCoeff (pAlfParam);
}

/** encode merge flag
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiPUIdx
 * \returns Void
 */
Void TEncEntropy::encodeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPUIdx )
{ 
  // at least one merge candidate exists
  m_pcEntropyCoderIf->codeMergeFlag( pcCU, uiAbsPartIdx );
}

/** encode merge index
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiPUIdx
 * \param bRD
 * \returns Void
 */
Void TEncEntropy::encodeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPUIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
    assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
  }

  UInt uiNumCand = MRG_MAX_NUM_CANDS;
  if ( uiNumCand > 1 )
  {
    m_pcEntropyCoderIf->codeMergeIndex( pcCU, uiAbsPartIdx );
  }
}

#if LCU_SYNTAX_ALF
/** parse the fixed length code (smaller than one max value) in ALF
 * \param run: coded value
 * \param rx: cur addr
 * \param numLCUInWidth: # of LCU in one LCU 
 * \returns Void
 */
Void TEncEntropy::encodeAlfFixedLengthRun(UInt run, UInt rx, UInt numLCUInWidth)
{
  assert(numLCUInWidth > rx);
  UInt maxValue = numLCUInWidth - rx - 1;
  m_pcEntropyCoderIf->codeAlfFixedLengthIdx(run, maxValue);
}

/** parse the fixed length code (smaller than one max value) in ALF
 * \param idx: coded value
 * \param numFilterSetsInBuffer: max value 
 * \returns Void
 */
Void TEncEntropy::encodeAlfStoredFilterSetIdx(UInt idx, UInt numFilterSetsInBuffer)
{
  assert(numFilterSetsInBuffer > 0);
  UInt maxValue = numFilterSetsInBuffer - 1;
  m_pcEntropyCoderIf->codeAlfFixedLengthIdx(idx, maxValue);
}

Void TEncEntropy::encodeAlfParam(AlfParamSet* pAlfParamSet, Bool bSentInAPS, Int firstLCUAddr, Bool alfAcrossSlice)
{
  Bool isEnabled[NUM_ALF_COMPONENT];
  Bool isUniParam[NUM_ALF_COMPONENT];

  isEnabled[ALF_Y] = true;
  isEnabled[ALF_Cb]= pAlfParamSet->isEnabled[ALF_Cb];
  isEnabled[ALF_Cr]= pAlfParamSet->isEnabled[ALF_Cr];

  isUniParam[ALF_Y]= pAlfParamSet->isUniParam[ALF_Y];
  isUniParam[ALF_Cb]= pAlfParamSet->isUniParam[ALF_Cb];
  isUniParam[ALF_Cr]= pAlfParamSet->isUniParam[ALF_Cr]; 


  //alf_cb_enable_flag
  m_pcEntropyCoderIf->codeAlfFlag(isEnabled[ALF_Cb]?1:0);
  //alf_cr_enable_flag
  m_pcEntropyCoderIf->codeAlfFlag(isEnabled[ALF_Cr]?1:0);  

  for(Int compIdx = 0; compIdx< NUM_ALF_COMPONENT; compIdx++)
  {
    if(isEnabled[compIdx])
    {
      //alf_one_{luma, cb, cr}_unit_per_slice_flag
      m_pcEntropyCoderIf->codeAlfFlag(isUniParam[compIdx]?1:0);
    }
  }
  if(bSentInAPS)
  {
    //alf_num_lcu_in_width_minus1
    m_pcEntropyCoderIf->codeAlfUvlc(pAlfParamSet->numLCUInWidth-1);
    //alf_num_lcu_in_height_minus1
    m_pcEntropyCoderIf->codeAlfUvlc(pAlfParamSet->numLCUInHeight-1);
  }
  else //sent in slice header
  {
    //alf_num_lcu_in_slice_minus1
    m_pcEntropyCoderIf->codeAlfUvlc(pAlfParamSet->numLCU-1);
  }


  encodeAlfParamSet(pAlfParamSet, pAlfParamSet->numLCUInWidth, pAlfParamSet->numLCU, firstLCUAddr, alfAcrossSlice, 0, (Int)NUM_ALF_COMPONENT-1);

}

Bool TEncEntropy::getAlfRepeatRowFlag(Int compIdx, AlfParamSet* pAlfParamSet
                                    , Int lcuIdxInSlice, Int lcuPos
                                    , Int startlcuPosX, Int endlcuPosX
                                    , Int numLCUInWidth
                                    )
{
  assert(startlcuPosX == 0); //only the beginning of one LCU row needs to send repeat_row_flag
  
  Int len = endlcuPosX - startlcuPosX +1;
  Bool isRepeatRow = true;
  Int curPos;

  for(Int i= 0; i < len; i++)
  {
    curPos = lcuIdxInSlice +i;
    AlfUnitParam& alfUnitParam = pAlfParamSet->alfUnitParam[compIdx][curPos];
    AlfUnitParam& alfUpUnitParam = pAlfParamSet->alfUnitParam[compIdx][curPos-numLCUInWidth];

    if ( !(alfUnitParam == alfUpUnitParam) )
    {
      isRepeatRow = false;
      break;
    }
  }

  return isRepeatRow;
}


Int TEncEntropy::getAlfRun(Int compIdx, AlfParamSet* pAlfParamSet
                          , Int lcuIdxInSlice, Int lcuPos
                          , Int startlcuPosX, Int endlcuPosX
                          )
{
  Int alfRun = 0;
  Int len = endlcuPosX - startlcuPosX +1;
  AlfUnitParam& alfLeftUnitParam = pAlfParamSet->alfUnitParam[compIdx][lcuIdxInSlice];



  for(Int i= 1; i < len; i++)
  {
    AlfUnitParam& alfUnitParam = pAlfParamSet->alfUnitParam[compIdx][lcuIdxInSlice+ i];

    if (alfUnitParam == alfLeftUnitParam)
    {
      alfRun++;
    }
    else
    {
      break;
    }
  }

  return alfRun;

}



Void TEncEntropy::encodeAlfParamSet(AlfParamSet* pAlfParamSet, Int numLCUInWidth, Int numLCU, Int firstLCUAddr, Bool alfAcrossSlice, Int startCompIdx, Int endCompIdx)
{
  Int endLCUY       = (numLCU -1 + firstLCUAddr)/numLCUInWidth;
  Int endLCUX       = (numLCU -1 + firstLCUAddr)%numLCUInWidth;

  static Bool isRepeatedRow   [NUM_ALF_COMPONENT];
  static Int  numStoredFilters[NUM_ALF_COMPONENT];
  static Int* run             [NUM_ALF_COMPONENT];

  for(Int compIdx =startCompIdx; compIdx <= endCompIdx; compIdx++)
  {
    isRepeatedRow[compIdx]    = false;
    numStoredFilters[compIdx] = 0;

    run[compIdx] = new Int[numLCU+1];
    run[compIdx][0] = -1; 
  }

  Int  ry, rx, addrUp, endrX, lcuPos;

  for(Int i=0; i< numLCU; i++)
  {
    lcuPos= firstLCUAddr+ i;
    rx    = lcuPos% numLCUInWidth;
    ry    = lcuPos/ numLCUInWidth;
    endrX = ( ry == endLCUY)?( endLCUX ):(numLCUInWidth-1);

    for(Int compIdx =startCompIdx; compIdx <= endCompIdx; compIdx++)
    {
      AlfUnitParam& alfUnitParam = pAlfParamSet->alfUnitParam[compIdx][i];
      if(pAlfParamSet->isEnabled[compIdx])
      {
        if(!pAlfParamSet->isUniParam[compIdx])
        {
          addrUp = i-numLCUInWidth;
          if(rx ==0 && addrUp >=0)
          {
            isRepeatedRow[compIdx] = getAlfRepeatRowFlag(compIdx, pAlfParamSet, i, lcuPos, rx, endrX, numLCUInWidth);

            //alf_repeat_row_flag
            m_pcEntropyCoderIf->codeAlfFlag(isRepeatedRow[compIdx]?1:0);
          }

          if(isRepeatedRow[compIdx])
          {
            assert(addrUp >=0);
            run[compIdx][i] = run[compIdx][addrUp];
          }
          else
          {
            if(rx == 0 || run[compIdx][i] < 0)
            {             
              run[compIdx][i] = getAlfRun(compIdx, pAlfParamSet, i, lcuPos, rx, endrX);

              if(addrUp < 0)
              {
                //alf_run_diff u(v)
                encodeAlfFixedLengthRun(run[compIdx][i], rx, numLCUInWidth);                
              }
              else
              {
                //alf_run_diff s(v)
                m_pcEntropyCoderIf->codeAlfSvlc(run[compIdx][i]- run[compIdx][addrUp]);

              }

              if(ry > 0 && (addrUp >=0 || alfAcrossSlice))
              {
                //alf_merge_up_flag
                m_pcEntropyCoderIf->codeAlfFlag(  (alfUnitParam.mergeType == ALF_MERGE_UP)?1:0   );  
              }

              if(alfUnitParam.mergeType != ALF_MERGE_UP)
              {
                assert(alfUnitParam.mergeType == ALF_MERGE_DISABLED);

                //alf_lcu_enable_flag
                m_pcEntropyCoderIf->codeAlfFlag(alfUnitParam.isEnabled ? 1 : 0);

                if(alfUnitParam.isEnabled)
                {
                  if(numStoredFilters[compIdx] > 0)
                  {
                    //alf_new_filter_set_flag
                    m_pcEntropyCoderIf->codeAlfFlag(alfUnitParam.isNewFilt ? 1:0);

                    if(!alfUnitParam.isNewFilt)
                    {
                      //alf_stored_filter_set_idx
                      encodeAlfStoredFilterSetIdx(alfUnitParam.storedFiltIdx, numStoredFilters[compIdx]);

                    }
                  }
                  else
                  {
                    assert(alfUnitParam.isNewFilt);
                  }

                  if(alfUnitParam.isNewFilt)
                  {
                    assert(alfUnitParam.alfFiltParam->alf_flag == 1);
                    encodeAlfParam(alfUnitParam.alfFiltParam);
                    numStoredFilters[compIdx]++;
                  }
                }

              }
            }

            run[compIdx][i+1] = run[compIdx][i] -1;
          }

        }
        else // uni-param
        {
          if(i == 0)
          {
            //alf_lcu_enable_flag
            m_pcEntropyCoderIf->codeAlfFlag(alfUnitParam.isEnabled?1:0);
            if(alfUnitParam.isEnabled)
            {
              encodeAlfParam(alfUnitParam.alfFiltParam);
            }
          }
        }
      } // component enabled/disable
    } //comp

  }

  for(Int compIdx =startCompIdx; compIdx <= endCompIdx; compIdx++)
  {
    delete[] run[compIdx];
  }

}
#endif


Void TEncEntropy::encodeAlfParam(ALFParam* pAlfParam)
{
#if LCU_SYNTAX_ALF
  const Int numCoeff = (Int)ALF_MAX_NUM_COEF;

  switch(pAlfParam->componentID)
  {
  case ALF_Cb:
  case ALF_Cr:
    {
      for(Int pos=0; pos< numCoeff; pos++)
      {
        m_pcEntropyCoderIf->codeAlfSvlc(  pAlfParam->coeffmulti[0][pos]);

      }
    }
    break;
  case ALF_Y:
    {
      codeAux(pAlfParam);
      codeFilt(pAlfParam);
    }
    break;
  default:
    {
      printf("Not a legal component ID\n");
      assert(0);
      exit(-1);
    }
  }
#else
  if (!pAlfParam->alf_flag)
  {
    return;
  }
  Int pos;
  codeAux(pAlfParam);
  codeFilt(pAlfParam);
  
  // filter parameters for chroma
  m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->chroma_idc);
  if(pAlfParam->chroma_idc)
  {
#if !ALF_SINGLE_FILTER_SHAPE
    m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->filter_shape_chroma);
#endif
    // filter coefficients for chroma
    for(pos=0; pos<pAlfParam->num_coeff_chroma; pos++)
    {
      m_pcEntropyCoderIf->codeAlfSvlc(pAlfParam->coeff_chroma[pos]);
    }
  }
#endif
}

Void TEncEntropy::encodeAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  m_pcEntropyCoderIf->codeAlfCtrlFlag( pcCU, uiAbsPartIdx );
}


/** Encode ALF CU control flag
 * \param uiFlag ALF CU control flag: 0 or 1
 */
Void TEncEntropy::encodeAlfCtrlFlag(UInt uiFlag)
{
  assert(uiFlag == 0 || uiFlag == 1);
  m_pcEntropyCoderIf->codeAlfCtrlFlag( uiFlag );
}


/** Encode ALF CU control flag parameters
 * \param pAlfParam ALF parameters
 */
Void TEncEntropy::encodeAlfCtrlParam(AlfCUCtrlInfo& cAlfParam, Int iNumCUsInPic)
{
  // region control parameters for luma
  m_pcEntropyCoderIf->codeAlfFlag(cAlfParam.cu_control_flag);

  if (cAlfParam.cu_control_flag == 0)
  { 
    return;
  }

  m_pcEntropyCoderIf->codeAlfCtrlDepth();

  Int iSymbol    = ((Int)cAlfParam.num_alf_cu_flag - iNumCUsInPic);
  m_pcEntropyCoderIf->codeAlfSvlc(iSymbol);

  for(UInt i=0; i< cAlfParam.num_alf_cu_flag; i++)
  {
    m_pcEntropyCoderIf->codeAlfCtrlFlag( cAlfParam.alf_cu_flag[i] );
  }
}

/** encode prediction mode
 * \param pcCU
 * \param uiAbsPartIdx
 * \param bRD
 * \returns Void
 */
Void TEncEntropy::encodePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
#if BURST_IPCM
  if( !bRD )
  {
    if( pcCU->getLastCUSucIPCMFlag() && pcCU->getIPCMFlag(uiAbsPartIdx) )
    {
      return;
    }
  }
#endif

  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }

  m_pcEntropyCoderIf->codePredMode( pcCU, uiAbsPartIdx );
}

// Split mode
Void TEncEntropy::encodeSplitFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
#if BURST_IPCM
  if( !bRD )
  {
    if( pcCU->getLastCUSucIPCMFlag() && pcCU->getIPCMFlag(uiAbsPartIdx) )
    {
      return;
    }
  }
#endif

  m_pcEntropyCoderIf->codeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
}

/** encode partition size
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \param bRD
 * \returns Void
 */
Void TEncEntropy::encodePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
#if BURST_IPCM
  if( !bRD )
  {
    if( pcCU->getLastCUSucIPCMFlag() && pcCU->getIPCMFlag(uiAbsPartIdx) )
    {
      return;
    }
  }
#endif  
  m_pcEntropyCoderIf->codePartSize( pcCU, uiAbsPartIdx, uiDepth );
}

/** Encode I_PCM information. 
 * \param pcCU pointer to CU 
 * \param uiAbsPartIdx CU index
 * \param bRD flag indicating estimation or encoding
 * \returns Void
 */
Void TEncEntropy::encodeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if(!pcCU->getSlice()->getSPS()->getUsePCM()
    || pcCU->getWidth(uiAbsPartIdx) > (1<<pcCU->getSlice()->getSPS()->getPCMLog2MaxSize())
    || pcCU->getWidth(uiAbsPartIdx) < (1<<pcCU->getSlice()->getSPS()->getPCMLog2MinSize()))
  {
    return;
  }
  
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
#if BURST_IPCM
  Int numIPCM = 0;
  Bool firstIPCMFlag = false;

  if( pcCU->getIPCMFlag(uiAbsPartIdx) )
  {
    numIPCM = 1;
    firstIPCMFlag = true;

    if( !bRD )
    {
      numIPCM = pcCU->getNumSucIPCM();
      firstIPCMFlag = !pcCU->getLastCUSucIPCMFlag();
    }
  }
  m_pcEntropyCoderIf->codeIPCMInfo ( pcCU, uiAbsPartIdx, numIPCM, firstIPCMFlag);
#else
  m_pcEntropyCoderIf->codeIPCMInfo ( pcCU, uiAbsPartIdx );
#endif

}

#if UNIFIED_TRANSFORM_TREE
#if NSQT_LFFIX
Void TEncEntropy::xEncodeTransform( TComDataCU* pcCU,UInt offsetLuma, UInt offsetChroma, UInt uiAbsPartIdx, UInt uiNSAbsPartIdx, UInt uiDepth, UInt width, UInt height, UInt uiTrIdx, UInt uiInnerQuadIdx, UInt& uiYCbfFront3, UInt& uiUCbfFront3, UInt& uiVCbfFront3, Bool& bCodeDQP )
#else
Void TEncEntropy::xEncodeTransform( TComDataCU* pcCU,UInt offsetLuma, UInt offsetChroma, UInt uiAbsPartIdx, UInt uiDepth, UInt width, UInt height, UInt uiTrIdx, UInt uiInnerQuadIdx, UInt& uiYCbfFront3, UInt& uiUCbfFront3, UInt& uiVCbfFront3, Bool& bCodeDQP )
#endif
#else
#if NSQT_LFFIX
Void TEncEntropy::xEncodeTransformSubdiv( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiNSAbsPartIdx, UInt uiDepth, UInt uiInnerQuadIdx, UInt& uiYCbfFront3, UInt& uiUCbfFront3, UInt& uiVCbfFront3 )
#else
Void TEncEntropy::xEncodeTransformSubdiv( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiInnerQuadIdx, UInt& uiYCbfFront3, UInt& uiUCbfFront3, UInt& uiVCbfFront3 )
#endif
#endif
{
  const UInt uiSubdiv = pcCU->getTransformIdx( uiAbsPartIdx ) + pcCU->getDepth( uiAbsPartIdx ) > uiDepth;
  const UInt uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()]+2 - uiDepth;
#if UNIFIED_TRANSFORM_TREE
  UInt cbfY = pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA    , uiTrIdx );
  UInt cbfU = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrIdx );
  UInt cbfV = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrIdx );

  if( uiLog2TrafoSize == 2 )
  {
    UInt partNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
    if( ( uiAbsPartIdx % partNum ) == 0 )
    {
      m_uiBakAbsPartIdx   = uiAbsPartIdx;
      m_uiBakChromaOffset = offsetChroma;
    }
    else if( ( uiAbsPartIdx % partNum ) == (partNum - 1) )
    {
      cbfU = pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_U, uiTrIdx );
      cbfV = pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_V, uiTrIdx );
    }
  }
#endif // UNIFIED_TRANSFORM_TREE
  {//CABAC
    if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN && uiDepth == pcCU->getDepth(uiAbsPartIdx) )
    {
      assert( uiSubdiv );
    }
    else if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER && (pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N) && uiDepth == pcCU->getDepth(uiAbsPartIdx) &&  (pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) )
    {
      if ( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
      {
        assert( uiSubdiv );
      }
      else
      {
        assert(!uiSubdiv );
      }
    }
    else if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
    {
      assert( uiSubdiv );
    }
    else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
    {
      assert( !uiSubdiv );
    }
    else if( uiLog2TrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
    {
      assert( !uiSubdiv );
    }
    else
    {
      assert( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
      m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSubdiv, uiDepth );
    }
  }

  {
    if( uiLog2TrafoSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
    {
      const UInt uiTrDepthCurr = uiDepth - pcCU->getDepth( uiAbsPartIdx );
      const Bool bFirstCbfOfCU = uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() || uiTrDepthCurr == 0;
      if( bFirstCbfOfCU || uiLog2TrafoSize > 2 )
      {
        if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr - 1 ) )
        {
          if ( uiInnerQuadIdx == 3 && uiUCbfFront3 == 0 && uiLog2TrafoSize < pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
          {
            uiUCbfFront3++;
          }
          else
          {
            m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr );
            uiUCbfFront3 += pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr );
          }
        }
        if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr - 1 ) )
        {
          if ( uiInnerQuadIdx == 3 && uiVCbfFront3 == 0 && uiLog2TrafoSize < pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize()  )
          {
            uiVCbfFront3++;
          }
          else
          {
            m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr );
            uiVCbfFront3 += pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr );
          }
        }
      }
      else if( uiLog2TrafoSize == 2 )
      {
        assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr - 1 ) );
        assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr - 1 ) );
        
        uiUCbfFront3 += pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr );
        uiVCbfFront3 += pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr );
      }
    }
    
    if( uiSubdiv )
    {
#if UNIFIED_TRANSFORM_TREE
      UInt size;
      width  >>= 1;
      height >>= 1;
      size = width*height;
      uiTrIdx++;
#endif // UNIFIED_TRANSFORM_TREE
      ++uiDepth;
#if UNIFIED_TRANSFORM_TREE
      const UInt partNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
#else
      const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
#endif
      
      UInt uiCurrentCbfY = 0;
      UInt uiCurrentCbfU = 0;
      UInt uiCurrentCbfV = 0;
      
#if UNIFIED_TRANSFORM_TREE
#if NSQT_LFFIX
      UInt uiNSAddr = 0;
      uiNSAddr = pcCU->getNSAbsPartIdx( uiLog2TrafoSize-1, uiAbsPartIdx, uiNSAbsPartIdx, 0, uiDepth - pcCU->getDepth( uiAbsPartIdx ) );
      xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiNSAddr, uiDepth, width, height, uiTrIdx, 0, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV, bCodeDQP );

      uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
      uiNSAddr = pcCU->getNSAbsPartIdx( uiLog2TrafoSize-1, uiAbsPartIdx, uiNSAbsPartIdx, 1, uiDepth - pcCU->getDepth( uiAbsPartIdx ) );
      xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiNSAddr, uiDepth, width, height, uiTrIdx, 1, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV, bCodeDQP );

      uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
      uiNSAddr = pcCU->getNSAbsPartIdx( uiLog2TrafoSize-1, uiAbsPartIdx, uiNSAbsPartIdx, 2, uiDepth - pcCU->getDepth( uiAbsPartIdx ) );
      xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiNSAddr, uiDepth, width, height, uiTrIdx, 2, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV, bCodeDQP );

      uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
      uiNSAddr = pcCU->getNSAbsPartIdx( uiLog2TrafoSize-1, uiAbsPartIdx, uiNSAbsPartIdx, 3, uiDepth - pcCU->getDepth( uiAbsPartIdx ) );
      xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiNSAddr, uiDepth, width, height, uiTrIdx, 3, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV, bCodeDQP );      
#else
      xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, 0, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV, bCodeDQP );
      uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
      xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, 1, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV, bCodeDQP );
      uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
      xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, 2, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV, bCodeDQP );
      uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
      xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, 3, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV, bCodeDQP );
#endif
#else // UNIFIED_TRANSFORM_TREE
#if NSQT_LFFIX
      UInt uiNSAddr = 0;
      uiNSAddr = pcCU->getNSAbsPartIdx( uiLog2TrafoSize-1, uiAbsPartIdx, uiNSAbsPartIdx, 0, uiDepth - pcCU->getDepth( uiAbsPartIdx ) );
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiNSAddr, uiDepth, 0, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );

      uiAbsPartIdx += uiQPartNum;
      uiNSAddr = pcCU->getNSAbsPartIdx( uiLog2TrafoSize-1, uiAbsPartIdx, uiNSAbsPartIdx, 1, uiDepth - pcCU->getDepth( uiAbsPartIdx ) );
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiNSAddr, uiDepth, 1, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );

      uiAbsPartIdx += uiQPartNum;
      uiNSAddr = pcCU->getNSAbsPartIdx( uiLog2TrafoSize-1, uiAbsPartIdx, uiNSAbsPartIdx, 2, uiDepth - pcCU->getDepth( uiAbsPartIdx ) );
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiNSAddr, uiDepth, 2, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );

      uiAbsPartIdx += uiQPartNum;
      uiNSAddr = pcCU->getNSAbsPartIdx( uiLog2TrafoSize-1, uiAbsPartIdx, uiNSAbsPartIdx, 3, uiDepth - pcCU->getDepth( uiAbsPartIdx ) );
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiNSAddr, uiDepth, 3, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );
#else
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );
      uiAbsPartIdx += uiQPartNum;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 1, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );
      uiAbsPartIdx += uiQPartNum;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 2, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );
      uiAbsPartIdx += uiQPartNum;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 3, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );
#endif
#endif // UNIFIED_TRANSFORM_TREE
      
      uiYCbfFront3 += uiCurrentCbfY;
      uiUCbfFront3 += uiCurrentCbfU;
      uiVCbfFront3 += uiCurrentCbfV;
    }
    else
    {
      {
        DTRACE_CABAC_VL( g_nSymbolCounter++ );
        DTRACE_CABAC_T( "\tTrIdx: abspart=" );
        DTRACE_CABAC_V( uiAbsPartIdx );
        DTRACE_CABAC_T( "\tdepth=" );
        DTRACE_CABAC_V( uiDepth );
        DTRACE_CABAC_T( "\ttrdepth=" );
        DTRACE_CABAC_V( pcCU->getTransformIdx( uiAbsPartIdx ) );
        DTRACE_CABAC_T( "\n" );
      }
      UInt uiLumaTrMode, uiChromaTrMode;
      pcCU->convertTransIdx( uiAbsPartIdx, pcCU->getTransformIdx( uiAbsPartIdx ), uiLumaTrMode, uiChromaTrMode );
#if NSQT_LFFIX
      if(pcCU->getPredictionMode( uiAbsPartIdx ) == MODE_INTER && pcCU->useNonSquarePU( uiAbsPartIdx ) )
      {
        pcCU->setNSQTIdxSubParts( uiLog2TrafoSize, uiAbsPartIdx, uiNSAbsPartIdx, uiLumaTrMode );
      }
#endif
      if( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA && uiDepth == pcCU->getDepth( uiAbsPartIdx ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, 0 ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, 0 ) )
      {
        assert( pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, 0 ) );
        //      printf( "saved one bin! " );
      }
      else
      {
        const UInt uiLog2CUSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()] + 2 - pcCU->getDepth( uiAbsPartIdx );
        if ( pcCU->getPredictionMode( uiAbsPartIdx ) != MODE_INTRA && uiInnerQuadIdx == 3 && uiYCbfFront3 == 0 && uiUCbfFront3 == 0 && uiVCbfFront3 == 0
            && ( uiLog2CUSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() + 1 || uiLog2TrafoSize < pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() ) )
        {      
          uiYCbfFront3++;
        }    
        else
        {
          m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, uiLumaTrMode );
          uiYCbfFront3 += pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiLumaTrMode );
        }
      }
      
#if UNIFIED_TRANSFORM_TREE
      if ( cbfY || cbfU || cbfV )
      {
        // dQP: only for LCU once
        if ( pcCU->getSlice()->getPPS()->getUseDQP() )
        {
          if ( bCodeDQP )
          {
            encodeQP( pcCU, uiAbsPartIdx );
            bCodeDQP = false;
          }
        }
      }
      if( cbfY )
      {
        Int trWidth = width;
        Int trHeight = height;
        pcCU->getNSQTSize( uiTrIdx, uiAbsPartIdx, trWidth, trHeight );
        m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffY()+offsetLuma), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_LUMA );
      }
      if( uiLog2TrafoSize > 2 )
      {
        Int trWidth = width >> 1;
        Int trHeight = height >> 1;
        pcCU->getNSQTSize( uiTrIdx, uiAbsPartIdx, trWidth, trHeight );
        if( cbfU )
        {
          m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCb()+offsetChroma), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
        }
        if( cbfV )
        {
          m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCr()+offsetChroma), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
        }
      }
      else
      {
        UInt partNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
        if( ( uiAbsPartIdx % partNum ) == (partNum - 1) )
        {
          Int trWidth = width;
          Int trHeight = height;
          pcCU->getNSQTSize( uiTrIdx-1, uiAbsPartIdx, trWidth, trHeight );
          if( cbfU )
          {
            m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCb()+m_uiBakChromaOffset), m_uiBakAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
          }
          if( cbfV )
          {
            m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCr()+m_uiBakChromaOffset), m_uiBakAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
          }
        }
      }
#endif // UNIFIED_TRANSFORM_TREE
    }
  }
}

#if !UNIFIED_TRANSFORM_TREE
// transform index
Void TEncEntropy::encodeTransformIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  assert( !bRD ); // parameter bRD can be removed
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tdecodeTransformIdx()\tCUDepth=" )
  DTRACE_CABAC_V( uiDepth )
  DTRACE_CABAC_T( "\n" )
  UInt temp = 0;
  UInt temp1 = 0;
  UInt temp2 = 0;
#if NSQT_LFFIX
  xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiAbsPartIdx, uiDepth, 0, temp, temp1, temp2 );
#else
  xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0, temp, temp1, temp2 );
#endif
}
#endif // !UNIFIED_TRANSFORM_TREE

// Intra direction for Luma
Void TEncEntropy::encodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  m_pcEntropyCoderIf->codeIntraDirLumaAng( pcCU, uiAbsPartIdx );
}

// Intra direction for Chroma
Void TEncEntropy::encodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  m_pcEntropyCoderIf->codeIntraDirChroma( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::encodePredInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  PartSize eSize = pcCU->getPartitionSize( uiAbsPartIdx );
  
  if( pcCU->isIntra( uiAbsPartIdx ) )                                 // If it is Intra mode, encode intra prediction mode.
  {
    if( eSize == SIZE_NxN )                                         // if it is NxN size, encode 4 intra directions.
    {
      UInt uiPartOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;
      // if it is NxN size, this size might be the smallest partition size.
      encodeIntraDirModeLuma( pcCU, uiAbsPartIdx                  );
      encodeIntraDirModeLuma( pcCU, uiAbsPartIdx + uiPartOffset   );
      encodeIntraDirModeLuma( pcCU, uiAbsPartIdx + uiPartOffset*2 );
      encodeIntraDirModeLuma( pcCU, uiAbsPartIdx + uiPartOffset*3 );
      encodeIntraDirModeChroma( pcCU, uiAbsPartIdx, bRD );
    }
    else                                                              // if it is not NxN size, encode 1 intra directions
    {
      encodeIntraDirModeLuma  ( pcCU, uiAbsPartIdx );
      encodeIntraDirModeChroma( pcCU, uiAbsPartIdx, bRD );
    }
  }
  else                                                                // if it is Inter mode, encode motion vector and reference index
  {
    encodePUWise( pcCU, uiAbsPartIdx, bRD );
  }
}

/** encode motion information for every PU block
 * \param pcCU
 * \param uiAbsPartIdx
 * \param bRD
 * \returns Void
 * This function is called only if merge is enabled.
 */
Void TEncEntropy::encodePUWise( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  UInt uiNumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
  UInt uiDepth = pcCU->getDepth( uiAbsPartIdx );
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;

  for ( UInt uiPartIdx = 0, uiSubPartIdx = uiAbsPartIdx; uiPartIdx < uiNumPU; uiPartIdx++, uiSubPartIdx += uiPUOffset )
  {
    if ( pcCU->getSlice()->getSPS()->getUseMRG() )
    {
      {
        encodeMergeFlag( pcCU, uiSubPartIdx, uiPartIdx );
      }
    }
    if ( pcCU->getMergeFlag( uiSubPartIdx ) )
    {
      encodeMergeIndex( pcCU, uiSubPartIdx, uiPartIdx );
    }
    else
    {
      encodeInterDirPU( pcCU, uiSubPartIdx );
      for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
      {
        if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
        {
          encodeRefFrmIdxPU ( pcCU, uiSubPartIdx, RefPicList( uiRefListIdx ) );
          encodeMvdPU       ( pcCU, uiSubPartIdx, RefPicList( uiRefListIdx ) );
          encodeMVPIdxPU    ( pcCU, uiSubPartIdx, RefPicList( uiRefListIdx ) );
        }
      }
    }
  }

  return;
}

Void TEncEntropy::encodeInterDirPU( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if ( !pcCU->getSlice()->isInterB() )
  {
    return;
  }

  m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
  return;
}

/** encode reference frame index for a PU block
 * \param pcCU
 * \param uiAbsPartIdx
 * \param eRefList
 * \returns Void
 */
Void TEncEntropy::encodeRefFrmIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );

  if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C)>0 && pcCU->getInterDir( uiAbsPartIdx ) != 3)
  {
    if ((eRefList== REF_PIC_LIST_1) || ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_C ) == 1 ) )
    {
      return;
    }

    if ( pcCU->getSlice()->getNumRefIdx ( REF_PIC_LIST_C ) > 1 )
    {
      m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, RefPicList(pcCU->getInterDir( uiAbsPartIdx )-1) );
    }

  }
  else
  {
    if ( ( pcCU->getSlice()->getNumRefIdx( eRefList ) == 1 ) )
    {
      return;
    }

    if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
    {
      m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
    }
  }

  return;
}

/** encode motion vector difference for a PU block
 * \param pcCU
 * \param uiAbsPartIdx
 * \param eRefList
 * \returns Void
 */
Void TEncEntropy::encodeMvdPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );

  if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
  {
    m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
  }
  return;
}

Void TEncEntropy::encodeMVPIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
  {
    m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
  }

  return;
}

Void TEncEntropy::encodeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, eType, uiTrDepth );
}

Void TEncEntropy::encodeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSymbol, uiCtx );
}

Void TEncEntropy::encodeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  m_pcEntropyCoderIf->codeQtRootCbf( pcCU, uiAbsPartIdx );
}

// dQP
Void TEncEntropy::encodeQP( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  if ( pcCU->getSlice()->getPPS()->getUseDQP() )
  {
    m_pcEntropyCoderIf->codeDeltaQP( pcCU, uiAbsPartIdx );
  }
}


// texture
#if !UNIFIED_TRANSFORM_TREE
Void TEncEntropy::xEncodeCoeff( TComDataCU* pcCU, UInt uiLumaOffset, UInt uiChromaOffset, UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, UInt uiTrIdx, UInt uiCurrTrIdx, Bool& bCodeDQP )
{
  UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth ] + 2;
  UInt uiCbfY = pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrIdx );
  UInt uiCbfU = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrIdx );
  UInt uiCbfV = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrIdx );

  if( uiLog2TrSize == 2 )
  {
    UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
    if( ( uiAbsPartIdx % uiQPDiv ) == 0 )
    {
      m_uiBakAbsPartIdx   = uiAbsPartIdx;
      m_uiBakChromaOffset = uiChromaOffset;
    }
    else if( ( uiAbsPartIdx % uiQPDiv ) == (uiQPDiv - 1) )
    {
      uiCbfU = pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_U, uiTrIdx );
      uiCbfV = pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_V, uiTrIdx );
    }
  }

  if ( uiCbfY || uiCbfU || uiCbfV )
  {
    // dQP: only for LCU once
    if ( pcCU->getSlice()->getPPS()->getUseDQP() )
    {
      if ( bCodeDQP )
      {
        encodeQP( pcCU, uiAbsPartIdx );
        bCodeDQP = false;
      }
    }
    UInt uiLumaTrMode, uiChromaTrMode;
    pcCU->convertTransIdx( uiAbsPartIdx, pcCU->getTransformIdx( uiAbsPartIdx ), uiLumaTrMode, uiChromaTrMode );
    const UInt uiStopTrMode = uiLumaTrMode;
    
    assert(1); // as long as quadtrees are not used for residual transform
    
    if( uiTrIdx == uiStopTrMode )
    {
      if( pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrIdx ) )
      {
        Int trWidth = uiWidth;
        Int trHeight = uiHeight;
        pcCU->getNSQTSize( uiTrIdx, uiAbsPartIdx, trWidth, trHeight );
        m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffY()+uiLumaOffset), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_LUMA );
      }

      uiWidth  >>= 1;
      uiHeight >>= 1;

      if( uiLog2TrSize == 2 )
      {
        UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
        if( ( uiAbsPartIdx % uiQPDiv ) == (uiQPDiv - 1) )
        {
          uiWidth  <<= 1;
          uiHeight <<= 1;
          Int trWidth = uiWidth;
          Int trHeight = uiHeight;
          pcCU->getNSQTSize( uiTrIdx-1, uiAbsPartIdx, trWidth, trHeight );
          if( pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_U, uiTrIdx ) )
          {
            m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCb()+m_uiBakChromaOffset), m_uiBakAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
          }
          if( pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_V, uiTrIdx ) )
          {
            m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCr()+m_uiBakChromaOffset), m_uiBakAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
          }
        }
      }
      else
      {
        Int trWidth = uiWidth;
        Int trHeight = uiHeight;
        pcCU->getNSQTSize( uiTrIdx, uiAbsPartIdx, trWidth, trHeight );
        if( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrIdx ) )
        {
          m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCb()+uiChromaOffset), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
        }
        if( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrIdx ) )
        {
          m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCr()+uiChromaOffset), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
        }
      }
    }
    else
    {
      {
        DTRACE_CABAC_VL( g_nSymbolCounter++ );
        DTRACE_CABAC_T( "\tgoing down\tdepth=" );
        DTRACE_CABAC_V( uiDepth );
        DTRACE_CABAC_T( "\ttridx=" );
        DTRACE_CABAC_V( uiTrIdx );
        DTRACE_CABAC_T( "\n" );
      }
      if( uiCurrTrIdx <= uiTrIdx )
        assert(1);
      
      UInt uiSize;
      uiWidth  >>= 1;
      uiHeight >>= 1;
      uiSize = uiWidth*uiHeight;
      uiDepth++;
      uiTrIdx++;
      
      UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
      UInt uiIdx      = uiAbsPartIdx;
      
      {
        xEncodeCoeff( pcCU, uiLumaOffset, uiChromaOffset, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, bCodeDQP );
        uiLumaOffset += uiSize;  uiChromaOffset += (uiSize>>2);  uiIdx += uiQPartNum;

        xEncodeCoeff( pcCU, uiLumaOffset, uiChromaOffset, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, bCodeDQP );
        uiLumaOffset += uiSize;  uiChromaOffset += (uiSize>>2);  uiIdx += uiQPartNum;

        xEncodeCoeff( pcCU, uiLumaOffset, uiChromaOffset, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, bCodeDQP );
        uiLumaOffset += uiSize;  uiChromaOffset += (uiSize>>2);  uiIdx += uiQPartNum;

        xEncodeCoeff( pcCU, uiLumaOffset, uiChromaOffset, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, bCodeDQP );
      }
      {
        DTRACE_CABAC_VL( g_nSymbolCounter++ );
        DTRACE_CABAC_T( "\tgoing up\n" );
      }
    }
  }
}
#endif // !UNIFIED_TRANSFORM_TREE

/** encode coefficients
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \param uiWidth
 * \param uiHeight
 */
Void TEncEntropy::encodeCoeff( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, Bool& bCodeDQP )
{
  UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
  UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
  UInt uiChromaOffset = uiLumaOffset>>2;
  
  UInt uiLumaTrMode, uiChromaTrMode;
  pcCU->convertTransIdx( uiAbsPartIdx, pcCU->getTransformIdx(uiAbsPartIdx), uiLumaTrMode, uiChromaTrMode );
  
  if( pcCU->isIntra(uiAbsPartIdx) )
  {
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tdecodeTransformIdx()\tCUDepth=" )
    DTRACE_CABAC_V( uiDepth )
    DTRACE_CABAC_T( "\n" )
#if !UNIFIED_TRANSFORM_TREE
    UInt temp = 0;
    UInt temp1 = 0;
    UInt temp2 = 0;
#if NSQT_LFFIX
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiAbsPartIdx, uiDepth, 0, temp, temp1, temp2 );
#else
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0, temp, temp1, temp2 );
#endif
#endif // !UNIFIED_TRANSFORM_TREE
  }
  else
  {
    {
      if( !(pcCU->getMergeFlag( uiAbsPartIdx ) && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N ) )
      {
        m_pcEntropyCoderIf->codeQtRootCbf( pcCU, uiAbsPartIdx );
      }
      if ( !pcCU->getQtRootCbf( uiAbsPartIdx ) )
      {
#if NSQT_LFFIX
        pcCU->setNSQTIdxSubParts( uiAbsPartIdx, uiDepth );
#endif
        return;
      }
    }
#if !UNIFIED_TRANSFORM_TREE
    encodeTransformIdx( pcCU, uiAbsPartIdx, pcCU->getDepth(uiAbsPartIdx) );
#endif
  }
  
#if UNIFIED_TRANSFORM_TREE
  UInt temp = 0;
  UInt temp1 = 0;
  UInt temp2 = 0;
#if NSQT_LFFIX
  xEncodeTransform( pcCU, uiLumaOffset, uiChromaOffset, uiAbsPartIdx, uiAbsPartIdx, uiDepth, uiWidth, uiHeight, 0, 0, temp, temp1, temp2, bCodeDQP );
#else
  xEncodeTransform( pcCU, uiLumaOffset, uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth, uiHeight, 0, 0, temp, temp1, temp2, bCodeDQP );
#endif
#else // UNIFIED_TRANSFORM_TREE
  xEncodeCoeff( pcCU, uiLumaOffset, uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth, uiHeight, 0, uiLumaTrMode, bCodeDQP );
#endif // UNIFIED_TRANSFORM_TREE
}

Void TEncEntropy::encodeCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoeff, UInt uiAbsPartIdx, UInt uiTrWidth, UInt uiTrHeight, UInt uiDepth, TextType eType )
{ // This is for Transform unit processing. This may be used at mode selection stage for Inter.
  m_pcEntropyCoderIf->codeCoeffNxN( pcCU, pcCoeff, uiAbsPartIdx, uiTrWidth, uiTrHeight, uiDepth, eType );
}

Void TEncEntropy::estimateBit (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType)
{  
  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA;
  
  m_pcEntropyCoderIf->estBit ( pcEstBitsSbac, width, height, eTType );
}

#if SAO_UNIT_INTERLEAVING
/** Encode SAO Offset
 * \param  psSaoLcuParam
 */
Void TEncEntropy::encodeSaoOffset(SaoLcuParam* psSaoLcuParam)
{
  UInt uiSymbol;
  Int i;

  uiSymbol = psSaoLcuParam->typeIdx + 1;
  m_pcEntropyCoderIf->codeSaoTypeIdx(uiSymbol);
  if (uiSymbol)
  {
    if( psSaoLcuParam->typeIdx == SAO_BO )
    {
      // Code Left Band Index
      uiSymbol = (UInt) (psSaoLcuParam->bandPosition);
      m_pcEntropyCoderIf->codeSaoUflc(uiSymbol);
      for( i=0; i< psSaoLcuParam->iLength; i++)
      {
        m_pcEntropyCoderIf->codeSaoSvlc(psSaoLcuParam->iOffset[i]);
      }  
    }
    else
      if( psSaoLcuParam->typeIdx < 4 )
      {
        m_pcEntropyCoderIf->codeSaoUvlc( psSaoLcuParam->iOffset[0]);
        m_pcEntropyCoderIf->codeSaoUvlc( psSaoLcuParam->iOffset[1]);
        m_pcEntropyCoderIf->codeSaoUvlc(-psSaoLcuParam->iOffset[2]);
        m_pcEntropyCoderIf->codeSaoUvlc(-psSaoLcuParam->iOffset[3]);
      }
  }
}
/** Encode SAO unit
* \param  rx
* \param  ry
* \param  iCompIdx
* \param  pSaoParam
* \param  bRepeatedRow
 */
Void TEncEntropy::encodeSaoUnit(Int rx, Int ry, Int iCompIdx, SAOParam* pSaoParam, Int bRepeatedRow )
{
  int iAddr, iAddrLeft; 
  int iNumCuInWidth  = pSaoParam->iNumCuInWidth;
  SaoLcuParam* psSaoOneLcu;
  Int iRunLeft;

  iAddr      =  rx + ry*iNumCuInWidth;
  iAddrLeft  =  (iAddr%iNumCuInWidth == 0) ? -1 : iAddr - 1;

  if (!bRepeatedRow)
  {
    psSaoOneLcu = &(pSaoParam->psSaoLcuParam[iCompIdx][iAddr]);    
    iRunLeft = (iAddrLeft>=0 ) ? pSaoParam->psSaoLcuParam[iCompIdx][iAddrLeft].run : -1;
    if (rx == 0 || iRunLeft==0)
    {
      if (ry == 0)
      {
        m_pcEntropyCoderIf->codeSaoRun(psSaoOneLcu->runDiff, iNumCuInWidth-rx-1); 
        psSaoOneLcu->mergeUpFlag = 0;
      }
      else 
      {
        m_pcEntropyCoderIf->codeSaoSvlc(psSaoOneLcu->runDiff); 
        m_pcEntropyCoderIf->codeSaoFlag(psSaoOneLcu->mergeUpFlag);  
      }
      if (!psSaoOneLcu->mergeUpFlag)
      {
        encodeSaoOffset(psSaoOneLcu);
      }
    }
  }
}

/** Encode SAO unit interleaving
* \param  rx
* \param  ry
* \param  pSaoParam
* \param  pcCU
* \param  iCUAddrInSlice
* \param  iCUAddrUpInSlice
* \param  bLFCrossSliceBoundaryFlag
 */
Void TEncEntropy::encodeSaoUnitInterleaving(Int rx, Int ry, SAOParam* pSaoParam, TComDataCU* pcCU, Int iCUAddrInSlice, Int iCUAddrUpInSlice, Bool bLFCrossSliceBoundaryFlag)
{
  Int iAddr = pcCU->getAddr();
  for (Int iCompIdx=0; iCompIdx<3; iCompIdx++)
  {
    if (pSaoParam->bSaoFlag[iCompIdx])
    {
      if (rx>0 && iCUAddrInSlice!=0)
      {
      m_pcEntropyCoderIf->codeSaoMergeLeft(pSaoParam->psSaoLcuParam[iCompIdx][iAddr].mergeLeftFlag,iCompIdx);
      }
      else
      {
        pSaoParam->psSaoLcuParam[iCompIdx][iAddr].mergeLeftFlag = 0;
      }
      if (pSaoParam->psSaoLcuParam[iCompIdx][iAddr].mergeLeftFlag == 0)
      {
        if ( (ry > 0) && (iCUAddrUpInSlice||bLFCrossSliceBoundaryFlag))
        {
          m_pcEntropyCoderIf->codeSaoMergeUp(pSaoParam->psSaoLcuParam[iCompIdx][iAddr].mergeUpFlag);
        }
        else
        {
          pSaoParam->psSaoLcuParam[iCompIdx][iAddr].mergeUpFlag = 0;
        }
        if (!pSaoParam->psSaoLcuParam[iCompIdx][iAddr].mergeUpFlag)
        {
          encodeSaoOffset(&(pSaoParam->psSaoLcuParam[iCompIdx][iAddr]));
        }
      }
    }
  }
}

/** Encode SAO parameter
* \param  pcAPS
 */
Void TEncEntropy::encodeSaoParam(TComAPS* pcAPS)
{
  SaoLcuParam* psSaoOneLcu;
  int i,j,k, iCompIdx; 
  int iNumCuInWidth  ;
  int iNumCuInHeight ;
  Bool bRepeatedRow[3];
  Int iAddr;
  m_pcEntropyCoderIf->codeSaoFlag(pcAPS->getSaoInterleavingFlag());  
  if(!pcAPS->getSaoInterleavingFlag())
  {
    m_pcEntropyCoderIf->codeSaoFlag(pcAPS->getSaoEnabled());  
    if (pcAPS->getSaoEnabled())
    {
      SAOParam* pSaoParam = pcAPS->getSaoParam();
      iNumCuInWidth  = pSaoParam->iNumCuInWidth;
      iNumCuInHeight = pSaoParam->iNumCuInHeight;
      m_pcEntropyCoderIf->codeSaoFlag(pSaoParam->bSaoFlag[1]); 
      m_pcEntropyCoderIf->codeSaoFlag(pSaoParam->bSaoFlag[2]); 
      m_pcEntropyCoderIf->codeSaoUvlc(iNumCuInWidth-1); 
      m_pcEntropyCoderIf->codeSaoUvlc(iNumCuInHeight-1); 
      for (iCompIdx=0;iCompIdx<3;iCompIdx++)
      {
        if (pSaoParam->bSaoFlag[iCompIdx])
        {
          m_pcEntropyCoderIf->codeSaoFlag(pSaoParam->oneUnitFlag[iCompIdx]); 
          if (pSaoParam->oneUnitFlag[iCompIdx])
          {
            psSaoOneLcu = &(pSaoParam->psSaoLcuParam[iCompIdx][0]);   
            encodeSaoOffset(psSaoOneLcu);
          }
        }
      }

      for (j=0;j<iNumCuInHeight;j++)
      {
        for (iCompIdx=0; iCompIdx<3; iCompIdx++)
        {
          bRepeatedRow[iCompIdx] = true;
          for (k=0;k<iNumCuInWidth;k++)
          {
            iAddr       =  k + j*iNumCuInWidth;
            psSaoOneLcu = &(pSaoParam->psSaoLcuParam[iCompIdx][iAddr]);    
            if (!psSaoOneLcu->mergeUpFlag || psSaoOneLcu->runDiff)
            {
              bRepeatedRow[iCompIdx] = false;
              break;
            }
          }
        }
        for (i=0;i<iNumCuInWidth;i++)
        {
          for (iCompIdx=0; iCompIdx<3; iCompIdx++)
          {
            if (pSaoParam->bSaoFlag[iCompIdx]  && !pSaoParam->oneUnitFlag[iCompIdx]) 
            {
              if (j>0 && i==0) 
              {
                m_pcEntropyCoderIf->codeSaoFlag(bRepeatedRow[iCompIdx]); 
              }
              encodeSaoUnit (i,j, iCompIdx, pSaoParam, bRepeatedRow[iCompIdx]);
            }
          }
        }
      }
    }
  }
}
#else
/** Encode SAO for one partition
 * \param  pSaoParam, iPartIdx
 */
Void TEncEntropy::encodeSaoOnePart(SAOParam* pSaoParam, Int iPartIdx, Int iYCbCr)
{
  SAOQTPart*  pAlfPart = NULL;
  pAlfPart = &(pSaoParam->psSaoPart[iYCbCr][iPartIdx]); 

  UInt uiSymbol;

  if(!pAlfPart->bSplit)
  {
    if (pAlfPart->bEnableFlag)
    {
      uiSymbol = pAlfPart->iBestType + 1;
    }
    else
    {
      uiSymbol = 0;
    }
    
    m_pcEntropyCoderIf->codeSaoUvlc(uiSymbol);

    if (pAlfPart->bEnableFlag)
    {
      for(Int i=0; i< pAlfPart->iLength; i++)
      {
        m_pcEntropyCoderIf->codeSaoSvlc(pAlfPart->iOffset[i]);
      }   
    }
    return;
  }

  //split
  if (pAlfPart->PartLevel < pSaoParam->iMaxSplitLevel)
  {
    for (Int i=0;i<NUM_DOWN_PART;i++)
    {
      encodeSaoOnePart(pSaoParam, pAlfPart->DownPartsIdx[i], iYCbCr);
    }
  }
}

/** Encode quadtree split flag
 * \param  pSaoParam, iPartIdx
 */
Void TEncEntropy::encodeQuadTreeSplitFlag(SAOParam* pSaoParam, Int iPartIdx, Int iYCbCr)
{
  SAOQTPart*  pSaoPart = NULL;
  pSaoPart = &(pSaoParam->psSaoPart[iYCbCr][iPartIdx]);

  if(pSaoPart->PartLevel < pSaoParam->iMaxSplitLevel)
  {
    //send one flag
    m_pcEntropyCoderIf->codeSaoFlag( (pSaoPart->bSplit)?(1):(0)  );

    if(pSaoPart->bSplit)
    {
      for (Int i=0;i<NUM_DOWN_PART;i++)
      {
        encodeQuadTreeSplitFlag(pSaoParam, pSaoPart->DownPartsIdx[i], iYCbCr);
      }
    } 
  }
}
/** Encode SAO parameters
 * \param  pSaoParam
 */
Void TEncEntropy::encodeSaoParam(SAOParam* pSaoParam)
{
  if (pSaoParam->bSaoFlag[0])
  {
    encodeQuadTreeSplitFlag(pSaoParam, 0, 0);
    encodeSaoOnePart(pSaoParam, 0, 0);
    m_pcEntropyCoderIf->codeSaoFlag(pSaoParam->bSaoFlag[1]); 
    if (pSaoParam->bSaoFlag[1])
    {
      encodeQuadTreeSplitFlag(pSaoParam, 0, 1);
      encodeSaoOnePart(pSaoParam, 0, 1);
    }
    m_pcEntropyCoderIf->codeSaoFlag(pSaoParam->bSaoFlag[2]); 
    if (pSaoParam->bSaoFlag[2])
    {
      encodeQuadTreeSplitFlag(pSaoParam, 0, 2);
      encodeSaoOnePart(pSaoParam, 0, 2);
    }
  }
}
#endif

Int TEncEntropy::countNonZeroCoeffs( TCoeff* pcCoef, UInt uiSize )
{
  Int count = 0;
  
  for ( Int i = 0; i < uiSize; i++ )
  {
    count += pcCoef[i] != 0;
  }
  
  return count;
}

/** encode quantization matrix
 * \param scalingList quantization matrix information
 */
Void TEncEntropy::encodeScalingList( TComScalingList* scalingList )
{
  m_pcEntropyCoderIf->codeScalingList( scalingList );
}

Void TEncEntropy::encodeDFParams(TComAPS* pcAPS)
{
  m_pcEntropyCoderIf->codeDFFlag(pcAPS->getLoopFilterDisable(), "loop_filter_disable");

  if (!pcAPS->getLoopFilterDisable())
  {
    m_pcEntropyCoderIf->codeDFSvlc(pcAPS->getLoopFilterBetaOffset(), "beta_offset_div2");
    m_pcEntropyCoderIf->codeDFSvlc(pcAPS->getLoopFilterTcOffset(), "tc_offset_div2");
  }
}

//! \}
