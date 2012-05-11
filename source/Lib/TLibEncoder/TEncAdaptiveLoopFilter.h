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

/** \file     TEncAdaptiveLoopFilter.h
 \brief    estimation part of adaptive loop filter class (header)
 */

#ifndef __TENCADAPTIVELOOPFILTER__
#define __TENCADAPTIVELOOPFILTER__

#include "TLibCommon/TComAdaptiveLoopFilter.h"
#include "TLibCommon/TComPic.h"

#include "TEncEntropy.h"
#include "TEncSbac.h"
#include "TLibCommon/TComBitCounter.h"

//! \ingroup TLibEncoder
//! \{
#if !AHG6_ALF_OPTION2
#define LCUALF_FILTER_BUDGET_CONTROL_ENC        1 //!< filter budget control 
#endif
#define LCUALF_AVOID_USING_BOTTOM_LINES_ENCODER 1 //!< avoid using LCU bottom lines when lcu-based encoder RDO is used
#if AHG6_ALF_OPTION2
#define LCUALF_AVOID_USING_RIGHT_LINES_ENCODER 1  //!< Avoid using right-most lines per LCU during encoder LCU on/off decision
#endif
// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// correlation info
struct AlfCorrData
{
  Double*** ECorr; //!< auto-correlation matrix
  Double**  yCorr; //!< cross-correlation
  Double*   pixAcc;
  Int componentID;

  //constructor & operator
  AlfCorrData();
  AlfCorrData(Int cIdx);
  ~AlfCorrData();
  Void reset();
  Void mergeFrom(const AlfCorrData& src, Int* mergeTable, Bool doPixAccMerge);
  AlfCorrData& operator += (const AlfCorrData& src);
};
#if !AHG6_ALF_OPTION2
/// picture quad-tree info
struct AlfPicQTPart
{
  Int         componentID;
  Int         partCUXS;
  Int         partCUYS;
  Int         partCUXE;
  Int         partCUYE;
  Int         partIdx;
  Int         partLevel;
  Int         partCol;
  Int         partRow;
  Int         childPartIdx[4];
  Int         parentPartIdx;
  Bool        isBottomLevel;
  Bool        isSplit;
  Bool        isProcessed;
  Double      splitMinCost;
  Int64       splitMinDist;
  Int64       splitMinRate;
  Double      selfMinCost;
  Int64       selfMinDist;
  Int64       selfMinRate;
  Int         numFilterBudget;

  AlfUnitParam* alfUnitParam; 
  AlfCorrData*  alfCorr;

  //constructor & operator
  AlfPicQTPart();
  ~AlfPicQTPart();
  AlfPicQTPart& operator= (const AlfPicQTPart& src);
};
#endif
/// estimation part of adaptive loop filter class
class TEncAdaptiveLoopFilter : public TComAdaptiveLoopFilter
{
private:
  ///
  /// variables for correlation calculation
  ///
#if AHG6_ALF_OPTION2
  Double*  m_y_merged[NO_VAR_BINS];
  Double** m_E_merged[NO_VAR_BINS];
  Double   m_pixAcc_merged[NO_VAR_BINS];
  Double   m_y_temp[ALF_MAX_NUM_COEF];
#else
  double **m_y_merged;
  double ***m_E_merged;
  double *m_pixAcc_merged;
  double *m_y_temp;
#endif
  double **m_E_temp;
#if !AHG6_ALF_OPTION2
  static const Int  m_alfNumPartsInRowTab[5];
  static const Int  m_alfNumPartsLevelTab[5];
  static const Int  m_alfNumCulPartsLevelTab[5];
#endif
  Int    m_lastSliceIdx;
#if AHG6_ALF_OPTION2
  Bool   m_alfLowLatencyEncoding;  
#else
  Bool   m_picBasedALFEncode;
  Bool   m_alfCoefInSlice;
#endif
  Int*   m_numSlicesDataInOneLCU;
  Int*   m_coeffNoFilter[NO_VAR_BINS]; //!< used for RDO
#if !AHG6_ALF_OPTION2
  AlfParamSet* m_bestAlfParamSet;
#endif
  AlfCorrData** m_alfCorr[NUM_ALF_COMPONENT];
  AlfCorrData*  m_alfCorrMerged[NUM_ALF_COMPONENT]; //!< used for RDO
#if AHG6_ALF_OPTION2
  AlfCorrData** m_alfNonSkippedCorr[NUM_ALF_COMPONENT];
  ALFParam*** m_alfPictureParam;
  Int         m_gopSize;                
  Double      m_dLambdaLuma;
  Double      m_dLambdaChroma;
#else
  AlfUnitParam* m_alfPicFiltUnits[NUM_ALF_COMPONENT];
  Int    m_alfPQTMaxDepth;
  AlfPicQTPart* m_alfPQTPart[NUM_ALF_COMPONENT]; 
#if LCUALF_FILTER_BUDGET_CONTROL_ENC
  Double m_alfFiltBudgetPerLcu;
  Int    m_alfUsedFilterNum;
#endif

