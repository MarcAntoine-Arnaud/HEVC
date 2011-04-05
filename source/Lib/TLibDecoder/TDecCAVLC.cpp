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

/** \file     TDecCAVLC.cpp
    \brief    CAVLC decoder class
*/

#include "TDecCAVLC.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCavlc::TDecCavlc()
{
  m_bAlfCtrl = false;
  m_uiMaxAlfCtrlDepth = 0;
}

TDecCavlc::~TDecCavlc()
{
  
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void  TDecCavlc::parseNalUnitHeader ( NalUnitType& eNalUnitType, UInt& TemporalId, Bool& bOutputFlag )
{
  UInt  uiCode;
  
  xReadCode ( 1, uiCode ); assert( 0 == uiCode); // forbidden_zero_bit
  xReadCode ( 2, uiCode );                       // nal_ref_idc
  xReadCode ( 5, uiCode );                       // nal_unit_type
  eNalUnitType = (NalUnitType) uiCode;

  if ( (eNalUnitType == NAL_UNIT_CODED_SLICE) || (eNalUnitType == NAL_UNIT_CODED_SLICE_IDR) || (eNalUnitType == NAL_UNIT_CODED_SLICE_CDR) )
  {
    xReadCode(3, uiCode); // temporal_id
    TemporalId = uiCode;
    xReadFlag(uiCode);    // output_flag
    bOutputFlag = (0!=uiCode);
    xReadCode(4, uiCode); // reserved_one_4bits    
  }
  else
  {
    TemporalId = 0;
    bOutputFlag = true;
  }
}

Void TDecCavlc::parsePPS(TComPPS* pcPPS)
{
  UInt  uiCode;
  
#if CONSTRAINED_INTRA_PRED
  xReadFlag ( uiCode ); pcPPS->setConstrainedIntraPred( uiCode ? true : false );
#endif

  return;
}

Void TDecCavlc::parseSPS(TComSPS* pcSPS)
{
  UInt  uiCode;
  
  // Structure
  xReadUvlc ( uiCode ); pcSPS->setWidth       ( uiCode    );
  xReadUvlc ( uiCode ); pcSPS->setHeight      ( uiCode    );
  xReadUvlc ( uiCode ); pcSPS->setPadX        ( uiCode    );
  xReadUvlc ( uiCode ); pcSPS->setPadY        ( uiCode    );
  
  xReadUvlc ( uiCode ); 
  pcSPS->setMaxCUWidth  ( uiCode    ); g_uiMaxCUWidth  = uiCode;
  pcSPS->setMaxCUHeight ( uiCode    ); g_uiMaxCUHeight = uiCode;
  
  xReadUvlc ( uiCode ); 
  pcSPS->setMaxCUDepth  ( uiCode+1  ); g_uiMaxCUDepth  = uiCode + 1;
  UInt uiMaxCUDepthCorrect = uiCode;
  
  xReadUvlc( uiCode ); pcSPS->setQuadtreeTULog2MinSize( uiCode + 2 );
  xReadUvlc( uiCode ); pcSPS->setQuadtreeTULog2MaxSize( uiCode + pcSPS->getQuadtreeTULog2MinSize() );
  pcSPS->setMaxTrSize( 1<<(uiCode + pcSPS->getQuadtreeTULog2MinSize()) );  
  xReadUvlc ( uiCode ); pcSPS->setQuadtreeTUMaxDepthInter( uiCode+1 );
  xReadUvlc ( uiCode ); pcSPS->setQuadtreeTUMaxDepthIntra( uiCode+1 );
  g_uiAddCUDepth = 0;
  while( ( pcSPS->getMaxCUWidth() >> uiMaxCUDepthCorrect ) > ( 1 << ( pcSPS->getQuadtreeTULog2MinSize() + g_uiAddCUDepth )  ) ) g_uiAddCUDepth++;    
  pcSPS->setMaxCUDepth( uiMaxCUDepthCorrect+g_uiAddCUDepth  ); g_uiMaxCUDepth  = uiMaxCUDepthCorrect+g_uiAddCUDepth;
  // BB: these parameters may be removed completly and replaced by the fixed values
  pcSPS->setMinTrDepth( 0 );
  pcSPS->setMaxTrDepth( 1 );
  
  // Tool on/off
  xReadFlag( uiCode ); pcSPS->setUseALF ( uiCode ? true : false );
  xReadFlag( uiCode ); pcSPS->setUseDQP ( uiCode ? true : false );
  xReadFlag( uiCode ); pcSPS->setUseLDC ( uiCode ? true : false );
  xReadFlag( uiCode ); pcSPS->setUseMRG ( uiCode ? true : false ); // SOPH:
  
#if HHI_RMP_SWITCH
  xReadFlag( uiCode ); pcSPS->setUseRMP( uiCode ? true : false );
#endif
  
  // AMVP mode for each depth (AM_NONE or AM_EXPL)
  for (Int i = 0; i < pcSPS->getMaxCUDepth(); i++)
  {
    xReadFlag( uiCode );
    pcSPS->setAMVPMode( i, (AMVP_MODE)uiCode );
  }
  
  // Bit-depth information
#if FULL_NBIT
  xReadUvlc( uiCode );
  g_uiBitDepth = 8 + uiCode;
  g_uiBitIncrement = 0;
  pcSPS->setBitDepth(g_uiBitDepth);
  pcSPS->setBitIncrement(g_uiBitIncrement);
#else
#if ENABLE_IBDI
  xReadUvlc( uiCode ); pcSPS->setBitDepth     ( uiCode+8 ); g_uiBitDepth     = uiCode + 8;
  xReadUvlc( uiCode ); pcSPS->setBitIncrement ( uiCode   ); g_uiBitIncrement = uiCode;
#else
  xReadUvlc( uiCode );
  g_uiBitDepth = 8;
  g_uiBitIncrement = uiCode;
  pcSPS->setBitDepth(g_uiBitDepth);
  pcSPS->setBitIncrement(g_uiBitIncrement);
#endif
#endif
  
  g_uiBASE_MAX  = ((1<<(g_uiBitDepth))-1);
  
#if IBDI_NOCLIP_RANGE
  g_uiIBDI_MAX  = g_uiBASE_MAX << g_uiBitIncrement;
#else
  g_uiIBDI_MAX  = ((1<<(g_uiBitDepth+g_uiBitIncrement))-1);
#endif
#if MTK_NONCROSS_INLOOP_FILTER
  xReadFlag( uiCode );
  pcSPS->setLFCrossSliceBoundaryFlag( uiCode ? true : false);
#endif

  return;
}

Void TDecCavlc::parseSliceHeader (TComSlice*& rpcSlice)
{
  UInt  uiCode;
  Int   iCode;

  xReadFlag ( uiCode );
  Bool bEntropySlice = uiCode ? true : false;
  if (!bEntropySlice)
  {
    xReadCode (10, uiCode);  rpcSlice->setPOC              (uiCode);             // 9 == SPS->Log2MaxFrameNum()
    xReadUvlc (   uiCode);  rpcSlice->setSliceType        ((SliceType)uiCode);
    xReadSvlc (    iCode);  rpcSlice->setSliceQp          (iCode);
  }
  if (bEntropySlice)
  {
    rpcSlice->setNextSlice        ( false );
    rpcSlice->setNextEntropySlice ( true  );

    xReadUvlc(uiCode);
    rpcSlice->setEntropySliceCurStartCUAddr( uiCode ); // start CU addr for entropy slice
  }
  else
  {
    rpcSlice->setNextSlice        ( true  );
    rpcSlice->setNextEntropySlice ( false );
    
    xReadUvlc(uiCode);
    rpcSlice->setSliceCurStartCUAddr( uiCode );        // start CU addr for slice
    rpcSlice->setEntropySliceCurStartCUAddr( uiCode ); // start CU addr for entropy slice  
    
    xReadFlag ( uiCode );
    rpcSlice->setSymbolMode( uiCode );
    
    if (!rpcSlice->isIntra())
      xReadFlag (   uiCode);
    else
      uiCode = 1;
    
    rpcSlice->setReferenced       (uiCode ? true : false);
    
#if !HIGH_ACCURACY_BI
#ifdef ROUNDING_CONTROL_BIPRED
    if(!rpcSlice->isIntra())
    {
      xReadFlag( uiCode );
      Bool b = (uiCode != 0);
      rpcSlice->setRounding(b);
    }
#endif
#else
    if(!rpcSlice->isIntra())
    {
      rpcSlice->setRounding(false);
    }
#endif
    
    xReadFlag (   uiCode);  rpcSlice->setLoopFilterDisable(uiCode ? 1 : 0);
    
    if (!rpcSlice->isIntra())
    {
      xReadCode (3, uiCode);  rpcSlice->setNumRefIdx      (REF_PIC_LIST_0, uiCode);
    }
    else
    {
      rpcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
    }
    if (rpcSlice->isInterB())
    {
      xReadCode (3, uiCode);  rpcSlice->setNumRefIdx      (REF_PIC_LIST_1, uiCode);
    }
    else
    {
      rpcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
    }
    
#if DCM_COMB_LIST
    if (rpcSlice->isInterB())
    {
      xReadFlag (uiCode);      rpcSlice->setRefPicListCombinationFlag(uiCode ? 1 : 0);
      if(uiCode)
      {
        xReadUvlc(uiCode);      rpcSlice->setNumRefIdx      (REF_PIC_LIST_C, uiCode+1);
        
        xReadFlag (uiCode);     rpcSlice->setRefPicListModificationFlagLC(uiCode ? 1 : 0);
        if(uiCode)
        {
          for (UInt i=0;i<rpcSlice->getNumRefIdx(REF_PIC_LIST_C);i++)
          {
            xReadFlag(uiCode);
            rpcSlice->setListIdFromIdxOfLC(i, uiCode);
            xReadUvlc(uiCode);
            rpcSlice->setRefIdxFromIdxOfLC(i, uiCode);
            rpcSlice->setRefIdxOfLC((RefPicList)rpcSlice->getListIdFromIdxOfLC(i), rpcSlice->getRefIdxFromIdxOfLC(i), i);
          }
        }
      }
      else
      {
        rpcSlice->setRefPicListCombinationFlag(false);
        rpcSlice->setRefPicListModificationFlagLC(false);
        rpcSlice->setNumRefIdx(REF_PIC_LIST_C, -1);
      }
    }
#endif
    
    xReadFlag (uiCode);     rpcSlice->setDRBFlag          (uiCode ? 1 : 0);
    if ( !rpcSlice->getDRBFlag() )
    {
      xReadCode(2, uiCode); rpcSlice->setERBIndex( (ERBIndex)uiCode );    assert (uiCode == ERB_NONE || uiCode == ERB_LTR);
    }  
    
#if AMVP_NEIGH_COL
    if ( rpcSlice->getSliceType() == B_SLICE )
    {
      xReadFlag (uiCode);
      rpcSlice->setColDir(uiCode);
    }
#endif
  }
  return;
}

Void TDecCavlc::resetEntropy          (TComSlice* pcSlice)
{
  m_bRunLengthCoding = ! pcSlice->isIntra();
  m_uiRun = 0;
  
  ::memcpy(m_uiLPTableD8,        g_auiLPTableD8,        10*128*sizeof(UInt));
  ::memcpy(m_uiLPTableD4,        g_auiLPTableD4,        3*32*sizeof(UInt));
  ::memcpy(m_uiLastPosVlcIndex,  g_auiLastPosVlcIndex,  10*sizeof(UInt));
  
  ::memcpy(m_uiCBPTableD,        g_auiCBPTableD,        2*8*sizeof(UInt));
  m_uiCbpVlcIdx[0] = 0;
  m_uiCbpVlcIdx[1] = 0;
  
  ::memcpy(m_uiBlkCBPTableD,     g_auiBlkCBPTableD,     2*15*sizeof(UInt));
  m_uiBlkCbpVlcIdx = 0;
  
  ::memcpy(m_uiMI1TableD,        g_auiMI1TableD,        8*sizeof(UInt));
  ::memcpy(m_uiMI2TableD,        g_auiMI2TableD,        15*sizeof(UInt));
  
#if DCM_COMB_LIST
  if ( pcSlice->getNoBackPredFlag() || pcSlice->getNumRefIdx(REF_PIC_LIST_C)>0)
#else
  if ( pcSlice->getNoBackPredFlag() )
#endif
  {
    ::memcpy(m_uiMI1TableD,        g_auiMI1TableDNoL1,        8*sizeof(UInt));
    ::memcpy(m_uiMI2TableD,        g_auiMI2TableDNoL1,        15*sizeof(UInt));
  }
  
#if MS_LCEC_ONE_FRAME
  if ( pcSlice->getNumRefIdx(REF_PIC_LIST_0) <= 1 && pcSlice->getNumRefIdx(REF_PIC_LIST_1) <= 1 )
  {
    if ( pcSlice->getNoBackPredFlag() || ( pcSlice->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcSlice->getNumRefIdx(REF_PIC_LIST_C) <= 1 ) )
    {
      ::memcpy(m_uiMI1TableD,        g_auiMI1TableDOnly1RefNoL1,        8*sizeof(UInt));
    }
    else
    {
      ::memcpy(m_uiMI1TableD,        g_auiMI1TableDOnly1Ref,        8*sizeof(UInt));
    }
  }
#endif
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  if (pcSlice->getNumRefIdx(REF_PIC_LIST_C)>0)
  {
    m_uiMI1TableD[8] = 8;
  }
  else  // GPB case
  {
    m_uiMI1TableD[8] = m_uiMI1TableD[6];
    m_uiMI1TableD[6] = 8;
  }
#endif
  
#if LCEC_INTRA_MODE
  ::memcpy(m_uiIntraModeTableD17, g_auiIntraModeTableD17, 16*sizeof(UInt));
  ::memcpy(m_uiIntraModeTableD34, g_auiIntraModeTableD34, 33*sizeof(UInt));
#endif
#if QC_LCEC_INTER_MODE 
  ::memcpy(m_uiSplitTableD, g_auiInterModeTableD, 4*7*sizeof(UInt));
#endif 
  m_uiMITableVlcIdx = 0;
}

Void TDecCavlc::parseTerminatingBit( UInt& ruiBit )
{
  ruiBit = false;
  Int iBitsLeft = m_pcBitstream->getBitsLeft();
  if(iBitsLeft <= 8)
  {
    UInt uiPeekValue = m_pcBitstream->peekBits(iBitsLeft);
    if (uiPeekValue == (1<<(iBitsLeft-1)))
      ruiBit = true;
  }
}

Void TDecCavlc::parseAlfCtrlDepth              ( UInt& ruiAlfCtrlDepth )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, g_uiMaxCUDepth-1);
  ruiAlfCtrlDepth = uiSymbol;
}

