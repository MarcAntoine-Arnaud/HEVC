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

/** \file     TEncCavlc.cpp
    \brief    CAVLC encoder class
*/

#include "../TLibCommon/CommonDef.h"
#include "TEncCavlc.h"
#include "SEIwrite.h"

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE

#define WRITE_CODE( value, length, name)    xWriteCodeTr ( value, length, name )
#define WRITE_UVLC( value,         name)    xWriteUvlcTr ( value,         name )
#define WRITE_SVLC( value,         name)    xWriteSvlcTr ( value,         name )
#define WRITE_FLAG( value,         name)    xWriteFlagTr ( value,         name )

Void  xWriteUvlcTr          ( UInt value,               const Char *pSymbolName);
Void  xWriteSvlcTr          ( Int  value,               const Char *pSymbolName);
Void  xWriteFlagTr          ( UInt value,               const Char *pSymbolName);

Void  xTraceSPSHeader (TComSPS *pSPS)
{
  fprintf( g_hTrace, "=========== Sequence Parameter Set ID: %d ===========\n", pSPS->getSPSId() );
}

Void  xTracePPSHeader (TComPPS *pPPS)
{
  fprintf( g_hTrace, "=========== Picture Parameter Set ID: %d ===========\n", pPPS->getPPSId() );
}

Void  xTraceAPSHeader (TComAPS *pAPS)
{
  fprintf( g_hTrace, "=========== Adaptation Parameter Set ===========\n");
}

Void  xTraceSliceHeader (TComSlice *pSlice)
{
  fprintf( g_hTrace, "=========== Slice ===========\n");
}


Void  TEncCavlc::xWriteCodeTr (UInt value, UInt  length, const Char *pSymbolName)
{
  xWriteCode (value,length);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s u(%d) : %d\n", pSymbolName, length, value ); 
}

Void  TEncCavlc::xWriteUvlcTr (UInt value, const Char *pSymbolName)
{
  xWriteUvlc (value);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s u(v) : %d\n", pSymbolName, value ); 
}

Void  TEncCavlc::xWriteSvlcTr (Int value, const Char *pSymbolName)
{
  xWriteSvlc(value);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s s(v) : %d\n", pSymbolName, value ); 
}

Void  TEncCavlc::xWriteFlagTr(UInt value, const Char *pSymbolName)
{
  xWriteFlag(value);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s u(1) : %d\n", pSymbolName, value ); 
}

#else

#define WRITE_CODE( value, length, name)     xWriteCode ( value, length )
#define WRITE_UVLC( value,         name)     xWriteUvlc ( value )
#define WRITE_SVLC( value,         name)     xWriteSvlc ( value )
#define WRITE_FLAG( value,         name)     xWriteFlag ( value )

#endif



// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCavlc::TEncCavlc()
{
  m_pcBitIf           = NULL;
  m_bRunLengthCoding  = false;   //  m_bRunLengthCoding  = !rcSliceHeader.isIntra();
  m_uiCoeffCost       = 0;
  m_bAlfCtrl = false;
  m_uiMaxAlfCtrlDepth = 0;
  
  m_bAdaptFlag        = true;    // adaptive VLC table
  m_iSliceGranularity = 0;
}

TEncCavlc::~TEncCavlc()
{
}


// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncCavlc::resetEntropy()
{
  m_bRunLengthCoding = ! m_pcSlice->isIntra();
  m_uiRun = 0;
  ::memcpy(m_uiLPTableE4, g_auiLPTableE4, 3*32*sizeof(UInt));
  ::memcpy(m_uiLPTableD4, g_auiLPTableD4, 3*32*sizeof(UInt));
  ::memcpy(m_uiLastPosVlcIndex, g_auiLastPosVlcIndex, 10*sizeof(UInt));
  
  ::memcpy( m_uiIntraModeTableD17, g_auiIntraModeTableD17, 17*sizeof(UInt) );
  ::memcpy( m_uiIntraModeTableE17, g_auiIntraModeTableE17, 17*sizeof(UInt) );
  ::memcpy( m_uiIntraModeTableD34, g_auiIntraModeTableD34, 34*sizeof(UInt) );
  ::memcpy( m_uiIntraModeTableE34, g_auiIntraModeTableE34, 34*sizeof(UInt) );
  
  ::memcpy(m_uiCBP_YUV_TableE, g_auiCBP_YUV_TableE, 4*8*sizeof(UInt));
  ::memcpy(m_uiCBP_YUV_TableD, g_auiCBP_YUV_TableD, 4*8*sizeof(UInt));
  ::memcpy(m_uiCBP_YS_TableE,  g_auiCBP_YS_TableE,  2*4*sizeof(UInt));
  ::memcpy(m_uiCBP_YS_TableD,  g_auiCBP_YS_TableD,  2*4*sizeof(UInt));
  ::memcpy(m_uiCBP_YCS_TableE, g_auiCBP_YCS_TableE, 2*8*sizeof(UInt));
  ::memcpy(m_uiCBP_YCS_TableD, g_auiCBP_YCS_TableD, 2*8*sizeof(UInt));
  ::memcpy(m_uiCBP_4Y_TableE,  g_auiCBP_4Y_TableE,  2*15*sizeof(UInt));
  ::memcpy(m_uiCBP_4Y_TableD,  g_auiCBP_4Y_TableD,  2*15*sizeof(UInt));
  m_uiCBP_4Y_VlcIdx = 0;

  ::memcpy(m_uiMI1TableE, g_auiComMI1TableE, 9*sizeof(UInt));
  ::memcpy(m_uiMI1TableD, g_auiComMI1TableD, 9*sizeof(UInt));

  ::memcpy(m_uiSplitTableE, g_auiInterModeTableE, 4*11*sizeof(UInt));
  ::memcpy(m_uiSplitTableD, g_auiInterModeTableD, 4*11*sizeof(UInt));
  
  m_uiMITableVlcIdx = 0;  

  ::memset(m_ucCBP_YUV_TableCounter,   0,        4*4*sizeof(UChar));
  ::memset(m_ucCBP_4Y_TableCounter,    0,        2*2*sizeof(UChar));
  ::memset(m_ucCBP_YCS_TableCounter,   0,        2*4*sizeof(UChar));
  ::memset(m_ucCBP_YS_TableCounter,    0,        2*3*sizeof(UChar));

  ::memset(m_ucMI1TableCounter,        0,          4*sizeof(UChar));
  ::memset(m_ucSplitTableCounter,      0,        4*4*sizeof(UChar));

  m_ucCBP_YUV_TableCounterSum[0] = m_ucCBP_YUV_TableCounterSum[1] = m_ucCBP_YUV_TableCounterSum[2] = m_ucCBP_YUV_TableCounterSum[3] = 0;
  m_ucCBP_4Y_TableCounterSum[0] = m_ucCBP_4Y_TableCounterSum[1] = 0;
  m_ucCBP_YCS_TableCounterSum[0] = m_ucCBP_YCS_TableCounterSum[1] = 0;
  m_ucCBP_YS_TableCounterSum[0] = m_ucCBP_YS_TableCounterSum[1] = 0;
  m_ucSplitTableCounterSum[0] = m_ucSplitTableCounterSum[1] = m_ucSplitTableCounterSum[2]= m_ucSplitTableCounterSum[3] = 0;
  m_ucMI1TableCounterSum = 0;
}

UInt* TEncCavlc::GetLP4Table()
{   
  return &m_uiLPTableE4[0][0];
}

UInt* TEncCavlc::GetLastPosVlcIndexTable()
{   
  return &m_uiLastPosVlcIndex[0];
}

/**
 * marshall the SEI message sei.
 */
void TEncCavlc::codeSEI(const SEI& sei)
{
  writeSEImessage(*m_pcBitIf, sei);
}

Void  TEncCavlc::codeAPSInitInfo(TComAPS* pcAPS)
{

#if ENC_DEC_TRACE  
  xTraceAPSHeader(pcAPS);
#endif
  //APS ID
  WRITE_UVLC( pcAPS->getAPSID(), "aps_id" );

#if SCALING_LIST
  WRITE_FLAG( pcAPS->getScalingListEnabled()?1:0, "aps_scaling_list_data_present_flag");
#endif
  //DF flag
  WRITE_FLAG(pcAPS->getLoopFilterOffsetInAPS()?1:0, "aps_deblocking_filter_flag");
  //SAO flag
  WRITE_FLAG( pcAPS->getSaoEnabled()?1:0, "aps_sample_adaptive_offset_flag"); 

  //ALF flag
  WRITE_FLAG( pcAPS->getAlfEnabled()?1:0, "aps_adaptive_loop_filter_flag"); 

#if !G220_PURE_VLC_SAO_ALF
  if(pcAPS->getSaoEnabled() || pcAPS->getAlfEnabled())
  {
    //CABAC usage flag
    WRITE_FLAG(pcAPS->getCABACForAPS()?1:0, "aps_cabac_use_flag"); 
    if(pcAPS->getCABACForAPS())
    {
      //CABAC init IDC
      WRITE_UVLC(pcAPS->getCABACinitIDC(), "aps_cabac_init_idc" );
      //CABAC init QP
      WRITE_SVLC(pcAPS->getCABACinitQP()-26, "aps_cabac_init_qp_minus26" );
    }
  }
#endif
}

Void TEncCavlc::codeDFFlag(UInt uiCode, const Char *pSymbolName)
{
  WRITE_FLAG(uiCode, pSymbolName);
}
Void TEncCavlc::codeDFSvlc(Int iCode, const Char *pSymbolName)
{
  WRITE_SVLC(iCode, pSymbolName);
}

Void TEncCavlc::codeShortTermRefPicSet( TComPPS* pcPPS, TComReferencePictureSet* pcRPS )
{
#if PRINT_RPS_INFO
  int lastBits = getNumberOfWrittenBits();
#endif
#if INTER_RPS_PREDICTION
  WRITE_FLAG( pcRPS->getInterRPSPrediction(), "inter_ref_pic_set_prediction_flag" ); // inter_RPS_prediction_flag
  if (pcRPS->getInterRPSPrediction()) 
  {
    //Int rIdx = i - pcRPS->getDeltaRIdxMinus1() - 1;
    Int deltaRPS = pcRPS->getDeltaRPS();
    //assert (rIdx <= i);
    WRITE_UVLC( pcRPS->getDeltaRIdxMinus1(), "delta_idx_minus1" ); // delta index of the Reference Picture Set used for prediction minus 1
    WRITE_CODE( (deltaRPS >=0 ? 0: 1), 1, "delta_rps_sign" ); //delta_rps_sign
    WRITE_UVLC( abs(deltaRPS) - 1, "abs_delta_rps_minus1"); // absolute delta RPS minus 1

    for(Int j=0; j < pcRPS->getNumRefIdc(); j++)
    {
      Int refIdc = pcRPS->getRefIdc(j);
      WRITE_CODE( (refIdc==1? 1: 0), 1, "ref_idc0" ); //first bit is "1" if Idc is 1 
      if (refIdc != 1) 
      {
        WRITE_CODE( refIdc>>1, 1, "ref_idc1" ); //second bit is "1" if Idc is 2, "0" otherwise.
      }
    }
  }
  else
  {
#endif //INTER_RPS_PREDICTION
    WRITE_UVLC( pcRPS->getNumberOfNegativePictures(), "num_negative_pics" );
    WRITE_UVLC( pcRPS->getNumberOfPositivePictures(), "num_positive_pics" );
    Int prev = 0;
    for(Int j=0 ; j < pcRPS->getNumberOfNegativePictures(); j++)
    {
      WRITE_UVLC( prev-pcRPS->getDeltaPOC(j)-1, "delta_poc_s0_minus1" );
      prev = pcRPS->getDeltaPOC(j);
      WRITE_FLAG( pcRPS->getUsed(j), "used_by_curr_pic_s0_flag"); 
    }
    prev = 0;
    for(Int j=pcRPS->getNumberOfNegativePictures(); j < pcRPS->getNumberOfNegativePictures()+pcRPS->getNumberOfPositivePictures(); j++)
    {
      WRITE_UVLC( pcRPS->getDeltaPOC(j)-prev-1, "delta_poc_s1_minus1" );
      prev = pcRPS->getDeltaPOC(j);
      WRITE_FLAG( pcRPS->getUsed(j), "used_by_curr_pic_s1_flag" ); 
    }
#if INTER_RPS_PREDICTION
  }
#endif // INTER_RPS_PREDICTION

#if PRINT_RPS_INFO
#if INTER_RPS_PREDICTION  
  printf("irps=%d (%2d bits) ", pcRPS->getInterRPSPrediction(), getNumberOfWrittenBits() - lastBits);
#else
  printf("(%2d bits) ", getNumberOfWrittenBits() - lastBits);
#endif
  pcRPS->printDeltaPOC();
#endif
}


