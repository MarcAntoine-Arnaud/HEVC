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

/** \file     TComPrediction.h
    \brief    prediction class (header)
*/

#ifndef __TCOMPREDICTION__
#define __TCOMPREDICTION__


// Include files
#include "TComPic.h"
#include "TComMotionInfo.h"
#include "TComPattern.h"
#include "TComTrQuant.h"
#include "TComPredFilter.h"

#if HHI_INTERP_FILTER
#include "TComPredFilterMOMS.h"
#endif

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// prediction class
class TComPrediction : public TComPredFilter
#if HHI_INTERP_FILTER
                      ,public TComPredFilterMOMS
#endif
{
protected:
  Int*      m_piYuvExt;
  Int       m_iYuvExtStride;
  Int       m_iYuvExtHeight;
  Int       m_iDIFHalfTap;
#if SAMSUNG_CHROMA_IF_EXT
  Int		m_iDIFHalfTapC;
#endif

  TComYuv   m_acYuvPred[2];
  TComYuv   m_cYuvPredTemp;
  TComYuv   m_cYuvExt;
#ifdef DCM_PBIC
  TComYuv   m_acYuvTempIC[2];
#endif

#if PLANAR_INTRA
  Void xPredIntraPlanar         ( Int* pSrc, Int iSrcStride, Pel*& rpDst, Int iDstStride, UInt iWidth, UInt iHeight, Int iSampleBottomRight, Bool bAbove, Bool bLeft );
#endif

  Void xPredIntraAng            ( Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, UInt dirMode, Bool blkAboveAvailable, Bool blkLeftAvailable );

  // motion compensation functions
  Void xPredInterUni            ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx          );
  Void xPredInterBi             ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight,                         TComYuv*& rpcYuvPred, Int iPartIdx          );
  Void xPredInterLumaBlk        ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv );
  Void xPredInterChromaBlk      ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv                            );
#if TEN_DIRECTIONAL_INTERP
  Void xPredInterLumaBlk_TEN    ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv );
#if TEN_DIRECTIONAL_INTERP_CHROMA
  Void xPredInterChromaBlk_TEN  ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv                            );
#endif
#endif
  Void xWeightedAverage         ( TComDataCU* pcCU, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst );
#ifdef QC_AMVRES
  Void  xPredInterLumaBlkHMV ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv,Int iWidth, Int iHeight, TComYuv*& rpcYuv );
  Void  xPredInterLumaBlkHMVME ( Pel* piSrcY, Int iSrcStride, Pel* piDstY, Int iDstStride, TComMv* pcMv,Int iWidth, Int iHeight, Int dMVx, Int dMVy);
  Void  xHAM_Filter (Pel*  piRefY, Int iRefStride,Pel*  piDstY,Int iDstStride, Int iWidth, Int iHeight,Int iMVyFrac,Int iMVxFrac);
#endif
#ifdef QC_SIFO
  Void xSIFOFilter              (Pel*  piRefY, Int iRefStride,Pel*  piDstY,Int iDstStride, Int iWidth, Int iHeight,Int iyFrac,Int ixFrac);
  Void xPredInterLumaBlk_SIFOApplyME ( Pel* piSrcY, Int iSrcStride, Pel* piDstY, Int iDstStride, TComMv* pcMv, 
                                                   Int iWidth, Int iHeight, Pel* piOrg,Int iStrideOrg, Int dMVx, Int dMVy);
#endif

#ifdef DCM_PBIC
  Void xPredICompLumaBlk        (TComIc* pcIc, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel* piDst, Int iSrcStride, Int iSrcStep, Pel* piSrc, RefPicList eRefPicList);
  Void xPredICompChromaBlk      (TComIc* pcIc, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel* piDst, Int iSrcStride, Int iSrcStep, Pel* piSrc, RefPicList eRefPicList);
#endif

#if SAMSUNG_CHROMA_IF_EXT
  Void  xDCTIF_FilterC ( Pel*  piRefC, Int iRefStride,Pel*  piDstC,Int iDstStride, Int iWidth, Int iHeight,Int iMVyFrac,Int iMVxFrac);
#endif

public:
  TComPrediction();
  virtual ~TComPrediction();

  Void    initTempBuff();

  // inter
  Void motionCompensation         ( TComDataCU*  pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList = REF_PIC_LIST_X, Int iPartIdx = -1 );

  // motion vector prediction
  Void getMvPredAMVP              ( TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred );

#ifdef HHI_IMVP
  Void getMvPredIMVP( TComDataCU* pcSubCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefList, Int iRefIdx, TComCUMvField* pcSubCUMvField, TComMv& rcMvPred );
#ifdef QC_AMVRES
  Void getMvPredIMVP_onefourth( TComDataCU* pcSubCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefList, Int iRefIdx, TComCUMvField* pcSubCUMvField, TComMv& rcMvPred );
#endif
#endif

#ifdef DCM_PBIC
  Void getIcPredAICP              ( TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, TComIc& rcIcPred );
#endif

  // CIP
  Void recIntraLumaCIP            ( TComPattern* pcTComPattern, Pel* pPred, Pel* pResi, Pel* pReco, UInt uiStride, Int iWidth, Int iHeight, TComDataCU* pcCU, Bool bAboveAvail, Bool bLeftAvail );

  // Angular Intra
#if HHI_AIS
  Void predIntraLumaAng           ( TComPattern* pcTComPattern, UInt uiDirMode, Bool bSmoothing, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight,  TComDataCU* pcCU, Bool bAbove, Bool bLeft );
#else
  Void predIntraLumaAng           ( TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight,  TComDataCU* pcCU, Bool bAbove, Bool bLeft );
#endif
  Void predIntraChromaAng         ( TComPattern* pcTComPattern, Int* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, TComDataCU* pcCU, Bool bAbove, Bool bLeft );

  Pel  predIntraGetPredValDC      ( Int* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight, Bool bAbove, Bool bLeft );

#if PLANAR_INTRA
  Void predIntraPlanar            ( Int* piSrc, Int iDelta, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft );
#endif

  Int* getPredicBuf()             { return m_piYuvExt;      }
  Int  getPredicBufWidth()        { return m_iYuvExtStride; }
  Int  getPredicBufHeight()       { return m_iYuvExtHeight; }
};


#endif // __TCOMPREDICTION__

