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

/** \file     TComTrQuant.h
    \brief    transform and quantization class (header)
*/

#ifndef __TCOMTRQUANT__
#define __TCOMTRQUANT__

#include "CommonDef.h"
#include "TComYuv.h"
#include "TComDataCU.h"
#include "ContextTables.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define QP_BITS                 15

// ====================================================================================================================
// Type definition
// ====================================================================================================================

typedef struct
{
#if MULTI_LEVEL_SIGNIFICANCE
  Int significantCoeffGroupBits[NUM_SIG_CG_FLAG_CTX][2];
#endif
  Int significantBits[NUM_SIG_FLAG_CTX][2];
  Int lastXBits[32];
  Int lastYBits[32];
  Int m_greaterOneBits[NUM_ONE_FLAG_CTX][2];
  Int m_levelAbsBits[NUM_ABS_FLAG_CTX][2];

  Int blockCbpBits[3*NUM_QT_CBF_CTX][2];
  Int blockRootCbpBits[4][2];
  Int scanZigzag[2];            ///< flag for zigzag scan
  Int scanNonZigzag[2];         ///< flag for non zigzag scan
} estBitsSbacStruct;

#if !DISABLE_CAVLC
typedef struct
{
  Int level[4];
  Int pre_level;
  Int coeff_ctr;
  Int levelDouble;
  Double errLevel[4];
  Int noLevels;
  Int levelQ;
  Bool lowerInt;
  UInt quantInd;
  Int iNextRun;
} levelDataStruct;

typedef struct
{
  Int run;
  Int maxrun;
  Int nextLev;
  Int nexLevelVal;
} quantLevelStruct;

class TEncCavlc;
#endif

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// QP class
class QpParam
{
public:
  QpParam();
  
  Int m_iQP;
  Int m_iPer;
  Int m_iRem;
  
public:
  Int m_iBits;
    
  Void setQpParam( Int iQP, Bool bLowpass, SliceType eSliceType )
  {
    assert ( iQP >= MIN_QP && iQP <= MAX_QP );
    m_iQP   = iQP;
    
    m_iPer  = (iQP + 6*g_uiBitIncrement)/6;
#if FULL_NBIT
    m_iPer += g_uiBitDepth - 8;
#endif
    m_iRem  = iQP % 6;
    
    m_iBits = QP_BITS + m_iPer;
  }
  
  Void clear()
  {
    m_iQP   = 0;
    m_iPer  = 0;
    m_iRem  = 0;
    m_iBits = 0;
  }
  
  
  Int per()   const { return m_iPer; }
  Int rem()   const { return m_iRem; }
  Int bits()  const { return m_iBits; }
  
  Int qp() {return m_iQP;}
}; // END CLASS DEFINITION QpParam

/// transform and quantization class
class TComTrQuant
{
public:
  TComTrQuant();
  ~TComTrQuant();
  
  // initialize class
  Void init                 ( UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxTrSize, Int iSymbolMode = 0, UInt *aTable4 = NULL, UInt *aTable8 = NULL, UInt *aTableLastPosVlcIndex=NULL, Bool bUseRDOQ = false,  Bool bEnc = false );
  
  // transform & inverse transform functions
  Void transformNxN         ( TComDataCU* pcCU, Pel*   pcResidual, UInt uiStride, TCoeff* rpcCoeff, UInt uiWidth, UInt uiHeight,
                             UInt& uiAbsSum, TextType eTType, UInt uiAbsPartIdx );
#if SCALING_LIST
  Void invtransformNxN      (TextType eText, UInt uiMode,Pel*& rpcResidual, UInt uiStride, TCoeff*   pcCoeff, UInt uiWidth, UInt uiHeight, Int scalingListType);
#else
  Void invtransformNxN      (TextType eText, UInt uiMode,Pel* rpcResidual, UInt uiStride, TCoeff*   pcCoeff, UInt uiWidth, UInt uiHeight);
#endif
  Void invRecurTransformNxN ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eTxt, Pel* rpcResidual, UInt uiAddr,   UInt uiStride, UInt uiWidth, UInt uiHeight,
                             UInt uiMaxTrMode,  UInt uiTrMode, TCoeff* rpcCoeff );
  
  // Misc functions
