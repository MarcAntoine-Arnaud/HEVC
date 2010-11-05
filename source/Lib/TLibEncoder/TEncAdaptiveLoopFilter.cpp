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

/** \file     TEncAdaptiveLoopFilter.cpp
    \brief    estimation part of adaptive loop filter class
*/
#include "TEncAdaptiveLoopFilter.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define ALF_NUM_OF_REDESIGN 3

// ====================================================================================================================
// Tables
// ====================================================================================================================

const Int TEncAdaptiveLoopFilter::m_aiSymmetricArray9x9[81] =
{
   0,  1,  2,  3,  4,  5,  6,  7,  8,
   9, 10, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 32, 33, 34, 35,
  36, 37, 38, 39, 40, 39, 38, 37, 36,
  35, 34, 33, 32, 31, 30, 29, 28, 27,
  26, 25, 24, 23, 22, 21, 20, 19, 18,
  17, 16, 15, 14, 13, 12, 11, 10,  9,
   8,  7,  6,  5,  4,  3,  2,  1,  0
};
const Int TEncAdaptiveLoopFilter::m_aiSymmetricArray7x7[49] =
{
   0,  1,  2,  3,  4,  5,  6,
   7,  8,  9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 23, 22, 21,
  20, 19, 18, 17, 16, 15, 14,
  13, 12, 11, 10,  9,  8,  7,
   6,  5,  4,  3,  2,  1,  0,
};
const Int TEncAdaptiveLoopFilter::m_aiSymmetricArray5x5[25] =
{
   0,  1,  2,  3,  4,
   5,  6,  7,  8,  9,
  10, 11, 12, 11, 10,
   9,  8,  7,  6,  5,
   4,  3,  2,  1,  0,
};

// ====================================================================================================================
// Constructor / destructor
// ====================================================================================================================

TEncAdaptiveLoopFilter::TEncAdaptiveLoopFilter()
{
  m_ppdAlfCorr = NULL;
  m_pdDoubleAlfCoeff = NULL;
  m_puiCUCorr = NULL;
  m_pcPic = NULL;
  m_pcEntropyCoder = NULL;
  m_pcBestAlfParam = NULL;
  m_pcTempAlfParam = NULL;
  m_pcPicYuvBest = NULL;
  m_pcPicYuvTmp = NULL;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param	pcPic						picture (TComPic) pointer
		\param	pcEntropyCoder	entropy coder class
 */
Void TEncAdaptiveLoopFilter::startALFEnc( TComPic* pcPic, TEncEntropy* pcEntropyCoder )
{
  m_pcPic = pcPic;
  m_pcEntropyCoder = pcEntropyCoder;

  m_eSliceType = pcPic->getSlice()->getSliceType();
  m_iPicNalReferenceIdc = (pcPic->getSlice()->isReferenced() ? 1 :0);

  m_uiNumSCUInCU = m_pcPic->getNumPartInCU();
  m_uiSCUWidth = (m_pcPic->getMinCUWidth()<<1);
  m_uiSCUHeight = (m_pcPic->getMinCUHeight()<<1);

  xInitParam();
  xCreateTmpAlfCtrlFlags();

  Int iWidth = pcPic->getPicYuvOrg()->getWidth();
  Int iHeight = pcPic->getPicYuvOrg()->getHeight();

  m_pcPicYuvTmp = new TComPicYuv();
  m_pcPicYuvTmp->createLuma(iWidth, iHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth);
  m_pcPicYuvBest = pcPic->getPicYuvPred();

  m_pcBestAlfParam = new ALFParam;
  m_pcTempAlfParam = new ALFParam;
  allocALFParam(m_pcBestAlfParam);
  allocALFParam(m_pcTempAlfParam);
#if QC_ALF
  im_width = iWidth;
  im_height = iHeight;

  //original and reconst 
#if !ALF_MEM_PATCH
  get_mem2Dpel (&(imgY_rec), im_height, im_width);
  get_mem2Dpel (&(imgY_org), im_height, im_width);
  get_mem2Dpel (&(imgY_rest), im_height, im_width);
  get_mem2Dpel (&(imgY_ext), im_height+ALF_MAX_NUM_TAP, im_width+ALF_MAX_NUM_TAP);
  get_mem2Dpel (&(imgY_temp), im_height, im_width);
#endif

  // init qc_filter
  initMatrix4D_double(&EGlobalSym, NO_TEST_FILT,  NO_VAR_BINS, MAX_SQR_FILT_LENGTH, MAX_SQR_FILT_LENGTH);
  initMatrix3D_double(&yGlobalSym, NO_TEST_FILT, NO_VAR_BINS, MAX_SQR_FILT_LENGTH); 
#if !ALF_MEM_PATCH
  initMatrix_int(&g_filterCoeffSym, NO_VAR_BINS, MAX_SQR_FILT_LENGTH); 
  initMatrix_int(&g_filterCoeffPrevSelected, NO_VAR_BINS, MAX_SQR_FILT_LENGTH); 
#endif
  initMatrix_int(&g_filterCoeffSymQuant, NO_VAR_BINS, MAX_SQR_FILT_LENGTH); 

  pixAcc = (double *) calloc(NO_VAR_BINS, sizeof(double));
  get_mem2Dpel(&varImg, im_height, im_width);
  get_mem2Dpel(&maskImg, im_height, im_width);
  

#if ALF_MEM_PATCH
	initMatrix_double(&E_temp, MAX_SQR_FILT_LENGTH, MAX_SQR_FILT_LENGTH);//
    y_temp = (double *) calloc(MAX_SQR_FILT_LENGTH, sizeof(double));//
    initMatrix3D_double(&E_merged, NO_VAR_BINS, MAX_SQR_FILT_LENGTH, MAX_SQR_FILT_LENGTH);//
    initMatrix_double(&y_merged, NO_VAR_BINS, MAX_SQR_FILT_LENGTH); //
    pixAcc_merged = (double *) calloc(NO_VAR_BINS, sizeof(double));//

	filterCoeffQuantMod = (int *) calloc(MAX_SQR_FILT_LENGTH, sizeof(int));//
    filterCoeff = (double *) calloc(MAX_SQR_FILT_LENGTH, sizeof(double));//
    filterCoeffQuant = (int *) calloc(MAX_SQR_FILT_LENGTH, sizeof(int));//
    initMatrix_int(&diffFilterCoeffQuant, NO_VAR_BINS, MAX_SQR_FILT_LENGTH);//
    initMatrix_int(&FilterCoeffQuantTemp, NO_VAR_BINS, MAX_SQR_FILT_LENGTH);//

#endif

  ALFp = new ALFParam;
  tempALFp = new ALFParam;
  allocALFParam(ALFp);
  allocALFParam(tempALFp);
  m_pcDummyEntropyCoder = m_pcEntropyCoder;
#endif
}

Void TEncAdaptiveLoopFilter::endALFEnc()
{
  xUninitParam();
  xDestroyTmpAlfCtrlFlags();

  m_pcPicYuvTmp->destroyLuma();
  delete m_pcPicYuvTmp;
  m_pcPicYuvTmp = NULL;
  m_pcPic = NULL;
  m_pcEntropyCoder = NULL;

  freeALFParam(m_pcBestAlfParam);
  freeALFParam(m_pcTempAlfParam);
  delete m_pcBestAlfParam;
  delete m_pcTempAlfParam;
#if QC_ALF
#if !ALF_MEM_PATCH
  free_mem2Dpel (imgY_rec);
  free_mem2Dpel (imgY_org);

  free_mem2Dpel (imgY_rest);
  free_mem2Dpel (imgY_ext);
  free_mem2Dpel (imgY_temp);
#endif
// delete qc filters
  destroyMatrix4D_double(EGlobalSym, NO_TEST_FILT,  NO_VAR_BINS);
  destroyMatrix3D_double(yGlobalSym, NO_TEST_FILT);
  destroyMatrix_int(g_filterCoeffSymQuant);
#if !ALF_MEM_PATCH
  destroyMatrix_int(g_filterCoeffSym);
  destroyMatrix_int(g_filterCoeffPrevSelected);
#endif

  free(pixAcc);
  free_mem2Dpel(varImg);
  free_mem2Dpel(maskImg);

#if ALF_MEM_PATCH
    destroyMatrix3D_double(E_merged, NO_VAR_BINS);
	destroyMatrix_double(y_merged);
	destroyMatrix_double(E_temp);
	free(pixAcc_merged);

	free(filterCoeffQuantMod);
	free(y_temp);

	free(filterCoeff);
	free(filterCoeffQuant);
	destroyMatrix_int(diffFilterCoeffQuant);
	destroyMatrix_int(FilterCoeffQuantTemp);

#endif
  freeALFParam(ALFp);
  freeALFParam(tempALFp);
  delete ALFp;
  delete tempALFp;
#endif
}

/** \param	pcAlfParam					ALF parameter
		\param	dLambda							lambda value for RD cost computation
		\retval	ruiDist							distortion
		\retval	ruiBits							required bits
		\retval	ruiMaxAlfCtrlDepth	optimal partition depth
 */
Void TEncAdaptiveLoopFilter::ALFProcess( ALFParam* pcAlfParam, Double dLambda, UInt64& ruiDist, UInt64& ruiBits, UInt& ruiMaxAlfCtrlDepth )
{
  Int tap, num_coef;

  // set global variables
  tap         = ALF_MAX_NUM_TAP;
  num_coef    = (tap*tap+1)>>1;
  num_coef    = num_coef + 1; // DC offset

  // set lambda
	m_dLambdaLuma   = dLambda;
  m_dLambdaChroma = dLambda;

  TComPicYuv* pcPicOrg = m_pcPic->getPicYuvOrg();

  // extend image for filtering
  TComPicYuv* pcPicYuvRec    = m_pcPic->getPicYuvRec();
  TComPicYuv* pcPicYuvExtRec = m_pcTempPicYuv;

  pcPicYuvRec->copyToPic(pcPicYuvExtRec);
	pcPicYuvExtRec->setBorderExtension( false );
  pcPicYuvExtRec->extendPicBorder   ();

  // set min cost
  UInt64 uiMinRate = MAX_INT;
  UInt64 uiMinDist = MAX_INT;
  Double dMinCost  = MAX_DOUBLE;

  UInt64  uiOrigRate;
  UInt64  uiOrigDist;
  Double	dOrigCost;

  // calc original cost
  xCalcRDCost( pcPicOrg, pcPicYuvRec, NULL, uiOrigRate, uiOrigDist, dOrigCost );
  m_pcBestAlfParam->alf_flag = 0;
  m_pcBestAlfParam->cu_control_flag = 0;

  // initialize temp_alfps
  m_pcTempAlfParam->alf_flag        = 1;
  m_pcTempAlfParam->tap							= tap;
  m_pcTempAlfParam->num_coeff				= num_coef;
  m_pcTempAlfParam->chroma_idc      = 0;
  m_pcTempAlfParam->cu_control_flag = 0;

#if QC_ALF
 // adaptive in-loop wiener filtering
  xEncALFLuma_qc( pcPicOrg, pcPicYuvExtRec, pcPicYuvRec, uiMinRate, uiMinDist, dMinCost );

  // cu-based filter on/off control
  xCUAdaptiveControl_qc( pcPicOrg, pcPicYuvExtRec, pcPicYuvRec, uiMinRate, uiMinDist, dMinCost );

  // adaptive tap-length
  xFilterTapDecision_qc( pcPicOrg, pcPicYuvExtRec, pcPicYuvRec, uiMinRate, uiMinDist, dMinCost );
#else
  // adaptive in-loop wiener filtering
  xEncALFLuma( pcPicOrg, pcPicYuvExtRec, pcPicYuvRec, uiMinRate, uiMinDist, dMinCost );

  // cu-based filter on/off control
  xCUAdaptiveControl( pcPicOrg, pcPicYuvExtRec, pcPicYuvRec, uiMinRate, uiMinDist, dMinCost );

  // adaptive tap-length
  xFilterTapDecision( pcPicOrg, pcPicYuvExtRec, pcPicYuvRec, uiMinRate, uiMinDist, dMinCost );
#endif  

	// compute RD cost
  xCalcRDCost( pcPicOrg, pcPicYuvRec, m_pcBestAlfParam, uiMinRate, uiMinDist, dMinCost );

	// compare RD cost to non-ALF case
  if( dMinCost < dOrigCost )
  {
    m_pcBestAlfParam->alf_flag = 1;

    ruiBits = uiMinRate;
    ruiDist = uiMinDist;
  }
  else
  {
    m_pcBestAlfParam->alf_flag				= 0;
    m_pcBestAlfParam->cu_control_flag = 0;

		uiMinRate = uiOrigRate;
    uiMinDist = uiOrigDist;
    dMinCost = dMinCost;

    m_pcEntropyCoder->setAlfCtrl(false);
    pcPicYuvExtRec->copyToPicLuma(pcPicYuvRec);

    ruiBits = uiOrigRate;
    ruiDist = uiOrigDist;
  }

	// if ALF works
  if( m_pcBestAlfParam->alf_flag )
  {
		// predict ALF coefficients
    predictALFCoeff( m_pcBestAlfParam );

    // do additional ALF process for chroma
    xEncALFChroma( uiMinRate, pcPicOrg, pcPicYuvExtRec, pcPicYuvRec, ruiDist, ruiBits );
  }

	// copy to best storage
  copyALFParam(pcAlfParam, m_pcBestAlfParam);

	// store best depth
  ruiMaxAlfCtrlDepth = m_pcEntropyCoder->getMaxAlfCtrlDepth();
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TEncAdaptiveLoopFilter::xEncALFLuma(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiMinRate, UInt64& ruiMinDist, Double& rdMinCost)
{
  UInt64  uiRate;
  UInt64  uiDist;
  Double dCost;

  Int tap, num_coef;

  tap                 = ALF_MIN_NUM_TAP;
  m_pcTempAlfParam->tap = tap;
  num_coef            = (tap*tap+1)>>1;
  num_coef            = num_coef + 1; // DC offset
  m_pcTempAlfParam->num_coeff = num_coef;

  xFilteringFrameLuma(pcPicOrg, pcPicDec, pcPicRest, true);
  xCalcRDCost(pcPicOrg, pcPicRest, m_pcTempAlfParam, uiRate, uiDist, dCost);

  if( dCost < rdMinCost)
  {
    ruiMinRate = uiRate;
    ruiMinDist = uiDist;
    rdMinCost = dCost;
    copyALFParam(m_pcBestAlfParam, m_pcTempAlfParam);
  }
}

Void TEncAdaptiveLoopFilter::xEncALFChroma( UInt64 uiLumaRate, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist, UInt64& ruiBits )
{
  // restriction for non-referenced B-slice
  if (m_eSliceType == B_SLICE && m_iPicNalReferenceIdc == 0)
  {
    return;
  }

  Int tap, num_coef;

  // set global variables
  tap         = ALF_MAX_NUM_TAP_C;
  num_coef    = (tap*tap+1)>>1;
  num_coef    = num_coef + 1; // DC offset

  // set min cost
  UInt64 uiMinRate = uiLumaRate;
  UInt64 uiMinDist = MAX_INT;
  Double dMinCost  = MAX_DOUBLE;

  // calc original cost
  copyALFParam(m_pcTempAlfParam, m_pcBestAlfParam);
  xCalcRDCostChroma(pcPicOrg, pcPicRest, m_pcTempAlfParam, uiMinRate, uiMinDist, dMinCost);

  // initialize temp_alfps
  m_pcTempAlfParam->chroma_idc = 3;
  m_pcTempAlfParam->tap_chroma       = tap;
  m_pcTempAlfParam->num_coeff_chroma = num_coef;

  // Adaptive in-loop wiener filtering for chroma
  xFilteringFrameChroma(pcPicOrg, pcPicDec, pcPicRest);

  // filter on/off decision for chroma
  Int iCWidth = (pcPicOrg->getWidth()>>1);
  Int iCHeight = (pcPicOrg->getHeight()>>1);
  Int iCStride = pcPicOrg->getCStride();
  UInt64 uiFiltDistCb = xCalcSSD(pcPicOrg->getCbAddr(), pcPicRest->getCbAddr(), iCWidth, iCHeight, iCStride);
  UInt64 uiFiltDistCr = xCalcSSD(pcPicOrg->getCrAddr(), pcPicRest->getCrAddr(), iCWidth, iCHeight, iCStride);
  UInt64 uiOrgDistCb = xCalcSSD(pcPicOrg->getCbAddr(), pcPicDec->getCbAddr(), iCWidth, iCHeight, iCStride);
  UInt64 uiOrgDistCr = xCalcSSD(pcPicOrg->getCrAddr(), pcPicDec->getCrAddr(), iCWidth, iCHeight, iCStride);

  m_pcTempAlfParam->chroma_idc = 0;
  if(uiOrgDistCb > uiFiltDistCb)
    m_pcTempAlfParam->chroma_idc += 2;
  if(uiOrgDistCr  > uiFiltDistCr )
    m_pcTempAlfParam->chroma_idc += 1;

  if(m_pcTempAlfParam->chroma_idc)
  {
    if(m_pcTempAlfParam->chroma_idc!=3)
    {
      // chroma filter re-design
      xFilteringFrameChroma(pcPicOrg, pcPicDec, pcPicRest);
    }

    UInt64 uiRate, uiDist;
    Double dCost;
    xCalcRDCostChroma(pcPicOrg, pcPicRest, m_pcTempAlfParam, uiRate, uiDist, dCost);

    if( dCost < dMinCost )
    {
      copyALFParam(m_pcBestAlfParam, m_pcTempAlfParam);
      predictALFCoeffChroma(m_pcBestAlfParam);

      ruiBits += uiRate;
      ruiDist += uiDist;
    }
    else
    {
      m_pcBestAlfParam->chroma_idc = 0;

      if((m_pcTempAlfParam->chroma_idc>>1)&0x01)
        pcPicDec->copyToPicCb(pcPicRest);
      if(m_pcTempAlfParam->chroma_idc&0x01)
        pcPicDec->copyToPicCr(pcPicRest);

      ruiBits += uiMinRate;
      ruiDist += uiMinDist;
    }
  }
  else
  {
    m_pcBestAlfParam->chroma_idc = 0;

    ruiBits += uiMinRate;
    ruiDist += uiMinDist;

		pcPicDec->copyToPicCb(pcPicRest);
    pcPicDec->copyToPicCr(pcPicRest);
  }
}

Void TEncAdaptiveLoopFilter::xCUAdaptiveControl(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiMinRate, UInt64& ruiMinDist, Double& rdMinCost)
{
  m_pcEntropyCoder->setAlfCtrl(true);

  UInt uiBestDepth = 0;

  ALFParam cFrmAlfParam;
  allocALFParam(&cFrmAlfParam);
  copyALFParam(&cFrmAlfParam, m_pcBestAlfParam);

  for (UInt uiDepth = 0; uiDepth < g_uiMaxCUDepth; uiDepth++)
  {
    m_pcEntropyCoder->setMaxAlfCtrlDepth(uiDepth);
    pcPicRest->copyToPicLuma(m_pcPicYuvTmp);
    copyALFParam(m_pcTempAlfParam, &cFrmAlfParam);
    m_pcTempAlfParam->cu_control_flag = 1;

    for (UInt uiRD = 0; uiRD <= ALF_NUM_OF_REDESIGN; uiRD++)
    {
      if (uiRD)
      {
        // re-design filter coefficients
        xReDesignFilterCoeff(pcPicOrg, pcPicDec, true);
        xFrame(pcPicDec, m_pcPicYuvTmp, m_pcTempAlfParam->coeff, m_pcTempAlfParam->tap);
      }

      UInt64 uiRate, uiDist;
      Double dCost;
#if TSB_ALF_HEADER
      xSetCUAlfCtrlFlags(uiDepth, pcPicOrg, pcPicDec, m_pcPicYuvTmp, uiDist, m_pcTempAlfParam);
#else
      xSetCUAlfCtrlFlags(uiDepth, pcPicOrg, pcPicDec, m_pcPicYuvTmp, uiDist);
#endif

			// compute RD cost
      xCalcRDCost(m_pcTempAlfParam, uiRate, uiDist, dCost);

      if (dCost < rdMinCost)
      {
        uiBestDepth = uiDepth;
        rdMinCost = dCost;
        ruiMinDist = uiDist;
        ruiMinRate = uiRate;
        m_pcPicYuvTmp->copyToPicLuma(m_pcPicYuvBest);
        copyALFParam(m_pcBestAlfParam, m_pcTempAlfParam);
        xCopyTmpAlfCtrlFlagsFrom();
      }
    }
  }

  if (m_pcBestAlfParam->cu_control_flag)
  {
    m_pcEntropyCoder->setAlfCtrl(true);
    m_pcEntropyCoder->setMaxAlfCtrlDepth(uiBestDepth);
    xCopyTmpAlfCtrlFlagsTo();
    m_pcPicYuvBest->copyToPicLuma(pcPicRest);
    xCopyDecToRestCUs(pcPicDec, pcPicRest);
  }
  else
  {
    m_pcEntropyCoder->setAlfCtrl(false);
    m_pcEntropyCoder->setMaxAlfCtrlDepth(0);
  }
  freeALFParam(&cFrmAlfParam);
}

Void TEncAdaptiveLoopFilter::xFilterTapDecision(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiMinRate, UInt64& ruiMinDist, Double& rdMinCost)
{
  // restriction for non-referenced B-slice
  if (m_eSliceType == B_SLICE && m_iPicNalReferenceIdc == 0)
  {
    return;
  }

  UInt64 uiRate, uiDist;
  Double dCost;

  if (m_pcBestAlfParam->cu_control_flag)
  {
    xCopyTmpAlfCtrlFlagsFrom();
  }

  Bool bChanged = false;
  for (Int iTap = ALF_MIN_NUM_TAP; iTap <= ALF_MAX_NUM_TAP; iTap += 2)
  {
    copyALFParam(m_pcTempAlfParam, m_pcBestAlfParam);
    m_pcTempAlfParam->tap = iTap;
    m_pcTempAlfParam->num_coeff = ((iTap*iTap+1)>>1) + 1;
    if (m_pcTempAlfParam->cu_control_flag)
    {
      xReDesignFilterCoeff(pcPicOrg, pcPicDec, false);
      xFrame(pcPicDec, m_pcPicYuvTmp, m_pcTempAlfParam->coeff, m_pcTempAlfParam->tap);
#if TSB_ALF_HEADER
      xSetCUAlfCtrlFlags(m_pcEntropyCoder->getMaxAlfCtrlDepth(), pcPicOrg, pcPicDec, m_pcPicYuvTmp, uiDist, m_pcTempAlfParam);
#else
      xSetCUAlfCtrlFlags(m_pcEntropyCoder->getMaxAlfCtrlDepth(), pcPicOrg, pcPicDec, m_pcPicYuvTmp, uiDist);
#endif

			// compute RD cost
      xCalcRDCost(m_pcTempAlfParam, uiRate, uiDist, dCost);
    }
    else
    {
      xFilteringFrameLuma(pcPicOrg, pcPicDec, m_pcPicYuvTmp, false);
      xCalcRDCost(pcPicOrg, m_pcPicYuvTmp, m_pcTempAlfParam, uiRate, uiDist, dCost);
    }

    if (dCost < rdMinCost)
    {
      rdMinCost = dCost;
      ruiMinDist = uiDist;
      ruiMinRate = uiRate;
      m_pcPicYuvTmp->copyToPicLuma(m_pcPicYuvBest);
      copyALFParam(m_pcBestAlfParam, m_pcTempAlfParam);
      bChanged = true;
      if (m_pcTempAlfParam->cu_control_flag)
      {
        xCopyTmpAlfCtrlFlagsFrom();
      }
    }
  }

  if (m_pcBestAlfParam->cu_control_flag)
  {
    xCopyTmpAlfCtrlFlagsTo();
    if (bChanged)
    {
      m_pcPicYuvBest->copyToPicLuma(pcPicRest);
      xCopyDecToRestCUs(pcPicDec, pcPicRest);
    }
  }
  else if (m_pcBestAlfParam->tap > ALF_MIN_NUM_TAP)
  {
    m_pcPicYuvBest->copyToPicLuma(pcPicRest);
  }

  copyALFParam(m_pcTempAlfParam, m_pcBestAlfParam);
}

// ====================================================================================================================
// Private member functions
// ====================================================================================================================

Void TEncAdaptiveLoopFilter::xInitParam()
{
  Int i, j, k, l;

  if (m_ppdAlfCorr != NULL)
  {
    for (i = 0; i < ALF_MAX_NUM_COEF; i++)
    {
      for (j = 0; j < ALF_MAX_NUM_COEF+1; j++)
      {
        m_ppdAlfCorr[i][j] = 0;
      }
    }
  }
  else
  {
    m_ppdAlfCorr = new Double*[ALF_MAX_NUM_COEF];
    for (i = 0; i < ALF_MAX_NUM_COEF; i++)
    {
      m_ppdAlfCorr[i] = new Double[ALF_MAX_NUM_COEF+1];
      for (j = 0; j < ALF_MAX_NUM_COEF+1; j++)
      {
        m_ppdAlfCorr[i][j] = 0;
      }
    }
  }

  if (m_puiCUCorr != NULL)
  {
    for (i = 0; i < m_pcPic->getNumCUsInFrame(); i++)
    {
      for (j = 0; j < m_uiNumSCUInCU; j++)
      {
        for (k = 0; k < ALF_MIN_NUM_COEF; k++)
        {
          for (l = 0; l< ALF_MIN_NUM_COEF+1; l++)
          {
            m_puiCUCorr[i][j][k][l] = 0;
          }
        }
      }
    }
  }
  else
  {
    m_puiCUCorr = new CorrBlk*[m_pcPic->getNumCUsInFrame()];
    for (i = 0; i < m_pcPic->getNumCUsInFrame(); i++)
    {
      m_puiCUCorr[i] = new CorrBlk[m_uiNumSCUInCU];

      for (j = 0; j < m_uiNumSCUInCU; j++)
      {
        for (k = 0; k < ALF_MIN_NUM_COEF; k++)
        {
          for (l = 0; l< ALF_MIN_NUM_COEF+1; l++)
          {
            m_puiCUCorr[i][j][k][l] = 0;
          }
        }
      }        
    }
  }

  if (m_pdDoubleAlfCoeff != NULL)
  {
    for (i = 0; i < ALF_MAX_NUM_COEF; i++)
    {
      m_pdDoubleAlfCoeff[i] = 0;
    }
  }
  else
  {
    m_pdDoubleAlfCoeff = new Double[ALF_MAX_NUM_COEF];
    for (i = 0; i < ALF_MAX_NUM_COEF; i++)
    {
      m_pdDoubleAlfCoeff[i] = 0;
    }
  }
}

Void TEncAdaptiveLoopFilter::xUninitParam()
{
  Int i;

  if (m_ppdAlfCorr != NULL)
  {
    for (i = 0; i < ALF_MAX_NUM_COEF; i++)
    {
      delete[] m_ppdAlfCorr[i];
      m_ppdAlfCorr[i] = NULL;
    }
    delete[] m_ppdAlfCorr;
    m_ppdAlfCorr = NULL;
  }

  if (m_puiCUCorr != NULL)
  {
    for (i = 0; i < m_pcPic->getNumCUsInFrame(); i++)
    {
      delete[] m_puiCUCorr[i];
      m_puiCUCorr[i] = NULL;
    }
    delete[] m_puiCUCorr;
    m_puiCUCorr = NULL;
  }

  if (m_pdDoubleAlfCoeff != NULL)
  {
    delete[] m_pdDoubleAlfCoeff;
    m_pdDoubleAlfCoeff = NULL;
  }
}

Void TEncAdaptiveLoopFilter::xCreateTmpAlfCtrlFlags()
{
  for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = m_pcPic->getCU( uiCUAddr );
    pcCU->createTmpAlfCtrlFlag();
  }
}

Void TEncAdaptiveLoopFilter::xDestroyTmpAlfCtrlFlags()
{
  for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = m_pcPic->getCU( uiCUAddr );
    pcCU->destroyTmpAlfCtrlFlag();
  }
}

