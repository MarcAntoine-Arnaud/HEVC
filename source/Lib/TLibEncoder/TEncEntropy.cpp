/* ====================================================================================================================

  The copyright in this software is being made available under the License included below.
  This software may be subject to other third party and   contributor rights, including patent rights, and no such
  rights are granted under this license.

  Copyright (c) 2010, SAMSUNG ELECTRONICS CO., LTD. and BRITISH BROADCASTING CORPORATION
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted only for
  the purpose of developing standards within the Joint Collaborative Team on Video Coding and for testing and
  promoting such standards. The following conditions are required to be met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
      the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
      the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of SAMSUNG ELECTRONICS CO., LTD. nor the name of the BRITISH BROADCASTING CORPORATION
      may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * ====================================================================================================================
*/

/** \file     TEncEntropy.cpp
    \brief    entropy encoder class
*/

#include "TEncEntropy.h"

Void TEncEntropy::setEntropyCoder ( TEncEntropyIf* e, TComSlice* pcSlice )
{
  m_pcEntropyCoderIf = e;
  m_pcEntropyCoderIf->setSlice ( pcSlice );
}

Void TEncEntropy::encodeSliceHeader ( TComSlice* pcSlice )
{
  m_pcEntropyCoderIf->codeSliceHeader( pcSlice );
  return;
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
#if HHI_MRG && !SAMSUNG_MRG_SKIP_DIRECT
  if ( pcCU->getSlice()->getSPS()->getUseMRG() )
  {
    return;
  }
#endif

  if( bRD )
    uiAbsPartIdx = 0;

  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }

  m_pcEntropyCoderIf->codeSkipFlag( pcCU, uiAbsPartIdx );
}
#if QC_ALF
#include "../TLibCommon/TypeDef.h"
#include "../TLibCommon/TComAdaptiveLoopFilter.h"
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
#if ENABLE_FORCECOEFF0
  if (pAlfParam->filtNo>=0) m_pcEntropyCoderIf->codeAlfFlag(1);
  else m_pcEntropyCoderIf->codeAlfFlag(0);
#endif
  Int FiltTab[3] = {9, 7, 5};
  Int Tab = FiltTab[pAlfParam->realfiltNo];
//  m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->realfiltNo); 
  m_pcEntropyCoderIf->codeAlfUvlc((Tab-5)/2); 

  if (pAlfParam->filtNo>=0)
  {
	if(pAlfParam->realfiltNo >= 0)
	{
	  // filters_per_fr
	  m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->noFilters);

	  if(pAlfParam->noFilters == 1)
	  {
		m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->startSecondFilter);
	  }
	  else if (pAlfParam->noFilters == 2)
	  {
		for (int i=1; i<NO_VAR_BINS; i++) m_pcEntropyCoderIf->codeAlfFlag (pAlfParam->filterPattern[i]);
	  }
	}
  }
}

Int TEncEntropy::lengthGolomb(int coeffVal, int k)
{
  int m = 2 << (k - 1);
  int q = coeffVal / m;
  if(coeffVal != 0)
    return(q + 2 + k);
  else
    return(q + 1 + k);
}

Int TEncEntropy::codeFilterCoeff(ALFParam* ALFp)
{
  int filters_per_group = ALFp->filters_per_group_diff;
  int sqrFiltLength = ALFp->num_coeff;
  int filtNo = ALFp->realfiltNo;
  int flTab[]={9/2, 7/2, 5/2};
  int fl = flTab[filtNo];
  int i, k, kMin, kStart, minBits, ind, scanPos, maxScanVal, coeffVal, len = 0,
    *pDepthInt=NULL, kMinTab[MAX_SQR_FILT_LENGTH], bitsCoeffScan[MAX_SCAN_VAL][MAX_EXP_GOLOMB],
    minKStart, minBitsKStart, bitsKStart;

  pDepthInt = pDepthIntTab[fl-2];

  maxScanVal = 0;
  for(i = 0; i < sqrFiltLength; i++)
    maxScanVal = max(maxScanVal, pDepthInt[i]);

  // vlc for all
  memset(bitsCoeffScan, 0, MAX_SCAN_VAL * MAX_EXP_GOLOMB * sizeof(int));
  for(ind=0; ind<filters_per_group; ++ind){	
    for(i = 0; i < sqrFiltLength; i++){	     
      scanPos=pDepthInt[i]-1;
	  coeffVal=abs(ALFp->coeffmulti[ind][i]);
      for (k=1; k<15; k++){
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
    for(scanPos = 0; scanPos < maxScanVal; scanPos++)
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
  for(scanPos = 0; scanPos < maxScanVal; scanPos++)
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
  ALFp->maxScanVal = maxScanVal;
  for(scanPos = 0; scanPos < maxScanVal; scanPos++)
  {
     ALFp->kMinTab[scanPos] = kMinTab[scanPos];
  }
  len += writeFilterCodingParams(minKStart, maxScanVal, kMinTab);

  // Filter coefficients
  len += writeFilterCoeffs(sqrFiltLength, filters_per_group, pDepthInt, ALFp->coeffmulti, kMinTab);

  return len;
}

Int TEncEntropy::writeFilterCodingParams(int minKStart, int maxScanVal, int kMinTab[])
{
  int scanPos;
  int golombIndexBit;
  int kMin;

    // Golomb parameters
	m_pcEntropyCoderIf->codeAlfUvlc(minKStart - 1);

    kMin = minKStart; 
    for(scanPos = 0; scanPos < maxScanVal; scanPos++)
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
      golombEncode(FilterCoeff[ind][i], kMinTab[scanPos]);
    }
  }
  return 0;
}

Int TEncEntropy::golombEncode(int coeff, int k)
{
  int q, i, m;
  int symbol = abs(coeff);

  m = (int)pow(2.0, k);
  q = symbol / m;

  for (i = 0; i < q; i++)
	  m_pcEntropyCoderIf->codeAlfFlag(1);
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
#if ENABLE_FORCECOEFF0
	m_pcEntropyCoderIf->codeAlfFlag (pAlfParam->forceCoeff0);
	if (pAlfParam->forceCoeff0)
	{
	  for (int i=0; i<pAlfParam->filters_per_group; i++) m_pcEntropyCoderIf->codeAlfFlag (pAlfParam->codedVarBins[i]);
	}
#endif 
	m_pcEntropyCoderIf->codeAlfFlag (pAlfParam->predMethod);
  }
  codeFilterCoeff (pAlfParam);
}
Void  print(ALFParam* pAlfParam)
{
  Int i=0;
  Int ind=0;
  Int FiltLengthTab[] = {22, 14, 8}; //0:9tap
  Int FiltLength = FiltLengthTab[pAlfParam->realfiltNo];

  printf("set of params\n");
  printf("realfiltNo:%d\n", pAlfParam->realfiltNo);
  printf("filtNo:%d\n", pAlfParam->filtNo);
  printf("filterPattern:");
  for (i=0; i<NO_VAR_BINS; i++) printf("%d ", pAlfParam->filterPattern[i]);
  printf("\n");
  
  printf("startSecondFilter:%d\n", pAlfParam->startSecondFilter);
  printf("noFilters:%d\n", pAlfParam->noFilters);
  printf("varIndTab:");
  for (i=0; i<NO_VAR_BINS; i++) printf("%d ", pAlfParam->varIndTab[i]);
  printf("\n");
  printf("filters_per_group_diff:%d\n", pAlfParam->filters_per_group_diff);
  printf("filters_per_group:%d\n", pAlfParam->filters_per_group);
  printf("codedVarBins:");
  for (i=0; i<NO_VAR_BINS; i++) printf("%d ", pAlfParam->codedVarBins[i]);
  printf("\n");
  printf("forceCoeff0:%d\n", pAlfParam->forceCoeff0);
  printf("predMethod:%d\n", pAlfParam->predMethod);

  for (ind=0; ind<pAlfParam->filters_per_group_diff; ind++)
  {
    printf("coeffmulti(%d):", ind);
	for (i=0; i<FiltLength; i++) printf("%d ", pAlfParam->coeffmulti[ind][i]);
	printf("\n");
  }

  printf("minKStart:%d\n", pAlfParam->minKStart);  
  printf("maxScanVal:%d\n", pAlfParam->maxScanVal);  
  printf("kMinTab:");
  for(Int scanPos = 0; scanPos < pAlfParam->maxScanVal; scanPos++)
  {
     printf("%d ", pAlfParam->kMinTab[scanPos]);
  }
  printf("\n");

  printf("chroma_idc:%d\n", pAlfParam->chroma_idc);  
  printf("tap_chroma:%d\n", pAlfParam->tap_chroma);  
  printf("chroma_coeff:");
  for(Int scanPos = 0; scanPos < pAlfParam->num_coeff_chroma; scanPos++)
  {
     printf("%d ", pAlfParam->coeff_chroma[scanPos]);
  }
  printf("\n");
}
#endif


