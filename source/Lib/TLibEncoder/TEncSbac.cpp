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

/** \file     TEncSbac.cpp
    \brief    SBAC encoder class
*/

#include "TEncTop.h"
#include "TEncSbac.h"

#include <map>

extern UChar  stateMappingTable[113];
extern Int entropyBits[128];

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSbac::TEncSbac()
  // new structure here
  : m_cCUSplitFlagSCModel     ( 1,             1,               NUM_SPLIT_FLAG_CTX            )
  , m_cCUSkipFlagSCModel      ( 1,             1,               NUM_SKIP_FLAG_CTX             )
#if HHI_MRG
  , m_cCUMergeFlagSCModel     ( 1,             1,               NUM_MERGE_FLAG_CTX            )
  , m_cCUMergeIndexSCModel    ( 1,             1,               NUM_MERGE_INDEX_CTX           )
#endif
  , m_cCUAlfCtrlFlagSCModel   ( 1,             1,               NUM_ALF_CTRL_FLAG_CTX         )
  , m_cCUPartSizeSCModel      ( 1,             1,               NUM_PART_SIZE_CTX             )
  , m_cCUXPosiSCModel         ( 1,             1,               NUM_CU_X_POS_CTX              )
  , m_cCUYPosiSCModel         ( 1,             1,               NUM_CU_Y_POS_CTX              )
  , m_cCUPredModeSCModel      ( 1,             1,               NUM_PRED_MODE_CTX             )
  , m_cCUIntraPredSCModel     ( 1,             1,               NUM_ADI_CTX                   )
  , m_cCUChromaPredSCModel    ( 1,             1,               NUM_CHROMA_PRED_CTX           )
  , m_cCUInterDirSCModel      ( 1,             1,               NUM_INTER_DIR_CTX             )
  , m_cCUMvdSCModel           ( 1,             2,               NUM_MV_RES_CTX                )
  , m_cCURefPicSCModel        ( 1,             1,               NUM_REF_NO_CTX                )
  , m_cCUTransSubdivFlagSCModel( 1,          1,               NUM_TRANS_SUBDIV_FLAG_CTX )
  , m_cCUQtRootCbfSCModel     ( 1,             1,               NUM_QT_ROOT_CBF_CTX   )
  , m_cCUTransIdxSCModel      ( 1,             1,               NUM_TRANS_IDX_CTX             )
  , m_cCUDeltaQpSCModel       ( 1,             1,               NUM_DELTA_QP_CTX              )
  , m_cCUCbfSCModel           ( 1,             2,               NUM_CBF_CTX                   )

  , m_cCUQtCbfSCModel       ( 1,             3,               NUM_QT_CBF_CTX        )

  , m_cCuCtxModSig            ( MAX_CU_DEPTH,  2,               NUM_SIG_FLAG_CTX              )
  , m_cCuCtxModLast           ( MAX_CU_DEPTH,  2,               NUM_LAST_FLAG_CTX             )
  , m_cCuCtxModAbsGreOne      ( 1,             2,               NUM_ABS_GREATER_ONE_CTX       )
  , m_cCuCtxModCoeffLevelM1   ( 1,             2,               NUM_COEFF_LEVEL_MINUS_ONE_CTX )

  , m_cMVPIdxSCModel          ( 1,             1,               NUM_MVP_IDX_CTX               )
  , m_cCUROTindexSCModel      ( 1,             1,               NUM_ROT_IDX_CTX               )
  , m_cCUCIPflagCCModel       ( 1,             1,               NUM_CIP_FLAG_CTX              )
  , m_cALFFlagSCModel         ( 1,             1,               NUM_ALF_FLAG_CTX              )
  , m_cALFUvlcSCModel         ( 1,             1,               NUM_ALF_UVLC_CTX              )
  , m_cALFSvlcSCModel         ( 1,             1,               NUM_ALF_SVLC_CTX              )
#if PLANAR_INTRA
  , m_cPlanarIntraSCModel     ( 1,             1,               NUM_PLANAR_INTRA_CTX          )
#endif
{
  m_pcBitIf = 0;
  m_pcSlice = 0;
  m_pcBinIf = 0;

  m_uiCoeffCost = 0;
  m_bAlfCtrl = false;
  m_uiMaxAlfCtrlDepth = 0;
}

TEncSbac::~TEncSbac()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSbac::resetEntropy           ()
{
  Int  iQp              = m_pcSlice->getSliceQp();
  SliceType eSliceType  = m_pcSlice->getSliceType();

  m_cCUSplitFlagSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_SPLIT_FLAG );

  m_cCUSkipFlagSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_SKIP_FLAG );
  m_cCUAlfCtrlFlagSCModel.initBuffer  ( eSliceType, iQp, (Short*)INIT_SKIP_FLAG );
#if HHI_MRG
  m_cCUMergeFlagSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_MERGE_FLAG );
  m_cCUMergeIndexSCModel.initBuffer   ( eSliceType, iQp, (Short*)INIT_MERGE_INDEX );
#endif
  m_cCUPartSizeSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_PART_SIZE );
  m_cCUXPosiSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_CU_X_POS );
  m_cCUYPosiSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_CU_Y_POS );
  m_cCUPredModeSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_INTRA_PRED_MODE );
  m_cCUChromaPredSCModel.initBuffer   ( eSliceType, iQp, (Short*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_MVD );
  m_cCURefPicSCModel.initBuffer       ( eSliceType, iQp, (Short*)INIT_REF_PIC );
  m_cCUDeltaQpSCModel.initBuffer      ( eSliceType, iQp, (Short*)INIT_DQP );
  m_cCUCbfSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_CBF );
  m_cCUQtCbfSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_QT_CBF );
  m_cCUQtRootCbfSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_QT_ROOT_CBF );

  m_cCuCtxModSig.initBuffer           ( eSliceType, iQp, (Short*)INIT_SIG_FLAG );
  m_cCuCtxModLast.initBuffer          ( eSliceType, iQp, (Short*)INIT_LAST_FLAG );
  m_cCuCtxModAbsGreOne.initBuffer     ( eSliceType, iQp, (Short*)INIT_ABS_GREATER_ONE_FLAG );
  m_cCuCtxModCoeffLevelM1.initBuffer  ( eSliceType, iQp, (Short*)INIT_COEFF_LEVEL_MINUS_ONE_FLAG );

  m_cMVPIdxSCModel.initBuffer         ( eSliceType, iQp, (Short*)INIT_MVP_IDX );
  m_cCUROTindexSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_ROT_IDX );
  m_cCUCIPflagCCModel.initBuffer      ( eSliceType, iQp, (Short*)INIT_CIP_IDX );

  m_cALFFlagSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_ALF_FLAG );
  m_cALFUvlcSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_ALF_UVLC );
  m_cALFSvlcSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_ALF_SVLC );
  m_cCUTransSubdivFlagSCModel.initBuffer( eSliceType, iQp, (Short*)INIT_TRANS_SUBDIV_FLAG );
  m_cCUTransIdxSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_TRANS_IDX );

#if PLANAR_INTRA
  m_cPlanarIntraSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_PLANAR_INTRA );
#endif

  // new structure
  m_uiLastQp          = iQp;

  m_pcBinIf->start();

  return;
}

