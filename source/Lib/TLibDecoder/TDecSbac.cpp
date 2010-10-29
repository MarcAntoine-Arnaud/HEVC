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

/** \file     TDecSbac.cpp
    \brief    Context-adaptive entropy decoder class
*/

#include "TDecSbac.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TDecSbac::TDecSbac() 
    // new structure here
  : m_cCUSplitFlagSCModel     ( 1,             1,               NUM_SPLIT_FLAG_CTX            )
  , m_cCUSkipFlagSCModel      ( 1,             1,               NUM_SKIP_FLAG_CTX             )
  , m_cCUAlfCtrlFlagSCModel   ( 1,             1,               NUM_ALF_CTRL_FLAG_CTX         )
#if HHI_MRG
  , m_cCUMergeFlagSCModel     ( 1,             1,               NUM_MERGE_FLAG_CTX            )
  , m_cCUMergeIndexSCModel    ( 1,             1,               NUM_MERGE_INDEX_CTX           )
#endif
  , m_cCUPartSizeSCModel      ( 1,             1,               NUM_PART_SIZE_CTX             )
  , m_cCUXPosiSCModel         ( 1,             1,               NUM_CU_X_POS_CTX              )
  , m_cCUYPosiSCModel         ( 1,             1,               NUM_CU_Y_POS_CTX              )
  , m_cCUPredModeSCModel      ( 1,             1,               NUM_PRED_MODE_CTX             )
  , m_cCUIntraPredSCModel     ( 1,             1,               NUM_ADI_CTX                   )
#if HHI_AIS
  , m_cCUIntraFiltFlagSCModel ( 1,             1,               NUM_ADI_FILT_CTX              )
#endif
  , m_cCUChromaPredSCModel    ( 1,             1,               NUM_CHROMA_PRED_CTX           )
  , m_cCUInterDirSCModel      ( 1,             1,               NUM_INTER_DIR_CTX             )
  , m_cCUMvdSCModel           ( 1,             2,               NUM_MV_RES_CTX                )
#ifdef DCM_PBIC
  , m_cCUIcdSCModel           ( 1,             3,               NUM_IC_RES_CTX                )
#endif
  , m_cCURefPicSCModel        ( 1,             1,               NUM_REF_NO_CTX                )
#ifdef QC_AMVRES
  , m_cCUMvResCModel       ( 1,             1,               NUM_MV_RES_FALG_CTX  )
#endif
#if HHI_RQT
  , m_cCUTransSubdivFlagSCModel( 1,            1,               NUM_TRANS_SUBDIV_FLAG_CTX     )
#if HHI_RQT_ROOT
  , m_cCUQtRootCbfSCModel     ( 1,             1,               NUM_QT_ROOT_CBF_CTX           )
#endif
#endif
  , m_cCUTransIdxSCModel      ( 1,             1,               NUM_TRANS_IDX_CTX             )
  , m_cCUDeltaQpSCModel       ( 1,             1,               NUM_DELTA_QP_CTX              )
  , m_cCUCbfSCModel           ( 1,             2,               NUM_CBF_CTX                   )
#if HHI_RQT
  , m_cCUQtCbfSCModel         ( 1,             3,               NUM_QT_CBF_CTX                )
#endif
#if HHI_TRANSFORM_CODING
  , m_cCuCtxModSig            ( MAX_CU_DEPTH,  2,               NUM_SIG_FLAG_CTX              )
  , m_cCuCtxModLast           ( MAX_CU_DEPTH,  2,               NUM_LAST_FLAG_CTX             )
  , m_cCuCtxModAbsGreOne      ( 1,             2,               NUM_ABS_GREATER_ONE_CTX       )
  , m_cCuCtxModCoeffLevelM1   ( 1,             2,               NUM_COEFF_LEVEL_MINUS_ONE_CTX )
#else
  , m_cCUMapSCModel           ( MAX_CU_DEPTH,  2,               NUM_MAP_CTX                   )
  , m_cCULastSCModel          ( MAX_CU_DEPTH,  2,               NUM_LAST_CTX                  )
  , m_cCUOneSCModel           ( MAX_CU_DEPTH,  2,               NUM_ONE_CTX                   )
  , m_cCUAbsSCModel           ( MAX_CU_DEPTH,  2,               NUM_ABS_CTX                   )
#endif

  , m_cMVPIdxSCModel          ( 1,             1,               NUM_MVP_IDX_CTX               )
#ifdef DCM_PBIC
  , m_cICPIdxSCModel          ( 1,             1,               NUM_ICP_IDX_CTX               )
  , m_cZTreeMV0SCModel        ( 1,             1,               NUM_ZTREE_MV0_CTX             )
  , m_cZTreeMV1SCModel        ( 1,             1,               NUM_ZTREE_MV1_CTX             )
  , m_cZTreeMV2SCModel        ( 1,             1,               NUM_ZTREE_MV2_CTX             )
#endif
  , m_cCUROTindexSCModel      ( 1,             1,               NUM_ROT_IDX_CTX               )
  , m_cCUCIPflagCCModel       ( 1,             1,               NUM_CIP_FLAG_CTX              )
  , m_cALFFlagSCModel         ( 1,             1,               NUM_ALF_FLAG_CTX              )
  , m_cALFUvlcSCModel         ( 1,             1,               NUM_ALF_UVLC_CTX              )
  , m_cALFSvlcSCModel         ( 1,             1,               NUM_ALF_SVLC_CTX              )
#if HHI_ALF
  , m_cALFSplitFlagSCModel    ( 1,             1,               NUM_ALF_SPLITFLAG_CTX         )
#endif
#if PLANAR_INTRA
  , m_cPlanarIntraSCModel     ( 1,             1,               NUM_PLANAR_INTRA_CTX          )
#endif
{
  m_pcBitstream = 0;
  m_pcTDecBinIf = 0;

  m_bAlfCtrl = false;
  m_uiMaxAlfCtrlDepth = 0;
}

TDecSbac::~TDecSbac()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TDecSbac::resetEntropy          (TComSlice* pcSlice)
{
  Int  iQp              = pcSlice->getSliceQp();
  SliceType eSliceType  = pcSlice->getSliceType();

  m_cCUSplitFlagSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_SPLIT_FLAG );

  m_cCUSkipFlagSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_SKIP_FLAG );
#if HHI_MRG
  m_cCUMergeFlagSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_MERGE_FLAG );
  m_cCUMergeIndexSCModel.initBuffer   ( eSliceType, iQp, (Short*)INIT_MERGE_INDEX );
#endif
  m_cCUAlfCtrlFlagSCModel.initBuffer  ( eSliceType, iQp, (Short*)INIT_SKIP_FLAG );
  m_cCUPartSizeSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_PART_SIZE );
  m_cCUXPosiSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_CU_X_POS );
  m_cCUYPosiSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_CU_Y_POS );
  m_cCUPredModeSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_INTRA_PRED_MODE );
#if HHI_AIS
  m_cCUIntraFiltFlagSCModel.initBuffer( eSliceType, iQp, (Short*)INIT_INTRA_PRED_FILT );
#endif
  m_cCUChromaPredSCModel.initBuffer   ( eSliceType, iQp, (Short*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_MVD );
#ifdef QC_AMVRES
  m_cCUMvResCModel.initBuffer         (eSliceType,  iQp, (Short*)INIT_MVRES_FLAG );
#endif
#ifdef DCM_PBIC
  m_cCUIcdSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_ICD );
#endif
  m_cCURefPicSCModel.initBuffer       ( eSliceType, iQp, (Short*)INIT_REF_PIC );

  m_cCUDeltaQpSCModel.initBuffer      ( eSliceType, iQp, (Short*)INIT_DQP );
  m_cCUCbfSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_CBF );
#if HHI_RQT
  m_cCUQtCbfSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_QT_CBF );
#if HHI_RQT_ROOT
  m_cCUQtRootCbfSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_QT_ROOT_CBF );
#endif
#endif

#if HHI_TRANSFORM_CODING
  m_cCuCtxModSig.initBuffer           ( eSliceType, iQp, (Short*)INIT_SIG_FLAG );
  m_cCuCtxModLast.initBuffer          ( eSliceType, iQp, (Short*)INIT_LAST_FLAG );
  m_cCuCtxModAbsGreOne.initBuffer     ( eSliceType, iQp, (Short*)INIT_ABS_GREATER_ONE_FLAG );
  m_cCuCtxModCoeffLevelM1.initBuffer  ( eSliceType, iQp, (Short*)INIT_COEFF_LEVEL_MINUS_ONE_FLAG );
#else
  m_cCUMapSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_SIGMAP );
  m_cCULastSCModel.initBuffer         ( eSliceType, iQp, (Short*)INIT_LAST_FLAG );
  m_cCUOneSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_ONE_FLAG );
  m_cCUAbsSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_TCOEFF_LEVEL );
#endif

  m_cMVPIdxSCModel.initBuffer         ( eSliceType, iQp, (Short*)INIT_MVP_IDX );
#ifdef DCM_PBIC
  m_cICPIdxSCModel.initBuffer         ( eSliceType, iQp, (Short*)INIT_ICP_IDX );
  m_cZTreeMV0SCModel.initBuffer       ( eSliceType, iQp, (Short*)INIT_ZTree_MV0 );
  m_cZTreeMV1SCModel.initBuffer       ( eSliceType, iQp, (Short*)INIT_ZTree_MV1 );
  m_cZTreeMV2SCModel.initBuffer       ( eSliceType, iQp, (Short*)INIT_ZTree_MV2 );
#endif
  m_cCUROTindexSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_ROT_IDX );
  m_cCUCIPflagCCModel.initBuffer      ( eSliceType, iQp, (Short*)INIT_CIP_IDX );

#if HHI_ALF
  m_cALFSplitFlagSCModel.initBuffer   ( eSliceType, iQp, (Short*)INIT_ALF_SPLITFLAG );
#endif
  m_cALFFlagSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_ALF_FLAG );
  m_cALFUvlcSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_ALF_UVLC );
  m_cALFSvlcSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_ALF_SVLC );
#if HHI_RQT
  m_cCUTransSubdivFlagSCModel.initBuffer( eSliceType, iQp, (Short*)INIT_TRANS_SUBDIV_FLAG );
#endif
  m_cCUTransIdxSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_TRANS_IDX );

#if PLANAR_INTRA
  m_cPlanarIntraSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_PLANAR_INTRA );
#endif

  m_uiLastDQpNonZero  = 0;

  // new structure
  m_uiLastQp          = iQp;

  m_pcTDecBinIf->start();
}

#if PLANAR_INTRA
// Temporary VLC function
UInt TDecSbac::xParsePlanarBins( )
{
  Bool bDone    = false;
  UInt uiZeroes = 0;
  UInt uiBit;
  UInt uiCodeword;

  while (!bDone)
  {
    m_pcTDecBinIf->decodeBinEP( uiBit );

    if ( uiBit )
    {
      m_pcTDecBinIf->decodeBinEP( uiCodeword );
      bDone = true;
    }
    else
      uiZeroes++;
  }

  return ( ( uiZeroes << 1 ) + uiCodeword );
}