#if HHI_MRG
#if HHI_MRG_PU
Void TEncEntropy::encodeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{ 
  TComMvField cMvFieldNeighbours[4]; // above ref_list_0, above ref_list_1, left ref_list_0, left ref_list_1
  UInt uiNeighbourInfo;
  UChar uhInterDirNeighbours[2];
#ifdef DCM_PBIC
  TComIc cIcNeighbours[2]; //above, left
  pcCU->getInterMergeCandidates( uiAbsPartIdx, cMvFieldNeighbours, cIcNeighbours, uhInterDirNeighbours, uiNeighbourInfo );
#else
  pcCU->getInterMergeCandidates( uiAbsPartIdx, cMvFieldNeighbours, uhInterDirNeighbours, uiNeighbourInfo );
#endif

  if ( uiNeighbourInfo )
  {
    // at least one merge candidate exists
    m_pcEntropyCoderIf->codeMergeFlag( pcCU, uiAbsPartIdx );
  }
  else
  {
    assert( !pcCU->getMergeFlag( uiAbsPartIdx ) );
  }
}

Void TEncEntropy::encodeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  TComMvField cMvFieldNeighbours[4]; // above ref_list_0, above ref_list_1, left ref_list_0, left ref_list_1
  UInt uiNeighbourInfo;
  UChar uhInterDirNeighbours[2];
#ifdef DCM_PBIC
  TComIc cIcNeighbours[2]; //above, left
  pcCU->getInterMergeCandidates( uiAbsPartIdx, cMvFieldNeighbours, cIcNeighbours, uhInterDirNeighbours, uiNeighbourInfo );
#else
  pcCU->getInterMergeCandidates( uiAbsPartIdx, cMvFieldNeighbours, uhInterDirNeighbours, uiNeighbourInfo );
#endif

  UInt uiMergeIndex = 0;
  if ( uiNeighbourInfo == 3 ) 
  {
    m_pcEntropyCoderIf->codeMergeIndex( pcCU, uiAbsPartIdx );
  }
}

#else
Void TEncEntropy::encodeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }
  
  m_pcEntropyCoderIf->codeMergeFlag( pcCU, uiAbsPartIdx );
}
  
Void TEncEntropy::encodeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  m_pcEntropyCoderIf->codeMergeIndex( pcCU, uiAbsPartIdx );
}
#endif
#endif

Void TEncEntropy::encodeAlfParam(ALFParam* pAlfParam)
{
#if HHI_ALF
  m_pcEntropyCoderIf->codeAlfFlag(pAlfParam->alf_flag);
  if (!pAlfParam->alf_flag)
    return;
  Int pos;
  Int iCenterPos ;

  // filter parameters for luma
  // horizontal filter
  AlfFilter *pHorizontalFilter = &(pAlfParam->acHorizontalAlfFilter[0]) ;
  iCenterPos = ( pHorizontalFilter->iFilterSymmetry == 0 ) ? (pHorizontalFilter->iFilterLength + 1) >> 1 : pHorizontalFilter->iNumOfCoeffs - 1 ;

  m_pcEntropyCoderIf->codeAlfUvlc( (pHorizontalFilter->iFilterLength-ALF_MIN_LENGTH)/2 );
  m_pcEntropyCoderIf->codeAlfUvlc( pHorizontalFilter->iFilterSymmetry );

  Int iCoeff;
  for(pos=0; pos < pHorizontalFilter->iNumOfCoeffs; pos++)
  {
    iCoeff = pHorizontalFilter->aiQuantFilterCoeffs[pos] ;
    m_pcEntropyCoderIf->codeAlfCoeff(iCoeff,pHorizontalFilter->iFilterLength, pos );
  }
#if ALF_DC_CONSIDERED
  m_pcEntropyCoderIf->codeAlfDc( pHorizontalFilter->aiQuantFilterCoeffs[ pHorizontalFilter->iNumOfCoeffs ] );
#endif
  // vertical filter
  AlfFilter *pVerticalFilter = &(pAlfParam->acVerticalAlfFilter[0]) ;

  m_pcEntropyCoderIf->codeAlfUvlc( (pVerticalFilter->iFilterLength-ALF_MIN_LENGTH)/2 );
  m_pcEntropyCoderIf->codeAlfUvlc( pVerticalFilter->iFilterSymmetry );

  iCenterPos = ( pVerticalFilter->iFilterSymmetry == 0 ) ? (pVerticalFilter->iFilterLength + 1) >> 1 : pVerticalFilter->iNumOfCoeffs - 1 ;
  for(pos=0; pos < pVerticalFilter->iNumOfCoeffs ; pos++ )
  {
    iCoeff = pVerticalFilter->aiQuantFilterCoeffs[pos] ;
    m_pcEntropyCoderIf->codeAlfCoeff(iCoeff,pVerticalFilter->iFilterLength, pos );
  }
#if ALF_DC_CONSIDERED
  m_pcEntropyCoderIf->codeAlfDc( pVerticalFilter->aiQuantFilterCoeffs[ pVerticalFilter->iNumOfCoeffs ] );
#endif
  // filter parameters for chroma
  m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->chroma_idc);
  for(Int iPlane = 1; iPlane <3; iPlane++)
  {
    if(pAlfParam->chroma_idc&iPlane)
    {
      m_pcEntropyCoderIf->codeAlfUvlc( pAlfParam->aiPlaneFilterMapping[iPlane] ) ;
      if( pAlfParam->aiPlaneFilterMapping[iPlane] == iPlane )
      {
        // horizontal filter
        pHorizontalFilter = &(pAlfParam->acHorizontalAlfFilter[iPlane]) ;
        iCenterPos = ( pHorizontalFilter->iFilterSymmetry == 0 ) ? (pHorizontalFilter->iFilterLength + 1) >> 1 : pHorizontalFilter->iNumOfCoeffs - 1 ;

        m_pcEntropyCoderIf->codeAlfUvlc( (pHorizontalFilter->iFilterLength-ALF_MIN_LENGTH)/2 );
        m_pcEntropyCoderIf->codeAlfUvlc( pHorizontalFilter->iFilterSymmetry );

        for(pos=0; pos < pHorizontalFilter->iNumOfCoeffs; pos++)
        {
          iCoeff = pHorizontalFilter->aiQuantFilterCoeffs[pos] ;
          m_pcEntropyCoderIf->codeAlfCoeff(iCoeff,pHorizontalFilter->iFilterLength, pos );
        }
#if ALF_DC_CONSIDERED
        m_pcEntropyCoderIf->codeAlfDc( pHorizontalFilter->aiQuantFilterCoeffs[ pHorizontalFilter->iNumOfCoeffs ] );
#endif
        // vertical filter
        pVerticalFilter = &(pAlfParam->acVerticalAlfFilter[iPlane]) ;

        m_pcEntropyCoderIf->codeAlfUvlc( (pVerticalFilter->iFilterLength-ALF_MIN_LENGTH)/2 );
        m_pcEntropyCoderIf->codeAlfUvlc( pVerticalFilter->iFilterSymmetry );

        iCenterPos = ( pVerticalFilter->iFilterSymmetry == 0 ) ? (pVerticalFilter->iFilterLength + 1) >> 1 : pVerticalFilter->iNumOfCoeffs - 1 ;
        for(pos=0; pos < pVerticalFilter->iNumOfCoeffs ; pos++ )
        {
          iCoeff = pVerticalFilter->aiQuantFilterCoeffs[pos] ;
    			m_pcEntropyCoderIf->codeAlfCoeff(iCoeff,pVerticalFilter->iFilterLength, pos );
        }
#if ALF_DC_CONSIDERED
        m_pcEntropyCoderIf->codeAlfDc( pVerticalFilter->aiQuantFilterCoeffs[ pVerticalFilter->iNumOfCoeffs ] );
#endif
      }
    }
  }

  // region control parameters for luma
  m_pcEntropyCoderIf->codeAlfFlag(pAlfParam->cu_control_flag);
  if (pAlfParam->cu_control_flag)
  {
    assert( (pAlfParam->cu_control_flag && m_pcEntropyCoderIf->getAlfCtrl()) || (!pAlfParam->cu_control_flag && !m_pcEntropyCoderIf->getAlfCtrl()));
    m_pcEntropyCoderIf->codeAlfFlag( pAlfParam->bSeparateQt );
    m_pcEntropyCoderIf->codeAlfCtrlDepth();
  }
#else
  m_pcEntropyCoderIf->codeAlfFlag(pAlfParam->alf_flag);
  if (!pAlfParam->alf_flag)
    return;
  Int pos;
#if QC_ALF  
  codeAux(pAlfParam);
  codeFilt(pAlfParam);
#else
  // filter parameters for luma
  m_pcEntropyCoderIf->codeAlfUvlc((pAlfParam->tap-5)/2);
  for(pos=0; pos<pAlfParam->num_coeff; pos++)
  {
    m_pcEntropyCoderIf->codeAlfSvlc(pAlfParam->coeff[pos]);
  }
#endif

  // filter parameters for chroma
  m_pcEntropyCoderIf->codeAlfUvlc(pAlfParam->chroma_idc);
  if(pAlfParam->chroma_idc)
  {
    m_pcEntropyCoderIf->codeAlfUvlc((pAlfParam->tap_chroma-5)/2);

    // filter coefficients for chroma
    for(pos=0; pos<pAlfParam->num_coeff_chroma; pos++)
    {
      m_pcEntropyCoderIf->codeAlfSvlc(pAlfParam->coeff_chroma[pos]);
    }
  }

  // region control parameters for luma
  m_pcEntropyCoderIf->codeAlfFlag(pAlfParam->cu_control_flag);
  if (pAlfParam->cu_control_flag)
  {
    assert( (pAlfParam->cu_control_flag && m_pcEntropyCoderIf->getAlfCtrl()) || (!pAlfParam->cu_control_flag && !m_pcEntropyCoderIf->getAlfCtrl()));
    m_pcEntropyCoderIf->codeAlfCtrlDepth();
  }