  ///
  /// ALF parameters
  ///
  ALFParam *m_tempALFp;

  ///
  /// temporary picture buffers or pointers
  ///
  TComPicYuv* m_pcPicYuvBest;
  TComPicYuv* m_pcPicYuvTmp;

  ///
  /// temporary filter buffers or pointers
  ///
  Int    *m_filterCoeffQuantMod;
  double *m_filterCoeff;
  Int    *m_filterCoeffQuant;
  Int    **m_filterCoeffSymQuant;
  Int    **m_diffFilterCoeffQuant;
  Int    **m_FilterCoeffQuantTemp;
  ///
  /// coding control parameters
  ///
  Double m_dLambdaLuma;
  Double m_dLambdaChroma;
  Int  m_iALFEncodePassReduction; //!< 0: 16-pass encoding, 1: 1-pass encoding, 2: 2-pass encoding

  Int  m_iALFMaxNumberFilters;    //!< ALF Max Number Filters per unit

  Int  m_iALFNumOfRedesign;       //!< number of redesigning filter for each CU control depth

  ///
  /// variables for on/off control
  ///
  Pel **m_maskImg;
  Bool m_bAlfCUCtrlEnabled;                         //!< if input ALF CU control param is NULL, this variable is set to be false (Disable CU control)
  std::vector<AlfCUCtrlInfo> m_vBestAlfCUCtrlParam; //!< ALF CU control parameters container to store the ALF CU control parameters after RDO

