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

/** \file     TDecCAVLC.cpp
    \brief    CAVLC decoder class
*/

#include "TDecCAVLC.h"
#include "SEIread.h"

//! \ingroup TLibDecoder
//! \{

#if ENC_DEC_TRACE

#define READ_CODE(length, code, name)     xReadCodeTr ( length, code, name )
#define READ_UVLC(        code, name)     xReadUvlcTr (         code, name )
#define READ_SVLC(        code, name)     xReadSvlcTr (         code, name )
#define READ_FLAG(        code, name)     xReadFlagTr (         code, name )

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


Void  TDecCavlc::xReadCodeTr           (UInt length, UInt& rValue, const Char *pSymbolName)
{
  xReadCode (length, rValue);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s u(%d) : %d\n", pSymbolName, length, rValue ); 
  fflush ( g_hTrace );
}

Void  TDecCavlc::xReadUvlcTr           (UInt& rValue, const Char *pSymbolName)
{
  xReadUvlc (rValue);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s u(v) : %d\n", pSymbolName, rValue ); 
  fflush ( g_hTrace );
}

Void  TDecCavlc::xReadSvlcTr           (Int& rValue, const Char *pSymbolName)
{
  xReadSvlc(rValue);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s s(v) : %d\n", pSymbolName, rValue ); 
  fflush ( g_hTrace );
}

Void  TDecCavlc::xReadFlagTr           (UInt& rValue, const Char *pSymbolName)
{
  xReadFlag(rValue);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s u(1) : %d\n", pSymbolName, rValue ); 
  fflush ( g_hTrace );
}

#else

#define READ_CODE(length, code, name)     xReadCode ( length, code )
#define READ_UVLC(        code, name)     xReadUvlc (         code )
#define READ_SVLC(        code, name)     xReadSvlc (         code )
#define READ_FLAG(        code, name)     xReadFlag (         code )

#endif



// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCavlc::TDecCavlc()
{
  m_bAlfCtrl = false;
  m_uiMaxAlfCtrlDepth = 0;
  m_iSliceGranularity = 0;
}