#if G509_CHROMA_QP_OFFSET
  Void setQPforQuant( Int iQP, Bool bLowpass, SliceType eSliceType, TextType eTxtType, Int Shift);
#else
  Void setQPforQuant( Int iQP, Bool bLowpass, SliceType eSliceType, TextType eTxtType);
#endif
#if RDOQ_CHROMA_LAMBDA 
  Void setLambda(Double dLambdaLuma, Double dLambdaChroma) { m_dLambdaLuma = dLambdaLuma; m_dLambdaChroma = dLambdaChroma; }
  Void selectLambda(TextType eTType) { m_dLambda = (eTType == TEXT_LUMA) ? m_dLambdaLuma : m_dLambdaChroma; }
#else
  Void setLambda(Double dLambda) { m_dLambda = dLambda;}
#endif
  Void setRDOQOffset( UInt uiRDOQOffset ) { m_uiRDOQOffset = uiRDOQOffset; }
  
  estBitsSbacStruct* m_pcEstBitsSbac;
  
  static UInt     getSigCtxInc     ( TCoeff*                         pcCoeff,
                                     const UInt                      uiPosX,
                                     const UInt                      uiPosY,
                                     const UInt                      uiLog2BlkSize,
#if NSQT_DIAG_SCAN
#if SIGMAP_CTX_RED
                                     const Int                       uiStride,
                                     const Int                       height, 
                                     const TextType                  textureType );
#else
                                    Int uiStride, Int height );
#endif
#else
#if SIGMAP_CTX_RED
                                     const UInt                       uiStride,
                                     const TextType                  textureType );
#else
                                     const UInt                      uiStride );
#endif
#endif
#if MULTI_LEVEL_SIGNIFICANCE
#if NSQT_DIAG_SCAN
  static UInt getSigCoeffGroupCtxInc  ( const UInt*                   uiSigCoeffGroupFlag,
                                       const UInt                       uiCGPosX,
                                       const UInt                       uiCGPosY,
                                       Int width, Int height);
  
  static Bool bothCGNeighboursOne  ( const UInt*                      uiSigCoeffGroupFlag,
                                    const UInt                       uiCGPosX,
                                    const UInt                       uiCGPosY,
                                    Int width, Int height);
#else
  static UInt getSigCoeffGroupCtxInc  ( const UInt*                   uiSigCoeffGroupFlag,
                                     const UInt                       uiCGPosX,
                                     const UInt                       uiCGPosY,
                                     const UInt                       uiLog2BlockSize);

  static Bool bothCGNeighboursOne  ( const UInt*                      uiSigCoeffGroupFlag,
                                     const UInt                       uiCGPosX,
                                     const UInt                       uiCGPosY,
                                     const UInt                       uiLog2BlockSize);
#endif
#endif
#if SCALING_LIST
  Void initScalingList                      ();
  Void destroyScalingList                   ();
  Void setErrScaleCoeff    ( UInt list, UInt size, UInt qp, UInt dir);
  double* getErrScaleCoeff ( UInt list, UInt size, UInt qp, UInt dir);
  Int* getQuantCoeff       ( UInt list, UInt qp, UInt size, UInt dir);
  Int* getDequantCoeff     ( UInt list, UInt qp, UInt size, UInt dir);
  Void setUseScalingList   ( Bool bUseScalingList){ m_scalingListEnabledFlag = bUseScalingList; };
  Bool getUseScalingList   (){ return m_scalingListEnabledFlag; };
  Void setFlatScalingList  ();
  Void xsetFlatScalingList ( UInt list, UInt size, UInt qp);
  Void xSetScalingListEnc  ( Int *scalingList, UInt list, UInt size, UInt qp);
  Void xSetScalingListDec  ( Int *scalingList, UInt list, UInt size, UInt qp);
  Void setScalingList      ( TComScalingList *scalingList);
  Void setScalingListDec   ( TComScalingList *scalingList);