UInt TEncSbac::xGetCTXIdxFromWidth( Int iWidth )
{
  UInt uiCTXIdx;

  switch( iWidth )
  {
  case  2: uiCTXIdx = 6; break;
  case  4: uiCTXIdx = 5; break;
  case  8: uiCTXIdx = 4; break;
  case 16: uiCTXIdx = 3; break;
  case 32: uiCTXIdx = 2; break;
  case 64: uiCTXIdx = 1; break;
  default: uiCTXIdx = 0; break;
  }

  return uiCTXIdx;
}

Void TEncSbac::codeSPS( TComSPS* pcSPS )
{
  assert (0);
  return;
}

Void TEncSbac::codePPS( TComPPS* pcPPS )
{
  assert (0);
  return;
}

Void TEncSbac::codeSliceHeader( TComSlice* pcSlice )
{
  assert (0);
  return;
}

Void TEncSbac::codeTerminatingBit( UInt uilsLast )
{
  m_pcBinIf->encodeBinTrm( uilsLast );
}

Void TEncSbac::codeSliceFinish()
{
  m_pcBinIf->finish();
}



Void TEncSbac::xWriteUnarySymbol( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset )
{
  m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[0] );

  if( 0 == uiSymbol)
  {
    return;
  }

  while( uiSymbol-- )
  {
    m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[ iOffset ] );
  }

  return;
}

Void TEncSbac::xWriteUnaryMaxSymbol( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol )
{
  m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[ 0 ] );

  if ( uiSymbol == 0 )
  {
    return;
  }

  Bool bCodeLast = ( uiMaxSymbol > uiSymbol );

  while( --uiSymbol )
  {
    m_pcBinIf->encodeBin( 1, pcSCModel[ iOffset ] );
  }
  if( bCodeLast )
  {
    m_pcBinIf->encodeBin( 0, pcSCModel[ iOffset ] );
  }

  return;
}

Void TEncSbac::xWriteEpExGolomb( UInt uiSymbol, UInt uiCount )
{
  while( uiSymbol >= (UInt)(1<<uiCount) )
  {
    m_pcBinIf->encodeBinEP( 1 );
    uiSymbol -= 1<<uiCount;
    uiCount  ++;
  }
  m_pcBinIf->encodeBinEP( 0 );
  while( uiCount-- )
  {
    m_pcBinIf->encodeBinEP( (uiSymbol>>uiCount) & 1 );
  }

  return;
}

Void TEncSbac::xWriteExGolombLevel( UInt uiSymbol, ContextModel& rcSCModel  )
{
  if( uiSymbol )
  {
    m_pcBinIf->encodeBin( 1, rcSCModel );
    UInt uiCount = 0;
    Bool bNoExGo = (uiSymbol < 13);

    while( --uiSymbol && ++uiCount < 13 )
    {
      m_pcBinIf->encodeBin( 1, rcSCModel );
    }
    if( bNoExGo )
    {
      m_pcBinIf->encodeBin( 0, rcSCModel );
    }
    else
    {
      xWriteEpExGolomb( uiSymbol, 0 );
    }
  }
  else
  {
    m_pcBinIf->encodeBin( 0, rcSCModel );
  }

  return;
}


// SBAC RD
Void  TEncSbac::load ( TEncSbac* pScr)
{
  this->xCopyFrom(pScr);
}

Void  TEncSbac::store( TEncSbac* pDest)
{
  pDest->xCopyFrom( this );
}


Void TEncSbac::xCopyFrom( TEncSbac* pSrc )
{
  m_pcBinIf->copyState( pSrc->m_pcBinIf );

  this->m_uiCoeffCost        = pSrc->m_uiCoeffCost;
  this->m_uiLastQp           = pSrc->m_uiLastQp;

  this->m_cCUSplitFlagSCModel     .copyFrom( &pSrc->m_cCUSplitFlagSCModel     );
  this->m_cCUSkipFlagSCModel      .copyFrom( &pSrc->m_cCUSkipFlagSCModel      );
#if HHI_MRG
  this->m_cCUMergeFlagSCModel     .copyFrom( &pSrc->m_cCUMergeFlagSCModel     );
  this->m_cCUMergeIndexSCModel    .copyFrom( &pSrc->m_cCUMergeIndexSCModel    );
#endif
  this->m_cCUPartSizeSCModel      .copyFrom( &pSrc->m_cCUPartSizeSCModel      );
  this->m_cCUPredModeSCModel      .copyFrom( &pSrc->m_cCUPredModeSCModel      );
  this->m_cCUIntraPredSCModel     .copyFrom( &pSrc->m_cCUIntraPredSCModel     );
  this->m_cCUChromaPredSCModel.copyFrom( &pSrc->m_cCUChromaPredSCModel  );
  this->m_cCUDeltaQpSCModel   .copyFrom( &pSrc->m_cCUDeltaQpSCModel     );
  this->m_cCUInterDirSCModel  .copyFrom( &pSrc->m_cCUInterDirSCModel    );
  this->m_cCURefPicSCModel    .copyFrom( &pSrc->m_cCURefPicSCModel      );
  this->m_cCUMvdSCModel       .copyFrom( &pSrc->m_cCUMvdSCModel         );
  this->m_cCUCbfSCModel       .copyFrom( &pSrc->m_cCUCbfSCModel         );
  this->m_cCUQtCbfSCModel     .copyFrom( &pSrc->m_cCUQtCbfSCModel       );
  this->m_cCUTransSubdivFlagSCModel.copyFrom( &pSrc->m_cCUTransSubdivFlagSCModel );
  this->m_cCUQtRootCbfSCModel .copyFrom( &pSrc->m_cCUQtRootCbfSCModel   );
  this->m_cCUTransIdxSCModel  .copyFrom( &pSrc->m_cCUTransIdxSCModel    );

  this->m_cCuCtxModSig         .copyFrom( &pSrc->m_cCuCtxModSig          );
  this->m_cCuCtxModLast        .copyFrom( &pSrc->m_cCuCtxModLast         );
  this->m_cCuCtxModAbsGreOne   .copyFrom( &pSrc->m_cCuCtxModAbsGreOne    );
  this->m_cCuCtxModCoeffLevelM1.copyFrom( &pSrc->m_cCuCtxModCoeffLevelM1 );

  this->m_cMVPIdxSCModel      .copyFrom( &pSrc->m_cMVPIdxSCModel        );

  this->m_cCUROTindexSCModel  .copyFrom( &pSrc->m_cCUROTindexSCModel    );
  this->m_cCUCIPflagCCModel   .copyFrom( &pSrc->m_cCUCIPflagCCModel     );
  this->m_cCUXPosiSCModel     .copyFrom( &pSrc->m_cCUXPosiSCModel       );
  this->m_cCUYPosiSCModel     .copyFrom( &pSrc->m_cCUXPosiSCModel       );
}

// CIP
Void TEncSbac::codeCIPflag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  Int CIPflag = pcCU->getCIPflag   ( uiAbsPartIdx );
  Int iCtx    = pcCU->getCtxCIPFlag( uiAbsPartIdx );

  m_pcBinIf->encodeBin( (CIPflag) ? 1 : 0, m_cCUCIPflagCCModel.get( 0, 0, iCtx ) );
}