TDecCavlc::~TDecCavlc()
{
  
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 * unmarshal a sequence of SEI messages from bitstream.
 */
void TDecCavlc::parseSEI(SEImessages& seis)
{
  assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  do
  {
    parseSEImessage(*m_pcBitstream, seis);
    /* SEI messages are an integer number of bytes, something has failed
     * in the parsing if bitstream not byte-aligned */
    assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  } while (0x80 != m_pcBitstream->peekBits(8));
  assert(m_pcBitstream->getNumBitsLeft() == 8); /* rsbp_trailing_bits */
}
#if INTER_RPS_PREDICTION
void TDecCavlc::parseShortTermRefPicSet( TComPPS* pcPPS, TComReferencePictureSet* pcRPS, Int idx )
#else
void TDecCavlc::parseShortTermRefPicSet( TComPPS* pcPPS, TComReferencePictureSet* pcRPS )
#endif
{
  UInt uiCode;
#if INTER_RPS_PREDICTION
  UInt uiInterRPSPred;
  READ_FLAG(uiInterRPSPred, "inter_RPS_flag");  pcRPS->setInterRPSPrediction(uiInterRPSPred);
  if (uiInterRPSPred) 
  {
    UInt uiBit;
    READ_UVLC(uiCode, "delta_idx_minus1" ); // delta index of the Reference Picture Set used for prediction minus 1
    Int rIdx =  idx - 1 - uiCode;
    assert (rIdx <= idx && rIdx >= 0);
    TComReferencePictureSet*   pcRPSRef = pcPPS->getRPSList()->getReferencePictureSet(rIdx);
    Int k = 0, k0 = 0, k1 = 0;
    READ_CODE(1, uiBit, "delta_rps_sign"); // delta_RPS_sign
    READ_UVLC(uiCode, "abs_delta_rps_minus1");  // absolute delta RPS minus 1
    Int deltaRPS = (1 - (uiBit<<1)) * (uiCode + 1); // delta_RPS
    for(Int j=0 ; j <= pcRPSRef->getNumberOfPictures(); j++)
    {
      READ_CODE(1, uiBit, "ref_idc0" ); //first bit is "1" if Idc is 1 
      Int refIdc = uiBit;
      if (refIdc == 0) 
      {
        READ_CODE(1, uiBit, "ref_idc1" ); //second bit is "1" if Idc is 2, "0" otherwise.
        refIdc = uiBit<<1; //second bit is "1" if refIdc is 2, "0" if refIdc = 0.
      }
      if (refIdc == 1 || refIdc == 2)
      {
        Int deltaPOC = deltaRPS + ((j < pcRPSRef->getNumberOfPictures())? pcRPSRef->getDeltaPOC(j) : 0);
        pcRPS->setDeltaPOC(k, deltaPOC);
        pcRPS->setUsed(k, (refIdc == 1));

        if (deltaPOC < 0) {
          k0++;
        }
        else 
        {
          k1++;
        }
        k++;
      }  
      pcRPS->setRefIdc(j,refIdc);  
    }
    pcRPS->setNumRefIdc(pcRPSRef->getNumberOfPictures()+1);  
    pcRPS->setNumberOfPictures(k);
    pcRPS->setNumberOfNegativePictures(k0);
    pcRPS->setNumberOfPositivePictures(k1);
    pcRPS->sortDeltaPOC();
  }
  else
  {
#else
    pcRPS->create(pcPPS->getSPS()->getMaxNumberOfReferencePictures());
#endif //INTER_RPS_PREDICTION
    READ_UVLC(uiCode, "num_negative_pics");           pcRPS->setNumberOfNegativePictures(uiCode);
    READ_UVLC(uiCode, "num_positive_pics");           pcRPS->setNumberOfPositivePictures(uiCode);
    Int prev = 0;
    Int poc;
    for(Int j=0 ; j < pcRPS->getNumberOfNegativePictures(); j++)
    {
      READ_UVLC(uiCode, "delta_poc_s0_minus1");
      poc = prev-uiCode-1;
      prev = poc;
      pcRPS->setDeltaPOC(j,poc);
      READ_FLAG(uiCode, "used_by_curr_pic_s0_flag");  pcRPS->setUsed(j,uiCode);
    }
    prev = 0;
    for(Int j=pcRPS->getNumberOfNegativePictures(); j < pcRPS->getNumberOfNegativePictures()+pcRPS->getNumberOfPositivePictures(); j++)
    {
      READ_UVLC(uiCode, "delta_poc_s1_minus1");
      poc = prev+uiCode+1;
      prev = poc;
      pcRPS->setDeltaPOC(j,poc);
      READ_FLAG(uiCode, "used_by_curr_pic_s1_flag");  pcRPS->setUsed(j,uiCode);
    }
    pcRPS->setNumberOfPictures(pcRPS->getNumberOfNegativePictures()+pcRPS->getNumberOfPositivePictures());
#if INTER_RPS_PREDICTION
  }
#endif // INTER_RPS_PREDICTION   
#if PRINT_RPS_INFO
  pcRPS->printDeltaPOC();
#endif
}

Void TDecCavlc::parseAPSInitInfo(TComAPS& cAPS)
{
#if ENC_DEC_TRACE  
  xTraceAPSHeader(&cAPS);
#endif

  UInt uiCode;
  //aps ID
  READ_UVLC(uiCode, "aps_id");  cAPS.setAPSID(uiCode);
#if SCALING_LIST
  READ_FLAG(uiCode,"aps_scaling_list_data_present_flag");      cAPS.setScalingListEnabled( (uiCode==1)?true:false );
#endif
  //DF flag
  READ_FLAG(uiCode, "aps_deblocking_filter_flag"); cAPS.setLoopFilterOffsetInAPS( (uiCode==1)?true:false );
  //SAO flag
  READ_FLAG(uiCode, "aps_sample_adaptive_offset_flag");  cAPS.setSaoEnabled( (uiCode==1)?true:false );
  //ALF flag
  READ_FLAG(uiCode, "aps_adaptive_loop_filter_flag");  cAPS.setAlfEnabled( (uiCode==1)?true:false );

#if !G220_PURE_VLC_SAO_ALF
  if(cAPS.getSaoEnabled() || cAPS.getAlfEnabled())
  {
    //CABAC usage flag
    READ_FLAG(uiCode, "aps_cabac_use_flag");  cAPS.setCABACForAPS( (uiCode==1)?true:false );
    if(cAPS.getCABACForAPS())
    {
      Int iCode;
      //CABAC init IDC
      READ_UVLC(uiCode, "aps_cabac_init_idc");  cAPS.setCABACinitIDC( uiCode );
      //CABAC init QP
      READ_SVLC(iCode, "aps_cabac_init_qp_minus26");  cAPS.setCABACinitQP( iCode + 26);
    }
  }
#endif
}

Void TDecCavlc::parsePPS(TComPPS* pcPPS)
{
#if ENC_DEC_TRACE  
  xTracePPSHeader (pcPPS);
#endif
  UInt  uiCode;

#if G509_CHROMA_QP_OFFSET
  Int   iCode;
#endif

  TComRPS* pcRPSList = pcPPS->getRPSList();
  READ_UVLC( uiCode, "pic_parameter_set_id");                      pcPPS->setPPSId (uiCode);
  READ_UVLC( uiCode, "seq_parameter_set_id");                      pcPPS->setSPSId (uiCode);
  // RPS is put before entropy_coding_mode_flag
  // since entropy_coding_mode_flag will probably be removed from the WD
  TComReferencePictureSet*      pcRPS;

  READ_UVLC( uiCode, "num_short_term_ref_pic_sets" );
  pcRPSList->create(uiCode);

  for(UInt i=0; i< pcRPSList->getNumberOfReferencePictureSets(); i++)
  {
    pcRPS = pcRPSList->getReferencePictureSet(i);
#if INTER_RPS_PREDICTION
    parseShortTermRefPicSet(pcPPS,pcRPS,i);
#else
    parseShortTermRefPicSet(pcPPS,pcRPS);
#endif
  }
  READ_FLAG( uiCode, "long_term_ref_pics_present_flag" );          pcPPS->setLongTermRefsPresent(uiCode);
  // entropy_coding_mode_flag
  // We code the entropy_coding_mode_flag, it's needed for tests.
  READ_FLAG( uiCode, "entropy_coding_mode_flag" );                 pcPPS->setEntropyCodingMode( uiCode ? true : false );
  if (pcPPS->getEntropyCodingMode())
  {
    READ_UVLC( uiCode, "entropy_coding_synchro" );                 pcPPS->setEntropyCodingSynchro( uiCode );
    READ_FLAG( uiCode, "cabac_istate_reset" );                     pcPPS->setCabacIstateReset( uiCode ? true : false );
    if ( pcPPS->getEntropyCodingSynchro() )
    {
      READ_UVLC( uiCode, "num_substreams_minus1" );                pcPPS->setNumSubstreams(uiCode+1);
    }
  }
  READ_UVLC( uiCode, "num_temporal_layer_switching_point_flags" ); pcPPS->setNumTLayerSwitchingFlags( uiCode );
  for ( UInt i = 0; i < pcPPS->getNumTLayerSwitchingFlags(); i++ )
  {
    READ_FLAG( uiCode, "temporal_layer_switching_point_flag" );    pcPPS->setTLayerSwitchingFlag( i, uiCode > 0 ? true : false );
  }
  
  // num_ref_idx_l0_default_active_minus1
  // num_ref_idx_l1_default_active_minus1
#if G507_QP_ISSUE_FIX
  READ_SVLC(iCode, "pic_init_qp_minus26" );                        pcPPS->setPicInitQPMinus26(iCode);
#else
  // pic_init_qp_minus26  /* relative to 26 */
#endif
  READ_FLAG( uiCode, "constrained_intra_pred_flag" );              pcPPS->setConstrainedIntraPred( uiCode ? true : false );
  READ_FLAG( uiCode, "enable_temporal_mvp_flag" );                 pcPPS->setEnableTMVPFlag( uiCode ? true : false );
  READ_CODE( 2, uiCode, "slice_granularity" );                     pcPPS->setSliceGranularity(uiCode);

  // alf_param() ?

#if G507_QP_ISSUE_FIX
  READ_UVLC( uiCode, "max_cu_qp_delta_depth");
  if(uiCode == 0)
  {
    pcPPS->setUseDQP (false);
    pcPPS->setMaxCuDQPDepth( 0 );
  }
  else
  {
    pcPPS->setUseDQP (true);
    pcPPS->setMaxCuDQPDepth(uiCode - 1);
  }
  pcPPS->setMinCuDQPSize( pcPPS->getSPS()->getMaxCUWidth() >> ( pcPPS->getMaxCuDQPDepth()) );
#else
  if( pcPPS->getSPS()->getUseDQP() )
  {
    READ_UVLC( uiCode, "max_cu_qp_delta_depth");
    pcPPS->setMaxCuDQPDepth(uiCode);
    pcPPS->setMinCuDQPSize( pcPPS->getSPS()->getMaxCUWidth() >> ( pcPPS->getMaxCuDQPDepth()) );
  }
  else
  {
    pcPPS->setMaxCuDQPDepth( 0 );
    pcPPS->setMinCuDQPSize( pcPPS->getSPS()->getMaxCUWidth() >> ( pcPPS->getMaxCuDQPDepth()) );
  }
#endif

#if G509_CHROMA_QP_OFFSET
  READ_SVLC( iCode, "chroma_qp_offset");
  pcPPS->setChromaQpOffset(iCode);

  READ_SVLC( iCode, "chroma_qp_offset_2nd");
  pcPPS->setChromaQpOffset2nd(iCode);
#endif

  READ_FLAG( uiCode, "weighted_pred_flag" );          // Use of Weighting Prediction (P_SLICE)
  pcPPS->setUseWP( uiCode==1 );
  READ_CODE( 2, uiCode, "weighted_bipred_idc" );      // Use of Bi-Directional Weighting Prediction (B_SLICE)
  pcPPS->setWPBiPredIdc( uiCode );
  printf("TDecCavlc::parsePPS():\tm_bUseWeightPred=%d\tm_uiBiPredIdc=%d\n", pcPPS->getUseWP(), pcPPS->getWPBiPredIdc());

  READ_FLAG ( uiCode, "tile_info_present_flag" );
  pcPPS->setColumnRowInfoPresent(uiCode);
#if NONCROSS_TILE_IN_LOOP_FILTERING
  READ_FLAG ( uiCode, "tile_control_present_flag" );
  pcPPS->setTileBehaviorControlPresentFlag(uiCode);
#endif
  if( pcPPS->getColumnRowInfoPresent() == 1 )
  {
    READ_FLAG ( uiCode, "uniform_spacing_flag" );  
    pcPPS->setUniformSpacingIdr( uiCode );
#if !NONCROSS_TILE_IN_LOOP_FILTERING
    READ_FLAG ( uiCode, "tile_boundary_independence_flag" );  
    pcPPS->setTileBoundaryIndependenceIdr( uiCode );
#endif

    READ_UVLC ( uiCode, "num_tile_columns_minus1" );   
    pcPPS->setNumColumnsMinus1( uiCode );  
    READ_UVLC ( uiCode, "num_tile_rows_minus1" );  
    pcPPS->setNumRowsMinus1( uiCode );  

    if( pcPPS->getUniformSpacingIdr() == 0 )
    {
      UInt* columnWidth = (UInt*)malloc(pcPPS->getNumColumnsMinus1()*sizeof(UInt));
      for(UInt i=0; i<pcPPS->getNumColumnsMinus1(); i++)
      { 
        READ_UVLC( uiCode, "column_width" );  
        columnWidth[i] = uiCode;  
      }
      pcPPS->setColumnWidth(columnWidth);
      free(columnWidth);

      UInt* rowHeight = (UInt*)malloc(pcPPS->getNumRowsMinus1()*sizeof(UInt));
      for(UInt i=0; i<pcPPS->getNumRowsMinus1(); i++)
      {
        READ_UVLC( uiCode, "row_height" );  
        rowHeight[i] = uiCode;  
      }
      pcPPS->setRowHeight(rowHeight);
      free(rowHeight);  
    }
  }

#if NONCROSS_TILE_IN_LOOP_FILTERING
  if(pcPPS->getTileBehaviorControlPresentFlag() == 1)
  {
    Int iNumColTilesMinus1 = (pcPPS->getColumnRowInfoPresent() == 1)?(pcPPS->getNumColumnsMinus1()):(pcPPS->getSPS()->getNumColumnsMinus1());
    Int iNumRowTilesMinus1 = (pcPPS->getColumnRowInfoPresent() == 1)?(pcPPS->getNumColumnsMinus1()):(pcPPS->getSPS()->getNumRowsMinus1());

    pcPPS->setTileBoundaryIndependenceIdr( 1 ); //default
    pcPPS->setLFCrossTileBoundaryFlag(true); //default

    if(iNumColTilesMinus1 !=0 || iNumRowTilesMinus1 !=0)
    {
      READ_FLAG ( uiCode, "tile_boundary_independence_flag" );  
      pcPPS->setTileBoundaryIndependenceIdr( uiCode );

      if(pcPPS->getTileBoundaryIndependenceIdr() == 1)
      {
        READ_FLAG ( uiCode, "loop_filter_across_tile_flag" );  
        pcPPS->setLFCrossTileBoundaryFlag( (uiCode == 1)?true:false );
      }

    }
  }
#endif

  return;
}

Void TDecCavlc::parseSPS(TComSPS* pcSPS)
{
#if ENC_DEC_TRACE  
  xTraceSPSHeader (pcSPS);
#endif
  
  UInt  uiCode;

  READ_CODE( 8,  uiCode, "profile_idc" );                        pcSPS->setProfileIdc( uiCode );
  READ_CODE( 8,  uiCode, "reserved_zero_8bits" );
  READ_CODE( 8,  uiCode, "level_idc" );                          pcSPS->setLevelIdc( uiCode );
  READ_UVLC(     uiCode, "seq_parameter_set_id" );               pcSPS->setSPSId( uiCode );
  READ_UVLC(     uiCode, "chroma_format_idc" );                  pcSPS->setChromaFormatIdc( uiCode );
  READ_CODE( 3,  uiCode, "max_temporal_layers_minus1" );         pcSPS->setMaxTLayers( uiCode+1 );
#if PIC_SIZE_VLC
  READ_UVLC (    uiCode, "pic_width_in_luma_samples" );          pcSPS->setWidth       ( uiCode    );
  READ_UVLC (    uiCode, "pic_height_in_luma_samples" );         pcSPS->setHeight      ( uiCode    );
#else
  READ_CODE( 16, uiCode, "pic_width_in_luma_samples" );          pcSPS->setWidth       ( uiCode    );
  READ_CODE( 16, uiCode, "pic_height_in_luma_samples" );         pcSPS->setHeight      ( uiCode    );
#endif

#if FULL_NBIT
  READ_UVLC(     uiCode, "bit_depth_luma_minus8" );
  g_uiBitDepth = 8 + uiCode;
  g_uiBitIncrement = 0;
  pcSPS->setBitDepth(g_uiBitDepth);
  pcSPS->setBitIncrement(g_uiBitIncrement);
#else
  READ_UVLC(     uiCode, "bit_depth_luma_minus8" );
  g_uiBitDepth = 8;
  g_uiBitIncrement = uiCode;
  pcSPS->setBitDepth(g_uiBitDepth);
  pcSPS->setBitIncrement(g_uiBitIncrement);
#endif
  
  g_uiBASE_MAX  = ((1<<(g_uiBitDepth))-1);
  
#if IBDI_NOCLIP_RANGE
  g_uiIBDI_MAX  = g_uiBASE_MAX << g_uiBitIncrement;
#else
  g_uiIBDI_MAX  = ((1<<(g_uiBitDepth+g_uiBitIncrement))-1);
#endif
  READ_UVLC( uiCode,    "bit_depth_chroma_minus8" );

  READ_FLAG( uiCode, "pcm_enabled_flag" ); pcSPS->setUsePCM( uiCode ? true : false );

  if( pcSPS->getUsePCM() )
  {
    READ_CODE( 4, uiCode, "pcm_bit_depth_luma_minus1" );           pcSPS->setPCMBitDepthLuma   ( 1 + uiCode );
    READ_CODE( 4, uiCode, "pcm_bit_depth_chroma_minus1" );         pcSPS->setPCMBitDepthChroma ( 1 + uiCode );
  }
  READ_UVLC( uiCode,    "log2_max_pic_order_cnt_lsb_minus4" );   pcSPS->setBitsForPOC( 4 + uiCode );
  READ_UVLC( uiCode,    "max_num_ref_pics" );                    pcSPS->setMaxNumberOfReferencePictures(uiCode);
  READ_UVLC( uiCode,    "num_reorder_frames" );                  pcSPS->setNumReorderFrames(uiCode);
#if MAX_DPB_AND_LATENCY
  READ_UVLC ( uiCode, "max_dec_frame_buffering");
  pcSPS->setMaxDecFrameBuffering( uiCode );
  READ_UVLC ( uiCode, "max_latency_increase");
  pcSPS->setMaxLatencyIncrease( uiCode );
#endif
#if !G507_COND_4X4_ENABLE_FLAG
  xReadFlag( uiCode ); pcSPS->setDisInter4x4( uiCode ? true : false );
#endif
  READ_UVLC( uiCode, "log2_min_coding_block_size_minus3" );
  UInt log2MinCUSize = uiCode + 3;
  READ_UVLC( uiCode, "log2_diff_max_min_coding_block_size" );
  UInt uiMaxCUDepthCorrect = uiCode;
  pcSPS->setMaxCUWidth  ( 1<<(log2MinCUSize + uiMaxCUDepthCorrect) ); g_uiMaxCUWidth  = 1<<(log2MinCUSize + uiMaxCUDepthCorrect);
  pcSPS->setMaxCUHeight ( 1<<(log2MinCUSize + uiMaxCUDepthCorrect) ); g_uiMaxCUHeight = 1<<(log2MinCUSize + uiMaxCUDepthCorrect);
  READ_UVLC( uiCode, "log2_min_transform_block_size_minus2" );   pcSPS->setQuadtreeTULog2MinSize( uiCode + 2 );

  READ_UVLC( uiCode, "log2_diff_max_min_transform_block_size" ); pcSPS->setQuadtreeTULog2MaxSize( uiCode + pcSPS->getQuadtreeTULog2MinSize() );
  pcSPS->setMaxTrSize( 1<<(uiCode + pcSPS->getQuadtreeTULog2MinSize()) );

#if G507_COND_4X4_ENABLE_FLAG
  if(log2MinCUSize == 3)
  {
    xReadFlag( uiCode ); pcSPS->setDisInter4x4( uiCode ? true : false );
  }
#endif

  if( pcSPS->getUsePCM() )
  {
    READ_UVLC( uiCode, "log2_min_pcm_coding_block_size_minus3" );  pcSPS->setPCMLog2MinSize (uiCode+3); 
    READ_UVLC( uiCode, "log2_diff_max_min_pcm_coding_block_size" ); pcSPS->setPCMLog2MaxSize ( uiCode+pcSPS->getPCMLog2MinSize() );
  }

  READ_UVLC( uiCode, "max_transform_hierarchy_depth_inter" );    pcSPS->setQuadtreeTUMaxDepthInter( uiCode+1 );
  READ_UVLC( uiCode, "max_transform_hierarchy_depth_intra" );    pcSPS->setQuadtreeTUMaxDepthIntra( uiCode+1 );
  g_uiAddCUDepth = 0;
  while( ( pcSPS->getMaxCUWidth() >> uiMaxCUDepthCorrect ) > ( 1 << ( pcSPS->getQuadtreeTULog2MinSize() + g_uiAddCUDepth )  ) ) g_uiAddCUDepth++;    
  pcSPS->setMaxCUDepth( uiMaxCUDepthCorrect+g_uiAddCUDepth  ); g_uiMaxCUDepth  = uiMaxCUDepthCorrect+g_uiAddCUDepth;
  // BB: these parameters may be removed completly and replaced by the fixed values
  pcSPS->setMinTrDepth( 0 );
  pcSPS->setMaxTrDepth( 1 );
#if SCALING_LIST
  READ_FLAG( uiCode, "scaling_list_enable_flag" );               pcSPS->setScalingListFlag ( uiCode );
#endif
  READ_FLAG( uiCode, "chroma_pred_from_luma_enabled_flag" );     pcSPS->setUseLMChroma ( uiCode ? true : false ); 
  READ_FLAG( uiCode, "deblocking_filter_In_APS_enabled_flag" );    pcSPS->setUseDF ( uiCode ? true : false );  
  READ_FLAG( uiCode, "loop_filter_across_slice_flag" );          pcSPS->setLFCrossSliceBoundaryFlag( uiCode ? true : false);
  READ_FLAG( uiCode, "sample_adaptive_offset_enabled_flag" );    pcSPS->setUseSAO ( uiCode ? true : false );  
  READ_FLAG( uiCode, "adaptive_loop_filter_enabled_flag" );      pcSPS->setUseALF ( uiCode ? true : false );

  if( pcSPS->getUsePCM() )
  {
    READ_FLAG( uiCode, "pcm_loop_filter_disable_flag" );           pcSPS->setPCMFilterDisableFlag ( uiCode ? true : false );
  }

#if !G507_QP_ISSUE_FIX
  READ_FLAG( uiCode, "cu_qp_delta_enabled_flag" );               pcSPS->setUseDQP ( uiCode ? true : false );
#endif
  READ_FLAG( uiCode, "temporal_id_nesting_flag" );               pcSPS->setTemporalIdNestingFlag ( uiCode > 0 ? true : false );

  // !!!KS: Syntax not in WD !!!

  xReadUvlc ( uiCode ); pcSPS->setPadX        ( uiCode    );
  xReadUvlc ( uiCode ); pcSPS->setPadY        ( uiCode    );

  xReadFlag( uiCode ); pcSPS->setUseMRG ( uiCode ? true : false );
  
  // AMVP mode for each depth (AM_NONE or AM_EXPL)
  for (Int i = 0; i < pcSPS->getMaxCUDepth(); i++)
  {
    xReadFlag( uiCode );
    pcSPS->setAMVPMode( i, (AMVP_MODE)uiCode );
  }

  READ_FLAG ( uiCode, "uniform_spacing_flag" ); 
  pcSPS->setUniformSpacingIdr( uiCode );
#if !NONCROSS_TILE_IN_LOOP_FILTERING
  READ_FLAG ( uiCode, "tile_boundary_independence_flag" );  
  pcSPS->setTileBoundaryIndependenceIdr( uiCode );
#endif 
  READ_UVLC ( uiCode, "num_tile_columns_minus1" );
  pcSPS->setNumColumnsMinus1( uiCode );  
  READ_UVLC ( uiCode, "num_tile_rows_minus1" ); 
  pcSPS->setNumRowsMinus1( uiCode ); 
  if( pcSPS->getUniformSpacingIdr() == 0 )
  {
    UInt* columnWidth = (UInt*)malloc(pcSPS->getNumColumnsMinus1()*sizeof(UInt));
    for(UInt i=0; i<pcSPS->getNumColumnsMinus1(); i++)
    { 
      READ_UVLC( uiCode, "column_width" );
      columnWidth[i] = uiCode;  
    }
    pcSPS->setColumnWidth(columnWidth);
    free(columnWidth);
  
    UInt* rowHeight = (UInt*)malloc(pcSPS->getNumRowsMinus1()*sizeof(UInt));
    for(UInt i=0; i<pcSPS->getNumRowsMinus1(); i++)
    {
      READ_UVLC( uiCode, "row_height" );
      rowHeight[i] = uiCode;  
    }
    pcSPS->setRowHeight(rowHeight);
    free(rowHeight);  
  }
#if NONCROSS_TILE_IN_LOOP_FILTERING
  pcSPS->setTileBoundaryIndependenceIdr( 1 ); //default
  pcSPS->setLFCrossTileBoundaryFlag(true); //default

  if( pcSPS->getNumColumnsMinus1() !=0 || pcSPS->getNumRowsMinus1() != 0)
  {
    READ_FLAG ( uiCode, "tile_boundary_independence_flag" );  
    pcSPS->setTileBoundaryIndependenceIdr( uiCode );
    if(pcSPS->getTileBoundaryIndependenceIdr() == 1)
    {
      READ_FLAG ( uiCode, "loop_filter_across_tile_flag" );  
      pcSPS->setLFCrossTileBoundaryFlag( (uiCode==1)?true:false);
    }
  }
#endif

  // Software-only flags
  READ_FLAG( uiCode, "enable_nsqt" );
  pcSPS->setUseNSQT( uiCode );
  READ_FLAG( uiCode, "enable_amp" );
  pcSPS->setUseAMP( uiCode );
  return;
}

#if TILES_DECODER
Void TDecCavlc::readTileMarker   ( UInt& uiTileIdx, UInt uiBitsUsed )
{
  xReadCode ( uiBitsUsed, uiTileIdx );
}
#endif

Void TDecCavlc::parseSliceHeader (TComSlice*& rpcSlice)
{
  UInt  uiCode;
  Int   iCode;
  
#if ENC_DEC_TRACE
  xTraceSliceHeader(rpcSlice);
#endif
  
#if SLICEADDR_BEGIN  
  // if( nal_ref_idc != 0 )
  //   dec_ref_pic_marking( )
  // if( entropy_coding_mode_flag  &&  slice_type  !=  I)
  //   cabac_init_idc
  // first_slice_in_pic_flag
  // if( first_slice_in_pic_flag == 0 )
  //   slice_address
  Int numCUs = ((rpcSlice->getSPS()->getWidth()+rpcSlice->getSPS()->getMaxCUWidth()-1)/rpcSlice->getSPS()->getMaxCUWidth())*((rpcSlice->getSPS()->getHeight()+rpcSlice->getSPS()->getMaxCUHeight()-1)/rpcSlice->getSPS()->getMaxCUHeight());
  Int maxParts = (1<<(rpcSlice->getSPS()->getMaxCUDepth()<<1));
  Int numParts = (1<<(rpcSlice->getPPS()->getSliceGranularity()<<1));
  UInt lCUAddress = 0;
  Int reqBitsOuter = 0;
  while(numCUs>(1<<reqBitsOuter))
  {
    reqBitsOuter++;
  }
  Int reqBitsInner = 0;
  while((numParts)>(1<<reqBitsInner)) 
  {
    reqBitsInner++;
  }
  READ_FLAG( uiCode, "first_slice_in_pic_flag" );
  UInt address;
  UInt innerAddress = 0;
  if(!uiCode)
  {
    READ_CODE( reqBitsOuter+reqBitsInner, address, "slice_address" );
    lCUAddress = address >> reqBitsInner;
    innerAddress = address - (lCUAddress<<reqBitsInner);
  }
  //set uiCode to equal slice start address (or entropy slice start address)
  uiCode=(maxParts*lCUAddress)+(innerAddress*(maxParts>>(rpcSlice->getPPS()->getSliceGranularity()<<1)));
  
  rpcSlice->setEntropySliceCurStartCUAddr( uiCode );
  rpcSlice->setEntropySliceCurEndCUAddr(numCUs*maxParts);
#endif

#if INC_CABACINITIDC_SLICETYPE
  //   slice_type
  READ_UVLC (    uiCode, "slice_type" );            rpcSlice->setSliceType((SliceType)uiCode);
#endif
  // lightweight_slice_flag
  READ_FLAG( uiCode, "lightweight_slice_flag" );
  Bool bEntropySlice = uiCode ? true : false;


#if SLICEADDR_BEGIN
  if (bEntropySlice)
  {
    rpcSlice->setNextSlice        ( false );
    rpcSlice->setNextEntropySlice ( true  );
  }
  else
  {
    rpcSlice->setNextSlice        ( true  );
    rpcSlice->setNextEntropySlice ( false );
    
    uiCode=(maxParts*lCUAddress)+(innerAddress*(maxParts>>(rpcSlice->getPPS()->getSliceGranularity()<<1)));
    rpcSlice->setSliceCurStartCUAddr(uiCode);
    rpcSlice->setSliceCurEndCUAddr(numCUs*maxParts);
  }
#endif
  
  // if( !lightweight_slice_flag ) {
  if (!bEntropySlice)
  {
#if !INC_CABACINITIDC_SLICETYPE
    //   slice_type
    READ_UVLC (    uiCode, "slice_type" );            rpcSlice->setSliceType((SliceType)uiCode);
#endif
    READ_UVLC (    uiCode, "pic_parameter_set_id" );  rpcSlice->setPPSId(uiCode);
    if(rpcSlice->getNalUnitType()==NAL_UNIT_CODED_SLICE_IDR) 
    { 
      READ_UVLC( uiCode, "idr_pic_id" );  //ignored
      READ_FLAG( uiCode, "no_output_of_prior_pics_flag" );  //ignored
      rpcSlice->setPOC(0);
      TComReferencePictureSet* pcRPS = rpcSlice->getLocalRPS();
      pcRPS->setNumberOfNegativePictures(0);
      pcRPS->setNumberOfPositivePictures(0);
      pcRPS->setNumberOfLongtermPictures(0);
      pcRPS->setNumberOfPictures(0);
      rpcSlice->setRPS(pcRPS);
    }
    else
    {
      READ_CODE(rpcSlice->getSPS()->getBitsForPOC(), uiCode, "pic_order_cnt_lsb");  
      Int iPOClsb = uiCode;
      Int iPrevPOC = rpcSlice->getPrevPOC();
      Int iMaxPOClsb = 1<<rpcSlice->getSPS()->getBitsForPOC();
      Int iPrevPOClsb = iPrevPOC%iMaxPOClsb;
      Int iPrevPOCmsb = iPrevPOC-iPrevPOClsb;
      Int iPOCmsb;
      if( ( iPOClsb  <  iPrevPOClsb ) && ( ( iPrevPOClsb - iPOClsb )  >=  ( iMaxPOClsb / 2 ) ) )
      {
        iPOCmsb = iPrevPOCmsb + iMaxPOClsb;
      }
      else if( (iPOClsb  >  iPrevPOClsb )  && ( (iPOClsb - iPrevPOClsb )  >  ( iMaxPOClsb / 2 ) ) ) 
      {
        iPOCmsb = iPrevPOCmsb - iMaxPOClsb;
      }
      else
      {
        iPOCmsb = iPrevPOCmsb;
      }
      rpcSlice->setPOC              (iPOCmsb+iPOClsb);

      TComReferencePictureSet* pcRPS;
      READ_FLAG( uiCode, "short_term_ref_pic_set_pps_flag" );
      if(uiCode == 0) // use short-term reference picture set explicitly signalled in slice header
      {
        pcRPS = rpcSlice->getLocalRPS();
#if INTER_RPS_PREDICTION
        parseShortTermRefPicSet(rpcSlice->getPPS(),pcRPS, rpcSlice->getPPS()->getRPSList()->getNumberOfReferencePictureSets());
#else
        parseShortTermRefPicSet(rpcSlice->getPPS(),pcRPS);
#endif        
        rpcSlice->setRPS(pcRPS);
      }
      else // use reference to short-term reference picture set in PPS
      {
        READ_UVLC( uiCode, "short_term_ref_pic_set_idx"); rpcSlice->setRPS(rpcSlice->getPPS()->getRPSList()->getReferencePictureSet(uiCode));
        pcRPS = rpcSlice->getRPS();
      }
      if(rpcSlice->getPPS()->getLongTermRefsPresent())
      {
        Int offset = pcRPS->getNumberOfNegativePictures()+pcRPS->getNumberOfPositivePictures();
        READ_UVLC( uiCode, "num_long_term_pics");             pcRPS->setNumberOfLongtermPictures(uiCode);
        Int prev = 0;
        for(Int j=pcRPS->getNumberOfLongtermPictures()+offset-1 ; j > offset-1; j--)
        {
          READ_UVLC(uiCode,"delta_poc_lsb_lt_minus1"); 
          prev += 1+uiCode;
          pcRPS->setPOC(j,rpcSlice->getPOC()-prev);          
          pcRPS->setDeltaPOC(j,-(Int)prev);
          READ_FLAG( uiCode, "used_by_curr_pic_lt_flag");     pcRPS->setUsed(j,uiCode);
        }
        offset += pcRPS->getNumberOfLongtermPictures();
        pcRPS->setNumberOfPictures(offset);        
      }  
    }
#if SCALING_LIST
    if(rpcSlice->getSPS()->getUseSAO() || rpcSlice->getSPS()->getUseALF() || rpcSlice->getSPS()->getScalingListFlag() || rpcSlice->getSPS()->getUseDF())
#else
    if(rpcSlice->getSPS()->getUseSAO() || rpcSlice->getSPS()->getUseALF())
#endif
    {
#if ALF_SAO_SLICE_FLAGS
      if (rpcSlice->getSPS()->getUseALF())
      {
        READ_FLAG(uiCode, "ALF flag in slice header");
        rpcSlice->setAlfEnabledFlag((Bool)uiCode);
      }
      if (rpcSlice->getSPS()->getUseSAO())
      {
        READ_FLAG(uiCode, "SAO flag in slice header");
        rpcSlice->setSaoEnabledFlag((Bool)uiCode);
      }
#endif
      READ_UVLC (    uiCode, "aps_id" );  rpcSlice->setAPSId(uiCode);
    }
    if (!rpcSlice->isIntra())
    {
      READ_FLAG( uiCode, "num_ref_idx_active_override_flag");
      if (uiCode)
      {
        READ_CODE (3, uiCode, "num_ref_idx_l0_active_minus1" );  rpcSlice->setNumRefIdx( REF_PIC_LIST_0, uiCode + 1 );
        if (rpcSlice->isInterB())
        {
          READ_CODE (3, uiCode, "num_ref_idx_l1_active_minus1" );  rpcSlice->setNumRefIdx( REF_PIC_LIST_1, uiCode + 1 );
        }
        else
        {
          rpcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
        }
      }
      else
      {
        rpcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
        rpcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
      }
    }
    // }
    TComRefPicListModification* refPicListModification = rpcSlice->getRefPicListModification();
    if(!rpcSlice->isIntra())
    {
      READ_FLAG( uiCode, "ref_pic_list_modification_flag_l0" ); refPicListModification->setRefPicListModificationFlagL0( uiCode ? 1 : 0 );
      
      if(refPicListModification->getRefPicListModificationFlagL0())
      {
        uiCode = 0;
        Int i = 0;
        Int list_modification_idc = 0;
        while(list_modification_idc != 3)  
        {
          READ_UVLC( uiCode, "list_modification_idc" ); refPicListModification->setListIdcL0(i, uiCode );
          list_modification_idc = uiCode;
          if(uiCode != 3)
          {
            READ_UVLC( uiCode, "ref_pic_set_idx" ); refPicListModification->setRefPicSetIdxL0(i, uiCode );
          }
          i++;
        }
        refPicListModification->setNumberOfRefPicListModificationsL0(i-1);
      }
      else
      {
        refPicListModification->setNumberOfRefPicListModificationsL0(0); 
      }
    }
    else
    {
      refPicListModification->setRefPicListModificationFlagL0(0);
      refPicListModification->setNumberOfRefPicListModificationsL0(0);
    }
    if(rpcSlice->isInterB())
    {
      READ_FLAG( uiCode, "ref_pic_list_modification_flag_l1" ); refPicListModification->setRefPicListModificationFlagL1( uiCode ? 1 : 0 );
      if(refPicListModification->getRefPicListModificationFlagL1())
      {
        uiCode = 0;
        Int i = 0;
        Int list_modification_idc = 0;
        while(list_modification_idc != 3)  
        {
          READ_UVLC( uiCode, "list_modification_idc" ); refPicListModification->setListIdcL1(i, uiCode );
          list_modification_idc = uiCode;
          if(uiCode != 3)
          {
            READ_UVLC( uiCode, "ref_pic_set_idx" ); refPicListModification->setRefPicSetIdxL1(i, uiCode );
          }
          i++;
        }
        refPicListModification->setNumberOfRefPicListModificationsL1(i-1);
      }
      else
      {
        refPicListModification->setNumberOfRefPicListModificationsL1(0);
      }
    }  
    else
    {
      refPicListModification->setRefPicListModificationFlagL1(0);
      refPicListModification->setNumberOfRefPicListModificationsL1(0);
    }
  }
  // ref_pic_list_combination( )
  if (rpcSlice->isInterB())
  {
    READ_FLAG( uiCode, "ref_pic_list_combination_flag" );       rpcSlice->setRefPicListCombinationFlag( uiCode ? 1 : 0 );
    if(uiCode)
    {
      READ_UVLC( uiCode, "num_ref_idx_lc_active_minus1" );      rpcSlice->setNumRefIdx( REF_PIC_LIST_C, uiCode + 1 );
      
      READ_FLAG( uiCode, "ref_pic_list_modification_flag_lc" ); rpcSlice->setRefPicListModificationFlagLC( uiCode ? 1 : 0 );
      if(uiCode)
      {
        for (UInt i=0;i<rpcSlice->getNumRefIdx(REF_PIC_LIST_C);i++)
        {
          READ_FLAG( uiCode, "pic_from_list_0_flag" );
          rpcSlice->setListIdFromIdxOfLC(i, uiCode);
          READ_UVLC( uiCode, "ref_idx_list_curr" );
          rpcSlice->setRefIdxFromIdxOfLC(i, uiCode);
          rpcSlice->setRefIdxOfLC((RefPicList)rpcSlice->getListIdFromIdxOfLC(i), rpcSlice->getRefIdxFromIdxOfLC(i), i);
        }
      }
    }
    else
    {
      rpcSlice->setRefPicListModificationFlagLC(false);
      rpcSlice->setNumRefIdx(REF_PIC_LIST_C, 0);
    }
  }
  else
  {
    rpcSlice->setRefPicListCombinationFlag(false);      
  }
  
#if INC_CABACINITIDC_SLICETYPE
  if(rpcSlice->getPPS()->getEntropyCodingMode() && !rpcSlice->isIntra())
  {
    READ_UVLC(uiCode, "cabac_init_idc");
    rpcSlice->setCABACinitIDC(uiCode);
  }
  else if (rpcSlice->getPPS()->getEntropyCodingMode() && rpcSlice->isIntra())
  {
    rpcSlice->setCABACinitIDC(0);
  }
#endif

#if !SLICEADDR_BEGIN
  // if( nal_ref_idc != 0 )
  //   dec_ref_pic_marking( )
  // if( entropy_coding_mode_flag  &&  slice_type  !=  I)
  //   cabac_init_idc
  // first_slice_in_pic_flag
  // if( first_slice_in_pic_flag == 0 )
  //   slice_address
  int iNumCUs = ((rpcSlice->getSPS()->getWidth()+rpcSlice->getSPS()->getMaxCUWidth()-1)/rpcSlice->getSPS()->getMaxCUWidth())*((rpcSlice->getSPS()->getHeight()+rpcSlice->getSPS()->getMaxCUHeight()-1)/rpcSlice->getSPS()->getMaxCUHeight());
  int iMaxParts = (1<<(rpcSlice->getSPS()->getMaxCUDepth()<<1));
  int iNumParts = (1<<(rpcSlice->getPPS()->getSliceGranularity()<<1));
  UInt uiLCUAddress = 0;
  int iReqBitsOuter = 0;
  while(iNumCUs>(1<<iReqBitsOuter))
  {
    iReqBitsOuter++;
  }
  int iReqBitsInner = 0;
  while((iNumParts)>(1<<iReqBitsInner)) 
  {
    iReqBitsInner++;
  }
  READ_FLAG( uiCode, "first_slice_in_pic_flag" );
  UInt uiAddress;
  UInt uiInnerAddress = 0;
  if(!uiCode)
  {
    READ_CODE( iReqBitsOuter+iReqBitsInner, uiAddress, "slice_address" );
    uiLCUAddress = uiAddress >> iReqBitsInner;
    uiInnerAddress = uiAddress - (uiLCUAddress<<iReqBitsInner);
  }
  //set uiCode to equal slice start address (or entropy slice start address)
  uiCode=(iMaxParts*uiLCUAddress)+(uiInnerAddress*(iMaxParts>>(rpcSlice->getPPS()->getSliceGranularity()<<1)));
  
  rpcSlice->setEntropySliceCurStartCUAddr( uiCode );
  rpcSlice->setEntropySliceCurEndCUAddr(iNumCUs*iMaxParts);
  if (bEntropySlice)
  {
    rpcSlice->setNextSlice        ( false );
    rpcSlice->setNextEntropySlice ( true  );
  }
  else
  {
    rpcSlice->setNextSlice        ( true  );
    rpcSlice->setNextEntropySlice ( false );
    
    rpcSlice->setSliceCurStartCUAddr(uiCode);
    rpcSlice->setSliceCurEndCUAddr(iNumCUs*iMaxParts);
  }
#endif
  if(!bEntropySlice)
  {
#if G507_QP_ISSUE_FIX
    READ_SVLC( iCode, "slice_qp_delta" ); 
    rpcSlice->setSliceQp (26 + rpcSlice->getPPS()->getPicInitQPMinus26() + iCode);
#else
    // if( !lightweight_slice_flag ) {
    //   slice_qp_delta
    // should be delta
    READ_SVLC( iCode, "slice_qp" );  rpcSlice->setSliceQp          (iCode);
#endif   
    //   if( sample_adaptive_offset_enabled_flag )
    //     sao_param()
    //   if( deblocking_filter_control_present_flag ) {
    //     disable_deblocking_filter_idc
    // this should be an idc
    READ_FLAG ( uiCode, "inherit_dbl_param_from_APS_flag" ); rpcSlice->setInheritDblParamFromAPS(uiCode ? 1 : 0);
    if(!rpcSlice->getInheritDblParamFromAPS())
    {
      READ_FLAG ( uiCode, "loop_filter_disable" );  rpcSlice->setLoopFilterDisable(uiCode ? 1 : 0);
      if(!rpcSlice->getLoopFilterDisable())
      {
        READ_SVLC( iCode, "beta_offset_div2" ); rpcSlice->setLoopFilterBetaOffset(iCode);
        READ_SVLC( iCode, "tc_offset_div2" ); rpcSlice->setLoopFilterTcOffset(iCode);
      }
    }
    //     if( disable_deblocking_filter_idc  !=  1 ) {
    //       slice_alpha_c0_offset_div2
    //       slice_beta_offset_div2
    //     }
    //   }
    //   if( slice_type = = B )
    //     collocated_from_l0_flag
    if ( rpcSlice->getSliceType() == B_SLICE )
    {
      READ_FLAG( uiCode, "collocated_from_l0_flag" );
      rpcSlice->setColDir(uiCode);
    }
    
    //   if( adaptive_loop_filter_enabled_flag ) {
    //     if( !shared_pps_info_enabled_flag )
    //       alf_param( )
    //     alf_cu_control_param( )
    //   }
    // }
  
    if ( (rpcSlice->getPPS()->getUseWP() && rpcSlice->getSliceType()==P_SLICE) || (rpcSlice->getPPS()->getWPBiPredIdc() && rpcSlice->getSliceType()==B_SLICE) )
    {
      xParsePredWeightTable(rpcSlice);
      rpcSlice->initWpScaling();
    }
  }

  // !!!! Syntax elements not in the WD  !!!!!
  
#if G091_SIGNAL_MAX_NUM_MERGE_CANDS
  READ_UVLC( uiCode, "MaxNumMergeCand");
  rpcSlice->setMaxNumMergeCand(MRG_MAX_NUM_CANDS - uiCode);
  assert(rpcSlice->getMaxNumMergeCand()==MRG_MAX_NUM_CANDS_SIGNALED);
#endif

#if !G220_PURE_VLC_SAO_ALF
#if TILES_DECODER
  rpcSlice->setTileMarkerFlag ( 0 ); // default
  if (!bEntropySlice)
  {
    if (rpcSlice->getSPS()->getTileBoundaryIndependenceIdr())
    {   
      xReadCode(1, uiCode); // read flag indicating if tile markers transmitted
      rpcSlice->setTileMarkerFlag( uiCode );
    }
  }
#endif

  if (rpcSlice->getPPS()->getEntropyCodingSynchro())
  {
    UInt uiNumSubstreams = rpcSlice->getPPS()->getNumSubstreams();
    rpcSlice->allocSubstreamSizes(uiNumSubstreams);
    UInt *puiSubstreamSizes = rpcSlice->getSubstreamSizes();

    for (UInt ui = 0; ui+1 < uiNumSubstreams; ui++)
    {
      xReadCode(2, uiCode);
      
      switch ( uiCode )
      {
      case 0:
        xReadCode(8,  uiCode);
        break;
      case 1:
        xReadCode(16, uiCode);
        break;
      case 2:
        xReadCode(24, uiCode);
        break;
      case 3:
        xReadCode(32, uiCode);
        break;
      default:
        printf("Error in parseSliceHeader\n");
        exit(-1);
        break;
      }
      puiSubstreamSizes[ui] = uiCode;
    }
  }

#if TILES_DECODER
  if (!bEntropySlice)
  {
    // Reading location information
    if (rpcSlice->getSPS()->getTileBoundaryIndependenceIdr())
    {   
      xReadCode(1, uiCode); // read flag indicating if location information signaled in slice header
      Bool bTileLocationInformationInSliceHeaderFlag = (uiCode)? true : false;

      if (bTileLocationInformationInSliceHeaderFlag)
      {
        // location count
        xReadCode(5, uiCode); // number of tiles for which location information signaled
        rpcSlice->setTileLocationCount ( uiCode + 1 );

        xReadCode(5, uiCode); // number of bits used by diff
        Int iBitsUsedByDiff = uiCode + 1;

        // read out tile start location
        Int iLastSize = 0;
        for (UInt uiIdx=0; uiIdx<rpcSlice->getTileLocationCount(); uiIdx++)
        {
          Int iAbsDiff, iCurSize, iCurDiff;
          if (uiIdx==0)
          {
            xReadCode(iBitsUsedByDiff-1, uiCode); iAbsDiff  = uiCode;
            rpcSlice->setTileLocation( uiIdx, iAbsDiff );
            iCurDiff  = iAbsDiff;
            iLastSize = iAbsDiff;
          }
          else
          {
            xReadCode(1, uiCode); // read sign
            Int iSign = (uiCode) ? -1 : +1;

            xReadCode(iBitsUsedByDiff-1, uiCode); iAbsDiff  = uiCode;
            iCurDiff  = (iSign) * iAbsDiff;
            iCurSize  = iLastSize + iCurDiff;
            iLastSize = iCurSize;
            rpcSlice->setTileLocation( uiIdx, rpcSlice->getTileLocation( uiIdx-1 ) + iCurSize ); // calculate byte location
          }
        }
      }

      // read out trailing bits
      m_pcBitstream->readOutTrailingBits();
    }
  }
#endif
#endif
  return;
}

#if G220_PURE_VLC_SAO_ALF
Void TDecCavlc::parseWPPTileInfoToSliceHeader(TComSlice*& rpcSlice)
{
  Bool bEntropySlice = (!rpcSlice->isNextSlice());
  UInt uiCode;

#if TILES_DECODER
  rpcSlice->setTileMarkerFlag ( 0 ); // default
  if (!bEntropySlice)
  {
#if !TILES_LOW_LATENCY_CABAC_INI
    if (rpcSlice->getSPS()->getTileBoundaryIndependenceIdr())
#endif
    {   
      xReadCode(1, uiCode); // read flag indicating if tile markers transmitted
      rpcSlice->setTileMarkerFlag( uiCode );
    }
  }

  if (rpcSlice->getPPS()->getEntropyCodingSynchro())
  {
    UInt uiNumSubstreams = rpcSlice->getPPS()->getNumSubstreams();
    rpcSlice->allocSubstreamSizes(uiNumSubstreams);
    UInt *puiSubstreamSizes = rpcSlice->getSubstreamSizes();

    for (UInt ui = 0; ui+1 < uiNumSubstreams; ui++)
    {
      xReadCode(2, uiCode);

      switch ( uiCode )
      {
      case 0:
        xReadCode(8,  uiCode);
        break;
      case 1:
        xReadCode(16, uiCode);
        break;
      case 2:
        xReadCode(24, uiCode);
        break;
      case 3:
        xReadCode(32, uiCode);
        break;
      default:
        printf("Error in parseSliceHeader\n");
        exit(-1);
        break;
      }
      puiSubstreamSizes[ui] = uiCode;
    }
  }

#if TILES_DECODER
  if (!bEntropySlice)
  {
    // Reading location information
#if !TILES_LOW_LATENCY_CABAC_INI
    if (rpcSlice->getSPS()->getTileBoundaryIndependenceIdr())
#endif
    {   
      xReadCode(1, uiCode); // read flag indicating if location information signaled in slice header
      Bool bTileLocationInformationInSliceHeaderFlag = (uiCode)? true : false;

      if (bTileLocationInformationInSliceHeaderFlag)
      {
        // location count
        xReadCode(5, uiCode); // number of tiles for which location information signaled
        rpcSlice->setTileLocationCount ( uiCode + 1 );

        xReadCode(5, uiCode); // number of bits used by diff
        Int iBitsUsedByDiff = uiCode + 1;

        // read out tile start location
        Int iLastSize = 0;
        for (UInt uiIdx=0; uiIdx<rpcSlice->getTileLocationCount(); uiIdx++)
        {
          Int iAbsDiff, iCurSize, iCurDiff;
          if (uiIdx==0)
          {
            xReadCode(iBitsUsedByDiff-1, uiCode); iAbsDiff  = uiCode;
            rpcSlice->setTileLocation( uiIdx, iAbsDiff );
            iCurDiff  = iAbsDiff;
            iLastSize = iAbsDiff;
          }
          else
          {
            xReadCode(1, uiCode); // read sign
            Int iSign = (uiCode) ? -1 : +1;

            xReadCode(iBitsUsedByDiff-1, uiCode); iAbsDiff  = uiCode;
            iCurDiff  = (iSign) * iAbsDiff;
            iCurSize  = iLastSize + iCurDiff;
            iLastSize = iCurSize;
            rpcSlice->setTileLocation( uiIdx, rpcSlice->getTileLocation( uiIdx-1 ) + iCurSize ); // calculate byte location
          }
        }
      }

      // read out trailing bits
      m_pcBitstream->readOutTrailingBits();
    }
  }
#endif

}
#endif
#endif


Void TDecCavlc::resetEntropy          (TComSlice* pcSlice)
{
  m_bRunLengthCoding = ! pcSlice->isIntra();
  m_uiRun = 0;
  
  ::memcpy(m_uiLPTableD4,        g_auiLPTableD4,        3*32*sizeof(UInt));
  ::memcpy(m_uiLastPosVlcIndex,  g_auiLastPosVlcIndex,  10*sizeof(UInt));

  ::memcpy(m_uiCBP_YUV_TableD, g_auiCBP_YUV_TableD, 4*8*sizeof(UInt));
  ::memcpy(m_uiCBP_YS_TableD,  g_auiCBP_YS_TableD,  2*4*sizeof(UInt));
  ::memcpy(m_uiCBP_YCS_TableD, g_auiCBP_YCS_TableD, 2*8*sizeof(UInt));
  ::memcpy(m_uiCBP_4Y_TableD,  g_auiCBP_4Y_TableD,  2*15*sizeof(UInt));
  m_uiCBP_4Y_VlcIdx = 0;

  ::memcpy(m_uiMI1TableD, g_auiComMI1TableD, 9*sizeof(UInt));
  
  ::memcpy(m_uiIntraModeTableD17, g_auiIntraModeTableD17, 17*sizeof(UInt));
  ::memcpy(m_uiIntraModeTableD34, g_auiIntraModeTableD34, 34*sizeof(UInt));
  ::memcpy(m_uiSplitTableD, g_auiInterModeTableD, 4*11*sizeof(UInt));
  m_uiMITableVlcIdx = 0;

  ::memset(m_ucCBP_YUV_TableCounter, 0, 4*4*sizeof(UChar));
  ::memset(m_ucCBP_4Y_TableCounter,  0, 2*2*sizeof(UChar));
  ::memset(m_ucCBP_YCS_TableCounter, 0, 2*4*sizeof(UChar));
  ::memset(m_ucCBP_YS_TableCounter,  0, 2*3*sizeof(UChar));

  ::memset(m_ucMI1TableCounter,        0,          4*sizeof(UChar));
  ::memset(m_ucSplitTableCounter,      0,        4*4*sizeof(UChar));

  m_ucCBP_YUV_TableCounterSum[0] = m_ucCBP_YUV_TableCounterSum[1] = m_ucCBP_YUV_TableCounterSum[2] = m_ucCBP_YUV_TableCounterSum[3] = 0;
  m_ucCBP_4Y_TableCounterSum[0] = m_ucCBP_4Y_TableCounterSum[1] = 0;
  m_ucCBP_YCS_TableCounterSum[0] = m_ucCBP_YCS_TableCounterSum[1] = 0;
  m_ucCBP_YS_TableCounterSum[0] = m_ucCBP_YS_TableCounterSum[1] = 0;

  m_ucSplitTableCounterSum[0] = m_ucSplitTableCounterSum[1] = m_ucSplitTableCounterSum[2]= m_ucSplitTableCounterSum[3] = 0;
  m_ucMI1TableCounterSum = 0;
}

Void TDecCavlc::parseTerminatingBit( UInt& ruiBit )
{
  ruiBit = false;
  Int iBitsLeft = m_pcBitstream->getNumBitsLeft();
  if(iBitsLeft <= 8)
  {
    UInt uiPeekValue = m_pcBitstream->peekBits(iBitsLeft);
    if (uiPeekValue == (1<<(iBitsLeft-1)))
    {
      ruiBit = true;
    }
  }
}

Void TDecCavlc::parseAlfCtrlDepth              ( UInt& ruiAlfCtrlDepth )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, g_uiMaxCUDepth-1);
  ruiAlfCtrlDepth = uiSymbol;
}