  ///
  /// miscs. 
  ///
  TEncEntropy* m_pcEntropyCoder;
#endif
private:

#if AHG6_ALF_OPTION2
  Void initALFEncoderParam();
#else
  Void disableComponentAlfParam(Int compIdx, AlfParamSet* alfParamSet, AlfUnitParam* alfUnitPic);
  Void copyAlfParamSet(AlfParamSet* dst, AlfParamSet* src);
  Void initALFEncoderParam(AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam);
  Void assignALFEncoderParam(AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam);
#endif
  Void getStatistics(TComPicYuv* pPicOrg, TComPicYuv* pPicDec);
#if AHG6_ALF_OPTION2
  Void getOneCompStatistics(AlfCorrData** alfCorrComp, Int compIdx, Pel* imgOrg, Pel* imgDec, Int stride, Int formatShift);
  Void getStatisticsOneLCU(Bool skipLCUBottomLines, Int compIdx, AlfLCUInfo* alfLCU, AlfCorrData* alfCorr, Pel* pPicOrg, Pel* pPicSrc, Int stride, Int formatShift);
  Void decideParameters(ALFParam** alfPictureParam, TComPicYuv* pPicOrg, TComPicYuv* pPicDec, TComPicYuv* pPicRest);
  Void setCurAlfParam(ALFParam** alfPictureParam);
  Int  getTemporalLayerNo(Int poc, Int gopSize);
  Void decideAlfPictureParam(ALFParam** alfPictureParam, Bool useAllLCUs);
  Void estimateLcuControl(ALFParam** alfPictureParam);
  Void deriveFilterInfo(Int compIdx, AlfCorrData* alfCorr, ALFParam* alfFiltParam, Int maxNumFilters, Double lambda);
#else
  Void getOneCompStatistics(AlfCorrData** alfCorrComp, Int compIdx, Pel* imgOrg, Pel* imgDec, Int stride, Int formatShift, Bool isRedesignPhase);
  Void getStatisticsOneLCU(Bool skipLCUBottomLines, Int compIdx, AlfLCUInfo* alfLCU, AlfCorrData* alfCorr, Pel* pPicOrg, Pel* pPicSrc, Int stride, Int formatShift, Bool isRedesignPhase);
  Void decideParameters(TComPicYuv* pPicOrg, TComPicYuv* pPicDec, TComPicYuv* pPicRest, AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam);
  Void deriveFilterInfo(Int compIdx, AlfCorrData* alfCorr, ALFParam* alfFiltParam, Int maxNumFilters);
  Void decideBlockControl(Pel* pOrg, Pel* pDec, Pel* pRest, Int stride, AlfPicQTPart* alfPicQTPart, AlfParamSet* & alfParamSet, Int64 &minRate, Int64 &minDist, Double &minCost);
  Void copyPicQT(AlfPicQTPart* alfPicQTPartDest, AlfPicQTPart* alfPicQTPartSrc);
  Void copyPixelsInOneRegion(Pel* imgDest, Pel* imgSrc, Int stride, Int yPos, Int height, Int xPos, Int width);
  Void reDesignQT(AlfPicQTPart *alfPicQTPart, Int partIdx, Int partLevel);
  Void setCUAlfCtrlFlags(UInt uiAlfCtrlDepth, Pel* imgOrg, Pel* imgDec, Pel* imgRest, Int stride, UInt64& ruiDist, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam);
  Void setCUAlfCtrlFlag(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiAlfCtrlDepth, Pel* imgOrg, Pel* imgDec, Pel* imgRest, Int stride, UInt64& ruiDist, std::vector<UInt>& vCUCtrlFlag);
  Void xCopyDecToRestCUs( Pel* imgDec, Pel* imgRest, Int stride );
  Void xCopyDecToRestCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Pel* imgDec, Pel* imgRest, Int stride );
#endif
  Void calcCorrOneCompRegionChma(Pel* imgOrg, Pel* imgPad, Int stride, Int yPos, Int xPos, Int height, Int width, Double **eCorr, Double *yCorr, Bool isSymmCopyBlockMatrix); //!< Calculate correlations for chroma                                        
#if AHG6_ALF_OPTION2
  Void calcCorrOneCompRegionLuma(Pel* imgOrg, Pel* imgPad, Int stride, Int yPos, Int xPos, Int height, Int width, Double ***eCorr, Double **yCorr, Double *pixAcc, Bool isSymmCopyBlockMatrix);

  //LCU-based mode decision
  Void  executeLCUOnOffDecision(Int compIdx, ALFParam* alfParam, Pel* pOrg, Pel* pDec, Pel* pRest, Int stride, Int formatShift, AlfCorrData** alfCorrLCUs);
  Int64 estimateFilterDistortion(Int compIdx, AlfCorrData* alfCorr, Int** coeff = NULL, Int filterSetSize = 1, Int* mergeTable = NULL, Bool doPixAccMerge = false);
