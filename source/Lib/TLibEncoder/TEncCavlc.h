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

/** \file     TEncCavlc.h
    \brief    CAVLC encoder class (header)
*/

#ifndef __TENCCAVLC__
#define __TENCCAVLC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComBitStream.h"
#include "TEncEntropy.h"

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CAVLC encoder class
class TEncCavlc : public TEncEntropyIf
{
private:
  Bool m_bAdaptFlag;


public:
  TEncCavlc();
  virtual ~TEncCavlc();

protected:
  TComBitIf*    m_pcBitIf;
  TComSlice*    m_pcSlice;
  UInt          m_uiCoeffCost;
  Bool          m_bRunLengthCoding;
  UInt          m_uiRun;
  Bool          m_bAlfCtrl;
  UInt          m_uiMaxAlfCtrlDepth;
  UInt          m_uiLPTableE4[3][32];
  UInt          m_uiLPTableD4[3][32];
  UInt          m_uiLPTableE8[10][128];
  UInt          m_uiLPTableD8[10][128];
  UInt          m_uiLastPosVlcIndex[10];

#if LCEC_STAT 
  UInt m_uiBitHLS;
  UInt m_uiBitMVPId;
  UInt m_uiBitPartSize;
  UInt m_uiBitPredMode;
  UInt m_uiBitMergeFlag;
  UInt m_uiBitMergeIndex;
  UInt m_uiBitIntraFiltFlag;
  UInt m_uiBitAlfCtrlFlag;
  UInt m_uiBitAlfCtrlDepth;
  UInt m_uiBitSkipFlag;
  UInt m_uiBitCurrSplitFlag;
  UInt m_uiBitTransformSubdivFlag;
  UInt m_uiBitQtCbf;
  UInt m_uiBitTransformIdx;
  UInt m_uiBitPlanarVlc;
  UInt m_uiBitIntraDir;
  UInt m_uiBitIRefFrmIdx;
  UInt m_uiBitMVD;
  UInt m_uiBitDeltaQP;
  UInt m_uiBitCbf;
  UInt m_uiBitAlfFlag;
  UInt m_uiBitAlfUvlc;
  UInt m_uiBitAlfSvlc;
  UInt m_uiBitMVPIdx;
  UInt m_uiBitPlanarInfo;
  UInt m_uiBitInterDir;
  UInt m_uiBitMI;
  UInt m_uiBitSF;
  UInt m_uiBitCoeff;
  UInt m_uiBitCbp;
#endif

  UInt          m_uiCBPTableE[2][8];
  UInt          m_uiCBPTableD[2][8];
  UInt          m_uiCbpVlcIdx[2];

#if QC_BLK_CBP
  UInt          m_uiBlkCBPTableE[2][15];
  UInt          m_uiBlkCBPTableD[2][15];
  UInt          m_uiBlkCbpVlcIdx;
#endif

  UInt          m_uiMI1TableE[8];
  UInt          m_uiMI1TableD[8];
  UInt          m_uiMI2TableE[15];
  UInt          m_uiMI2TableD[15];

  UInt          m_uiMITableVlcIdx;

  Void  xCheckCoeff( TCoeff* pcCoef, UInt uiSize, UInt uiDepth, UInt& uiNumofCoeff, UInt& uiPart );

#if LCEC_STAT
  UInt  xWriteCode            ( UInt uiCode, UInt uiLength );
  UInt  xWriteUvlc            ( UInt uiCode );
  UInt  xWriteSvlc            ( Int iCode   );
  Void  xWriteFlag            ( UInt uiCode );
  UInt  xWriteEpExGolomb      ( UInt uiSymbol, UInt uiCount );
  UInt  xWriteExGolombLevel   ( UInt uiSymbol );
  UInt  xWriteUnaryMaxSymbol  ( UInt uiSymbol, UInt uiMaxSymbol );
  UInt  xLeadingZeros         ( UInt uiCode );
  UInt  xWriteVlc             ( UInt uiTableNumber, UInt uiCodeNumber );
#else
  Void  xWriteCode            ( UInt uiCode, UInt uiLength );
  Void  xWriteUvlc            ( UInt uiCode );
  Void  xWriteSvlc            ( Int iCode   );
  Void  xWriteFlag            ( UInt uiCode );
  Void  xWriteEpExGolomb      ( UInt uiSymbol, UInt uiCount );
  Void  xWriteExGolombLevel    ( UInt uiSymbol );
  Void  xWriteUnaryMaxSymbol  ( UInt uiSymbol, UInt uiMaxSymbol );
  UInt  xLeadingZeros         ( UInt uiCode );
  Void  xWriteVlc             ( UInt uiTableNumber, UInt uiCodeNumber );
#endif
  Void  xCodeCoeff4x4          ( TCoeff* scoeff, Int iTableNumber );
  Void  xCodeCoeff8x8          ( TCoeff* scoeff, Int iTableNumber );