Void TDecCavlc::parseSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parseMVPIdx( Int& riMVPIdx )
{
  assert(0);
}

Void TDecCavlc::parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parsePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parsePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

/** Parse I_PCM information. 
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \param uiDepth CU depth
 * \returns Void
 *
 * If I_PCM flag indicates that the CU is I_PCM, parse its PCM alignment bits and codes.  
 */
Void TDecCavlc::parseIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;

  xReadFlag( uiSymbol );

  if ( uiSymbol )
  {
    Bool bIpcmFlag   = true;

    xReadPCMAlignZero();

    pcCU->setPartSizeSubParts  ( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts      ( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setIPCMFlagSubParts  ( bIpcmFlag, uiAbsPartIdx, uiDepth );

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
        UInt uiSample;
        xReadCode(uiSampleBits, uiSample);

        piPCMSample[uiX] = uiSample;
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
        UInt uiSample;
        xReadCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
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
        UInt uiSample;
        xReadCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }
  }
}

Void TDecCavlc::parseIntraDirLumaAng  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{ 
  assert(0);
}

Void TDecCavlc::parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parseInterDir( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parseRefFrmIdx( TComDataCU* pcCU, Int& riRefFrmIdx, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList )
{
  assert(0);
}

Void TDecCavlc::parseMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  assert(0);
}