Void TEncCavlc::codePPS( TComPPS* pcPPS )
{
#if ENC_DEC_TRACE  
  xTracePPSHeader (pcPPS);
#endif
   TComRPS* pcRPSList = pcPPS->getRPSList();
  
  WRITE_UVLC( pcPPS->getPPSId(),                             "pic_parameter_set_id" );
  WRITE_UVLC( pcPPS->getSPSId(),                             "seq_parameter_set_id" );
  // RPS is put before entropy_coding_mode_flag
  // since entropy_coding_mode_flag will probably be removed from the WD
  TComReferencePictureSet*      pcRPS;

  WRITE_UVLC(pcRPSList->getNumberOfReferencePictureSets(), "num_short_term_ref_pic_sets" );
  for(UInt i=0; i < pcRPSList->getNumberOfReferencePictureSets(); i++)
  {
    pcRPS = pcRPSList->getReferencePictureSet(i);
    codeShortTermRefPicSet(pcPPS,pcRPS);
  }    
  WRITE_FLAG( pcPPS->getLongTermRefsPresent() ? 1 : 0,         "long_term_ref_pics_present_flag" );
  // entropy_coding_mode_flag
  // We code the entropy_coding_mode_flag, it's needed for tests.
  WRITE_FLAG( pcPPS->getEntropyCodingMode() ? 1 : 0,         "entropy_coding_mode_flag" );
  if (pcPPS->getEntropyCodingMode())
  {
    WRITE_UVLC( pcPPS->getEntropyCodingSynchro(),            "entropy_coding_synchro" );
    WRITE_FLAG( pcPPS->getCabacIstateReset() ? 1 : 0,        "cabac_istate_reset" );
    if ( pcPPS->getEntropyCodingSynchro() )
    {
      WRITE_UVLC( pcPPS->getNumSubstreams()-1,               "num_substreams_minus1" );
    }
  }
  WRITE_UVLC( pcPPS->getNumTLayerSwitchingFlags(),           "num_temporal_layer_switching_point_flags" );
  for( UInt i = 0; i < pcPPS->getNumTLayerSwitchingFlags(); i++ ) 
  {
    WRITE_FLAG( pcPPS->getTLayerSwitchingFlag( i ) ? 1 : 0 , "temporal_layer_switching_point_flag" ); 
  }
  //   num_ref_idx_l0_default_active_minus1
  //   num_ref_idx_l1_default_active_minus1
#if G507_QP_ISSUE_FIX
  WRITE_SVLC( pcPPS->getPicInitQPMinus26(),                  "pic_init_qp_minus26");
#else
  //   pic_init_qp_minus26  /* relative to 26 */
#endif
  WRITE_FLAG( pcPPS->getConstrainedIntraPred() ? 1 : 0,      "constrained_intra_pred_flag" );
#if NO_TMVP_MARKING
  WRITE_FLAG( pcPPS->getEnableTMVPFlag() ? 1 : 0,            "enable_temporal_mvp_flag" );
#endif
  WRITE_CODE( pcPPS->getSliceGranularity(), 2,               "slice_granularity");
#if G507_QP_ISSUE_FIX
  WRITE_UVLC( pcPPS->getMaxCuDQPDepth() + pcPPS->getUseDQP(),                   "max_cu_qp_delta_depth" );
#else
  if( pcPPS->getSPS()->getUseDQP() )
  {
    WRITE_UVLC( pcPPS->getMaxCuDQPDepth(),                   "max_cu_qp_delta_depth" );
  }
#endif

#if G509_CHROMA_QP_OFFSET
  WRITE_SVLC( pcPPS->getChromaQpOffset(),                   "chroma_qp_offset"     );
  WRITE_SVLC( pcPPS->getChromaQpOffset2nd(),                "chroma_qp_offset_2nd" );
#endif

  WRITE_FLAG( pcPPS->getUseWP() ? 1 : 0,  "weighted_pred_flag" );   // Use of Weighting Prediction (P_SLICE)
  WRITE_CODE( pcPPS->getWPBiPredIdc(), 2, "weighted_bipred_idc" );  // Use of Weighting Bi-Prediction (B_SLICE)

  WRITE_FLAG( pcPPS->getColumnRowInfoPresent(),           "tile_info_present_flag" );
#if NONCROSS_TILE_IN_LOOP_FILTERING
  WRITE_FLAG( pcPPS->getTileBehaviorControlPresentFlag(),  "tile_control_present_flag");
#endif
  if( pcPPS->getColumnRowInfoPresent() == 1 )
  {
    WRITE_FLAG( pcPPS->getUniformSpacingIdr(),                                   "uniform_spacing_flag" );
#if !NONCROSS_TILE_IN_LOOP_FILTERING
    WRITE_FLAG( pcPPS->getTileBoundaryIndependenceIdr(),                         "tile_boundary_independence_flag" );
#endif
    WRITE_UVLC( pcPPS->getNumColumnsMinus1(),                                    "num_tile_columns_minus1" );
    WRITE_UVLC( pcPPS->getNumRowsMinus1(),                                       "num_tile_rows_minus1" );
    if( pcPPS->getUniformSpacingIdr() == 0 )
    {
      for(UInt i=0; i<pcPPS->getNumColumnsMinus1(); i++)
      {
        WRITE_UVLC( pcPPS->getColumnWidth(i),                                    "column_width" );
      }
      for(UInt i=0; i<pcPPS->getNumRowsMinus1(); i++)
      {
        WRITE_UVLC( pcPPS->getRowHeight(i),                                      "row_height" );
      }
    }
  }
#if NONCROSS_TILE_IN_LOOP_FILTERING
  if(pcPPS->getTileBehaviorControlPresentFlag() == 1)
  {
    Int iNumColTilesMinus1 = (pcPPS->getColumnRowInfoPresent() == 1)?(pcPPS->getNumColumnsMinus1()):(pcPPS->getSPS()->getNumColumnsMinus1());
    Int iNumRowTilesMinus1 = (pcPPS->getColumnRowInfoPresent() == 1)?(pcPPS->getNumColumnsMinus1()):(pcPPS->getSPS()->getNumRowsMinus1());

    if(iNumColTilesMinus1 !=0 || iNumRowTilesMinus1 !=0)
    {
      WRITE_FLAG( pcPPS->getTileBoundaryIndependenceIdr(),                         "tile_boundary_independence_flag" );
      if(pcPPS->getTileBoundaryIndependenceIdr() == 1)
      {
        WRITE_FLAG( pcPPS->getLFCrossTileBoundaryFlag()?1 : 0,            "loop_filter_across_tile_flag");
      }
    }
  }
#endif

  return;
}

Void TEncCavlc::codeSPS( TComSPS* pcSPS )
{
#if ENC_DEC_TRACE  
  xTraceSPSHeader (pcSPS);
#endif
  WRITE_CODE( pcSPS->getProfileIdc (),     8,       "profile_idc" );
  WRITE_CODE( 0,                           8,       "reserved_zero_8bits" );
  WRITE_CODE( pcSPS->getLevelIdc (),       8,       "level_idc" );
  WRITE_UVLC( pcSPS->getSPSId (),                   "seq_parameter_set_id" );
  WRITE_UVLC( pcSPS->getChromaFormatIdc (),         "chroma_format_idc" );
  WRITE_CODE( pcSPS->getMaxTLayers() - 1,  3,       "max_temporal_layers_minus1" );
#if PIC_SIZE_VLC
  WRITE_UVLC( pcSPS->getWidth (),                   "pic_width_in_luma_samples" );
  WRITE_UVLC( pcSPS->getHeight(),                   "pic_height_in_luma_samples" );
#else
  WRITE_CODE( pcSPS->getWidth (),         16,       "pic_width_in_luma_samples" );
  WRITE_CODE( pcSPS->getHeight(),         16,       "pic_height_in_luma_samples" );
#endif

#if FULL_NBIT
  WRITE_UVLC( pcSPS->getBitDepth() - 8,             "bit_depth_luma_minus8" );
#else
  WRITE_UVLC( pcSPS->getBitIncrement(),             "bit_depth_luma_minus8" );
#endif
#if FULL_NBIT
  WRITE_UVLC( pcSPS->getBitDepth() - 8,             "bit_depth_chroma_minus8" );
#else
  WRITE_UVLC( pcSPS->getBitIncrement(),             "bit_depth_chroma_minus8" );
#endif

#if MAX_PCM_SIZE
  WRITE_FLAG( pcSPS->getUsePCM() ? 1 : 0,                   "pcm_enabled_flag");
#endif

#if MAX_PCM_SIZE
  if( pcSPS->getUsePCM() )
#endif
  {
  WRITE_CODE( pcSPS->getPCMBitDepthLuma() - 1, 4,   "pcm_bit_depth_luma_minus1" );
  WRITE_CODE( pcSPS->getPCMBitDepthChroma() - 1, 4, "pcm_bit_depth_chroma_minus1" );
  }
  WRITE_UVLC( pcSPS->getBitsForPOC()-4,                 "log2_max_pic_order_cnt_lsb_minus4" );
  WRITE_UVLC( pcSPS->getMaxNumberOfReferencePictures(), "max_num_ref_pics" ); 
  WRITE_UVLC( pcSPS->getNumReorderFrames(),             "num_reorder_frames" ); 
#if MAX_DPB_AND_LATENCY
  WRITE_UVLC(pcSPS->getMaxDecFrameBuffering(),          "max_dec_frame_buffering" );
  WRITE_UVLC(pcSPS->getMaxLatencyIncrease(),            "max_latency_increase"    );
#endif
#if !G507_COND_4X4_ENABLE_FLAG
  xWriteFlag  ( (pcSPS->getDisInter4x4()) ? 1 : 0 );
#endif  
  assert( pcSPS->getMaxCUWidth() == pcSPS->getMaxCUHeight() );
  
  UInt MinCUSize = pcSPS->getMaxCUWidth() >> ( pcSPS->getMaxCUDepth()-g_uiAddCUDepth );
  UInt log2MinCUSize = 0;
  while(MinCUSize > 1)
  {
    MinCUSize >>= 1;
    log2MinCUSize++;
  }

  WRITE_UVLC( log2MinCUSize - 3,                                                     "log2_min_coding_block_size_minus3" );
  WRITE_UVLC( pcSPS->getMaxCUDepth()-g_uiAddCUDepth,                                 "log2_diff_max_min_coding_block_size" );
  WRITE_UVLC( pcSPS->getQuadtreeTULog2MinSize() - 2,                                 "log2_min_transform_block_size_minus2" );
  WRITE_UVLC( pcSPS->getQuadtreeTULog2MaxSize() - pcSPS->getQuadtreeTULog2MinSize(), "log2_diff_max_min_transform_block_size" );

#if G507_COND_4X4_ENABLE_FLAG
  if(log2MinCUSize == 3)
  {
    xWriteFlag  ( (pcSPS->getDisInter4x4()) ? 1 : 0 );
  }
#endif

#if MAX_PCM_SIZE
  if( pcSPS->getUsePCM() )
  {
#endif
  WRITE_UVLC( pcSPS->getPCMLog2MinSize() - 3,                                        "log2_min_pcm_coding_block_size_minus3" );

#if MAX_PCM_SIZE
  WRITE_UVLC( pcSPS->getPCMLog2MaxSize() - pcSPS->getPCMLog2MinSize(),               "log2_diff_max_min_pcm_coding_block_size" );
  }
#endif
  WRITE_UVLC( pcSPS->getQuadtreeTUMaxDepthInter() - 1,                               "max_transform_hierarchy_depth_inter" );
  WRITE_UVLC( pcSPS->getQuadtreeTUMaxDepthIntra() - 1,                               "max_transform_hierarchy_depth_intra" );
#if SCALING_LIST
  WRITE_FLAG( (pcSPS->getScalingListFlag()) ? 1 : 0,                                  "scaling_list_enabled_flag" ); 
#endif
  WRITE_FLAG  ( (pcSPS->getUseLMChroma ()) ? 1 : 0,                                  "chroma_pred_from_luma_enabled_flag" ); 
  WRITE_FLAG( pcSPS->getUseDF() ? 1 : 0,                                             "deblocking_filter_in_aps_enabled_flag");
  WRITE_FLAG( pcSPS->getLFCrossSliceBoundaryFlag()?1 : 0,                            "loop_filter_across_slice_flag");
  WRITE_FLAG( pcSPS->getUseSAO() ? 1 : 0,                                            "sample_adaptive_offset_enabled_flag");
  WRITE_FLAG( (pcSPS->getUseALF ()) ? 1 : 0,                                         "adaptive_loop_filter_enabled_flag");

#if MAX_PCM_SIZE
  if( pcSPS->getUsePCM() )
#endif
  {
  WRITE_FLAG( pcSPS->getPCMFilterDisableFlag()?1 : 0,                                "pcm_loop_filter_disable_flag");
  }

#if !G507_QP_ISSUE_FIX
  WRITE_FLAG( (pcSPS->getUseDQP ()) ? 1 : 0,                                         "cu_qp_delta_enabled_flag" );
#endif
  assert( pcSPS->getMaxTLayers() > 0 );         

  WRITE_FLAG( pcSPS->getTemporalIdNestingFlag() ? 1 : 0,                             "temporal_id_nesting_flag" );

  // !!!KS: Syntax not in WD !!!
  
  xWriteUvlc  ( pcSPS->getPad (0) );
  xWriteUvlc  ( pcSPS->getPad (1) );

  // Tools
  xWriteFlag  ( (pcSPS->getUseMRG ()) ? 1 : 0 ); // SOPH:
  
  // AMVP mode for each depth
  for (Int i = 0; i < pcSPS->getMaxCUDepth(); i++)
  {
    xWriteFlag( pcSPS->getAMVPMode(i) ? 1 : 0);
  }

  WRITE_FLAG( pcSPS->getUniformSpacingIdr(),                          "uniform_spacing_flag" );
#if !NONCROSS_TILE_IN_LOOP_FILTERING
  WRITE_FLAG( pcSPS->getTileBoundaryIndependenceIdr(),                "tile_boundary_independence_flag" );
#endif
  WRITE_UVLC( pcSPS->getNumColumnsMinus1(),                           "num_tile_columns_minus1" );
  WRITE_UVLC( pcSPS->getNumRowsMinus1(),                              "num_tile_rows_minus1" );
  if( pcSPS->getUniformSpacingIdr()==0 )
  {
    for(UInt i=0; i<pcSPS->getNumColumnsMinus1(); i++)
    {
      WRITE_UVLC( pcSPS->getColumnWidth(i),                           "column_width" );
    }
    for(UInt i=0; i<pcSPS->getNumRowsMinus1(); i++)
    {
      WRITE_UVLC( pcSPS->getRowHeight(i),                             "row_height" );
    }
  }

#if NONCROSS_TILE_IN_LOOP_FILTERING
  if( pcSPS->getNumColumnsMinus1() !=0 || pcSPS->getNumRowsMinus1() != 0)
  {
    WRITE_FLAG( pcSPS->getTileBoundaryIndependenceIdr(),                "tile_boundary_independence_flag" );
    if(pcSPS->getTileBoundaryIndependenceIdr() == 1)
    {
      WRITE_FLAG( pcSPS->getLFCrossTileBoundaryFlag()?1 : 0,            "loop_filter_across_tile_flag");
    }
  }
#endif

  // Software-only flags
#if NSQT
  WRITE_FLAG( pcSPS->getUseNSQT(), "enable_nsqt" );
#endif
  WRITE_FLAG( pcSPS->getUseAMP(), "enable_amp" );
}

