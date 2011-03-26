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

/** \file     TDecCAVLC.h
    \brief    CAVLC decoder class (header)
*/

#ifndef __TDECCAVLC__
#define __TDECCAVLC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TDecEntropy.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CAVLC decoder class
class TDecCavlc : public TDecEntropyIf
{
public:
  TDecCavlc();
  virtual ~TDecCavlc();
  
protected:
  Void  xReadCode             (UInt uiLength, UInt& ruiCode);
  Void  xReadUvlc             (UInt& ruiVal);
  Void  xReadSvlc             (Int& riVal);
  Void  xReadFlag             (UInt& ruiCode);
  Void  xReadEpExGolomb     ( UInt& ruiSymbol, UInt uiCount );
  Void  xReadExGolombLevel  ( UInt& ruiSymbol );
  Void  xReadUnaryMaxSymbol ( UInt& ruiSymbol, UInt uiMaxSymbol );
  
  UInt  xGetBit             ();
  Int   xReadVlc            ( Int n );
  Void  xParseCoeff4x4      ( TCoeff* scoeff, Int iTableNumber );
  Void  xParseCoeff8x8      ( TCoeff* scoeff, Int iTableNumber );
#if QC_MOD_LCEC
  Void  xRunLevelIndInv     (LastCoeffStruct *combo, Int maxrun, UInt lrg1Pos, UInt cn);
#endif
  
private:
  TComBitstream*        m_pcBitstream;
  UInt                  m_uiCoeffCost;
  Bool                  m_bRunLengthCoding;
  UInt                  m_uiRun;
  Bool m_bAlfCtrl;
  UInt m_uiMaxAlfCtrlDepth;
  UInt                      m_uiLPTableD4[3][32];
  UInt                      m_uiLPTableD8[10][128];
  UInt                      m_uiLastPosVlcIndex[10];
  
#if LCEC_INTRA_MODE
  UInt                      m_uiIntraModeTableD17[16];
  UInt                      m_uiIntraModeTableD34[33];
#endif
#if QC_LCEC_INTER_MODE
  UInt                      m_uiSplitTableD[4][7];
#endif
  UInt                      m_uiCBPTableD[2][8];
  UInt                      m_uiCbpVlcIdx[2];
  
#if QC_BLK_CBP
  UInt                      m_uiBlkCBPTableD[2][15];
  UInt                      m_uiBlkCbpVlcIdx;
#endif
  
  Int                   m_iRefFrame0[1000];
  Int                   m_iRefFrame1[1000];
  Bool                  m_bMVres0[1000];
  Bool                  m_bMVres1[1000];
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  UInt                  m_uiMI1TableD[9];
#else
  UInt                  m_uiMI1TableD[8];
#endif
  UInt                  m_uiMI2TableD[15]; 
  UInt                  m_uiMITableVlcIdx;
  
public:
  Void  resetEntropy        ( TComSlice* pcSlice  );
  Void  setBitstream        ( TComBitstream* p    )      { m_pcBitstream = p; }
  Void  setAlfCtrl          ( Bool bAlfCtrl )            { m_bAlfCtrl = bAlfCtrl; }
  Void  setMaxAlfCtrlDepth  ( UInt uiMaxAlfCtrlDepth )  { m_uiMaxAlfCtrlDepth = uiMaxAlfCtrlDepth; }
  
  Void  parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize );
  Void  parseQtCbf          ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth );
  Void  parseQtRootCbf      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& uiQtRootCbf );
  Void  parseAlfFlag        ( UInt& ruiVal );
  Void  parseAlfUvlc        ( UInt& ruiVal );
  Void  parseAlfSvlc        ( Int&  riVal  );
  
  Void  parseNalUnitHeader  ( NalUnitType& eNalUnitType, UInt& TemporalId, Bool& bOutputFlag );
  
  Void  parseSPS            ( TComSPS* pcSPS );
  Void  parsePPS            ( TComPPS* pcPPS);
  Void  parseSliceHeader    ( TComSlice*& rpcSlice );
  Void  parseTerminatingBit ( UInt& ruiBit );
  
  Void  parseMVPIdx         ( TComDataCU* pcCU, Int& riMVPIdx, Int iMVPNum, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList );
  
  Void  parseSkipFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseMergeFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void parseMergeIndex      ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseSplitFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePartSize        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePredMode        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirLumaAng ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirChroma  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseInterDir        ( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseRefFrmIdx       ( TComDataCU* pcCU, Int& riRefFrmIdx,  UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList );
  Void parseMvd             ( TComDataCU* pcCU, UInt uiAbsPartAddr,UInt uiPartIdx,    UInt uiDepth, RefPicList eRefList );
  
  Void parseDeltaQP         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseCbf             ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth );
  Void parseBlockCbf        ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth, UInt uiQPartNum );
  Void parseCoeffNxN        ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType );
  
  Void parseAlfCtrlDepth    ( UInt& ruiAlfCtrlDepth );
  Void parseAlfCtrlFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if TSB_ALF_HEADER
  Void parseAlfFlagNum      ( UInt& ruiVal, UInt minValue, UInt depth );
  Void parseAlfCtrlFlag     ( UInt &ruiAlfCtrlFlag );
#endif
  
};
#endif // !defined(AFX_TDECCAVLC_H__9732DD64_59B0_4A41_B29E_1A5B18821EAD__INCLUDED_)