#endif
protected:
  Int*    m_plTempCoeff;
  
  QpParam  m_cQP;
#if RDOQ_CHROMA_LAMBDA
  Double   m_dLambdaLuma;
  Double   m_dLambdaChroma;
#endif
  Double   m_dLambda;
  UInt     m_uiRDOQOffset;
  UInt     m_uiMaxTrSize;
  Bool     m_bEnc;
  Bool     m_bUseRDOQ;
  
#if !DISABLE_CAVLC 
  UInt     *m_uiLPTableE8;
  UInt     *m_uiLPTableE4;
  Int      m_iSymbolMode;
  UInt     *m_uiLastPosVlcIndex;
#endif
#if SCALING_LIST
  Bool     m_scalingListEnabledFlag;
  Int      *m_quantCoef      [SCALING_LIST_NUM][SCALING_LIST_REM_NUM];             ///< array of quantization matrix coefficient 4x4
  Int      *m_dequantCoef    [SCALING_LIST_NUM][SCALING_LIST_REM_NUM];             ///< array of dequantization matrix coefficient 4x4
  Int      *m_quantCoef64    [SCALING_LIST_NUM][SCALING_LIST_REM_NUM][SCALING_LIST_DIR_NUM]; ///< array of quantization matrix coefficient 8x8
  Int      *m_dequantCoef64  [SCALING_LIST_NUM][SCALING_LIST_REM_NUM][SCALING_LIST_DIR_NUM]; ///< array of dequantization matrix coefficient 8x8
  Int      *m_quantCoef256   [SCALING_LIST_NUM][SCALING_LIST_REM_NUM][SCALING_LIST_DIR_NUM]; ///< array of quantization matrix coefficient 16x16
  Int      *m_dequantCoef256 [SCALING_LIST_NUM][SCALING_LIST_REM_NUM][SCALING_LIST_DIR_NUM]; ///< array of dequantization matrix coefficient 16x16
  Int      *m_quantCoef1024  [SCALING_LIST_NUM][SCALING_LIST_REM_NUM];             ///< array of quantization matrix coefficient 32x32
  Int      *m_dequantCoef1024[SCALING_LIST_NUM][SCALING_LIST_REM_NUM];             ///< array of dequantization matrix coefficient 32x32
  double   *m_errScale       [SCALING_LIST_NUM][SCALING_LIST_REM_NUM];             ///< array of quantization matrix coefficient 4x4
  double   *m_errScale64     [SCALING_LIST_NUM][SCALING_LIST_REM_NUM][SCALING_LIST_DIR_NUM]; ///< array of quantization matrix coefficient 8x8
  double   *m_errScale256    [SCALING_LIST_NUM][SCALING_LIST_REM_NUM][SCALING_LIST_DIR_NUM]; ///< array of quantization matrix coefficient 16x16
  double   *m_errScale1024   [SCALING_LIST_NUM][SCALING_LIST_REM_NUM];             ///< array of quantization matrix coefficient 32x32
#endif  
private:
  // forward Transform
#if NSQT
  Void xT   ( UInt uiMode,Pel* pResidual, UInt uiStride, Int* plCoeff, Int iWidth, Int iHeight );
#else
  Void xT   ( UInt uiMode,Pel* pResidual, UInt uiStride, Int* plCoeff, Int iSize );
#endif
  
  // quantization
  Void xQuant( TComDataCU* pcCU, Int* pSrc, TCoeff* pDes, Int iWidth, Int iHeight, UInt& uiAcSum, TextType eTType, UInt uiAbsPartIdx );

  // RDOQ functions