#if TILES_DECODER
Void TEncCavlc::writeTileMarker( UInt uiTileIdx, UInt uiBitsUsed )
{
  xWriteCode( uiTileIdx, uiBitsUsed );
}
#endif

Void TEncCavlc::codeSliceHeader         ( TComSlice* pcSlice )
{
#if ENC_DEC_TRACE  
  xTraceSliceHeader (pcSlice);
#endif

#if SLICEADDR_BEGIN
  // if( nal_ref_idc != 0 )
  //   dec_ref_pic_marking( )
  // if( entropy_coding_mode_flag  &&  slice_type  !=  I)
  //   cabac_init_idc
  // first_slice_in_pic_flag
  // if( first_slice_in_pic_flag == 0 )
  //    slice_address
  //calculate number of bits required for slice address
  Int maxAddrOuter = pcSlice->getPic()->getNumCUsInFrame();
  Int reqBitsOuter = 0;
  while(maxAddrOuter>(1<<reqBitsOuter)) 
  {
    reqBitsOuter++;
  }
  Int maxAddrInner = pcSlice->getPic()->getNumPartInCU()>>(2);
  maxAddrInner = (1<<(pcSlice->getPPS()->getSliceGranularity()<<1));
  Int reqBitsInner = 0;
  
  while(maxAddrInner>(1<<reqBitsInner))
  {
    reqBitsInner++;
  }
  Int lCUAddress;
  Int innerAddress;
  if (pcSlice->isNextSlice())
  {
    // Calculate slice address
    lCUAddress = (pcSlice->getSliceCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU());
    innerAddress = (pcSlice->getSliceCurStartCUAddr()%(pcSlice->getPic()->getNumPartInCU()))>>((pcSlice->getSPS()->getMaxCUDepth()-pcSlice->getPPS()->getSliceGranularity())<<1);
  }
  else
  {
    // Calculate slice address
    lCUAddress = (pcSlice->getEntropySliceCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU());
    innerAddress = (pcSlice->getEntropySliceCurStartCUAddr()%(pcSlice->getPic()->getNumPartInCU()))>>((pcSlice->getSPS()->getMaxCUDepth()-pcSlice->getPPS()->getSliceGranularity())<<1);
    
  }
  //write slice address
  Int address = (pcSlice->getPic()->getPicSym()->getCUOrderMap(lCUAddress) << reqBitsInner) + innerAddress;
  WRITE_FLAG( address==0, "first_slice_in_pic_flag" );
  if(address>0) 
  {
    WRITE_CODE( address, reqBitsOuter+reqBitsInner, "slice_address" );
  }
#endif

#if INC_CABACINITIDC_SLICETYPE
  WRITE_UVLC( pcSlice->getSliceType(),       "slice_type" );
#endif
  Bool bEntropySlice = (!pcSlice->isNextSlice());
  WRITE_FLAG( bEntropySlice ? 1 : 0, "lightweight_slice_flag" );
  
  if (!bEntropySlice)
  {
#if !INC_CABACINITIDC_SLICETYPE
    WRITE_UVLC( pcSlice->getSliceType(),       "slice_type" );
#endif
    WRITE_UVLC( pcSlice->getPPS()->getPPSId(), "pic_parameter_set_id" );
    if(pcSlice->getNalUnitType()==NAL_UNIT_CODED_SLICE_IDR) 
    {
      WRITE_UVLC( 0, "idr_pic_id" );
      WRITE_FLAG( 0, "no_output_of_prior_pics_flag" );
    }
    else
    {
      WRITE_CODE( (pcSlice->getPOC()-pcSlice->getLastIDR()+(1<<pcSlice->getSPS()->getBitsForPOC()))%(1<<pcSlice->getSPS()->getBitsForPOC()), pcSlice->getSPS()->getBitsForPOC(), "pic_order_cnt_lsb");
      TComReferencePictureSet* pcRPS = pcSlice->getRPS();
      if(pcSlice->getRPSidx() < 0)
      {
        WRITE_FLAG( 0, "short_term_ref_pic_set_pps_flag");
        codeShortTermRefPicSet(pcSlice->getPPS(), pcRPS);
      }
      else
      {
        WRITE_FLAG( 1, "short_term_ref_pic_set_pps_flag");
        WRITE_UVLC( pcSlice->getRPSidx(), "short_term_ref_pic_set_idx" );
      }
      if(pcSlice->getPPS()->getLongTermRefsPresent())
      {
        WRITE_UVLC( pcRPS->getNumberOfLongtermPictures(), "num_long_term_pics");
        Int maxPocLsb = 1<<pcSlice->getSPS()->getBitsForPOC();
        Int prev = 0;
        for(Int i=pcRPS->getNumberOfPictures()-1 ; i > pcRPS->getNumberOfPictures()-pcRPS->getNumberOfLongtermPictures()-1; i--)
        {
          WRITE_UVLC((maxPocLsb-pcRPS->getDeltaPOC(i)+prev-1)%maxPocLsb, "delta_poc_lsb_lt_minus1");
          prev = pcRPS->getDeltaPOC(i);
          WRITE_FLAG( pcRPS->getUsed(i), "used_by_curr_pic_lt_flag"); 
        }
      }
    }
#if SCALING_LIST
    if(pcSlice->getSPS()->getUseSAO() || pcSlice->getSPS()->getUseALF()|| pcSlice->getSPS()->getScalingListFlag() || pcSlice->getSPS()->getUseDF())
#else
    if(pcSlice->getSPS()->getUseSAO() || pcSlice->getSPS()->getUseALF())
#endif
    {
#if ALF_SAO_SLICE_FLAGS
      if (pcSlice->getSPS()->getUseALF())
      {
         if (pcSlice->getAlfEnabledFlag())
         {
           assert (pcSlice->getAPS()->getAlfEnabled());
         }
         WRITE_FLAG( pcSlice->getAlfEnabledFlag(), "ALF on/off flag in slice header" );
      }
      if (pcSlice->getSPS()->getUseSAO())
      {
         assert (pcSlice->getSaoEnabledFlag() == pcSlice->getAPS()->getSaoEnabled());
         WRITE_FLAG( pcSlice->getSaoEnabledFlag(), "SAO on/off flag in slice header" );
      }
#endif
      WRITE_UVLC( pcSlice->getAPS()->getAPSID(), "aps_id");
    }

    // we always set num_ref_idx_active_override_flag equal to one. this might be done in a more intelligent way 
    if (!pcSlice->isIntra())
    {
      WRITE_FLAG( 1 ,                                             "num_ref_idx_active_override_flag");
      WRITE_CODE( pcSlice->getNumRefIdx( REF_PIC_LIST_0 ) - 1, 3, "num_ref_idx_l0_active_minus1" );
    }
    else
    {
      pcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
    }
    if (pcSlice->isInterB())
    {
      WRITE_CODE( pcSlice->getNumRefIdx( REF_PIC_LIST_1 ) - 1, 3, "num_ref_idx_l1_active_minus1" );
    }
    else
    {
      pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
    }
    TComRefPicListModification* refPicListModification = pcSlice->getRefPicListModification();
    if(!pcSlice->isIntra())
    {
      WRITE_FLAG(pcSlice->getRefPicListModification()->getRefPicListModificationFlagL0() ? 1 : 0,       "ref_pic_list_modification_flag" );    
      for(Int i = 0; i < refPicListModification->getNumberOfRefPicListModificationsL0(); i++)
      {
        WRITE_UVLC( refPicListModification->getListIdcL0(i), "ref_pic_list_modification_idc");
        WRITE_UVLC( refPicListModification->getRefPicSetIdxL0(i), "ref_pic_set_idx");
      }
      if(pcSlice->getRefPicListModification()->getRefPicListModificationFlagL0())
        WRITE_UVLC( 3, "ref_pic_list_modification_idc");
    }
    if(pcSlice->isInterB())
    {    
      WRITE_FLAG(pcSlice->getRefPicListModification()->getRefPicListModificationFlagL1() ? 1 : 0,       "ref_pic_list_modification_flag" );
      for(Int i = 0; i < refPicListModification->getNumberOfRefPicListModificationsL1(); i++)
      {
        WRITE_UVLC( refPicListModification->getListIdcL1(i), "ref_pic_list_modification_idc");
        WRITE_UVLC( refPicListModification->getRefPicSetIdxL1(i), "ref_pic_set_idx");
      }
      if(pcSlice->getRefPicListModification()->getRefPicListModificationFlagL1())
        WRITE_UVLC( 3, "ref_pic_list_modification_idc");
    }
  }
  // ref_pic_list_combination( )
  // maybe move to own function?
  if (pcSlice->isInterB())
  {
    WRITE_FLAG(pcSlice->getRefPicListCombinationFlag() ? 1 : 0,       "ref_pic_list_combination_flag" );
    if(pcSlice->getRefPicListCombinationFlag())
    {
      WRITE_UVLC( pcSlice->getNumRefIdx(REF_PIC_LIST_C) - 1,          "num_ref_idx lc_active_minus1");
      
      WRITE_FLAG( pcSlice->getRefPicListModificationFlagLC() ? 1 : 0, "ref_pic_list_modification_flag_lc" );
      if(pcSlice->getRefPicListModificationFlagLC())
      {
        for (UInt i=0;i<pcSlice->getNumRefIdx(REF_PIC_LIST_C);i++)
        {
          WRITE_FLAG( pcSlice->getListIdFromIdxOfLC(i),               "pic_from_list_0_flag" );
          WRITE_UVLC( pcSlice->getRefIdxFromIdxOfLC(i),               "ref_idx_list_curr" );
        }
      }
    }
  }
    
#if INC_CABACINITIDC_SLICETYPE
  if(pcSlice->getPPS()->getEntropyCodingMode() && !pcSlice->isIntra())
  {
    WRITE_UVLC(pcSlice->getCABACinitIDC(),  "cabac_init_idc");
  }
#endif

#if !SLICEADDR_BEGIN
  // if( nal_ref_idc != 0 )
  //   dec_ref_pic_marking( )
  // if( entropy_coding_mode_flag  &&  slice_type  !=  I)
  //   cabac_init_idc
  // first_slice_in_pic_flag
  // if( first_slice_in_pic_flag == 0 )
  //    slice_address
  //calculate number of bits required for slice address
  Int iMaxAddrOuter = pcSlice->getPic()->getNumCUsInFrame();
  Int iReqBitsOuter = 0;
  while(iMaxAddrOuter>(1<<iReqBitsOuter)) 
  {
    iReqBitsOuter++;
  }
  Int iMaxAddrInner = pcSlice->getPic()->getNumPartInCU()>>(2);
  iMaxAddrInner = (1<<(pcSlice->getPPS()->getSliceGranularity()<<1));
  int iReqBitsInner = 0;
  
  while(iMaxAddrInner>(1<<iReqBitsInner))
  {
    iReqBitsInner++;
  }
  Int iLCUAddress;
  Int iInnerAddress;
  if (pcSlice->isNextSlice())
  {
    // Calculate slice address
    iLCUAddress = (pcSlice->getSliceCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU());
    iInnerAddress = (pcSlice->getSliceCurStartCUAddr()%(pcSlice->getPic()->getNumPartInCU()))>>((pcSlice->getSPS()->getMaxCUDepth()-pcSlice->getPPS()->getSliceGranularity())<<1);
  }
  else
  {
    // Calculate slice address
    iLCUAddress = (pcSlice->getEntropySliceCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU());
    iInnerAddress = (pcSlice->getEntropySliceCurStartCUAddr()%(pcSlice->getPic()->getNumPartInCU()))>>((pcSlice->getSPS()->getMaxCUDepth()-pcSlice->getPPS()->getSliceGranularity())<<1);
  }
  //write slice address
  int iAddress = (pcSlice->getPic()->getPicSym()->getCUOrderMap(iLCUAddress) << iReqBitsInner) + iInnerAddress;
  WRITE_FLAG( iAddress==0, "first_slice_in_pic_flag" );
  if(iAddress>0) 
  {
    WRITE_CODE( iAddress, iReqBitsOuter+iReqBitsInner, "slice_address" );
  }
#endif
  
  // if( !lightweight_slice_flag ) {
  if (!bEntropySlice)
  {
#if G507_QP_ISSUE_FIX
    Int iCode = pcSlice->getSliceQp() - ( pcSlice->getPPS()->getPicInitQPMinus26() + 26 );
    WRITE_SVLC( iCode, "slice_qp_delta" ); 
#else
    WRITE_SVLC( pcSlice->getSliceQp(), "slice_qp" ); // this should be delta
#endif
  //   if( sample_adaptive_offset_enabled_flag )
  //     sao_param()
  //   if( deblocking_filter_control_present_flag ) {
  //     disable_deblocking_filter_idc
    WRITE_FLAG(pcSlice->getInheritDblParamFromAPS(), "inherit_dbl_param_from_APS_flag");
    if (!pcSlice->getInheritDblParamFromAPS())
    {
      WRITE_FLAG(pcSlice->getLoopFilterDisable(), "loop_filter_disable");  // should be an IDC
      if(!pcSlice->getLoopFilterDisable())
      {
        WRITE_SVLC (pcSlice->getLoopFilterBetaOffset(), "beta_offset_div2");
        WRITE_SVLC (pcSlice->getLoopFilterTcOffset(), "tc_offset_div2");
      }
    }
  //     if( disable_deblocking_filter_idc  !=  1 ) {
  //       slice_alpha_c0_offset_div2
  //       slice_beta_offset_div2
  //     }
  //   }
  //   if( slice_type = = B )
  //   collocated_from_l0_flag
    if ( pcSlice->getSliceType() == B_SLICE )
    {
      WRITE_FLAG( pcSlice->getColDir(), "collocated_from_l0_flag" );
    }
    //   if( adaptive_loop_filter_enabled_flag ) {
  //     if( !shared_pps_info_enabled_flag )
  //       alf_param( )
  //     alf_cu_control_param( )
  //   }
  // }
  
    if ( (pcSlice->getPPS()->getUseWP() && pcSlice->getSliceType()==P_SLICE) || (pcSlice->getPPS()->getWPBiPredIdc()==1 && pcSlice->getSliceType()==B_SLICE) )
    {
      xCodePredWeightTable( pcSlice );
    }
  }

  // !!!! sytnax elements not in the WD !!!!
  
#if G091_SIGNAL_MAX_NUM_MERGE_CANDS
  assert(pcSlice->getMaxNumMergeCand()<=MRG_MAX_NUM_CANDS_SIGNALED);
  assert(MRG_MAX_NUM_CANDS_SIGNALED<=MRG_MAX_NUM_CANDS);
  WRITE_UVLC(MRG_MAX_NUM_CANDS - pcSlice->getMaxNumMergeCand(), "maxNumMergeCand");
#endif

#if !G220_PURE_VLC_SAO_ALF
#if TILES_DECODER
  if (!bEntropySlice && pcSlice->getSPS()->getTileBoundaryIndependenceIdr())
  {
    xWriteFlag  (pcSlice->getTileMarkerFlag() ? 1 : 0 );
  }
#endif
#endif
}