Int TDecSbac::xParsePlanarDelta( TextType ttText )
{
  /* Planar quantization
  Y        qY              cW
  0-3   :  0,1,2,3         0-3
  4-15  :  4,6,8..14       4-9
  16-63 : 18,22,26..62    10-21
  64-.. : 68,76...        22-
  */
  UInt uiDeltaNegative = 0;
  Int  iDelta          = xParsePlanarBins();

  if( iDelta > 21 )
    iDelta = ( ( iDelta - 14 ) << 3 ) + 4;
  else if( iDelta > 9 )
    iDelta = ( ( iDelta - 6 ) << 2 ) + 2;
  else if( iDelta > 3 )
    iDelta = ( iDelta - 2 ) << 1;

  if( iDelta > 0 )
  {
    m_pcTDecBinIf->decodeBinEP( uiDeltaNegative );

    if( uiDeltaNegative )
      iDelta = -iDelta;
  }

  return iDelta;
}

Void TDecSbac::parsePlanarInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cPlanarIntraSCModel.get( 0, 0, 0 ) );

  if ( uiSymbol )
  {
    Int iPlanarFlag   = 1;
    Int iPlanarDeltaY = xParsePlanarDelta( TEXT_LUMA );
    Int iPlanarDeltaU = 0;
    Int iPlanarDeltaV = 0;

    // Planar delta for U and V
     m_pcTDecBinIf->decodeBin( uiSymbol, m_cPlanarIntraSCModel.get( 0, 0, 1 ) );

    if ( !uiSymbol )
    {
      iPlanarDeltaU = xParsePlanarDelta( TEXT_CHROMA_U );
      iPlanarDeltaV = xParsePlanarDelta( TEXT_CHROMA_V );
    }

    pcCU->setPartSizeSubParts  ( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts      ( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setPlanarInfoSubParts( iPlanarFlag, iPlanarDeltaY, iPlanarDeltaU, iPlanarDeltaV, uiAbsPartIdx, uiDepth );
  }
}
#endif

Void TDecSbac::parseCIPflag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  Int  iCtx = pcCU->getCtxCIPFlag( uiAbsPartIdx );

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUCIPflagCCModel.get( 0, 0, iCtx ) );
  pcCU->setCIPflagSubParts( (UChar)uiSymbol, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::parseROTindex  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  UInt indexROT = 0;
  Int   dictSize  = ROT_DICT;

  switch (dictSize)
  {
  case 9:
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 0) );
      if ( !uiSymbol )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 1 ) );
        indexROT  = uiSymbol;
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 2 ) );
        indexROT |= uiSymbol << 1;
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 2 ) );
        indexROT |= uiSymbol << 2;
        indexROT++;
      }
    }
    break;
  case 4:
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 0 ) );
      indexROT  = uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 1 ) );
      indexROT |= uiSymbol << 1;
    }
    break;
  case 2:
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 0) );
      if ( !uiSymbol ) indexROT =1;
    }
    break;
  case 5:
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 0) );
      if ( !uiSymbol )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 1 ) );
        indexROT  = uiSymbol;
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUROTindexSCModel.get( 0, 0, 2 ) );
        indexROT |= uiSymbol << 1;
        indexROT++;
      }
    }
    break;
  case 1:
    {
    }
    break;
  }
  pcCU->setROTindexSubParts( indexROT, uiAbsPartIdx, uiDepth );

  return;
}


Void TDecSbac::parseTerminatingBit( UInt& ruiBit )
{
  m_pcTDecBinIf->decodeBinTrm( ruiBit );
}


Void TDecSbac::xReadUnaryMaxSymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol )
{
  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] );

  if (ruiSymbol == 0 || uiMaxSymbol == 1)
  {
    return;
  }

  UInt uiSymbol = 0;
  UInt uiCont;

  do
  {
    m_pcTDecBinIf->decodeBin( uiCont, pcSCModel[ iOffset ] );
    uiSymbol++;
  }
  while( uiCont && (uiSymbol < uiMaxSymbol-1) );

  if( uiCont && (uiSymbol == uiMaxSymbol-1) )
  {
    uiSymbol++;
  }

  ruiSymbol = uiSymbol;
}

Void TDecSbac::xReadMvd( Int& riMvdComp, UInt uiAbsSum, UInt uiCtx )
{
  UInt uiLocalCtx = 0;

  if( uiAbsSum >= 3)
  {
    uiLocalCtx += ( uiAbsSum > 32) ? 2 : 1;
  }

  riMvdComp = 0;

  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMvdSCModel.get( 0, uiCtx, uiLocalCtx ) );

  if (!uiSymbol)
  {
    return;
  }

  xReadExGolombMvd( uiSymbol, &m_cCUMvdSCModel.get( 0, uiCtx, 3 ), 3 );
  uiSymbol++;

  UInt uiSign;
  m_pcTDecBinIf->decodeBinEP( uiSign );

  riMvdComp = ( 0 != uiSign ) ? -(Int)uiSymbol : (Int)uiSymbol;

  return;
}
#ifdef QC_AMVRES
Void TDecSbac::xReadMvResFlag ( Int& riVal, UInt uiCtx )
{
  UInt uiSymbol = 0;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMvResCModel.get( 0, 0, uiCtx ) );
  riVal = (Int)uiSymbol;
}
#ifdef DCM_PBIC
Bool TDecSbac::xParseMvResFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  Int MVres=0;
  if( pcCU->getSlice()->getSPS()->getUseAMVRes() )
  {
    UInt uiAbsPartIdxL, uiAbsPartIdxA;
    TComDataCU* pcCUL   = pcCU->getPULeft ( uiAbsPartIdxL, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
    TComDataCU* pcCUA   = pcCU->getPUAbove( uiAbsPartIdxA, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
    TComCUMvField* pcCUMvFieldL = ( pcCUL == NULL || pcCUL->isIntra( uiAbsPartIdxL ) ) ? NULL : pcCUL->getCUMvField( eRefList );
    TComCUMvField* pcCUMvFieldA = ( pcCUA == NULL || pcCUA->isIntra( uiAbsPartIdxA ) ) ? NULL : pcCUA->getCUMvField( eRefList );
    Int iL =   ( (pcCUMvFieldL == NULL) ? 1 : (Int)(pcCUMvFieldL->getMVRes(uiAbsPartIdxL)));
    Int iV =   ( (pcCUMvFieldA == NULL) ? 1 : (Int)(pcCUMvFieldA->getMVRes(uiAbsPartIdxA)));
    xReadMvResFlag(MVres,iL+iV);
  }
  pcCU->getCUMvField( eRefList )->setAllMVRes( MVres>0, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  return (MVres>0);
}
#endif
#endif


#ifdef DCM_PBIC
Void TDecSbac::xReadMvdNZ( Int& riMvdComp, UInt uiCtx )
{
  UInt uiSymbol;

  xReadExGolombMvd( uiSymbol, &m_cCUMvdSCModel.get( 0, uiCtx, 3 ), 3 );
  uiSymbol++;

  UInt uiSign;
  m_pcTDecBinIf->decodeBinEP( uiSign );

  riMvdComp = ( 0 != uiSign ) ? -(Int)uiSymbol : (Int)uiSymbol;

  return;
}

Void TDecSbac::xReadIcdNZ( Int& riIcdComp, UInt uiCtx )
{
  UInt uiSymbol;

  xReadExGolombIcd( uiSymbol, &m_cCUIcdSCModel.get( 0, uiCtx, 3 ), 3 );
  uiSymbol++;

  UInt uiSign;
  m_pcTDecBinIf->decodeBinEP( uiSign );

  riIcdComp = ( 0 != uiSign ) ? -(Int)uiSymbol : (Int)uiSymbol;

  return;
}

Void TDecSbac::xReadExGolombIcd( UInt& ruiSymbol, ContextModel* pcSCModel, UInt uiMaxBin )
{
  UInt uiSymbol;

  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] );

  if (!ruiSymbol) { return; }

  m_pcTDecBinIf->decodeBin( uiSymbol, pcSCModel[1] );

  ruiSymbol = 1;

  if (!uiSymbol)  { return; }

  pcSCModel += 2;
  UInt uiCount = 2;

  do
  {
    if( uiMaxBin == uiCount )
    {
      pcSCModel++;
    }
    m_pcTDecBinIf->decodeBin( uiSymbol, *pcSCModel );
    uiCount++;
  }
  while( uiSymbol && (uiCount != 8));

  ruiSymbol = uiCount-1;

  if( uiSymbol )
  {
    xReadEpExGolomb( uiSymbol, 3 );
    ruiSymbol += uiSymbol+1;
  }

  return;
}
#endif

Void TDecSbac::xReadExGolombMvd( UInt& ruiSymbol, ContextModel* pcSCModel, UInt uiMaxBin )
{
  UInt uiSymbol;

  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] );

  if (!ruiSymbol) { return; }

  m_pcTDecBinIf->decodeBin( uiSymbol, pcSCModel[1] );

  ruiSymbol = 1;

  if (!uiSymbol)  { return; }

  pcSCModel += 2;
  UInt uiCount = 2;

  do
  {
    if( uiMaxBin == uiCount )
    {
      pcSCModel++;
    }
    m_pcTDecBinIf->decodeBin( uiSymbol, *pcSCModel );
    uiCount++;
  }
  while( uiSymbol && (uiCount != 8));

  ruiSymbol = uiCount-1;

  if( uiSymbol )
  {
    xReadEpExGolomb( uiSymbol, 3 );
    ruiSymbol += uiSymbol+1;
  }

  return;
}

Void TDecSbac::xReadEpExGolomb( UInt& ruiSymbol, UInt uiCount )
{
  UInt uiSymbol = 0;
  UInt uiBit = 1;


  while( uiBit )
  {
    m_pcTDecBinIf->decodeBinEP( uiBit );
    uiSymbol += uiBit << uiCount++;
  }

  uiCount--;
  while( uiCount-- )
  {
    m_pcTDecBinIf->decodeBinEP( uiBit );
    uiSymbol += uiBit << uiCount;
  }

  ruiSymbol = uiSymbol;

  return;
}

Void TDecSbac::xReadUnarySymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset )
{
  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] );

  if (!ruiSymbol) { return; }

  UInt uiSymbol = 0;
  UInt uiCont;

  do
  {
    m_pcTDecBinIf->decodeBin( uiCont, pcSCModel[ iOffset ] );
    uiSymbol++;
  }
  while( uiCont );

  ruiSymbol = uiSymbol;
}

Void TDecSbac::xReadExGolombLevel( UInt& ruiSymbol, ContextModel& rcSCModel  )
{
  UInt uiSymbol;
  UInt uiCount = 0;
  do
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, rcSCModel );
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

Void TDecSbac::parseAlfCtrlDepth              ( UInt& ruiAlfCtrlDepth )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, m_cALFUvlcSCModel.get(0), 1, g_uiMaxCUDepth-1);
  ruiAlfCtrlDepth = uiSymbol;
}