Void TDecCavlc::parseDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiQp;
  Int  iDQp;
  
  xReadSvlc( iDQp );
  uiQp = pcCU->getRefQP( uiAbsPartIdx ) + iDQp;

  UInt uiAbsQpCUPartIdx = (uiAbsPartIdx>>(8-(pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()<<1)))<<(8-(pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()<<1)) ;
  UInt uiQpCUDepth =   min(uiDepth,pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()) ;
  pcCU->setQPSubParts( uiQp, uiAbsQpCUPartIdx, uiQpCUDepth );
}

Void TDecCavlc::parseCbfTrdiv( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiTrDepth, UInt uiDepth, UInt& uiSubdiv )
{
  assert(0);
}

UInt TDecCavlc::xGetFlagPattern( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& uiSubdiv )
{
  assert(0);
  return 0;
}

Void TDecCavlc::parseCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parseBlockCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth, UInt uiQPartNum )
{
  assert(0);
}

Void TDecCavlc::parseCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
  assert(0);
}

Void TDecCavlc::parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize )
{
  assert(0);
}

Void TDecCavlc::parseQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parseQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& uiQtRootCbf )
{
  assert(0);
}

Void TDecCavlc::parseAlfFlag (UInt& ruiVal)
{
  xReadFlag( ruiVal );
}