#else
  Void calcCorrOneCompRegionLuma(Pel* imgOrg, Pel* imgPad, Int stride, Int yPos, Int xPos, Int height, Int width, Double ***eCorr, Double **yCorr, Double *pixAcc, Bool isforceCollection, Bool isSymmCopyBlockMatrix);

  //LCU-based mode decision
  Void  executeLCUBasedModeDecision(AlfParamSet* alfParamSet, Int compIdx, Pel* pOrg, Pel* pDec, Pel* pRest, Int stride, Int formatShift, AlfCorrData** alfCorrLCUs);
  Void  decideLCUALFUnitParam(Int compIdx, AlfUnitParam* alfUnitPic, Int lcuIdx, Int lcuPos, Int numLCUWidth, AlfUnitParam* alfUnitParams, AlfCorrData* alfCorr, std::vector<ALFParam*>& storedFilters, Int maxNumFilter, Double lambda, Bool isLeftUnitAvailable, Bool isUpUnitAvailable);
  Void  getFiltOffAlfUnitParam(AlfUnitParam* alfFiltOffParam, Int lcuPos, AlfUnitParam* alfUnitPic, Bool isLeftUnitAvailable, Bool isUpUnitAvailable);
  Int64 estimateFilterDistortion(Int compIdx, AlfCorrData* alfCorr, Int** coeff = NULL, Int filterSetSize = 1, Int* mergeTable = NULL, Bool doPixAccMerge = false);
  Int   calculateAlfUnitRateRDO(AlfUnitParam* alfUnitParam, Int numStoredFilters = 0);
#endif
  Int64 calcAlfLCUDist(Bool skipLCUBottomLines, Int compIdx, AlfLCUInfo& alfLCUInfo, Pel* picSrc, Pel* picCmp, Int stride, Int formatShift);
#if AHG6_ALF_OPTION2
  Void  reconstructOneAlfLCU(Int compIdx, AlfLCUInfo& alfLCUInfo, Bool alfEnabled, ALFParam* alfParam, Pel* picDec, Pel* picRest, Int stride, Int formatShift);
#else
  Void  reconstructOneAlfLCU(Int compIdx, AlfLCUInfo& alfLCUInfo, AlfUnitParam* alfUnitParam, Pel* picDec, Pel* picRest, Int stride, Int formatShift);
#endif
  Void  copyOneAlfLCU(AlfLCUInfo& alfLCUInfo, Pel* picDst, Pel* picSrc, Int stride, Int formatShift);
#if !AHG6_ALF_OPTION2
  //picture-based mode decision
  Void executePicBasedModeDecision(AlfParamSet* alfParamSet, AlfPicQTPart* alfPicQTPart, Int compIdx, Pel* pOrg, Pel* pDec, Pel* pRest, Int stride, Int formatShift, AlfCorrData** alfCorrLCUs);
  Void creatPQTPart               (Int partLevel, Int partRow, Int partCol, Int parentPartIdx, Int partCUXS, Int partCUXE, Int partCUYS, Int partCUYE);
  Void resetPQTPart               ();
  Int  convertLevelRowCol2Idx     (Int level, Int row, Int col);
  Void convertIdx2LevelRowCol     (Int idx, Int *level, Int *row, Int *col);
  Void executeModeDecisionOnePart (AlfPicQTPart *alfPicQTPart, AlfCorrData** alfPicLCUCorr, Int partIdx, Int partLevel);
  Void decideQTPartition          (AlfPicQTPart* alfPicQTPart, AlfCorrData** alfPicLCUCorr, Int partIdx, Int partLevel, Double &cost, Int64 &dist, Int64 &rate);
  Void patchAlfUnitParams(AlfPicQTPart* alfPicQTPart, Int partIdx, AlfUnitParam* alfUnitPic);
  Void checkMerge(Int compIdx, AlfUnitParam* alfUnitPic);
  Void transferToAlfParamSet(Int compIdx, AlfUnitParam* alfUnitPic, AlfParamSet* & alfParamSet);
  Int  calculateAlfParamSetRateRDO(Int compIdx, AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCUCtrlParam);
  // ALF on/off control related functions
  Void xCreateTmpAlfCtrlFlags   ();
  Void xDestroyTmpAlfCtrlFlags  ();
  Void xCopyTmpAlfCtrlFlagsTo   ();
  Void xCopyTmpAlfCtrlFlagsFrom ();
  Void getCtrlFlagsFromCU(AlfLCUInfo* pcAlfLCU, std::vector<UInt> *pvFlags, Int iAlfDepth, UInt uiMaxNumSUInLCU);
  Void xEncodeCUAlfCtrlFlags  (std::vector<AlfCUCtrlInfo> &vAlfCUCtrlParam);
  Void xEncodeCUAlfCtrlFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth);