Void TDecCavlc::parseAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if (!m_bAlfCtrl)
    return;
  
  if( uiDepth > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, m_uiMaxAlfCtrlDepth))
  {
    return;
  }
  
  UInt uiSymbol;
  xReadFlag( uiSymbol );
  
  if (uiDepth > m_uiMaxAlfCtrlDepth)
  {
    pcCU->setAlfCtrlFlagSubParts( uiSymbol, uiAbsPartIdx, m_uiMaxAlfCtrlDepth);
  }
  else
  {
    pcCU->setAlfCtrlFlagSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
  }
}

Void TDecCavlc::parseSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if QC_LCEC_INTER_MODE
  return;
#else
  
  if( pcCU->getSlice()->isIntra() )
  {
    return;
  }
  
  UInt uiSymbol = 0;
  xReadFlag( uiSymbol );
  
  if( uiSymbol )
  {
    pcCU->setPredModeSubParts( MODE_SKIP,  uiAbsPartIdx, uiDepth );
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
#if HHI_MRG_SKIP
    pcCU->setMergeFlagSubParts( true , uiAbsPartIdx, 0, uiDepth );
#else
    TComMv cZeroMv(0,0);
    pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    
    pcCU->setTrIdxSubParts( 0, uiAbsPartIdx, uiDepth );
    pcCU->setCbfSubParts  ( 0, 0, 0, uiAbsPartIdx, uiDepth );
    
    if ( pcCU->getSlice()->isInterP() )
    {
      pcCU->setInterDirSubParts( 1, uiAbsPartIdx, 0, uiDepth );
      
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
        pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllRefIdx(  0, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( NOT_VALID, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    }
    else
    {
      pcCU->setInterDirSubParts( 3, uiAbsPartIdx, 0, uiDepth );
      
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
        pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllRefIdx(  0, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( 0, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    }
  }
#endif // HHI_MRG_SKIP
#endif // QC_LCEC_INTER_MODE
}

/** parse the motion vector predictor index
 * \param pcCU
 * \param riMVPIdx 
 * \param iMVPNum
 * \param uiAbsPartIdx
 * \param uiDepth
 * \param eRefList
 * \returns Void
 */
Void TDecCavlc::parseMVPIdx( TComDataCU* pcCU, Int& riMVPIdx, Int iMVPNum, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, iMVPNum-1);
  riMVPIdx = uiSymbol;
}
#if QC_LCEC_INTER_MODE
/** parse the split flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecCavlc::parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if (pcCU->getSlice()->isIntra())
  {
    if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    {
      pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
      return ;
    }

    UInt uiSymbol;
    xReadFlag( uiSymbol );
    pcCU->setDepthSubParts( uiDepth + uiSymbol, uiAbsPartIdx );

    return ;
  }
  UInt tmp=0;
  UInt cx=0;
  UInt uiMode ;
  {
    UInt iMaxLen= (uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth)?5:6;
    while (tmp==0 && cx<iMaxLen)
    {
      xReadFlag( tmp );
      cx++;
    };
    if(tmp!=0)
      cx--;

    UInt uiDepthRemember = uiDepth;
    if ( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    {
      uiDepth = 3;
    }
    UInt x = m_uiSplitTableD[uiDepth][cx];
    /* Adapt table */
    uiMode = x;
    if (cx>0)
    {    
      UInt cy = Max(0,cx-1);
      UInt y = m_uiSplitTableD[uiDepth][cy];
      m_uiSplitTableD[uiDepth][cy] = x;
      m_uiSplitTableD[uiDepth][cx] = y;
    }
    uiDepth = uiDepthRemember;
  }
  if (uiMode==0)
  {
    pcCU->setDepthSubParts( uiDepth + 1, uiAbsPartIdx );
  }
  else
  {
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setMergeFlagSubParts(false, uiAbsPartIdx,0, uiDepth );
    pcCU->setDepthSubParts( uiDepth    , uiAbsPartIdx );
    if (uiMode ==1)
    {
      TComMv cZeroMv(0,0);
      pcCU->setPredModeSubParts( MODE_SKIP,  uiAbsPartIdx, uiDepth );
      pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
      pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
      pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
      pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
      pcCU->setTrIdxSubParts( 0, uiAbsPartIdx, uiDepth );
      pcCU->setCbfSubParts  ( 0, 0, 0, uiAbsPartIdx, uiDepth );
      
#if HHI_MRG_SKIP
      pcCU->setMergeFlagSubParts( true, uiAbsPartIdx, 0, uiDepth );
#else
      if ( pcCU->getSlice()->isInterP() )
      {
        pcCU->setInterDirSubParts( 1, uiAbsPartIdx, 0, uiDepth );
        
        if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
          pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllRefIdx(  0, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
        if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
          pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( NOT_VALID, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
      }
      else
      {
        pcCU->setInterDirSubParts( 3, uiAbsPartIdx, 0, uiDepth );
        
        if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
          pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllRefIdx(  0, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
        if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
          pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( 0, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
      }
#endif
    }
    else if (uiMode==2)
    {
      pcCU->setPredModeSubParts( MODE_INTER, uiAbsPartIdx, uiDepth );
      pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
      pcCU->setMergeFlagSubParts(true, uiAbsPartIdx,0, uiDepth );
      pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    }
    else if (uiMode==6)
    {
#if MTK_DISABLE_INTRA_NxN_SPLIT && HHI_DISABLE_INTER_NxN_SPLIT 
      if (uiDepth != g_uiMaxCUDepth - g_uiAddCUDepth)
      {
        pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
        pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
        pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
        UInt uiTrLevel = 0;
        UInt uiWidthInBit  = g_aucConvertToBit[pcCU->getWidth(uiAbsPartIdx)]+2;
        UInt uiTrSizeInBit = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxTrSize()]+2;
        uiTrLevel          = uiWidthInBit >= uiTrSizeInBit ? uiWidthInBit - uiTrSizeInBit : 0;
        pcCU->setTrIdxSubParts( uiTrLevel, uiAbsPartIdx, uiDepth );       
      }
      else
#endif
      {
        pcCU->setPredModeSubParts( MODE_INTER, uiAbsPartIdx, uiDepth );
        pcCU->setPartSizeSubParts( SIZE_NxN, uiAbsPartIdx, uiDepth );
      }
    }
    else
    {
      pcCU->setPredModeSubParts( MODE_INTER, uiAbsPartIdx, uiDepth );
      pcCU->setPartSizeSubParts( PartSize(uiMode-3), uiAbsPartIdx, uiDepth );
      pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    }
  }
}
#else
Void TDecCavlc::parseSplitFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
  {
    pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
    return ;
  }
  
  UInt uiSymbol;
  xReadFlag( uiSymbol );
  pcCU->setDepthSubParts( uiDepth + uiSymbol, uiAbsPartIdx );
  
  return ;
}
#endif
#if QC_LCEC_INTER_MODE

/** parse partition size
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecCavlc::parsePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if !HHI_DIRECT_CLEANUP
  if ( pcCU->isSkip( uiAbsPartIdx ))
  {
    return ;
  }
#endif

  UInt uiMode=0;
  if ( pcCU->getSlice()->isIntra()&& pcCU->isIntra( uiAbsPartIdx ) )
  {
#if MTK_DISABLE_INTRA_NxN_SPLIT
    uiMode = 1;
    if ( uiDepth == (g_uiMaxCUDepth - g_uiAddCUDepth ))
#endif
    {
      UInt uiSymbol;
      xReadFlag( uiSymbol );
      uiMode = uiSymbol ? 1 : 2;
    }
  }
#if MTK_DISABLE_INTRA_NxN_SPLIT && HHI_DISABLE_INTER_NxN_SPLIT 
  else if (uiDepth != (g_uiMaxCUDepth - g_uiAddCUDepth ) || pcCU->getPartitionSize(uiAbsPartIdx ) != SIZE_NxN)
#else
  else if (pcCU->getPartitionSize(uiAbsPartIdx ) != SIZE_NxN)
#endif
  { 
    return;
  }
  else
  {
    UInt uiSymbol;
    xReadFlag( uiSymbol );
    if(uiSymbol)
    {
      uiMode = 1;
    }
    else
    {
#if (MTK_DISABLE_INTRA_NxN_SPLIT && !HHI_DISABLE_INTER_NxN_SPLIT) || (!MTK_DISABLE_INTRA_NxN_SPLIT && HHI_DISABLE_INTER_NxN_SPLIT )
      if ( uiDepth != (g_uiMaxCUDepth - g_uiAddCUDepth ))
      {
#if MTK_DISABLE_INTRA_NxN_SPLIT && !HHI_DISABLE_INTER_NxN_SPLIT 
        uiMode = 0;
#elif !MTK_DISABLE_INTRA_NxN_SPLIT && HHI_DISABLE_INTER_NxN_SPLIT 
        uiMode = 2;
#endif
      }
      else
#endif
      {
        xReadFlag( uiSymbol );
        uiMode = uiSymbol ? 2 : 0;
      }
    }
  }
  PartSize ePartSize;
  PredMode eMode;
  if (uiMode > 0)
  {
    eMode = MODE_INTRA;
    ePartSize = (uiMode==1) ? SIZE_2Nx2N:SIZE_NxN;
  }
  else
  {
    eMode = MODE_INTER;
    ePartSize = SIZE_NxN;
  }
  pcCU->setPredModeSubParts( eMode    , uiAbsPartIdx, uiDepth );
  pcCU->setPartSizeSubParts( ePartSize, uiAbsPartIdx, uiDepth );
  pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );

  if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA )
  {
    UInt uiTrLevel = 0;
    UInt uiWidthInBit  = g_aucConvertToBit[pcCU->getWidth(uiAbsPartIdx)] + 2;
    UInt uiTrSizeInBit = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxTrSize()] + 2;
    uiTrLevel          = uiWidthInBit >= uiTrSizeInBit ? uiWidthInBit - uiTrSizeInBit : 0;
    if( pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN )
    {
      pcCU->setTrIdxSubParts( 1 + uiTrLevel, uiAbsPartIdx, uiDepth );
    }
    else
    {
      pcCU->setTrIdxSubParts( uiTrLevel, uiAbsPartIdx, uiDepth );
    }
  }
}
#else
Void TDecCavlc::parsePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if !HHI_DIRECT_CLEANUP
  if ( pcCU->isSkip( uiAbsPartIdx ) )
  {
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    return ;
  }
#endif

  
  UInt uiSymbol, uiMode = 0;
  PartSize eMode;
  
  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
#if MTK_DISABLE_INTRA_NxN_SPLIT
    eMode = SIZE_2Nx2N;
    if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
    {
      xReadFlag( uiSymbol );
      eMode = uiSymbol ? SIZE_2Nx2N : SIZE_NxN;
    }
  }
  else
  {
#if HHI_RMP_SWITCH
    if ( !pcCU->getSlice()->getSPS()->getUseRMP())
    {
      xReadFlag( uiSymbol );
      if( uiSymbol )
        uiMode = 0;
      else
        uiMode = 3;
    }
    else
#endif
    {
      UInt uiMaxNumBits = 3;
      for ( UInt ui = 0; ui < uiMaxNumBits; ui++ )
      {
        xReadFlag( uiSymbol );
        if ( uiSymbol )
        {
          break;
        }
        uiMode++;
      }
    }
    eMode = (PartSize) uiMode;
    
    if (pcCU->getSlice()->isInterB() && uiMode == 3)
    {
#if HHI_DISABLE_INTER_NxN_SPLIT
      uiSymbol = 0;
      if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
      {
        xReadFlag( uiSymbol );
      }
      
      if (uiSymbol == 0)
      {
        pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
#if MTK_DISABLE_INTRA_NxN_SPLIT
        if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
        {
          xReadFlag( uiSymbol );
        }
        if (uiSymbol == 0)
          eMode = SIZE_2Nx2N;
      }
    }
    
  }
  
  pcCU->setPartSizeSubParts( eMode, uiAbsPartIdx, uiDepth );
  pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
  
  UInt uiTrLevel = 0;
  
  UInt uiWidthInBit  = g_aucConvertToBit[pcCU->getWidth(uiAbsPartIdx)]+2;
  UInt uiTrSizeInBit = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxTrSize()]+2;
  uiTrLevel          = uiWidthInBit >= uiTrSizeInBit ? uiWidthInBit - uiTrSizeInBit : 0;
  
  if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA )
  {
    if( pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN )
    {
      pcCU->setTrIdxSubParts( 1+uiTrLevel, uiAbsPartIdx, uiDepth );
    }
    else
    {
      pcCU->setTrIdxSubParts( uiTrLevel, uiAbsPartIdx, uiDepth );
    }
  }
}
#endif

/** parse prediction mode
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecCavlc::parsePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( pcCU->getSlice()->isIntra() )
  {
    pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
    return ;
  }
#if !QC_LCEC_INTER_MODE  
  UInt uiSymbol;
  Int  iPredMode = MODE_INTER;
 
  if ( pcCU->getSlice()->isInterB() )
  {
    pcCU->setPredModeSubParts( (PredMode)iPredMode, uiAbsPartIdx, uiDepth );
    return;
  }
#if HHI_DIRECT_CLEANUP
  xReadFlag( uiSymbol );
  iPredMode += uiSymbol;
#else
  if ( iPredMode != MODE_SKIP )
  {
    xReadFlag( uiSymbol );
    iPredMode += uiSymbol;
  }
#endif

  
  pcCU->setPredModeSubParts( (PredMode)iPredMode, uiAbsPartIdx, uiDepth );
#endif
}

#if LCEC_INTRA_MODE
Void TDecCavlc::parseIntraDirLumaAng  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  Int  uiIPredMode;
  Int  iMostProbable = pcCU->getMostProbableIntraDirLuma( uiAbsPartIdx );
  Int  iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
  Int  iDir, iDirLarger, iRankIntraMode, iRankIntraModeLarger;

  Int  iLeft          = pcCU->getLeftIntraDirLuma( uiAbsPartIdx );
  Int  iAbove         = pcCU->getAboveIntraDirLuma( uiAbsPartIdx );
  UInt ind=(iLeft==iAbove)? 0 : 1;

  const UInt *huff17=huff17_2[ind];
  const UInt *lengthHuff17=lengthHuff17_2[ind];
  const UInt *huff34=huff34_2[ind];
  const UInt *lengthHuff34=lengthHuff34_2[ind];

  if ( g_aucIntraModeBitsAng[iIntraIdx] < 5 )
  {
    xReadFlag( uiSymbol );
    if ( uiSymbol )
      uiIPredMode = iMostProbable;
    else
    {
      xReadFlag( uiSymbol ); uiIPredMode  = uiSymbol;
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) { xReadFlag( uiSymbol ); uiIPredMode |= uiSymbol << 1; }
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) { xReadFlag( uiSymbol ); uiIPredMode |= uiSymbol << 2; }
      if(uiIPredMode >= iMostProbable) 
        uiIPredMode ++;
    }
  }
  else if ( g_aucIntraModeBitsAng[iIntraIdx] == 5 )
  {
    UInt uiCode;
    UInt uiLength = lengthHuff17[15];
    m_pcBitstream->pseudoRead(uiLength,uiCode);
    if ((uiCode>>(uiLength- lengthHuff17[0])) == huff17[0])
    {
      m_pcBitstream->read(lengthHuff17[0],uiCode);
      uiIPredMode = iMostProbable;
    }
    else
    {
      iRankIntraMode = 0;
      for (Int i=1;i<17;i++)
      {
        if( (uiCode>>(uiLength- lengthHuff17[i])) == huff17[i])
        {
          m_pcBitstream->read(lengthHuff17[i], uiCode);
          iRankIntraMode = i;
          break;
        }
      }
      
      if ( iRankIntraMode > 0 )
        iRankIntraMode --;
      iDir = m_uiIntraModeTableD17[iRankIntraMode];
      
      iRankIntraModeLarger = Max(0,iRankIntraMode-1);
      iDirLarger = m_uiIntraModeTableD17[iRankIntraModeLarger];
      
      m_uiIntraModeTableD17[iRankIntraModeLarger] = iDir;
      m_uiIntraModeTableD17[iRankIntraMode] = iDirLarger;
      
      uiIPredMode = (iDir>=iMostProbable? iDir+1: iDir);
    }
  }
  else
  {
    UInt uiCode;
    UInt uiLength = lengthHuff34[32];
    m_pcBitstream->pseudoRead(uiLength,uiCode);
    if ((uiCode>>(uiLength- lengthHuff34[0])) == huff34[0])
    {
      m_pcBitstream->read(lengthHuff34[0],uiCode);
      uiIPredMode = iMostProbable;
    }
    else
    {
      iRankIntraMode = 0;
      for (Int i=1;i<34;i++)
      {
        if( (uiCode>>(uiLength- lengthHuff34[i])) == huff34[i])
        {
          m_pcBitstream->read(lengthHuff34[i], uiCode);
          iRankIntraMode = i;
          break;
        }
      }
      
      if ( iRankIntraMode > 0 )
        iRankIntraMode --;
      iDir = m_uiIntraModeTableD34[iRankIntraMode];
      
      iRankIntraModeLarger = Max(0,iRankIntraMode-1);
      iDirLarger = m_uiIntraModeTableD34[iRankIntraModeLarger];
      
      m_uiIntraModeTableD34[iRankIntraModeLarger] = iDir;
      m_uiIntraModeTableD34[iRankIntraMode] = iDirLarger;
      
      uiIPredMode = (iDir>=iMostProbable? iDir+1: iDir);
    }
  }
  
  pcCU->setLumaIntraDirSubParts( (UChar)uiIPredMode, uiAbsPartIdx, uiDepth );     
}

#else
Void TDecCavlc::parseIntraDirLumaAng  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  Int  uiIPredMode;
  Int  iMostProbable = pcCU->getMostProbableIntraDirLuma( uiAbsPartIdx );
  
  xReadFlag( uiSymbol );
  
  if ( uiSymbol )
    uiIPredMode = iMostProbable;
  else
  {
    Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
    if ( g_aucIntraModeBitsAng[iIntraIdx] < 6 )
    {
      xReadFlag( uiSymbol ); uiIPredMode  = uiSymbol;
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) { xReadFlag( uiSymbol ); uiIPredMode |= uiSymbol << 1; }
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) { xReadFlag( uiSymbol ); uiIPredMode |= uiSymbol << 2; }
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 4 ) { xReadFlag( uiSymbol ); uiIPredMode |= uiSymbol << 3; }
    }
    else
    {
      xReadFlag( uiSymbol ); uiIPredMode  = uiSymbol;
      xReadFlag( uiSymbol ); uiIPredMode |= uiSymbol << 1;
      xReadFlag( uiSymbol ); uiIPredMode |= uiSymbol << 2;
      xReadFlag( uiSymbol ); uiIPredMode |= uiSymbol << 3;
      xReadFlag( uiSymbol ); uiIPredMode |= uiSymbol << 4;
      
      if (uiIPredMode == 31)
      { // Escape coding for the last two modes
        xReadFlag( uiSymbol );
        uiIPredMode = uiSymbol ? 32 : 31;
      }
    }
    
    if (uiIPredMode >= iMostProbable)
      uiIPredMode++;
  }
  
  pcCU->setLumaIntraDirSubParts( (UChar)uiIPredMode, uiAbsPartIdx, uiDepth );
}
#endif

Void TDecCavlc::parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
#if CHROMA_CODEWORD
  UInt uiMode = pcCU->getLumaIntraDir(uiAbsPartIdx);
  Int  iMax = uiMode < 4 ? 3 : 4;
  xReadUnaryMaxSymbol( uiSymbol, iMax );
  
  //switch codeword
  if (uiSymbol == 0)
  {
    uiSymbol = 4;
  }
#if CHROMA_CODEWORD_SWITCH 
  else
  {
    uiSymbol = ChromaMapping[iMax-3][uiSymbol];
    if (uiSymbol <= uiMode)
    {
      uiSymbol --;
    }
  }
#else
  else if (uiSymbol <= uiMode)
  {
    uiSymbol --;
  }
#endif
  //printf("uiMode %d, chroma %d, codeword %d, imax %d\n", uiMode, uiSymbol, uiRead, iMax);
#else
  xReadFlag( uiSymbol );
  
  if ( uiSymbol )
  {
    xReadUnaryMaxSymbol( uiSymbol, 3 );
    uiSymbol++;
  }
#endif
  pcCU->setChromIntraDirSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
  
  return ;
}

Void TDecCavlc::parseInterDir( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  if ( pcCU->getSlice()->getRefIdxCombineCoding() )
#else
  if(pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2)
#endif
  {
    UInt uiIndex,uiInterDir,tmp;
    Int x,cx,y,cy;
    
#if MS_LCEC_LOOKUP_TABLE_MAX_VALUE
    UInt uiMaxVal = 7;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
    uiMaxVal = 8;
#endif
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 1 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 1 )
    {
      if ( pcCU->getSlice()->getNoBackPredFlag() || ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) <= 1 ) )
      {
        uiMaxVal = 1;
      }
      else
      {
        uiMaxVal = 2;
      }
    }
    else if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2 )
    {
      if ( pcCU->getSlice()->getNoBackPredFlag() || ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) <= 2 ) )
      {
        uiMaxVal = 5;
      }
      else
      {
        uiMaxVal = 7;
      }
    }
    else if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) <= 0 )
    {
      uiMaxVal = 4+1+MS_LCEC_UNI_EXCEPTION_THRES;
    }
    
    xReadUnaryMaxSymbol( tmp, uiMaxVal );
#else    
    UInt vlcn = g_auiMITableVlcNum[m_uiMITableVlcIdx];
    tmp = xReadVlc( vlcn );
#endif
    UInt *m_uiMITableD = m_uiMI1TableD;
    x = m_uiMITableD[tmp];
    uiIndex = x;
    
    /* Adapt table */
    
    cx = tmp;
    cy = Max(0,cx-1);  
    y = m_uiMITableD[cy];
    m_uiMITableD[cy] = x;
    m_uiMITableD[cx] = y;
    m_uiMITableVlcIdx += cx == m_uiMITableVlcIdx ? 0 : (cx < m_uiMITableVlcIdx ? -1 : 1);
    
    {
      uiInterDir = Min(2,uiIndex>>1);  
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
      if ( uiIndex >=4 )
      {
        uiInterDir = 2;
      }
      else
      {
        uiInterDir = 0;
      }
#endif
#if DCM_COMB_LIST
      if(uiInterDir!=2 && pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C)>0)
      {
        uiInterDir = 0;
        m_iRefFrame0[uiAbsPartIdx] = uiIndex;
      }
      else 
#endif
      if (uiInterDir==0)
      {
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
        m_iRefFrame0[uiAbsPartIdx] = uiIndex;
#else
        m_iRefFrame0[uiAbsPartIdx] = uiIndex&1;
#endif
      }
      else if (uiInterDir==1)
        m_iRefFrame1[uiAbsPartIdx] = uiIndex&1;
      else
      {
        m_iRefFrame0[uiAbsPartIdx] = (uiIndex>>1)&1;
        m_iRefFrame1[uiAbsPartIdx] = (uiIndex>>0)&1;
      }
    }
    ruiInterDir = uiInterDir+1;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
    if ( x < 8 )
#endif
    {
      return;
    }
  }
  
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  m_iRefFrame0[uiAbsPartIdx] = 1000;
  m_iRefFrame1[uiAbsPartIdx] = 1000;