Void TDecCavlc::parseAlfCtrlFlag( UInt &ruiAlfCtrlFlag )
{
  UInt uiSymbol;
  xReadFlag( uiSymbol );
  ruiAlfCtrlFlag = uiSymbol;
}

Void TDecCavlc::parseAlfUvlc (UInt& ruiVal)
{
  xReadUvlc( ruiVal );
}

Void TDecCavlc::parseAlfSvlc (Int&  riVal)
{
  xReadSvlc( riVal );
}

Void TDecCavlc::parseSaoFlag (UInt& ruiVal)
{
  xReadFlag( ruiVal );
}

Void TDecCavlc::parseSaoUvlc (UInt& ruiVal)
{
  xReadUvlc( ruiVal );
}

Void TDecCavlc::parseSaoSvlc (Int&  riVal)
{
  xReadSvlc( riVal );
}

Void TDecCavlc::parseMergeFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx )
{
  assert(0);
}

Void TDecCavlc::parseMergeIndex ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
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

/** Parse PCM alignment zero bits.
 * \returns Void
 */
Void TDecCavlc::xReadPCMAlignZero( )
{
  UInt uiNumberOfBits = m_pcBitstream->getNumBitsUntilByteAligned();

  if(uiNumberOfBits)
  {
    UInt uiBits;
    UInt uiSymbol;

    for(uiBits = 0; uiBits < uiNumberOfBits; uiBits++)
    {
      xReadFlag( uiSymbol );

      if(uiSymbol)
      {
        printf("\nWarning! pcm_align_zero include a non-zero value.\n");
      }
    }
  }
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


/** parse explicit wp tables
 * \param TComSlice* pcSlice
 * \returns Void
 */
Void TDecCavlc::xParsePredWeightTable( TComSlice* pcSlice )
{
  wpScalingParam  *wp;
  Bool            bChroma     = true; // color always present in HEVC ?
  TComPPS*        pps         = pcSlice->getPPS();
  SliceType       eSliceType  = pcSlice->getSliceType();
  Int             iNbRef       = (eSliceType == B_SLICE ) ? (2) : (1);
  UInt            uiLog2WeightDenomLuma, uiLog2WeightDenomChroma;
  UInt            uiMode      = 0;

#if WP_IMPROVED_SYNTAX
  if ( (eSliceType==P_SLICE && pps->getUseWP()) || (eSliceType==B_SLICE && pps->getWPBiPredIdc()==1 && pcSlice->getRefPicListCombinationFlag()==0) )
    uiMode = 1; // explicit
  else if ( eSliceType==B_SLICE && pps->getWPBiPredIdc()==2 )
    uiMode = 2; // implicit
  else if (eSliceType==B_SLICE && pps->getWPBiPredIdc()==1 && pcSlice->getRefPicListCombinationFlag())
    uiMode = 3; // combined explicit
#else
  if ( (eSliceType==P_SLICE && pps->getUseWP()) || (eSliceType==B_SLICE && pps->getWPBiPredIdc()==1) )
    uiMode = 1;
  else if ( eSliceType==B_SLICE && pps->getWPBiPredIdc()==2 )
    uiMode = 2;
#endif

  if ( uiMode == 1 )  // explicit
  {
    printf("\nTDecCavlc::xParsePredWeightTable(poc=%d) explicit...\n", pcSlice->getPOC());
#if WP_IMPROVED_SYNTAX
    Int iDeltaDenom;
    // decode delta_luma_log2_weight_denom :
    READ_UVLC( uiLog2WeightDenomLuma, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom
    if( bChroma ) 
    {
      READ_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );     // se(v): delta_chroma_log2_weight_denom
      assert((iDeltaDenom + (Int)uiLog2WeightDenomLuma)>=0);
      uiLog2WeightDenomChroma = (UInt)(iDeltaDenom + uiLog2WeightDenomLuma);
    }
#else
    READ_UVLC( uiLog2WeightDenomLuma, "luma_log2_weight_denom" );       // ue(v): luma_log2_weight_denom
    if( bChroma ) 
    {
      READ_UVLC( uiLog2WeightDenomChroma, "chroma_log2_weight_denom" ); // ue(v): chroma_log2_weight_denom
    }
#endif

    for ( Int iNumRef=0 ; iNumRef<iNbRef ; iNumRef++ ) 
    {
      RefPicList  eRefPicList = ( iNumRef ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);

        wp[0].uiLog2WeightDenom = uiLog2WeightDenomLuma;
        wp[1].uiLog2WeightDenom = uiLog2WeightDenomChroma;
        wp[2].uiLog2WeightDenom = uiLog2WeightDenomChroma;

        UInt  uiCode;
        READ_FLAG( uiCode, "luma_weight_lX_flag" );           // u(1): luma_weight_l0_flag
        wp[0].bPresentFlag = ( uiCode == 1 );
        if ( wp[0].bPresentFlag ) 
        {
#if WP_IMPROVED_SYNTAX
          Int iDeltaWeight;
          READ_SVLC( iDeltaWeight, "delta_luma_weight_lX" );  // se(v): delta_luma_weight_l0[i]
          wp[0].iWeight = (iDeltaWeight + (1<<wp[0].uiLog2WeightDenom));
#else
          READ_SVLC( wp[0].iWeight, "luma_weight_lX" );       // se(v): luma_weight_l0[i]
#endif
          READ_SVLC( wp[0].iOffset, "luma_offset_lX" );       // se(v): luma_offset_l0[i]
        }
        else 
        {
          wp[0].iWeight = (1 << wp[0].uiLog2WeightDenom);
          wp[0].iOffset = 0;
        }
        if ( bChroma ) 
        {
          READ_FLAG( uiCode, "chroma_weight_lX_flag" );      // u(1): chroma_weight_l0_flag
          wp[1].bPresentFlag = ( uiCode == 1 );
          wp[2].bPresentFlag = ( uiCode == 1 );
          if ( wp[1].bPresentFlag ) 
          {
            for ( Int j=1 ; j<3 ; j++ ) 
            {
#if WP_IMPROVED_SYNTAX
              Int iDeltaWeight;
              READ_SVLC( iDeltaWeight, "delta_chroma_weight_lX" );  // se(v): chroma_weight_l0[i][j]
              wp[j].iWeight = (iDeltaWeight + (1<<wp[1].uiLog2WeightDenom));

              Int iDeltaChroma;
              READ_SVLC( iDeltaChroma, "delta_chroma_offset_lX" );  // se(v): delta_chroma_offset_l0[i][j]
              wp[j].iOffset = iDeltaChroma - ( ( (g_uiIBDI_MAX>>1)*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) + (g_uiIBDI_MAX>>1);
#else
              READ_SVLC( wp[j].iWeight, "chroma_weight_lX" );      // se(v): chroma_weight_l0[i][j]
              READ_SVLC( wp[j].iOffset, "chroma_offset_lX" );      // se(v): chroma_offset_l0[i][j]
#endif
            }
          }
          else 
          {
            for ( Int j=1 ; j<3 ; j++ ) 
            {
              wp[j].iWeight = (1 << wp[j].uiLog2WeightDenom);
              wp[j].iOffset = 0;
            }
          }
        }
      }

      for ( Int iRefIdx=pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx<MAX_NUM_REF ; iRefIdx++ ) 
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);

        wp[0].bPresentFlag = false;
        wp[1].bPresentFlag = false;
        wp[2].bPresentFlag = false;
      }
    }
  }
  else if ( uiMode == 2 )  // implicit
  {
    printf("\nTDecCavlc::xParsePredWeightTable(poc=%d) implicit...\n", pcSlice->getPOC());
  }
