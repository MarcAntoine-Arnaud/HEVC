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

/** \file     TAppEncCfg.h
    \brief    Handle encoder configuration parameters (header)
*/

#ifndef __TAPPENCCFG__
#define __TAPPENCCFG__

#include "TLibCommon/CommonDef.h"

#if G1002_RPS
#include "TLibEncoder/TEncCfg.h"
#include <sstream>
#endif
//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TAppEncCfg
{
protected:
  // file I/O
  char*     m_pchInputFile;                                   ///< source file name
  char*     m_pchBitstreamFile;                               ///< output bitstream file
  char*     m_pchReconFile;                                   ///< output reconstruction file
  
  // source specification
  Int       m_iFrameRate;                                     ///< source frame-rates (Hz)
  unsigned int m_FrameSkip;                                   ///< number of skipped frames from the beginning
  Int       m_iSourceWidth;                                   ///< source width in pixel
  Int       m_iSourceHeight;                                  ///< source height in pixel
  Int       m_iFrameToBeEncoded;                              ///< number of encoded frames
  Bool      m_bUsePAD;                                        ///< flag for using source padding
  Int       m_aiPad[2];                                       ///< number of padded pixels for width and height
  
  // coding structure
  Int       m_iIntraPeriod;                                   ///< period of I-slice (random access period)
  Int       m_iDecodingRefreshType;                           ///< random access type
  Int       m_iGOPSize;                                       ///< GOP size of hierarchical structure
#if G1002_RPS
  Int       m_iExtraRPSs;
  GOPEntry  m_pcGOPList[MAX_GOP];
  UInt      m_uiMaxNumberOfReorderPictures;                   ///< total number of reference pictures needed for decoding
  UInt      m_uiMaxNumberOfReferencePictures;                 ///< total number of reorder pictures
#else
  Int       m_iRateGOPSize;                                   ///< GOP size for QP variance
  Int       m_iNumOfReference;                                ///< total number of reference frames in P-slice
  Int       m_iNumOfReferenceB_L0;                            ///< total number of reference frames for reference list L0 in B-slice
  Int       m_iNumOfReferenceB_L1;                            ///< total number of reference frames for reference list L1 in B-slice
  Bool      m_bHierarchicalCoding;                            ///< flag for specifying hierarchical B structure
  Bool      m_bUseLDC;                                        ///< flag for using low-delay coding mode
  Bool      m_bUseNRF;                                        ///< flag for using non-referenced frame in hierarchical structure
  Bool      m_bUseGPB;                                        ///< flag for using generalized P & B structure
#endif
  Bool      m_bUseLComb;                                      ///< flag for using combined reference list for uni-prediction in B-slices (JCTVC-D421)
  Bool      m_bLCMod;                                         ///< flag for specifying whether the combined reference list for uni-prediction in B-slices is uploaded explicitly
#if DISABLE_4x4_INTER
  Bool      m_bDisInter4x4;
#endif
#if NSQT
  Bool      m_enableNSQT;                                     ///< flag for enabling NSQT
#endif
#if AMP
  Bool      m_enableAMP;
#endif
  // coding quality
  Double    m_fQP;                                            ///< QP value of key-picture (floating point)
  Int       m_iQP;                                            ///< QP value of key-picture (integer)
  Int       m_aiTLayerQPOffset[MAX_TLAYER];                   ///< QP offset corresponding to temporal layer depth
  char*     m_pchdQPFile;                                     ///< QP offset for each slice (initialized from external file)
  Int*      m_aidQP;                                          ///< array of slice QP values
  Int       m_iMaxDeltaQP;                                    ///< max. |delta QP|
  UInt      m_uiDeltaQpRD;                                    ///< dQP range for multi-pass slice QP optimization
  Int       m_iMaxCuDQPDepth;                                 ///< Max. depth for a minimum CuDQPSize (0:default)
#if QP_ADAPTATION
  Bool      m_bUseAdaptiveQP;                                 ///< Flag for enabling QP adaptation based on a psycho-visual model
  Int       m_iQPAdaptationRange;                             ///< dQP range by QP adaptation
#endif
  
  Bool      m_bTLayering;                                     ///< indicates whether temporal IDs are set based on the hierarchical coding structure
  Bool      m_abTLayerSwitchingFlag[MAX_TLAYER];              ///< temporal layer switching flags corresponding to each temporal layer

  // coding unit (CU) definition
  UInt      m_uiMaxCUWidth;                                   ///< max. CU width in pixel
  UInt      m_uiMaxCUHeight;                                  ///< max. CU height in pixel
  UInt      m_uiMaxCUDepth;                                   ///< max. CU depth
  
  // transfom unit (TU) definition
  UInt      m_uiQuadtreeTULog2MaxSize;
  UInt      m_uiQuadtreeTULog2MinSize;
  
  UInt      m_uiQuadtreeTUMaxDepthInter;
  UInt      m_uiQuadtreeTUMaxDepthIntra;
  
  // coding tools (bit-depth)
  UInt      m_uiInputBitDepth;                                ///< bit-depth of input file
  UInt      m_uiOutputBitDepth;                               ///< bit-depth of output file
  UInt      m_uiInternalBitDepth;                             ///< Internal bit-depth (BitDepth+BitIncrement)

  // coding tools (PCM bit-depth)
#if E192_SPS_PCM_BIT_DEPTH_SYNTAX
  Bool      m_bPCMInputBitDepthFlag;                          ///< 0: PCM bit-depth is internal bit-depth. 1: PCM bit-depth is input bit-depth.
  UInt      m_uiPCMBitDepthLuma;                              ///< PCM bit-depth for luma
#endif

#if SAO
  Bool      m_bUseSAO; 
#endif

  // coding tools (loop filter)
  Bool      m_bUseALF;                                        ///< flag for using adaptive loop filter
  Int       m_iALFEncodePassReduction;                        //!< ALF encoding pass, 0 = original 16-pass, 1 = 1-pass, 2 = 2-pass
  
#if G215_ALF_NUM_FILTER
  Int       m_iALFMaxNumberFilters;                           ///< ALF Max Number Filters in one picture
#endif

  Bool      m_bLoopFilterDisable;                             ///< flag for using deblocking filter
  Int       m_iLoopFilterAlphaC0Offset;                       ///< alpha offset for deblocking filter
  Int       m_iLoopFilterBetaOffset;                          ///< beta offset for deblocking filter
  
#if !DISABLE_CAVLC
  // coding tools (entropy coder)
  Int       m_iSymbolMode;                                    ///< entropy coder mode, 0 = VLC, 1 = CABAC
#endif
  
  // coding tools (inter - merge motion partitions)
  Bool      m_bUseMRG;                                        ///< SOPH: flag for using motion partition Merge Mode
  
  Bool      m_bUseLMChroma;                                  ///< JL: Chroma intra prediction based on luma signal

  // coding tools (PCM)
#if MAX_PCM_SIZE
  Bool      m_usePCM;                                         ///< flag for using IPCM
  UInt      m_pcmLog2MaxSize;                                 ///< log2 of maximum PCM block size
#endif
  UInt      m_uiPCMLog2MinSize;                               ///< log2 of minimum PCM block size
#if E192_SPS_PCM_FILTER_DISABLE_SYNTAX
  Bool      m_bPCMFilterDisableFlag;                          ///< PCM filter disable flag
#endif

  // coding tools (encoder-only parameters)
  Bool      m_bUseSBACRD;                                     ///< flag for using RD optimization based on SBAC
  Bool      m_bUseASR;                                        ///< flag for using adaptive motion search range
  Bool      m_bUseHADME;                                      ///< flag for using HAD in sub-pel ME
  Bool      m_bUseRDOQ;                                       ///< flag for using RD optimized quantization
#if !G1002_RPS
  Bool      m_bUseBQP;                                        ///< flag for using B-slice based QP assignment in low-delay hier. structure
#endif
  Int       m_iFastSearch;                                    ///< ME mode, 0 = full, 1 = diamond, 2 = PMVFAST
  Int       m_iSearchRange;                                   ///< ME search range
  Int       m_bipredSearchRange;                              ///< ME search range for bipred refinement
  Bool      m_bUseFastEnc;                                    ///< flag for using fast encoder setting
#if EARLY_CU_DETERMINATION
  Bool      m_bUseEarlyCU;                                    ///< flag for using Early CU setting
#endif  
#if CBF_FAST_MODE
  Bool      m_bUseCbfFastMode;                              ///< flag for using Cbf Fast PU Mode Decision
#endif  
  Int       m_iSliceMode;           ///< 0: Disable all Recon slice limits, 1 : Maximum number of largest coding units per slice, 2: Maximum number of bytes in a slice
  Int       m_iSliceArgument;       ///< If m_iSliceMode==1, m_iSliceArgument=max. # of largest coding units. If m_iSliceMode==2, m_iSliceArgument=max. # of bytes.
  Int       m_iEntropySliceMode;    ///< 0: Disable all entropy slice limits, 1 : Maximum number of largest coding units per slice, 2: Constraint based entropy slice
  Int       m_iEntropySliceArgument;///< If m_iEntropySliceMode==1, m_iEntropySliceArgument=max. # of largest coding units. If m_iEntropySliceMode==2, m_iEntropySliceArgument=max. # of bins.

#if FINE_GRANULARITY_SLICES
  Int       m_iSliceGranularity;///< 0: Slices always end at LCU borders. 1-3: slices may end at a depth of 1-3 below LCU level.
#endif
  Bool m_bLFCrossSliceBoundaryFlag;  ///< 0: Cross-slice-boundary in-loop filtering 1: non-cross-slice-boundary in-loop filtering
#if TILES
  Int       m_iColumnRowInfoPresent;
  Int       m_iUniformSpacingIdr;
  Int       m_iTileBoundaryIndependenceIdr;
  Int       m_iNumColumnsMinus1;
  char*     m_pchColumnWidth;
  Int       m_iNumRowsMinus1;
  char*     m_pchRowHeight;
#if TILES_DECODER
  Int       m_iTileLocationInSliceHeaderFlag; //< enable(1)/disable(0) transmitssion of tile location in slice header
  Int       m_iTileMarkerFlag;              //< enable(1)/disable(0) transmitssion of light weight tile marker
  Int       m_iMaxTileMarkerEntryPoints;    //< maximum number of tile markers allowed in a slice (controls degree of parallelism)
  Double    m_dMaxTileMarkerOffset;         //< Calculated offset. Light weight tile markers will be transmitted for TileIdx= Offset, 2*Offset, 3*Offset ... 
#endif
#endif

#if OL_USE_WPP
  Int       m_iWaveFrontSynchro; //< 0: no WPP. >= 1: WPP is enabled, the "Top right" from which inheritance occurs is this LCU offset in the line above the current.
  Int       m_iWaveFrontFlush; //< enable(1)/disable(0) the CABAC flush at the end of each line of LCUs.
  Int       m_iWaveFrontSubstreams; //< If iWaveFrontSynchro, this is the number of substreams per frame (dependent tiles) or per tile (independent tiles).
#endif

  Bool      m_bUseConstrainedIntraPred;                       ///< flag for using constrained intra prediction
  
  bool m_pictureDigestEnabled; ///< enable(1)/disable(0) md5 computation and SEI signalling

#if !G1002_RPS
#if REF_SETTING_FOR_LD
  Bool      m_bUseNewRefSetting;
#endif
#endif

  // weighted prediction
#if WEIGHT_PRED
  Bool      m_bUseWeightPred;                                 ///< Use of explicit Weighting Prediction for P_SLICE
  UInt      m_uiBiPredIdc;                                    ///< Use of Bi-Directional Weighting Prediction (B_SLICE): explicit(1) or implicit(2)
#endif

#if NO_TMVP_MARKING
  Bool      m_bEnableTMVP;
#endif

#if SCALING_LIST
  Int       m_useScalingListId;                               ///< using quantization matrix
  char*     m_scalingListFile;                                ///< quantization matrix file name
#endif
  // internal member functions
  Void  xSetGlobal      ();                                   ///< set global variables
  Void  xCheckParameter ();                                   ///< check validity of configuration values
  Void  xPrintParameter ();                                   ///< print configuration values
  Void  xPrintUsage     ();                                   ///< print usage
  
public:
  TAppEncCfg();
  virtual ~TAppEncCfg();
  
public:
  Void  create    ();                                         ///< create option handling class
  Void  destroy   ();                                         ///< destroy option handling class
  Bool  parseCfg  ( Int argc, Char* argv[] );                 ///< parse configuration file to fill member variables
  
};// END CLASS DEFINITION TAppEncCfg

//! \}

#endif // __TAPPENCCFG__