Void TEncSbac::codeROTindex( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
    uiAbsPartIdx = 0;

  Int indexROT = pcCU->getROTindex( uiAbsPartIdx );
  Int dictSize = ROT_DICT;

  switch (dictSize)
  {
   case 9:
    {
      m_pcBinIf->encodeBin( indexROT> 0 ? 0 : 1 , m_cCUROTindexSCModel.get( 0, 0, 0 ) );
      if ( indexROT > 0 )
      {
        indexROT = indexROT-1;
        m_pcBinIf->encodeBin( (indexROT & 0x01),      m_cCUROTindexSCModel.get( 0, 0, 1 ) );
        m_pcBinIf->encodeBin( (indexROT & 0x02) >> 1, m_cCUROTindexSCModel.get( 0, 0, 2 ) );
        m_pcBinIf->encodeBin( (indexROT & 0x04) >> 2, m_cCUROTindexSCModel.get( 0, 0, 2 ) );
      }
    }
    break;
  case 4:
    {
      m_pcBinIf->encodeBin( (indexROT & 0x01),      m_cCUROTindexSCModel.get( 0, 0, 0 ) );
      m_pcBinIf->encodeBin( (indexROT & 0x02) >> 1, m_cCUROTindexSCModel.get( 0, 0, 1 ) );
    }
    break;
  case 2:
    {
      m_pcBinIf->encodeBin( indexROT> 0 ? 0 : 1 , m_cCUROTindexSCModel.get( 0, 0, 0 ) );
    }
    break;
  case 5:
    {
      m_pcBinIf->encodeBin( indexROT> 0 ? 0 : 1 , m_cCUROTindexSCModel.get( 0, 0, 0 ) );
      if ( indexROT > 0 )
      {
        indexROT = indexROT-1;
        m_pcBinIf->encodeBin( (indexROT & 0x01),      m_cCUROTindexSCModel.get( 0, 0, 1 ) );
        m_pcBinIf->encodeBin( (indexROT & 0x02) >> 1, m_cCUROTindexSCModel.get( 0, 0, 2 ) );
      }
    }
    break;
  case 1:
    {
    }
    break;
  }
  return;
}

Void TEncSbac::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iSymbol = pcCU->getMVPIdx(eRefList, uiAbsPartIdx);
  Int iNum    = pcCU->getMVPNum(eRefList, uiAbsPartIdx);

  xWriteUnaryMaxSymbol(iSymbol, m_cMVPIdxSCModel.get(0), 1, iNum-1);
}

Void TEncSbac::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  PartSize eSize         = pcCU->getPartitionSize( uiAbsPartIdx );

  if ( pcCU->getSlice()->isInterB() && pcCU->isIntra( uiAbsPartIdx ) )
  {
    m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
#if HHI_RMP_SWITCH
    if( pcCU->getSlice()->getSPS()->getUseRMP() )
#endif
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 1) );
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 2) );
    }
#if HHI_DISABLE_INTER_NxN_SPLIT
    if( pcCU->getWidth( uiAbsPartIdx ) == 8 )
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 3) );
    }
#else
    m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 3) );
#endif
    m_pcBinIf->encodeBin( (eSize == SIZE_2Nx2N? 0 : 1), m_cCUPartSizeSCModel.get( 0, 0, 4) );
    return;
  }

  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
    m_pcBinIf->encodeBin( eSize == SIZE_2Nx2N? 1 : 0, m_cCUPartSizeSCModel.get( 0, 0, 0 ) );
    return;
  }

  switch(eSize)
  {
  case SIZE_2Nx2N:
    {
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      break;
    }
  case SIZE_2NxN:
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 1) );

      break;
    }
  case SIZE_Nx2N:
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 1) );
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 2) );

      break;
    }
  case SIZE_NxN:
    {
#if HHI_DISABLE_INTER_NxN_SPLIT
      if( pcCU->getWidth( uiAbsPartIdx ) == 8 )
#endif
      {
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
#if HHI_RMP_SWITCH
        if( pcCU->getSlice()->getSPS()->getUseRMP() )
#endif
        {
          m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 1) );
          m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 2) );
        }

        if (pcCU->getSlice()->isInterB())
        {
          m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 3) );
        }
      }
      break;
    }
  default:
    {
      assert(0);
    }
  }
}

Void TEncSbac::codePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  Int iPredMode = pcCU->getPredictionMode( uiAbsPartIdx );

#if HHI_MRG && !SAMSUNG_MRG_SKIP_DIRECT
  if ( !pcCU->getSlice()->getSPS()->getUseMRG() )
  {
    m_pcBinIf->encodeBin( iPredMode == MODE_SKIP ? 0 : 1, m_cCUPredModeSCModel.get( 0, 0, 0 ) );
  }
#else
  m_pcBinIf->encodeBin( iPredMode == MODE_SKIP ? 0 : 1, m_cCUPredModeSCModel.get( 0, 0, 0 ) );
#endif

  if (pcCU->getSlice()->isInterB() )
  {
    return;
  }

  if ( iPredMode != MODE_SKIP )
  {
    m_pcBinIf->encodeBin( iPredMode == MODE_INTER ? 0 : 1, m_cCUPredModeSCModel.get( 0, 0, 1 ) );
  }
}

Void TEncSbac::codeAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if (!m_bAlfCtrl)
    return;

  if( pcCU->getDepth(uiAbsPartIdx) > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, m_uiMaxAlfCtrlDepth))
  {
    return;
  }

  // get context function is here
  UInt uiSymbol = pcCU->getAlfCtrlFlag( uiAbsPartIdx ) ? 1 : 0;

  m_pcBinIf->encodeBin( uiSymbol, m_cCUAlfCtrlFlagSCModel.get( 0, 0, pcCU->getCtxAlfCtrlFlag( uiAbsPartIdx) ) );
}

Void TEncSbac::codeAlfCtrlDepth()
{
  if (!m_bAlfCtrl)
    return;

  UInt uiDepth = m_uiMaxAlfCtrlDepth;
  xWriteUnaryMaxSymbol(uiDepth, m_cALFUvlcSCModel.get(0), 1, g_uiMaxCUDepth-1);
}

Void TEncSbac::codeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  UInt uiSymbol = pcCU->isSkipped( uiAbsPartIdx ) ? 1 : 0;
  m_pcBinIf->encodeBin( uiSymbol, m_cCUSkipFlagSCModel.get( 0, 0, pcCU->getCtxSkipFlag( uiAbsPartIdx) ) );
}

#if HHI_MRG
Void TEncSbac::codeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiSymbol = pcCU->getMergeFlag( uiAbsPartIdx ) ? 1 : 0;
  m_pcBinIf->encodeBin( uiSymbol, m_cCUMergeFlagSCModel.get( 0, 0, pcCU->getCtxMergeFlag( uiAbsPartIdx ) ) );
}

Void TEncSbac::codeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiSymbol = pcCU->getMergeIndex( uiAbsPartIdx ) ? 1 : 0;
  m_pcBinIf->encodeBin( uiSymbol, m_cCUMergeIndexSCModel.get( 0, 0, pcCU->getCtxMergeIndex( uiAbsPartIdx ) ) );
}
#endif