Void TEncAdaptiveLoopFilter::xCopyTmpAlfCtrlFlagsTo()
{
  for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = m_pcPic->getCU( uiCUAddr );
    pcCU->copyAlfCtrlFlagFromTmp();
  }
}

Void TEncAdaptiveLoopFilter::xCopyTmpAlfCtrlFlagsFrom()
{
  for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = m_pcPic->getCU( uiCUAddr );
    pcCU->copyAlfCtrlFlagToTmp();
  }
}

Void TEncAdaptiveLoopFilter::xReadOrCalcCorrFromCUs(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, Bool bReadCorr)
{
  Int N = m_pcTempAlfParam->num_coeff;
  Int tap = m_pcTempAlfParam->tap;

  for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = m_pcPic->getCU( uiCUAddr );
    for (UInt uiIdx = 0; uiIdx < pcCU->getTotalNumPart(); uiIdx+=4)
    {
      UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];

      if (uiLPelX >= pcPicOrg->getWidth() || uiTPelY >= pcPicOrg->getHeight())
      {
        continue;
      }

      if (pcCU->getAlfCtrlFlag(uiIdx))
      {
        if (bReadCorr)
        {
          for(Int j=0; j<N; j++)
          {
            for(Int k=j; k<N+1; k++)
            {
              m_ppdAlfCorr[j][k] += (Double) m_puiCUCorr[uiCUAddr][(uiIdx>>2)][j][k-j];
            }
          }
        }
        else
        {
          Pel* pOrg = pcPicOrg->getLumaAddr(uiCUAddr, uiIdx);
          Pel* pCmp = pcPicDec->getLumaAddr(uiCUAddr, uiIdx);

          xCalcCorrelationFuncBlock(pOrg, pCmp, tap, m_uiSCUWidth, m_uiSCUHeight, pcPicOrg->getStride(), pcPicDec->getStride());
        }
      }
    }
  }
}

Void TEncAdaptiveLoopFilter::xEncodeCUAlfCtrlFlags()
{
  for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = m_pcPic->getCU( uiCUAddr );
    xEncodeCUAlfCtrlFlag(pcCU, 0, 0);
  }
}

Void TEncAdaptiveLoopFilter::xEncodeCUAlfCtrlFlag(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth)
{
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;

  if( ( uiRPelX >= pcCU->getSlice()->getSPS()->getWidth() ) || ( uiBPelY >= pcCU->getSlice()->getSPS()->getHeight() ) )
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < (g_uiMaxCUDepth-g_uiAddCUDepth) ) ) || bBoundary )
  {
    UInt uiQNumParts = ( m_pcPic->getNumPartInCU() >> (uiDepth<<1) )>>2;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
        xEncodeCUAlfCtrlFlag(pcCU, uiAbsPartIdx, uiDepth+1);
    }
    return;
  }

  m_pcEntropyCoder->encodeAlfCtrlFlag(pcCU, uiAbsPartIdx);
}

Void TEncAdaptiveLoopFilter::xCalcALFCoeff( ALFParam* pAlfParam )
{
  Int iErrCode;

  Int    *qh;

  Int tap			= pAlfParam->tap;
  Int N				= pAlfParam->num_coeff;
  Double* h   = m_pdDoubleAlfCoeff;
  qh					= pAlfParam->coeff;

  iErrCode = xGauss(m_ppdAlfCorr, N);

  if(iErrCode)
  {
    xClearFilterCoefInt(pAlfParam->coeff, N);
  }
  else
  {
    for(Int i=0; i<N; i++)
      h[i] = m_ppdAlfCorr[i][N];
    xQuantFilterCoef(h, pAlfParam->coeff, tap, g_uiBitDepth + g_uiBitIncrement);
  }
}

Void TEncAdaptiveLoopFilter::xCalcStoredCorrelationFuncBlock(Pel* pOrg, Pel* pCmp, CorrBlk& ppuiCorr, Int iTap, Int iWidth, Int iHeight, Int iOrgStride, Int iCmpStride)
{
  Int N      = (iTap*iTap+1)>>1;
  Int offset = iTap>>1;

  const Int* pFiltPos;

  switch(iTap)
  {
  case 5:
    pFiltPos = m_aiSymmetricArray5x5;
    break;
  case 7:
    pFiltPos = m_aiSymmetricArray7x7;
    break;
  case 9:
    pFiltPos = m_aiSymmetricArray9x9;
    break;
  default:
		pFiltPos = m_aiSymmetricArray9x9;
    assert(0);
    break;
  }

  UInt* pTerm = new UInt[N];

  Int i, j;

  for (Int y = 0; y < iHeight; y++)
  {
    for (Int x = 0; x < iWidth; x++)
    {
      i = 0;
      ::memset(pTerm, 0, sizeof(UInt)*N);
      for(Int yy=y-offset; yy<=y+offset; yy++)
      {
        for(Int xx=x-offset; xx<=x+offset; xx++)
        {
          pTerm[pFiltPos[i]] += (UInt) pCmp[xx + yy*iCmpStride];
          i++;
        }
      }

      for(j=0; j<N; j++)
      {
        ppuiCorr[j][0] += pTerm[j]*pTerm[j];
        for(i=j+1; i<N; i++)
          ppuiCorr[j][i-j] += pTerm[j]*pTerm[i];

        // DC offset
        ppuiCorr[j][N-j]   += pTerm[j];
        ppuiCorr[j][N-j+1] += (UInt) pOrg[x+y*iOrgStride]*pTerm[j];
      }
      // DC offset
      ppuiCorr[N][0] += 1;
      ppuiCorr[N][1] += (UInt) pOrg[x+y*iOrgStride];
    }
  }

  delete[] pTerm;
  pTerm = NULL;
}

Void TEncAdaptiveLoopFilter::xCalcCorrelationFunc(Pel* pOrg, Pel* pCmp, Int iTap, Int iWidth, Int iHeight, Int iOrgStride, Int iCmpStride)
{
  //Patch should be extended before this point................
  //ext_offset  = tap>>1;

  Int N      = (iTap*iTap+1)>>1;
  Int offset = iTap>>1;

  const Int* pFiltPos;

  switch(iTap)
  {
  case 5:
    pFiltPos = m_aiSymmetricArray5x5;
    break;
  case 7:
    pFiltPos = m_aiSymmetricArray7x7;
    break;
  case 9:
    pFiltPos = m_aiSymmetricArray9x9;
    break;
  default:
		pFiltPos = m_aiSymmetricArray9x9;
    assert(0);
    break;
  }

  Pel* pTerm = new Pel[N];

  Int i, j;

  for (Int y = 0; y < iHeight; y++)
  {
    for (Int x = 0; x < iWidth; x++)
    {
      i = 0;
      ::memset(pTerm, 0, sizeof(Pel)*N);
      for(Int yy=y-offset; yy<=y+offset; yy++)
      {
        for(Int xx=x-offset; xx<=x+offset; xx++)
        {
          pTerm[pFiltPos[i]] += pCmp[xx + yy*iCmpStride];
          i++;
        }
      }

      for(j=0; j<N; j++)
      {
        m_ppdAlfCorr[j][j] += pTerm[j]*pTerm[j];
        for(i=j+1; i<N; i++)
          m_ppdAlfCorr[j][i] += pTerm[j]*pTerm[i];

        // DC offset
        m_ppdAlfCorr[j][N]   += pTerm[j];
        m_ppdAlfCorr[j][N+1] += pOrg[x+y*iOrgStride]*pTerm[j];
      }
      // DC offset
      for(i=0; i<N; i++)
        m_ppdAlfCorr[N][i] += pTerm[i];
      m_ppdAlfCorr[N][N]   += 1;
      m_ppdAlfCorr[N][N+1] += pOrg[x+y*iOrgStride];
    }
  }

  for(j=0; j<N-1; j++)
    for(i=j+1; i<N; i++)
      m_ppdAlfCorr[i][j] = m_ppdAlfCorr[j][i];

  delete[] pTerm;
  pTerm = NULL;
}