#if WP_IMPROVED_SYNTAX
  else if ( uiMode == 3 )  // combined explicit
  {
    printf("\nTDecCavlc::xParsePredWeightTable(poc=%d) combined explicit...\n", pcSlice->getPOC());
    Int iDeltaDenom;
    // decode delta_luma_log2_weight_denom :
    READ_UVLC ( uiLog2WeightDenomLuma, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom
    if( bChroma ) 
    {
      READ_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );      // ue(v): delta_chroma_log2_weight_denom
      assert((iDeltaDenom + (Int)uiLog2WeightDenomLuma)>=0);
      uiLog2WeightDenomChroma = (UInt)(iDeltaDenom + uiLog2WeightDenomLuma);
    }

    for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(REF_PIC_LIST_C) ; iRefIdx++ ) 
    {
      pcSlice->getWpScalingLC(iRefIdx, wp);

      wp[0].uiLog2WeightDenom = uiLog2WeightDenomLuma;
      wp[1].uiLog2WeightDenom = uiLog2WeightDenomChroma;
      wp[2].uiLog2WeightDenom = uiLog2WeightDenomChroma;

      UInt  uiCode;
      READ_FLAG( uiCode, "luma_weight_lc_flag" );                  // u(1): luma_weight_lc_flag
      wp[0].bPresentFlag = ( uiCode == 1 );
      if ( wp[0].bPresentFlag ) 
      {
        Int iDeltaWeight;
        READ_SVLC( iDeltaWeight, "delta_luma_weight_lc" );          // se(v): delta_luma_weight_lc
        wp[0].iWeight = (iDeltaWeight + (1<<wp[0].uiLog2WeightDenom));
        READ_SVLC( wp[0].iOffset, "luma_offset_lc" );               // se(v): luma_offset_lc
      }
      else 
      {
        wp[0].iWeight = (1 << wp[0].uiLog2WeightDenom);
        wp[0].iOffset = 0;
      }
      if ( bChroma ) 
      {
        READ_FLAG( uiCode, "chroma_weight_lc_flag" );                // u(1): chroma_weight_lc_flag
        wp[1].bPresentFlag = ( uiCode == 1 );
        wp[2].bPresentFlag = ( uiCode == 1 );
        if ( wp[1].bPresentFlag ) 
        {
          for ( Int j=1 ; j<3 ; j++ ) 
          {
            Int iDeltaWeight;
            READ_SVLC( iDeltaWeight, "delta_chroma_weight_lc" );      // se(v): delta_chroma_weight_lc
            wp[j].iWeight = (iDeltaWeight + (1<<wp[1].uiLog2WeightDenom));

            Int iDeltaChroma;
            READ_SVLC( iDeltaChroma, "delta_chroma_offset_lc" );      // se(v): delta_chroma_offset_lc
            wp[j].iOffset = iDeltaChroma - ( ( (g_uiIBDI_MAX>>1)*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) + (g_uiIBDI_MAX>>1);
          }
        }
        else 
        {
          for ( Int j=1 ; j<3 ; j++ ) 
          {
            wp[j].iWeight = (1 << wp[j].uiLog2WeightDenom);
            wp[j].iOffset = 0;
          }
        }
      }
    }

    for ( Int iRefIdx=pcSlice->getNumRefIdx(REF_PIC_LIST_C) ; iRefIdx<2*MAX_NUM_REF ; iRefIdx++ ) 
    {
      pcSlice->getWpScalingLC(iRefIdx, wp);

      wp[0].bPresentFlag = false;
      wp[1].bPresentFlag = false;
      wp[2].bPresentFlag = false;
    }
  }
  else
  {
    printf("\n wrong weight pred table syntax \n ");
    assert(0);
  }