#endif
}

#if HHI_ALF
Void TEncEntropy::encodeAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD, Bool bSeparateQt )
{
  if( bRD )
    uiAbsPartIdx = 0;

  if( bSeparateQt )
  {
    m_pcEntropyCoderIf->codeAlfQTCtrlFlag( pcCU, uiAbsPartIdx );
  }
  else
  {
    m_pcEntropyCoderIf->codeAlfCtrlFlag( pcCU, uiAbsPartIdx );
  }
}

Void TEncEntropy::encodeAlfQTSplitFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiMaxDepth, Bool bRD )
{
  if( bRD )
      uiAbsPartIdx = 0;

  m_pcEntropyCoderIf->codeAlfQTSplitFlag( pcCU, uiAbsPartIdx, uiDepth, uiMaxDepth );
}
#else
Void TEncEntropy::encodeAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  m_pcEntropyCoderIf->codeAlfCtrlFlag( pcCU, uiAbsPartIdx );
}
#endif

#if TSB_ALF_HEADER
Void TEncEntropy::encodeAlfCtrlParam( ALFParam* pAlfParam )
{
  m_pcEntropyCoderIf->codeAlfFlagNum( pAlfParam->num_alf_cu_flag, pAlfParam->num_cus_in_frame );

  for(UInt i=0; i<pAlfParam->num_alf_cu_flag; i++)
  {
    m_pcEntropyCoderIf->codeAlfCtrlFlag( pAlfParam->alf_cu_flag[i] );
  }
}
#endif

Void TEncEntropy::encodePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }

#if HHI_MRG && !HHI_MRG_PU
  if ( pcCU->getMergeFlag( uiAbsPartIdx ) )
  {
    return;
  }
#endif

  if (pcCU->isSkipped( uiAbsPartIdx ))
    return;

  m_pcEntropyCoderIf->codePredMode( pcCU, uiAbsPartIdx );
}

// Split mode
Void TEncEntropy::encodeSplitFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  m_pcEntropyCoderIf->codeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
}

Void TEncEntropy::encodePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

#if HHI_MRG && !HHI_MRG_PU
  if ( pcCU->getMergeFlag( uiAbsPartIdx ) )
  {
    return;
  }
#endif

  if ( pcCU->isSkip( uiAbsPartIdx ) )
    return;

  m_pcEntropyCoderIf->codePartSize( pcCU, uiAbsPartIdx, uiDepth );
}

#if HHI_RQT

#if MS_LAST_CBF

Void TEncEntropy::xEncodeTransformSubdiv( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiInnerQuadIdx, UInt& uiYCbfFront3, UInt& uiUCbfFront3, UInt& uiVCbfFront3 )
{
  const UInt uiSubdiv = pcCU->getTransformIdx( uiAbsPartIdx ) + pcCU->getDepth( uiAbsPartIdx ) > uiDepth;
  const UInt uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()]+2 - uiDepth;

#if HHI_RQT_INTRA
  if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN && uiDepth == pcCU->getDepth(uiAbsPartIdx) )
  {
    assert( uiSubdiv );
  }
  else
#endif
  if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    assert( uiSubdiv );
  }
  else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    assert( !uiSubdiv );
  }
#if HHI_RQT_DEPTH || HHI_RQT_DISABLE_SUB
  else if( uiLog2TrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
  {
    assert( !uiSubdiv );
  }
#endif  
  else
  {
#if HHI_RQT_DEPTH || HHI_RQT_DISABLE_SUB
    assert( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
#else  
    assert( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() );
#endif
#if HHI_RQT_FORCE_SPLIT_ACC2_PU
    UInt uiCtx = uiDepth;
    const UInt uiTrMode = uiDepth - pcCU->getDepth( uiAbsPartIdx );
#if HHI_RQT_FORCE_SPLIT_NxN
   const Bool bNxNOK         = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN && uiTrMode > 0;
#else
   const Bool bNxNOK         = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN;
#endif
#if HHI_RQT_FORCE_SPLIT_RECT
   const Bool bSymmetricOK   = pcCU->getPartitionSize( uiAbsPartIdx ) >= SIZE_2NxN  && pcCU->getPartitionSize( uiAbsPartIdx ) < SIZE_NxN   && uiTrMode > 0;
#else
   const Bool bSymmetricOK   = pcCU->getPartitionSize( uiAbsPartIdx ) >= SIZE_2NxN  && pcCU->getPartitionSize( uiAbsPartIdx ) < SIZE_NxN;
#endif
#if HHI_RQT_FORCE_SPLIT_ASYM
  const Bool bAsymmetricOK   = pcCU->getPartitionSize( uiAbsPartIdx ) >= SIZE_2NxnU && pcCU->getPartitionSize( uiAbsPartIdx ) <= SIZE_nRx2N && uiTrMode > 1;
  const Bool b2NxnUOK        = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2NxnU && uiInnerQuadIdx > 1      && uiTrMode == 1;
  const Bool b2NxnDOK        = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2NxnD && uiInnerQuadIdx < 2      && uiTrMode == 1;
  const Bool bnLx2NOK        = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_nLx2N &&  ( uiInnerQuadIdx & 1 ) && uiTrMode == 1;
  const Bool bnRx2NOK        = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_nRx2N && !( uiInnerQuadIdx & 1 ) && uiTrMode == 1;
  const Bool bNeedSubdivFlag = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N || pcCU->getPredictionMode( uiAbsPartIdx ) == MODE_INTRA ||
                               bNxNOK || bSymmetricOK || bAsymmetricOK || b2NxnUOK || b2NxnDOK || bnLx2NOK || bnRx2NOK;
#else
  const Bool bAsymmetricOK   = pcCU->getPartitionSize( uiAbsPartIdx ) >= SIZE_2NxnU && pcCU->getPartitionSize( uiAbsPartIdx ) <= SIZE_nRx2N;
  const Bool bNeedSubdivFlag = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N || pcCU->getPredictionMode( uiAbsPartIdx ) == MODE_INTRA ||
                               bNxNOK || bSymmetricOK || bAsymmetricOK ;
#endif
    if( bNeedSubdivFlag )
    {
      m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSubdiv, uiCtx );
    }
#else
    m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSubdiv, uiDepth );
#endif
  }

#if LCEC_CBP_YUV_ROOT
  if(pcCU->getSlice()->getSymbolMode() == 0)
  {
    if( uiSubdiv )
    {
      ++uiDepth;
      const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
      UInt uiDummyCbfY = 0;
      UInt uiDummyCbfU = 0;
      UInt uiDummyCbfV = 0;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0, uiDummyCbfY, uiDummyCbfU, uiDummyCbfV );
      uiAbsPartIdx += uiQPartNum;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 1, uiDummyCbfY, uiDummyCbfU, uiDummyCbfV );
      uiAbsPartIdx += uiQPartNum;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 2, uiDummyCbfY, uiDummyCbfU, uiDummyCbfV );
      uiAbsPartIdx += uiQPartNum;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 3, uiDummyCbfY, uiDummyCbfU, uiDummyCbfV );
    }
  }
  else
  {
#endif
#if HHI_RQT_CHROMA_CBF_MOD
  if( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA && uiLog2TrafoSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    const UInt uiTrDepthCurr = uiDepth - pcCU->getDepth( uiAbsPartIdx );
    const Bool bFirstCbfOfCU = uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() || uiTrDepthCurr == 0;
    if( bFirstCbfOfCU || uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
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
    else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
    {
      assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr - 1 ) );
      assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr - 1 ) );

      uiUCbfFront3 += pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr );
      uiVCbfFront3 += pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr );
    }
  }
#endif

  if( uiSubdiv )
  {
    ++uiDepth;
    const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);

    UInt uiCurrentCbfY = 0;
    UInt uiCurrentCbfU = 0;
    UInt uiCurrentCbfV = 0;

    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );
    uiAbsPartIdx += uiQPartNum;
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 1, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );
    uiAbsPartIdx += uiQPartNum;
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 2, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );
    uiAbsPartIdx += uiQPartNum;
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 3, uiCurrentCbfY, uiCurrentCbfU, uiCurrentCbfV );

    uiYCbfFront3 += uiCurrentCbfY;
    uiUCbfFront3 += uiCurrentCbfU;
    uiVCbfFront3 += uiCurrentCbfV;
  }
  else
  {
    {
      DTRACE_CABAC_V( g_nSymbolCounter++ );
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
#if HHI_RQT_ROOT && HHI_RQT_CHROMA_CBF_MOD
    if( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA && uiDepth == pcCU->getDepth( uiAbsPartIdx ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, 0 ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, 0 ) )
    {
      assert( pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, 0 ) );
      //      printf( "saved one bin! " );
    }
    else
#endif
    if ( pcCU->getPredictionMode( uiAbsPartIdx ) != MODE_INTRA && uiInnerQuadIdx == 3 && uiYCbfFront3 == 0 && uiUCbfFront3 == 0 && uiVCbfFront3 == 0
         && uiLog2TrafoSize < pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiLumaTrMode ) )
    {      
      uiYCbfFront3++;
    }    
    else
    {
      m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, uiLumaTrMode );
      uiYCbfFront3 += pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiLumaTrMode );
    }