#if G220_PURE_VLC_SAO_ALF
#if TILES_DECODER
Void TEncCavlc::codeTileMarkerFlag(TComSlice* pcSlice) 
{
  Bool bEntropySlice = (!pcSlice->isNextSlice());
#if TILES_LOW_LATENCY_CABAC_INI
  if (!bEntropySlice)
#else
  if (!bEntropySlice && pcSlice->getSPS()->getTileBoundaryIndependenceIdr())
#endif
  {
    xWriteFlag  (pcSlice->getTileMarkerFlag() ? 1 : 0 );
  }
}
#endif
#endif

/**
 - write wavefront substreams sizes for the slice header.
 .
 \param pcSlice Where we find the substream size information.
 */
Void TEncCavlc::codeSliceHeaderSubstreamTable( TComSlice* pcSlice )
{
  UInt uiNumSubstreams = pcSlice->getPPS()->getNumSubstreams();
  UInt*puiSubstreamSizes = pcSlice->getSubstreamSizes();

  // Write header information for all substreams except the last.
  for (UInt ui = 0; ui+1 < uiNumSubstreams; ui++)
  {
    UInt uiNumbits = puiSubstreamSizes[ui];

    //the 2 first bits are used to give the size of the header
    if ( uiNumbits < (1<<8) )
    {
      xWriteCode(0,         2  );
      xWriteCode(uiNumbits, 8  );
    }
    else if ( uiNumbits < (1<<16) )
    {
      xWriteCode(1,         2  );
      xWriteCode(uiNumbits, 16 );
    }
    else if ( uiNumbits < (1<<24) )
    {
      xWriteCode(2,         2  );
      xWriteCode(uiNumbits, 24 );
    }
    else if ( uiNumbits < (1<<31) )
    {
      xWriteCode(3,         2  );
      xWriteCode(uiNumbits, 32 );
    }
    else
    {
      printf("Error in codeSliceHeaderTable\n");
      exit(-1);
    }
  }
}

Void TEncCavlc::codeTerminatingBit      ( UInt uilsLast )
{
}

Void TEncCavlc::codeSliceFinish ()
{
  if ( m_bRunLengthCoding && m_uiRun)
  {
    xWriteUvlc(m_uiRun);
  }
}

Void TEncCavlc::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iSymbol = pcCU->getMVPIdx(eRefList, uiAbsPartIdx);
  Int iNum = AMVP_MAX_NUM_CANDS;
  xWriteUnaryMaxSymbol(iSymbol, iNum-1);
}

Void TEncCavlc::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if ( pcCU->getSlice()->isIntra() && pcCU->isIntra( uiAbsPartIdx ) )
  {
    if( uiDepth == (g_uiMaxCUDepth - g_uiAddCUDepth))
    {
      xWriteFlag( pcCU->getPartitionSize(uiAbsPartIdx ) == SIZE_2Nx2N? 1 : 0 );
    }
    return;
  }


  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
  {    
    if ((pcCU->getPartitionSize(uiAbsPartIdx ) == SIZE_NxN) || pcCU->isIntra( uiAbsPartIdx ))
    {
      UInt uiIntraFlag = ( pcCU->isIntra(uiAbsPartIdx));
      if (pcCU->getPartitionSize(uiAbsPartIdx ) == SIZE_2Nx2N)
      {
        xWriteFlag(1);
      }
      else
      {
        xWriteFlag(0);
        if(!( pcCU->getSlice()->getSPS()->getDisInter4x4() && (pcCU->getWidth(uiAbsPartIdx)==8) && (pcCU->getHeight(uiAbsPartIdx)==8) ))
        {
        xWriteFlag( uiIntraFlag? 1 : 0 );
        }
      }

      return;
    }
  }
}

/** code prediction mode
 * \param pcCU
 * \param uiAbsPartIdx 
 * \returns Void
 */
Void TEncCavlc::codePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  codeInterModeFlag(pcCU, uiAbsPartIdx,(UInt)pcCU->getDepth(uiAbsPartIdx),2);
  return;
}

/** code merge flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \returns Void
 */
Void TEncCavlc::codeMergeFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N )
     return;
  UInt uiSymbol = pcCU->getMergeFlag( uiAbsPartIdx ) ? 1 : 0;
  xWriteFlag( uiSymbol );
}

/** code merge index
 * \param pcCU
 * \param uiAbsPartIdx 
 * \returns Void
 */
Void TEncCavlc::codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiNumCand = MRG_MAX_NUM_CANDS;
  assert( uiNumCand > 1 );
  UInt uiUnaryIdx = pcCU->getMergeIndex( uiAbsPartIdx );
#if G091_SIGNAL_MAX_NUM_MERGE_CANDS
  uiNumCand = pcCU->getSlice()->getMaxNumMergeCand();
  if ( uiNumCand > 1 )
  {
#endif
    for( UInt ui = 0; ui < uiNumCand - 1; ++ui )
    {
      const UInt uiSymbol = ui == uiUnaryIdx ? 0 : 1;
      xWriteFlag( uiSymbol );
      if( uiSymbol == 0 )
      {
        break;
      }
    }
#if G091_SIGNAL_MAX_NUM_MERGE_CANDS
  }
#endif
}

Void TEncCavlc::codeAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{  
  if (!m_bAlfCtrl)
    return;
  
  if( pcCU->getDepth(uiAbsPartIdx) > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, m_uiMaxAlfCtrlDepth))
  {
    return;
  }
  
  // get context function is here
  UInt uiSymbol = pcCU->getAlfCtrlFlag( uiAbsPartIdx ) ? 1 : 0;
  
  xWriteFlag( uiSymbol );
}

Void TEncCavlc::codeAlfCtrlDepth()
{  
  if (!m_bAlfCtrl)
    return;
  
  UInt uiDepth = m_uiMaxAlfCtrlDepth;
  
  xWriteUnaryMaxSymbol(uiDepth, g_uiMaxCUDepth-1);
}

Void TEncCavlc::codeInterModeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiEncMode )
{
  Bool bHasSplit = ( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )? 0 : 1;
  UInt uiSplitFlag = ( pcCU->getDepth( uiAbsPartIdx ) > uiDepth ) ? 1 : 0;
  UInt uiMode=0,uiControl=0;
  UInt uiTableDepth = uiDepth;
  if ( !bHasSplit )
  {
    uiTableDepth = 3;
  }
  if(!uiSplitFlag || !bHasSplit)
  {
    uiMode = 1;
    uiControl = 1;
    if (!pcCU->isSkipped(uiAbsPartIdx ))
    {
      uiControl = 2;
      uiMode = 6;
      if (pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER)
      {
        if(pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N)
          uiMode=pcCU->getMergeFlag(uiAbsPartIdx) ? 2 : 3;
        else 
          uiMode=3+(UInt)pcCU->getPartitionSize(uiAbsPartIdx);
      }
    }
  }
  if (uiEncMode != uiControl )
    return;
  UInt uiEndSym = bHasSplit ? 11 : 11;
  uiDepth = uiTableDepth;
  UInt uiLength = m_uiSplitTableE[uiDepth][uiMode] + 1;
  if (uiLength == uiEndSym)
  {
    xWriteCode( 0, uiLength - 1);
  }
  else
  {
    xWriteCode( 1, uiLength );
  }
  UInt x = uiMode;
  UInt cx = m_uiSplitTableE[uiDepth][x];
  /* Adapt table */
  if ( m_bAdaptFlag)
  {
    adaptCodeword(cx, m_ucSplitTableCounter[uiDepth],  m_ucSplitTableCounterSum[uiDepth],   m_uiSplitTableD[uiDepth],  m_uiSplitTableE[uiDepth], 4);
  }
  return;
}

Void TEncCavlc::codeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  codeInterModeFlag(pcCU,uiAbsPartIdx,(UInt)pcCU->getDepth(uiAbsPartIdx),1);
  return;
}

Void TEncCavlc::codeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    return;
  if (!pcCU->getSlice()->isIntra())
  {
    codeInterModeFlag(pcCU,uiAbsPartIdx,uiDepth,0);
    return;
  }
  UInt uiCurrSplitFlag = ( pcCU->getDepth( uiAbsPartIdx ) > uiDepth ) ? 1 : 0;
  
  xWriteFlag( uiCurrSplitFlag );
  return;
}

Void TEncCavlc::codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  xWriteFlag( uiSymbol );
}

Void TEncCavlc::codeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  UInt uiCbf = pcCU->getCbf( uiAbsPartIdx, eType, uiTrDepth );
  xWriteFlag( uiCbf );
}

Void TEncCavlc::codeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiCbf = pcCU->getQtRootCbf( uiAbsPartIdx );
  xWriteFlag( uiCbf ? 1 : 0 );
}

/** Code I_PCM information. 
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \returns Void
 *
 * If I_PCM flag indicates that the CU is I_PCM, code its PCM alignment bits and codes.  
 */
Void TEncCavlc::codeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx)
{
  UInt uiIPCM = (pcCU->getIPCMFlag(uiAbsPartIdx) == true)? 1 : 0;

  xWriteFlag(uiIPCM);

  if (uiIPCM)
  {
    xWritePCMAlignZero();

    UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
    UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
    UInt uiChromaOffset = uiLumaOffset>>2;

    Pel* piPCMSample;
    UInt uiWidth;
    UInt uiHeight;
    UInt uiSampleBits;
    UInt uiX, uiY;

    piPCMSample = pcCU->getPCMSampleY() + uiLumaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx);
    uiHeight = pcCU->getHeight(uiAbsPartIdx);
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample = piPCMSample[uiX];

        xWriteCode(uiSample, uiSampleBits);
      }
      piPCMSample += uiWidth;
    }

    piPCMSample = pcCU->getPCMSampleCb() + uiChromaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx)/2;
    uiHeight = pcCU->getHeight(uiAbsPartIdx)/2;
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample = piPCMSample[uiX];

        xWriteCode(uiSample, uiSampleBits);
      }
      piPCMSample += uiWidth;
    }

    piPCMSample = pcCU->getPCMSampleCr() + uiChromaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx)/2;
    uiHeight = pcCU->getHeight(uiAbsPartIdx)/2;
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample = piPCMSample[uiX];

        xWriteCode(uiSample, uiSampleBits);
      }
      piPCMSample += uiWidth;
    }
  }
}