#else
  else
  {
    assert(0);
  }
#endif
}

#if SCALING_LIST
/** decode quantization matrix
 * \param scalingList quantization matrix information
 */
Void TDecCavlc::parseScalingList(TComScalingList* scalingList)
{
  UInt  code, sizeId, listId;
  Int *dst=0;
  READ_FLAG( code, "use_default_scaling_list_flag" );
  scalingList->setUseDefaultOnlyFlag  ( code    );
  if(scalingList->getUseDefaultOnlyFlag() == false)
  {
      //for each size
    for(sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
      for(listId = 0; listId <  g_auiScalingListNum[sizeId]; listId++)
      {
        dst = scalingList->getScalingListAddress(sizeId, listId);
        READ_FLAG( code, "pred_mode_flag");
        scalingList->setPredMode (sizeId,listId,code);
        if(scalingList->getPredMode (sizeId,listId) == SCALING_LIST_PRED_COPY) // Matrix_Copy_Mode
        {
          READ_UVLC( code, "pred_matrix_id_delta");
          scalingList->setPredMatrixId (sizeId,listId,(UInt)((Int)(listId)-(code+1)));
          scalingList->xPredScalingListMatrix( scalingList, dst, sizeId, listId,sizeId, scalingList->getPredMatrixId (sizeId,listId));
        }
        else if(scalingList->getPredMode (sizeId,listId) == SCALING_LIST_PRED_DPCM)//DPCM mode
        {
          xDecodeDPCMScalingListMatrix(scalingList, dst, sizeId, listId);
        }
      }
    }
  }

  return;
}
/** read Code
 * \param scalingList  quantization matrix information
 * \param piBuf buffer of decoding data
 * \param uiSizeId size index
 * \param listId list index
 */