#endif
  // functions related to filtering
  Void xFilterCoefQuickSort   ( Double *coef_data, Int *coef_num, Int upper, Int lower );
  Void xQuantFilterCoef       ( Double* h, Int* qh, Int tap, int bit_depth );
  // distortion / misc functions
  UInt64 xCalcSSD             ( Pel* pOrg, Pel* pCmp, Int iWidth, Int iHeight, Int iStride );
  Int64 xFastFiltDistEstimation(Double** ppdE, Double* pdy, Int* piCoeff, Int iFiltLength); //!< Estimate filtering distortion by correlation values and filter coefficients

  /// code filter coefficients
#if AHG6_ALF_OPTION2
  Void xcodeFiltCoeff(Int **filterCoeff, Int* varIndTab, Int numFilters, ALFParam* alfParam);
  Void xfindBestFilterVarPred(double **ySym, double ***ESym, double *pixAcc, Int **filterCoeffSym, Int *filters_per_fr_best, Int varIndTab[], double lambda_val, Int numMaxFilters);
#else
  UInt xcodeFiltCoeff(Int **filterCoeffSymQuant, Int filter_shape, Int varIndTab[], Int filters_per_fr_best, ALFParam* ALFp);
  Void xfindBestFilterVarPred(double **ySym, double ***ESym, double *pixAcc, Int **filterCoeffSym, Int **filterCoeffSymQuant,Int filter_shape, Int *filters_per_fr_best, Int varIndTab[], Pel **imgY_rec, Pel **varImg, Pel **maskImg, Pel **imgY_pad, double lambda_val, Int numMaxFilters = NO_FILTERS);
#endif
  double xfindBestCoeffCodMethod(int **filterCoeffSymQuant, int filter_shape, int sqrFiltLength, int filters_per_fr, double errorForce0CoeffTab[NO_VAR_BINS][2], double lambda);
#if AHG6_ALF_OPTION2
  UInt uvlcBitrateEstimate(Int val);
  UInt svlcBitrateEsitmate(Int val) {  return uvlcBitrateEstimate (( val <= 0) ? (-val<<1) : ((val<<1)-1));}
  UInt golombBitrateEstimate(Int coeff, Int k) { return (UInt)(((((Int)abs(coeff) )>> k) + 1+ k) + ((coeff !=0)?1:0));}
  UInt ALFParamBitrateEstimate(ALFParam* alfParam);
  UInt filterCoeffBitrateEstimate(Int compIdx, Int* coeff);
  Void predictALFCoeff(Int** coeff, Int numCoef, Int numFilters);
#else
  Int xsendAllFiltersPPPred(int **FilterCoeffQuant, int filter_shape, int sqrFiltLength, int filters_per_group, int createBistream, ALFParam* ALFp);
  Int xcodeAuxInfo(int filters_per_fr, int varIndTab[NO_VAR_BINS], int filter_shape, ALFParam* ALFp);
  Int xcodeFilterCoeff(int **pDiffQFilterCoeffIntPP, int filter_shape, int sqrFiltLength, int filters_per_group, int createBitstream);
  Int lengthGolomb(int coeffVal, int k);
  Int lengthPredFlags(int force0, int predMethod, int codedVarBins[NO_VAR_BINS], int filters_per_group, int createBitstream);
  Int lengthFilterCoeffs(int sqrFiltLength, int filters_per_group, int pDepthInt[], int **FilterCoeff, int kMinTab[], int createBitstream);
  Void predictALFCoeffLumaEnc(ALFParam* pcAlfParam, Int **pfilterCoeffSym, Int filter_shape); //!< prediction of luma ALF coefficients