Void TEncAdaptiveLoopFilter::xCalcCorrelationFuncBlock(Pel* pOrg, Pel* pCmp, Int iTap, Int iWidth, Int iHeight, Int iOrgStride, Int iCmpStride)
{
  //Patch should be extended before this point................
  //ext_offset  = tap>>1;

  Int N      = (iTap*iTap+1)>>1;
  Int offset = iTap>>1;

  const Int* pFiltPos;

  switch(iTap)
  {
  case 5:
    pFiltPos = m_aiSymmetricArray5x5;
    break;
  case 7:
    pFiltPos = m_aiSymmetricArray7x7;
    break;
  case 9:
    pFiltPos = m_aiSymmetricArray9x9;
    break;
  default:
		pFiltPos = m_aiSymmetricArray9x9;
    assert(0);
    break;
  }

  Pel* pTerm = new Pel[N];

  Int i, j;

  for (Int y = 0; y < iHeight; y++)
  {
    for (Int x = 0; x < iWidth; x++)
    {
      i = 0;
      ::memset(pTerm, 0, sizeof(Pel)*N);
      for(Int yy=y-offset; yy<=y+offset; yy++)
      {
        for(Int xx=x-offset; xx<=x+offset; xx++)
        {
          pTerm[pFiltPos[i]] += pCmp[xx + yy*iCmpStride];
          i++;
        }
      }

      for(j=0; j<N; j++)
      {
        m_ppdAlfCorr[j][j] += pTerm[j]*pTerm[j];
        for(i=j+1; i<N; i++)
          m_ppdAlfCorr[j][i] += pTerm[j]*pTerm[i];

        // DC offset
        m_ppdAlfCorr[j][N]   += pTerm[j];
        m_ppdAlfCorr[j][N+1] += pOrg[x+y*iOrgStride]*pTerm[j];
      }
      // DC offset
      for(i=0; i<N; i++)
        m_ppdAlfCorr[N][i] += pTerm[i];
      m_ppdAlfCorr[N][N]   += 1;
      m_ppdAlfCorr[N][N+1] += pOrg[x+y*iOrgStride];
    }
  }

  delete[] pTerm;
  pTerm = NULL;
}

UInt64 TEncAdaptiveLoopFilter::xCalcSSD(Pel* pOrg, Pel* pCmp, Int iWidth, Int iHeight, Int iStride )
{
  UInt64 uiSSD = 0;
  Int x, y;

  UInt uiShift = g_uiBitIncrement<<1;
  Int iTemp;

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iTemp = pOrg[x] - pCmp[x]; uiSSD += ( iTemp * iTemp ) >> uiShift;
    }
    pOrg += iStride;
    pCmp += iStride;
  }

	return uiSSD;;
}

Int TEncAdaptiveLoopFilter::xGauss(Double **a, Int N)
{
  Int i, j, k;
  Double t;

#if ALF_FIX
  for(k=0; k<N; k++)
  {
    if (a[k][k] <0.000001)
        return 1;
  }
#endif

  for(k=0; k<N-1; k++)
  {
    for(i=k+1;i<N; i++)
    {
      t=a[i][k]/a[k][k];
      for(j=k+1; j<=N; j++)
      {
        a[i][j] -= t * a[k][j];
        if(i==j && fabs(a[i][j])<0.000001) return 1;
      }
    }
  }
  for(i=N-1; i>=0; i--)
  {
    t = a[i][N];
    for(j=i+1; j<N; j++)
      t -= a[i][j] * a[j][N];
    a[i][N] = t / a[i][i];
  }
  return 0;
}

Void TEncAdaptiveLoopFilter::xFilterCoefQuickSort( Double *coef_data, Int *coef_num, Int upper, Int lower )
{
  Double mid, tmp_data;
  Int i, j, tmp_num;

  i = upper;
  j = lower;
  mid = coef_data[(lower+upper)>>1];
  do
  {
    while( coef_data[i] < mid ) i++;
    while( mid < coef_data[j] ) j--;
    if( i <= j )
    {
      tmp_data = coef_data[i];
      tmp_num  = coef_num[i];
      coef_data[i] = coef_data[j];
      coef_num[i]  = coef_num[j];
      coef_data[j] = tmp_data;
      coef_num[j]  = tmp_num;
      i++;
      j--;
    }
  } while( i <= j );
  if ( upper < j ) xFilterCoefQuickSort(coef_data, coef_num, upper, j);
  if ( i < lower ) xFilterCoefQuickSort(coef_data, coef_num, i, lower);
}

Void TEncAdaptiveLoopFilter::xQuantFilterCoef(Double* h, Int* qh, Int tap, int bit_depth)
{
  Int i, N;
  Int max_value, min_value;
  Double dbl_total_gain;
  Int total_gain, q_total_gain;
  Int upper, lower;
  Double *dh;
  Int    *nc;
  const Int    *pFiltMag;

  switch(tap)
  {
  case 5:
    pFiltMag = m_aiSymmetricMag5x5;
    break;
  case 7:
    pFiltMag = m_aiSymmetricMag7x7;
    break;
  case 9:
    pFiltMag = m_aiSymmetricMag9x9;
    break;
  default:
		pFiltMag = m_aiSymmetricMag9x9;
    assert(0);
    break;
  }

  N = (tap*tap+1)>>1;

  dh = new Double[N];
  nc = new Int[N];

  max_value =   (1<<(1+ALF_NUM_BIT_SHIFT))-1;
  min_value = 0-(1<<(1+ALF_NUM_BIT_SHIFT));

  dbl_total_gain=0.0;
  q_total_gain=0;
  for(i=0; i<N; i++)
  {
    if(h[i]>=0.0)
      qh[i] =  (Int)( h[i]*(1<<ALF_NUM_BIT_SHIFT)+0.5);
    else
      qh[i] = -(Int)(-h[i]*(1<<ALF_NUM_BIT_SHIFT)+0.5);

    dh[i] = (Double)qh[i]/(Double)(1<<ALF_NUM_BIT_SHIFT) - h[i];
    dh[i]*=pFiltMag[i];
    dbl_total_gain += h[i]*pFiltMag[i];
    q_total_gain   += qh[i]*pFiltMag[i];
    nc[i] = i;
  }

  // modification of quantized filter coefficients
  total_gain = (Int)(dbl_total_gain*(1<<ALF_NUM_BIT_SHIFT)+0.5);

  if( q_total_gain != total_gain )
  {
    xFilterCoefQuickSort(dh, nc, 0, N-1);
    if( q_total_gain > total_gain )
    {
      upper = N-1;
      while( q_total_gain > total_gain+1 )
      {
        i = nc[upper%N];
        qh[i]--;
        q_total_gain -= pFiltMag[i];
        upper--;
      }
      if( q_total_gain == total_gain+1 )
      {
        if(dh[N-1]>0)
          qh[N-1]--;
        else
        {
          i=nc[upper%N];
          qh[i]--;
          qh[N-1]++;
        }
      }
    }
    else if( q_total_gain < total_gain )
    {
      lower = 0;
      while( q_total_gain < total_gain-1 )
      {
        i=nc[lower%N];
        qh[i]++;
        q_total_gain += pFiltMag[i];
        lower++;
      }
      if( q_total_gain == total_gain-1 )
      {
        if(dh[N-1]<0)
          qh[N-1]++;
        else
        {
          i=nc[lower%N];
          qh[i]++;
          qh[N-1]--;
        }
      }
    }
  }

  // set of filter coefficients
  for(i=0; i<N; i++)
  {
    qh[i] = Max(min_value,Min(max_value, qh[i]));
  }

  // DC offset
//  max_value = Min(  (1<<(3+Max(img_bitdepth_luma,img_bitdepth_chroma)))-1, (1<<14)-1);
//  min_value = Max( -(1<<(3+Max(img_bitdepth_luma,img_bitdepth_chroma))),  -(1<<14)  );
  max_value = Min(  (1<<(3+g_uiBitDepth + g_uiBitIncrement))-1, (1<<14)-1);
  min_value = Max( -(1<<(3+g_uiBitDepth + g_uiBitIncrement)),  -(1<<14)  );

  qh[N] =  (h[N]>=0.0)? (Int)( h[N]*(1<<(ALF_NUM_BIT_SHIFT-bit_depth+8)) + 0.5) : -(Int)(-h[N]*(1<<(ALF_NUM_BIT_SHIFT-bit_depth+8)) + 0.5);
  qh[N] = Max(min_value,Min(max_value, qh[N]));

  delete[] dh;
  dh = NULL;

  delete[] nc;
  nc = NULL;
}

Void TEncAdaptiveLoopFilter::xClearFilterCoefInt(Int* qh, Int N)
{
	// clear
  memset( qh, 0, sizeof( Int ) * N );

  // center pos
  qh[N-2]  = 1<<ALF_NUM_BIT_SHIFT;
}

Void TEncAdaptiveLoopFilter::xCalcRDCost(ALFParam* pAlfParam, UInt64& ruiRate, UInt64 uiDist, Double& rdCost)
{
  if(pAlfParam != NULL)
  {
    Int* piTmpCoef;
    piTmpCoef = new Int[ALF_MAX_NUM_COEF];

    memcpy(piTmpCoef, pAlfParam->coeff, sizeof(Int)*pAlfParam->num_coeff);

    predictALFCoeff(pAlfParam);

    m_pcEntropyCoder->resetEntropy();
    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeAlfParam(pAlfParam);

    if(pAlfParam->cu_control_flag)
    {
#if TSB_ALF_HEADER
      m_pcEntropyCoder->encodeAlfCtrlParam(pAlfParam);
#else
      xEncodeCUAlfCtrlFlags();
#endif
    }
    ruiRate = m_pcEntropyCoder->getNumberOfWrittenBits();
    memcpy(pAlfParam->coeff, piTmpCoef, sizeof(int)*pAlfParam->num_coeff);
    delete[] piTmpCoef;
    piTmpCoef = NULL;
  }
  else
  {
    ruiRate = 1;
  }

  rdCost      = (Double)(ruiRate) * m_dLambdaLuma + (Double)(uiDist);
}

Void TEncAdaptiveLoopFilter::xCalcRDCost(TComPicYuv* pcPicOrg, TComPicYuv* pcPicCmp, ALFParam* pAlfParam, UInt64& ruiRate, UInt64& ruiDist, Double& rdCost)
{
  if(pAlfParam != NULL)
  {
    Int* piTmpCoef;
    piTmpCoef = new Int[ALF_MAX_NUM_COEF];

    memcpy(piTmpCoef, pAlfParam->coeff, sizeof(Int)*pAlfParam->num_coeff);

    predictALFCoeff(pAlfParam);

    m_pcEntropyCoder->resetEntropy();
    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeAlfParam(pAlfParam);

    if(pAlfParam->cu_control_flag)
    {
#if TSB_ALF_HEADER
      m_pcEntropyCoder->encodeAlfCtrlParam(pAlfParam);
#else
      xEncodeCUAlfCtrlFlags();
#endif
    }
    ruiRate = m_pcEntropyCoder->getNumberOfWrittenBits();
    memcpy(pAlfParam->coeff, piTmpCoef, sizeof(int)*pAlfParam->num_coeff);
    delete[] piTmpCoef;
    piTmpCoef = NULL;
  }
  else
  {
    ruiRate = 1;
  }

  ruiDist     = xCalcSSD(pcPicOrg->getLumaAddr(), pcPicCmp->getLumaAddr(), pcPicOrg->getWidth(), pcPicOrg->getHeight(), pcPicOrg->getStride());
  rdCost      = (Double)(ruiRate) * m_dLambdaLuma + (Double)(ruiDist);
}

Void TEncAdaptiveLoopFilter::xCalcRDCostChroma(TComPicYuv* pcPicOrg, TComPicYuv* pcPicCmp, ALFParam* pAlfParam, UInt64& ruiRate, UInt64& ruiDist, Double& rdCost)
{
  if(pAlfParam->chroma_idc)
  {
    Int* piTmpCoef;
    piTmpCoef = new Int[ALF_MAX_NUM_COEF_C];

    memcpy(piTmpCoef, pAlfParam->coeff_chroma, sizeof(Int)*pAlfParam->num_coeff_chroma);

    predictALFCoeffChroma(pAlfParam);

    m_pcEntropyCoder->resetEntropy();
    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeAlfParam(pAlfParam);

    if(pAlfParam->cu_control_flag)
    {
#if TSB_ALF_HEADER
      m_pcEntropyCoder->encodeAlfCtrlParam(pAlfParam);
#else
      xEncodeCUAlfCtrlFlags();
#endif
    }
    ruiRate = m_pcEntropyCoder->getNumberOfWrittenBits();
    memcpy(pAlfParam->coeff_chroma, piTmpCoef, sizeof(int)*pAlfParam->num_coeff_chroma);
    delete[] piTmpCoef;
    piTmpCoef = NULL;
  }
  ruiDist = 0;
  ruiDist += xCalcSSD(pcPicOrg->getCbAddr(), pcPicCmp->getCbAddr(), (pcPicOrg->getWidth()>>1), (pcPicOrg->getHeight()>>1), pcPicOrg->getCStride());
  ruiDist += xCalcSSD(pcPicOrg->getCrAddr(), pcPicCmp->getCrAddr(), (pcPicOrg->getWidth()>>1), (pcPicOrg->getHeight()>>1), pcPicOrg->getCStride());
  rdCost  = (Double)(ruiRate) * m_dLambdaChroma + (Double)(ruiDist);
}

Void TEncAdaptiveLoopFilter::xFilteringFrameLuma(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, Bool bStoreCorr)
{
  Int    i, tap, N, err_code;
  Int* qh;
  Int    j, k;
  
  tap  = m_pcTempAlfParam->tap;
  N    = m_pcTempAlfParam->num_coeff;
  qh   = m_pcTempAlfParam->coeff;

  // initialize correlation
  for(i=0; i<N; i++)
    memset(m_ppdAlfCorr[i], 0, sizeof(Double)*(N+1));

  if(bStoreCorr)
  {
    // store correlation per minimum size cu
    for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
    for( UInt uiIdx = 0; uiIdx < m_pcPic->getNumPartInCU() ; uiIdx+=4 )
    {
      TComDataCU* pcCU = m_pcPic->getCU(uiCUAddr);
      UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];

      if (uiLPelX >= pcPicOrg->getWidth() || uiTPelY >= pcPicOrg->getHeight())
      {
        continue;
      }
      CorrBlk &ppuiBlkCorr = m_puiCUCorr[uiCUAddr][(uiIdx>>2)]; 

      for(j=0; j<N; j++)
        memset(ppuiBlkCorr[j], 0, sizeof(UInt)*(N+1-j));      

      Pel* pOrg = pcPicOrg->getLumaAddr(uiCUAddr, uiIdx);
      Pel* pCmp = pcPicDec->getLumaAddr(uiCUAddr, uiIdx);
      xCalcStoredCorrelationFuncBlock(pOrg, pCmp, ppuiBlkCorr, tap, m_uiSCUWidth, m_uiSCUHeight, pcPicOrg->getStride(), pcPicDec->getStride());
      
      for(j=0; j<N; j++)
        for(k=j; k<N+1; k++)
          m_ppdAlfCorr[j][k] += (Double) ppuiBlkCorr[j][k-j];
    }
    for(j=0; j<N-1; j++)
      for(k=j+1; k<N; k++)
        m_ppdAlfCorr[k][j] = m_ppdAlfCorr[j][k];
  }
  else
  {
    Pel* pOrg = pcPicOrg->getLumaAddr();
    Pel* pCmp = pcPicDec->getLumaAddr();

    xCalcCorrelationFunc(pOrg, pCmp, tap, pcPicOrg->getWidth(), pcPicOrg->getHeight(), pcPicOrg->getStride(), pcPicDec->getStride());
  }

  err_code = xGauss(m_ppdAlfCorr, N);

  if(err_code)
  {
    xClearFilterCoefInt(qh, N);
  }
  else
  {
    for(i=0; i<N; i++)
      m_pdDoubleAlfCoeff[i] = m_ppdAlfCorr[i][N];

    xQuantFilterCoef(m_pdDoubleAlfCoeff, qh, tap, g_uiBitDepth + g_uiBitIncrement);
  }

  xFrame(pcPicDec, pcPicRest, qh, tap);
}

Void TEncAdaptiveLoopFilter::xFilteringFrameChroma(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest)
{
  Int    i, tap, N, err_code;
  Int* qh;

  tap  = m_pcTempAlfParam->tap_chroma;
  N    = m_pcTempAlfParam->num_coeff_chroma;
  qh   = m_pcTempAlfParam->coeff_chroma;

  // initialize correlation
  for(i=0; i<N; i++)
    memset(m_ppdAlfCorr[i], 0, sizeof(Double)*(N+1));

  if ((m_pcTempAlfParam->chroma_idc>>1)&0x01)
  {
    Pel* pOrg = pcPicOrg->getCbAddr();
    Pel* pCmp = pcPicDec->getCbAddr();

    xCalcCorrelationFunc(pOrg, pCmp, tap, (pcPicOrg->getWidth()>>1), (pcPicOrg->getHeight()>>1), pcPicOrg->getCStride(), pcPicDec->getCStride());
  }
  if ((m_pcTempAlfParam->chroma_idc)&0x01)
  {
    Pel* pOrg = pcPicOrg->getCrAddr();
    Pel* pCmp = pcPicDec->getCrAddr();

    xCalcCorrelationFunc(pOrg, pCmp, tap, (pcPicOrg->getWidth()>>1), (pcPicOrg->getHeight()>>1), pcPicOrg->getCStride(), pcPicDec->getCStride());
  }

  err_code = xGauss(m_ppdAlfCorr, N);

  if(err_code)
  {
    xClearFilterCoefInt(qh, N);
  }
  else
  {
    for(i=0; i<N; i++)
      m_pdDoubleAlfCoeff[i] = m_ppdAlfCorr[i][N];

    xQuantFilterCoef(m_pdDoubleAlfCoeff, qh, tap, g_uiBitDepth + g_uiBitIncrement);
  }


  if ((m_pcTempAlfParam->chroma_idc>>1)&0x01)
  {
    xFrameChroma(pcPicDec, pcPicRest, qh, tap, 0);
  }
  if ((m_pcTempAlfParam->chroma_idc)&0x01)
  {
    xFrameChroma(pcPicDec, pcPicRest, qh, tap, 1);
  }

  if(m_pcTempAlfParam->chroma_idc<3)
  {
    if(m_pcTempAlfParam->chroma_idc==1)
    {
      pcPicDec->copyToPicCb(pcPicRest);
    }
    if(m_pcTempAlfParam->chroma_idc==2)
    {
      pcPicDec->copyToPicCr(pcPicRest);
    }
  }

}