#if HHI_RQT_CHROMA_CBF_MOD
    if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA )
#endif
    {
      Bool bCodeChroma = true;
      if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
      {
        UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
        bCodeChroma  = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
      }
      if( bCodeChroma )
      {
        m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiChromaTrMode );
        m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiChromaTrMode );
      }
    }
  }
#if LCEC_CBP_YUV_ROOT
  }
#endif
}
#else

Void TEncEntropy::xEncodeTransformSubdiv( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiInnerQuadIdx )
{
  const UInt uiSubdiv = pcCU->getTransformIdx( uiAbsPartIdx ) + pcCU->getDepth( uiAbsPartIdx ) > uiDepth;
  const UInt uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()]+2 - uiDepth;

#if HHI_RQT_INTRA
  if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN && uiDepth == pcCU->getDepth(uiAbsPartIdx) )
  {
    assert( uiSubdiv );
  }
  else
#endif
  if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    assert( uiSubdiv );
  }
  else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    assert( !uiSubdiv );
  }
#if HHI_RQT_DEPTH || HHI_RQT_DISABLE_SUB
  else if( uiLog2TrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
  {
    assert( !uiSubdiv );
  }
#endif  
  else
  {
#if HHI_RQT_DEPTH || HHI_RQT_DISABLE_SUB
    assert( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
#else  
    assert( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() );
#endif
#if HHI_RQT_FORCE_SPLIT_ACC2_PU
    UInt uiCtx = uiDepth;
    const UInt uiTrMode = uiDepth - pcCU->getDepth( uiAbsPartIdx );
#if HHI_RQT_FORCE_SPLIT_NxN
   const Bool bNxNOK         = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN && uiTrMode > 0;
#else
   const Bool bNxNOK         = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN;
#endif
#if HHI_RQT_FORCE_SPLIT_RECT
   const Bool bSymmetricOK   = pcCU->getPartitionSize( uiAbsPartIdx ) >= SIZE_2NxN  && pcCU->getPartitionSize( uiAbsPartIdx ) < SIZE_NxN   && uiTrMode > 0;
#else
   const Bool bSymmetricOK   = pcCU->getPartitionSize( uiAbsPartIdx ) >= SIZE_2NxN  && pcCU->getPartitionSize( uiAbsPartIdx ) < SIZE_NxN;
#endif
#if HHI_RQT_FORCE_SPLIT_ASYM
  const Bool bAsymmetricOK   = pcCU->getPartitionSize( uiAbsPartIdx ) >= SIZE_2NxnU && pcCU->getPartitionSize( uiAbsPartIdx ) <= SIZE_nRx2N && uiTrMode > 1;
  const Bool b2NxnUOK        = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2NxnU && uiInnerQuadIdx > 1      && uiTrMode == 1;
  const Bool b2NxnDOK        = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2NxnD && uiInnerQuadIdx < 2      && uiTrMode == 1;
  const Bool bnLx2NOK        = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_nLx2N &&  ( uiInnerQuadIdx & 1 ) && uiTrMode == 1;
  const Bool bnRx2NOK        = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_nRx2N && !( uiInnerQuadIdx & 1 ) && uiTrMode == 1;
  const Bool bNeedSubdivFlag = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N || pcCU->getPredictionMode( uiAbsPartIdx ) == MODE_INTRA ||
                               bNxNOK || bSymmetricOK || bAsymmetricOK || b2NxnUOK || b2NxnDOK || bnLx2NOK || bnRx2NOK;
#else
  const Bool bAsymmetricOK   = pcCU->getPartitionSize( uiAbsPartIdx ) >= SIZE_2NxnU && pcCU->getPartitionSize( uiAbsPartIdx ) <= SIZE_nRx2N;
  const Bool bNeedSubdivFlag = pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N || pcCU->getPredictionMode( uiAbsPartIdx ) == MODE_INTRA ||
                               bNxNOK || bSymmetricOK || bAsymmetricOK ;
#endif
    if( bNeedSubdivFlag )
    {
      m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSubdiv, uiCtx );
    }
#else
    m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSubdiv, uiDepth );
#endif
  }

#if LCEC_CBP_YUV_ROOT
  if(pcCU->getSlice()->getSymbolMode() == 0)
  {
    if( uiSubdiv )
    {
      ++uiDepth;
      const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0 );
      uiAbsPartIdx += uiQPartNum;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 1 );
      uiAbsPartIdx += uiQPartNum;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 2 );
      uiAbsPartIdx += uiQPartNum;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 3 );
    }
  }
  else
  {
#endif
#if HHI_RQT_CHROMA_CBF_MOD
  if( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA && uiLog2TrafoSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    const UInt uiTrDepthCurr = uiDepth - pcCU->getDepth( uiAbsPartIdx );
    const Bool bFirstCbfOfCU = uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() || uiTrDepthCurr == 0;
    if( bFirstCbfOfCU || uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
    {
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr - 1 ) )
      {
        m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr );
      }
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr - 1 ) )
      {
        m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr );
      }
    }
    else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
    {
      assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr - 1 ) );
      assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr - 1 ) );
    }
  }
#endif

  if( uiSubdiv )
  {
    ++uiDepth;
    const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0 );
    uiAbsPartIdx += uiQPartNum;
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 1 );
    uiAbsPartIdx += uiQPartNum;
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 2 );
    uiAbsPartIdx += uiQPartNum;
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 3 );
  }
  else
  {
    {
      DTRACE_CABAC_V( g_nSymbolCounter++ );
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
#if HHI_RQT_ROOT && HHI_RQT_CHROMA_CBF_MOD
    if( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA && uiDepth == pcCU->getDepth( uiAbsPartIdx ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, 0 ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, 0 ) )
    {
      assert( pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, 0 ) );
      //      printf( "saved one bin! " );
    }
    else
#endif
    m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, uiLumaTrMode );
#if HHI_RQT_CHROMA_CBF_MOD
    if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA )
#endif
    {
      Bool bCodeChroma = true;
      if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
      {
        UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
        bCodeChroma  = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
      }
      if( bCodeChroma )
      {
        m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiChromaTrMode );
        m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiChromaTrMode );
      }
    }
  }
#if LCEC_CBP_YUV_ROOT
  }
#endif
}
#endif
#endif

// transform index
Void TEncEntropy::encodeTransformIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
#if HHI_RQT
  assert( !bRD ); // parameter bRD can be removed
#endif
  if( bRD )
    uiAbsPartIdx = 0;

#if HHI_RQT
  if( pcCU->getSlice()->getSPS()->getQuadtreeTUFlag() )
  {
    DTRACE_CABAC_V( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tdecodeTransformIdx()\tCUDepth=" )
    DTRACE_CABAC_V( uiDepth )
    DTRACE_CABAC_T( "\n" )
#if MS_LAST_CBF
    UInt temp = 0;
    UInt temp1 = 0;
    UInt temp2 = 0;
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0, temp, temp1, temp2 );
#else
    xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0 );
#endif
  }
  else
#endif
  m_pcEntropyCoderIf->codeTransformIdx( pcCU, uiAbsPartIdx, uiDepth );
}

// ROT index
Void TEncEntropy::encodeROTindex  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

    if (pcCU->getPredictionMode( uiAbsPartIdx )==MODE_INTRA)
    {
      if( ( pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA) + pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U) + pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V) ) == 0 )
      {
        return;
      }
      m_pcEntropyCoderIf->codeROTindex( pcCU, uiAbsPartIdx, bRD );
  }
}

// CIP index
Void TEncEntropy::encodeCIPflag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
  uiAbsPartIdx = 0;

  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
    m_pcEntropyCoderIf->codeCIPflag( pcCU, uiAbsPartIdx, bRD );
  }
}

// Intra direction for Luma
Void TEncEntropy::encodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
#if ANG_INTRA
  if ( pcCU->angIntraEnabledPredPart(uiAbsPartIdx) )
    m_pcEntropyCoderIf->codeIntraDirLumaAng( pcCU, uiAbsPartIdx );
  else
    m_pcEntropyCoderIf->codeIntraDirLumaAdi( pcCU, uiAbsPartIdx );
#else
  m_pcEntropyCoderIf->codeIntraDirLumaAdi( pcCU, uiAbsPartIdx );
#endif
}

#if PLANAR_INTRA
Void TEncEntropy::encodePlanarInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( pcCU->getSlice()->isInterB() )
    return;

  if( bRD )
    uiAbsPartIdx = 0;

  m_pcEntropyCoderIf->codePlanarInfo( pcCU, uiAbsPartIdx );
}
#endif

#if HHI_AIS
// BB: Intra ref. samples filtering for Luma
Void TEncEntropy::encodeIntraFiltFlagLuma ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // DC (mode 2) always uses DEFAULT_IS so no signaling needed
  // (no g_aucIntraModeOrder[][] mapping needed because mode 2 always mapped to 2)
  if( (pcCU->getSlice()->getSPS()->getUseAIS()) && (pcCU->getLumaIntraDir( uiAbsPartIdx ) != 2) )
    m_pcEntropyCoderIf->codeIntraFiltFlagLumaAdi( pcCU, uiAbsPartIdx );
}
#endif