#if HHI_ALF
Void TDecSbac::parseAlfCoeff( Int& riCoeff, Int iLength, Int iPos )
{
  UInt uiCode;
  Int  iSign;


  m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 0 ) );

  if ( uiCode == 0 )
  {
    riCoeff = 0;
    return;
  }

  // read sign
  m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 1 ) );

  if ( uiCode == 0 ) iSign =  1;
  else               iSign = -1;

  Int iM =4;
  if(iLength==3)
  {
    if(iPos == 0)
      iM = 4 ;
    else if(iPos == 1)
      iM = 1 ;
    else if(iPos == 2)
      iM = 2 ;
  }
  else if(iLength==5)
  {
    if(iPos == 0)
      iM = 3 ;
    else if(iPos == 1)
      iM = 5 ;
    else if(iPos == 2)
      iM = 1 ;
    else if(iPos == 3)
      iM = 3 ;
    else if(iPos == 4)
      iM = 2 ;
  }
  else if(iLength==7)
  {
    if(iPos == 0)
      iM = 3 ;
    else if(iPos == 1)
      iM = 4 ;
    else if(iPos == 2)
      iM = 5;
    else if(iPos == 3)
      iM = 1 ;
    else if(iPos == 4)
      iM = 3 ;
    else if(iPos == 5)
     iM = 3 ;
    else if(iPos == 6)
     iM = 2 ;
  }
  else if(iLength==9)
  {
    if(iPos == 0)
      iM = 2 ;
    else if(iPos == 1)
      iM = 4 ;
    else if(iPos == 2)
      iM = 4 ;
    else if(iPos == 3)
      iM = 5 ;
    else if(iPos == 4)
      iM = 1 ;
    else if(iPos == 5)
     iM = 3 ;
    else if(iPos == 6)
     iM = 3 ;
    else if(iPos == 7)
    iM = 3 ;
   else if(iPos == 8)
    iM = 2 ;
  }

  xReadEpExGolomb( uiCode, iM );

  riCoeff = uiCode*iSign;

}

Void TDecSbac::parseAlfDc( Int& riDc    )
{
  UInt uiCode;
  Int  iSign;


  m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 0 ) );

  if ( uiCode == 0 )
  {
    riDc = 0;
    return;
  }

  // read sign
  m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 1 ) );

  if ( uiCode == 0 ) iSign =  1;
  else               iSign = -1;


  xReadEpExGolomb( uiCode, 9 );

  riDc = uiCode*iSign;

}

Void TDecSbac::parseAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if (!m_bAlfCtrl)
    return;

  if( uiDepth > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, m_uiMaxAlfCtrlDepth))
  {
    return;
  }

  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUAlfCtrlFlagSCModel.get( 0, 0, pcCU->getCtxAlfCtrlFlag( uiAbsPartIdx ) ) );

  if (uiDepth > m_uiMaxAlfCtrlDepth)
  {
    pcCU->setAlfCtrlFlagSubParts( uiSymbol, uiAbsPartIdx, m_uiMaxAlfCtrlDepth);
  }
  else
  {
    pcCU->setAlfCtrlFlagSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
  }
}

Void TDecSbac::parseAlfQTCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if (!m_bAlfCtrl)
    return;

  if( uiDepth > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, m_uiMaxAlfCtrlDepth))
  {
    return;
  }

  UInt uiSymbol;
   m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUAlfCtrlFlagSCModel.get( 0, 0, 0 ) );
    pcCU->setAlfCtrlFlagSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
  }

Void TDecSbac::parseAlfQTSplitFlag( TComDataCU* pcCU ,UInt uiAbsPartIdx, UInt uiDepth, UInt uiMaxDepth )
{
  //if( uiDepth >= uiMaxDepth )
  if( uiDepth >= g_uiMaxCUDepth-g_uiAddCUDepth ) // fix HS
   {
     pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
     return;
   }

   UInt uiSymbol;
   m_pcTDecBinIf->decodeBin( uiSymbol, m_cALFSplitFlagSCModel.get( 0, 0, 0 ) );
   pcCU->setDepthSubParts( uiDepth + uiSymbol, uiAbsPartIdx );
   return;
}
#else
Void TDecSbac::parseAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if (!m_bAlfCtrl)
    return;

  if( uiDepth > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, m_uiMaxAlfCtrlDepth))
  {
    return;
  }

  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUAlfCtrlFlagSCModel.get( 0, 0, pcCU->getCtxAlfCtrlFlag( uiAbsPartIdx ) ) );

  if (uiDepth > m_uiMaxAlfCtrlDepth)
  {
    pcCU->setAlfCtrlFlagSubParts( uiSymbol, uiAbsPartIdx, m_uiMaxAlfCtrlDepth);
  }
  else
  {
    pcCU->setAlfCtrlFlagSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
  }
}
#endif

Void TDecSbac::parseSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if HHI_MRG && !SAMSUNG_MRG_SKIP_DIRECT
  if ( pcCU->getSlice()->getSPS()->getUseMRG() )
  {
    return;
  }
#endif

  if( pcCU->getSlice()->isIntra() )
  {
    return;
  }

  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUSkipFlagSCModel.get( 0, 0, pcCU->getCtxSkipFlag( uiAbsPartIdx ) ) );

  if( uiSymbol )
  {
    pcCU->setPredModeSubParts( MODE_SKIP,  uiAbsPartIdx, uiDepth );
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );

    TComMv cZeroMv(0,0);
    pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );

#ifdef DCM_PBIC
    if (pcCU->getSlice()->getSPS()->getUseIC())
    {
      TComIc cDefaultIc;
      pcCU->getCUIcField()->setAllIcd ( cDefaultIc, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    }
#endif

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
}

#if HHI_MRG
#if HHI_MRG_PU
Void TDecSbac::parseMergeFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx )
{
  UInt uiSymbol;
  UInt uiCtxIdx = pcCU->getCtxMergeFlag( uiAbsPartIdx );
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMergeFlagSCModel.get( 0, 0, uiCtxIdx ) );
  pcCU->setMergeFlagSubParts( uiSymbol ? true : false, uiAbsPartIdx, uiPUIdx, uiDepth );
}

Void TDecSbac::parseMergeIndex ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  UInt uiCtxIdx = pcCU->getCtxMergeIndex( uiAbsPartIdx );
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMergeIndexSCModel.get( 0, 0, uiCtxIdx ) );
  ruiMergeIndex = uiSymbol;
}
#else
Void TDecSbac::parseMergeFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMergeFlagSCModel.get( 0, 0, pcCU->getCtxMergeFlag( uiAbsPartIdx ) ) );
  pcCU->setMergeFlagSubParts( uiSymbol ? true : false, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::parseMergeIndex ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMergeIndexSCModel.get( 0, 0, pcCU->getCtxMergeIndex( uiAbsPartIdx ) ) );
  pcCU->setMergeIndexSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
}
#endif
#endif

Void TDecSbac::parseMVPIdx      ( TComDataCU* pcCU, Int& riMVPIdx, Int iMVPNum, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, m_cMVPIdxSCModel.get(0), 1, iMVPNum-1);
  riMVPIdx = uiSymbol;
 }

#ifdef DCM_PBIC
Void TDecSbac::parseICPIdx      ( TComDataCU* pcCU, Int& riICPIdx, Int iICPNum, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, m_cICPIdxSCModel.get(0), 1, iICPNum-1);
  riICPIdx = uiSymbol;
}
#endif

Void TDecSbac::parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
  {
    pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
    return;
  }

  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUSplitFlagSCModel.get( 0, 0, pcCU->getCtxSplitFlag( uiAbsPartIdx, uiDepth ) ) );
#if HHI_RQT
  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tSplitFlag\n" )
#endif
  pcCU->setDepthSubParts( uiDepth + uiSymbol, uiAbsPartIdx );

  return;
}

Void TDecSbac::parsePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if HHI_MRG && !HHI_MRG_PU
  if ( pcCU->getMergeFlag( uiAbsPartIdx ) )
  {
    return;
  }
#endif

  if ( pcCU->isSkip( uiAbsPartIdx ) )
  {
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    return;
  }

  UInt uiSymbol, uiMode = 0;
  PartSize eMode;

  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 0) );
    eMode = uiSymbol ? SIZE_2Nx2N : SIZE_NxN;
  }
  else
  {
#if HHI_RMP_SWITCH
    if ( !pcCU->getSlice()->getSPS()->getUseRMP() && !pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 0) );
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
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, ui) );
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
      if( g_uiMaxCUWidth>>uiDepth == 8 )
#endif
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 3) );
      if (uiSymbol == 0)
      {
        pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 4) );
        if (uiSymbol == 0)
          eMode = SIZE_2Nx2N;
      }
    }

#if HHI_RMP_SWITCH
    if ( pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) && pcCU->getSlice()->getSPS()->getUseRMP() )
#else
    if ( pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) )
#endif
    {
      if (eMode == SIZE_2NxN)
      {
        m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUYPosiSCModel.get( 0, 0, 0 ));
        if (uiSymbol == 0)
        {
          m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUYPosiSCModel.get( 0, 0, 1 ));
          eMode = (uiSymbol == 0? SIZE_2NxnU : SIZE_2NxnD);
        }
      }
      else if (eMode == SIZE_Nx2N)
      {
        m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUXPosiSCModel.get( 0, 0, 0 ));
        if (uiSymbol == 0)
        {
          m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUXPosiSCModel.get( 0, 0, 1 ));
          eMode = (uiSymbol == 0? SIZE_nLx2N : SIZE_nRx2N);
        }
      }
    }
#if HHI_RMP_SWITCH
    if ( pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) && !pcCU->getSlice()->getSPS()->getUseRMP() )
    {
      if ( eMode == SIZE_2NxN )
      {
        m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUYPosiSCModel.get( 0, 0, 1 ));
        eMode = (uiSymbol == 0? SIZE_2NxnU : SIZE_2NxnD);
      }
      else if ( eMode == SIZE_Nx2N )
      {
        m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUXPosiSCModel.get( 0, 0, 1 ));
        eMode = (uiSymbol == 0? SIZE_nLx2N : SIZE_nRx2N);
      }
    }
#endif

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

Void TDecSbac::parsePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if HHI_MRG && !HHI_MRG_PU
  if ( pcCU->getMergeFlag( uiAbsPartIdx ) )
  {
    pcCU->setPredModeSubParts( MODE_INTER, uiAbsPartIdx, uiDepth );
    return;
  }
#endif

  if( pcCU->getSlice()->isIntra() )
  {
    pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
    return;
  }

  UInt uiSymbol;
  Int  iPredMode = MODE_INTER;

#if HHI_MRG && !SAMSUNG_MRG_SKIP_DIRECT
  if ( !pcCU->getSlice()->getSPS()->getUseMRG() )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPredModeSCModel.get( 0, 0, 0 ) );
    if ( uiSymbol == 0 )
    {
      iPredMode = MODE_SKIP;
    }
  }
#else
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPredModeSCModel.get( 0, 0, 0 ) );
  if ( uiSymbol == 0 )
  {
    iPredMode = MODE_SKIP;
  }
#endif

  if ( pcCU->getSlice()->isInterB() )
  {
    pcCU->setPredModeSubParts( (PredMode)iPredMode, uiAbsPartIdx, uiDepth );
    return;
  }

  if ( iPredMode != MODE_SKIP )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPredModeSCModel.get( 0, 0, 1 ) );
    iPredMode += uiSymbol;
  }

  pcCU->setPredModeSubParts( (PredMode)iPredMode, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::parseIntraDirLuma  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  Int  uiIPredMode;

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 0) );

  if ( !uiSymbol )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
    uiIPredMode  = uiSymbol;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
    uiIPredMode |= uiSymbol << 1;

    // note: only 4 directions are allowed if PU size >= 16
    Int iPartWidth = pcCU->getWidth( uiAbsPartIdx );
    if ( pcCU->getPartitionSize( uiAbsPartIdx) == SIZE_NxN ) iPartWidth >>= 1;
    if ( iPartWidth <= 8 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
      uiIPredMode |= uiSymbol << 2;
    }

    uiIPredMode  = pcCU->revertIntraDirLuma( uiAbsPartIdx, uiIPredMode );
  }
  else
  {
    uiIPredMode  = pcCU->revertIntraDirLuma( uiAbsPartIdx, -1 );
  }
  pcCU->setLumaIntraDirSubParts( (UChar)uiIPredMode, uiAbsPartIdx, uiDepth );

  return;
}