#if !DISABLE_CAVLC
  Int            xCodeCoeffCountBitsLast(TCoeff* scoeff, levelDataStruct* levelData, Int nTab, UInt uiNoCoeff, Int iStartLast
                                        , Int isIntra
                                        );
  UInt           xCountVlcBits(UInt uiTableNumber, UInt uiCodeNumber);
  Int            bitCountRDOQ(Int coeff, Int pos, Int nTab, Int lastCoeffFlag,Int levelMode,Int run, Int maxrun, Int* vlc_adaptive, Int N, 
                              UInt uiTr1, Int iSum_big_coef, Int iBlockType, TComDataCU* pcCU, const UInt **pLumaRunTr1, Int iNextRun
                              , Int isIntra
                              );
  Void           xRateDistOptQuant_LCEC ( TComDataCU*                     pcCU,
                                          Int*                            plSrcCoeff,
                                          TCoeff*                         piDstCoeff,
                                          UInt                            uiWidth,
                                          UInt                            uiHeight,
                                          UInt&                           uiAbsSum,
                                          TextType                        eTType,
                                          UInt                            uiAbsPartIdx );
#endif
  
  Void           xRateDistOptQuant ( TComDataCU*                     pcCU,
                                     Int*                            plSrcCoeff,
                                     TCoeff*                         piDstCoeff,
                                     UInt                            uiWidth,
                                     UInt                            uiHeight,
                                     UInt&                           uiAbsSum,
                                     TextType                        eTType,
                                     UInt                            uiAbsPartIdx );
__inline UInt              xGetCodedLevel  ( Double&                         rd64CodedCost,
                                             Double&                         rd64CodedCost0,
                                             Double&                         rd64CodedCostSig,
                                             Int                             lLevelDouble,
                                             UInt                            uiMaxAbsLevel,
                                             UShort                          ui16CtxNumSig,
                                             UShort                          ui16CtxNumOne,
                                             UShort                          ui16CtxNumAbs,
                                             UShort                          ui16AbsGoRice,
                                             Int                             iQBits,
                                             Double                          dTemp,
                                             Bool                            bLast        ) const;
  __inline Double xGetICRateCost   ( UInt                            uiAbsLevel,
                                     UShort                          ui16CtxNumOne,
                                     UShort                          ui16CtxNumAbs,
                                     UShort                          ui16AbsGoRice ) const;
  __inline Double xGetRateLast     ( const UInt                      uiPosX,
                                     const UInt                      uiPosY,
                                     const UInt                      uiBlkWdth     ) const;
#if MULTI_LEVEL_SIGNIFICANCE
  __inline Double xGetRateSigCoeffGroup (  UShort                    uiSignificanceCoeffGroup,
                                     UShort                          ui16CtxNumSig ) const;
#endif
  __inline Double xGetRateSigCoef (  UShort                          uiSignificance,
                                     UShort                          ui16CtxNumSig ) const;
  __inline Double xGetICost        ( Double                          dRate         ) const; 
  __inline Double xGetIEPRate      (                                               ) const;
  
  
  // dequantization
#if SCALING_LIST
  Void xDeQuant( const TCoeff* pSrc, Int* pDes, Int iWidth, Int iHeight, Int scalingListType );
#else
  Void xDeQuant( const TCoeff* pSrc,     Int* pDes,       Int iWidth, Int iHeight );
#endif
  
  // inverse transform
#if NSQT
  Void xIT    ( UInt uiMode, Int* plCoef, Pel* pResidual, UInt uiStride, Int iWidth, Int iHeight );
#else
  Void xIT    ( UInt uiMode, Int* plCoef, Pel* pResidual, UInt uiStride, Int iSize );
#endif
  
};// END CLASS DEFINITION TComTrQuant

//! \}

#endif // __TCOMTRQUANT__