// Intra direction for Chroma
Void TEncEntropy::encodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  m_pcEntropyCoderIf->codeIntraDirChroma( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::encodePredInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

#if HHI_MRG && !HHI_MRG_PU
  if ( pcCU->getMergeFlag( uiAbsPartIdx ) )
  {
    return;
  }
#endif

  if (pcCU->isSkip( uiAbsPartIdx ))
  {
    if (pcCU->getSlice()->isInterB())
    {
      encodeInterDir(pcCU, uiAbsPartIdx, bRD);
    }
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) //if ( ref. frame list0 has at least 1 entry )
    {
      encodeMVPIdx( pcCU, uiAbsPartIdx, REF_PIC_LIST_0);
    }
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) //if ( ref. frame list1 has at least 1 entry )
    {
      encodeMVPIdx( pcCU, uiAbsPartIdx, REF_PIC_LIST_1);
    }
#ifdef DCM_PBIC
    if (pcCU->getSlice()->getSPS()->getUseIC())
    {
      encodeICPIdx( pcCU, uiAbsPartIdx );
    }
#endif
    return;
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
#if HHI_AIS
      //BB: intra ref. samples filtering flag
      encodeIntraFiltFlagLuma( pcCU, uiAbsPartIdx                  );
      encodeIntraFiltFlagLuma( pcCU, uiAbsPartIdx + uiPartOffset   );
      encodeIntraFiltFlagLuma( pcCU, uiAbsPartIdx + uiPartOffset*2 );
      encodeIntraFiltFlagLuma( pcCU, uiAbsPartIdx + uiPartOffset*3 );
      //
#endif
      encodeIntraDirModeChroma( pcCU, uiAbsPartIdx, bRD );
    }
    else                                                              // if it is not NxN size, encode 1 intra directions
    {
      encodeIntraDirModeLuma  ( pcCU, uiAbsPartIdx );
#if HHI_AIS
      //BB: intra ref. samples filtering flag
      encodeIntraFiltFlagLuma ( pcCU, uiAbsPartIdx );
      //
#endif
      encodeIntraDirModeChroma( pcCU, uiAbsPartIdx, bRD );
    }
  }
  else                                                                // if it is Inter mode, encode motion vector and reference index
  {
#if HHI_MRG_PU
    if ( pcCU->getSlice()->getSPS()->getUseMRG() )
    {
      encodePUWise( pcCU, uiAbsPartIdx, bRD );
    }
    else
#endif
    {
      encodeInterDir( pcCU, uiAbsPartIdx, bRD );

#ifdef DCM_PBIC
      if (pcCU->getSlice()->getSPS()->getUseIC())
      {
        if ( ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) && ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) )
        {
          // Both ref. frame list0 & ref. frame list1 have at least 1 entry each
          encodeRefFrmIdx ( pcCU, uiAbsPartIdx, REF_PIC_LIST_0, bRD );
          encodeRefFrmIdx ( pcCU, uiAbsPartIdx, REF_PIC_LIST_1, bRD );
          encodeMvdIcd    ( pcCU, uiAbsPartIdx, REF_PIC_LIST_X, bRD );
          encodeMVPIdx    ( pcCU, uiAbsPartIdx, REF_PIC_LIST_0      );
          encodeMVPIdx    ( pcCU, uiAbsPartIdx, REF_PIC_LIST_1      );
        }
        else if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
        {
          // Only ref. frame list0 has at least 1 entry
          encodeRefFrmIdx ( pcCU, uiAbsPartIdx, REF_PIC_LIST_0, bRD );
          encodeMvdIcd    ( pcCU, uiAbsPartIdx, REF_PIC_LIST_0, bRD );
          encodeMVPIdx    ( pcCU, uiAbsPartIdx, REF_PIC_LIST_0      );
        }
        else if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
        {
          // Only ref. frame list1 has at least 1 entry
          encodeRefFrmIdx ( pcCU, uiAbsPartIdx, REF_PIC_LIST_1, bRD );
          encodeMvdIcd    ( pcCU, uiAbsPartIdx, REF_PIC_LIST_1, bRD );
          encodeMVPIdx    ( pcCU, uiAbsPartIdx, REF_PIC_LIST_1      );
        }
        if ( ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) || ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) )
        {
          encodeICPIdx  ( pcCU, uiAbsPartIdx                      );
        }
      }
      else
#endif
      {
        if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )       // if ( ref. frame list0 has at least 1 entry )
        {
          encodeRefFrmIdx ( pcCU, uiAbsPartIdx, REF_PIC_LIST_0, bRD );
          encodeMvd       ( pcCU, uiAbsPartIdx, REF_PIC_LIST_0, bRD );
          encodeMVPIdx    ( pcCU, uiAbsPartIdx, REF_PIC_LIST_0      );
        }

        if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )       // if ( ref. frame list1 has at least 1 entry )
        {
          encodeRefFrmIdx ( pcCU, uiAbsPartIdx, REF_PIC_LIST_1, bRD );
          encodeMvd       ( pcCU, uiAbsPartIdx, REF_PIC_LIST_1, bRD );
          encodeMVPIdx    ( pcCU, uiAbsPartIdx, REF_PIC_LIST_1      );
        }
      }
    }
  }
}

#if HHI_MRG && !HHI_MRG_PU
Void TEncEntropy::encodeMergeInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( !pcCU->getSlice()->getSPS()->getUseMRG() )
  {
    return;
  }

  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }

#if SAMSUNG_MRG_SKIP_DIRECT
  if ( pcCU->isSkipped(uiAbsPartIdx) )
  {
    return;
  }  
#endif

  if( bRD )
    uiAbsPartIdx = 0;

  // find left and top vectors. take vectors from PUs to the left and above.
  TComMvField cMvFieldNeighbours[4]; // above ref_list_0, above ref_list_1, left ref_list_0, left ref_list_1
  UInt uiNeighbourInfo;
  UChar uhInterDirNeighbours[2];
#ifdef DCM_PBIC
  TComIc cIcNeighbour[2]; //above 0, left 1
  pcCU->getInterMergeCandidates( uiAbsPartIdx, cMvFieldNeighbours,cIcNeighbour,uhInterDirNeighbours, uiNeighbourInfo );
#else
  pcCU->getInterMergeCandidates( uiAbsPartIdx, cMvFieldNeighbours, uhInterDirNeighbours, uiNeighbourInfo );
#endif
  if ( uiNeighbourInfo )
  {
    // at least one merge candidate exists
    encodeMergeFlag( pcCU, uiAbsPartIdx, bRD );
  }
  else
  {
    assert( !pcCU->getMergeFlag( uiAbsPartIdx ) );
  }

  if ( !pcCU->getMergeFlag( uiAbsPartIdx ) )
  {
    // CU is not merged
    return;
  }

  if ( uiNeighbourInfo == 3 )
  {
    // different merge candidates exist. write Merge Index
    encodeMergeIndex( pcCU, uiAbsPartIdx, bRD );
  }
  
}
#endif

Void TEncEntropy::encodeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );
  assert( !pcCU->isSkip( uiAbsPartIdx ) || pcCU->getSlice()->isInterB());

  if( bRD )
    uiAbsPartIdx = 0;

  if ( !pcCU->getSlice()->isInterB() )
  {
    return;
  }

  UInt uiPartOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;

  switch ( pcCU->getPartitionSize( uiAbsPartIdx ) )
  {
  case SIZE_2Nx2N:
    {
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
      break;
    }

  case SIZE_2NxN:
    {
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
      uiAbsPartIdx += uiPartOffset << 1;
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
      break;
    }

  case SIZE_Nx2N:
    {
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
      uiAbsPartIdx += uiPartOffset;
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
      break;
    }

  case SIZE_NxN:
    {
      for ( Int iPartIdx = 0; iPartIdx < 4; iPartIdx++ )
      {
        m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
        uiAbsPartIdx += uiPartOffset;
      }
      break;
    }
  case SIZE_2NxnU:
    {
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx + (uiPartOffset>>1) );
      break;
    }
  case SIZE_2NxnD:
    {
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx + (uiPartOffset<<1) + (uiPartOffset>>1) );
      break;
    }
  case SIZE_nLx2N:
    {
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx + (uiPartOffset>>2) );
      break;
    }
  case SIZE_nRx2N:
    {
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
      m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx + uiPartOffset + (uiPartOffset>>2) );
      break;
    }
  default:
    break;
  }

  return;
}