Void TEncCavlc::codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDir         = pcCU->getLumaIntraDir( uiAbsPartIdx );
  Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);

  Int uiPreds[2] = {-1, -1};
  Int uiPredNum = pcCU->getIntraDirLumaPredictor(uiAbsPartIdx, uiPreds);  

  Int uiPredIdx = -1;

  for(UInt i = 0; i < uiPredNum; i++)
  {
    if(iDir == uiPreds[i])
    {
      uiPredIdx = i;
    }
  }

  if ( g_aucIntraModeBitsAng[iIntraIdx] < 5 )
  {
    if(uiPredIdx != -1)
    {
      xWriteFlag(1);
      xWriteFlag( (UInt)uiPredIdx );
    }
    else
    {
      xWriteFlag(0);

      for(Int i = (uiPredNum - 1); i >= 0; i--)
      {
        iDir = iDir > uiPreds[i] ? iDir - 1 : iDir;
      }

      xWriteFlag( iDir & 0x01 ? 1 : 0 );
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) { xWriteFlag( iDir & 0x02 ? 1 : 0 ); }
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) { xWriteFlag( iDir & 0x04 ? 1 : 0 ); }
    }
    
  }
  else 
  {
     UInt uiCode, uiLength;
     Int iRankIntraMode, iRankIntraModeLarger, iDirLarger;
      
     const UInt *huff;
     const UInt *lengthHuff;
     UInt  *m_uiIntraModeTableD;
     UInt  *m_uiIntraModeTableE;

     if( g_aucIntraModeBitsAng[iIntraIdx] == 5 )
     {
       huff = huff17_2;
       lengthHuff = lengthHuff17_2;
       m_uiIntraModeTableD = m_uiIntraModeTableD17;
       m_uiIntraModeTableE = m_uiIntraModeTableE17;
     }
     else
     {
       huff = huff34_2;
       lengthHuff = lengthHuff34_2;
       m_uiIntraModeTableD = m_uiIntraModeTableD34;
       m_uiIntraModeTableE = m_uiIntraModeTableE34;
     }

     if(uiPredIdx != -1)
     {
       uiCode=huff[0];
       uiLength=lengthHuff[0];
       xWriteCode(uiCode, uiLength);
       xWriteFlag((UInt)uiPredIdx);
     }
     else
     {
       for(Int i = (uiPredNum - 1); i >= 0; i--)
       {
         iDir = iDir > uiPreds[i] ? iDir - 1 : iDir;
       }

       iRankIntraMode=m_uiIntraModeTableE[iDir];

       uiCode=huff[iRankIntraMode+1];
       uiLength=lengthHuff[iRankIntraMode+1];

       xWriteCode(uiCode, uiLength);

       if ( m_bAdaptFlag )
       {
         iRankIntraModeLarger = max(0,iRankIntraMode-1);
         iDirLarger = m_uiIntraModeTableD[iRankIntraModeLarger];
  
         m_uiIntraModeTableD[iRankIntraModeLarger] = iDir;
         m_uiIntraModeTableD[iRankIntraMode] = iDirLarger;
         m_uiIntraModeTableE[iDir] = iRankIntraModeLarger;
         m_uiIntraModeTableE[iDirLarger] = iRankIntraMode;
       }
     }
  }
}

Void TEncCavlc::codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIntraDirChroma = pcCU->getChromaIntraDir( uiAbsPartIdx );

  if (uiIntraDirChroma == DM_CHROMA_IDX ) 
  {
    xWriteFlag(0);
  } 
  else if (uiIntraDirChroma == LM_CHROMA_IDX)
  {
    xWriteCode( 2, 2 );
  }
  else
  { 
    UInt uiAllowedChromaDir[ NUM_CHROMA_MODE ];
    pcCU->getAllowedChromaDir( uiAbsPartIdx, uiAllowedChromaDir );

    for(Int i = 0; i < NUM_CHROMA_MODE - 2; i++ )
    {
      if( uiIntraDirChroma == uiAllowedChromaDir[i] )
      {
        uiIntraDirChroma = i;
        break;
      }
    }
    if( pcCU->getSlice()->getSPS()->getUseLMChroma() )
    {
      xWriteCode( 3, 2 );
    }
    else
    {
      xWriteFlag(1);
    }

    xWriteUnaryMaxSymbol( uiIntraDirChroma, 3 );
  }
  return;
}

Void TEncCavlc::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiInterDir = pcCU->getInterDir   ( uiAbsPartIdx );
  uiInterDir--;

  UInt uiNumRefIdxOfLC = pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C);
  UInt uiValNumRefIdxOfLC = min(4,pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C));
  UInt uiValNumRefIdxOfL0 = min(2,pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0));
  UInt uiValNumRefIdxOfL1 = min(2,pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1));

  if ( pcCU->getSlice()->getRefIdxCombineCoding() )
  {
    Int x,cx;
    Int iRefFrame0,iRefFrame1;
    UInt uiIndex = 0;
    
    UInt *m_uiMITableE;
    UInt *m_uiMITableD;
       
    m_uiMITableE = m_uiMI1TableE;
    m_uiMITableD = m_uiMI1TableD;

    UInt uiMaxVal = (uiNumRefIdxOfLC > 0 ? uiValNumRefIdxOfLC : uiValNumRefIdxOfL0) + uiValNumRefIdxOfL0*uiValNumRefIdxOfL1;

    if (uiInterDir==0)
    { 
      if(uiNumRefIdxOfLC > 0)
      {
        iRefFrame0 = pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_0, pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx ));
      }
      else
      {
        iRefFrame0 = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx );
      }
      uiIndex = iRefFrame0;
      if(uiNumRefIdxOfLC > 0)
      {
        if ( iRefFrame0 >= 4 )
        {
          uiIndex = uiMaxVal;
        }
      }
      else
      {
        if ( iRefFrame0 > 1 )
        {
          uiIndex = uiMaxVal;
        }
      }        
    }
    else if (uiInterDir==1)
    {
      if(uiNumRefIdxOfLC > 0)
      {
        iRefFrame1 = pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_1, pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx ));
        uiIndex = iRefFrame1;
      }
      else
      {
        iRefFrame1 = pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx );
        uiIndex = uiValNumRefIdxOfL0 + iRefFrame1;
      }
      if(uiNumRefIdxOfLC > 0)
      {
        if ( iRefFrame1 >= 4 )
        {
          uiIndex = uiMaxVal;
        }
      }
      else
      {
        if ( iRefFrame1 > 1 )
        {
          uiIndex = uiMaxVal;
        }
      } 
    }
    else
    {
      iRefFrame0 = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx );
      iRefFrame1 = pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx );
      if ( iRefFrame0 >= 2 || iRefFrame1 >= 2 )
      {
        uiIndex = uiMaxVal;
      }
      else
      {
        if(uiNumRefIdxOfLC > 0)
        {
          uiIndex = uiValNumRefIdxOfLC + iRefFrame0*uiValNumRefIdxOfL1 + iRefFrame1;
        } 
        else if (m_pcSlice->getNoBackPredFlag())
        {
          uiIndex = uiValNumRefIdxOfL0 + iRefFrame0*uiValNumRefIdxOfL1 + iRefFrame1;
        }
        else
        {
          uiIndex = uiValNumRefIdxOfL0 + uiValNumRefIdxOfL1 + iRefFrame0*uiValNumRefIdxOfL1 + iRefFrame1;
        }
      }
    }
      
    x = uiIndex;
    
    cx = m_uiMITableE[x];
    
    /* Adapt table */
    if ( m_bAdaptFlag )
    {        
      adaptCodeword(cx, m_ucMI1TableCounter,  m_ucMI1TableCounterSum,   m_uiMITableD,  m_uiMITableE, 4);
    }

    Bool bCodeException = false;
    if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 4 ||
         pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0) > 2 ||
         pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 2 )
    {
      bCodeException = true;
    }
    else
    {
      bCodeException = false;
      uiMaxVal--;
    }
    
    xWriteUnaryMaxSymbol( cx, uiMaxVal );
    
    if ( x<uiMaxVal || !bCodeException )
    {
      return;
    }
  }

  xWriteFlag( ( uiInterDir == 2 ? 1 : 0 ));
  
  return;
}

Void TEncCavlc::codeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iRefFrame;
  RefPicList eRefListTemp;

  if( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C)>0)
  {
    if ( pcCU->getInterDir( uiAbsPartIdx ) != 3)
    {
      eRefListTemp = REF_PIC_LIST_C;
      iRefFrame = pcCU->getSlice()->getRefIdxOfLC(eRefList, pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx ));
    }
    else
    {
      eRefListTemp = eRefList;
      iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );
    }
  }
  else
  {
    eRefListTemp = eRefList;
    iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );
  }

  if (pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2 && pcCU->getSlice()->isInterB())
  {
    return;
  }
  
  if ( pcCU->getSlice()->getRefIdxCombineCoding() && pcCU->getInterDir(uiAbsPartIdx)==3 &&
      pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx ) < 2 &&
      pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx ) < 2 )
  {
    return;
  }
  else if ( pcCU->getSlice()->getRefIdxCombineCoding() && pcCU->getInterDir(uiAbsPartIdx)==1 && 
           ( ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C)>0  && pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_0, pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx )) < 4 ) || 
            ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C)<=0 && pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx ) <= MS_LCEC_UNI_EXCEPTION_THRES ) ) )
  {
    return;
  }
  else if ( pcCU->getSlice()->getRefIdxCombineCoding() && pcCU->getInterDir(uiAbsPartIdx)==2 && pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_1, pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx )) < 4 )
  {
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
        assert( iRefFrame >=4 );
      }
      else
      {
        uiRefFrmIdxMinus = MS_LCEC_UNI_EXCEPTION_THRES+1;
        assert( iRefFrame > MS_LCEC_UNI_EXCEPTION_THRES );
      }
      
    }
    else if ( eRefList == REF_PIC_LIST_1 && pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx ) < 2 )
    {
      uiRefFrmIdxMinus = 2;
      assert( iRefFrame >= 2 );
    }
  }
  
  if ( pcCU->getSlice()->getNumRefIdx( eRefListTemp ) - uiRefFrmIdxMinus <= 1 )
  {
    return;
  }
  xWriteFlag( ( iRefFrame - uiRefFrmIdxMinus == 0 ? 0 : 1 ) );
  
  if ( iRefFrame - uiRefFrmIdxMinus > 0 )
  {
    {
      xWriteUnaryMaxSymbol( iRefFrame - 1 - uiRefFrmIdxMinus, pcCU->getSlice()->getNumRefIdx( eRefListTemp )-2 - uiRefFrmIdxMinus );
    }
  }
  return;
}

Void TEncCavlc::codeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  TComCUMvField* pcCUMvField = pcCU->getCUMvField( eRefList );
  Int iHor = pcCUMvField->getMvd( uiAbsPartIdx ).getHor();
  Int iVer = pcCUMvField->getMvd( uiAbsPartIdx ).getVer();
    
  xWriteSvlc( iHor );
  xWriteSvlc( iVer );
  
  return;
}

Void TEncCavlc::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getRefQP( uiAbsPartIdx );

  xWriteSvlc( iDQp );
  
  return;
}