Void TEncSbac::codeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    return;

  UInt uiCtx           = pcCU->getCtxSplitFlag( uiAbsPartIdx, uiDepth );
  UInt uiCurrSplitFlag = ( pcCU->getDepth( uiAbsPartIdx ) > uiDepth ) ? 1 : 0;

  assert( uiCtx < 3 );
  m_pcBinIf->encodeBin( uiCurrSplitFlag, m_cCUSplitFlagSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tSplitFlag\n" )
  return;
}

Void TEncSbac::codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  m_pcBinIf->encodeBin( uiSymbol, m_cCUTransSubdivFlagSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseTransformSubdivFlag()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\n" )
}

Void TEncSbac::codeTransformIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiTrLevel = 0;

  UInt uiWidthInBit  = g_aucConvertToBit[pcCU->getWidth(uiAbsPartIdx)]+2;
  UInt uiTrSizeInBit = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxTrSize()]+2;
  uiTrLevel          = uiWidthInBit >= uiTrSizeInBit ? uiWidthInBit - uiTrSizeInBit : 0;

  UInt uiMinTrDepth = pcCU->getSlice()->getSPS()->getMinTrDepth() + uiTrLevel;
  UInt uiMaxTrDepth = pcCU->getSlice()->getSPS()->getMaxTrDepth() + uiTrLevel;

  if ( uiMinTrDepth == uiMaxTrDepth )
  {
    return;
  }

  UInt uiSymbol = pcCU->getTransformIdx(uiAbsPartIdx) - uiMinTrDepth;

  m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, m_cCUTransIdxSCModel.get( 0, 0, pcCU->getCtxTransIdx( uiAbsPartIdx ) ) );

  if ( !uiSymbol )
  {
    return;
  }

  Int  iCount = 1;
  uiSymbol--;
  while( ++iCount <= (Int)( uiMaxTrDepth - uiMinTrDepth ) )
  {
    m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, m_cCUTransIdxSCModel.get( 0, 0, 3 ) );
    if ( uiSymbol == 0 )
    {
      return;
    }
    uiSymbol--;
  }

  return;
}

#if PLANAR_INTRA
Void TEncSbac::xPutPlanarBins( Int n, Int cn )
{
  UInt tmp  = 1<<(n-4);
  UInt code = tmp+cn%tmp;
  UInt len  = 1+(n-4)+(cn>>(n-4));
  Int  ctr;

  for( ctr = len-1; ctr >= 0; ctr-- )
    m_pcBinIf->encodeBinEP( (code & (1 << ctr)) >> ctr );
}

Void TEncSbac::xCodePlanarDelta( TComDataCU* pcCU, UInt uiAbsPartIdx , Int iDelta )
{
  /* Planar quantization
  Y        qY              cW
  0-3   :  0,1,2,3         0-3
  4-15  :  4,6,8..14       4-9
  16-63 : 18,22,26..62    10-21
  64-.. : 68,76...        22-
  */
  Bool bDeltaNegative = iDelta < 0 ? true : false;
  UInt uiDeltaAbs     = abs(iDelta);

  if( uiDeltaAbs < 4 )
    xPutPlanarBins( 5, uiDeltaAbs );
  else if( uiDeltaAbs < 16 )
    xPutPlanarBins( 5, (uiDeltaAbs>>1)+2 );
  else if( uiDeltaAbs < 64)
    xPutPlanarBins( 5, (uiDeltaAbs>>2)+6 );
  else
    xPutPlanarBins( 5, (uiDeltaAbs>>3)+14 );

  if(uiDeltaAbs > 0)
    m_pcBinIf->encodeBinEP( bDeltaNegative ? 1 : 0 );

}

Void TEncSbac::codePlanarInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if (pcCU->isIntra( uiAbsPartIdx ))
  {
    UInt uiPlanar = pcCU->getPlanarInfo(uiAbsPartIdx, PLANAR_FLAG);

    m_pcBinIf->encodeBin( uiPlanar, m_cPlanarIntraSCModel.get( 0, 0, 0 ) );

    if ( uiPlanar )
    {
      // Planar delta for Y
      xCodePlanarDelta( pcCU, uiAbsPartIdx, pcCU->getPlanarInfo(uiAbsPartIdx, PLANAR_DELTAY) );

      // Planar delta for U and V
      Int  iPlanarDeltaU = pcCU->getPlanarInfo(uiAbsPartIdx, PLANAR_DELTAU);
      Int  iPlanarDeltaV = pcCU->getPlanarInfo(uiAbsPartIdx, PLANAR_DELTAV);

      m_pcBinIf->encodeBin( ( iPlanarDeltaU == 0 && iPlanarDeltaV == 0 ) ? 1 : 0, m_cPlanarIntraSCModel.get( 0, 0, 1 ) );

      if ( iPlanarDeltaU != 0 || iPlanarDeltaV != 0 )
      {
        xCodePlanarDelta( pcCU, uiAbsPartIdx, iPlanarDeltaU );
        xCodePlanarDelta( pcCU, uiAbsPartIdx, iPlanarDeltaV );
      }
    }
  }
}
#endif

Void TEncSbac::codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiDir         = pcCU->getLumaIntraDir( uiAbsPartIdx );
  Int  iMostProbable = pcCU->getMostProbableIntraDirLuma( uiAbsPartIdx );

  if (uiDir == iMostProbable)
    m_pcBinIf->encodeBin( 1, m_cCUIntraPredSCModel.get( 0, 0, 0 ) );
  else{
    m_pcBinIf->encodeBin( 0, m_cCUIntraPredSCModel.get( 0, 0, 0 ) );
    uiDir = uiDir > iMostProbable ? uiDir - 1 : uiDir;
    Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
    if ( g_aucIntraModeBitsAng[iIntraIdx] < 6 )
    {
      m_pcBinIf->encodeBin((uiDir & 0x01), m_cCUIntraPredSCModel.get(0, 0, 1));
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) m_pcBinIf->encodeBin((uiDir & 0x02) >> 1, m_cCUIntraPredSCModel.get(0, 0, 1));
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) m_pcBinIf->encodeBin((uiDir & 0x04) >> 2, m_cCUIntraPredSCModel.get(0, 0, 1));
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 4 ) m_pcBinIf->encodeBin((uiDir & 0x08) >> 3, m_cCUIntraPredSCModel.get(0, 0, 1));
    }
    else
    if (uiDir < 31){ // uiDir is here 0...32, 5 bits for uiDir 0...30, 31 is an escape code for coding one more bit for 31 and 32
      m_pcBinIf->encodeBin((uiDir & 0x01),      m_cCUIntraPredSCModel.get(0, 0, 1));
      m_pcBinIf->encodeBin((uiDir & 0x02) >> 1, m_cCUIntraPredSCModel.get(0, 0, 1));
      m_pcBinIf->encodeBin((uiDir & 0x04) >> 2, m_cCUIntraPredSCModel.get(0, 0, 1));
      m_pcBinIf->encodeBin((uiDir & 0x08) >> 3, m_cCUIntraPredSCModel.get(0, 0, 1));
      m_pcBinIf->encodeBin((uiDir & 0x10) >> 4, m_cCUIntraPredSCModel.get(0, 0, 1));
    }
    else{
      m_pcBinIf->encodeBin(1, m_cCUIntraPredSCModel.get(0, 0, 1));
      m_pcBinIf->encodeBin(1, m_cCUIntraPredSCModel.get(0, 0, 1));
      m_pcBinIf->encodeBin(1, m_cCUIntraPredSCModel.get(0, 0, 1));
      m_pcBinIf->encodeBin(1, m_cCUIntraPredSCModel.get(0, 0, 1));
      m_pcBinIf->encodeBin(1, m_cCUIntraPredSCModel.get(0, 0, 1));
      m_pcBinIf->encodeBin((uiDir == 32) ? 1 : 0, m_cCUIntraPredSCModel.get(0, 0, 1));
    }
  }

  return;
}