#if HHI_MRG_PU
Void TEncEntropy::encodePUWise( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( bRD )
    uiAbsPartIdx = 0;

  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  UInt uiNumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
  UInt uiDepth = pcCU->getDepth( uiAbsPartIdx );
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;

  for ( UInt uiPartIdx = 0, uiSubPartIdx = uiAbsPartIdx; uiPartIdx < uiNumPU; uiPartIdx++, uiSubPartIdx += uiPUOffset )
  {
    encodeMergeFlag( pcCU, uiSubPartIdx );
    if ( pcCU->getMergeFlag( uiSubPartIdx ) )
    {
      encodeMergeIndex( pcCU, uiSubPartIdx );
    }
    else
    {
      encodeInterDirPU( pcCU, uiSubPartIdx );
#ifdef DCM_PBIC
      if (pcCU->getSlice()->getSPS()->getUseIC())
      {
        if ( ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) && ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) )
        {
          encodeRefFrmIdxPU ( pcCU, uiSubPartIdx, REF_PIC_LIST_0 );
          encodeRefFrmIdxPU ( pcCU, uiSubPartIdx, REF_PIC_LIST_1 );
          encodeMvdIcdPU    ( pcCU, uiSubPartIdx                 );
          encodeMVPIdxPU    ( pcCU, uiSubPartIdx, REF_PIC_LIST_0 );
          encodeMVPIdxPU    ( pcCU, uiSubPartIdx, REF_PIC_LIST_1 );
        }
        else if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
        {
          encodeRefFrmIdxPU ( pcCU, uiSubPartIdx, REF_PIC_LIST_0 );
          encodeMvdIcdPU    ( pcCU, uiSubPartIdx                 );
          encodeMVPIdxPU    ( pcCU, uiSubPartIdx, REF_PIC_LIST_0 );
        }
        else if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
        {
          encodeRefFrmIdxPU ( pcCU, uiSubPartIdx, REF_PIC_LIST_1 );
          encodeMvdIcdPU    ( pcCU, uiSubPartIdx                 );
          encodeMVPIdxPU    ( pcCU, uiSubPartIdx, REF_PIC_LIST_1 );
        }
        if ( ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) || ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) )
        {
          encodeICPIdxPU    ( pcCU, uiSubPartIdx                 );
        }
      }
      else
#endif
      {
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

Void TEncEntropy::encodeRefFrmIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );
  assert( !pcCU->isSkip( uiAbsPartIdx ) );

#ifdef QC_AMVRES
  if ( ( pcCU->getSlice()->getNumRefIdx( eRefList ) == 1 ))
  {
    if ((pcCU->getSlice()->getSymbolMode() != 0) || (!pcCU->getSlice()->getSPS()->getUseAMVRes()))
      return;
  }
#else
  if ( ( pcCU->getSlice()->getNumRefIdx( eRefList ) == 1 ) )
  {
    return;
  }
#endif

  if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
  {
    m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
  }

  return;
}

Void TEncEntropy::encodeMvdPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );
  assert( !pcCU->isSkip( uiAbsPartIdx ) );

  if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
  {
    m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
  }
  return;
}

Void TEncEntropy::encodeMVPIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
  {
    m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
  }

  return;
}

#ifdef DCM_PBIC
Void TEncEntropy::encodeMvdIcdPU( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );
  assert( !pcCU->isSkip( uiAbsPartIdx ) );

  UChar uhInterDir = pcCU->getInterDir( uiAbsPartIdx );
  assert( (uhInterDir == 1) || (uhInterDir == 2) || (uhInterDir == 3) );
  RefPicList eRefList = (uhInterDir == 3) ? REF_PIC_LIST_X : ( (uhInterDir == 2) ? REF_PIC_LIST_1 : REF_PIC_LIST_0);

  m_pcEntropyCoderIf->codeMvdIcd( pcCU, uiAbsPartIdx, eRefList );
}

Void TEncEntropy::encodeICPIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if (pcCU->getICPNum(uiAbsPartIdx) > 1)
    m_pcEntropyCoderIf->codeICPIdx( pcCU, uiAbsPartIdx );
}
#endif

#endif

Void TEncEntropy::encodeMVPIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  UInt uiPartOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;

  switch ( pcCU->getPartitionSize( uiAbsPartIdx ) )
  {
  case SIZE_2Nx2N:
    {
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
      {
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
      }
      break;
    }

  case SIZE_2NxN:
    {
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
      {
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
      }
      uiAbsPartIdx += uiPartOffset << 1;
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
      {
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
      }
      break;
    }

  case SIZE_Nx2N:
    {
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
      {
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
      }

      uiAbsPartIdx += uiPartOffset;
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
      {
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
      }

      break;
    }

  case SIZE_NxN:
    {
      for ( Int iPartIdx = 0; iPartIdx < 4; iPartIdx++ )
      {
        if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
        {
          m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
        }
        uiAbsPartIdx += uiPartOffset;
      }
      break;
    }
  case SIZE_2NxnU:
    {
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += (uiPartOffset>>1);
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  case SIZE_2NxnD:
    {
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += (uiPartOffset<<1) + (uiPartOffset>>1);
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  case SIZE_nLx2N:
    {
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += (uiPartOffset>>2);
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  case SIZE_nRx2N:
    {
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += uiPartOffset + (uiPartOffset>>2);
      if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) && (pcCU->getMVPNum(eRefList, uiAbsPartIdx)> 1) && (pcCU->getAMVPMode(uiAbsPartIdx) == AM_EXPL) )
        m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  default:
    break;
  }

  return;

}

#ifdef DCM_PBIC
Void TEncEntropy::encodeICPIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  UInt uiPartIdxTotal, uiPartAddrInc;
  UInt uiPartOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;

  switch ( pcCU->getPartitionSize( uiAbsPartIdx ) )
  {
  case SIZE_2Nx2N:
    {
      uiPartIdxTotal = 1;
      uiPartAddrInc  = 0;
      break;
    }
  case SIZE_2NxN:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = (uiPartOffset << 1);
      break;
    }
  case SIZE_Nx2N:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = uiPartOffset;
      break;
    }
  case SIZE_NxN:
    {
      uiPartIdxTotal = 4;
      uiPartAddrInc  = uiPartOffset;
      break;
    }
  case SIZE_2NxnU:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = (uiPartOffset>>1);
      break;
    }
  case SIZE_2NxnD:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = (uiPartOffset<<1) + (uiPartOffset>>1);
      break;
    }
  case SIZE_nLx2N:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = (uiPartOffset>>2);
      break;
    }
  case SIZE_nRx2N:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = uiPartOffset + (uiPartOffset>>2);
      break;
    }
  default:
    {
      uiPartIdxTotal = 0;
      uiPartAddrInc  = 0;
      break;
    }
  }

  for (UInt uiPartIdx = 0, uiPartAddr = uiAbsPartIdx; uiPartIdx < uiPartIdxTotal; uiPartIdx++, uiPartAddr += uiPartAddrInc)
  {
    if (pcCU->getICPNum(uiPartAddr) > 1)
      m_pcEntropyCoderIf->codeICPIdx( pcCU, uiPartAddr );
  }
}
#endif

Void TEncEntropy::encodeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList, Bool bRD )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );

  if( bRD )
    uiAbsPartIdx = 0;

#ifdef QC_AMVRES
  if (pcCU->isSkip( uiAbsPartIdx ))
  {
	  return;
  }
  else if ( ( pcCU->getSlice()->getNumRefIdx( eRefList ) == 1 ))
  {
	  if ((pcCU->getSlice()->getSymbolMode() != 0) || (!pcCU->getSlice()->getSPS()->getUseAMVRes()))
		  return;
  }
#else
  if ( ( pcCU->getSlice()->getNumRefIdx( eRefList ) == 1 ) || pcCU->isSkip( uiAbsPartIdx ) )
  {
    return;
  }
#endif
  UInt uiPartOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;

  switch ( pcCU->getPartitionSize( uiAbsPartIdx ) )
  {
  case SIZE_2Nx2N:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
      }
      break;
    }

  case SIZE_2NxN:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
      }

      uiAbsPartIdx += uiPartOffset << 1;
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
      }
      break;
    }

  case SIZE_Nx2N:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
      }

      uiAbsPartIdx += uiPartOffset;
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
      }
      break;
    }

  case SIZE_NxN:
    {
      for ( Int iPartIdx = 0; iPartIdx < 4; iPartIdx++ )
      {
        if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        {
          m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
        }
        uiAbsPartIdx += uiPartOffset;
      }
      break;
    }
  case SIZE_2NxnU:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += (uiPartOffset>>1);

      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  case SIZE_2NxnD:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += (uiPartOffset<<1) + (uiPartOffset>>1);

      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  case SIZE_nLx2N:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += (uiPartOffset>>2);

      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  case SIZE_nRx2N:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += uiPartOffset + (uiPartOffset>>2);

      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  default:
    break;
  }

  return;
}

Void TEncEntropy::encodeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList, Bool bRD )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );

  if( bRD )
    uiAbsPartIdx = 0;

  if ( pcCU->isSkip( uiAbsPartIdx ) )
  {
    return;
  }

  UInt uiPartOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;

  switch ( pcCU->getPartitionSize( uiAbsPartIdx ) )
  {
  case SIZE_2Nx2N:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
      }
      break;
    }

  case SIZE_2NxN:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
      }

      uiAbsPartIdx += uiPartOffset << 1;
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
      }
      break;
    }

  case SIZE_Nx2N:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
      }

      uiAbsPartIdx += uiPartOffset;
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
      {
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
      }
      break;
    }

  case SIZE_NxN:
    {
      for ( Int iPartIdx = 0; iPartIdx < 4; iPartIdx++ )
      {
        if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        {
          m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
        }
        uiAbsPartIdx += uiPartOffset;
      }
      break;
    }
  case SIZE_2NxnU:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += (uiPartOffset>>1);

      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  case SIZE_2NxnD:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += (uiPartOffset<<1) + (uiPartOffset>>1);

      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  case SIZE_nLx2N:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += (uiPartOffset>>2);

      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  case SIZE_nRx2N:
    {
      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );

      uiAbsPartIdx += uiPartOffset + (uiPartOffset>>2);

      if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
        m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );

      break;
    }
  default:
    break;
  }

  return;
}