#endif
  //cholesky related
  Int   xGauss( Double **a, Int N );
  Double findFilterCoeff(double ***EGlobalSeq, double **yGlobalSeq, double *pixAccGlobalSeq, int **filterCoeffSeq,int **filterCoeffQuantSeq, int intervalBest[NO_VAR_BINS][2], int varIndTab[NO_VAR_BINS], int sqrFiltLength, int filters_per_fr, int *weights, double errorTabForce0Coeff[NO_VAR_BINS][2]);
  Double QuantizeIntegerFilterPP(Double *filterCoeff, Int *filterCoeffQuant, Double **E, Double *y, Int sqrFiltLength, Int *weights);
  Void roundFiltCoeff(int *FilterCoeffQuan, double *FilterCoeff, int sqrFiltLength, int factor);
  double mergeFiltersGreedy(double **yGlobalSeq, double ***EGlobalSeq, double *pixAccGlobalSeq, int intervalBest[NO_VAR_BINS][2], int sqrFiltLength, int noIntervals);
  double calculateErrorAbs(double **A, double *b, double y, int size);
  double calculateErrorCoeffProvided(double **A, double *b, double *c, int size);
  Void add_b(double *bmerged, double **b, int start, int stop, int size);
  Void add_A(double **Amerged, double ***A, int start, int stop, int size);
  Int  gnsSolveByChol(double **LHS, double *rhs, double *x, int noEq);
  Void gnsBacksubstitution(double R[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF], double z[ALF_MAX_NUM_COEF], int R_size, double A[ALF_MAX_NUM_COEF]);
  Void gnsTransposeBacksubstitution(double U[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF], double rhs[], double x[],int order);
  Int  gnsCholeskyDec(double **inpMatr, double outMatr[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF], int noEq);

public:
  TEncAdaptiveLoopFilter          ();
  virtual ~TEncAdaptiveLoopFilter () {}
#if AHG6_ALF_OPTION2
  Void setALFLowLatencyEncoding(Bool b) {m_alfLowLatencyEncoding = b;}
#if ALF_CHROMA_LAMBDA
  Void ALFProcess(ALFParam** alfPictureParam, Double lambdaLuma, Double lambdaChroma);
#else
  Void ALFProcess(ALFParam** alfPictureParam, Double lambda);
#endif
  Void setGOPSize(Int val) { m_gopSize = val; } //!< set GOP size
#else
  Void startALFEnc(TComPic* pcPic, TEncEntropy* pcEntropyCoder); //!< allocate temporal memory
  Void endALFEnc(); //!< destroy temporal memory
#if ALF_CHROMA_LAMBDA
  Void ALFProcess( AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam, Double lambdaLuma, Double lambdaChroma);
#else
  Void ALFProcess( AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam, Double lambda);
#endif
  Void initALFEnc(Bool isAlfParamInSlice, Bool isPicBasedEncode, Int numSlices, AlfParamSet* & alfParams, std::vector<AlfCUCtrlInfo>* & alfCUCtrlParam);
  Void uninitALFEnc(AlfParamSet* & alfParams, std::vector<AlfCUCtrlInfo>* & alfCUCtrlParam);
  Void resetPicAlfUnit();
  Void setAlfCoefInSlice(Bool b) {m_alfCoefInSlice = b;}
  Void setALFEncodePassReduction (Int iVal) {m_iALFEncodePassReduction = iVal;} //!< set N-pass encoding. 0: 16(14)-pass encoding, 1: 1-pass encoding, 2: 2-pass encoding

  Void setALFMaxNumberFilters    (Int iVal) {m_iALFMaxNumberFilters = iVal;} //!< set ALF Max Number of Filters
#endif
  Void createAlfGlobalBuffers(); //!< create ALF global buffers
#if !AHG6_ALF_OPTION2
  Void initPicQuadTreePartition(Bool isPicBasedEncode);
#endif
  Void destroyAlfGlobalBuffers(); //!< destroy ALF global buffers
  Void PCMLFDisableProcess (TComPic* pcPic);
};

//! \}

#endif