#if ANG_INTRA
Void TDecSbac::parseIntraDirLumaAng  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  Int  uiIPredMode;
  Int  iMostProbable = pcCU->getMostProbableIntraDirLuma( uiAbsPartIdx );

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 0) );

  if ( uiSymbol )
    uiIPredMode = iMostProbable;
  else{
#if UNIFIED_DIRECTIONAL_INTRA
    Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
    if ( g_aucIntraModeBitsAng[iIntraIdx] < 6 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode  = uiSymbol;
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) { m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 1; }
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) { m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 2; }
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 4 ) { m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 3; }
    }
    else
    {
#endif
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode  = uiSymbol;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 1;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 2;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 3;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 4;

    if (uiIPredMode == 31){ // Escape coding for the last two modes
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
      uiIPredMode = uiSymbol ? 32 : 31;
    }
#if UNIFIED_DIRECTIONAL_INTRA
    }
#endif

    if (uiIPredMode >= iMostProbable)
      uiIPredMode++;
  }

  pcCU->setLumaIntraDirSubParts( (UChar)uiIPredMode, uiAbsPartIdx, uiDepth );
}
#endif

Void TDecSbac::parseIntraDirLumaAdi  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  Int  uiIPredMode;
  Int iIntraIdx= pcCU->getIntraSizeIdx(uiAbsPartIdx);

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 0) );

  if ( !uiSymbol )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
    uiIPredMode  = uiSymbol;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
    uiIPredMode |= uiSymbol << 1;
    if (g_aucIntraModeBits[iIntraIdx]>=4)
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
      uiIPredMode |= uiSymbol << 2;
      if (g_aucIntraModeBits[iIntraIdx]>=5)
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
        uiIPredMode |= uiSymbol << 3;
        if (g_aucIntraModeBits[iIntraIdx]>=6)
        {
           m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
           uiIPredMode |= uiSymbol << 4;
        }
      }
    }
    uiIPredMode = pcCU->revertIntraDirLumaAdi( pcCU,uiAbsPartIdx, (Int)( uiIPredMode  ) );
  }
  else
  {
    uiIPredMode  = pcCU->revertIntraDirLumaAdi( pcCU,uiAbsPartIdx, -1 );
  }
  pcCU->setLumaIntraDirSubParts( (UChar)uiIPredMode, uiAbsPartIdx, uiDepth );

  return;
}

#if HHI_AIS
Void TDecSbac::parseIntraFiltFlagLumaAdi  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;

#if ANG_INTRA
  UInt uiCtx = pcCU->angIntraEnabledPredPart( uiAbsPartIdx ) ?  pcCU->getCtxIntraFiltFlagLumaAng( uiAbsPartIdx ) : pcCU->getCtxIntraFiltFlagLuma( uiAbsPartIdx );
#else
  UInt uiCtx = pcCU->getCtxIntraFiltFlagLuma( uiAbsPartIdx );
#endif
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraFiltFlagSCModel.get( 0, 0, uiCtx ) );

  pcCU->setLumaIntraFiltFlagSubParts( uiSymbol != 0, uiAbsPartIdx, uiDepth );
  return;
}
#endif

Void TDecSbac::parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0, pcCU->getCtxIntraDirChroma( uiAbsPartIdx ) ) );

  if ( uiSymbol )
  {
    xReadUnaryMaxSymbol( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0 ) + 3, 0, 3 );
    uiSymbol++;
  }

  pcCU->setChromIntraDirSubParts( uiSymbol, uiAbsPartIdx, uiDepth );

  return;
}

Void TDecSbac::parseInterDir( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  UInt uiCtx = pcCU->getCtxInterDir( uiAbsPartIdx );

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUInterDirSCModel.get( 0, 0, uiCtx ) );

  if ( uiSymbol )
  {
    uiSymbol = 2;
  }
#if MS_NO_BACK_PRED_IN_B0
  else if ( pcCU->getSlice()->getNoBackPredFlag() )
  {
    uiSymbol = 0;
  }
#endif
  else
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUInterDirSCModel.get( 0, 0, 3 ) );
  }
  uiSymbol++;
  ruiInterDir = uiSymbol;
  return;
}

Void TDecSbac::parseRefFrmIdx( TComDataCU* pcCU, Int& riRefFrmIdx, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;
  UInt uiCtx = pcCU->getCtxRefIdx( uiAbsPartIdx, eRefList );

  m_pcTDecBinIf->decodeBin ( uiSymbol, m_cCURefPicSCModel.get( 0, 0, uiCtx ) );
  if ( uiSymbol )
  {
    xReadUnaryMaxSymbol( uiSymbol, &m_cCURefPicSCModel.get( 0, 0, 4 ), 1, pcCU->getSlice()->getNumRefIdx( eRefList )-2 );

    uiSymbol++;
  }
  riRefFrmIdx = uiSymbol;
  return;
}
#ifdef QC_AMVRES
Void TDecSbac::parseMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{  
  Int iHor, iVer;  
  UInt uiAbsPartIdxL, uiAbsPartIdxA; 
  Int iHorPred, iVerPred;
  Int MVres=0;
  TComDataCU* pcCUL   = pcCU->getPULeft ( uiAbsPartIdxL, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  TComDataCU* pcCUA   = pcCU->getPUAbove( uiAbsPartIdxA, pcCU->getZorderIdxInCU() + uiAbsPartIdx );

  TComCUMvField* pcCUMvFieldL = ( pcCUL == NULL || pcCUL->isIntra( uiAbsPartIdxL ) ) ? NULL : pcCUL->getCUMvField( eRefList );
  TComCUMvField* pcCUMvFieldA = ( pcCUA == NULL || pcCUA->isIntra( uiAbsPartIdxA ) ) ? NULL : pcCUA->getCUMvField( eRefList );

  if( pcCU->getSlice()->getSPS()->getUseAMVRes() )
  {
	  Int iL =   ( (pcCUMvFieldL == NULL) ? 1 : (Int)(pcCUMvFieldL->getMVRes(uiAbsPartIdxL)));
	  Int iV =   ( (pcCUMvFieldA == NULL) ? 1 : (Int)(pcCUMvFieldA->getMVRes(uiAbsPartIdxA)));  
	  xReadMvResFlag(MVres,iL+iV);
	  if (MVres)
	  {
		  iHorPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsHor()>>1 ) + 
					 ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsHor()>>1 );
		  iVerPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsVer()>>1 ) + 
					 ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsVer()>>1 );

		  xReadMvd( iHor, iHorPred, 0 );
		  xReadMvd( iVer, iVerPred, 1 );
		  iHor *=2;
		  iVer *=2;
	  } 
	  else
	  {
		  iHorPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsHor() ) + 
					 ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsHor() );
		  iVerPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsVer() ) + 
					 ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsVer() );
	  
		  xReadMvd( iHor, iHorPred, 0 );
		  xReadMvd( iVer, iVerPred, 1 );
	  }
	  pcCU->getCUMvField( eRefList )->setAllMVRes( MVres>0, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  }
  else
  {
	  iHorPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsHor() ) + 
				 ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsHor() );
	  iVerPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsVer() ) + 
				 ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsVer() );
	  xReadMvd( iHor, iHorPred, 0 );
      xReadMvd( iVer, iVerPred, 1 );
	  pcCU->getCUMvField( eRefList )->setAllMVRes( 0, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  }
  TComMv cTmpMv( 0, 0 );
  pcCU->getCUMvField( eRefList )->setAllMv( cTmpMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );

  TComMv cMv( iHor, iVer );
  pcCU->getCUMvField( eRefList )->setAllMvd( cMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );

  return;
}
#else
Void TDecSbac::parseMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
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

  xReadMvd( iHor, iHorPred, 0 );
  xReadMvd( iVer, iVerPred, 1 );

  // set mvd
  TComMv cMv( iHor, iVer );
  pcCU->getCUMvField( eRefList )->setAllMvd( cMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );

  return;
}
#endif

#ifdef DCM_PBIC
Void TDecSbac::parseMvdIcd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  Int iZeroPatt;
  Int aaiNZMv[2][2];
  Int aiNZIc[3];
  UInt uiZeroFlag;
  TComZeroTree* pcZTree;

#ifdef QC_AMVRES
  // Decode MV resolution flag (if necessary)
  Bool bMvResFlag[2] = {false, false};
  if ( (eRefList == REF_PIC_LIST_0) || (eRefList == REF_PIC_LIST_X) )
    bMvResFlag[REF_PIC_LIST_0] = xParseMvResFlag( pcCU, uiAbsPartIdx, uiPartIdx, uiDepth, REF_PIC_LIST_0 );
  if ( (eRefList == REF_PIC_LIST_1) || (eRefList == REF_PIC_LIST_X) )
    bMvResFlag[REF_PIC_LIST_1] = xParseMvResFlag( pcCU, uiAbsPartIdx, uiPartIdx, uiDepth, REF_PIC_LIST_1 );