Void TEncAdaptiveLoopFilter::xReDesignFilterCoeff(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, Bool bReadCorr)
{
  Int i, j, k;

  Int tap = m_pcTempAlfParam->tap;
  Int N = m_pcTempAlfParam->num_coeff;
  Int* qh = m_pcTempAlfParam->coeff;
  // initialize correlation
  for(i=0; i<N; i++)
    memset(m_ppdAlfCorr[i], 0, sizeof(Double)*(N+1));

  xReadOrCalcCorrFromCUs(pcPicOrg, pcPicDec, bReadCorr);

  for(j=0; j<N-1; j++)
    for(k=j+1; k<N; k++)
      m_ppdAlfCorr[k][j] = m_ppdAlfCorr[j][k];

  Int err_code = xGauss(m_ppdAlfCorr, N);

  if(err_code)
  {
    xClearFilterCoefInt(qh, N);
  }
  else
  {
    for(i=0; i<N; i++)
      m_pdDoubleAlfCoeff[i] = m_ppdAlfCorr[i][N];

    xQuantFilterCoef(m_pdDoubleAlfCoeff, qh, tap, g_uiBitDepth + g_uiBitIncrement);
  }
}

#if TSB_ALF_HEADER
Void TEncAdaptiveLoopFilter::xSetCUAlfCtrlFlags(UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist, ALFParam *pAlfParam)
#else
Void TEncAdaptiveLoopFilter::xSetCUAlfCtrlFlags(UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist)
#endif
{
  ruiDist = 0;
#if TSB_ALF_HEADER
  pAlfParam->num_alf_cu_flag = 0;
#endif

  for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = m_pcPic->getCU( uiCUAddr );
#if TSB_ALF_HEADER
    xSetCUAlfCtrlFlag(pcCU, 0, 0, uiAlfCtrlDepth, pcPicOrg, pcPicDec, pcPicRest, ruiDist, pAlfParam);
#else
    xSetCUAlfCtrlFlag(pcCU, 0, 0, uiAlfCtrlDepth, pcPicOrg, pcPicDec, pcPicRest, ruiDist);
#endif
  }
}

#if TSB_ALF_HEADER
Void TEncAdaptiveLoopFilter::xSetCUAlfCtrlFlag(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist, ALFParam *pAlfParam)
#else
Void TEncAdaptiveLoopFilter::xSetCUAlfCtrlFlag(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist)
#endif
{
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;

  if( ( uiRPelX >= pcCU->getSlice()->getSPS()->getWidth() ) || ( uiBPelY >= pcCU->getSlice()->getSPS()->getHeight() ) )
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < (g_uiMaxCUDepth-g_uiAddCUDepth) ) ) || bBoundary )
  {
    UInt uiQNumParts = ( m_pcPic->getNumPartInCU() >> (uiDepth<<1) )>>2;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
#if TSB_ALF_HEADER
        xSetCUAlfCtrlFlag(pcCU, uiAbsPartIdx, uiDepth+1, uiAlfCtrlDepth, pcPicOrg, pcPicDec, pcPicRest, ruiDist, pAlfParam);
#else
        xSetCUAlfCtrlFlag(pcCU, uiAbsPartIdx, uiDepth+1, uiAlfCtrlDepth, pcPicOrg, pcPicDec, pcPicRest, ruiDist);
#endif
    }
    return;
  }

  if( uiDepth > uiAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, uiAlfCtrlDepth))
  {
    return;
  }

  UInt uiCUAddr = pcCU->getAddr();
  UInt64 uiRecSSD = 0;
  UInt64 uiFiltSSD = 0;

  Int iWidth;
  Int iHeight;
  UInt uiSetDepth;

  if (uiDepth > uiAlfCtrlDepth && pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, uiAlfCtrlDepth))
  {
    iWidth = g_uiMaxCUWidth >> uiAlfCtrlDepth;
    iHeight = g_uiMaxCUHeight >> uiAlfCtrlDepth;

    uiRPelX   = uiLPelX + iWidth  - 1;
    uiBPelY   = uiTPelY + iHeight - 1;

    if( uiRPelX >= pcCU->getSlice()->getSPS()->getWidth() )
    {
      iWidth = pcCU->getSlice()->getSPS()->getWidth() - uiLPelX;
    }

    if( uiBPelY >= pcCU->getSlice()->getSPS()->getHeight() )
    {
      iHeight = pcCU->getSlice()->getSPS()->getHeight() - uiTPelY;
    }

    uiSetDepth = uiAlfCtrlDepth;
  }
  else
  {
    iWidth = pcCU->getWidth(uiAbsPartIdx);
    iHeight = pcCU->getHeight(uiAbsPartIdx);
    uiSetDepth = uiDepth;
  }

  Pel* pOrg = pcPicOrg->getLumaAddr(uiCUAddr, uiAbsPartIdx);
  Pel* pRec = pcPicDec->getLumaAddr(uiCUAddr, uiAbsPartIdx);
  Pel* pFilt = pcPicRest->getLumaAddr(uiCUAddr, uiAbsPartIdx);

  uiRecSSD  += xCalcSSD( pOrg, pRec,  iWidth, iHeight, pcPicOrg->getStride() );
  uiFiltSSD += xCalcSSD( pOrg, pFilt, iWidth, iHeight, pcPicOrg->getStride() );

  if (uiFiltSSD < uiRecSSD)
  {
    ruiDist += uiFiltSSD;
    pcCU->setAlfCtrlFlagSubParts(1, uiAbsPartIdx, uiSetDepth);
#if TSB_ALF_HEADER
    pAlfParam->alf_cu_flag[pAlfParam->num_alf_cu_flag]=1;
#endif
  }
  else
  {
    ruiDist += uiRecSSD;
    pcCU->setAlfCtrlFlagSubParts(0, uiAbsPartIdx, uiSetDepth);
#if TSB_ALF_HEADER
    pAlfParam->alf_cu_flag[pAlfParam->num_alf_cu_flag]=0;
#endif
  }
#if TSB_ALF_HEADER
  pAlfParam->num_alf_cu_flag++;
#endif
}

Void TEncAdaptiveLoopFilter::xCopyDecToRestCUs(TComPicYuv* pcPicDec, TComPicYuv* pcPicRest)
{
  for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = m_pcPic->getCU( uiCUAddr );
    xCopyDecToRestCU(pcCU, 0, 0, pcPicDec, pcPicRest);
  }
}

Void TEncAdaptiveLoopFilter::xCopyDecToRestCU(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest)
{
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;

  if( ( uiRPelX >= pcCU->getSlice()->getSPS()->getWidth() ) || ( uiBPelY >= pcCU->getSlice()->getSPS()->getHeight() ) )
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < (g_uiMaxCUDepth-g_uiAddCUDepth) ) ) || bBoundary )
  {
    UInt uiQNumParts = ( m_pcPic->getNumPartInCU() >> (uiDepth<<1) )>>2;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
        xCopyDecToRestCU(pcCU, uiAbsPartIdx, uiDepth+1, pcPicDec, pcPicRest);
    }
    return;
  }

  if (!pcCU->getAlfCtrlFlag(uiAbsPartIdx))
  {
    UInt uiCUAddr = pcCU->getAddr();

    Int iWidth = pcCU->getWidth(uiAbsPartIdx);
    Int iHeight = pcCU->getHeight(uiAbsPartIdx);

    Pel* pRec = pcPicDec->getLumaAddr(uiCUAddr, uiAbsPartIdx);
    Pel* pFilt = pcPicRest->getLumaAddr(uiCUAddr, uiAbsPartIdx);

    Int iRecStride = pcPicDec->getStride();
    Int iFiltStride = pcPicRest->getStride();

    for (Int y = 0; y < iHeight; y++)
    {
      for (Int x = 0; x < iWidth; x++)
      {
        pFilt[x] = pRec[x];
      }
      pRec += iRecStride;
      pFilt += iFiltStride;
    }
  }
}

#if QC_ALF
Void TEncAdaptiveLoopFilter::xcollectStatCodeFilterCoeffForce0(int **pDiffQFilterCoeffIntPP, int fl, int sqrFiltLength, 
                                     int filters_per_group, int bitsVarBin[])
{
  int i, k, kMin, kStart, minBits, ind, scanPos, maxScanVal, coeffVal, 
    *pDepthInt=NULL, kMinTab[MAX_SQR_FILT_LENGTH], bitsCoeffScan[MAX_SCAN_VAL][MAX_EXP_GOLOMB],
    minKStart, minBitsKStart, bitsKStart;

  pDepthInt=pDepthIntTab[fl-2];

  maxScanVal=0;
  for (i=0; i<sqrFiltLength; i++)
  {
    maxScanVal=max(maxScanVal, pDepthInt[i]);
  }

  // vlc for all
  memset(bitsCoeffScan, 0, MAX_SCAN_VAL * MAX_EXP_GOLOMB * sizeof(int));
  for(ind=0; ind<filters_per_group; ++ind){	
    for(i = 0; i < sqrFiltLength; i++){	     
      scanPos=pDepthInt[i]-1;
      coeffVal=abs(pDiffQFilterCoeffIntPP[ind][i]);
      for (k=1; k<15; k++){
        bitsCoeffScan[scanPos][k] += lengthGolomb(coeffVal, k);
      }
    }
  }

  minBitsKStart=0;
  minKStart = -1;
  for (k=1; k<8; k++){ 
    bitsKStart=0; kStart=k;
    for (scanPos=0; scanPos<maxScanVal; scanPos++){
      kMin=kStart; minBits=bitsCoeffScan[scanPos][kMin];

      if (bitsCoeffScan[scanPos][kStart+1]<minBits){
        kMin=kStart+1; minBits=bitsCoeffScan[scanPos][kMin];
      }
      kStart=kMin;
      bitsKStart+=minBits;
    }
    if (bitsKStart<minBitsKStart || k==1){
      minBitsKStart=bitsKStart;
      minKStart=k;
    }
  }

  kStart = minKStart; 
  for (scanPos=0; scanPos<maxScanVal; scanPos++)
  {
    kMin=kStart; minBits=bitsCoeffScan[scanPos][kMin];

    if (bitsCoeffScan[scanPos][kStart+1]<minBits)
    {
      kMin = kStart+1; 
      minBits = bitsCoeffScan[scanPos][kMin];
    }
   
    kMinTab[scanPos] = kMin;
    kStart = kMin;
  }

  for(ind=0; ind<filters_per_group; ++ind){
    bitsVarBin[ind]=0;
    for(i = 0; i < sqrFiltLength; i++){	
      scanPos=pDepthInt[i]-1;
      bitsVarBin[ind] += lengthGolomb(abs(pDiffQFilterCoeffIntPP[ind][i]), kMinTab[scanPos]);
    }
  }
}

Void TEncAdaptiveLoopFilter::xdecideCoeffForce0(int codedVarBins[NO_VAR_BINS], double errorForce0Coeff[], double errorForce0CoeffTab[NO_VAR_BINS][2], int bitsVarBin[NO_VAR_BINS], double lambda, int filters_per_fr)
{
  int filtNo;
  double lagrangianDiff;
  int ind;

  errorForce0Coeff[0]=errorForce0Coeff[1]=0;
  for (ind=0; ind<16; ind++) codedVarBins[ind]=0;

  for(filtNo=0; filtNo<filters_per_fr; filtNo++)
  {
    // No coeffcient prediction bits used
#if ENABLE_FORCECOEFF0
    lagrangianDiff=errorForce0CoeffTab[filtNo][0]-(errorForce0CoeffTab[filtNo][1]+lambda*bitsVarBin[filtNo]);
    codedVarBins[filtNo]=(lagrangianDiff>0)? 1 : 0;
    errorForce0Coeff[0]+=errorForce0CoeffTab[filtNo][codedVarBins[filtNo]];
    errorForce0Coeff[1]+=errorForce0CoeffTab[filtNo][1];
#else
    lagrangianDiff=errorForce0CoeffTab[filtNo][0]-(errorForce0CoeffTab[filtNo][1]+lambda*bitsVarBin[filtNo]);
    codedVarBins[filtNo]= 1;
    errorForce0Coeff[0]+=errorForce0CoeffTab[filtNo][codedVarBins[filtNo]];
    errorForce0Coeff[1]+=errorForce0CoeffTab[filtNo][1];
#endif
  }   
}

double TEncAdaptiveLoopFilter::xfindBestCoeffCodMethod(int codedVarBins[NO_VAR_BINS], int *forceCoeff0, 
                              int **filterCoeffSymQuant, int fl, int sqrFiltLength, 
                              int filters_per_fr, double errorForce0CoeffTab[NO_VAR_BINS][2], 
                              double *errorQuant, double lambda)

{
  int bitsVarBin[NO_VAR_BINS], createBistream, coeffBits, coeffBitsForce0;
  double errorForce0Coeff[2], lagrangianForce0, lagrangian;

  xcollectStatCodeFilterCoeffForce0(filterCoeffSymQuant, fl, sqrFiltLength,  
    filters_per_fr, bitsVarBin);

  xdecideCoeffForce0(codedVarBins, errorForce0Coeff, errorForce0CoeffTab, bitsVarBin, lambda, filters_per_fr);
  
  coeffBitsForce0 = xsendAllFiltersPPPredForce0(filterCoeffSymQuant, fl, sqrFiltLength, 
    filters_per_fr, codedVarBins, createBistream=0, tempALFp);

  coeffBits = xsendAllFiltersPPPred(filterCoeffSymQuant, fl, sqrFiltLength, filters_per_fr, 
    createBistream=0, tempALFp);

  lagrangianForce0=errorForce0Coeff[0]+lambda*coeffBitsForce0;
  lagrangian=errorForce0Coeff[1]+lambda*coeffBits;
  if (lagrangianForce0<lagrangian)
  {
    *errorQuant=errorForce0Coeff[0];
    *forceCoeff0=1;
    return(lagrangianForce0);
  }
  else
  {
    *errorQuant=errorForce0Coeff[1];
    *forceCoeff0=0;
    return(lagrangian);
  }
}

Int TEncAdaptiveLoopFilter::xsendAllFiltersPPPred(int **FilterCoeffQuant, int fl, int sqrFiltLength, 
                         int filters_per_group, int createBistream, ALFParam* ALFp)
{
  int ind, bit_ct = 0, bit_ct0 = 0, i;
  int predMethod = 0;
  int force0 = 0;
#if !ALF_MEM_PATCH 
  static int **diffFilterCoeffQuant;
  static int first = 1;
#endif
  Int64 Newbit_ct;
  
#if !ALF_MEM_PATCH 
  if(first == 1)
  {
    initMatrix_int(&diffFilterCoeffQuant, NO_VAR_BINS, MAX_SQR_FILT_LENGTH);
    first = 0;
  }
#endif
  bit_ct0 = xcodeFilterCoeff(FilterCoeffQuant, fl, sqrFiltLength, filters_per_group, 0);

  for(ind = 0; ind < filters_per_group; ++ind)
  {		
    if(ind == 0)
    {
      for(i = 0; i < sqrFiltLength; i++)
        diffFilterCoeffQuant[ind][i] = FilterCoeffQuant[ind][i];
    }
    else
    {
      for(i = 0; i < sqrFiltLength; i++)
        diffFilterCoeffQuant[ind][i] = FilterCoeffQuant[ind][i] - FilterCoeffQuant[ind-1][i];
    }
  }

  if(xcodeFilterCoeff(diffFilterCoeffQuant, fl, sqrFiltLength, filters_per_group, 0) >= bit_ct0)
  {
    predMethod = 0;  
    if(filters_per_group > 1)
      bit_ct += lengthPredFlags(force0, predMethod, NULL, 0, createBistream);
    bit_ct += xcodeFilterCoeff(FilterCoeffQuant, fl, sqrFiltLength, filters_per_group, createBistream);
  }
  else
  {
    predMethod = 1;
    if(filters_per_group > 1)
      bit_ct += lengthPredFlags(force0, predMethod, NULL, 0, createBistream);
    bit_ct += xcodeFilterCoeff(diffFilterCoeffQuant, fl, sqrFiltLength, filters_per_group, createBistream);
  }
  
  ALFp->forceCoeff0 = 0;
  ALFp->filters_per_group_diff = filters_per_group;
  ALFp->filters_per_group = filters_per_group;
  ALFp->predMethod = predMethod;
  ALFp->num_coeff = sqrFiltLength;
  if (ALFp->num_coeff == 8) ALFp->realfiltNo=2;
  else if (ALFp->num_coeff == 14) ALFp->realfiltNo=1;
  else ALFp->realfiltNo=0;

  for(ind = 0; ind < filters_per_group; ++ind)
  {		
    for(i = 0; i < sqrFiltLength; i++)
	{
      if (predMethod) ALFp->coeffmulti[ind][i] = diffFilterCoeffQuant[ind][i];
	  else ALFp->coeffmulti[ind][i] = FilterCoeffQuant[ind][i];
	}
  }
  m_pcDummyEntropyCoder->codeFiltCountBit(ALFp, &Newbit_ct);


//  return(bit_ct);
  return ((Int)Newbit_ct);
}