#endif
  
  xReadFlag( uiSymbol );
  
  if ( uiSymbol )
  {
    uiSymbol = 2;
  }
#if DCM_COMB_LIST
  else if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0)
  {
    uiSymbol = 0;
  }
#endif
  else if ( pcCU->getSlice()->getNoBackPredFlag() )
  {
    uiSymbol = 0;
  }
  else
  {
    xReadFlag( uiSymbol );
  }
  uiSymbol++;
  ruiInterDir = uiSymbol;
  return;
}

Void TDecCavlc::parseRefFrmIdx( TComDataCU* pcCU, Int& riRefFrmIdx, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;
  
  if (pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2 && pcCU->getSlice()->isInterB())
  {
#if DCM_COMB_LIST
    if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C ) > 0 && eRefList==REF_PIC_LIST_C)
    {
      riRefFrmIdx = m_iRefFrame0[uiAbsPartIdx]; 
    }
    else 
#endif
    if (eRefList==REF_PIC_LIST_0)
    {
      riRefFrmIdx = m_iRefFrame0[uiAbsPartIdx];      
    }
    if (eRefList==REF_PIC_LIST_1)
    {
      riRefFrmIdx = m_iRefFrame1[uiAbsPartIdx];
    }
    return;
  }    
  
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  if ( ( m_iRefFrame0[uiAbsPartIdx] != 1000 || m_iRefFrame1[uiAbsPartIdx] != 1000 ) &&
      pcCU->getSlice()->getRefIdxCombineCoding() )
  {
    if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C ) > 0 && eRefList==REF_PIC_LIST_C )
    {
      riRefFrmIdx = m_iRefFrame0[uiAbsPartIdx]; 
    }
    else if (eRefList==REF_PIC_LIST_0)
    {
      riRefFrmIdx = m_iRefFrame0[uiAbsPartIdx];      
    }
    else if (eRefList==REF_PIC_LIST_1)
    {
      riRefFrmIdx = m_iRefFrame1[uiAbsPartIdx];
    }
    return;
  }
  
  UInt uiRefFrmIdxMinus = 0;
  if ( pcCU->getSlice()->getRefIdxCombineCoding() )
  {
    if ( pcCU->getInterDir( uiAbsPartIdx ) != 3 )
    {
      if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 )
      {
        uiRefFrmIdxMinus = 4;
      }
      else
      {
        uiRefFrmIdxMinus = MS_LCEC_UNI_EXCEPTION_THRES+1;
      }
    }
    else if ( eRefList == REF_PIC_LIST_1 && pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx ) < 2 )
    {
      uiRefFrmIdxMinus = 2;
    }
  }
  if ( pcCU->getSlice()->getNumRefIdx( eRefList ) - uiRefFrmIdxMinus <= 1 )
  {
    uiSymbol = 0;
    riRefFrmIdx = uiSymbol;
    riRefFrmIdx += uiRefFrmIdxMinus;
    return;
  }