/** Function for coding cbf and split flag
* \param pcCU pointer to CU
* \param uiAbsPartIdx CU index
* \param uiDepth CU Depth
* \returns 
* This function performs coding of cbf and split flag
*/
Void TEncCavlc::codeCbfTrdiv( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt n,cx;
  UInt uiTrDepth = uiDepth - pcCU->getDepth(uiAbsPartIdx);
  UInt uiCBFY = pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, uiTrDepth);
  UInt uiCBFU = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth);
  UInt uiCBFV = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth);
  UInt uiQPartNumParent  = pcCU->getPic()->getNumPartInCU() >> ((uiDepth-1) << 1);
  UInt uiQPartNumCurr    = pcCU->getPic()->getNumPartInCU() >> ((uiDepth) << 1);

  const UInt uiSubdiv = pcCU->getTransformIdx( uiAbsPartIdx ) + pcCU->getDepth( uiAbsPartIdx ) > uiDepth;
  UInt uiFlagPattern = xGetFlagPattern( pcCU, uiAbsPartIdx, uiDepth );
  n = pcCU->isIntra( uiAbsPartIdx ) ? 0 : 1;

  Bool bMRG_2Nx2N_TrDepth_Is_Zero = false;
  if(  uiTrDepth==0 
    && pcCU->getPartitionSize( uiAbsPartIdx) == SIZE_2Nx2N 
    && pcCU->getMergeFlag( uiAbsPartIdx ) )
  {
    bMRG_2Nx2N_TrDepth_Is_Zero = true;
  }

  if(uiFlagPattern < 8)
  {
    UInt uiFullDepth = pcCU->getDepth(uiAbsPartIdx) + uiTrDepth;
    UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
    if( uiLog2TrSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
    {
      UInt uiCBFU_Parent = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth-1);
      UInt uiCBFV_Parent = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth-1);
      if(uiCBFU_Parent)
      {
        xWriteFlag(uiCBFU);
      }
      if(uiCBFV_Parent)
      {
        xWriteFlag(uiCBFV);
      }
    }

    if(uiFlagPattern & 0x01)
    {
      codeTransformSubdivFlag( uiSubdiv, 0);
    }
  }
  else
  {
    if(uiFlagPattern == 8)
    {
      if (uiAbsPartIdx % uiQPartNumParent ==0)
      {
        codeBlockCbf(pcCU, uiAbsPartIdx, TEXT_LUMA, uiTrDepth, uiQPartNumCurr);
      }
    } 
    else if(uiFlagPattern == 9)
    {
      bool bNeedToCode = true;
      if ( n==1 && (uiAbsPartIdx%uiQPartNumParent) / uiQPartNumCurr == 3 )  
      {
        UInt uiTempAbsPartIdx = uiAbsPartIdx/uiQPartNumParent*uiQPartNumParent;
        if ( pcCU->getCbf( uiTempAbsPartIdx + uiQPartNumCurr*0, TEXT_LUMA, uiTrDepth ) ||
          pcCU->getCbf( uiTempAbsPartIdx + uiQPartNumCurr*1, TEXT_LUMA, uiTrDepth ) ||
          pcCU->getCbf( uiTempAbsPartIdx + uiQPartNumCurr*2, TEXT_LUMA, uiTrDepth ) )
        {
          bNeedToCode = true;
        }
        else
        {
          bNeedToCode = false;
          xWriteFlag( uiSubdiv );
        }
      }
      if ( bNeedToCode )
      {
        cx = m_uiCBP_YS_TableE[n][(uiCBFY<<1)+uiSubdiv];
        xWriteUnaryMaxSymbol(cx,n?2:3);//intra 3; inter 2

        if ( m_bAdaptFlag )
        {
          adaptCodeword(cx, m_ucCBP_YS_TableCounter[n],  m_ucCBP_YS_TableCounterSum[n],  m_uiCBP_YS_TableD[n],  m_uiCBP_YS_TableE[n], 3);
        }
      }
    }
    else if( uiFlagPattern == 14)
    {
      UInt  uiIdx = uiTrDepth? (2 + n) : n;
      cx = m_uiCBP_YUV_TableE[uiIdx][(uiCBFV<<0) + (uiCBFU<<1) + (uiCBFY<<2)];

      if (bMRG_2Nx2N_TrDepth_Is_Zero)
      {
        UInt uiCxOri = cx;
        if ( cx > m_uiCBP_YUV_TableE[uiIdx][0] )
        {
          cx--;
        }

        xWriteUnaryMaxSymbol(cx,6);

        if ( m_bAdaptFlag )
        {
          adaptCodeword(uiCxOri,  m_ucCBP_YUV_TableCounter[uiIdx],  m_ucCBP_YUV_TableCounterSum[uiIdx],  m_uiCBP_YUV_TableD[uiIdx],  m_uiCBP_YUV_TableE[uiIdx], 4);
        }
      }
      else
      {   
        xWriteUnaryMaxSymbol(cx,7);

        if ( m_bAdaptFlag )
        {
          adaptCodeword(cx,  m_ucCBP_YUV_TableCounter[uiIdx],  m_ucCBP_YUV_TableCounterSum[uiIdx],  m_uiCBP_YUV_TableD[uiIdx],  m_uiCBP_YUV_TableE[uiIdx], 4);
        }
      }
    }
    else if ( uiFlagPattern == 11 || uiFlagPattern == 13 || uiFlagPattern == 15 )
    {
      cx = m_uiCBP_YCS_TableE[n][(uiCBFY<<2)+((uiCBFU||uiCBFV?1:0)<<1)+uiSubdiv];
      if (bMRG_2Nx2N_TrDepth_Is_Zero)
      {
        UInt uiCxOri = cx;
        if ( cx > m_uiCBP_YCS_TableE[n][0] )
          cx--;
        xWriteUnaryMaxSymbol(cx,5);

        if ( m_bAdaptFlag )
        {
          adaptCodeword(uiCxOri, m_ucCBP_YCS_TableCounter[n],  m_ucCBP_YCS_TableCounterSum[n],  m_uiCBP_YCS_TableD[n],  m_uiCBP_YCS_TableE[n], 4);
        }
      }
      else
      {      
        xWriteCode(g_auiCBP_YCS_Table[n][cx], g_auiCBP_YCS_TableLen[n][cx]);

        if ( m_bAdaptFlag )
        {
          adaptCodeword(cx, m_ucCBP_YCS_TableCounter[n],  m_ucCBP_YCS_TableCounterSum[n],  m_uiCBP_YCS_TableD[n],  m_uiCBP_YCS_TableE[n], 4);
        }
      }

      //U and V          
      if ( uiFlagPattern == 15)
      {
        UInt uiCBFUV = (uiCBFU<<1) + uiCBFV;
        if(uiCBFUV > 0)
        {
          xWriteUnaryMaxSymbol(n? (uiCBFUV - 1) : (3-uiCBFUV), 2); 
        }
      }
    }
    else if (uiFlagPattern == 10 || uiFlagPattern == 12)
    {
      xWriteUnaryMaxSymbol(g_auiCBP_YC_TableE[n][(uiCBFY<<1)+(uiCBFU||uiCBFV?1:0)],3);//intra 3; inter 2
    }
  }
  return;
}


/** Function for parsing cbf and split 
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \param uiDepth CU Depth
 * \returns flag pattern
 * This function gets flagpattern for cbf and split flag
 */
UInt TEncCavlc::xGetFlagPattern( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  const UInt uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()]+2 - uiDepth;
  UInt uiTrDepth =  uiDepth - pcCU->getDepth( uiAbsPartIdx );
  UInt patternYUV, patternDiv;
  UInt bY, bU, bV;

  UInt uiFullDepth = uiDepth;
  UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
  if(uiTrDepth == 0)
  {
    patternYUV = 7;
  }
  else if( uiLog2TrSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    bY = pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, uiTrDepth - 1)?1:0;
    bU = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth - 1)?1:0;
    bV = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth - 1)?1:0;
    patternYUV = (bY<<2) + (bU<<1) + bV;
  }
  else
  {
    bY = pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, uiTrDepth - 1)?1:0;
    patternYUV = bY<<2;
  }

  if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN && uiDepth == pcCU->getDepth(uiAbsPartIdx) )
  {
    patternDiv = 0;
  }
#if G519_TU_AMP_NSQT_HARMONIZATION
  else if( (pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && (pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER) && ( pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N ) && (uiDepth == pcCU->getDepth(uiAbsPartIdx)) )
#else
  else if( (pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && (pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER) && ( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2NxN || pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_Nx2N) && (uiDepth == pcCU->getDepth(uiAbsPartIdx)) )
#endif
  {
    patternDiv = 0; 
  }
  else if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    patternDiv = 0;
  }
  else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    patternDiv = 0;
  }
  else if( uiLog2TrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
  {
    patternDiv = 0;
  }
  else
  {
    patternDiv = 1;
  }
  return ((patternYUV<<1)+patternDiv);
}

Void TEncCavlc::codeCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  if (eType == TEXT_ALL)
  {
    Int n,x,cx;

    UInt uiCBFY = pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, 0);
    UInt uiCBFU = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, 0);
    UInt uiCBFV = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, 0);
    UInt uiCBP = (uiCBFV<<2) + (uiCBFU<<1) + (uiCBFY<<0);
    
    /* Start adaptation */
    n = pcCU->isIntra( uiAbsPartIdx ) ? 0 : 1;
    x = uiCBP;
    cx = m_uiCBP_YUV_TableE[n][x];
    UInt vlcn = 0;

    if ( m_bAdaptFlag )
    {                
      adaptCodeword(cx, m_ucCBP_YUV_TableCounter[n], m_ucCBP_YUV_TableCounterSum[n], m_uiCBP_YUV_TableD[n], m_uiCBP_YUV_TableE[n], 4);
    }
    xWriteVlc( vlcn, cx );
  }
}

Void TEncCavlc::codeBlockCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiQPartNum, Bool bRD )
{
  UInt uiCbf0 = pcCU->getCbf   ( uiAbsPartIdx, eType, uiTrDepth );
  UInt uiCbf1 = pcCU->getCbf   ( uiAbsPartIdx + uiQPartNum, eType, uiTrDepth );
  UInt uiCbf2 = pcCU->getCbf   ( uiAbsPartIdx + uiQPartNum*2, eType, uiTrDepth );
  UInt uiCbf3 = pcCU->getCbf   ( uiAbsPartIdx + uiQPartNum*3, eType, uiTrDepth );
  UInt uiCbf = (uiCbf0<<3) | (uiCbf1<<2) | (uiCbf2<<1) | uiCbf3;
  
  assert(uiTrDepth > 0);
  
  if(bRD && uiCbf==0)
  {
    xWriteCode(0, 4); 
    return;
  }
  
  assert(uiCbf > 0);
  
  uiCbf --;

  Int cx;
  UInt n = (pcCU->isIntra(uiAbsPartIdx) && eType == TEXT_LUMA)? 0:1;

  cx = m_uiCBP_4Y_TableE[n][uiCbf];
  UInt vlcn = (n==0)?g_auiCBP_4Y_VlcNum[m_uiCBP_4Y_VlcIdx]:11;


  if ( m_bAdaptFlag )
  {                
    adaptCodeword(cx, m_ucCBP_4Y_TableCounter[n],  m_ucCBP_4Y_TableCounterSum[n],  m_uiCBP_4Y_TableD[n],  m_uiCBP_4Y_TableE[n], 2);

    if(n==0)
      m_uiCBP_4Y_VlcIdx += cx == m_uiCBP_4Y_VlcIdx ? 0 : (cx < m_uiCBP_4Y_VlcIdx ? -1 : 1);
  }
  
  xWriteVlc( vlcn, cx );
  return;
}

Void TEncCavlc::codeCoeffNxN    ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
#if NSQT
  Bool bNonSqureFlag = ( uiWidth != uiHeight );
  UInt uiNonSqureScanTableIdx = 0;
  if( bNonSqureFlag )
  {
    UInt uiWidthBit  =  g_aucConvertToBit[ uiWidth ] + 2;
    UInt uiHeightBit =  g_aucConvertToBit[ uiHeight ] + 2;
    uiNonSqureScanTableIdx = ( uiWidth * uiHeight ) == 64 ? 2 * ( uiHeight > uiWidth ) : 2 * ( uiHeight > uiWidth ) + 1;
    uiWidth  = 1 << ( ( uiWidthBit + uiHeightBit) >> 1 );
    uiHeight = uiWidth;
  }    
#endif

  if ( uiWidth > m_pcSlice->getSPS()->getMaxTrSize() )
  {
    uiWidth  = m_pcSlice->getSPS()->getMaxTrSize();
    uiHeight = m_pcSlice->getSPS()->getMaxTrSize();
  }
  UInt uiSize   = uiWidth*uiHeight;
  
  // point to coefficient
  TCoeff* piCoeff = pcCoef;
  UInt uiNumSig = 0;
  UInt uiScanning;
  
  // compute number of significant coefficients
  uiNumSig = TEncEntropy::countNonZeroCoeffs(piCoeff, uiWidth);
  
  if ( uiNumSig == 0 )
  {
    return;
  }
  
  // initialize scan
  UInt maxBlSize = 32;
  UInt uiBlSize = min(maxBlSize,uiWidth);
  UInt uiNoCoeff = uiBlSize*uiBlSize;
  
  UInt uiBlkPos;
  UInt uiLog2BlkSize = g_aucConvertToBit[ pcCU->isIntra( uiAbsPartIdx ) ? uiWidth : uiBlSize] + 2;
  const UInt uiScanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, eTType==TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
  
  static TCoeff scoeff[1024];
  Int iBlockType;

  if( uiSize == 2*2 )
  {
    // hack: re-use 4x4 coding
    ::memset( scoeff, 0, 16*sizeof(TCoeff) );
    for (uiScanning=0; uiScanning<4; uiScanning++)
    {
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
      scoeff[15-uiScanning] = piCoeff[ uiBlkPos ];
    }
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
      iBlockType = eTType-2;
    else
      iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
    
    xCodeCoeff( scoeff, iBlockType, 4
              , pcCU->isIntra(uiAbsPartIdx)
              );
  }
  else if ( uiSize == 4*4 )
  {
    for (uiScanning=0; uiScanning<16; uiScanning++)
    {
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
      scoeff[15-uiScanning] = piCoeff[ uiBlkPos ];
    }
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
      iBlockType = eTType-2;
    else
      iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
    
    xCodeCoeff( scoeff, iBlockType, 4
              , pcCU->isIntra(uiAbsPartIdx)
              );
  }
  else if ( uiSize == 8*8 )
  {
#if NSQT
    if( bNonSqureFlag )  
    {
      for ( uiScanning = 0; uiScanning < 64; uiScanning++ )
      { 
        scoeff[63-uiScanning] = piCoeff[ g_auiNonSquareSigLastScan[ uiNonSqureScanTableIdx ][ uiScanning ] ];
      } 
    }
    else  
#endif
    for (uiScanning=0; uiScanning<64; uiScanning++)
    {
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
      scoeff[63-uiScanning] = piCoeff[ uiBlkPos ];
    }
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
      iBlockType = eTType-2;
    else
      iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
    
    xCodeCoeff( scoeff, iBlockType, 8
              , pcCU->isIntra(uiAbsPartIdx)
              );
  }
  else
  {
    if(!pcCU->isIntra( uiAbsPartIdx ))
    {
      UInt uiBlSizeInBit = g_aucConvertToBit[uiBlSize] + 2;
      UInt uiWidthInBit = g_aucConvertToBit[uiWidth] + 2;
#if NSQT
      if( bNonSqureFlag ) 
      {
        for (uiScanning=0; uiScanning< uiNoCoeff; uiScanning++)
        {
          scoeff[uiNoCoeff-uiScanning-1] = piCoeff[ g_auiNonSquareSigLastScan[ uiNonSqureScanTableIdx ][ uiScanning ] ];
        }    
      }
      else  
#endif 
      for (uiScanning=0; uiScanning<uiNoCoeff; uiScanning++)
      {
        uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
        uiBlkPos = ((uiBlkPos>>uiBlSizeInBit) <<uiWidthInBit) + (uiBlkPos&(uiBlSize-1));
        scoeff[uiNoCoeff-uiScanning-1] = piCoeff[ uiBlkPos ];
      }
    }    
    
    if(pcCU->isIntra( uiAbsPartIdx ))
    {
      for (uiScanning=0; uiScanning<uiNoCoeff; uiScanning++)
      {
        uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
        scoeff[uiNoCoeff-uiScanning-1] = piCoeff[ uiBlkPos ];
      }
    }

    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V) 
    {
      iBlockType = eTType-2;
    }
    else
    {
      iBlockType = 5 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
    }
    xCodeCoeff( scoeff, iBlockType, uiBlSize
              , pcCU->isIntra(uiAbsPartIdx)
              );

    //#endif
  }
  