Void TEncSbac::codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiCtx            = pcCU->getCtxIntraDirChroma( uiAbsPartIdx );
  UInt uiIntraDirChroma = pcCU->getChromaIntraDir   ( uiAbsPartIdx );

  if ( 0 == uiIntraDirChroma )
  {
    m_pcBinIf->encodeBin( 0, m_cCUChromaPredSCModel.get( 0, 0, uiCtx ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cCUChromaPredSCModel.get( 0, 0, uiCtx ) );
    xWriteUnaryMaxSymbol( uiIntraDirChroma - 1, m_cCUChromaPredSCModel.get( 0, 0 ) + 3, 0, 3 );
  }

  return;
}

Void TEncSbac::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiInterDir = pcCU->getInterDir   ( uiAbsPartIdx );
  UInt uiCtx      = pcCU->getCtxInterDir( uiAbsPartIdx );
  uiInterDir--;
  m_pcBinIf->encodeBin( ( uiInterDir == 2 ? 1 : 0 ), m_cCUInterDirSCModel.get( 0, 0, uiCtx ) );

#if MS_NO_BACK_PRED_IN_B0
  if ( pcCU->getSlice()->getNoBackPredFlag() )
  {
    assert( uiInterDir != 1 );
    return;
  }
#endif

  if ( uiInterDir < 2 )
  {
    m_pcBinIf->encodeBin( uiInterDir, m_cCUInterDirSCModel.get( 0, 0, 3 ) );
  }

  return;
}

Void TEncSbac::codeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );

  UInt uiCtx = pcCU->getCtxRefIdx( uiAbsPartIdx, eRefList );

  m_pcBinIf->encodeBin( ( iRefFrame == 0 ? 0 : 1 ), m_cCURefPicSCModel.get( 0, 0, uiCtx ) );

  if ( iRefFrame > 0 )
  {
    xWriteUnaryMaxSymbol( iRefFrame - 1, &m_cCURefPicSCModel.get( 0, 0, 4 ), 1, pcCU->getSlice()->getNumRefIdx( eRefList )-2 );
  }
  return;
}

Void TEncSbac::codeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  TComCUMvField* pcCUMvField = pcCU->getCUMvField( eRefList );
  Int iHor = pcCUMvField->getMvd( uiAbsPartIdx ).getHor();
  Int iVer = pcCUMvField->getMvd( uiAbsPartIdx ).getVer();

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

  xWriteMvd( iHor, iHorPred, 0 );
  xWriteMvd( iVer, iVerPred, 1 );

  return;
}

Void TEncSbac::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getSlice()->getSliceQp();

  if ( iDQp == 0 )
  {
    m_pcBinIf->encodeBin( 0, m_cCUDeltaQpSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cCUDeltaQpSCModel.get( 0, 0, 0 ) );

    UInt uiDQp = (UInt)( iDQp > 0 ? ( 2 * iDQp - 2 ) : ( -2 * iDQp - 1 ) );
    xWriteUnarySymbol( uiDQp, &m_cCUDeltaQpSCModel.get( 0, 0, 2 ), 1 );
  }

  return;
}

Void TEncSbac::codeCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  return;
}


#if LCEC_CBP_YUV_ROOT
Void TEncSbac::codeBlockCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiQPartNum, Bool bRD )
{
  return;
}
#endif

Void TEncSbac::codeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  UInt uiCbf = pcCU->getCbf     ( uiAbsPartIdx, eType, uiTrDepth );
  UInt uiCtx = pcCU->getCtxQtCbf( uiAbsPartIdx, eType, uiTrDepth );
  m_pcBinIf->encodeBin( uiCbf , m_cCUQtCbfSCModel.get( 0, eType ? eType - 1 : eType, uiCtx ) );
  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiCbf )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tetype=" )
  DTRACE_CABAC_V( eType )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
}

UInt xCheckCoeffPlainCNoRecur( const TCoeff* pcCoef, UInt uiSize, UInt uiDepth )
{
  UInt uiNumofCoeff = 0;
  UInt ui = uiSize>>uiDepth;
  {
    UInt x, y;
    const TCoeff* pCeoff = pcCoef;
    for( y=0 ; y<ui ; y++ )
    {
      for( x=0 ; x<ui ; x++ )
      {
        if( pCeoff[x] != 0 )
        {
          uiNumofCoeff++;
        }
      }
      pCeoff += uiSize;
    }
  }
  return uiNumofCoeff;
}

Void TEncSbac::codeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiCbf = pcCU->getQtRootCbf( uiAbsPartIdx );
  UInt uiCtx = pcCU->getCtxQtRootCbf( uiAbsPartIdx );
  m_pcBinIf->encodeBin( uiCbf , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtRootCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiCbf )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
}

Void TEncSbac::xCheckCoeff( TCoeff* pcCoef, UInt uiSize, UInt uiDepth, UInt& uiNumofCoeff, UInt& uiPart )
{
  UInt ui = uiSize>>uiDepth;
  if( uiPart == 0 )
  {
    if( ui <= 4 )
    {
      UInt x, y;
      TCoeff* pCeoff = pcCoef;
      for( y=0 ; y<ui ; y++ )
      {
        for( x=0 ; x<ui ; x++ )
        {
          if( pCeoff[x] != 0 )
          {
            uiNumofCoeff++;
          }
        }
        pCeoff += uiSize;
      }
    }
    else
    {
      xCheckCoeff( pcCoef,                            uiSize, uiDepth+1, uiNumofCoeff, uiPart ); uiPart++; //1st Part
      xCheckCoeff( pcCoef             + (ui>>1),      uiSize, uiDepth+1, uiNumofCoeff, uiPart ); uiPart++; //2nd Part
      xCheckCoeff( pcCoef + (ui>>1)*uiSize,           uiSize, uiDepth+1, uiNumofCoeff, uiPart ); uiPart++; //3rd Part
      xCheckCoeff( pcCoef + (ui>>1)*uiSize + (ui>>1), uiSize, uiDepth+1, uiNumofCoeff, uiPart );           //4th Part
    }
  }
  else
  {
    UInt x, y;
    TCoeff* pCeoff = pcCoef;
    for( y=0 ; y<ui ; y++ )
    {
      for( x=0 ; x<ui ; x++ )
      {
        if( pCeoff[x] != 0 )
        {
          uiNumofCoeff++;
        }
      }
      pCeoff += uiSize;
    }
  }
}