#endif

  // Is any component non-zero?
  m_pcTDecBinIf->decodeBin( uiZeroFlag, *getZTreeCtx(IDX_ZEROFLAG) );
  if (uiZeroFlag)
  {
    TComMv cZeroMv;
    if (eRefList == REF_PIC_LIST_X)
    {
      pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvd( cZeroMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
      pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvd( cZeroMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
    }
    else
      pcCU->getCUMvField(       eRefList )->setAllMvd( cZeroMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );

    if (pcCU->getSlice()->getSPS()->getUseIC())
    {
      TComIc cDefaultIc;
      pcCU->getCUIcField()->setAllIcd( cDefaultIc, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
    }
    return;
  }

  // Identify the non-zero components
  if (eRefList == REF_PIC_LIST_X)
  {
    if (pcCU->getSlice()->getSPS()->getUseIC())
    {
      pcZTree   = pcCU->getSlice()->getZTree(IDX_ZTREE_MVDICDBI);
      iZeroPatt = parseZTree( pcZTree, getZTreeCtx(IDX_ZTREE_MVDICDBI) );
    }
    else
    {
      pcZTree   = pcCU->getSlice()->getZTree(IDX_ZTREE_MVDBI);
      iZeroPatt = parseZTree( pcZTree, getZTreeCtx(IDX_ZTREE_MVDBI) );
    }
    aaiNZMv[REF_PIC_LIST_0][0] = iZeroPatt & 1;
    aaiNZMv[REF_PIC_LIST_0][1] = iZeroPatt & 2;
    aaiNZMv[REF_PIC_LIST_1][0] = iZeroPatt & 4;
    aaiNZMv[REF_PIC_LIST_1][1] = iZeroPatt & 8;
    aiNZIc[0] = iZeroPatt & 16;
    aiNZIc[1] = iZeroPatt & 32;
    aiNZIc[2] = iZeroPatt & 64;
  }
  else
  {
    if (pcCU->getSlice()->getSPS()->getUseIC())
    {
      pcZTree   = pcCU->getSlice()->getZTree(IDX_ZTREE_MVDICDUNI);
      iZeroPatt = parseZTree( pcZTree, getZTreeCtx(IDX_ZTREE_MVDICDUNI) );
    }
    else
    {
      pcZTree   = pcCU->getSlice()->getZTree(IDX_ZTREE_MVDUNI);
      iZeroPatt = parseZTree( pcZTree, getZTreeCtx(IDX_ZTREE_MVDUNI) );
    }
    aaiNZMv[eRefList  ][0] = iZeroPatt & 1;
    aaiNZMv[eRefList  ][1] = iZeroPatt & 2;
    aaiNZMv[eRefList^1][0] = 0;
    aaiNZMv[eRefList^1][1] = 0;
    aiNZIc[0] = iZeroPatt & 4;
    aiNZIc[1] = 0;
    aiNZIc[2] = iZeroPatt & 8;
  }

  //Decode the non-zero components
  TComMv cMvd0, cMvd1;
  Int iHor, iVer;

  iHor = iVer = 0;
  if (aaiNZMv[REF_PIC_LIST_0][0]) xReadMvdNZ(iHor, 0);
  if (aaiNZMv[REF_PIC_LIST_0][1]) xReadMvdNZ(iVer, 1);
#ifdef QC_AMVRES
  if (bMvResFlag[REF_PIC_LIST_0] == true)
  {
    iHor *= 2;
    iVer *= 2;
  }
#endif
  cMvd0.set( iHor, iVer );

  iHor = iVer = 0;
  if (aaiNZMv[REF_PIC_LIST_1][0]) xReadMvdNZ(iHor, 0);
  if (aaiNZMv[REF_PIC_LIST_1][1]) xReadMvdNZ(iVer, 1);
#ifdef QC_AMVRES
  if (bMvResFlag[REF_PIC_LIST_1] == true)
  {
    iHor *= 2;
    iVer *= 2;
  }
#endif
  cMvd1.set( iHor, iVer );

  if (eRefList == REF_PIC_LIST_X)
  {
    pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvd( cMvd0, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
    pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvd( cMvd1, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  }
  else if (eRefList == REF_PIC_LIST_1)
    pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvd( cMvd1, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  else
    pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvd( cMvd0, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );

  if (pcCU->getSlice()->getSPS()->getUseIC())
  {
    TComIc cIcd;
    Int iParam0, iParam1, iParam2;
    iParam0 = iParam1 = iParam2 = 0;
    if (aiNZIc[0]) xReadIcdNZ(iParam0, 0);
    if (aiNZIc[1]) xReadIcdNZ(iParam1, 1);
    if (aiNZIc[2]) xReadIcdNZ(iParam2, 2);
    cIcd.setIcParam( iParam0, iParam1, iParam2 );
    pcCU->getCUIcField()->setAllIcd( cIcd, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  }
}

Int TDecSbac::parseZTree(TComZeroTree* pcZTree, ContextModel *pcCtxModel)
{
  Int iVal;
  Int iStack;
  Int iResult;
  Int iIsNotLeaf;
  UInt uiNodeBoth;
  UInt uiNodeLeft;
  UInt uiCtx;
  Int* piZTreeStructure = pcZTree->m_piStructure;
  Int* piLeafIdx        = pcZTree->m_piLeafIdx;

  iVal    = 1;
  iStack  = 0;
  iResult = 0;
  uiCtx   = 0;

  while (true)
  {
    STATE_1:
    iIsNotLeaf = *piZTreeStructure++;

    if (iIsNotLeaf == 0)
    {
      iResult += (1 << (*piLeafIdx));

      if (iStack == 0)
        break;

      piLeafIdx++;
      iVal = iStack & 1;
      iStack >>= 1;

      if (iVal == 0)
        goto STATE_0;
    }
    else
    {
      iStack <<= 1;

      m_pcTDecBinIf->decodeBin( uiNodeBoth, *(pcCtxModel+uiCtx) );
      if (uiNodeBoth)
        iStack++;
      else
      {
        m_pcTDecBinIf->decodeBin( uiNodeLeft, *(pcCtxModel+uiCtx+1) );
        if (uiNodeLeft == 0)
        {
          iStack++;
          uiCtx += 2;
          goto STATE_0;
        }
      }

      uiCtx += 2;
    }

    goto STATE_1;


    STATE_0:
    iIsNotLeaf = *piZTreeStructure++;

    if (iIsNotLeaf == 0)
    {
      if (iStack == 0)
        break;

      piLeafIdx++;
      iVal = iStack & 1;
      iStack >>= 1;

      if (iVal != 0)
        goto STATE_1;

    }
    else
    {
      iStack <<= 1;
      uiCtx += 2;
    }

    goto STATE_0;
  }

  return iResult;
}

ContextModel* TDecSbac::getZTreeCtx( Int iIdx )
{
  switch (iIdx)
  {
  case IDX_ZEROFLAG:
    return &m_cZTreeMV0SCModel.get(0, 0, 0);
    break;
  case IDX_ZTREE_MVDICDUNI:
    return &m_cZTreeMV1SCModel.get(0, 0, 0);
    break;
  case IDX_ZTREE_MVDICDBI:
    return &m_cZTreeMV2SCModel.get(0, 0, 0);
    break;
  case IDX_ZTREE_MVDUNI:
    return &m_cZTreeMV1SCModel.get(0, 0, 2);
    break;
  case IDX_ZTREE_MVDBI:
    return &m_cZTreeMV2SCModel.get(0, 0, 2);
    break;
  default:
    printf("Wrong index to TDecSbac::getZTreeCtx()\n");
    exit(0);
    break;
  }
}
#endif

#if HHI_RQT
Void TDecSbac::parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize )
{
  m_pcTDecBinIf->decodeBin( ruiSubdivFlag, m_cCUTransSubdivFlagSCModel.get( 0, 0, uiLog2TransformBlockSize ) );
  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseTransformSubdivFlag()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( ruiSubdivFlag )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiLog2TransformBlockSize )
  DTRACE_CABAC_T( "\n" )
}

#if HHI_RQT_ROOT
Void TDecSbac::parseQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& uiQtRootCbf )
{
  UInt uiSymbol;
  const UInt uiCtx = pcCU->getCtxQtRootCbf( uiAbsPartIdx );
  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtRootCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )

  uiQtRootCbf = uiSymbol;
}
#endif
#endif

Void TDecSbac::parseTransformIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiTrLevel = 0;

  UInt uiWidthInBit  = g_aucConvertToBit[pcCU->getWidth(uiAbsPartIdx)]+2;
  UInt uiTrSizeInBit = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxTrSize()]+2;
  uiTrLevel          = uiWidthInBit >= uiTrSizeInBit ? uiWidthInBit - uiTrSizeInBit : 0;

  UInt uiMinTrDepth = pcCU->getSlice()->getSPS()->getMinTrDepth() + uiTrLevel;
  UInt uiMaxTrDepth = pcCU->getSlice()->getSPS()->getMaxTrDepth() + uiTrLevel;

  if ( uiMinTrDepth == uiMaxTrDepth )
  {
    pcCU->setTrIdxSubParts( uiMinTrDepth, uiAbsPartIdx, uiDepth );
    return;
  }

  UInt uiTrIdx;
  m_pcTDecBinIf->decodeBin( uiTrIdx, m_cCUTransIdxSCModel.get( 0, 0, pcCU->getCtxTransIdx( uiAbsPartIdx ) ) );

  if ( !uiTrIdx )
  {
    uiTrIdx = uiTrIdx + uiMinTrDepth;
    pcCU->setTrIdxSubParts( uiTrIdx, uiAbsPartIdx, uiDepth );
    return;
  }

  if (pcCU->getPartitionSize(uiAbsPartIdx) >= SIZE_2NxnU && pcCU->getPartitionSize(uiAbsPartIdx) <= SIZE_nRx2N && uiMinTrDepth == 0 && uiMaxTrDepth == 1)
  {
    uiTrIdx++;

    ///Maybe unnecessary///
    UInt      uiWidth      = pcCU->getWidth ( uiAbsPartIdx );
    while((uiWidth>>uiTrIdx) < (g_uiMaxCUWidth>>g_uiMaxCUDepth)) uiTrIdx--;
    ////////////////////////

    uiTrIdx = uiTrIdx + uiMinTrDepth;
    pcCU->setTrIdxSubParts( uiTrIdx, uiAbsPartIdx, uiDepth );
    return;
  }

  UInt uiSymbol;
  Int  iCount = 1;
  while( ++iCount <= (Int)( uiMaxTrDepth - uiMinTrDepth ) )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUTransIdxSCModel.get( 0, 0, 3 ) );
    if ( uiSymbol == 0 )
    {
      uiTrIdx = uiTrIdx + uiMinTrDepth;
      pcCU->setTrIdxSubParts( uiTrIdx, uiAbsPartIdx, uiDepth );
      return;
    }
    uiTrIdx += uiSymbol;
  }

  uiTrIdx = uiTrIdx + uiMinTrDepth;

  pcCU->setTrIdxSubParts( uiTrIdx, uiAbsPartIdx, uiDepth );

  return;
}

Void TDecSbac::parseDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiDQp;
  Int  iDQp;

  m_pcTDecBinIf->decodeBin( uiDQp, m_cCUDeltaQpSCModel.get( 0, 0, 0 ) );

  if ( uiDQp == 0 )
  {
    uiDQp = pcCU->getSlice()->getSliceQp();
  }
  else
  {
    xReadUnarySymbol( uiDQp, &m_cCUDeltaQpSCModel.get( 0, 0, 2 ), 1 );
    iDQp = ( uiDQp + 2 ) / 2;

    if ( uiDQp & 1 )
    {
      iDQp = -iDQp;
    }
    uiDQp = pcCU->getSlice()->getSliceQp() + iDQp;
  }

  pcCU->setQPSubParts( uiDQp, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::parseCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
#if HHI_RQT
  if( pcCU->getSlice()->getSPS()->getQuadtreeTUFlag() )
  {
#if HHI_RQT_INTRA
    return;
#else
    if( !pcCU->isIntra( uiAbsPartIdx ) )
    {
      return;
    }
#endif
  }
#endif
  UInt uiSymbol;
  UInt uiCbf = pcCU->getCbf( uiAbsPartIdx, eType );
  UInt uiCtx = pcCU->getCtxCbf( uiAbsPartIdx, eType, uiTrDepth );

  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUCbfSCModel.get( 0, eType == TEXT_LUMA ? 0 : 1, 3 - uiCtx ) );
  pcCU->setCbfSubParts( uiCbf | ( uiSymbol << uiTrDepth ), eType, uiAbsPartIdx, uiDepth );

  if( !uiSymbol )
  {
    m_uiLastDQpNonZero = 0;
  }

  return;
}

#if HHI_RQT
Void TDecSbac::parseQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
  UInt uiSymbol;
  const UInt uiCtx = pcCU->getCtxQtCbf( uiAbsPartIdx, eType, uiTrDepth );
  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUQtCbfSCModel.get( 0, eType ? eType - 1: eType, uiCtx ) );

  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tetype=" )
  DTRACE_CABAC_V( eType )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )

  pcCU->setCbfSubParts( uiSymbol << uiTrDepth, eType, uiAbsPartIdx, uiDepth );
}
#endif