#if NSQT
  if( bNonSqureFlag && !pcCU->isIntra( uiAbsPartIdx ) )
  {
    TCoeff orgCoeff[ 256 ];
    memcpy( &orgCoeff[ 0 ], pcCoef, uiNoCoeff * sizeof( TCoeff ) );
    for( UInt uiScanPos = 0; uiScanPos < uiNoCoeff; uiScanPos++ )
    {
      uiBlkPos = g_auiNonSquareSigLastScan[ uiNonSqureScanTableIdx ][uiScanPos];
      pcCoef[ g_auiFrameScanXY[ (int)g_aucConvertToBit[ uiWidth ] + 1 ][ uiScanPos ] ] = orgCoeff[uiBlkPos]; 
    }  
  }
#endif
}

Void TEncCavlc::codeAlfFlag( UInt uiCode )
{
  
  xWriteFlag( uiCode );
}

Void TEncCavlc::codeAlfCtrlFlag( UInt uiSymbol )
{
  xWriteFlag( uiSymbol );
}

Void TEncCavlc::codeAlfUvlc( UInt uiCode )
{
  xWriteUvlc( uiCode );
}

Void TEncCavlc::codeAlfSvlc( Int iCode )
{
  xWriteSvlc( iCode );
}

Void TEncCavlc::codeSaoFlag( UInt uiCode )
{
  xWriteFlag( uiCode );
}

Void TEncCavlc::codeSaoUvlc( UInt uiCode )
{
    xWriteUvlc( uiCode );
}

Void TEncCavlc::codeSaoSvlc( Int iCode )
{
    xWriteSvlc( iCode );
}