UInt xCheckCoeffPlainCNoRecur( const TCoeff* pcCoef, UInt uiSize, UInt uiDepth );

Void TEncSbac::codeCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType, Bool bRD )
{
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
  if( uiWidth > m_pcSlice->getSPS()->getMaxTrSize() )
  {
    uiWidth  = m_pcSlice->getSPS()->getMaxTrSize();
    uiHeight = m_pcSlice->getSPS()->getMaxTrSize();
  }

  UInt uiNumSig = 0;
  UInt uiCTXIdx = xGetCTXIdxFromWidth( uiWidth );

  // compute number of significant coefficients
  UInt  uiPart = 0;
  xCheckCoeff(pcCoef, uiWidth, 0, uiNumSig, uiPart );

  if ( bRD )
  {
    UInt uiTempDepth = uiDepth - pcCU->getDepth( uiAbsPartIdx );
    pcCU->setCbfSubParts( ( uiNumSig ? 1 : 0 ) << uiTempDepth, eTType, uiAbsPartIdx, uiDepth );
    codeCbf( pcCU, uiAbsPartIdx, eTType, uiTempDepth );
  }

  if ( uiNumSig == 0 )
    return;

  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : ( eTType == TEXT_NONE ? TEXT_NONE : TEXT_CHROMA );

   //----- encode significance map -----
  const UInt   uiMaxNumCoeff     = uiWidth*uiHeight;
  const UInt   uiMaxNumCoeffM1   = uiMaxNumCoeff - 1;
  const UInt   uiLog2BlockSize   = g_aucConvertToBit[ uiWidth ] + 2;
  UInt         uiDownLeft        = 1;
  UInt         uiNumSigTopRight  = 0;
  UInt         uiNumSigBotLeft   = 0;


  for( UInt uiScanPos = 0; uiScanPos < uiMaxNumCoeffM1; uiScanPos++ )
  {
    UInt  uiBlkPos  = g_auiSigLastScan[ uiLog2BlockSize ][ uiDownLeft ][ uiScanPos ];
    UInt  uiPosY    = uiBlkPos >> uiLog2BlockSize;
    UInt  uiPosX    = uiBlkPos - ( uiPosY << uiLog2BlockSize );

    //===== code significance flag =====
    UInt  uiSig    = pcCoef[ uiBlkPos ] != 0 ? 1 : 0;
    UInt  uiCtxSig = TComTrQuant::getSigCtxInc( pcCoef, uiPosX, uiPosY, uiLog2BlockSize, uiWidth, ( uiDownLeft > 0 ) );
    m_pcBinIf->encodeBin( uiSig, m_cCuCtxModSig.get( uiCTXIdx, eTType, uiCtxSig ) );

    if( uiSig )
    {
      uiNumSig--;

      if( uiPosX > uiPosY )
      {
        uiNumSigTopRight++;
      }
      else if( uiPosY > uiPosX )
      {
        uiNumSigBotLeft ++;
      }

      //===== code last flag =====
      UInt  uiLast    = ( uiNumSig == 0 ) ? 1 : 0;
      UInt  uiCtxLast = TComTrQuant::getLastCtxInc( uiPosX, uiPosY, uiLog2BlockSize );
      m_pcBinIf->encodeBin( uiLast, m_cCuCtxModLast.get( uiCTXIdx, eTType, uiCtxLast ) );

      if( uiLast )
      {
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

#if TI_SIGN_BIN0_PCP
  /*
   * Sign and bin0 PCP (Section 3.2 and 3.3 of JCTVC-B088)
   */
  Int  c1, c2;
  UInt uiAbs;
  UInt uiSign;
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
        uiSubPosX = g_auiFrameScanX[ uiSubLog2 - 1 ][ uiSubBlk ] * 4;
        uiSubPosY = g_auiFrameScanY[ uiSubLog2 - 1 ][ uiSubBlk ] * 4;
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
          uiCtxSet = uiPrevAbsGreOne / uiNumOfSets + 1;//( ( uiGreOne * uiEqRangeSize ) >> 8 ) + 1;
          uiPrevAbsGreOne = 0;
        }

		// Code Bin0 plane for levels
        for ( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = (uiSubPosY + uiPosY) * uiWidth + uiSubPosX + uiPosX;

          if( pcCoef[ uiIndex ]  )
          {
            if( pcCoef[ uiIndex ] > 0) { uiAbs = static_cast<UInt>( pcCoef[ uiIndex ]);  uiSign = 0; }
            else                       { uiAbs = static_cast<UInt>(-pcCoef[ uiIndex ]);  uiSign = 1; }

            UInt uiCtx    = min<UInt>(c1, 4);
            UInt uiSymbol = uiAbs > 1 ? 1 : 0;
            m_pcBinIf->encodeBin( uiSymbol, m_cCuCtxModAbsGreOne.get( 0, eTType, (uiCtxSet * 5) + uiCtx ) );

            if( uiSymbol )
            {
              c1     = 0;
            }
            else if( c1 )
            {
              c1++;
            }
          }
        }

		// Code other bins for levels
        for ( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = (uiSubPosY + uiPosY) * uiWidth + uiSubPosX + uiPosX;

          if( pcCoef[ uiIndex ]  )
          {
            if( pcCoef[ uiIndex ] > 0) { uiAbs = static_cast<UInt>( pcCoef[ uiIndex ]);  uiSign = 0; }
            else                       { uiAbs = static_cast<UInt>(-pcCoef[ uiIndex ]);  uiSign = 1; }

            UInt uiSymbol = uiAbs > 1 ? 1 : 0;

            if( uiSymbol )
            {
              UInt uiCtx  = min<UInt>(c2, 4);
              uiAbs -= 2;
              c2++;
              uiPrevAbsGreOne++;
              xWriteExGolombLevel( uiAbs, m_cCuCtxModCoeffLevelM1.get( 0, eTType, (uiCtxSet * 5) + uiCtx ) );
            }
          }
        }

		// Code sign plane
        for ( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = (uiSubPosY + uiPosY) * uiWidth + uiSubPosX + uiPosX;

          if( pcCoef[ uiIndex ]  )
          {
            if( pcCoef[ uiIndex ] > 0) { uiAbs = static_cast<UInt>( pcCoef[ uiIndex ]);  uiSign = 0; }
            else                       { uiAbs = static_cast<UInt>(-pcCoef[ uiIndex ]);  uiSign = 1; }
            m_pcBinIf->encodeBinEP( uiSign );
          }
        }

      }
    }
  }
  else
  {
    c1 = 1;
    c2 = 0;

	// Code Bin0 plane for levels
    for ( UInt uiScanPos = 0; uiScanPos < uiWidth*uiHeight; uiScanPos++ )
    {
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] + 1 ][ uiWidth*uiHeight - uiScanPos - 1 ];
      if( pcCoef[ uiIndex ]  )
      {
        if( pcCoef[ uiIndex ] > 0) { uiAbs = static_cast<UInt>( pcCoef[ uiIndex ]);  uiSign = 0; }
        else                       { uiAbs = static_cast<UInt>(-pcCoef[ uiIndex ]);  uiSign = 1; }

        UInt uiCtx    = min<UInt>(c1, 4);
        UInt uiSymbol = uiAbs > 1 ? 1 : 0;
        m_pcBinIf->encodeBin( uiSymbol, m_cCuCtxModAbsGreOne.get( 0, eTType, uiCtx ) );

        if( uiSymbol )
        {
          c1     = 0;
        }
        else if( c1 )
        {
          c1++;
        }
      }
    }

	// Code other bins for levels
    for ( UInt uiScanPos = 0; uiScanPos < uiWidth*uiHeight; uiScanPos++ )
    {
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] + 1 ][ uiWidth*uiHeight - uiScanPos - 1 ];
      if( pcCoef[ uiIndex ]  )
      {
        if( pcCoef[ uiIndex ] > 0) { uiAbs = static_cast<UInt>( pcCoef[ uiIndex ]);  uiSign = 0; }
        else                       { uiAbs = static_cast<UInt>(-pcCoef[ uiIndex ]);  uiSign = 1; }

        UInt uiSymbol = uiAbs > 1 ? 1 : 0;

        if( uiSymbol )
        {
          UInt uiCtx  = min<UInt>(c2, 4);
          uiAbs -= 2;
          c2++;
          xWriteExGolombLevel( uiAbs, m_cCuCtxModCoeffLevelM1.get( 0, eTType, uiCtx ) );
        }
      }
    }

	// Code sign plane
    for ( UInt uiScanPos = 0; uiScanPos < uiWidth*uiHeight; uiScanPos++ )
    {
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] + 1 ][ uiWidth*uiHeight - uiScanPos - 1 ];
      if( pcCoef[ uiIndex ]  )
      {
        if( pcCoef[ uiIndex ] > 0) { uiAbs = static_cast<UInt>( pcCoef[ uiIndex ]);  uiSign = 0; }
        else                       { uiAbs = static_cast<UInt>(-pcCoef[ uiIndex ]);  uiSign = 1; }

        m_pcBinIf->encodeBinEP( uiSign );
      }
    }
  }