#if LCEC_CBP_YUV_ROOT
Void TDecSbac::parseBlockCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth, UInt uiQPartNum )
{
  return;
}
#endif

Void TDecSbac::parseCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
#if HHI_RQT
  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseCoeffNxN()\teType=" )
  DTRACE_CABAC_V( eTType )
  DTRACE_CABAC_T( "\twidth=" )
  DTRACE_CABAC_V( uiWidth )
  DTRACE_CABAC_T( "\theight=" )
  DTRACE_CABAC_V( uiHeight )
  DTRACE_CABAC_T( "\tdepth=" )
  DTRACE_CABAC_V( uiDepth )
  DTRACE_CABAC_T( "\tabspartidx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\ttoCU-X=" )
  DTRACE_CABAC_V( pcCU->getCUPelX() )
  DTRACE_CABAC_T( "\ttoCU-Y=" )
  DTRACE_CABAC_V( pcCU->getCUPelY() )
  DTRACE_CABAC_T( "\tCU-addr=" )
  DTRACE_CABAC_V(  pcCU->getAddr() )
  DTRACE_CABAC_T( "\tinCU-X=" )
  DTRACE_CABAC_V( g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_T( "\tinCU-Y=" )
  DTRACE_CABAC_V( g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_T( "\tpredmode=" )
  DTRACE_CABAC_V(  pcCU->getPredictionMode( uiAbsPartIdx ) )
  DTRACE_CABAC_T( "\n" )
#endif

#if HHI_TRANSFORM_CODING
  if( uiWidth > pcCU->getSlice()->getSPS()->getMaxTrSize() )
  {
    uiWidth  = pcCU->getSlice()->getSPS()->getMaxTrSize();
    uiHeight = pcCU->getSlice()->getSPS()->getMaxTrSize();
  }

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

  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : ( eTType == TEXT_NONE ? TEXT_NONE : TEXT_CHROMA );

  //----- parse significance map -----
  const UInt  uiMaxNumCoeff     = uiWidth*uiHeight;
  const UInt  uiMaxNumCoeffM1   = uiMaxNumCoeff - 1;
  const UInt  uiLog2BlockSize   = g_aucConvertToBit[ uiWidth ] + 2;
  UInt        uiDownLeft        = 1;
  UInt        uiNumSigTopRight  = 0;
  UInt        uiNumSigBotLeft   = 0;
  bool        bLastReceived     = false;
#if QC_MDDT//ADAPTIVE_SCAN
  const UInt*  pucScan;
  const UInt*  pucScanX;
  const UInt*  pucScanY;
  UInt *scanStats;
  UInt uiMode;
	int indexROT;
  if(pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA && (uiWidth == 4 || uiWidth == 8 /*|| uiWidth==16 || uiWidth==32 || uiWidth==64*/))
  {
    uiMode = pcCU->getLumaIntraDir(uiAbsPartIdx);
	  indexROT = pcCU->getROTindex(uiAbsPartIdx);
	//int scan_index;
#if ROT_CHECK
    if(uiWidth == 4 && indexROT == 0)
#else
    if(uiWidth == 4)
#endif
    {
       UInt uiPredMode = g_aucIntra9Mode[uiMode];
       pucScan = scanOrder4x4[uiPredMode]; pucScanX = scanOrder4x4X[uiPredMode]; pucScanY = scanOrder4x4Y[uiPredMode];

       scanStats = scanStats4x4[uiPredMode]; update4x4Count[uiPredMode]++;
    }
#if ROT_CHECK
    else if(uiWidth == 8 && indexROT == 0)
#else
    else if(uiWidth == 8)
#endif
    {
      UInt uiPredMode = ((1 << (pcCU->getIntraSizeIdx( uiAbsPartIdx ) + 1)) != uiWidth) ?  g_aucIntra9Mode[uiMode]: g_aucAngIntra9Mode[uiMode];
      pucScan = scanOrder8x8[uiPredMode]; pucScanX = scanOrder8x8X[uiPredMode]; pucScanY = scanOrder8x8Y[uiPredMode];
 
      scanStats = scanStats8x8[uiPredMode]; update8x8Count[uiPredMode]++;
    }
	/*else if(uiWidth == 16)
    {
	  scan_index = LUT16x16[indexROT][uiMode];
      pucScan = scanOrder16x16[scan_index]; pucScanX = scanOrder16x16X[scan_index]; pucScanY = scanOrder16x16Y[scan_index];
      scanStats = scanStats16x16[scan_index];
    }
    else if(uiWidth == 32)
    {
	  scan_index = LUT32x32[indexROT][uiMode];
      pucScan = scanOrder32x32[scan_index]; pucScanX = scanOrder32x32X[scan_index]; pucScanY = scanOrder32x32Y[scan_index];
      scanStats = scanStats32x32[scan_index];
    }
    else if(uiWidth == 64)
    {
	  scan_index = LUT64x64[indexROT][uiMode];
      pucScan = scanOrder64x64[scan_index]; pucScanX = scanOrder64x64X[scan_index]; pucScanY = scanOrder64x64Y[scan_index];
      scanStats = scanStats64x64[scan_index];
    }*/
    else
    {
      //printf("uiWidth = %d is not supported!\n", uiWidth);
      //exit(1);
    }
  }
#endif
  for( UInt uiScanPos = 0; uiScanPos < uiMaxNumCoeffM1; uiScanPos++ )
  {
    UInt  uiBlkPos  = g_auiSigLastScan[ uiLog2BlockSize ][ uiDownLeft ][ uiScanPos ];
#if QC_MDDT
    if(pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA && (uiWidth == 4 || uiWidth == 8 /*|| uiWidth==16 || uiWidth==32 || uiWidth==64*/))
    {
#if ROT_CHECK
      if(/*ipredmode<=8 && */indexROT == 0)
        uiBlkPos = pucScan[uiScanPos];
#else
        uiBlkPos = pucScan[uiScanPos];
#endif
    }
#endif
    UInt  uiPosY    = uiBlkPos >> uiLog2BlockSize;
    UInt  uiPosX    = uiBlkPos - ( uiPosY << uiLog2BlockSize );

    //===== code significance flag =====
    UInt  uiSig     = 0;
    UInt  uiCtxSig  = TComTrQuant::getSigCtxInc( pcCoef, uiPosX, uiPosY, uiLog2BlockSize, uiWidth, ( uiDownLeft > 0 ) );
    m_pcTDecBinIf->decodeBin( uiSig, m_cCuCtxModSig.get( uiCTXIdx, eTType, uiCtxSig ) );

    if( uiSig )
    {
#if QC_MDDT//ADAPTIVE_SCAN
#if ROT_CHECK
      if(pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA  && ((uiWidth == 4 && /*ipredmode<=8&&*/indexROT == 0)|| (uiWidth == 8 && /* ipredmode<=8 && */ indexROT == 0) /*|| uiWidth==16 || uiWidth==32 || uiWidth==64*/)) 
#else
     if(pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA  && (uiWidth == 4 || uiWidth == 8))
#endif
      {
        //scanStats[pucScan[ui]]++;
        scanStats[uiScanPos]++;
      }
#endif
      pcCoef[ uiBlkPos ] = 1;

      if( uiPosX > uiPosY )
      {
        uiNumSigTopRight++;
      }
      else if( uiPosY > uiPosX )
      {
        uiNumSigBotLeft ++;
      }

      //===== code last flag =====
      UInt  uiLast     = 0;
      UInt  uiCtxLast  = TComTrQuant::getLastCtxInc( uiPosX, uiPosY, uiLog2BlockSize );
      m_pcTDecBinIf->decodeBin( uiLast, m_cCuCtxModLast.get( uiCTXIdx, eTType, uiCtxLast ) );

      if( uiLast )
      {
        bLastReceived = true;
        break;
      }
    }

    //===== update scan direction =====
#if !HHI_DISABLE_SCAN
    if( ( uiDownLeft == 1 && ( uiPosX == 0 || uiPosY == uiHeight - 1 ) ) ||
        ( uiDownLeft == 0 && ( uiPosY == 0 || uiPosX == uiWidth  - 1 ) )   )
    {
      uiDownLeft = ( uiNumSigTopRight >= uiNumSigBotLeft ? 1 : 0 );
    }
#else
		if( uiScanPos && 
			( ( uiDownLeft == 1 && ( uiPosX == 0 || uiPosY == uiHeight - 1 ) ) ||
        ( uiDownLeft == 0 && ( uiPosY == 0 || uiPosX == uiWidth  - 1 ) )   ) )
    {
      uiDownLeft = 1 - uiDownLeft;
    }
#endif
  }
  if( !bLastReceived )
  {
    pcCoef[ uiMaxNumCoeffM1 ] = 1;
  }

#if TI_SIGN_BIN0_PCP
  /*
   * Sign and bin0 PCP (Section 3.2 and 3.3 of JCTVC-B088)
   */
  Int  c1, c2;
  UInt uiSign;
  UInt uiLevel;
  UInt uiPrevAbsGreOne     = 0;
  const UInt uiNumOfSets   = 4;
  const UInt uiNum4x4Blk   = max<UInt>( 1, uiMaxNumCoeff / 16 );

  if ( uiLog2BlockSize > 2 )
  {
    Bool bFirstBlock = true;

    for ( UInt uiSubBlk = 0; uiSubBlk < uiNum4x4Blk; uiSubBlk++ )
    {
      UInt uiCtxSet    = 0;
      UInt uiSubNumSig = 0;
      UInt uiSubLog2   = uiLog2BlockSize - 2;
      UInt uiSubPosX   = 0;
      UInt uiSubPosY   = 0;

      if ( uiSubLog2 > 1 )
      {
#if HHI_RQT
        uiSubPosX = g_auiFrameScanX[ uiSubLog2 - 1 ][ uiSubBlk ] * 4;
        uiSubPosY = g_auiFrameScanY[ uiSubLog2 - 1 ][ uiSubBlk ] * 4;
#else
        uiSubPosX = g_auiFrameScanX[ uiSubLog2 - 2 ][ uiSubBlk ] * 4;
        uiSubPosY = g_auiFrameScanY[ uiSubLog2 - 2 ][ uiSubBlk ] * 4;
#endif
      }
      else
      {
        uiSubPosX = ( uiSubBlk < 2      ) ? 0 : 1;
        uiSubPosY = ( uiSubBlk % 2 == 0 ) ? 0 : 1;
        uiSubPosX *= 4;
        uiSubPosY *= 4;
      }

      TCoeff* piCurr = &pcCoef[ uiSubPosX + uiSubPosY * uiWidth ];

      for ( UInt uiY = 0; uiY < 4; uiY++ )
      {
        for ( UInt uiX = 0; uiX < 4; uiX++ )
        {
          if( piCurr[ uiX ] )
          {
            uiSubNumSig++;
          }
        }
        piCurr += uiWidth;
      }

      if ( uiSubNumSig > 0 )
      {
        c1 = 1;
        c2 = 0;

        if ( bFirstBlock )
        {
          bFirstBlock = false;
          uiCtxSet = uiNumOfSets + 1;
        }
        else
        {
          //uiCtxSet = ( ( uiGreOne * uiEqRangeSize ) >> 8 ) + 1;
          uiCtxSet = uiPrevAbsGreOne / uiNumOfSets + 1;
          uiPrevAbsGreOne = 0;
        }

		// Decode level Bin0 plane
        for ( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
#if HHI_RQT
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
#else
          UInt  uiBlkPos  = g_auiFrameScanXY[ 0 ][ 15 - uiScanPos ];
#endif
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = (uiSubPosY + uiPosY) * uiWidth + uiSubPosX + uiPosX;

          uiLevel = pcCoef[ uiIndex ];

          if( uiLevel )
          {
            UInt uiCtx = min<UInt>(c1, 4);
            m_pcTDecBinIf->decodeBin( uiLevel, m_cCuCtxModAbsGreOne.get( 0, eTType, (uiCtxSet * 5) + uiCtx ) );
            if( uiLevel == 1 )
            {
              c1    = 0;
              uiLevel = 2;
            }
            else if( c1 )
            {
              c1++;
              uiLevel++;
            }
            else
            {
              uiLevel++;
            }
            pcCoef[ uiIndex ] = uiLevel;
          }
        }

		// Decode other bins of level 
        for ( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
#if HHI_RQT
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
#else
          UInt  uiBlkPos  = g_auiFrameScanXY[ 0 ][ 15 - uiScanPos ];
#endif
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = (uiSubPosY + uiPosY) * uiWidth + uiSubPosX + uiPosX;

          uiLevel = pcCoef[ uiIndex ];

          if( uiLevel )
          {
            if( uiLevel == 2 )
            {
              UInt uiCtx = min<UInt>(c2, 4);
              c2++;
              uiPrevAbsGreOne++;
              xReadExGolombLevel( uiLevel, m_cCuCtxModCoeffLevelM1.get( 0, eTType, (uiCtxSet * 5) + uiCtx ) );
              uiLevel += 2;
            }
            pcCoef[ uiIndex ] = uiLevel;
          }
        }

		// Decode sign plane
        for ( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
#if HHI_RQT
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
#else
          UInt  uiBlkPos  = g_auiFrameScanXY[ 0 ][ 15 - uiScanPos ];
#endif
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = (uiSubPosY + uiPosY) * uiWidth + uiSubPosX + uiPosX;

          uiLevel = pcCoef[ uiIndex ];

          if( uiLevel )
          {
            m_pcTDecBinIf->decodeBinEP( uiSign );
            pcCoef[ uiIndex ] = ( uiSign ? -(Int)uiLevel : (Int)uiLevel );
          }
        }
      }
    }
  }
  else
  {
    c1 = 1;
    c2 = 0;

    // Decode level Bin0 plane
    for ( UInt uiScanPos = 0; uiScanPos < uiWidth*uiHeight; uiScanPos++ )
    {
#if HHI_RQT
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] + 1 ][ uiWidth*uiHeight - uiScanPos - 1 ];
#else
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] ][ uiWidth*uiHeight - uiScanPos - 1 ];
#endif
      uiLevel = pcCoef[ uiIndex ];

      if( uiLevel )
      {
        UInt uiCtx = min<UInt>(c1, 4);
        m_pcTDecBinIf->decodeBin( uiLevel, m_cCuCtxModAbsGreOne.get( 0, eTType, uiCtx ) );
        if( uiLevel == 1 )
        {
          c1    = 0;
          uiLevel = 2;
        }
        else if( c1 )
        {
          c1++;
          uiLevel++;
        }
        else
        {
          uiLevel++;
        }
        pcCoef[ uiIndex ] = uiLevel;
      }
    }

	// Decode other bins of level 
    for ( UInt uiScanPos = 0; uiScanPos < uiWidth*uiHeight; uiScanPos++ )
    {
#if HHI_RQT
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] + 1 ][ uiWidth*uiHeight - uiScanPos - 1 ];
#else
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] ][ uiWidth*uiHeight - uiScanPos - 1 ];
#endif

      uiLevel = pcCoef[ uiIndex ];

      if( uiLevel )
      {
        if( uiLevel == 2 )
        {
          UInt uiCtx = min<UInt>(c2, 4);
          c2++;
          xReadExGolombLevel( uiLevel, m_cCuCtxModCoeffLevelM1.get( 0, eTType, uiCtx ) );
          uiLevel += 2;
        }
        pcCoef[ uiIndex ] = uiLevel;
      }
    }

	// Decode sign plane
    for ( UInt uiScanPos = 0; uiScanPos < uiWidth*uiHeight; uiScanPos++ )
    {
#if HHI_RQT
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] + 1 ][ uiWidth*uiHeight - uiScanPos - 1 ];
#else
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] ][ uiWidth*uiHeight - uiScanPos - 1 ];
#endif
      uiLevel = pcCoef[ uiIndex ];

      if( uiLevel )
      {
        m_pcTDecBinIf->decodeBinEP( uiSign );
        pcCoef[ uiIndex ] = ( uiSign ? -(Int)uiLevel : (Int)uiLevel );
      }
    }
  }