#ifdef DCM_PBIC
Void TEncEntropy::encodeMvdIcd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList, Bool bRD )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );

  if( bRD )
    uiAbsPartIdx = 0;

  if ( pcCU->isSkip( uiAbsPartIdx ) )
  {
    return;
  }

  UInt uiPartIdxTotal, uiPartAddrInc;
  UInt uiPartOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;

  switch ( pcCU->getPartitionSize( uiAbsPartIdx ) )
  {
  case SIZE_2Nx2N:
    {
      uiPartIdxTotal = 1;
      uiPartAddrInc  = 0;
      break;
    }
  case SIZE_2NxN:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = (uiPartOffset << 1);
      break;
    }
  case SIZE_Nx2N:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = uiPartOffset;
      break;
    }
  case SIZE_NxN:
    {
      uiPartIdxTotal = 4;
      uiPartAddrInc  = uiPartOffset;
      break;
    }
  case SIZE_2NxnU:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = (uiPartOffset>>1);
      break;
    }
  case SIZE_2NxnD:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = (uiPartOffset<<1) + (uiPartOffset>>1);
      break;
    }
  case SIZE_nLx2N:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = (uiPartOffset>>2);
      break;
    }
  case SIZE_nRx2N:
    {
      uiPartIdxTotal = 2;
      uiPartAddrInc  = uiPartOffset + (uiPartOffset>>2);
      break;
    }
  default:
    {
      uiPartIdxTotal = 0;
      uiPartAddrInc  = 0;
      break;
    }
  }

  UChar uhInterDir;
  for (UInt uiPartIdx = 0, uiPartAddr = uiAbsPartIdx; uiPartIdx < uiPartIdxTotal; uiPartIdx++, uiPartAddr += uiPartAddrInc)
  {
    uhInterDir = pcCU->getInterDir( uiPartAddr );
    assert ( (uhInterDir == 1) || (uhInterDir == 2) || (uhInterDir == 3) );
    eRefList = (uhInterDir == 3) ? REF_PIC_LIST_X : ( (uhInterDir == 2) ? REF_PIC_LIST_1 : REF_PIC_LIST_0);

		m_pcEntropyCoderIf->codeMvdIcd( pcCU, uiPartAddr, eRefList );
  }
}
#endif

#if HHI_RQT
Void TEncEntropy::encodeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, eType, uiTrDepth );
}

Void TEncEntropy::encodeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSymbol, uiCtx );
}

#if HHI_RQT_ROOT
Void TEncEntropy::encodeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  m_pcEntropyCoderIf->codeQtRootCbf( pcCU, uiAbsPartIdx );
}
#endif
#endif

// Coded block flag
Void TEncEntropy::encodeCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, eType, uiTrDepth );
}

// dQP
Void TEncEntropy::encodeQP( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  if ( pcCU->getSlice()->getSPS()->getUseDQP() )
  {
    m_pcEntropyCoderIf->codeDeltaQP( pcCU, uiAbsPartIdx );
  }
}


// texture
Void TEncEntropy::xEncodeCoeff( TComDataCU* pcCU, TCoeff* pcCoeff, UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, UInt uiTrIdx, UInt uiCurrTrIdx, TextType eType, Bool bRD )
{
  if ( pcCU->getCbf( uiAbsPartIdx, eType, uiTrIdx ) )
  {
#if HHI_RQT
    UInt uiLumaTrMode, uiChromaTrMode;
    pcCU->convertTransIdx( uiAbsPartIdx, pcCU->getTransformIdx( uiAbsPartIdx ), uiLumaTrMode, uiChromaTrMode );
    const UInt uiStopTrMode = eType == TEXT_LUMA ? uiLumaTrMode : uiChromaTrMode;

    assert( pcCU->getSlice()->getSPS()->getQuadtreeTUFlag() || uiStopTrMode == uiCurrTrIdx ); // as long as quadtrees are not used for residual transform

    if( uiTrIdx == uiStopTrMode )
#else
    if( uiCurrTrIdx == uiTrIdx )
#endif
    {
#if HHI_RQT
      assert( !bRD ); // parameter bRD can be removed

      UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth ] + 2;
      if( eType != TEXT_LUMA && uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
      {
        UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
        if( ( uiAbsPartIdx % uiQPDiv ) != 0 )
        {
          return;
        }
        uiWidth  <<= 1;
        uiHeight <<= 1;
      }
#endif
#if QC_MDDT
      m_pcEntropyCoderIf->codeCoeffNxN( pcCU, pcCoeff, uiAbsPartIdx, uiWidth, uiHeight, uiDepth, eType, pcCU->getLumaIntraDir( uiAbsPartIdx ), bRD );
#else
      m_pcEntropyCoderIf->codeCoeffNxN( pcCU, pcCoeff, uiAbsPartIdx, uiWidth, uiHeight, uiDepth, eType, bRD );
#endif
    }
    else
    {
#if HHI_RQT
      {
        DTRACE_CABAC_V( g_nSymbolCounter++ );
        DTRACE_CABAC_T( "\tgoing down\tdepth=" );
        DTRACE_CABAC_V( uiDepth );
        DTRACE_CABAC_T( "\ttridx=" );
        DTRACE_CABAC_V( uiTrIdx );
        DTRACE_CABAC_T( "\n" );
      }
#endif
      if( uiCurrTrIdx <= uiTrIdx )
#if HHI_RQT
        assert( pcCU->getSlice()->getSPS()->getQuadtreeTUFlag() );
#else
        assert(0);
#endif

      UInt uiSize;
      uiWidth  >>= 1;
      uiHeight >>= 1;
      uiSize = uiWidth*uiHeight;
      uiDepth++;
      uiTrIdx++;

      UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
      UInt uiIdx      = uiAbsPartIdx;

#if LCEC_CBP_YUV_ROOT
      if(pcCU->getSlice()->getSymbolMode() == 0)
      {
#if HHI_RQT
        if(pcCU->getSlice()->getSPS()->getQuadtreeTUFlag())
        {
          UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth ] + 2;
          if( eType == TEXT_LUMA || uiLog2TrSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
            m_pcEntropyCoderIf->codeBlockCbf(pcCU, uiIdx, eType, uiTrIdx, uiQPartNum);
        }
        else
          m_pcEntropyCoderIf->codeBlockCbf(pcCU, uiIdx, eType, uiTrIdx, uiQPartNum);
#else
        m_pcEntropyCoderIf->codeBlockCbf(pcCU, uiIdx, eType, uiTrIdx, uiQPartNum);
#endif
        xEncodeCoeff( pcCU, pcCoeff, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, eType, bRD ); pcCoeff += uiSize; uiIdx += uiQPartNum;
        xEncodeCoeff( pcCU, pcCoeff, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, eType, bRD ); pcCoeff += uiSize; uiIdx += uiQPartNum;
        xEncodeCoeff( pcCU, pcCoeff, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, eType, bRD ); pcCoeff += uiSize; uiIdx += uiQPartNum;
        xEncodeCoeff( pcCU, pcCoeff, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, eType, bRD );
      }
      else
      {
#endif
      m_pcEntropyCoderIf->codeCbf( pcCU, uiIdx, eType, uiTrIdx );
      xEncodeCoeff( pcCU, pcCoeff, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, eType, bRD ); pcCoeff += uiSize; uiIdx += uiQPartNum;

      m_pcEntropyCoderIf->codeCbf( pcCU, uiIdx, eType, uiTrIdx );
      xEncodeCoeff( pcCU, pcCoeff, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, eType, bRD ); pcCoeff += uiSize; uiIdx += uiQPartNum;

      m_pcEntropyCoderIf->codeCbf( pcCU, uiIdx, eType, uiTrIdx );
      xEncodeCoeff( pcCU, pcCoeff, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, eType, bRD ); pcCoeff += uiSize; uiIdx += uiQPartNum;

      m_pcEntropyCoderIf->codeCbf( pcCU, uiIdx, eType, uiTrIdx );
      xEncodeCoeff( pcCU, pcCoeff, uiIdx, uiDepth, uiWidth, uiHeight, uiTrIdx, uiCurrTrIdx, eType, bRD );
#if LCEC_CBP_YUV_ROOT
      }
#endif
#if HHI_RQT
      {
        DTRACE_CABAC_V( g_nSymbolCounter++ );
        DTRACE_CABAC_T( "\tgoing up\n" );
      }
#endif
    }
  }
}

Void TEncEntropy::encodeCoeff( TComDataCU* pcCU, TCoeff* pCoeff, UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, UInt uiMaxTrMode, UInt uiTrMode, TextType eType, Bool bRD )
{
  xEncodeCoeff( pcCU, pCoeff, uiAbsPartIdx, uiDepth, uiWidth, uiHeight, uiTrMode, uiMaxTrMode, eType, bRD );
}