#else   
  Int  c1, c2;
  UInt uiAbs;
  UInt uiSign;
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
        uiSubPosX = g_auiFrameScanX[ uiSubLog2 - 1 ][ uiSubBlk ] * 4;
        uiSubPosY = g_auiFrameScanY[ uiSubLog2 - 1 ][ uiSubBlk ] * 4;
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
          uiCtxSet = uiPrevAbsGreOne / uiNumOfSets + 1;//( ( uiGreOne * uiEqRangeSize ) >> 8 ) + 1;
          uiPrevAbsGreOne = 0;
        }

        for ( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = (uiSubPosY + uiPosY) * uiWidth + uiSubPosX + uiPosX;

          if( pcCoef[ uiIndex ]  )
          {
            if( pcCoef[ uiIndex ] > 0) { uiAbs = static_cast<UInt>( pcCoef[ uiIndex ]);  uiSign = 0; }
            else                       { uiAbs = static_cast<UInt>(-pcCoef[ uiIndex ]);  uiSign = 1; }

            UInt uiCtx    = min<UInt>(c1, 4);
            UInt uiSymbol = uiAbs > 1 ? 1 : 0;
            m_pcBinIf->encodeBin( uiSymbol, m_cCuCtxModAbsGreOne.get( 0, eTType, (uiCtxSet * 5) + uiCtx ) );

            if( uiSymbol )
            {
              uiCtx  = min<UInt>(c2, 4);
              uiAbs -= 2;
              c1     = 0;
              c2++;
              uiPrevAbsGreOne++;
              xWriteExGolombLevel( uiAbs, m_cCuCtxModCoeffLevelM1.get( 0, eTType, (uiCtxSet * 5) + uiCtx ) );
            }
            else if( c1 )
            {
              c1++;
            }
            m_pcBinIf->encodeBinEP( uiSign );
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
      UInt uiIndex = g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] + 1 ][ uiWidth*uiHeight - uiScanPos - 1 ];

      if( pcCoef[ uiIndex ]  )
      {
        if( pcCoef[ uiIndex ] > 0) { uiAbs = static_cast<UInt>( pcCoef[ uiIndex ]);  uiSign = 0; }
        else                       { uiAbs = static_cast<UInt>(-pcCoef[ uiIndex ]);  uiSign = 1; }

        UInt uiCtx    = min<UInt>(c1, 4);
        UInt uiSymbol = uiAbs > 1 ? 1 : 0;
        m_pcBinIf->encodeBin( uiSymbol, m_cCuCtxModAbsGreOne.get( 0, eTType, uiCtx ) );

        if( uiSymbol )
        {
          uiCtx  = min<UInt>(c2, 4);
          uiAbs -= 2;
          c1     = 0;
          c2++;
          xWriteExGolombLevel( uiAbs, m_cCuCtxModCoeffLevelM1.get( 0, eTType, uiCtx ) );
        }
        else if( c1 )
        {
          c1++;
        }
        m_pcBinIf->encodeBinEP( uiSign );
      }
    }
  }
#endif
  return;
}

Void TEncSbac::xWriteMvd( Int iMvd, UInt uiAbsSum, UInt uiCtx )
{
  UInt uiLocalCtx = 0;
  if ( uiAbsSum >= 3 )
  {
    uiLocalCtx += ( uiAbsSum > 32 ) ? 2 : 1;
  }

  UInt uiSymbol = ( 0 == iMvd ) ? 0 : 1;
  m_pcBinIf->encodeBin( uiSymbol, m_cCUMvdSCModel.get( 0, uiCtx, uiLocalCtx ) );
  if ( 0 == uiSymbol )
  {
    return;
  }

  UInt uiSign = 0;
  if ( 0 > iMvd )
  {
    uiSign = 1;
    iMvd   = -iMvd;
  }
  xWriteExGolombMvd( iMvd-1, &m_cCUMvdSCModel.get( 0, uiCtx, 3 ), 3 );
  m_pcBinIf->encodeBinEP( uiSign );

  return;
}

Void  TEncSbac::xWriteExGolombMvd( UInt uiSymbol, ContextModel* pcSCModel, UInt uiMaxBin )
{
  if ( ! uiSymbol )
  {
    m_pcBinIf->encodeBin( 0, *pcSCModel );
    return;
  }

  m_pcBinIf->encodeBin( 1, *pcSCModel );

  Bool bNoExGo = ( uiSymbol < 8 );
  UInt uiCount = 1;
  pcSCModel++;

  while ( --uiSymbol && ++uiCount <= 8 )
  {
    m_pcBinIf->encodeBin( 1, *pcSCModel );
    if ( uiCount == 2 )
    {
      pcSCModel++;
    }
    if ( uiCount == uiMaxBin )
    {
      pcSCModel++;
    }
  }

  if ( bNoExGo )
  {
    m_pcBinIf->encodeBin( 0, *pcSCModel );
  }
  else
  {
    xWriteEpExGolomb( uiSymbol, 3 );
  }

  return;
}

Void TEncSbac::codeAlfFlag       ( UInt uiCode )
{
  UInt uiSymbol = ( ( uiCode == 0 ) ? 0 : 1 );
  m_pcBinIf->encodeBin( uiSymbol, m_cALFFlagSCModel.get( 0, 0, 0 ) );
}