Int TEncAdaptiveLoopFilter::xsendAllFiltersPPPredForce0(int **FilterCoeffQuant, int fl, int sqrFiltLength, int filters_per_group, 
                               int codedVarBins[NO_VAR_BINS], int createBistream, ALFParam* ALFp)
{
  int ind, bit_ct=0, bit_ct0, i, j;
  int filters_per_group_temp, filters_per_group_diff;
  int chosenPred = 0;
#if !ALF_MEM_PATCH 
  static int **diffFilterCoeffQuant, **FilterCoeffQuantTemp, first = 1;
#endif
  int force0 = 1;
  Int64 Newbit_ct;

#if !ALF_MEM_PATCH 
  if(first == 1)
  {
    initMatrix_int(&diffFilterCoeffQuant, NO_VAR_BINS, MAX_SQR_FILT_LENGTH);
    initMatrix_int(&FilterCoeffQuantTemp, NO_VAR_BINS, MAX_SQR_FILT_LENGTH);
    first = 0;
  }
#endif
  i = 0;
  for(ind = 0; ind < filters_per_group; ind++)
  {		
    if(codedVarBins[ind] == 1)
    {
      for(j = 0; j < sqrFiltLength; j++)
        FilterCoeffQuantTemp[i][j]=FilterCoeffQuant[ind][j];
      i++;
    }
  }
  filters_per_group_diff = filters_per_group_temp = i;

  for(ind = 0; ind < filters_per_group; ++ind)
  {		
    if(ind == 0)
    {
      for(i = 0; i < sqrFiltLength; i++)
        diffFilterCoeffQuant[ind][i] = FilterCoeffQuantTemp[ind][i];
    }
    else
    {
      for(i = 0; i < sqrFiltLength; i++)
        diffFilterCoeffQuant[ind][i] = FilterCoeffQuantTemp[ind][i] - FilterCoeffQuantTemp[ind-1][i];
    }
  }

  if(!((filters_per_group_temp == 0) && (filters_per_group == 1)))
  {
    bit_ct0 = xcodeFilterCoeff(FilterCoeffQuantTemp, fl, sqrFiltLength, filters_per_group_temp, 0);

    if(xcodeFilterCoeff(diffFilterCoeffQuant, fl, sqrFiltLength, filters_per_group_diff, 0) >= bit_ct0)
    {
      chosenPred = 0;
      bit_ct += lengthPredFlags(force0, chosenPred, codedVarBins, filters_per_group, createBistream);
      bit_ct += xcodeFilterCoeff(FilterCoeffQuantTemp, fl, sqrFiltLength, filters_per_group_temp, createBistream);
    }
    else
    {
      chosenPred = 1;
      bit_ct += lengthPredFlags(force0, chosenPred, codedVarBins, filters_per_group, createBistream);
      bit_ct += xcodeFilterCoeff(diffFilterCoeffQuant, fl, sqrFiltLength, filters_per_group_temp, createBistream);
    }
  }
  ALFp->forceCoeff0 = 1;
  ALFp->predMethod = chosenPred;
  ALFp->filters_per_group_diff = filters_per_group_diff;
  ALFp->filters_per_group = filters_per_group;
  ALFp->num_coeff = sqrFiltLength;
  if (ALFp->num_coeff == 8) ALFp->realfiltNo=2;
  else if (ALFp->num_coeff == 14) ALFp->realfiltNo=1;
  else ALFp->realfiltNo=0;
  
  for(ind = 0; ind < filters_per_group; ++ind)
  {		
    ALFp->codedVarBins[ind] = codedVarBins[ind];
  }
  for(ind = 0; ind < filters_per_group_diff; ++ind)
  {		
	for(i = 0; i < sqrFiltLength; i++)
	{
      if (chosenPred) ALFp->coeffmulti[ind][i] = diffFilterCoeffQuant[ind][i];
	  else ALFp->coeffmulti[ind][i] = FilterCoeffQuantTemp[ind][i];
	}
  }
  m_pcDummyEntropyCoder->codeFiltCountBit(ALFp, &Newbit_ct);

  return ((Int)Newbit_ct);
}

//filtNo==-1/realfiltNo, noFilters=filters_per_frames, realfiltNo=filtNo
Int TEncAdaptiveLoopFilter::xcodeAuxInfo(int filtNo, int noFilters, int varIndTab[NO_VAR_BINS], int frNo, int createBitstream,int realfiltNo, ALFParam* ALFp)
{
  int i, filterPattern[NO_VAR_BINS], startSecondFilter=0, bitCt=0, codePrediction;
  Int64 NewbitCt;

  codePrediction = 0;

  //send realfiltNo (tap related)
  ALFp->realfiltNo = realfiltNo;
  ALFp->filtNo = filtNo;
  
  if(filtNo >= 0)
  {
    // decide startSecondFilter and filterPattern
	if(noFilters > 1)
    {
      memset(filterPattern, 0, NO_VAR_BINS * sizeof(int)); 
      for(i = 1; i < NO_VAR_BINS; ++i)
	  {
        if(varIndTab[i] != varIndTab[i-1])
        {
          filterPattern[i] = 1;
          startSecondFilter = i;
        }
	  }
	  memcpy (ALFp->filterPattern, filterPattern, NO_VAR_BINS * sizeof(int));
	  ALFp->startSecondFilter = startSecondFilter;
    }

	//send noFilters (filters_per_frame)
	//0: filters_per_frame = 1
    //1: filters_per_frame = 2
    //2: filters_per_frame > 2 (exact number from filterPattern)

    ALFp->noFilters = min(noFilters-1,2);
	if (noFilters<=0) printf("error\n");
  }
  m_pcDummyEntropyCoder->codeAuxCountBit(ALFp, &NewbitCt);
  bitCt = (int) NewbitCt;
  return(bitCt);
}

Int   TEncAdaptiveLoopFilter::xcodeFilterCoeff(int **pDiffQFilterCoeffIntPP, int fl, int sqrFiltLength, 
                    int filters_per_group, int createBitstream)
{
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
      coeffVal=abs(pDiffQFilterCoeffIntPP[ind][i]);
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
//  len += lengthFilterCodingParams(minKStart, maxScanVal, kMinTab, createBitstream);
  len += (3 + maxScanVal);

  // Filter coefficients
  len += lengthFilterCoeffs(sqrFiltLength, filters_per_group, pDepthInt, pDiffQFilterCoeffIntPP, 
    kMinTab, createBitstream);

  return len;
}

Int TEncAdaptiveLoopFilter::lengthGolomb(int coeffVal, int k)
{
  int m = 2 << (k - 1);
  int q = coeffVal / m;
  if(coeffVal != 0)
    return(q + 2 + k);
  else
    return(q + 1 + k);
}

Int TEncAdaptiveLoopFilter::lengthPredFlags(int force0, int predMethod, int codedVarBins[NO_VAR_BINS], 
                   int filters_per_group, int createBitstream)
{
  int bit_cnt = 0;

  if(force0)
    bit_cnt = 2 + filters_per_group;
  else
    bit_cnt = 2;
  return bit_cnt;
  
}
//important
Int TEncAdaptiveLoopFilter::lengthFilterCoeffs(int sqrFiltLength, int filters_per_group, int pDepthInt[], 
                      int **FilterCoeff, int kMinTab[], int createBitstream)
{
  int ind, scanPos, i;
  int bit_cnt = 0;

  for(ind = 0; ind < filters_per_group; ++ind)
  {
    for(i = 0; i < sqrFiltLength; i++)
    {	
      scanPos = pDepthInt[i] - 1;
      bit_cnt += lengthGolomb(abs(FilterCoeff[ind][i]), kMinTab[scanPos]);
    }
  }
  return bit_cnt;
}

Void   TEncAdaptiveLoopFilter::xEncALFLuma_qc ( TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiMinRate, UInt64& ruiMinDist, Double& rdMinCost )
{
//pcPicDec: extended decoded
//pcPicRest: original decoded: filtered signal will be stored

  UInt64  uiRate;
  UInt64  uiDist;
  Double dCost;
  Int    Height = pcPicOrg->getHeight();
  Int    Width = pcPicOrg->getWidth();
  Int    LumaStride = pcPicOrg->getStride();
#if ALF_MEM_PATCH
  imgpel* pOrg = (imgpel*) pcPicOrg->getLumaAddr();
  imgpel* pRest = (imgpel*) pcPicRest->getLumaAddr();
  imgpel* pDec = (imgpel*) pcPicDec->getLumaAddr();
#else
  Pel* pOrg = pcPicOrg->getLumaAddr();
  Pel* pRest = pcPicRest->getLumaAddr();
#endif

  Int tap               = ALF_MIN_NUM_TAP;
  m_pcTempAlfParam->tap = tap;
  m_pcTempAlfParam->num_coeff = (Int)tap*tap/4 + 2; 

#if ALF_MEM_PATCH
  for (Int i=0; i<Height; i++)
	for (Int j=0; j<Width; j++)
	{
	  maskImg[i][j] = 1;
	}
  calcVar(varImg, pDec, 9/2, VAR_SIZE, Height, Width, LumaStride);
  xFirstFilteringFrameLuma(pOrg, pDec, pRest, m_pcTempAlfParam, m_pcTempAlfParam->tap, LumaStride); 
#else
  //move pcPicOrg and pcPicDec and pcPicRest to imgY_org and imgY_ext and imgY_rest 
  for (Int i=0; i<Height; i++)
	for (Int j=0; j<Width; j++)
	{
	  imgY_org[i][j]=pOrg[j + i*LumaStride];
	  imgY_rest[i][j]=pRest[j + i*LumaStride];
	  maskImg[i][j] = 1;
	}
  padImage(imgY_rest, imgY_ext, 4, Height, Width);

  calcVar(varImg, imgY_ext, 9/2, VAR_SIZE, Height, Width);

  xFirstFilteringFrameLuma(imgY_org, imgY_ext, imgY_rest, m_pcTempAlfParam, m_pcTempAlfParam->tap); 

  for (Int i=0; i<Height; i++)
	for (Int j=0; j<Width; j++)
	{
	  pRest[j + i*LumaStride]=imgY_rest[i][j];
	}
#endif

  xCalcRDCost(pcPicOrg, pcPicRest, m_pcTempAlfParam, uiRate, uiDist, dCost); // change this function final coding 

  if( dCost < rdMinCost)
  {
    ruiMinRate = uiRate;
    ruiMinDist = uiDist;
    rdMinCost = dCost;
    copyALFParam(m_pcBestAlfParam, m_pcTempAlfParam); 
  }  
}
#if ALF_MEM_PATCH
Void   TEncAdaptiveLoopFilter::xFirstFilteringFrameLuma(imgpel* ImgOrg, imgpel* ImgDec, imgpel* ImgRest, ALFParam* ALFp, Int tap, Int Stride)
{
  xstoreInBlockMatrix(ImgOrg, ImgDec, tap, Stride);
  xFilteringFrameLuma_qc(ImgOrg, ImgDec, ImgRest, ALFp, tap, Stride);
}
Void   TEncAdaptiveLoopFilter::xstoreInBlockMatrix(imgpel* ImgOrg, imgpel* ImgDec, Int tap, Int Stride)
{
  Int i,j,k,l,varInd,ii,jj;
  Int x, y;
  Int fl =tap/2;
  Int sqrFiltLength=(((tap*tap)/4 + 1) + 1);
  Int fl2=9/2; //extended size at each side of the frame
  Int ELocal[MAX_SQR_FILT_LENGTH];
  Int yLocal;
  Int *p_pattern;
  Int filtNo =2; 
  double **E,*yy;
  Int count_valid=0;
  if (tap==9)
		filtNo =0;
  else if (tap==7)
		filtNo =1;

  p_pattern= patternTab[filtNo];

  memset( pixAcc, 0,sizeof(double)*NO_VAR_BINS);
  for (varInd=0; varInd<NO_VAR_BINS; varInd++)
  {
	memset(yGlobalSym[filtNo][varInd],0,sizeof(double)*MAX_SQR_FILT_LENGTH);
    for (k=0; k<sqrFiltLength; k++)
	{
	  memset(EGlobalSym[filtNo][varInd][k],0,sizeof(double)*MAX_SQR_FILT_LENGTH);
    }
  }
  for (i = fl2; i < im_height+fl2; i++)
  {
	for (j = fl2; j < im_width+fl2; j++)
	{	  
	  if (maskImg[i-fl2][j-fl2] == 1)
	  {
		count_valid++;
	  }
	}
  }

  if (1)
  {
    Int j;
	for (i=0,y=fl2; i<im_height; i++,y++)
	{
	  for (j=0,x=fl2; j<im_width; j++,x++)
	  {
		if (maskImg[i][j] == 0 && count_valid > 0)
		{

		}
		else
		{
		  varInd=min(varImg[i][j], NO_VAR_BINS-1);
		  k=0; 
		  memset(ELocal, 0, sqrFiltLength*sizeof(int));
		  for (ii=-fl; ii<0; ii++)
		  {
			for (jj=-fl-ii; jj<=fl+ii; jj++)
			{  
			  ELocal[p_pattern[k++]]+=(ImgDec[(i+ii)*Stride + (j+jj)]+ImgDec[(i-ii)*Stride + (j-jj)]);
			}
		  }
		  for (jj=-fl; jj<0; jj++)
			  ELocal[p_pattern[k++]]+=(ImgDec[(i)*Stride + (j+jj)]+ImgDec[(i)*Stride + (j-jj)]);
		  ELocal[p_pattern[k++]]+=ImgDec[(i)*Stride + (j)];
			ELocal[sqrFiltLength-1]=1;
		  yLocal=ImgOrg[(i)*Stride + (j)];

		  pixAcc[varInd]+=(yLocal*yLocal);
			E= EGlobalSym[filtNo][varInd];
			yy= yGlobalSym[filtNo][varInd];

		  for (k=0; k<sqrFiltLength; k++)
		  {
			  for (l=k; l<sqrFiltLength; l++)
				E[k][l]+=(double)(ELocal[k]*ELocal[l]);
			  yy[k]+=(double)(ELocal[k]*yLocal);
		  }
		}
	  }
	}
  }
  // Matrix EGlobalSeq is symmetric, only part of it is calculated
  for (varInd=0; varInd<NO_VAR_BINS; varInd++)
  {
	double **pE = EGlobalSym[filtNo][varInd];
    for (k=1; k<sqrFiltLength; k++)
	{
      for (l=0; l<k; l++)
	  {
        pE[k][l]=pE[l][k];
      }
    }
  }
}

Void   TEncAdaptiveLoopFilter::xFilteringFrameLuma_qc(imgpel* ImgOrg, imgpel* imgY_pad, imgpel* ImgFilt, ALFParam* ALFp, Int tap, Int Stride)
{
	int  filtNo,filters_per_fr;
 	static double **ySym, ***ESym;
    int lambda_val = (Int) m_dLambdaLuma;
    lambda_val = lambda_val * (1<<(2*g_uiBitIncrement));
	if (tap==9)
		filtNo =0;
	else if (tap==7)
		filtNo =1;
	else
		filtNo=2;
   
	ESym=EGlobalSym[filtNo];  
	ySym=yGlobalSym[filtNo];

	xfindBestFilterVarPred(ySym, ESym, pixAcc, filterCoeffSym, g_filterCoeffSymQuant, filtNo, &filters_per_fr,
						varIndTab, NULL, varImg, maskImg, NULL, lambda_val);

	// g_filterCoeffPrevSelected = g_filterCoeffSym
    xcalcPredFilterCoeff(filtNo);

	 //filter the frame with g_filterCoeffPrevSelected
	xfilterFrame_en(imgY_pad, ImgFilt, filtNo, Stride);

	xcodeFiltCoeff(g_filterCoeffSymQuant, filtNo, varIndTab, filters_per_fr,0, ALFp);
}

Void TEncAdaptiveLoopFilter::xfilterFrame_en(imgpel* ImgDec, imgpel* ImgRest,int filtNo, int Stride)
{
  int i,j,ii,jj,y,x;
  int  *pattern; 
  int fl, fl_temp, sqrFiltLength;
  int pixelInt;
  int offset = (1<<(NUM_BITS - 2));

  pattern=patternTab_filt[filtNo];
  fl_temp=flTab[filtNo];
  sqrFiltLength=MAX_SQR_FILT_LENGTH;  fl=FILTER_LENGTH/2;

  for (y=0, i = fl; i < im_height+fl; i++, y++)
  {
    for (x=0, j = fl; j < im_width+fl; j++, x++)
	{
		int varInd=varImg[i-fl][j-fl];
		imgpel *im1,*im2;
		int *coef = filterCoeffPrevSelected[varInd];
		pattern=patternTab_filt[filtNo];
		pixelInt= filterCoeffPrevSelected[varInd][sqrFiltLength-1]; 
		for (ii=-fl_temp; ii<0; ii++)
		{
		  im1= &(ImgDec[(y+ii)*Stride + x-fl_temp-ii]);
		  im2= &(ImgDec[(y-ii)*Stride + x+fl_temp+ii]);
		  for (jj=-fl_temp-ii; jj<=fl_temp+ii; jj++,im1++,im2--)
			  pixelInt+=((*im1+ *im2)*coef[*(pattern++)]);
		}
		im1= &(ImgDec[y*Stride + x-fl_temp]);
		im2= &(ImgDec[y*Stride + x+fl_temp]);	
		for (jj=-fl_temp; jj<0; jj++,im1++,im2--)
			pixelInt+=((*im1+ *im2)*coef[*(pattern++)]);
		pixelInt+=(ImgDec[y*Stride + x]*coef[*(pattern++)]);

		pixelInt=(int)((pixelInt+offset) >> (NUM_BITS - 1));
		ImgRest[y*Stride + x] = Clip3(0, g_uiIBDI_MAX, pixelInt);
    }
  }
}

#endif

Void   TEncAdaptiveLoopFilter::xFirstFilteringFrameLuma(imgpel** ImgOrg, imgpel** ImgDec, imgpel** ImgRest, ALFParam* ALFp, Int tap)
{
  xstoreInBlockMatrix(ImgOrg, ImgDec, tap);
  xFilteringFrameLuma_qc(ImgOrg, ImgDec, ImgRest, ALFp, tap);
}