#endif
  
  xReadFlag ( uiSymbol );
  if ( uiSymbol )
  {
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
    xReadUnaryMaxSymbol( uiSymbol, pcCU->getSlice()->getNumRefIdx( eRefList )-2 - uiRefFrmIdxMinus );
#else
    xReadUnaryMaxSymbol( uiSymbol, pcCU->getSlice()->getNumRefIdx( eRefList )-2 );
#endif
    
    uiSymbol++;
  }
  riRefFrmIdx = uiSymbol;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  riRefFrmIdx += uiRefFrmIdxMinus;
#endif
  
  return;
}

Void TDecCavlc::parseMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  Int iHor, iVer;
  UInt uiAbsPartIdxL, uiAbsPartIdxA;
  Int iHorPred, iVerPred;
  
  TComDataCU* pcCUL   = pcCU->getPULeft ( uiAbsPartIdxL, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  TComDataCU* pcCUA   = pcCU->getPUAbove( uiAbsPartIdxA, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  
  TComCUMvField* pcCUMvFieldL = ( pcCUL == NULL || pcCUL->isIntra( uiAbsPartIdxL ) ) ? NULL : pcCUL->getCUMvField( eRefList );
  TComCUMvField* pcCUMvFieldA = ( pcCUA == NULL || pcCUA->isIntra( uiAbsPartIdxA ) ) ? NULL : pcCUA->getCUMvField( eRefList );
  
  iHorPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsHor() ) +
  ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsHor() );
  iVerPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsVer() ) +
  ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsVer() );
  
  TComMv cTmpMv( 0, 0 );
  pcCU->getCUMvField( eRefList )->setAllMv( cTmpMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  
  xReadSvlc( iHor );
  xReadSvlc( iVer );
  
  // set mvd
  TComMv cMv( iHor, iVer );
  pcCU->getCUMvField( eRefList )->setAllMvd( cMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  
  return;
}

Void TDecCavlc::parseDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiDQp;
  Int  iDQp;
  
  xReadFlag( uiDQp );
  
  if ( uiDQp == 0 )
  {
    uiDQp = pcCU->getSlice()->getSliceQp();
  }
  else
  {
    xReadSvlc( iDQp );
    uiDQp = pcCU->getSlice()->getSliceQp() + iDQp;
  }
  
  pcCU->setQPSubParts( uiDQp, uiAbsPartIdx, uiDepth );
}