Void TDecCavlc::xReadScalingListCode(TComScalingList *scalingList, Int* buf, UInt sizeId, UInt listId)
{
  UInt i,size = g_scalingListSize[sizeId];
  UInt dataCounter = 0;
  for(i=0;i<size;i++)
  {
    READ_SVLC( buf[dataCounter], "delta_coef");
    dataCounter++;
  }
}
/** decode DPCM with quantization matrix
 * \param scalingList  quantization matrix information
 * \param piData decoded data
 * \param uiSizeId size index
 * \param listId list index
 */
Void TDecCavlc::xDecodeDPCMScalingListMatrix(TComScalingList *scalingList, Int* data, UInt sizeId, UInt listId)
{
  Int  buf[1024];
  Int  pcm[1024];
  Int  startValue   = SCALING_LIST_START_VALUE;

  xReadScalingListCode(scalingList, buf, sizeId, listId);
  //Inverse DPCM
  scalingList->xInvDPCM(buf, pcm, sizeId, startValue);
  //Inverse ZigZag scan
  scalingList->xInvZigZag(pcm, data, sizeId);
}
#endif

Void TDecCavlc::parseDFFlag(UInt& ruiVal, const Char *pSymbolName)
{
  READ_FLAG(ruiVal, pSymbolName);
}
Void TDecCavlc::parseDFSvlc(Int&  riVal, const Char *pSymbolName)
{
  READ_SVLC(riVal, pSymbolName);
}

//! \}