Void TEncEntropy::encodeCoeff( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight )
{
  UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
  UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
  UInt uiChromaOffset = uiLumaOffset>>2;

  UInt uiLumaTrMode, uiChromaTrMode;
  pcCU->convertTransIdx( uiAbsPartIdx, pcCU->getTransformIdx(uiAbsPartIdx), uiLumaTrMode, uiChromaTrMode );

  if( pcCU->isIntra(uiAbsPartIdx) )
  {
#if LCEC_CBP_YUV_ROOT
    if (pcCU->getSlice()->getSymbolMode()==0)
    {
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_ALL, 0 );
      if(pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, 0)==0 && pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, 0)==0
         && pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, 0)==0)
         return;
    }
#endif
#if HHI_RQT_INTRA
    if( pcCU->getSlice()->getSPS()->getQuadtreeTUFlag() )
    {
      DTRACE_CABAC_V( g_nSymbolCounter++ )
      DTRACE_CABAC_T( "\tdecodeTransformIdx()\tCUDepth=" )
      DTRACE_CABAC_V( uiDepth )
      DTRACE_CABAC_T( "\n" )
#if MS_LAST_CBF
      UInt temp = 0;
      UInt temp1 = 0;
      UInt temp2 = 0;
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0, temp, temp1, temp2 );
#else
      xEncodeTransformSubdiv( pcCU, uiAbsPartIdx, uiDepth, 0 );
#endif
    }
#endif

#if QC_MDDT
#if LCEC_PHASE2
#if LCEC_CBP_YUV_ROOT
    if (pcCU->getSlice()->getSymbolMode())
    {
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, 0 );
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, 0 );
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, 0 );
    }
#else
    if (pcCU->getSlice()->getSymbolMode()==0)
    {
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_ALL, 0 );
    }
    else
    {
	    m_pcEntropyCoderIf->codeCbf(pcCU, uiAbsPartIdx, TEXT_LUMA, 0);
	    m_pcEntropyCoderIf->codeCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_U, 0);
	    m_pcEntropyCoderIf->codeCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_V, 0);
    }
#endif
#else
	m_pcEntropyCoderIf->codeCbf(pcCU, uiAbsPartIdx, TEXT_LUMA, 0);
	m_pcEntropyCoderIf->codeCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_U, 0);
	m_pcEntropyCoderIf->codeCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_V, 0);
#endif

#if !QC_MDDT_ROT_UNIFIED
    if (pcCU->getSlice()->getSPS()->getUseROT() && uiWidth >= 16)
#endif
		encodeROTindex( pcCU, uiAbsPartIdx, uiDepth );

	xEncodeCoeff( pcCU, pcCU->getCoeffY()  + uiLumaOffset,   uiAbsPartIdx, uiDepth, uiWidth,    uiHeight,    0, uiLumaTrMode,   TEXT_LUMA     );
	xEncodeCoeff( pcCU, pcCU->getCoeffCb() + uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth>>1, uiHeight>>1, 0, uiChromaTrMode, TEXT_CHROMA_U );
	xEncodeCoeff( pcCU, pcCU->getCoeffCr() + uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth>>1, uiHeight>>1, 0, uiChromaTrMode, TEXT_CHROMA_V );
#else

#if LCEC_PHASE2
#if LCEC_CBP_YUV_ROOT
    if (pcCU->getSlice()->getSymbolMode())
    {
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, 0 );
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, 0 );
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, 0 );
    }
#else
    if (pcCU->getSlice()->getSymbolMode()==0)
    {
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_ALL, 0 );
    }
    else
    {
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, 0 );
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, 0 );
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, 0 );
    }
#endif
    xEncodeCoeff( pcCU, pcCU->getCoeffY()  + uiLumaOffset,   uiAbsPartIdx, uiDepth, uiWidth,    uiHeight,    0, uiLumaTrMode,   TEXT_LUMA     );
    xEncodeCoeff( pcCU, pcCU->getCoeffCb() + uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth>>1, uiHeight>>1, 0, uiChromaTrMode, TEXT_CHROMA_U );
    xEncodeCoeff( pcCU, pcCU->getCoeffCr() + uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth>>1, uiHeight>>1, 0, uiChromaTrMode, TEXT_CHROMA_V );
#else

    m_pcEntropyCoderIf->codeCbf(pcCU, uiAbsPartIdx, TEXT_LUMA, 0);
    xEncodeCoeff( pcCU, pcCU->getCoeffY()  + uiLumaOffset,   uiAbsPartIdx, uiDepth, uiWidth,    uiHeight,    0, uiLumaTrMode,   TEXT_LUMA     );

    m_pcEntropyCoderIf->codeCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_U, 0);
    xEncodeCoeff( pcCU, pcCU->getCoeffCb() + uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth>>1, uiHeight>>1, 0, uiChromaTrMode, TEXT_CHROMA_U );

    m_pcEntropyCoderIf->codeCbf(pcCU, uiAbsPartIdx, TEXT_CHROMA_V, 0);
    xEncodeCoeff( pcCU, pcCU->getCoeffCr() + uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth>>1, uiHeight>>1, 0, uiChromaTrMode, TEXT_CHROMA_V );
#endif
#endif
  }
  else
  {
#if HHI_RQT_ROOT
#if LCEC_CBP_YUV_ROOT
    if( pcCU->getSlice()->getSPS()->getQuadtreeTUFlag() && pcCU->getSlice()->getSymbolMode())
#else
    if( pcCU->getSlice()->getSPS()->getQuadtreeTUFlag() )
#endif
    {
      m_pcEntropyCoderIf->codeQtRootCbf( pcCU, uiAbsPartIdx );
      if ( !pcCU->getQtRootCbf( uiAbsPartIdx ) )
      {
        return;
      }
    }
#endif

#if LCEC_PHASE2
    if (pcCU->getSlice()->getSymbolMode()==0)
    {
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_ALL, 0 );
#if LCEC_CBP_YUV_ROOT
      if(pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, 0)==0 && pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, 0)==0
         && pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, 0)==0)
         return;
#endif
    }
    else
    {
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, 0 );
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, 0 );
      m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, 0 );
    }
#else
    m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, 0 );
    m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, 0 );
    m_pcEntropyCoderIf->codeCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, 0 );
#endif
#if HHI_RQT
    if( pcCU->getSlice()->getSPS()->getQuadtreeTUFlag() || pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, 0) || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, 0) || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, 0) )
#else
    if( pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, 0) || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, 0) || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, 0) )
#endif
      encodeTransformIdx( pcCU, uiAbsPartIdx, pcCU->getDepth(uiAbsPartIdx) );

    xEncodeCoeff( pcCU, pcCU->getCoeffY()  + uiLumaOffset,   uiAbsPartIdx, uiDepth, uiWidth,    uiHeight,    0, uiLumaTrMode,   TEXT_LUMA     );
    xEncodeCoeff( pcCU, pcCU->getCoeffCb() + uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth>>1, uiHeight>>1, 0, uiChromaTrMode, TEXT_CHROMA_U );
    xEncodeCoeff( pcCU, pcCU->getCoeffCr() + uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth>>1, uiHeight>>1, 0, uiChromaTrMode, TEXT_CHROMA_V );
  }
}
#if QC_MDDT
Void TEncEntropy::encodeCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoeff, UInt uiAbsPartIdx, UInt uiTrWidth, UInt uiTrHeight, UInt uiDepth, TextType eType, Bool bRD )
{ // This is for Transform unit processing. This may be used at mode selection stage for Inter.
  UInt uiMode;
  if(eType == TEXT_LUMA && pcCU->isIntra( uiAbsPartIdx ) )
    uiMode = pcCU->getLumaIntraDir( uiAbsPartIdx );
  else
    uiMode = REG_DCT;

  m_pcEntropyCoderIf->codeCoeffNxN( pcCU, pcCoeff, uiAbsPartIdx, uiTrWidth, uiTrHeight, uiDepth, eType, uiMode, bRD );
}
#else
Void TEncEntropy::encodeCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoeff, UInt uiAbsPartIdx, UInt uiTrWidth, UInt uiTrHeight, UInt uiDepth, TextType eType, Bool bRD )
{ // This is for Transform unit processing. This may be used at mode selection stage for Inter.
  m_pcEntropyCoderIf->codeCoeffNxN( pcCU, pcCoeff, uiAbsPartIdx, uiTrWidth, uiTrHeight, uiDepth, eType, bRD );
}
#endif


Void TEncEntropy::estimateBit (estBitsSbacStruct* pcEstBitsSbac, UInt uiWidth, TextType eTType)
{
  UInt uiCTXIdx;

  switch(uiWidth)
  {
  case  2: uiCTXIdx = 6; break;
  case  4: uiCTXIdx = 5; break;
  case  8: uiCTXIdx = 4; break;
  case 16: uiCTXIdx = 3; break;
  case 32: uiCTXIdx = 2; break;
  case 64: uiCTXIdx = 1; break;
  default: uiCTXIdx = 0; break;
  }

  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA;

  m_pcEntropyCoderIf->estBit ( pcEstBitsSbac, uiCTXIdx, eTType );
}

#ifdef QC_SIFO
Void TEncEntropy::encodeSwitched_Filters(TComSlice* pcSlice,TComPrediction *m_cPrediction)
{
	m_pcEntropyCoderIf->encodeSwitched_Filters(pcSlice,m_cPrediction);
}
#endif