#if TSB_ALF_HEADER
Void TEncSbac::codeAlfFlagNum( UInt uiCode, UInt minValue )
{
  UInt uiLength = 0;
  UInt maxValue = (minValue << (this->getMaxAlfCtrlDepth()*2));
  assert((uiCode>=minValue)&&(uiCode<=maxValue));
  UInt temp = maxValue - minValue;
  for(UInt i=0; i<32; i++)
  {
    if(temp&0x1)
    {
      uiLength = i+1;
    }
    temp = (temp >> 1);
  }
  UInt uiSymbol = uiCode - minValue;
  if(uiLength)
  {
    while( uiLength-- )
    {
      m_pcBinIf->encodeBinEP( (uiSymbol>>uiLength) & 0x1 );
    }
  }
}

Void TEncSbac::codeAlfCtrlFlag( UInt uiSymbol )
{
  m_pcBinIf->encodeBin( uiSymbol, m_cCUAlfCtrlFlagSCModel.get( 0, 0, 0) );
}
#endif

Void TEncSbac::codeAlfUvlc       ( UInt uiCode )
{
  Int i;

  if ( uiCode == 0 )
  {
    m_pcBinIf->encodeBin( 0, m_cALFUvlcSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cALFUvlcSCModel.get( 0, 0, 0 ) );
    for ( i=0; i<uiCode-1; i++ )
    {
      m_pcBinIf->encodeBin( 1, m_cALFUvlcSCModel.get( 0, 0, 1 ) );
    }
    m_pcBinIf->encodeBin( 0, m_cALFUvlcSCModel.get( 0, 0, 1 ) );
  }
}

Void TEncSbac::codeAlfSvlc       ( Int iCode )
{
  Int i;

  if ( iCode == 0 )
  {
    m_pcBinIf->encodeBin( 0, m_cALFSvlcSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cALFSvlcSCModel.get( 0, 0, 0 ) );

    // write sign
    if ( iCode > 0 )
    {
      m_pcBinIf->encodeBin( 0, m_cALFSvlcSCModel.get( 0, 0, 1 ) );
    }
    else
    {
      m_pcBinIf->encodeBin( 1, m_cALFSvlcSCModel.get( 0, 0, 1 ) );
      iCode = -iCode;
    }

    // write magnitude
    for ( i=0; i<iCode-1; i++ )
    {
      m_pcBinIf->encodeBin( 1, m_cALFSvlcSCModel.get( 0, 0, 2 ) );
    }
    m_pcBinIf->encodeBin( 0, m_cALFSvlcSCModel.get( 0, 0, 2 ) );
  }
}

/*!
 ****************************************************************************
 * \brief
 *   estimate bit cost for CBP, significant map and significant coefficients
 ****************************************************************************
 */
Void TEncSbac::estBit (estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType)
{
  estCBFBit (pcEstBitsSbac, 0, eTType);

  // encode significance map
  estSignificantMapBit (pcEstBitsSbac, uiCTXIdx, eTType);

  // encode significant coefficients
  estSignificantCoefficientsBit (pcEstBitsSbac, uiCTXIdx, eTType);
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost for each CBP bit
 ****************************************************************************
 */
Void TEncSbac::estCBFBit (estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType)
{
  Int ctx;
#if !BUGFIX85TMP
  Short cbp_bit;
#endif
  
  for ( ctx = 0; ctx <= 3; ctx++ )
  {
#if BUGFIX85TMP
    pcEstBitsSbac->blockCbpBits[ctx][0] = entropyBits[64];
    pcEstBitsSbac->blockCbpBits[ctx][1] = entropyBits[64];
#else
    cbp_bit = 0;
    pcEstBitsSbac->blockCbpBits[ctx][cbp_bit] = biari_no_bits (cbp_bit, m_cCUCbfSCModel.get( uiCTXIdx, eTType, ctx ));

    cbp_bit = 1;
    pcEstBitsSbac->blockCbpBits[ctx][cbp_bit] = biari_no_bits (cbp_bit, m_cCUCbfSCModel.get( uiCTXIdx, eTType, ctx ));
#endif
  }
}


/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient map
 ****************************************************************************
 */
Void TEncSbac::estSignificantMapBit (estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType)
{
  Int    k;
  UShort sig, last;
  Int    k1 = 16;

  for ( k = 0; k < k1; k++ )
  {
    sig = 0;
    pcEstBitsSbac->significantBits[k][sig] = biari_no_bits (sig, m_cCuCtxModSig.get( uiCTXIdx, eTType, k ));

    sig = 1;
    pcEstBitsSbac->significantBits[k][sig] = biari_no_bits (sig, m_cCuCtxModSig.get( uiCTXIdx, eTType, k ));

    last = 0;
    pcEstBitsSbac->lastBits[k][last] = biari_no_bits (last, m_cCuCtxModLast.get( uiCTXIdx, eTType, k ));

    last = 1;
    pcEstBitsSbac->lastBits[k][last] = biari_no_bits (last, m_cCuCtxModLast.get( uiCTXIdx, eTType, k ));
  }
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost of significant coefficient
 ****************************************************************************
 */
Void TEncSbac::estSignificantCoefficientsBit (estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType)
{
  Int   ctx;
  uiCTXIdx = 0;

  for ( UInt ui = 0; ui < 6; ui++ )
  {
    for ( ctx = 0; ctx <= 4; ctx++ )
    {
      pcEstBitsSbac->greaterOneBits[ui][0][ctx][0] = biari_no_bits( 0, m_cCuCtxModAbsGreOne.get( uiCTXIdx, eTType, (ui * 5) + ctx ) );
      pcEstBitsSbac->greaterOneBits[ui][0][ctx][1] = biari_no_bits( 1, m_cCuCtxModAbsGreOne.get( uiCTXIdx, eTType, (ui * 5) + ctx ) );
    }

    for ( ctx = 0; ctx <= 4; ctx++ )
    {
      pcEstBitsSbac->greaterOneBits[ui][1][ctx][0] = biari_no_bits( 0, m_cCuCtxModCoeffLevelM1.get( uiCTXIdx, eTType, (ui * 5) + ctx ) );
      pcEstBitsSbac->greaterOneBits[ui][1][ctx][1] = biari_no_bits( 1, m_cCuCtxModCoeffLevelM1.get( uiCTXIdx, eTType, (ui * 5) + ctx ) );
    }
  }
}

Int TEncSbac::biari_no_bits (Short symbol, ContextModel& rcSCModel)
{
  Int ctx_state, estBits;

  symbol = (Short) (symbol != 0);

  ctx_state = (symbol == rcSCModel.getMps() ) ? 64 + rcSCModel.getState() : 63 - rcSCModel.getState();

  estBits = entropyBits[127-ctx_state];

  return estBits;
}

Int TEncSbac::biari_state (Short symbol, ContextModel& rcSCModel)
{
  Int ctx_state;

  symbol = (Short) (symbol != 0);

  ctx_state = ( symbol == rcSCModel.getMps() ) ? 64 + rcSCModel.getState() : 63 - rcSCModel.getState();

  return ctx_state;
}