Void   TEncAdaptiveLoopFilter::xFilteringFrameLuma_qc(imgpel** ImgOrg, imgpel** imgY_pad, imgpel** ImgFilt, ALFParam* ALFp, Int tap)
{
	int  filtNo,filters_per_fr;
 	static double **ySym, ***ESym;
    int lambda_val = (Int) m_dLambdaLuma;
    lambda_val = lambda_val * (1<<(2*g_uiBitIncrement));
	if (tap==9)
		filtNo =0;
	else if (tap==7)
		filtNo =1;
	else
		filtNo=2;
   
	ESym=EGlobalSym[filtNo];  
	ySym=yGlobalSym[filtNo];

#if ALF_MEM_PATCH
	xfindBestFilterVarPred(ySym, ESym, pixAcc, filterCoeffSym, g_filterCoeffSymQuant, filtNo, &filters_per_fr,
						varIndTab, NULL, varImg, maskImg, NULL, lambda_val);
#else
	xfindBestFilterVarPred(ySym, ESym, pixAcc, g_filterCoeffSym, g_filterCoeffSymQuant, filtNo, &filters_per_fr,
						varIndTab, NULL, varImg, maskImg, NULL, lambda_val);
#endif
	// g_filterCoeffPrevSelected = g_filterCoeffSym
    xcalcPredFilterCoeff(filtNo);

	 //filter the frame with g_filterCoeffPrevSelected
	xfilterFrame_en(imgY_pad, ImgFilt, filtNo);

	xcodeFiltCoeff(g_filterCoeffSymQuant, filtNo, varIndTab, filters_per_fr,0, ALFp);
}

Void   TEncAdaptiveLoopFilter::xstoreInBlockMatrix(imgpel** ImgOrg, imgpel** ImgDec, Int tap)
{
  Int i,j,k,l,varInd,ii,jj;
  Int x, y;
  Int fl =tap/2;
  Int sqrFiltLength=(((tap*tap)/4 + 1) + 1);
  Int fl2=9/2; //extended size at each side of the frame
  Int ELocal[MAX_SQR_FILT_LENGTH];
  Int yLocal;
  Int *p_pattern;
  Int filtNo =2; 
  double **E,*yy;
  Int count_valid=0;
  if (tap==9)
		filtNo =0;
  else if (tap==7)
		filtNo =1;

  p_pattern= patternTab[filtNo];

  memset( pixAcc, 0,sizeof(double)*NO_VAR_BINS);
  for (varInd=0; varInd<NO_VAR_BINS; varInd++)
  {
	memset(yGlobalSym[filtNo][varInd],0,sizeof(double)*MAX_SQR_FILT_LENGTH);
    for (k=0; k<sqrFiltLength; k++)
	{
	  memset(EGlobalSym[filtNo][varInd][k],0,sizeof(double)*MAX_SQR_FILT_LENGTH);
    }
  }
  for (i = fl2; i < im_height+fl2; i++)
  {
	for (j = fl2; j < im_width+fl2; j++)
	{	  
	  if (maskImg[i-fl2][j-fl2] == 1)
	  {
		count_valid++;
	  }
	}
  }

  if (1)
  {
    Int j;
	for (i=0,y=fl2; i<im_height; i++,y++)
	{
	  for (j=0,x=fl2; j<im_width; j++,x++)
	  {
		if (maskImg[i][j] == 0 && count_valid > 0)
		{

		}
		else
		{
		  varInd=min(varImg[i][j], NO_VAR_BINS-1);
		  k=0; 
		  memset(ELocal, 0, sqrFiltLength*sizeof(int));
		  for (ii=-fl; ii<0; ii++)
		  {
			for (jj=-fl-ii; jj<=fl+ii; jj++)
			{  
			  ELocal[p_pattern[k++]]+=(ImgDec[y+ii][x+jj]+ImgDec[y-ii][x-jj]);
			}
		  }
		  for (jj=-fl; jj<0; jj++)
			  ELocal[p_pattern[k++]]+=(ImgDec[y][x+jj]+ImgDec[y][x-jj]);
		  ELocal[p_pattern[k++]]+=ImgDec[y][x];
			ELocal[sqrFiltLength-1]=1;
		  yLocal=ImgOrg[i][j];

		  pixAcc[varInd]+=(yLocal*yLocal);
			E= EGlobalSym[filtNo][varInd];
			yy= yGlobalSym[filtNo][varInd];

		  for (k=0; k<sqrFiltLength; k++)
		  {
			  for (l=k; l<sqrFiltLength; l++)
				E[k][l]+=(double)(ELocal[k]*ELocal[l]);
			  yy[k]+=(double)(ELocal[k]*yLocal);
		  }
		}
	  }
	}
  }
  // Matrix EGlobalSeq is symmetric, only part of it is calculated
  for (varInd=0; varInd<NO_VAR_BINS; varInd++)
  {
	double **pE = EGlobalSym[filtNo][varInd];
    for (k=1; k<sqrFiltLength; k++)
	{
      for (l=0; l<k; l++)
	  {
        pE[k][l]=pE[l][k];
      }
    }
  }
}

Void TEncAdaptiveLoopFilter::xfindBestFilterVarPred(double **ySym, double ***ESym, double *pixAcc, int **filterCoeffSym, int **filterCoeffSymQuant, int filtNo, int *filters_per_fr_best, int varIndTab[], imgpel **imgY_rec, imgpel **varImg, imgpel **maskImg, imgpel **imgY_pad, double lambda_val)
{
  int filters_per_fr, firstFilt, coded, forceCoeff0,
    interval[NO_VAR_BINS][2], intervalBest[NO_VAR_BINS][2];
  int i, k, varInd;
  static double ***E_temp, **y_temp, *pixAcc_temp;
  static int **FilterCoeffQuantTemp;
  double  error, lambda, lagrangian, lagrangianMin;

  int fl, sqrFiltLength;
  int *pattern, *patternMap, *weights;
  int numBits, coeffBits;
  double errorForce0CoeffTab[NO_VAR_BINS][2];
  int  codedVarBins[NO_VAR_BINS], createBistream /*, forceCoeff0 */;
  int  usePrevFilt[NO_VAR_BINS], usePrevFiltDefault[NO_VAR_BINS];
  static int first=0;

  for (i = 0; i < NO_VAR_BINS; i++)
    usePrevFiltDefault[i]=usePrevFilt[i]=1;
  lambda = lambda_val;
  sqrFiltLength=MAX_SQR_FILT_LENGTH;  fl=FILTER_LENGTH/2;

  if (first==0)
  {
    initMatrix3D_double(&E_temp, NO_VAR_BINS, MAX_SQR_FILT_LENGTH, MAX_SQR_FILT_LENGTH);
    initMatrix_double(&y_temp, NO_VAR_BINS, MAX_SQR_FILT_LENGTH); 
    pixAcc_temp = (double *) calloc(NO_VAR_BINS, sizeof(double));
	initMatrix_int(&FilterCoeffQuantTemp, NO_VAR_BINS, MAX_SQR_FILT_LENGTH);
    first=1;
  }

  sqrFiltLength=sqrFiltLengthTab[filtNo];   
  fl=flTab[filtNo];
  weights=weightsTab[filtNo];               
  patternMap=patternMapTab[filtNo];  
  pattern=patternTab[filtNo];

  memcpy(pixAcc_temp,pixAcc,sizeof(double)*NO_VAR_BINS);
  for (varInd=0; varInd<NO_VAR_BINS; varInd++)
  {
     memcpy(y_temp[varInd],ySym[varInd],sizeof(double)*sqrFiltLength);
     for (k=0; k<sqrFiltLength; k++)
		memcpy(E_temp[varInd][k],ESym[varInd][k],sizeof(double)*sqrFiltLength);
  }
 
  // zero all variables 
  memset(varIndTab,0,sizeof(int)*NO_VAR_BINS);

  for(i = 0; i < NO_VAR_BINS; i++)
  {
	memset(filterCoeffSym[i],0,sizeof(int)*MAX_SQR_FILT_LENGTH);
	memset(filterCoeffSymQuant[i],0,sizeof(int)*MAX_SQR_FILT_LENGTH);
  }

  firstFilt=1;  lagrangianMin=0;
  filters_per_fr=NO_FILTERS;
  
  while(filters_per_fr>=1)
  {
	findFilterGroupingError(E_temp, y_temp, pixAcc_temp, interval, sqrFiltLength, filters_per_fr);
	findFilterCoeff(E_temp, y_temp, pixAcc_temp, filterCoeffSym, filterCoeffSymQuant, interval,
	  varIndTab, sqrFiltLength, filters_per_fr, weights, numBits=NUM_BITS,  errorForce0CoeffTab);
	lagrangian=xfindBestCoeffCodMethod(codedVarBins, &forceCoeff0, filterCoeffSymQuant, fl, 
	  sqrFiltLength, filters_per_fr, errorForce0CoeffTab, &error, lambda);

	if (lagrangian<lagrangianMin || firstFilt==1)
	{
	  firstFilt=0;
	  lagrangianMin=lagrangian;

	  (*filters_per_fr_best)=filters_per_fr;
	  memcpy(intervalBest, interval, NO_VAR_BINS*2*sizeof(int));
	}
	filters_per_fr--;
  }

  findFilterCoeff(E_temp, y_temp, pixAcc_temp, filterCoeffSym, filterCoeffSymQuant, intervalBest,
	varIndTab, sqrFiltLength, (*filters_per_fr_best), weights, numBits=NUM_BITS, errorForce0CoeffTab);

  xfindBestCoeffCodMethod(codedVarBins, &forceCoeff0, filterCoeffSymQuant, fl, sqrFiltLength, 
	(*filters_per_fr_best), errorForce0CoeffTab, &error, lambda);

  coded=1;
  if (forceCoeff0==1 && (*filters_per_fr_best)==1)
  {
	coded=0;
	coeffBits = xcodeAuxInfo(-1, (*filters_per_fr_best), varIndTab, 0, createBistream=0,filtNo, tempALFp);
  }
  else
  {
	coeffBits = xcodeAuxInfo(filtNo, (*filters_per_fr_best), varIndTab, 0, createBistream=0,filtNo, tempALFp);
  }

  if (forceCoeff0==0)
  {
	coeffBits += xsendAllFiltersPPPred(filterCoeffSymQuant, fl, sqrFiltLength, 
	  (*filters_per_fr_best), createBistream=0, tempALFp);
  }
  else
  {
	if ((*filters_per_fr_best)==1)
	{
	  for(varInd=0; varInd<(*filters_per_fr_best); varInd++)
	  {
		memset(filterCoeffSym[varInd],0,sizeof(int)*MAX_SQR_FILT_LENGTH);
		memset(filterCoeffSymQuant[varInd],0,sizeof(int)*MAX_SQR_FILT_LENGTH);
	  }
	}
	else
	{
	  coeffBits += xsendAllFiltersPPPredForce0(filterCoeffSymQuant, fl, sqrFiltLength, 
		(*filters_per_fr_best), codedVarBins, createBistream=0, tempALFp);

	  for(varInd=0; varInd<(*filters_per_fr_best); varInd++)
	  {
		if (codedVarBins[varInd]==0)
		{
			memset(filterCoeffSym[varInd],0,sizeof(int)*MAX_SQR_FILT_LENGTH);
			memset(filterCoeffSymQuant[varInd],0,sizeof(int)*MAX_SQR_FILT_LENGTH);
		}
	  }
	}
  }
  
}

Void TEncAdaptiveLoopFilter::xfilterFrame_en(imgpel** ImgDec, imgpel** ImgRest,int filtNo)
{
  int i,j,ii,jj;
  int  *pattern; 
  int fl, fl_temp, sqrFiltLength;
  int pixelInt;
  int offset = (1<<(NUM_BITS - 2));

  pattern=patternTab_filt[filtNo];
  fl_temp=flTab[filtNo];
  sqrFiltLength=MAX_SQR_FILT_LENGTH;  fl=FILTER_LENGTH/2;

  for (i = fl; i < im_height+fl; i++)
  {
    for (j = fl; j < im_width+fl; j++)
	{
		int varInd=varImg[i-fl][j-fl];
		imgpel *im1,*im2;
#if ALF_MEM_PATCH
		int *coef = filterCoeffPrevSelected[varInd];
		pattern=patternTab_filt[filtNo];
		pixelInt= filterCoeffPrevSelected[varInd][sqrFiltLength-1]; 
#else
		int *coef = g_filterCoeffPrevSelected[varInd];

		pattern=patternTab_filt[filtNo];
		pixelInt=g_filterCoeffPrevSelected[varInd][sqrFiltLength-1]; 
#endif
		for (ii=-fl_temp; ii<0; ii++)
		{
		  im1= &(ImgDec[i+ii][j-fl_temp-ii]);
		  im2= &(ImgDec[i-ii][j+fl_temp+ii]);
		  for (jj=-fl_temp-ii; jj<=fl_temp+ii; jj++,im1++,im2--)
			  pixelInt+=((*im1+ *im2)*coef[*(pattern++)]);
		}
		im1= &(ImgDec[i][j-fl_temp]);
		im2= &(ImgDec[i][j+fl_temp]);	
		for (jj=-fl_temp; jj<0; jj++,im1++,im2--)
			pixelInt+=((*im1+ *im2)*coef[*(pattern++)]);
		pixelInt+=(ImgDec[i][j]*coef[*(pattern++)]);

		pixelInt=(int)((pixelInt+offset) >> (NUM_BITS - 1));
//		ImgRest[i-fl][j-fl] = Clip3(0, (1 << (g_uiBitDepth+g_uiBitIncrement))-1, pixelInt);
		ImgRest[i-fl][j-fl] = Clip3(0, g_uiIBDI_MAX, pixelInt);
    }
  }
}


Void TEncAdaptiveLoopFilter::xcalcPredFilterCoeff(int filtNo)
{
  int *patternMap, varInd, i, k;

  patternMap=patternMapTab[filtNo];
  for(varInd=0; varInd<NO_VAR_BINS; ++varInd)
  {		
	k=0;
	for(i = 0; i < MAX_SQR_FILT_LENGTH; i++)
	{
	  if (patternMap[i]>0)
	  {
#if ALF_MEM_PATCH
		filterCoeffPrevSelected[varInd][i]=filterCoeffSym[varIndTab[varInd]][k];
#else
		g_filterCoeffPrevSelected[varInd][i]=g_filterCoeffSym[varIndTab[varInd]][k];
#endif
		k++;
	  }
	  else
	  {
#if ALF_MEM_PATCH
		  filterCoeffPrevSelected[varInd][i]=0;
#else
		  g_filterCoeffPrevSelected[varInd][i]=0;
#endif
	  }
	}
  }
}

Void TEncAdaptiveLoopFilter::xcodeFiltCoeff(int **filterCoeffSymQuant, int filtNo, int varIndTab[], int filters_per_fr_best, int frNo, ALFParam* ALFp)
{
  int varInd, i, forceCoeff0, codedVarBins[NO_VAR_BINS], coeffBits, createBistream,   sqrFiltLength=sqrFiltLengthTab[filtNo], 
    fl=flTab[filtNo], coded;
  
  ALFp->filters_per_group_diff = filters_per_fr_best;
  ALFp->filters_per_group = filters_per_fr_best;

  for(varInd=0; varInd<filters_per_fr_best; varInd++)
  {
    codedVarBins[varInd]=0;
    for(i = 0; i < MAX_SQR_FILT_LENGTH; i++)
	{
      if (filterCoeffSymQuant[varInd][i] != 0)
	  {
        codedVarBins[varInd]=1;
        break;
      }
    }
  }
  memcpy (ALFp->codedVarBins, codedVarBins, sizeof(int)*NO_VAR_BINS);
  forceCoeff0=0;
  for(varInd=0; varInd<filters_per_fr_best; varInd++)
  {
    if (codedVarBins[varInd] == 0)
	{
      forceCoeff0=1;
      break;
    }
  }

  coded=1;
  if (forceCoeff0==1 && filters_per_fr_best==1)
  {
    coded=0;
    coeffBits = xcodeAuxInfo(-1, filters_per_fr_best, varIndTab, frNo, createBistream=1,filtNo, ALFp);
  }
  else
  {
    coeffBits = xcodeAuxInfo(filtNo, filters_per_fr_best, varIndTab, frNo, createBistream=1,filtNo, ALFp);
  }
  
  ALFp->forceCoeff0 = forceCoeff0;
  ALFp->predMethod = 0;
  ALFp->num_coeff = sqrFiltLength;
  ALFp->realfiltNo=filtNo;
  if (filters_per_fr_best <= 1)
  {
	ALFp->forceCoeff0 = 0;
	ALFp->predMethod = 0;
  }

  if (forceCoeff0==0) 
  {
    coeffBits += xsendAllFiltersPPPred(filterCoeffSymQuant, fl, sqrFiltLength, 
      filters_per_fr_best, createBistream=1, ALFp);
  }
  else if (filters_per_fr_best>1)
  {
    coeffBits += xsendAllFiltersPPPredForce0(filterCoeffSymQuant, fl, sqrFiltLength, 
      filters_per_fr_best, codedVarBins, createBistream=1, ALFp);
  }
}



#if TSB_ALF_HEADER
Void TEncAdaptiveLoopFilter::xSetCUAlfCtrlFlags_qc(UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist, ALFParam *pAlfParam)
#else
Void TEncAdaptiveLoopFilter::xSetCUAlfCtrlFlags_qc(UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist)
#endif
{
  ruiDist = 0;
#if TSB_ALF_HEADER
  pAlfParam->num_alf_cu_flag = 0;
#endif

  for( UInt uiCUAddr = 0; uiCUAddr < m_pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = m_pcPic->getCU( uiCUAddr );
#if TSB_ALF_HEADER
    xSetCUAlfCtrlFlag_qc(pcCU, 0, 0, uiAlfCtrlDepth, pcPicOrg, pcPicDec, pcPicRest, ruiDist, pAlfParam);
#else
    xSetCUAlfCtrlFlag_qc(pcCU, 0, 0, uiAlfCtrlDepth, pcPicOrg, pcPicDec, pcPicRest, ruiDist);
#endif
  }
}