#if NSQT_DIAG_SCAN
Void TEncCavlc::estBit( estBitsSbacStruct* pcEstBitsCabac, Int width, Int height, TextType eTType )
#else
Void TEncCavlc::estBit( estBitsSbacStruct* pcEstBitsCabac, UInt uiCTXIdx, TextType eTType )
#endif
{
  // printf("error : no VLC mode support in this version\n");
  return;
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TEncCavlc::xWriteCode     ( UInt uiCode, UInt uiLength )
{
  assert ( uiLength > 0 );
  m_pcBitIf->write( uiCode, uiLength );
}

Void TEncCavlc::xWriteUvlc     ( UInt uiCode )
{
  UInt uiLength = 1;
  UInt uiTemp = ++uiCode;
  
  assert ( uiTemp );
  
  while( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }
  
  //m_pcBitIf->write( uiCode, uiLength );
  // Take care of cases where uiLength > 32
  m_pcBitIf->write( 0, uiLength >> 1);
  m_pcBitIf->write( uiCode, (uiLength+1) >> 1);
}

Void TEncCavlc::xWriteSvlc     ( Int iCode )
{
  UInt uiCode;
  
  uiCode = xConvertToUInt( iCode );
  xWriteUvlc( uiCode );
}

Void TEncCavlc::xWriteFlag( UInt uiCode )
{
  m_pcBitIf->write( uiCode, 1 );
}

/** Write PCM alignment bits. 
 * \returns Void
 */
Void  TEncCavlc::xWritePCMAlignZero    ()
{
  m_pcBitIf->writeAlignZero();
}

Void TEncCavlc::xWriteUnaryMaxSymbol( UInt uiSymbol, UInt uiMaxSymbol )
{
  if (uiMaxSymbol == 0)
  {
    return;
  }
  xWriteFlag( uiSymbol ? 1 : 0 );
  if ( uiSymbol == 0 )
  {
    return;
  }
  
  Bool bCodeLast = ( uiMaxSymbol > uiSymbol );
  
  while( --uiSymbol )
  {
    xWriteFlag( 1 );
  }
  if( bCodeLast )
  {
    xWriteFlag( 0 );
  }
  return;
}

Void TEncCavlc::xWriteExGolombLevel( UInt uiSymbol )
{
  if( uiSymbol )
  {
    xWriteFlag( 1 );
    UInt uiCount = 0;
    Bool bNoExGo = (uiSymbol < 13);
    
    while( --uiSymbol && ++uiCount < 13 )
    {
      xWriteFlag( 1 );
    }
    if( bNoExGo )
    {
      xWriteFlag( 0 );
    }
    else
    {
      xWriteEpExGolomb( uiSymbol, 0 );
    }
  }
  else
  {
    xWriteFlag( 0 );
  }
  return;
}

Void TEncCavlc::xWriteEpExGolomb( UInt uiSymbol, UInt uiCount )
{
  while( uiSymbol >= (UInt)(1<<uiCount) )
  {
    xWriteFlag( 1 );
    uiSymbol -= 1<<uiCount;
    uiCount  ++;
  }
  xWriteFlag( 0 );
  while( uiCount-- )
  {
    xWriteFlag( (uiSymbol>>uiCount) & 1 );
  }
  return;
}

Void TEncCavlc::xWriteVlc(UInt uiTableNumber, UInt uiCodeNumber)
{
  assert( uiTableNumber<=13 );
  
  UInt uiTemp;
  UInt uiLength = 0;
  UInt uiCode = 0;
  
  if ( uiTableNumber < 5 )
  {
    if ((Int)uiCodeNumber < (6 * (1 << uiTableNumber)))
    {
      uiTemp = 1<<uiTableNumber;
      uiCode = uiTemp+uiCodeNumber%uiTemp;
      uiLength = 1+uiTableNumber+(uiCodeNumber>>uiTableNumber);
    }
    else
    {
      uiCode = uiCodeNumber - (6 * (1 << uiTableNumber)) + (1 << uiTableNumber);
      uiLength = (6-uiTableNumber)+1+2*xLeadingZeros(uiCode);
    }
  }
  else if (uiTableNumber < 8)
  {
    uiTemp = 1<<(uiTableNumber-4);
    uiCode = uiTemp+uiCodeNumber%uiTemp;
    uiLength = 1+(uiTableNumber-4)+(uiCodeNumber>>(uiTableNumber-4));
    if (uiLength>32)
    {
      if (uiLength>64)
      { 
        xWriteCode(0, 32);
        uiLength -=32;
      }
      xWriteCode(0, uiLength-32);
      uiLength  = 32;
    }
  }
  else if (uiTableNumber == 8)
  {
    assert( uiCodeNumber<=2 );
    if (uiCodeNumber == 0)
    {
      uiCode = 1;
      uiLength = 1;
    }
    else if (uiCodeNumber == 1)
    {
      uiCode = 1;
      uiLength = 2;
    }
    else if (uiCodeNumber == 2)
    {
      uiCode = 0;
      uiLength = 2;
    }
  }
  else if (uiTableNumber == 9)
  {
    if (uiCodeNumber == 0)
    {
      uiCode = 4;
      uiLength = 3;
    }
    else if (uiCodeNumber == 1)
    {
      uiCode = 10;
      uiLength = 4;
    }
    else if (uiCodeNumber == 2)
    {
      uiCode = 11;
      uiLength = 4;
    }
    else if (uiCodeNumber < 11)
    {
      uiCode = uiCodeNumber+21;
      uiLength = 5;
    }
    else
    {
      uiTemp = 1<<4;
      uiCode = uiTemp+(uiCodeNumber+5)%uiTemp;
      uiLength = 5+((uiCodeNumber+5)>>4);
      if (uiLength>32)
      {
        xWriteCode(0, uiLength-32);
        uiLength  = 32;
      }
    }
  }
  else if (uiTableNumber == 10)
  {
    uiCode = uiCodeNumber+1;
    uiLength = 1+2*xLeadingZeros(uiCode);
  }
  else if (uiTableNumber == 11)
  {
    if (uiCodeNumber == 0)
    {
      uiCode = 0;
      uiLength = 3;
    }
    else
    {
      uiCode = uiCodeNumber + 1;
      uiLength = 4;
    }
  }
  else if (uiTableNumber == 12)
  {
    uiCode = 64+(uiCodeNumber&0x3f);
    uiLength = 7+(uiCodeNumber>>6);
    if (uiLength>32)
    {
      xWriteCode(0, uiLength-32);
      uiLength  = 32;
    }
  }
  else if (uiTableNumber == 13)
  {
    uiTemp = 1<<4;
    uiCode = uiTemp+(uiCodeNumber&0x0f);
    uiLength = 5+(uiCodeNumber>>4);
  }

  xWriteCode(uiCode, uiLength);
}

/** Function for encoding a block of transform coefficients in CAVLC.
 * \param scoeff    pointer to transform coefficient buffer
 * \param blockType block type information, e.g. luma, chroma, intra, inter, etc. 
 * \param blSize block size
 */
Void TEncCavlc::xCodeCoeff( TCoeff* scoeff, Int blockType, Int blSize
                          , Int isIntra
                          )
{
  static const int switch_thr[10] = {49,49,0,49,49,0,49,49,49,49};
  static const int aiTableTr1[2][5] = {{0, 1, 1, 1, 0},{0, 1, 2, 3, 4}};
  int i, noCoeff = blSize*blSize;
  unsigned int cn;
  int level,vlc,sign,done,last_pos,start;
  int run_done,maxrun,run,lev;
  int tmprun,vlc_adaptive=0;
  int sum_big_coef = 0;
  Int tr1;

  Int scale = (isIntra && blockType < 2) ? 0 : 3;

  /* Do the last coefficient first */
  i = 0;
  done = 0;
  while (!done && i < noCoeff)
  {
    if (scoeff[i])
    {
      done = 1;
    }
    else
    {
      i++;
    }
  }
  if (i == noCoeff)
  {
    return;
  }

  last_pos = noCoeff-i-1;
  level = abs(scoeff[i]);
  lev = (level == 1) ? 0 : 1;

  if(blSize >= 8)
  {
    cn = xLastLevelInd(lev, last_pos, blSize);
    // ADAPT_VLC_NUM
    vlc = g_auiLastPosVlcNum[blockType][min(16u,m_uiLastPosVlcIndex[blockType])];
    xWriteVlc( vlc, cn );

    if ( m_bAdaptFlag )
    {
      // ADAPT_VLC_NUM
      cn = (blSize==8 || blockType<2)? cn:(cn>>2);
      m_uiLastPosVlcIndex[blockType] += cn == m_uiLastPosVlcIndex[blockType] ? 0 : (cn < m_uiLastPosVlcIndex[blockType] ? -1 : 1);
    }
  }
  else
  {
    int x,y,cx,cy;
    int nTab = max(0,blockType-2);
    
    x = (lev<<4) + last_pos;
    cx = m_uiLPTableE4[nTab][x];
    xWriteVlc( 2, cx );
    
    if ( m_bAdaptFlag )
    {
      cy = max( 0, cx-1 );
      y = m_uiLPTableD4[nTab][cy];
      m_uiLPTableD4[nTab][cy] = x;
      m_uiLPTableD4[nTab][cx] = y;
      m_uiLPTableE4[nTab][x] = cy;
      m_uiLPTableE4[nTab][y] = cx;
    }
  }

  sign = (scoeff[i++] < 0) ? 1 : 0;
  if (level > 1)
  {
    xWriteVlc( 0, ((level-2)<<1)+sign );
    tr1=0;
  }
  else
  {
    xWriteFlag( sign );
    tr1=1;
  }

  if (i < noCoeff)
  {
    /* Go into run mode */
    run_done = 0;
    UInt const *vlcTable;
    if(blockType==2||blockType==5)
    {
      vlcTable= g_auiVlcTable8x8Intra;
    }
    else
    {
      vlcTable= blSize<=8? g_auiVlcTable8x8Inter: g_auiVlcTable16x16Inter;
    }
    const UInt **pLumaRunTr1 =(blSize==4)? g_pLumaRunTr14x4:((blSize==8)? g_pLumaRunTr18x8: g_pLumaRunTr116x16);



    while ( !run_done )
    {
      maxrun = noCoeff-i-1;
      tmprun = min(maxrun, 28);
      if (tmprun < 28 || blSize<=8 || (blockType!=2&&blockType!=5))
      {
        vlc = vlcTable[tmprun];
      }
      else
      {
        vlc = 2;
      }
      run = 0;
      done = 0;
      while (!done)
      {
        if (!scoeff[i])
        {
          run++;
        }
        else
        {
          level = abs(scoeff[i]);
          lev = (level == 1) ? 0 : 1;

          if(blockType == 2 || blockType == 5)
          {
            cn = xRunLevelInd(lev, run, maxrun, pLumaRunTr1[aiTableTr1[(blSize&4)>>2][tr1]][tmprun]);
          }
          else
          {
            cn = xRunLevelIndInter(lev, run, maxrun, scale);
          }

          xWriteVlc( vlc, cn );

          if (tr1==0 || level>=2)
          {
            tr1=0;
          }
          else if (tr1 < MAX_TR1)
          {
            tr1++;
          }

          sign = (scoeff[i] < 0) ? 1 : 0;
          if (level > 1)
          {
            xWriteVlc( 0, ((level-2)<<1)+sign );

            sum_big_coef += level;
            if (blSize == 4 || i > switch_thr[blockType] || sum_big_coef > 2)
            {
            if (level > atable[vlc_adaptive])
            {
               vlc_adaptive++;
            }
              run_done = 1;
            }
          }
          else
          {
            xWriteFlag( sign );
          }
          run = 0;
          done = 1;
        }
        if (i == (noCoeff-1))
        {
          done = 1;
          run_done = 1;
          if (run)
          {
            if(blockType == 2 || blockType == 5)
            {
              cn = xRunLevelInd(0, run, maxrun, pLumaRunTr1[aiTableTr1[(blSize&4)>>2][tr1]][tmprun]);
            }
            else
            {
              cn = xRunLevelIndInter(0, run, maxrun, scale);
            }

            xWriteVlc( vlc, cn );
          }
        }
        i++;
      }
    }
  }

  /* Code the rest in level mode */
  start = i;
  for ( i=start; i<noCoeff; i++ )
  {
    int tmp = abs(scoeff[i]);

    xWriteVlc( vlc_adaptive, tmp );
    if (scoeff[i])
    {
      xWriteFlag( (scoeff[i] < 0) ? 1 : 0 );
      if (tmp > atable[vlc_adaptive])
      {
        vlc_adaptive++;
      }
    }
  }

  return;
}

/** code explicit wp tables
 * \param TComSlice* pcSlice
 * \returns Void
 */
Void TEncCavlc::xCodePredWeightTable( TComSlice* pcSlice )
{
  wpScalingParam  *wp;
  Bool            bChroma     = true; // color always present in HEVC ?
  Int             iNbRef       = (pcSlice->getSliceType() == B_SLICE ) ? (2) : (1);
  Bool            bDenomCoded  = false;

#if WP_IMPROVED_SYNTAX
  UInt            uiMode = 0;
  if ( (pcSlice->getSliceType()==P_SLICE && pcSlice->getPPS()->getUseWP()) || (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPredIdc()==1 && pcSlice->getRefPicListCombinationFlag()==0 ) )
    uiMode = 1; // explicit
  else if ( pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPredIdc()==2 )
    uiMode = 2; // implicit (does not use this mode in this syntax)
  if (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPredIdc()==1 && pcSlice->getRefPicListCombinationFlag())
    uiMode = 3; // combined explicit
#endif
#if WP_IMPROVED_SYNTAX
  if(uiMode == 1)
  {
#endif
    for ( Int iNumRef=0 ; iNumRef<iNbRef ; iNumRef++ ) 
    {
      RefPicList  eRefPicList = ( iNumRef ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
        if ( !bDenomCoded ) 
        {
#if WP_IMPROVED_SYNTAX
          Int iDeltaDenom;
          WRITE_UVLC( wp[0].uiLog2WeightDenom, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom

          if( bChroma )
          {
            iDeltaDenom = (wp[1].uiLog2WeightDenom - wp[0].uiLog2WeightDenom);
            WRITE_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );       // se(v): delta_chroma_log2_weight_denom
          }
#else
          WRITE_UVLC( wp[0].uiLog2WeightDenom, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom

          if( bChroma )
          {
            WRITE_UVLC( wp[1].uiLog2WeightDenom, "chroma_log2_weight_denom" ); // ue(v): chroma_log2_weight_denom
          }
#endif
          bDenomCoded = true;
        }

        WRITE_FLAG( wp[0].bPresentFlag, "luma_weight_lX_flag" );               // u(1): luma_weight_lX_flag

        if ( wp[0].bPresentFlag ) 
        {
#if WP_IMPROVED_SYNTAX
          Int iDeltaWeight = (wp[0].iWeight - (1<<wp[0].uiLog2WeightDenom));
          WRITE_SVLC( iDeltaWeight, "delta_luma_weight_lX" );                  // se(v): delta_luma_weight_lX
#else
          WRITE_SVLC( wp[0].iWeight, "luma_weight_lX" );                       // se(v): luma_weight_lX
#endif
          WRITE_SVLC( wp[0].iOffset, "luma_offset_lX" );                       // se(v): luma_offset_lX
        }

        if ( bChroma ) 
        {
          WRITE_FLAG( wp[1].bPresentFlag, "chroma_weight_lX_flag" );           // u(1): chroma_weight_lX_flag

          if ( wp[1].bPresentFlag )
          {
            for ( Int j=1 ; j<3 ; j++ ) 
            {
#if WP_IMPROVED_SYNTAX
              Int iDeltaWeight = (wp[j].iWeight - (1<<wp[1].uiLog2WeightDenom));
              WRITE_SVLC( iDeltaWeight, "delta_chroma_weight_lX" );            // se(v): delta_chroma_weight_lX

              Int iDeltaChroma = (wp[j].iOffset + ( ( (g_uiIBDI_MAX>>1)*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) - (g_uiIBDI_MAX>>1));
              WRITE_SVLC( iDeltaChroma, "delta_chroma_offset_lX" );            // se(v): delta_chroma_offset_lX
#else
              WRITE_SVLC( wp[j].iWeight, "chroma_weight_lX" );                 // se(v): chroma_weight_lX
              WRITE_SVLC( wp[j].iOffset, "chroma_offset_lX" );                 // se(v): chroma_offset_lX
#endif
            }
          }
        }
      }
    }
#if WP_IMPROVED_SYNTAX
  }
  else if (uiMode == 3)
  {
    for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(REF_PIC_LIST_C) ; iRefIdx++ ) 
    {
      RefPicList  eRefPicList = (RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIdx);
      Int iCombRefIdx = pcSlice->getRefIdxFromIdxOfLC(iRefIdx);

      pcSlice->getWpScaling(eRefPicList, iCombRefIdx, wp);
      if ( !bDenomCoded ) 
      {
        Int iDeltaDenom;
        WRITE_UVLC( wp[0].uiLog2WeightDenom, "luma_log2_weight_denom" );       // ue(v): luma_log2_weight_denom

        if( bChroma )
        {
          iDeltaDenom = (wp[1].uiLog2WeightDenom - wp[0].uiLog2WeightDenom);
          WRITE_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );         // se(v): delta_chroma_log2_weight_denom
        }
        bDenomCoded = true;
      }

      WRITE_FLAG( wp[0].bPresentFlag, "luma_weight_lc_flag" );                 // u(1): luma_weight_lc_flag

      if ( wp[0].bPresentFlag ) 
      {
        Int iDeltaWeight = (wp[0].iWeight - (1<<wp[0].uiLog2WeightDenom));
        WRITE_SVLC( iDeltaWeight, "delta_luma_weight_lc" );                    // se(v): delta_luma_weight_lc
        WRITE_SVLC( wp[0].iOffset, "luma_offset_lc" );                         // se(v): luma_offset_lc
      }
      if ( bChroma ) 
      {
        WRITE_FLAG( wp[1].bPresentFlag, "chroma_weight_lc_flag" );             // u(1): luma_weight_lc_flag

        if ( wp[1].bPresentFlag )
        {
          for ( Int j=1 ; j<3 ; j++ ) 
          {
            Int iDeltaWeight = (wp[j].iWeight - (1<<wp[1].uiLog2WeightDenom));
            WRITE_SVLC( iDeltaWeight, "delta_chroma_weight_lc" );              // se(v): delta_chroma_weight_lc

            Int iDeltaChroma = (wp[j].iOffset + ( ( (g_uiIBDI_MAX>>1)*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) - (g_uiIBDI_MAX>>1));
            WRITE_SVLC( iDeltaChroma, "delta_chroma_offset_lc" );              // se(v): delta_chroma_offset_lc
          }
        }
      }
    }
  }
#endif
}

#if SCALING_LIST
/** code quantization matrix
 *  \param scalingList quantization matrix information
 */
Void TEncCavlc::codeScalingList( TComScalingList* scalingList )
{
  UInt listId,sizeId;
  Int *dst=0;
#if SCALING_LIST_OUTPUT_RESULT
  Int *org=0;
  Int avg_error = 0;
  Int max_error = 0;
  Int startBit;
  Int startTotalBit;
  startBit = m_pcBitIf->getNumberOfWrittenBits();
  startTotalBit = m_pcBitIf->getNumberOfWrittenBits();
#endif

  WRITE_FLAG( scalingList->getUseDefaultOnlyFlag (), "use_default_scaling_list_flag" );

  if(scalingList->getUseDefaultOnlyFlag () == false)
  {
#if SCALING_LIST_OUTPUT_RESULT
    printf("Header Bit %d\n",m_pcBitIf->getNumberOfWrittenBits()-startBit);
#endif
    //for each size
    for(sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
      for(listId = 0; listId < g_auiScalingListNum[sizeId]; listId++)
      {
        dst = scalingList->getScalingListAddress(sizeId,listId);
#if SCALING_LIST_OUTPUT_RESULT
        org = scalingList->getScalingListOrgAddress(sizeId,listId);
        for(int i=0;i<g_scalingListSize[sizeId];i++)
        {
          avg_error += abs(dst[i] - org[i]);
          if(abs(max_error) < abs(dst[i] - org[i]))
          {
            max_error = dst[i] - org[i];
          }
        }
        startBit = m_pcBitIf->getNumberOfWrittenBits();
#endif
        WRITE_FLAG( scalingList->getPredMode (sizeId,listId), "pred_mode_flag" );
        if(scalingList->getPredMode (sizeId,listId) == SCALING_LIST_PRED_COPY)//Copy Mode
        {
          WRITE_UVLC( (Int)listId - (Int)scalingList->getPredMatrixId (sizeId,listId) - 1, "pred_matrix_id_delta");
          scalingList->xPredScalingListMatrix( scalingList, dst, sizeId, listId,sizeId, scalingList->getPredMatrixId (sizeId,listId));       
        }
        else if(scalingList->getPredMode (sizeId,listId) == SCALING_LIST_PRED_DPCM)//DPCM Mode
        {
          xCodeDPCMScalingListMatrix(scalingList, dst,sizeId);
        }
#if SCALING_LIST_OUTPUT_RESULT
        printf("Matrix [%d][%d] Bit %d\n",sizeId,listId,m_pcBitIf->getNumberOfWrittenBits() - startBit);
#endif
      }
    }
  }
#if SCALING_LIST_OUTPUT_RESULT
  else
  {
    printf("Header Bit %d\n",m_pcBitIf->getNumberOfWrittenBits()-startTotalBit);
  }
  printf("Total Bit %d\n",m_pcBitIf->getNumberOfWrittenBits()-startTotalBit);
  printf("MaxError %d\n",abs(max_error));
  printf("AvgError %lf\n",(double)avg_error/(double)((16+64+256)*6+(1024*2)));
#endif
  return;
}
/** code DPCM with quantization matrix
 * \param scalingList quantization matrix information
 * \param piData matrix data
 * \param uiSizeId size index
 */
Void TEncCavlc::xCodeDPCMScalingListMatrix(TComScalingList* scalingList, Int* piData, UInt uiSizeId)
{
  Int dpcm[1024];
  UInt uiDataCounter = g_scalingListSize[uiSizeId];

  //make DPCM
  scalingList->xMakeDPCM(piData, piData, dpcm, uiSizeId);
  xWriteResidualCode(uiDataCounter,dpcm);
}
/** write resiidual code
 * \param uiSize side index
 * \param data residual coefficient
 */
Void TEncCavlc::xWriteResidualCode(UInt uiSize, Int *data)
{
  for(UInt i=0;i<uiSize;i++)
  {
    WRITE_SVLC( data[i],  "delta_coef");
  }
}

#endif

//! \}