  UInt  xConvertToUInt        ( Int iValue ) {  return ( iValue <= 0) ? -iValue<<1 : (iValue<<1)-1; }

public:

  Void  resetEntropy          ();
#if LCEC_STAT
  Void  statistics            ( Bool bResetFlag, UInt uiPrintVar );
#endif
  UInt* GetLP8Table();
  UInt* GetLP4Table();
  Void  setBitstream          ( TComBitIf* p )  { m_pcBitIf = p;  }
  Void  setSlice              ( TComSlice* p )  { m_pcSlice = p;  }
  Bool getAlfCtrl() {return m_bAlfCtrl;}
  UInt getMaxAlfCtrlDepth() {return m_uiMaxAlfCtrlDepth;}
  Void setAlfCtrl(Bool bAlfCtrl) {m_bAlfCtrl = bAlfCtrl;}
  Void setMaxAlfCtrlDepth(UInt uiMaxAlfCtrlDepth) {m_uiMaxAlfCtrlDepth = uiMaxAlfCtrlDepth;}
  Void  resetBits             ()                { m_pcBitIf->resetBits(); }
  Void  resetCoeffCost        ()                { m_uiCoeffCost = 0;  }
  UInt  getNumberOfWrittenBits()                { return  m_pcBitIf->getNumberOfWrittenBits();  }
  UInt  getCoeffCost          ()                { return  m_uiCoeffCost;  }

  Void  codeSPS                 ( TComSPS* pcSPS );
  Void  codePPS                 ( TComPPS* pcPPS );
  Void  codeSliceHeader         ( TComSlice* pcSlice );
  Void  codeTerminatingBit      ( UInt uilsLast );
  Void  codeSliceFinish         ();

  Void codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void codeAlfFlag       ( UInt uiCode );
  Void codeAlfUvlc       ( UInt uiCode );
  Void codeAlfSvlc       ( Int   iCode );
  Void codeAlfCtrlDepth();

  Void codeSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if HHI_MRG
  Void codeMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  Void codeAlfCtrlFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if TSB_ALF_HEADER
  Void codeAlfFlagNum    ( UInt uiCode, UInt minValue );
  Void codeAlfCtrlFlag   ( UInt uiSymbol );
#endif

  Void codeSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

  Void codePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void codePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx );

  Void codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx );
  Void codeQtCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth );
  Void codeQtRootCbf     ( TComDataCU* pcCU, UInt uiAbsPartIdx );

  Void codeTransformIdx  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx );

  Void codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeInterDir      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeRefFrmIdx     ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void codeMvd           ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );

  Void codeDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if LCEC_CBP_YUV_ROOT
  Void codeCbf           ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth );
  Void	codeBlockCbf				  ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiQPartNum, Bool bRD = false);
#endif

  Void codeCoeffNxN      ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType, Bool bRD = false );

  Void estBit             (estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType);

  Bool  getAdaptFlag          ()          { return m_bAdaptFlag; }
  Void  setAdaptFlag          ( Bool b )  { m_bAdaptFlag = b;     }
};

#endif // !defined(AFX_TENCCAVLC_H__EE8A0B30_945B_4169_B290_24D3AD52296F__INCLUDED_)