#if TSB_ALF_HEADER
Void TEncAdaptiveLoopFilter::xSetCUAlfCtrlFlag_qc(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist, ALFParam *pAlfParam)
#else
Void TEncAdaptiveLoopFilter::xSetCUAlfCtrlFlag_qc(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist)
#endif
{
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;

  if( ( uiRPelX >= pcCU->getSlice()->getSPS()->getWidth() ) || ( uiBPelY >= pcCU->getSlice()->getSPS()->getHeight() ) )
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < (g_uiMaxCUDepth-g_uiAddCUDepth) ) ) || bBoundary )
  {
    UInt uiQNumParts = ( m_pcPic->getNumPartInCU() >> (uiDepth<<1) )>>2;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
#if TSB_ALF_HEADER
        xSetCUAlfCtrlFlag_qc(pcCU, uiAbsPartIdx, uiDepth+1, uiAlfCtrlDepth, pcPicOrg, pcPicDec, pcPicRest, ruiDist, pAlfParam);
#else
        xSetCUAlfCtrlFlag_qc(pcCU, uiAbsPartIdx, uiDepth+1, uiAlfCtrlDepth, pcPicOrg, pcPicDec, pcPicRest, ruiDist);
#endif
    }
    return;
  }

  if( uiDepth > uiAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, uiAlfCtrlDepth))
  {
    return;
  }

  UInt uiCUAddr = pcCU->getAddr();
  UInt64 uiRecSSD = 0;
  UInt64 uiFiltSSD = 0;

  Int iWidth;
  Int iHeight;
  UInt uiSetDepth;

  if (uiDepth > uiAlfCtrlDepth && pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, uiAlfCtrlDepth))
  {
    iWidth = g_uiMaxCUWidth >> uiAlfCtrlDepth;
    iHeight = g_uiMaxCUHeight >> uiAlfCtrlDepth;

    uiRPelX   = uiLPelX + iWidth  - 1;
    uiBPelY   = uiTPelY + iHeight - 1;

    if( uiRPelX >= pcCU->getSlice()->getSPS()->getWidth() )
    {
      iWidth = pcCU->getSlice()->getSPS()->getWidth() - uiLPelX;
    }

    if( uiBPelY >= pcCU->getSlice()->getSPS()->getHeight() )
    {
      iHeight = pcCU->getSlice()->getSPS()->getHeight() - uiTPelY;
    }

    uiSetDepth = uiAlfCtrlDepth;
  }
  else
  {
    iWidth = pcCU->getWidth(uiAbsPartIdx);
    iHeight = pcCU->getHeight(uiAbsPartIdx);
    uiSetDepth = uiDepth;
  }

  Pel* pOrg = pcPicOrg->getLumaAddr(uiCUAddr, uiAbsPartIdx);
  Pel* pRec = pcPicDec->getLumaAddr(uiCUAddr, uiAbsPartIdx);
  Pel* pFilt = pcPicRest->getLumaAddr(uiCUAddr, uiAbsPartIdx);

  uiRecSSD  += xCalcSSD( pOrg, pRec,  iWidth, iHeight, pcPicOrg->getStride() );
  uiFiltSSD += xCalcSSD( pOrg, pFilt, iWidth, iHeight, pcPicOrg->getStride() );

  if (uiFiltSSD < uiRecSSD)
  {
    ruiDist += uiFiltSSD;
    pcCU->setAlfCtrlFlagSubParts(1, uiAbsPartIdx, uiSetDepth);
#if TSB_ALF_HEADER
    pAlfParam->alf_cu_flag[pAlfParam->num_alf_cu_flag]=1;
#endif
	for (int i=uiTPelY ;i<=min(uiBPelY,(unsigned int)(pcPicOrg->getHeight()-1))  ;i++)
	  for (int j=uiLPelX ;j<=min(uiRPelX,(unsigned int)(pcPicOrg->getWidth()-1)) ;j++)
	  { 
		maskImg[i][j]=1;
	  }
  }
  else
  {
    ruiDist += uiRecSSD;
    pcCU->setAlfCtrlFlagSubParts(0, uiAbsPartIdx, uiSetDepth);
#if TSB_ALF_HEADER
    pAlfParam->alf_cu_flag[pAlfParam->num_alf_cu_flag]=0;
#endif
	for (int i=uiTPelY ;i<=min(uiBPelY,(unsigned int)(pcPicOrg->getHeight()-1))  ;i++)
	  for (int j=uiLPelX ;j<=min(uiRPelX,(unsigned int)(pcPicOrg->getWidth()-1)) ;j++)
	  { 
		maskImg[i][j]=0;
	  }
  }
#if TSB_ALF_HEADER
  pAlfParam->num_alf_cu_flag++;
#endif
}

Void TEncAdaptiveLoopFilter::xReDesignFilterCoeff_qc(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, Bool bReadCorr)
{

  Int tap = m_pcTempAlfParam->tap;
  Int    LumaStride = pcPicOrg->getStride();
#if ALF_MEM_PATCH
  imgpel* pOrg = (imgpel*)pcPicOrg->getLumaAddr();
  imgpel* pDec = (imgpel*)pcPicDec->getLumaAddr();
  imgpel* pRest = (imgpel*)pcPicRest->getLumaAddr();
  xFirstFilteringFrameLuma(pOrg, pDec, pRest, m_pcTempAlfParam, tap, LumaStride); 
#else
  Int    Height = pcPicOrg->getHeight();
  Int    Width = pcPicOrg->getWidth();
  Pel* pOrg = pcPicOrg->getLumaAddr();
  Pel* pDec = pcPicDec->getLumaAddr();
  Pel* pRest = pcPicRest->getLumaAddr();
  
  for (Int i=0; i<Height; i++)
	for (Int j=0; j<Width; j++)
	{
	  imgY_org[i][j]=pOrg[j + i*LumaStride];
	  imgY_rest[i][j]=pDec[j + i*LumaStride];
	}
  padImage(imgY_rest, imgY_ext, 4, Height, Width);

  xFirstFilteringFrameLuma(imgY_org, imgY_ext, imgY_rest, m_pcTempAlfParam, tap); 
  for (Int i=0; i<Height; i++)
	for (Int j=0; j<Width; j++)
	{
	  pRest[j + i*LumaStride]=imgY_rest[i][j];
	}
#endif

}
Void TEncAdaptiveLoopFilter::xCUAdaptiveControl_qc(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiMinRate, UInt64& ruiMinDist, Double& rdMinCost)
{
  m_pcEntropyCoder->setAlfCtrl(true);

  UInt uiBestDepth = 0;

  ALFParam cFrmAlfParam;
  allocALFParam(&cFrmAlfParam);
  copyALFParam(&cFrmAlfParam, m_pcBestAlfParam);

  for (UInt uiDepth = 0; uiDepth < g_uiMaxCUDepth; uiDepth++)
  {
    m_pcEntropyCoder->setMaxAlfCtrlDepth(uiDepth);
    pcPicRest->copyToPicLuma(m_pcPicYuvTmp);
    copyALFParam(m_pcTempAlfParam, &cFrmAlfParam);
    m_pcTempAlfParam->cu_control_flag = 1;

    for (UInt uiRD = 0; uiRD <= ALF_NUM_OF_REDESIGN; uiRD++)
    {
      if (uiRD)
      {
        // re-design filter coefficients
        xReDesignFilterCoeff_qc(pcPicOrg, pcPicDec, m_pcPicYuvTmp, true); //use filtering of mine
      }

      UInt64 uiRate, uiDist;
      Double dCost;
	  //m_pcPicYuvTmp: filtered signal, pcPicDec: orig reconst
#if TSB_ALF_HEADER
      xSetCUAlfCtrlFlags_qc(uiDepth, pcPicOrg, pcPicDec, m_pcPicYuvTmp, uiDist, m_pcTempAlfParam); //set up varImg here
#else
      xSetCUAlfCtrlFlags_qc(uiDepth, pcPicOrg, pcPicDec, m_pcPicYuvTmp, uiDist); //set up varImg here
#endif

      xCalcRDCost(m_pcTempAlfParam, uiRate, uiDist, dCost);

      if (dCost < rdMinCost)
      {
        uiBestDepth = uiDepth;
        rdMinCost = dCost;
        ruiMinDist = uiDist;
        ruiMinRate = uiRate;
        m_pcPicYuvTmp->copyToPicLuma(m_pcPicYuvBest);
        copyALFParam(m_pcBestAlfParam, m_pcTempAlfParam);
		//save maskImg
        xCopyTmpAlfCtrlFlagsFrom();
      }
    }
  }

  if (m_pcBestAlfParam->cu_control_flag)
  {
    m_pcEntropyCoder->setAlfCtrl(true);
    m_pcEntropyCoder->setMaxAlfCtrlDepth(uiBestDepth);
    xCopyTmpAlfCtrlFlagsTo();
    m_pcPicYuvBest->copyToPicLuma(pcPicRest);//copy m_pcPicYuvBest to pcPicRest
    xCopyDecToRestCUs(pcPicDec, pcPicRest); //pcPicRest = pcPicDec
  }
  else
  {
    m_pcEntropyCoder->setAlfCtrl(false);
    m_pcEntropyCoder->setMaxAlfCtrlDepth(0);
  }
  freeALFParam(&cFrmAlfParam);
}


Void TEncAdaptiveLoopFilter::xFilterTapDecision_qc(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiMinRate, UInt64& ruiMinDist, Double& rdMinCost)
{
  // restriction for non-referenced B-slice
  if (m_eSliceType == B_SLICE && m_iPicNalReferenceIdc == 0)
  {
    return;
  }

  UInt64 uiRate, uiDist;
  Double dCost;

  if (m_pcBestAlfParam->cu_control_flag)
  {
    xCopyTmpAlfCtrlFlagsFrom();
  }

  Bool bChanged = false;
  for (Int iTap = ALF_MIN_NUM_TAP; iTap <= ALF_MAX_NUM_TAP; iTap += 2)
  {
    copyALFParam(m_pcTempAlfParam, m_pcBestAlfParam);
    m_pcTempAlfParam->tap = iTap;
    m_pcTempAlfParam->num_coeff = (Int)(iTap*iTap/4) + 2; 
  
	if (m_pcTempAlfParam->cu_control_flag)
    {
      xReDesignFilterCoeff_qc(pcPicOrg, pcPicDec, m_pcPicYuvTmp, false);
#if TSB_ALF_HEADER
      xSetCUAlfCtrlFlags_qc(m_pcEntropyCoder->getMaxAlfCtrlDepth(), pcPicOrg, pcPicDec, m_pcPicYuvTmp, uiDist, m_pcTempAlfParam);
#else
      xSetCUAlfCtrlFlags_qc(m_pcEntropyCoder->getMaxAlfCtrlDepth(), pcPicOrg, pcPicDec, m_pcPicYuvTmp, uiDist);
#endif
      xCalcRDCost(m_pcTempAlfParam, uiRate, uiDist, dCost);
    }
    
	else
    {
#if ALF_MEM_PATCH
	  Int    Height = pcPicOrg->getHeight();
	  Int    Width = pcPicOrg->getWidth();
	  for (Int i=0; i<Height; i++)
		for (Int j=0; j<Width; j++)
		{
		  maskImg[i][j] = 1;
		}
      xReDesignFilterCoeff_qc(pcPicOrg, pcPicDec, m_pcPicYuvTmp, false);
#else
	  Int    Height = pcPicOrg->getHeight();
	  Int    Width = pcPicOrg->getWidth();
	  Int    LumaStride = pcPicOrg->getStride();
	  Pel* pOrg = pcPicOrg->getLumaAddr();
	  Pel* pDec = pcPicDec->getLumaAddr();
	  Pel* pRest = m_pcPicYuvTmp->getLumaAddr();
	  
	  for (Int i=0; i<Height; i++)
		for (Int j=0; j<Width; j++)
		{
		  imgY_org[i][j]=pOrg[j + i*LumaStride];
		  imgY_rest[i][j]=pDec[j + i*LumaStride];
		  maskImg[i][j] = 1;
		}
      padImage(imgY_rest, imgY_ext, 4, Height, Width);
	  xFirstFilteringFrameLuma(imgY_org, imgY_ext, imgY_rest, m_pcTempAlfParam, m_pcTempAlfParam->tap); 
	  for (Int i=0; i<Height; i++)
		for (Int j=0; j<Width; j++)
		{
//		  pRest[j + i*LumaStride]=Clip3(0, g_uiIBDI_MAX, imgY_rest[i][j]);
		  pRest[j + i*LumaStride]=imgY_rest[i][j];
		}
#endif

	  xCalcRDCost(pcPicOrg, m_pcPicYuvTmp, m_pcTempAlfParam, uiRate, uiDist, dCost);
    }

    if (dCost < rdMinCost)
    {
      rdMinCost = dCost;
      ruiMinDist = uiDist;
      ruiMinRate = uiRate;
      m_pcPicYuvTmp->copyToPicLuma(m_pcPicYuvBest);
      copyALFParam(m_pcBestAlfParam, m_pcTempAlfParam);
      bChanged = true;
      if (m_pcTempAlfParam->cu_control_flag)
      {
        xCopyTmpAlfCtrlFlagsFrom();
      }
    }
  }

  if (m_pcBestAlfParam->cu_control_flag)
  {
    xCopyTmpAlfCtrlFlagsTo();
    if (bChanged)
    {
      m_pcPicYuvBest->copyToPicLuma(pcPicRest);
      xCopyDecToRestCUs(pcPicDec, pcPicRest);
    }
  }
  else if (m_pcBestAlfParam->tap > ALF_MIN_NUM_TAP)
  {
    m_pcPicYuvBest->copyToPicLuma(pcPicRest);
  }

  copyALFParam(m_pcTempAlfParam, m_pcBestAlfParam);
}


#define ROUND(a)  (((a) < 0)? (int)((a) - 0.5) : (int)((a) + 0.5))
#define REG              0.0001
#define REG_SQR          0.0000001

//Find filter coeff related
Int TEncAdaptiveLoopFilter::gnsCholeskyDec(double **inpMatr, double outMatr[MAX_SQR_FILT_LENGTH][MAX_SQR_FILT_LENGTH], int noEq)
{ 
  int 
    i, j, k;     /* Looping Variables */
  double 
    scale;       /* scaling factor for each row */
  double 
    invDiag[MAX_SQR_FILT_LENGTH];  /* Vector of the inverse of diagonal entries of outMatr */


  /*
   *  Cholesky decomposition starts
   */

  for(i = 0; i < noEq; i++)
    for(j = i; j < noEq; j++)
    {
      /* Compute the scaling factor */
      scale=inpMatr[i][j];
      if ( i > 0) for( k = i - 1 ; k >= 0 ; k--)
        scale -= outMatr[k][j] * outMatr[k][i];

      /* Compute i'th row of outMatr */
      if(i==j)
	  {
        if(scale <= REG_SQR ) // if(scale <= 0 )  /* If inpMatr is singular */
		{
			return(0);
        }
        else              /* Normal operation */
          invDiag[i] =  1.0/(outMatr[i][i]=sqrt(scale));
      }
      else
	  {
        outMatr[i][j] = scale*invDiag[i]; /* Upper triangular part          */
        outMatr[j][i] = 0.0;              /* Lower triangular part set to 0 */
      }                    
    }
    return(1); /* Signal that Cholesky factorization is successfully performed */
}


Void TEncAdaptiveLoopFilter::gnsTransposeBacksubstitution(double U[MAX_SQR_FILT_LENGTH][MAX_SQR_FILT_LENGTH], double rhs[], double x[], int order)
{
  int 
    i,j;              /* Looping variables */
  double 
    sum;              /* Holds backsubstitution from already handled rows */

  /* Backsubstitution starts */
  x[0] = rhs[0]/U[0][0];               /* First row of U'                   */
  for (i = 1; i < order; i++){         /* For the rows 1..order-1           */

    for (j = 0, sum = 0.0; j < i; j++) /* Backsubst already solved unknowns */
      sum += x[j]*U[j][i];

    x[i]=(rhs[i] - sum)/U[i][i];       /* i'th component of solution vect.  */
  }
}



Void  TEncAdaptiveLoopFilter::gnsBacksubstitution(double R[MAX_SQR_FILT_LENGTH][MAX_SQR_FILT_LENGTH], double z[MAX_SQR_FILT_LENGTH], int R_size, double A[MAX_SQR_FILT_LENGTH])
{
  int
    i, j;

  double
    sum;

  R_size--;

  A[R_size] = z[R_size] / R[R_size][R_size];

  for (i = R_size-1; i >= 0; i--) {

    for (j = i+1, sum = 0.0; j <= R_size; j++)
      sum += R[i][j] * A[j];

    A[i] = (z[i] - sum) / R[i][i];

  }
}


Int TEncAdaptiveLoopFilter::gnsSolveByChol(double **LHS, double *rhs, double *x, int noEq)
{ 
  double aux[MAX_SQR_FILT_LENGTH];     /* Auxiliary vector */
  double U[MAX_SQR_FILT_LENGTH][MAX_SQR_FILT_LENGTH];    /* Upper triangular Cholesky factor of LHS */
  int  i, singular;          /* Looping variable */
  
  /* The equation to be solved is LHSx = rhs */
  
  /* Compute upper triangular U such that U'*U = LHS */
  if(gnsCholeskyDec(LHS, U, noEq)) /* If Cholesky decomposition has been successful */
  {
    singular=1;
    /* Now, the equation is  U'*U*x = rhs, where U is upper triangular
     * Solve U'*aux = rhs for aux
     */
    gnsTransposeBacksubstitution(U, rhs, aux, noEq);         

    /* The equation is now U*x = aux, solve it for x (new motion coefficients) */
    gnsBacksubstitution(U, aux, noEq, x);   

  }
  else /* LHS was singular */ 
  {
    singular=0;

    /* Regularize LHS */
    for(i=0; i<noEq; i++)
      LHS[i][i] += REG;
    /* Compute upper triangular U such that U'*U = regularized LHS */
    singular = gnsCholeskyDec(LHS, U, noEq);
    /* Solve  U'*aux = rhs for aux */  
    gnsTransposeBacksubstitution(U, rhs, aux, noEq);   

    /* Solve U*x = aux for x */
    gnsBacksubstitution(U, aux, noEq, x);
  }  
  return(singular);
}