Void TDecCavlc::parseCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
  if (eType == TEXT_ALL)
  {
    UInt uiCbf,tmp;
    UInt uiCBP,uiCbfY,uiCbfU,uiCbfV;
    Int n,x,cx,y,cy;
    
    /* Start adaptation */
    n = pcCU->isIntra( uiAbsPartIdx ) ? 0 : 1;
    UInt vlcn = g_auiCbpVlcNum[n][m_uiCbpVlcIdx[n]];
    tmp = xReadVlc( vlcn );    
    uiCBP = m_uiCBPTableD[n][tmp];
    
    /* Adapt LP table */
    cx = tmp;
    cy = Max(0,cx-1);
    x = uiCBP;
    y = m_uiCBPTableD[n][cy];
    m_uiCBPTableD[n][cy] = x;
    m_uiCBPTableD[n][cx] = y;
    m_uiCbpVlcIdx[n] += cx == m_uiCbpVlcIdx[n] ? 0 : (cx < m_uiCbpVlcIdx[n] ? -1 : 1);
    
    uiCbfY = (uiCBP>>0)&1;
    uiCbfU = (uiCBP>>1)&1;
    uiCbfV = (uiCBP>>2)&1;
    
    uiCbf = pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA );
    pcCU->setCbfSubParts( uiCbf | ( uiCbfY << uiTrDepth ), TEXT_LUMA, uiAbsPartIdx, uiDepth );
    
    uiCbf = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U );
    pcCU->setCbfSubParts( uiCbf | ( uiCbfU << uiTrDepth ), TEXT_CHROMA_U, uiAbsPartIdx, uiDepth );
    
    uiCbf = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V );
    pcCU->setCbfSubParts( uiCbf | ( uiCbfV << uiTrDepth ), TEXT_CHROMA_V, uiAbsPartIdx, uiDepth );
  }
}