#else 
  Int  c1, c2;
  UInt uiSign;
  UInt uiLevel;
  UInt uiPrevAbsGreOne     = 0;
  const UInt uiNumOfSets   = 4;
  const UInt uiNum4x4Blk   = max<UInt>( 1, uiMaxNumCoeff / 16 );

  if ( uiLog2BlockSize > 2 )
  {
    Bool bFirstBlock = true;

    for ( UInt uiSubBlk = 0; uiSubBlk < uiNum4x4Blk; uiSubBlk++ )
    {
      UInt uiCtxSet    = 0;
      UInt uiSubNumSig = 0;
      UInt uiSubLog2   = uiLog2BlockSize - 2;
      UInt uiSubPosX   = 0;
      UInt uiSubPosY   = 0;

      if ( uiSubLog2 > 1 )
      {
#if HHI_RQT
        uiSubPosX = g_auiFrameScanX[ uiSubLog2 - 1 ][ uiSubBlk ] * 4;
        uiSubPosY = g_auiFrameScanY[ uiSubLog2 - 1 ][ uiSubBlk ] * 4;
#else
        uiSubPosX = g_auiFrameScanX[ uiSubLog2 - 2 ][ uiSubBlk ] * 4;
        uiSubPosY = g_auiFrameScanY[ uiSubLog2 - 2 ][ uiSubBlk ] * 4;
#endif
      }
      else
      {
        uiSubPosX = ( uiSubBlk < 2      ) ? 0 : 1;
        uiSubPosY = ( uiSubBlk % 2 == 0 ) ? 0 : 1;
        uiSubPosX *= 4;
        uiSubPosY *= 4;
      }

      TCoeff* piCurr = &pcCoef[ uiSubPosX + uiSubPosY * uiWidth ];

      for ( UInt uiY = 0; uiY < 4; uiY++ )
      {
        for ( UInt uiX = 0; uiX < 4; uiX++ )
        {
          if( piCurr[ uiX ] )
          {
            uiSubNumSig++;
          }
        }
        piCurr += uiWidth;
      }

      if ( uiSubNumSig > 0 )
      {
        c1 = 1;
        c2 = 0;

        if ( bFirstBlock )
        {
          bFirstBlock = false;
          uiCtxSet = uiNumOfSets + 1;
        }
        else
        {
          //uiCtxSet = ( ( uiGreOne * uiEqRangeSize ) >> 8 ) + 1;
          uiCtxSet = uiPrevAbsGreOne / uiNumOfSets + 1;
          uiPrevAbsGreOne = 0;
        }

        for ( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
#if HHI_RQT
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
#else
          UInt  uiBlkPos  = g_auiFrameScanXY[ 0 ][ 15 - uiScanPos ];
#endif
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = (uiSubPosY + uiPosY) * uiWidth + uiSubPosX + uiPosX;

          uiLevel = pcCoef[ uiIndex ];

          if( uiLevel )
          {
            UInt uiCtx = min<UInt>(c1, 4);
            m_pcTDecBinIf->decodeBin( uiLevel, m_cCuCtxModAbsGreOne.get( 0, eTType, (uiCtxSet * 5) + uiCtx ) );

            if( uiLevel == 1 )
            {
              uiCtx = min<UInt>(c2, 4);
              c1    = 0;
              c2++;
              uiPrevAbsGreOne++;
              xReadExGolombLevel( uiLevel, m_cCuCtxModCoeffLevelM1.get( 0, eTType, (uiCtxSet * 5) + uiCtx ) );
              uiLevel += 2;
            }
            else if( c1 )
            {
              c1++;
              uiLevel++;
            }
            else
            {
              uiLevel++;
            }
            m_pcTDecBinIf->decodeBinEP( uiSign );
            pcCoef[ uiIndex ] = ( uiSign ? -(Int)uiLevel : (Int)uiLevel );
          }
        }
      }
    }
  }
  else
  {
    c1 = 1;
    c2 = 0;

    for ( UInt uiScanPos = 0; uiScanPos < uiWidth*uiHeight; uiScanPos++ )
    {
#if HHI_RQT
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] + 1 ][ uiWidth*uiHeight - uiScanPos - 1 ];
#else
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] ][ uiWidth*uiHeight - uiScanPos - 1 ];
#endif

      uiLevel = pcCoef[ uiIndex ];

      if( uiLevel )
      {
        UInt uiCtx = min<UInt>(c1, 4);
        m_pcTDecBinIf->decodeBin( uiLevel, m_cCuCtxModAbsGreOne.get( 0, eTType, uiCtx ) );

        if( uiLevel == 1 )
        {
          uiCtx = min<UInt>(c2, 4);
          c1    = 0;
          c2++;
          xReadExGolombLevel( uiLevel, m_cCuCtxModCoeffLevelM1.get( 0, eTType, uiCtx ) );
          uiLevel += 2;
        }
        else if( c1 )
        {
          c1++;
          uiLevel++;
        }
        else
        {
          uiLevel++;
        }
        m_pcTDecBinIf->decodeBinEP( uiSign );
        pcCoef[ uiIndex ] = ( uiSign ? -(Int)uiLevel : (Int)uiLevel );
      }
    }
  }
#endif

  return;
#else
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

  if( uiWidth > pcCU->getSlice()->getSPS()->getMaxTrSize() )
  {
    uiWidth  = pcCU->getSlice()->getSPS()->getMaxTrSize();
    uiHeight = pcCU->getSlice()->getSPS()->getMaxTrSize();
  }
  UInt uiSize   = uiWidth*uiHeight;

	// pre-compute shift from size
	UInt uiShift  = ( g_aucConvertToBit[ uiWidth ] + 2 ) + ( g_aucConvertToBit[ uiHeight ] + 2 );

  // point to coefficient
  TCoeff* piCoeff = pcCoef;
  UInt ui;
  UInt uiCtxSize  = 64;
	const Int* pos2ctx_map  = pos2ctx_map8x8;
  const Int* pos2ctx_last = pos2ctx_last8x8;

  if ( uiWidth < 8 )
  {
    pos2ctx_map  = pos2ctx_nomap;
    pos2ctx_last = pos2ctx_nomap;
    uiCtxSize    = 16;
  }

  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : ( eTType == TEXT_NONE ? TEXT_NONE : TEXT_CHROMA );

  UInt uiSig, uiLast, uiCtx, uiCtxOffst;
  uiLast = 0;