//////////////////////////////////////////////////////////////////////////////////////////


Void TEncAdaptiveLoopFilter::add_A(double **Amerged, double ***A, int start, int stop, int size)
{ 
  int
    i, j, ind;          /* Looping variable */

  for (i=0; i<size; i++){
    for (j=0; j<size; j++){
      Amerged[i][j]=0;
      for (ind=start; ind<=stop; ind++){
        Amerged[i][j]+=A[ind][i][j];
      }
    }
  }
}

Void TEncAdaptiveLoopFilter::add_b(double *bmerged, double **b, int start, int stop, int size)
{ 
  int
    i, ind;          /* Looping variable */

  for (i=0; i<size; i++){
    bmerged[i]=0;
    for (ind=start; ind<=stop; ind++){
      bmerged[i]+=b[ind][i];
    }
  }
}
  
double TEncAdaptiveLoopFilter::add_pixAcc(double *pixAcc, int start, int stop)
{ 
  int ind;
  double pixAccMerged=0;

  for (ind=start; ind<=stop; ind++){
    pixAccMerged+=pixAcc[ind];
  }
  return(pixAccMerged);
}


double TEncAdaptiveLoopFilter::calculateErrorCoeffProvided(double **A, double *b, double *c, int size)
{
  int i, j;
  double error, sum=0;

  error=0;
  for (i=0; i<size; i++)   //diagonal
  {
    sum=0;
	for (j=i+1; j<size; j++)
	  sum+=(A[j][i]+A[i][j])*c[j];
	error+=(A[i][i]*c[i]+sum-2*b[i])*c[i];
  }

  return(error);
}


double TEncAdaptiveLoopFilter::calculateErrorCoeffProvidedInt(double **A, double *b, int *c, int size,int *pattern)
{
  int i, j;
  double error, sum=0;
  double factor = 1.0/(double)(1<<(NUM_BITS-1));
  double d[MAX_SQR_FILT_LENGTH];
  error=0;

  for (i=0; i<size; i++)
  {
	d[i]=c[i]*factor;
  }

  for (i=0; i<size-1; i++)   //diagonal
  {
    sum=0;
	for (j=i+1; j<size-1; j++)
		sum+=(A[j][i]+A[i][j])*d[pattern[j]];
	sum+=(A[size-1][i]+A[i][size-1])*d[MAX_SQR_FILT_LENGTH-1];
	error+=(A[i][i]*d[pattern[i]]+sum-2*b[i])*d[pattern[i]];
  }
  i=size-1;
  error+=(A[i][i]*d[MAX_SQR_FILT_LENGTH-1]-2*b[i])*d[MAX_SQR_FILT_LENGTH-1];

  return(error);
}



double TEncAdaptiveLoopFilter::calculateErrorAbs(double **A, double *b, double y, int size)
{
  int i;
  double error, sum;
#if ALF_MEM_PATCH
  double c[MAX_SQR_FILT_LENGTH];
#else
  static double *c;
  static int first=0;
#endif

#if !ALF_MEM_PATCH
  if(first == 0)
  {
    first = 1;
    c = (double *) calloc(MAX_SQR_FILT_LENGTH, sizeof(double));
  }
#endif

  gnsSolveByChol(A, b, c, size);

  sum=0;
  for (i=0; i<size; i++)
  {
    sum+=c[i]*b[i];
  }
  error=y-sum;

  return(error);
}

double TEncAdaptiveLoopFilter::mergeFiltersGreedy(double **yGlobalSeq, double ***EGlobalSeq, double *pixAccGlobalSeq, int intervalBest[NO_VAR_BINS][2], int sqrFiltLength, int noIntervals)
{
#if ALF_MEM_PATCH
  int first, ind, ind1, ind2, i, j, bestToMerge ;
  double error, error1, error2, errorMin;
  static double pixAcc_temp, error_tab[NO_VAR_BINS],error_comb_tab[NO_VAR_BINS];
  static int indexList[NO_VAR_BINS], available[NO_VAR_BINS], noRemaining;
#else
  int first, ind, ind1, ind2, i, j, bestToMerge ;
  double error, error1, error2, errorMin;
  static double **y_merged, ***E_merged, *pixAcc_merged, *y_temp, **E_temp, pixAcc_temp, error_tab[NO_VAR_BINS],error_comb_tab[NO_VAR_BINS];
  static int init=0, indexList[NO_VAR_BINS], available[NO_VAR_BINS], noRemaining;

  if(init == 0)
  {
    initMatrix_double(&E_temp, MAX_SQR_FILT_LENGTH, MAX_SQR_FILT_LENGTH);
    y_temp = (double *) calloc(MAX_SQR_FILT_LENGTH, sizeof(double));

    initMatrix3D_double(&E_merged, NO_VAR_BINS, MAX_SQR_FILT_LENGTH, MAX_SQR_FILT_LENGTH);
    initMatrix_double(&y_merged, NO_VAR_BINS, MAX_SQR_FILT_LENGTH); 
    pixAcc_merged = (double *) calloc(NO_VAR_BINS, sizeof(double));
    init = 1;
  }
#endif
  if (noIntervals == NO_FILTERS)
  {
	  noRemaining=NO_VAR_BINS;
	  for (ind=0; ind<NO_VAR_BINS; ind++)
	  {
		indexList[ind]=ind; 
		available[ind]=1;
		pixAcc_merged[ind]=pixAccGlobalSeq[ind];
		memcpy(y_merged[ind],yGlobalSeq[ind],sizeof(double)*sqrFiltLength);
		for (i=0; i<sqrFiltLength; i++)
		{
	      memcpy(E_merged[ind][i],EGlobalSeq[ind][i],sizeof(double)*sqrFiltLength);
		}
	  }
  }
  // Try merging different matrices
      if (noIntervals == NO_FILTERS)
	  {
		  for (ind=0; ind<NO_VAR_BINS; ind++)
		  {
			  error_tab[ind]=calculateErrorAbs(E_merged[ind], y_merged[ind], pixAcc_merged[ind], sqrFiltLength);
		  }
		  for (ind=0; ind<NO_VAR_BINS-1; ind++)
		  {
			ind1=indexList[ind];
			ind2=indexList[ind+1];

			error1=error_tab[ind1];
			error2=error_tab[ind2];

			pixAcc_temp=pixAcc_merged[ind1]+pixAcc_merged[ind2];
			for (i=0; i<sqrFiltLength; i++)
			{
			  y_temp[i]=y_merged[ind1][i]+y_merged[ind2][i];
			  for (j=0; j<sqrFiltLength; j++)
			  {
				E_temp[i][j]=E_merged[ind1][i][j]+E_merged[ind2][i][j];
			  }
			}
			error_comb_tab[ind1]=calculateErrorAbs(E_temp, y_temp, pixAcc_temp, sqrFiltLength)-error1-error2;
		  }
	  }
	  while (noRemaining>noIntervals)
	  {
		errorMin=0; first=1;
		bestToMerge = 0;
		for (ind=0; ind<noRemaining-1; ind++)
		{
			error = error_comb_tab[indexList[ind]];
			if ((error<errorMin || first==1))
			{
			  errorMin=error;
			  bestToMerge=ind;
			  first=0;
			}
		}
		ind1=indexList[bestToMerge];
		ind2=indexList[bestToMerge+1];
		pixAcc_merged[ind1]+=pixAcc_merged[ind2];
		for (i=0; i<sqrFiltLength; i++)
		{
			y_merged[ind1][i]+=y_merged[ind2][i];
			for (j=0; j<sqrFiltLength; j++)
			{
			  E_merged[ind1][i][j]+=E_merged[ind2][i][j];
			}
		}
        available[ind2]=0;

		//update error tables
        error_tab[ind1]=error_comb_tab[ind1]+error_tab[ind1]+error_tab[ind2];
		if (indexList[bestToMerge] > 0)
		{
			ind1=indexList[bestToMerge-1];
			ind2=indexList[bestToMerge];
			error1=error_tab[ind1];
			error2=error_tab[ind2];
			pixAcc_temp=pixAcc_merged[ind1]+pixAcc_merged[ind2];
			for (i=0; i<sqrFiltLength; i++)
			{
				y_temp[i]=y_merged[ind1][i]+y_merged[ind2][i];
				for (j=0; j<sqrFiltLength; j++)
				{
					E_temp[i][j]=E_merged[ind1][i][j]+E_merged[ind2][i][j];
				}
			}
			error_comb_tab[ind1]=calculateErrorAbs(E_temp, y_temp, pixAcc_temp, sqrFiltLength)-error1-error2;
		}
		if (indexList[bestToMerge+1] < NO_VAR_BINS-1)
		{
			ind1=indexList[bestToMerge];
			ind2=indexList[bestToMerge+2];
			error1=error_tab[ind1];
			error2=error_tab[ind2];
			pixAcc_temp=pixAcc_merged[ind1]+pixAcc_merged[ind2];
			for (i=0; i<sqrFiltLength; i++)
			{
				y_temp[i]=y_merged[ind1][i]+y_merged[ind2][i];
				for (j=0; j<sqrFiltLength; j++)
				{
					E_temp[i][j]=E_merged[ind1][i][j]+E_merged[ind2][i][j];
				}
			}
			error_comb_tab[ind1]=calculateErrorAbs(E_temp, y_temp, pixAcc_temp, sqrFiltLength)-error1-error2;
		}

		ind=0;
		for (i=0; i<NO_VAR_BINS; i++)
		{
			if (available[i]==1)
			{
				indexList[ind]=i;
				ind++;
			}
		}
		noRemaining--;
	  }
	

  errorMin=0;
  for (ind=0; ind<noIntervals; ind++)
  {
    errorMin+=error_tab[indexList[ind]];
  }

  for (ind=0; ind<noIntervals-1; ind++)
  {
    intervalBest[ind][0]=indexList[ind]; intervalBest[ind][1]=indexList[ind+1]-1;
  }

  intervalBest[noIntervals-1][0]=indexList[noIntervals-1]; 
  intervalBest[noIntervals-1][1]=NO_VAR_BINS-1;

  return(errorMin);
}



double TEncAdaptiveLoopFilter::findFilterGroupingError(double ***EGlobalSeq, double **yGlobalSeq, double *pixAccGlobalSeq, int intervalBest[NO_VAR_BINS][2], int sqrFiltLength, int filters_per_fr)
{
  double error;

  // find best filters for each frame group
    error = 0;
    error += mergeFiltersGreedy(yGlobalSeq, EGlobalSeq, pixAccGlobalSeq, intervalBest, sqrFiltLength, filters_per_fr);

   return(error);
}


Void TEncAdaptiveLoopFilter::roundFiltCoeff(int *FilterCoeffQuan, double *FilterCoeff, int sqrFiltLength, int factor)
{
  int i;
  double diff; 
  int diffInt, sign; 

  for(i = 0; i < sqrFiltLength; i++)
  {
    sign               = (FilterCoeff[i]>0) ?  1: -1; 
    diff               = FilterCoeff[i]*sign; 
    diffInt            = (int)(diff*(double)factor+0.5); 
    FilterCoeffQuan[i] = diffInt*sign;	
  }
}







Double TEncAdaptiveLoopFilter::QuantizeIntegerFilterPP(double *filterCoeff, int *filterCoeffQuant, double **E, double *y, int sqrFiltLength, int *weights, int bit_depth)
{
  
  double error;

#if !ALF_MEM_PATCH
  static int init=0, *filterCoeffQuantMod;
#endif
 int factor = (1<<(bit_depth-1)), i; 
 int quantCoeffSum, minInd, targetCoeffSumInt, k, diff;
 double targetCoeffSum, errMin;

#if !ALF_MEM_PATCH
  if(init == 0)
  { 
    filterCoeffQuantMod = (int *) calloc(MAX_SQR_FILT_LENGTH, sizeof(int));
    init = 1;
  }
#endif
  gnsSolveByChol(E, y, filterCoeff, sqrFiltLength);
  targetCoeffSum=0;
  for (i=0; i<sqrFiltLength; i++)
  {
    targetCoeffSum+=(weights[i]*filterCoeff[i]*factor);
  }
  targetCoeffSumInt=ROUND(targetCoeffSum);
  roundFiltCoeff(filterCoeffQuant, filterCoeff, sqrFiltLength, factor);
  quantCoeffSum=0;
  for (i=0; i<sqrFiltLength; i++)
  {
    quantCoeffSum+=weights[i]*filterCoeffQuant[i];
  }

#if ALF_MEM_PATCH
  int count=0;
  while(quantCoeffSum!=targetCoeffSumInt && count < 10)
#else
  while(quantCoeffSum!=targetCoeffSumInt)
#endif
  {
    if (quantCoeffSum>targetCoeffSumInt)
	{
      diff=quantCoeffSum-targetCoeffSumInt;
      errMin=0; minInd=-1;
      for (k=0; k<sqrFiltLength; k++){
        if (weights[k]<=diff){
          for (i=0; i<sqrFiltLength; i++){
            filterCoeffQuantMod[i]=filterCoeffQuant[i];
          }
          filterCoeffQuantMod[k]--;
          for (i=0; i<sqrFiltLength; i++){
            filterCoeff[i]=(double)filterCoeffQuantMod[i]/(double)factor;
          }
          error=calculateErrorCoeffProvided(E, y, filterCoeff, sqrFiltLength);
          if (error<errMin || minInd==-1){
            errMin=error;
            minInd=k;
          }
        } // if (weights(k)<=diff){
      } // for (k=0; k<sqrFiltLength; k++){
      filterCoeffQuant[minInd]--;
    }
    else{
      diff=targetCoeffSumInt-quantCoeffSum;
      errMin=0; minInd=-1;
      for (k=0; k<sqrFiltLength; k++){
        if (weights[k]<=diff){
          for (i=0; i<sqrFiltLength; i++){
            filterCoeffQuantMod[i]=filterCoeffQuant[i];
          }
          filterCoeffQuantMod[k]++;
          for (i=0; i<sqrFiltLength; i++){
            filterCoeff[i]=(double)filterCoeffQuantMod[i]/(double)factor;
          }
          error=calculateErrorCoeffProvided(E, y, filterCoeff, sqrFiltLength);
          if (error<errMin || minInd==-1){
            errMin=error;
            minInd=k;
          }
        } // if (weights(k)<=diff){
      } // for (k=0; k<sqrFiltLength; k++){
      filterCoeffQuant[minInd]++;
    }

    quantCoeffSum=0;
    for (i=0; i<sqrFiltLength; i++){
      quantCoeffSum+=weights[i]*filterCoeffQuant[i];
    }
  }
#if ALF_MEM_PATCH
  if( count == 10 )
  {
   for (i=0; i<sqrFiltLength; i++)
   {
    filterCoeffQuant[i] = 0;
   }
  }
#endif

   for (i=0; i<sqrFiltLength; i++)
   {
    filterCoeff[i]=(double)filterCoeffQuant[i]/(double)factor;
  }

   error=calculateErrorCoeffProvided(E, y, filterCoeff, sqrFiltLength);
   return(error);

}



Double TEncAdaptiveLoopFilter::findFilterCoeff(double ***EGlobalSeq, double **yGlobalSeq, double *pixAccGlobalSeq, int **filterCoeffSeq, int **filterCoeffQuantSeq, int intervalBest[NO_VAR_BINS][2], int varIndTab[NO_VAR_BINS], int sqrFiltLength, int filters_per_fr, int *weights, int bit_depth, double errorTabForce0Coeff[NO_VAR_BINS][2])
{
#if ALF_MEM_PATCH
  static double pixAcc_temp;
#else
  static int init = 0;
  static double **E_temp, *y_temp, *filterCoeff, pixAcc_temp;
  static int *filterCoeffQuant;
#endif
  double error;
  int k, filtNo;


#if !ALF_MEM_PATCH
  if(init == 0)
  {
    initMatrix_double(&E_temp, MAX_SQR_FILT_LENGTH, MAX_SQR_FILT_LENGTH);
    y_temp = (double *) calloc(MAX_SQR_FILT_LENGTH, sizeof(double));
    filterCoeff = (double *) calloc(MAX_SQR_FILT_LENGTH, sizeof(double));
    filterCoeffQuant = (int *) calloc(MAX_SQR_FILT_LENGTH, sizeof(int));
    init = 1;
  }
#endif  

  error = 0;
  for(filtNo = 0; filtNo < filters_per_fr; filtNo++)
  {
    add_A(E_temp, EGlobalSeq, intervalBest[filtNo][0], intervalBest[filtNo][1], sqrFiltLength);
    add_b(y_temp, yGlobalSeq, intervalBest[filtNo][0], intervalBest[filtNo][1], sqrFiltLength);

    pixAcc_temp = 0;    
    for(k = intervalBest[filtNo][0]; k <= intervalBest[filtNo][1]; k++)
      pixAcc_temp += pixAccGlobalSeq[k];
    
    // Find coeffcients
    errorTabForce0Coeff[filtNo][1] = pixAcc_temp + QuantizeIntegerFilterPP(filterCoeff, filterCoeffQuant, E_temp, y_temp, sqrFiltLength, weights, bit_depth);
	errorTabForce0Coeff[filtNo][0] = pixAcc_temp;
    error += errorTabForce0Coeff[filtNo][1];

    for(k = 0; k < sqrFiltLength; k++)
    {
      filterCoeffSeq[filtNo][k] = filterCoeffQuant[k];
      filterCoeffQuantSeq[filtNo][k] = filterCoeffQuant[k];
    }
  }
  
  for(filtNo = 0; filtNo < filters_per_fr; filtNo++)
    for(k = intervalBest[filtNo][0]; k <= intervalBest[filtNo][1]; k++)
      varIndTab[k] = filtNo;

  return(error);
}

#endif