Void TDecCavlc::parseBlockCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth, UInt uiQPartNum )
{
  assert(uiTrDepth > 0);
  UInt uiCbf4, uiCbf;
  
  Int x,cx,y,cy;
  UInt tmp;
  
  UInt n = (pcCU->isIntra(uiAbsPartIdx) && eType == TEXT_LUMA)? 0:1;
  UInt vlcn = (n==0)?g_auiBlkCbpVlcNum[m_uiBlkCbpVlcIdx]:11;
  
  tmp = xReadVlc( vlcn );    
  uiCbf4 = m_uiBlkCBPTableD[n][tmp];
  
  cx = tmp;
  cy = Max(0,cx-1);
  x = uiCbf4;
  y = m_uiBlkCBPTableD[n][cy];
  m_uiBlkCBPTableD[n][cy] = x;
  m_uiBlkCBPTableD[n][cx] = y;
  if(n==0)
    m_uiBlkCbpVlcIdx += cx == m_uiBlkCbpVlcIdx ? 0 : (cx < m_uiBlkCbpVlcIdx ? -1 : 1);
  
  uiCbf4++;
  
  uiCbf = pcCU->getCbf( uiAbsPartIdx, eType );
  pcCU->setCbfSubParts( uiCbf | ( ((uiCbf4>>3)&0x01) << uiTrDepth ), eType, uiAbsPartIdx, uiDepth ); uiAbsPartIdx += uiQPartNum;
  uiCbf = pcCU->getCbf( uiAbsPartIdx, eType );
  pcCU->setCbfSubParts( uiCbf | ( ((uiCbf4>>2)&0x01) << uiTrDepth ), eType, uiAbsPartIdx, uiDepth ); uiAbsPartIdx += uiQPartNum;
  uiCbf = pcCU->getCbf( uiAbsPartIdx, eType );
  pcCU->setCbfSubParts( uiCbf | ( ((uiCbf4>>1)&0x01) << uiTrDepth ), eType, uiAbsPartIdx, uiDepth ); uiAbsPartIdx += uiQPartNum;
  uiCbf = pcCU->getCbf( uiAbsPartIdx, eType );
  pcCU->setCbfSubParts( uiCbf | ( (uiCbf4&0x01) << uiTrDepth ), eType, uiAbsPartIdx, uiDepth );
  
  return;
}

Void TDecCavlc::parseCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
  
  if ( uiWidth > pcCU->getSlice()->getSPS()->getMaxTrSize() )
  {
    uiWidth  = pcCU->getSlice()->getSPS()->getMaxTrSize();
    uiHeight = pcCU->getSlice()->getSPS()->getMaxTrSize();
  }
  UInt uiSize   = uiWidth*uiHeight;
  
  // point to coefficient
  TCoeff* piCoeff = pcCoef;
  
  // initialize scan
  const UInt*  pucScan;
  
  //UInt uiConvBit = g_aucConvertToBit[ Min(8,uiWidth) ];
  UInt uiConvBit = g_aucConvertToBit[ pcCU->isIntra( uiAbsPartIdx ) ? uiWidth : Min(8,uiWidth)    ];
  pucScan        = g_auiFrameScanXY  [ uiConvBit + 1 ];
  
#if QC_MDCS
  UInt uiBlkPos;
  UInt uiLog2BlkSize = g_aucConvertToBit[ pcCU->isIntra( uiAbsPartIdx ) ? uiWidth : Min(8,uiWidth)    ] + 2;
  const UInt uiScanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, eTType==TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
#endif //QC_MDCS
  
  UInt uiDecodeDCCoeff = 0;
  Int dcCoeff = 0;
  if (pcCU->isIntra(uiAbsPartIdx))
  {
    UInt uiAbsPartIdxL, uiAbsPartIdxA;
    TComDataCU* pcCUL   = pcCU->getPULeft (uiAbsPartIdxL, pcCU->getZorderIdxInCU() + uiAbsPartIdx);
    TComDataCU* pcCUA   = pcCU->getPUAbove(uiAbsPartIdxA, pcCU->getZorderIdxInCU() + uiAbsPartIdx);
    if (pcCUL == NULL && pcCUA == NULL)
    {
      uiDecodeDCCoeff = 1;
      dcCoeff = xReadVlc(eTType == TEXT_LUMA ? 3 : 1);
      if (dcCoeff)
      {
        UInt sign;
        xReadFlag(sign);
        if (sign)
        {
          dcCoeff = -dcCoeff;
        }
      }
    }
  }
  
  UInt uiScanning;
  
  TCoeff scoeff[64];
  Int iBlockType;
  if( uiSize == 2*2 )
  {
    // hack: re-use 4x4 coding
#if QC_MOD_LCEC
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
      iBlockType = eTType-2;
    else
      iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
#else
    iBlockType = pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType();
#endif
    xParseCoeff4x4( scoeff, iBlockType );
    
    for (uiScanning=0; uiScanning<4; uiScanning++)
    {
#if QC_MDCS
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning];  
      piCoeff[ uiBlkPos ] =  scoeff[15-uiScanning];
#else
      piCoeff[ pucScan[ uiScanning ] ] = scoeff[15-uiScanning];
#endif //QC_MDCS
    }
  }
  else if ( uiSize == 4*4 )
  {
#if QC_MOD_LCEC
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
      iBlockType = eTType-2;
    else
      iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
#else
    iBlockType = pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType();
#endif
    xParseCoeff4x4( scoeff, iBlockType );
    
    for (uiScanning=0; uiScanning<16; uiScanning++)
    {
#if QC_MDCS
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning];  
      piCoeff[ uiBlkPos ] =  scoeff[15-uiScanning];
#else
      piCoeff[ pucScan[ uiScanning ] ] = scoeff[15-uiScanning];
#endif //QC_MDCS
    }
  }
  else if ( uiSize == 8*8 )
  {
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V) //8x8 specific
      iBlockType = eTType-2;
    else
      iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
    xParseCoeff8x8( scoeff, iBlockType );
    
    for (uiScanning=0; uiScanning<64; uiScanning++)
    {
#if QC_MDCS
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
      piCoeff[ uiBlkPos ] =  scoeff[63-uiScanning];
#else
      piCoeff[ pucScan[ uiScanning ] ] = scoeff[63-uiScanning];
#endif //QC_MDCS
    }
    
  }
  else
  {
    if (!pcCU->isIntra( uiAbsPartIdx ))
    {
      memset(piCoeff,0,sizeof(TCoeff)*uiSize);
      if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V) 
        iBlockType = eTType-2;
      else
        iBlockType = 5 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
      xParseCoeff8x8( scoeff, iBlockType );
      
      for (uiScanning=0; uiScanning<64; uiScanning++)
      {  
#if QC_MDCS
        uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
        uiBlkPos = (uiBlkPos/8)* uiWidth + uiBlkPos%8;
        piCoeff[ uiBlkPos ] =  scoeff[63-uiScanning];
#else
        piCoeff[(pucScan[uiScanning]/8)*uiWidth + (pucScan[uiScanning]%8)] = scoeff[63-uiScanning];
#endif //QC_MDCS
      }
      return;
    }
    
    if(pcCU->isIntra( uiAbsPartIdx ))
    {
      memset(piCoeff,0,sizeof(TCoeff)*uiSize);
      
      if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V) 
        iBlockType = eTType-2;
      else
        iBlockType = 5 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
      xParseCoeff8x8( scoeff, iBlockType );
      
      for (uiScanning=0; uiScanning<64; uiScanning++)
      {
#if QC_MDCS
        uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
        piCoeff[ uiBlkPos ] =  scoeff[63-uiScanning];
#else
        piCoeff[ pucScan[ uiScanning ] ] = scoeff[63-uiScanning];
#endif //QC_MDCS
      }
    }
  }
  
  if (uiDecodeDCCoeff == 1)
  {
    piCoeff[0] = dcCoeff;
  }
  
  return ;
}

Void TDecCavlc::parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize )
{
  xReadFlag( ruiSubdivFlag );
}

Void TDecCavlc::parseQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
  UInt uiSymbol;
  xReadFlag( uiSymbol );
  pcCU->setCbfSubParts( uiSymbol << uiTrDepth, eType, uiAbsPartIdx, uiDepth );
}

Void TDecCavlc::parseQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& uiQtRootCbf )
{
  UInt uiSymbol;
  xReadFlag( uiSymbol );
  uiQtRootCbf = uiSymbol;
}

Void TDecCavlc::parseAlfFlag (UInt& ruiVal)
{
  xReadFlag( ruiVal );
}

#if TSB_ALF_HEADER
Void TDecCavlc::parseAlfFlagNum( UInt& ruiVal, UInt minValue, UInt depth )
{
  UInt uiLength = 0;
  UInt maxValue = (minValue << (depth*2));
  UInt temp = maxValue - minValue;
  for(UInt i=0; i<32; i++)
  {
    if(temp&0x1)
    {
      uiLength = i+1;
    }
    temp = (temp >> 1);
  }
  if(uiLength)
  {
    xReadCode( uiLength, ruiVal );
  }
  else
  {
    ruiVal = 0;
  }
  ruiVal += minValue;
}

Void TDecCavlc::parseAlfCtrlFlag( UInt &ruiAlfCtrlFlag )
{
  UInt uiSymbol;
  xReadFlag( uiSymbol );
  ruiAlfCtrlFlag = uiSymbol;
}
#endif