#if QC_MDDT == 0//ADAPTIVE_SCAN == 0
  m_pcTDecBinIf->decodeBin( uiSig, m_cCUMapSCModel.get( uiCTXIdx, eTType, pos2ctx_map[ 0 ] ) );

  piCoeff[0] = uiSig;

  if( uiSig )
    m_pcTDecBinIf->decodeBin( uiLast, m_cCULastSCModel.get( uiCTXIdx, eTType, pos2ctx_last[ 0 ] ) );
#endif
  // initialize scan
	const UInt*  pucScan;
  const UInt*  pucScanX;
  const UInt*  pucScanY;

  UInt uiConvBit = g_aucConvertToBit[ uiWidth ];
#if HHI_RQT
  pucScan  = g_auiFrameScanXY[ uiConvBit + 1 ]; 
  pucScanX = g_auiFrameScanX [ uiConvBit + 1 ];
  pucScanY = g_auiFrameScanY [ uiConvBit + 1 ];
#else
  switch(uiWidth)
  {
  case  2:
    {
      static UInt uiScanOrder [4] = {0, 1, 2, 3};
      static UInt uiScanOrderX[4] = {0, 1, 0, 1};
      static UInt uiScanOrderY[4] = {0, 0, 1, 1};
      pucScan  = &uiScanOrder [0];
      pucScanX = &uiScanOrderX[0];
      pucScanY = &uiScanOrderY[0];
      break;
    }
  case  4:
  case  8: pucScan = g_auiFrameScanXY[ uiConvBit ]; pucScanX = g_auiFrameScanX[uiConvBit]; pucScanY = g_auiFrameScanY[uiConvBit]; break;
  case 16:
  case 32:
  case 64: pucScan = g_auiFrameScanXY[ uiConvBit ]; pucScanX = g_auiFrameScanX[uiConvBit]; pucScanY = g_auiFrameScanY[uiConvBit]; break;
  default: pucScan = g_auiFrameScanXY[ uiConvBit ]; pucScanX = g_auiFrameScanX[uiConvBit]; pucScanY = g_auiFrameScanY[uiConvBit]; break;
  }
#endif

#if QC_MDDT//ADAPTIVE_SCAN
  UInt *scanStats;
  UInt uiPredMode;
	int indexROT;

  UInt uiMode;
  if(pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA )
  {
    uiMode = pcCU->getLumaIntraDir(uiAbsPartIdx);
	indexROT = pcCU->getROTindex(uiAbsPartIdx);
	int scan_index;
#if ROT_CHECK
    if(uiWidth == 4 && indexROT == 0)
#else
    if(uiWidth == 4)
#endif
    {
       uiPredMode = g_aucIntra9Mode[uiMode];
       pucScan = scanOrder4x4[uiPredMode]; pucScanX = scanOrder4x4X[uiPredMode]; pucScanY = scanOrder4x4Y[uiPredMode];

       scanStats = scanStats4x4[uiPredMode]; update4x4Count[uiPredMode]++;
    }
#if ROT_CHECK
    else if(uiWidth == 8 && indexROT == 0)
#else
    else if(uiWidth == 8)
#endif
    {
      uiPredMode = ((1 << (pcCU->getIntraSizeIdx( uiAbsPartIdx ) + 1)) != uiWidth) ?  g_aucIntra9Mode[uiMode]: g_aucAngIntra9Mode[uiMode];
      pucScan = scanOrder8x8[uiPredMode]; pucScanX = scanOrder8x8X[uiPredMode]; pucScanY = scanOrder8x8Y[uiPredMode];
 
      scanStats = scanStats8x8[uiPredMode]; update8x8Count[uiPredMode]++;
    }
	else if(uiWidth == 16)
    {
	  scan_index = LUT16x16[indexROT][uiMode];
      pucScan = scanOrder16x16[scan_index]; pucScanX = scanOrder16x16X[scan_index]; pucScanY = scanOrder16x16Y[scan_index];
      scanStats = scanStats16x16[scan_index];
    }
    else if(uiWidth == 32)
    {
	  scan_index = LUT32x32[indexROT][uiMode];
      pucScan = scanOrder32x32[scan_index]; pucScanX = scanOrder32x32X[scan_index]; pucScanY = scanOrder32x32Y[scan_index];
      scanStats = scanStats32x32[scan_index];
    }
    else if(uiWidth == 64)
    {
	  scan_index = LUT64x64[indexROT][uiMode];
      pucScan = scanOrder64x64[scan_index]; pucScanX = scanOrder64x64X[scan_index]; pucScanY = scanOrder64x64Y[scan_index];
      scanStats = scanStats64x64[scan_index];
    }
    else
    {
      //printf("uiWidth = %d is not supported!\n", uiWidth);
      //exit(1);
    }
  }
#endif
	//----- parse significance map -----
  // DC is decoded in the beginning
	Int		uiScanXShift = g_aucConvertToBit[ uiWidth  >> 3 ] + 2;
	Int		uiScanYShift = g_aucConvertToBit[ uiHeight >> 3 ] + 2;
  UInt	uiXX, uiYY;

	ContextModel* pMapCCModel  = m_cCUMapSCModel.get ( uiCTXIdx, eTType );
	ContextModel* pLastCCModel = m_cCULastSCModel.get( uiCTXIdx, eTType );

#if QC_MDDT//ADAPTIVE_SCAN
	for ( ui=0, uiCtxOffst=0 ; ui<(uiSize-1) && !uiLast ; ui++, uiCtxOffst+=uiCtxSize ) // if last coeff is reached, it has to be significant
#else
    for ( ui=1, uiCtxOffst=uiCtxSize ; ui<(uiSize-1) && !uiLast ; ui++, uiCtxOffst+=uiCtxSize ) // if last coeff is reached, it has to be significant
#endif
	{
#if HHI_RQT
    if (uiCtxSize < uiSize)
#else
    if (uiCtxSize != uiSize)
#endif
    {
      uiXX  = pucScanX[ui] >> uiScanXShift;
      uiYY  = pucScanY[ui] >> uiScanYShift;
      uiCtx = g_auiAntiScan8[ ( uiYY << 3 )+uiXX];
    }
    else
		{
      uiCtx = uiCtxOffst >> uiShift;
		}

    // SBAC_SEP
#if QC_MDDT//ADAPTIVE_SCAN
#if ROT_CHECK
     if(pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA && uiWidth == 8 && indexROT == 0)//!bRD &&  eTType == TEXT_LUMA)
#else
     if(pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA && uiWidth == 8 )
#endif
     {
       assert(uiCtx == ui);

#ifdef QC_CTX
	   m_pcTDecBinIf->decodeBin( uiSig, pMapCCModel[ pos2ctx_map[ uiCtx ] ] );
#else
	   m_pcTDecBinIf->decodeBin( uiSig, pMapCCModel[ pos2ctx_map[ g_auiAntiScan8[pucScan[uiCtx] ]] ] );
#endif

     }
     else
#endif
		m_pcTDecBinIf->decodeBin( uiSig, pMapCCModel[ pos2ctx_map[ uiCtx ] ] );

		piCoeff[ pucScan[ui] ] = uiSig;

		if( uiSig )
		{
#if QC_MDDT//ADAPTIVE_SCAN
#if ROT_CHECK
         if(pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA  && ((uiWidth == 4 && indexROT == 0)|| (uiWidth == 8 && indexROT == 0) || uiWidth==16 || uiWidth==32 || uiWidth==64))
#else
          if(pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA )
#endif
          {
            //scanStats[pucScan[ui]]++;
            scanStats[ui]++;
          }
#endif
      uiCtx       = uiCtxOffst >> uiShift;

      // SBAC_SEP
			m_pcTDecBinIf->decodeBin( uiLast, pLastCCModel[ pos2ctx_last[ uiCtx ] ] );
			if( uiLast )
			{
				break;
			}
		}
	}

  //--- last coefficient must be significant if no last symbol was received ---
  if ( ui == uiSize - 1 )
  {
    piCoeff[ pucScan[ui] ] = 1;
  }

	ContextModel* pOnesCCModel = m_cCUOneSCModel.get( uiCTXIdx, eTType );
	ContextModel* pAbsCCModel  = m_cCUAbsSCModel.get( uiCTXIdx, eTType );

  Int   c1 = 1;
  Int   c2 = 0;

  ui++;
  while( (ui--) != 0 )
  {
    Int   iIndex  = pucScan[ui];
    UInt  uiCoeff = piCoeff[ iIndex ];

    if( uiCoeff )
    {
      uiCtx = Min (c1,4);

      // SBAC_SEP
      m_pcTDecBinIf->decodeBin( uiCoeff, pOnesCCModel[ uiCtx ] );
      if( 1 == uiCoeff )
      {
        uiCtx = Min (c2,4);
        // SBAC_SEP
				xReadExGolombLevel( uiCoeff, pAbsCCModel[ uiCtx ] );
        uiCoeff += 2;

        c1=0;
        c2++;
      }
      else if (c1)
      {
        uiCoeff++;
        c1++;
      }
      else
      {
        uiCoeff++;
      }

      UInt uiSign;
      m_pcTDecBinIf->decodeBinEP( uiSign );
      piCoeff[iIndex] = ( uiSign ? -(Int)uiCoeff : (Int)uiCoeff );
    }
  }
  return;
#endif
}

Void TDecSbac::parseAlfFlag (UInt& ruiVal)
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cALFFlagSCModel.get( 0, 0, 0 ) );

  ruiVal = uiSymbol;
}

#if TSB_ALF_HEADER
Void TDecSbac::parseAlfFlagNum( UInt& ruiVal, UInt minValue, UInt depth )
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
  ruiVal = 0;
  UInt uiBit;
  if(uiLength)
  {
    while( uiLength-- )
    {
      m_pcTDecBinIf->decodeBinEP( uiBit );
      ruiVal += uiBit << uiLength;
    }
  }
  else
  {
    ruiVal = 0;
  }
  ruiVal += minValue;
}

Void TDecSbac::parseAlfCtrlFlag( UInt &ruiAlfCtrlFlag )
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUAlfCtrlFlagSCModel.get( 0, 0, 0 ) );
  ruiAlfCtrlFlag = uiSymbol;
}
#endif

Void TDecSbac::parseAlfUvlc (UInt& ruiVal)
{
  UInt uiCode;
  Int  i;

  m_pcTDecBinIf->decodeBin( uiCode, m_cALFUvlcSCModel.get( 0, 0, 0 ) );
  if ( uiCode == 0 )
  {
    ruiVal = 0;
    return;
  }

  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBin( uiCode, m_cALFUvlcSCModel.get( 0, 0, 1 ) );
    if ( uiCode == 0 ) break;
    i++;
  }

  ruiVal = i;
}

Void TDecSbac::parseAlfSvlc (Int&  riVal)
{
  UInt uiCode;
  Int  iSign;
  Int  i;

  m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 0 ) );

  if ( uiCode == 0 )
  {
    riVal = 0;
    return;
  }

  // read sign
  m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 1 ) );

  if ( uiCode == 0 ) iSign =  1;
  else               iSign = -1;

  // read magnitude
  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 2 ) );
    if ( uiCode == 0 ) break;
    i++;
  }

  riVal = i*iSign;
}