Void TDecCavlc::parseAlfUvlc (UInt& ruiVal)
{
  xReadUvlc( ruiVal );
}

Void TDecCavlc::parseAlfSvlc (Int&  riVal)
{
  xReadSvlc( riVal );
}


Void TDecCavlc::parseMergeFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx )
{
#if QC_LCEC_INTER_MODE
  if (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N )
    return;
#endif
  UInt uiSymbol;
  xReadFlag( uiSymbol );
  pcCU->setMergeFlagSubParts( uiSymbol ? true : false, uiAbsPartIdx, uiPUIdx, uiDepth );
}

/** parse merge index
 * \param pcCU
 * \param ruiMergeIndex 
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecCavlc::parseMergeIndex ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth )
{
  Bool bLeftInvolved = false;
  Bool bAboveInvolved = false;
  Bool bCollocatedInvolved = false;
  Bool bCornerInvolved = false;
  UInt uiNumCand = 0;
  for( UInt uiIter = 0; uiIter < MRG_MAX_NUM_CANDS; ++uiIter )
  {
    if( pcCU->getNeighbourCandIdx( uiIter, uiAbsPartIdx ) == uiIter + 1 )
    {
      uiNumCand++;
      if( uiIter == 0 )
      {
        bLeftInvolved = true;
      }
      else if( uiIter == 1 )
      {
        bAboveInvolved = true;
      }
      else if( uiIter == 2 )
      {
        bCollocatedInvolved = true;
      }
      else if( uiIter == 3 )
      {
        bCornerInvolved = true;
      }
    }
  }
  assert( uiNumCand > 1 );
  UInt uiUnaryIdx = 0;
  for( ; uiUnaryIdx < uiNumCand - 1; ++uiUnaryIdx )
  {
    UInt uiSymbol = 0;
    xReadFlag( uiSymbol );
    if( uiSymbol == 0 )
    {
      break;
    }
  }
  if( !bLeftInvolved )
  {
    ++uiUnaryIdx;
  }
  if( !bAboveInvolved && uiUnaryIdx >= 1 )
  {
    ++uiUnaryIdx;
  }

  if( !bCollocatedInvolved && uiUnaryIdx >= 2 )
  {
    ++uiUnaryIdx;
  }
  if( !bCornerInvolved && uiUnaryIdx >= 3 )
  {
    ++uiUnaryIdx;
  }
  ruiMergeIndex = uiUnaryIdx;
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TDecCavlc::xReadCode (UInt uiLength, UInt& ruiCode)
{
  assert ( uiLength > 0 );
  m_pcBitstream->read (uiLength, ruiCode);
}

Void TDecCavlc::xReadUvlc( UInt& ruiVal)
{
  UInt uiVal = 0;
  UInt uiCode = 0;
  UInt uiLength;
  m_pcBitstream->read( 1, uiCode );
  
  if( 0 == uiCode )
  {
    uiLength = 0;
    
    while( ! ( uiCode & 1 ))
    {
      m_pcBitstream->read( 1, uiCode );
      uiLength++;
    }
    
    m_pcBitstream->read( uiLength, uiVal );
    
    uiVal += (1 << uiLength)-1;
  }
  
  ruiVal = uiVal;
}

Void TDecCavlc::xReadSvlc( Int& riVal)
{
  UInt uiBits = 0;
  m_pcBitstream->read( 1, uiBits );
  if( 0 == uiBits )
  {
    UInt uiLength = 0;
    
    while( ! ( uiBits & 1 ))
    {
      m_pcBitstream->read( 1, uiBits );
      uiLength++;
    }
    
    m_pcBitstream->read( uiLength, uiBits );
    
    uiBits += (1 << uiLength);
    riVal = ( uiBits & 1) ? -(Int)(uiBits>>1) : (Int)(uiBits>>1);
  }
  else
  {
    riVal = 0;
  }
}

Void TDecCavlc::xReadFlag (UInt& ruiCode)
{
  m_pcBitstream->read( 1, ruiCode );
}

Void TDecCavlc::xReadUnaryMaxSymbol( UInt& ruiSymbol, UInt uiMaxSymbol )
{
  if (uiMaxSymbol == 0)
  {
    ruiSymbol = 0;
    return;
  }
  
  xReadFlag( ruiSymbol );
  
  if (ruiSymbol == 0 || uiMaxSymbol == 1)
  {
    return;
  }
  
  UInt uiSymbol = 0;
  UInt uiCont;
  
  do
  {
    xReadFlag( uiCont );
    uiSymbol++;
  }
  while( uiCont && (uiSymbol < uiMaxSymbol-1) );
  
  if( uiCont && (uiSymbol == uiMaxSymbol-1) )
  {
    uiSymbol++;
  }
  
  ruiSymbol = uiSymbol;
}

Void TDecCavlc::xReadExGolombLevel( UInt& ruiSymbol )
{
  UInt uiSymbol ;
  UInt uiCount = 0;
  do
  {
    xReadFlag( uiSymbol );
    uiCount++;
  }
  while( uiSymbol && (uiCount != 13));
  
  ruiSymbol = uiCount-1;
  
  if( uiSymbol )
  {
    xReadEpExGolomb( uiSymbol, 0 );
    ruiSymbol += uiSymbol+1;
  }
  
  return;
}

Void TDecCavlc::xReadEpExGolomb( UInt& ruiSymbol, UInt uiCount )
{
  UInt uiSymbol = 0;
  UInt uiBit = 1;
  
  
  while( uiBit )
  {
    xReadFlag( uiBit );
    uiSymbol += uiBit << uiCount++;
  }
  
  uiCount--;
  while( uiCount-- )
  {
    xReadFlag( uiBit );
    uiSymbol += uiBit << uiCount;
  }
  
  ruiSymbol = uiSymbol;
  
  return;
}

UInt TDecCavlc::xGetBit()
{
  UInt ruiCode;
  m_pcBitstream->read( 1, ruiCode );
  return ruiCode;
}

Int TDecCavlc::xReadVlc( Int n )
{
  assert( n>=0 && n<=11 );
  
  UInt zeroes=0, done=0, tmp;
  UInt cw, bit;
  UInt val = 0;
  UInt first;
  UInt lead = 0;
  
  if (n < 5)
  {
    while (!done && zeroes < 6)
    {
      xReadFlag( bit );
      if (bit)
      {
        if (n)
        {
          xReadCode( n, cw );
        }
        else
        {
          cw = 0;
        }
        done = 1;
      }
      else
      {
        zeroes++;
      }
    }
    if ( done )
    {
      val = (zeroes<<n)+cw;
    }
    else
    {
      lead = n;
      while (!done)
      {
        xReadFlag( first );
        if ( !first )
        {
          lead++;
        }
        else
        {
          if ( lead )
          {
            xReadCode( lead, tmp );
          }
          else
          {
            tmp = 0;
          }
          val = 6 * (1 << n) + (1 << lead) + tmp - (1 << n);
          done = 1;
        }
      }
    }
  }
  else if (n < 8)
  {
    while (!done)
    {
      xReadFlag( bit );
      if ( bit )
      {
        xReadCode( n-4, cw );
        done = 1;
      }
      else
      {
        zeroes++;
      }
    }
    val = (zeroes<<(n-4))+cw;
  }
  else if (n == 8)
  {
    if ( xGetBit() )
    {
      val = 0;
    }
    else if ( xGetBit() )
    {
      val = 1;
    }
    else
    {
      val = 2;
    }
  }
  else if (n == 9)
  {
    if ( xGetBit() )
    {
      if ( xGetBit() )
      {
        xReadCode(3, val);
        val += 3;
      }
      else if ( xGetBit() )
      {
        val = xGetBit() + 1;
      }
      else
      {
        val = 0;
      }
    }
    else
    {
      while (!done)
      {
        xReadFlag( bit );
        if ( bit )
        {
          xReadCode(4, cw);
          done = 1;
        }
        else
        {
          zeroes++;
        }
      }
      val = (zeroes<<4)+cw+11;
    }
  }
  else if (n == 10)
  {
    while (!done)
    {
      xReadFlag( first );
      if ( !first )
      {
        lead++;
      }
      else
      {
        if ( !lead )
        {
          val = 0;
        }
        else
        {
          xReadCode(lead, val);
          val += (1<<lead);
          val--;
        }
        done = 1;
      }
    }
  }
  else if (n == 11)
  {
    UInt code;
    xReadCode(3, val);
    if(val)
    {
      xReadCode(1, code);
      val = (val<<1)|code;
      val--;
    }
  }
  
  return val;
}

Void TDecCavlc::xParseCoeff4x4( TCoeff* scoeff, Int n )
{
  Int i;
  UInt sign;
  Int tmp;
  Int vlc,cn,this_pos;
  Int maxrun;
  Int last_position;
  Int atable[5] = {4,6,14,28,0xfffffff};
  Int vlc_adaptive=0;
  Int done;
  LastCoeffStruct combo;
  
#if QC_MOD_LCEC
  Int nTab;
  Int tr1;
  nTab=max(0,n-2);
#endif

  for (i = 0; i < 16; i++)
  {
    scoeff[i] = 0;
  }
  
  {
    /* Get the last nonzero coeff */
    Int x,y,cx,cy,vlcNum;
    Int vlcTable[8] = {2,2,2};
    
    /* Decode according to current LP table */
#if QC_MOD_LCEC
    vlcNum = vlcTable[nTab];
    tmp = xReadVlc( vlcNum );
    cn = m_uiLPTableD4[nTab][tmp];
#else
    vlcNum = vlcTable[n];
    
    tmp = xReadVlc( vlcNum );
    cn = m_uiLPTableD4[n][tmp];
#endif
    combo.level = (cn>15);
    combo.last_pos = cn&0x0f;
    
    /* Adapt LP table */
    cx = tmp;
    cy = Max( 0, cx-1 );
    x = cn;
#if QC_MOD_LCEC
    y = m_uiLPTableD4[nTab][cy];
    m_uiLPTableD4[nTab][cy] = x;
    m_uiLPTableD4[nTab][cx] = y;
#else
    y = m_uiLPTableD4[n][cy];
    m_uiLPTableD4[n][cy] = x;
    m_uiLPTableD4[n][cx] = y;
#endif
  }
  
  if ( combo.level == 1 )
  {
    tmp = xReadVlc( 0 );
    sign = tmp&1;
    tmp = (tmp>>1)+2;
  }
  else
  {
    tmp = 1;
    xReadFlag( sign );
  }
  
#if QC_MOD_LCEC
  if (tmp>1)
  {
    tr1=0;
  }
  else
  {
    tr1=1;
  }
#endif

  if ( sign )
  {
    tmp = -tmp;
  }
  
  last_position = combo.last_pos;
  this_pos = 15 - last_position;
  scoeff[this_pos] = tmp;
  i = this_pos;
  i++;
  
  done = 0;
  {
    while (!done && i < 16)
    {
      maxrun = 15-i;
#if QC_MOD_LCEC
      if(n==2)
        vlc = g_auiVlcTable8x8Intra[maxrun];
      else
        vlc = g_auiVlcTable8x8Inter[maxrun];
#else
      if (maxrun > 27)
      {
        maxrun = 28;
        vlc = 3;
      }
      else
      {
        vlc = g_auiVlcTable8x8[maxrun];
      }
#endif
      
      /* Go into run mode */
      cn = xReadVlc( vlc );
#if QC_MOD_LCEC
      if(n==2)
      {
        xRunLevelIndInv(&combo, maxrun, g_auiLumaRunTr14x4[tr1][maxrun], cn);
      }
      else
#endif
      {
        combo = g_acstructLumaRun8x8[maxrun][cn];
      }
      i += combo.last_pos;
      /* No sign for last zeroes */
      if (i < 16)
      {
        if (combo.level == 1)
        {
          tmp = xReadVlc( 0 );
          sign = tmp&1;
          tmp = (tmp>>1)+2;
          done = 1;
        }
        else
        {
          tmp = 1;
          xReadFlag( sign );
        }
        if ( sign )
        {
          tmp = -tmp;
        }
        scoeff[i] = tmp;
      }
      i++;
#if QC_MOD_LCEC
      if (tr1>0 && tr1<MAX_TR1)
      {
        tr1++;
      }
#endif
    }
  }
  if (i < 16)
  {
    /* Get the rest in level mode */
    while ( i < 16 )
    {
      tmp = xReadVlc( vlc_adaptive );
      if ( tmp > atable[vlc_adaptive] )
      {
        vlc_adaptive++;
      }
      if ( tmp )
      {
        xReadFlag( sign );
        if ( sign )
        {
          tmp = -tmp;
        }
      }
      scoeff[i] = tmp;
      i++;
    }
  }
  
  return;
}

#if QC_MOD_LCEC

Void TDecCavlc::xRunLevelIndInv(LastCoeffStruct *combo, Int maxrun, UInt lrg1Pos, UInt cn)
{
  int lev, run;
  if (lrg1Pos>0)
  {
    if(cn < min(lrg1Pos, maxrun+2))
    {
      lev = 0; 
      run = cn; 
    }
    else if(cn < (maxrun<<1) + 4 - (Int)lrg1Pos)
    {
      if((cn+lrg1Pos)&1)
      {
        lev = 0;
        run = (cn + lrg1Pos - 1) >> 1;
      }
      else
      {
        lev = 1; 
        run = (cn - lrg1Pos)>>1;
      }
    }
    else
    {
      lev = 1;
      run = cn - maxrun - 2;
    }
  }
  else
  {
    if( cn & 1 )
    {
      lev = 0; run = (cn-1)>>1;
    }
    else
    {
      run = cn >> 1;
      lev = (run <= maxrun)?1:0;
    }
  }
  combo->level = lev;
  combo->last_pos = run;
}
#endif


Void TDecCavlc::xParseCoeff8x8(TCoeff* scoeff, int n)
{
  Int i;
  UInt sign;
  Int tmp;
  LastCoeffStruct combo;
  Int vlc,cn,this_pos;
  Int maxrun;
  Int last_position;
  Int atable[5] = {4,6,14,28,0xfffffff};
  Int vlc_adaptive=0;
  Int done;
#if QC_MOD_LCEC
  Int tr1;
#endif
  
  static const Int switch_thr[10] = {49,49,0,49,49,0,49,49,49,49};
  Int sum_big_coef = 0;
  
  for (i = 0; i < 64; i++)
  {
    scoeff[i] = 0;
  }
  
  /* Get the last nonzero coeff */
  {
    Int x,y,cx,cy,vlcNum;
    
    /* Decode according to current LP table */
    // ADAPT_VLC_NUM
    vlcNum = g_auiLastPosVlcNum[n][Min(16,m_uiLastPosVlcIndex[n])];
    tmp = xReadVlc( vlcNum );
    cn = m_uiLPTableD8[n][tmp];
    combo.level = (cn>63);
    combo.last_pos = cn&0x3f;
    
    /* Adapt LP table */
    cx = tmp;
    cy = Max(0,cx-1);
    x = cn;
    y = m_uiLPTableD8[n][cy];
    m_uiLPTableD8[n][cy] = x;
    m_uiLPTableD8[n][cx] = y;
    // ADAPT_VLC_NUM
    m_uiLastPosVlcIndex[n] += cx == m_uiLastPosVlcIndex[n] ? 0 : (cx < m_uiLastPosVlcIndex[n] ? -1 : 1);
  }
  
  if (combo.level == 1)
  {
    tmp = xReadVlc( 0 );
    sign = tmp&1;
    tmp = (tmp>>1)+2;
  }
  else
  {
    tmp = 1;
    xReadFlag( sign );
  }

#if QC_MOD_LCEC
  if (tmp>1)
  {
    tr1=0;
  }
  else
  {
    tr1=1;
  }
#endif

  if ( sign )
  {
    tmp = -tmp;
  }
  
  last_position = combo.last_pos;
  this_pos = 63 - last_position;
  scoeff[this_pos] = tmp;
  i = this_pos;
  i++;
  
  done = 0;
  {
    while (!done && i < 64)
    {
      maxrun = 63-i;
#if QC_MOD_LCEC
      if (n == 2 || n == 5)
        vlc = g_auiVlcTable8x8Intra[Min(maxrun,28)];
      else
        vlc = g_auiVlcTable8x8Inter[Min(maxrun,28)];
#else
      if (maxrun > 27)
      {
        maxrun = 28;
        vlc = 3;
      }
      else
      {
        vlc = g_auiVlcTable8x8[maxrun];
      }
#endif
      
      /* Go into run mode */
      cn = xReadVlc( vlc );
#if QC_MOD_LCEC
      if (n == 2 || n == 5)
        xRunLevelIndInv(&combo, maxrun, g_auiLumaRunTr18x8[tr1][min(maxrun,28)], cn);
      else
        combo = g_acstructLumaRun8x8[Min(maxrun,28)][cn];
#else
      combo = g_acstructLumaRun8x8[maxrun][cn];
#endif
      i += combo.last_pos;
      /* No sign for last zeroes */
      if (i < 64)
      {
        if (combo.level == 1)
        {
          tmp = xReadVlc( 0 );
          sign = tmp&1;
          tmp = (tmp>>1)+2;
          
          sum_big_coef += tmp;
          if (i > switch_thr[n] || sum_big_coef > 2)
          {
            done = 1;
          }
        }
        else
        {
          tmp = 1;
          xReadFlag( sign );
        }
        if ( sign )
        {
          tmp = -tmp;
        }
        scoeff[i] = tmp;
      }
      i++;
#if QC_MOD_LCEC
      if (tr1==0 || combo.level != 0)
      {
        tr1=0;
      }
      else if( tr1 < MAX_TR1)
      {
        tr1++;
      }
#endif
    }
  }
  if (i < 64)
  {
    /* Get the rest in level mode */
    while (i<64)
    {
      tmp = xReadVlc( vlc_adaptive );
      
      if (tmp>atable[vlc_adaptive])
      {
        vlc_adaptive++;
      }
      if (tmp)
      {
        xReadFlag( sign );
        if ( sign )
        {
          tmp = -tmp;
        }
      }
      scoeff[i] = tmp;
      i++;
    }
  }
  return;
}
