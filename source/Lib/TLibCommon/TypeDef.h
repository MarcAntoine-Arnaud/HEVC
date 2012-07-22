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

/** \file     TypeDef.h
    \brief    Define basic types, new types and enumerations
*/

#ifndef _TYPEDEF__
#define _TYPEDEF__

//! \ingroup TLibCommon
//! \{

#define SLICE_HEADER_EXTENSION           1  ///< II0235: Slice header extension mechanism

#define TEMPORAL_ID_RESTRICTION          1  ///< I0600: Semantics of temporal_id changed
#define PREVREFPIC_DEFN                  1  ///< I0345: prevRefPic defined as previous reference picture that is at same or lower 
                                            ///< temporal layer.
#define CODE_POCLSBLT_FIXEDLEN           1  ///< I0342: code the LSB bits of LTRPs as u(v)

#define REMOVE_TILE_MARKERS              1
#define EXPLICITLY_SIGNAL_ENTRY_POINTS   1  ///< I0273: Explicitly signal all tiles/wavefront entry points

#define SLICE_TMVP_ENABLE                1  ///< I0420: Enable Temporal MVP for certain slices only
#define NO_MV_SCALING_IF_LONG_TERM_REF   1  ///< I0422: Disable MV scaling for long term reference
#define REMOVE_INTER_4X4                 1  ///< I0216: Removal of Inter 4x4 mode
#define PRED_QP_DERIVATION               1  ///< I0219: Changing cu_qp_delta parsing to enable CU-level processing 
#define NEW_NAL_UNIT_TYPES               1  ///< I0607: Definition of new NAL unit types
#if NEW_NAL_UNIT_TYPES
#define CRA_BLA_TFD_MODIFICATIONS        1  ///< I0404, I0278, I0275: Modifications for CRA, BLA and TFD pictures
#endif

#define TILE_ENTRY_START                 1  ///< I0357: Signal tile entry points from the end of the slice header
#define WPP_SUBSTREAM_PER_ROW            1  ///< I0360: mandate one substream per CTB row
#define WPP_SLICE_END_CONSTRAINT         1  ///< I0361: WPP Slice constraint
#define REMOVE_IMPLICIT_WP               1  ///< I0589: remove implicit weighted prediction
#define HASH_TYPE                        1  ///< I0218: changes to the Hash SEI calculation for improved usability
#define BYTE_ALIGNMENT                   0  ///< I0330: Add byte_alignment() procedure to end of slice header
#define TEMPORALNESTINGFLAG_TLA          1  ///< I0330: Mark picture as TLA when temporal_nesting flag is 1 and when temporal_id greater than 0
#define SLICE_ADDRESS_FIX                1  ///< I0113: Slice address parsing fix
#define TILES_OR_ENTROPY_FIX             1  ///< I0113: tiles_or_entropy_coding_sync_idc parsing fix
#define REMOVE_LASTTU_CBFDERIV           1  ///< I0152: CBF coding for last TU without derivation process 

#define INTRA_TRANSFORMSKIP              1  ///< I0408: intra transform skipping (TS) 

#define AHG6_ALF_OPTION2                 1  ///< I0157: AHG6 ALF baseline Option 2 RA- Variant 2
#define SLICE_TYPE_ORDER                 1  ///< I0500: ordering of slice types
#define ALF_COEFF_EXP_GOLOMB_K           1  ///< I0346: Use EG(k) to code ALF coefficients

#define POS_BASED_SIG_COEFF_CTX          1  ///< I0296: position based context index derivation for significant coeff flag for large TU

#define SBH_THRESHOLD                    4  ///< I0156: value of the fixed SBH controlling threshold
  
#define LOSSLESS_CODING                   1  ///< H0530: lossless and lossy (mixed) coding
#if LOSSLESS_CODING
#define SEQUENCE_LEVEL_LOSSLESS           0  ///< H0530: used only for sequence or frame-level lossless coding
#define CU_LEVEL_TRANSQUANT_BYPASS        1  ///< I0529: CU level flag for transquant bypass
#define IPCM_LOSSLESS_LOOP_FILTERING_UNIFICATION  1  ///< I0586: I_PCM/lossless loop filtering unification
#endif

#define DISABLING_CLIP_FOR_BIPREDME         1  ///< Ticket #175
  
#if !POS_BASED_SIG_COEFF_CTX
#define SIGMAP_CONST_AT_HIGH_FREQUENCY      1      ///< H0095 method2.1: const significance map at high freaquency
#endif

#define C1FLAG_NUMBER               8 // maximum number of largerThan1 flag coded in one chunk :  16 in HM5
#define C2FLAG_NUMBER               1 // maximum number of largerThan2 flag coded in one chunk:  16 in HM5 

#define REMOVE_SAO_LCU_ENC_CONSTRAINTS_1 1  ///< disable the encoder constraint that does not test SAO/BO mode for chroma in interleaved mode
#define REMOVE_SAO_LCU_ENC_CONSTRAINTS_2 1  ///< disable the encoder constraint that reduce the range of SAO/EO for chroma in interleaved mode
#define REMOVE_SAO_LCU_ENC_CONSTRAINTS_3 1  ///< disable the encoder constraint that conditionally disable SAO for chroma for entire slice in interleaved mode

#define SAO_SKIP_RIGHT                   1  ///< H1101: disallow using unavailable pixel during RDO

#define SAO_REMOVE_APS                   1  ///< USNB37: remove SAO APS syntax

#define SAO_NO_MERGE_CROSS_SLICE_TILE    1  ///< I0172: disallow merging across slice or tile boundaries

#define SAO_OFFSET_MAG_SIGN_SPLIT        1  ///< I0168: SAO offset magnitudes first, signs only for nonzero magnitudes, signs are bypass coded
#if SAO_OFFSET_MAG_SIGN_SPLIT
#define SAO_TRUNCATED_U                  1  ///< I0066: SAO offset magnitude using truncated unary binarization
#endif

#define SAO_ENCODING_CHOICE              1  ///< I0184: picture early termination
#if SAO_ENCODING_CHOICE
#define SAO_ENCODING_RATE                0.75
#endif

#define SAO_RDO_FIX                      1  ///< I0563: SAO RDO bug-fix
#if SAO_OFFSET_MAG_SIGN_SPLIT && SAO_OFFSET_MAG_SIGN_SPLIT && SAO_REMOVE_APS && SAO_TRUNCATED_U
#define SAO_CODE_CLEAN_UP                1 /// redundant code removal
#endif

#define MAX_NUM_SPS                32
#define MAX_NUM_PPS                256
#define MAX_NUM_APS                32         //< !!!KS: number not defined in WD yet

#define MRG_MAX_NUM_CANDS_SIGNALED         5   //<G091: value of maxNumMergeCand signaled in slice header 

#define WEIGHTED_CHROMA_DISTORTION  1   ///< F386: weighting of chroma for RDO
#define RDOQ_CHROMA_LAMBDA          1   ///< F386: weighting of chroma for RDOQ
#define ALF_CHROMA_LAMBDA           1   ///< F386: weighting of chroma for ALF
#define SAO_CHROMA_LAMBDA           1   ///< F386: weighting of chroma for SAO

#define MIN_SCAN_POS_CROSS          4

#define FAST_BIT_EST                1   ///< G763: Table-based bit estimation for CABAC

#define MLS_GRP_NUM                         64     ///< G644 : Max number of coefficient groups, max(16, 64)
#define MLS_CG_SIZE                         4      ///< G644 : Coefficient group size of 4x4

#define ADAPTIVE_QP_SELECTION               1      ///< G382: Adaptive reconstruction levels, non-normative part for adaptive QP selection
#if ADAPTIVE_QP_SELECTION
#define ARL_C_PRECISION                     7      ///< G382: 7-bit arithmetic precision
#define LEVEL_RANGE                         30     ///< G382: max coefficient level in statistics collection
#endif

#define NS_HAD                               1

#define APS_BITS_FOR_SAO_BYTE_LENGTH 12           
#define APS_BITS_FOR_ALF_BYTE_LENGTH 8

#define HHI_RQT_INTRA_SPEEDUP             1           ///< tests one best mode with full rqt
#define HHI_RQT_INTRA_SPEEDUP_MOD         0           ///< tests two best modes with full rqt

#if HHI_RQT_INTRA_SPEEDUP_MOD && !HHI_RQT_INTRA_SPEEDUP
#error
#endif

#define VERBOSE_RATE 0 ///< Print additional rate information in encoder

#define AMVP_DECIMATION_FACTOR            4

#define SCAN_SET_SIZE                     16
#define LOG2_SCAN_SET_SIZE                4

#define FAST_UDI_MAX_RDMODE_NUM               35          ///< maximum number of RD comparison in fast-UDI estimation loop 

#define ZERO_MVD_EST                          0           ///< Zero Mvd Estimation in normal mode

#define NUM_INTRA_MODE 36
#define LM_CHROMA_IDX  35

#define IBDI_DISTORTION                0           ///< enable/disable SSE modification when IBDI is used (JCTVC-D152)
#define FIXED_ROUNDING_FRAME_MEMORY    0           ///< enable/disable fixed rounding to 8-bitdepth of frame memory when IBDI is used  

#define WRITE_BACK                      1           ///< Enable/disable the encoder to replace the deltaPOC and Used by current from the config file with the values derived by the refIdc parameter.
#define AUTO_INTER_RPS                  1           ///< Enable/disable the automatic generation of refIdc from the deltaPOC and Used by current from the config file.
#define PRINT_RPS_INFO                  0           ///< Enable/disable the printing of bits used to send the RPS.
                                                    // using one nearest frame as reference frame, and the other frames are high quality (POC%4==0) frames (1+X)
                                                    // this should be done with encoder only decision
                                                    // but because of the absence of reference frame management, the related code was hard coded currently

#define RVM_VCEGAM10_M 4

#define PLANAR_IDX             0
#define VER_IDX                26                    // index for intra VERTICAL   mode
#define HOR_IDX                10                    // index for intra HORIZONTAL mode
#define DC_IDX                 1                     // index for intra DC mode
#define NUM_CHROMA_MODE        6                     // total number of chroma modes
#define DM_CHROMA_IDX          36                    // chroma mode index for derived from luma intra mode


#define FAST_UDI_USE_MPM 1

#define RDO_WITHOUT_DQP_BITS              0           ///< Disable counting dQP bits in RDO-based mode decision

#define FULL_NBIT 0 ///< When enabled, does not use g_uiBitIncrement anymore to support > 8 bit data

#define AD_HOC_SLICES_FIXED_NUMBER_OF_LCU_IN_SLICE      1          ///< OPTION IDENTIFIER. mode==1 -> Limit maximum number of largest coding tree blocks in a slice
#define AD_HOC_SLICES_FIXED_NUMBER_OF_BYTES_IN_SLICE    2          ///< OPTION IDENTIFIER. mode==2 -> Limit maximum number of bins/bits in a slice
#define AD_HOC_SLICES_FIXED_NUMBER_OF_TILES_IN_SLICE    3

#define DEPENDENT_SLICES       1 ///< JCTVC-I0229
// Dependent slice options
#define SHARP_FIXED_NUMBER_OF_LCU_IN_DEPENDENT_SLICE            1          ///< OPTION IDENTIFIER. Limit maximum number of largest coding tree blocks in an dependent slice
#define SHARP_MULTIPLE_CONSTRAINT_BASED_DEPENDENT_SLICE         2          ///< OPTION IDENTIFIER. Limit maximum number of bins/bits in an dependent slice
#if DEPENDENT_SLICES
#define FIXED_NUMBER_OF_TILES_IN_DEPENDENT_SLICE          3 // JCTVC-I0229
#endif

#define LOG2_MAX_NUM_COLUMNS_MINUS1        7
#define LOG2_MAX_NUM_ROWS_MINUS1           7
#define LOG2_MAX_COLUMN_WIDTH              13
#define LOG2_MAX_ROW_HEIGHT                13

#if !REMOVE_TILE_MARKERS
#define MAX_MARKER_PER_NALU                 1000
#endif

#define MATRIX_MULT                             0   // Brute force matrix multiplication instead of partial butterfly

#define REG_DCT 65535

#define AMP_SAD                               1           ///< dedicated SAD functions for AMP
#define AMP_ENC_SPEEDUP                       1           ///< encoder only speed-up by AMP mode skipping
#if AMP_ENC_SPEEDUP
#define AMP_MRG                               1           ///< encoder only force merge for AMP partition (no motion search for AMP)
#endif

#define SCALING_LIST_OUTPUT_RESULT    0 //JCTVC-G880/JCTVC-G1016 quantization matrices
#define SCALING_LIST_DPCM_IMPROVEMENT   1 //JCTVC-I0059. Improved DPCM coding for 16x16 and 32x32 matrices
#define SCALING_LIST_SIMPLYFY         1 //JCTVC-I0059/I0101. Simplification of Default matrices coding
#define SCALING_LIST_HL_SYNTAX        1 //JCTVC-I0465. Signalling of quantization matrices in SPS and PPS

#define CABAC_INIT_PRESENT_FLAG     1

#define DEBLOCK_IPCM_RECY             1 // JCTVC-I0035 scheme 1: reconstructed QP for IPCM deblocking
#define DBL_HL_SYNTAX                 1 // Signalling of deblocking filter in PPS and Slice header instead of in APS

#define REMOVE_LC  1 // JCTVC-I0125
#define CU_QP_DELTA_DEPTH_SYN  1 // JCTVC-I0127, differential coding of max cu qp delta depth

#define H0391_LF_ACROSS_SLICE_BOUNDARY_CONTROL 1 ///< H0391: Loop filtering across/non-across slice boundaries control

#define H0226                       1 // JCTVC-H0226
// ====================================================================================================================
// VPS constants
// ====================================================================================================================
#define VPS_INTEGRATION             1 // VPS 
#if VPS_INTEGRATION
#define MAX_LAYER_NUM                     10
#define MAX_NUM_VPS                16
#endif

// ====================================================================================================================
// Basic type redefinition
// ====================================================================================================================

typedef       void                Void;
typedef       bool                Bool;

typedef       char                Char;
typedef       unsigned char       UChar;
typedef       short               Short;
typedef       unsigned short      UShort;
typedef       int                 Int;
typedef       unsigned int        UInt;
typedef       double              Double;

// ====================================================================================================================
// 64-bit integer type
// ====================================================================================================================

#ifdef _MSC_VER
typedef       __int64             Int64;

#if _MSC_VER <= 1200 // MS VC6
typedef       __int64             UInt64;   // MS VC6 does not support unsigned __int64 to double conversion
#else
typedef       unsigned __int64    UInt64;
#endif

#else

typedef       long long           Int64;
typedef       unsigned long long  UInt64;

#endif

// ====================================================================================================================
// Type definition
// ====================================================================================================================

typedef       UChar           Pxl;        ///< 8-bit pixel type
typedef       Short           Pel;        ///< 16-bit pixel type
typedef       Int             TCoeff;     ///< transform coefficient

/// parameters for adaptive loop filter
class TComPicSym;

#define NUM_DOWN_PART 4

enum SAOTypeLen
{
  SAO_EO_LEN    = 4, 
  SAO_BO_LEN    = 4,
  SAO_MAX_BO_CLASSES = 32
};

enum SAOType
{
  SAO_EO_0 = 0, 
  SAO_EO_1,
  SAO_EO_2, 
  SAO_EO_3,
  SAO_BO,
  MAX_NUM_SAO_TYPE
};

typedef struct _SaoQTPart
{
  Int         iBestType;
  Int         iLength;
  Int         bandPosition ;
  Int         iOffset[4];
  Int         StartCUX;
  Int         StartCUY;
  Int         EndCUX;
  Int         EndCUY;

  Int         PartIdx;
  Int         PartLevel;
  Int         PartCol;
  Int         PartRow;

  Int         DownPartsIdx[NUM_DOWN_PART];
  Int         UpPartIdx;

  Bool        bSplit;

  //---- encoder only start -----//
  Bool        bProcessed;
  Double      dMinCost;
  Int64       iMinDist;
  Int         iMinRate;
  //---- encoder only end -----//
} SAOQTPart;

typedef struct _SaoLcuParam
{
  Bool       mergeUpFlag;
  Bool       mergeLeftFlag;
  Int        typeIdx;
  Int        bandPosition;
  Int        offset[4];
#if !SAO_CODE_CLEAN_UP
  Int        runDiff;
  Int        run;
#endif
  Int        partIdx;
  Int        partIdxTmp;
  Int        length;
} SaoLcuParam;

struct SAOParam
{
  Bool       bSaoFlag[3];
  SAOQTPart* psSaoPart[3];
  Int        iMaxSplitLevel;
  Int        iNumClass[MAX_NUM_SAO_TYPE];
  Bool         oneUnitFlag[3];
  SaoLcuParam* saoLcuParam[3];
  Int          numCuInHeight;
  Int          numCuInWidth;
  ~SAOParam();
};

struct ALFParam
{
  Int alf_flag;                           ///< indicates use of ALF
  Int num_coeff;                          ///< number of filter coefficients
  Int filter_shape;
  Int *filterPattern;
  Int startSecondFilter;
  Int filters_per_group;
#if !AHG6_ALF_OPTION2
  Int predMethod;
  Int *nbSPred;
#endif
  Int **coeffmulti;
#if !AHG6_ALF_OPTION2
  Int minKStart;
#endif
  Int componentID;
#if !AHG6_ALF_OPTION2
  Int* kMinTab;
#endif
  //constructor, operator
  ALFParam():componentID(-1){}
  ALFParam(Int cID){create(cID);}
  ALFParam(const ALFParam& src) {*this = src;}
  ~ALFParam(){destroy();}
  const ALFParam& operator= (const ALFParam& src);
private:
  Void create(Int cID);
  Void destroy();
  Void copy(const ALFParam& src);
};
#if !AHG6_ALF_OPTION2
struct AlfUnitParam
{
  Int   mergeType;
  Bool  isEnabled;
  Bool  isNewFilt;
  Int   storedFiltIdx;
  ALFParam* alfFiltParam;
  //constructor, operator 
  AlfUnitParam();
  AlfUnitParam(const AlfUnitParam& src){ *this = src;}
  const AlfUnitParam& operator= (const AlfUnitParam& src);
  Bool operator == (const AlfUnitParam& cmp);
};

struct AlfParamSet
{
  Bool isEnabled[3];
  Bool isUniParam[3];
  Int  numLCUInWidth;
  Int  numLCUInHeight;
  Int  numLCU;
  AlfUnitParam* alfUnitParam[3];
  //constructor, operator 
  AlfParamSet(){create();}
  ~AlfParamSet(){destroy();}
  Void create(Int width =0, Int height=0, Int num=0);
  Void init();
  Void releaseALFParam();
  Void createALFParam();
private:
  Void destroy();
};
#endif

/// parameters for deblocking filter
typedef struct _LFCUParam
{
  Bool bInternalEdge;                     ///< indicates internal edge
  Bool bLeftEdge;                         ///< indicates left edge
  Bool bTopEdge;                          ///< indicates top edge
} LFCUParam;

// ====================================================================================================================
// Enumeration
// ====================================================================================================================

/// supported slice type
#if SLICE_TYPE_ORDER
enum SliceType
{
  B_SLICE,
  P_SLICE,
  I_SLICE
};
#else
enum SliceType
{
  I_SLICE,
  P_SLICE,
  B_SLICE
};
#endif

/// chroma formats (according to semantics of chroma_format_idc)
enum ChromaFormat
{
  CHROMA_400  = 0,
  CHROMA_420  = 1,
  CHROMA_422  = 2,
  CHROMA_444  = 3
};

/// supported partition shape
enum PartSize
{
  SIZE_2Nx2N,           ///< symmetric motion partition,  2Nx2N
  SIZE_2NxN,            ///< symmetric motion partition,  2Nx N
  SIZE_Nx2N,            ///< symmetric motion partition,   Nx2N
  SIZE_NxN,             ///< symmetric motion partition,   Nx N
  SIZE_2NxnU,           ///< asymmetric motion partition, 2Nx( N/2) + 2Nx(3N/2)
  SIZE_2NxnD,           ///< asymmetric motion partition, 2Nx(3N/2) + 2Nx( N/2)
  SIZE_nLx2N,           ///< asymmetric motion partition, ( N/2)x2N + (3N/2)x2N
  SIZE_nRx2N,           ///< asymmetric motion partition, (3N/2)x2N + ( N/2)x2N
  SIZE_NONE = 15
};

/// supported prediction type
enum PredMode
{
  MODE_INTER,           ///< inter-prediction mode
  MODE_INTRA,           ///< intra-prediction mode
  MODE_NONE = 15
};

/// texture component type
enum TextType
{
  TEXT_LUMA,            ///< luma
  TEXT_CHROMA,          ///< chroma (U+V)
  TEXT_CHROMA_U,        ///< chroma U
  TEXT_CHROMA_V,        ///< chroma V
  TEXT_ALL,             ///< Y+U+V
  TEXT_NONE = 15
};

/// reference list index
enum RefPicList
{
  REF_PIC_LIST_0 = 0,   ///< reference list 0
  REF_PIC_LIST_1 = 1,   ///< reference list 1
  REF_PIC_LIST_C = 2,   ///< combined reference list for uni-prediction in B-Slices
  REF_PIC_LIST_X = 100  ///< special mark
};

/// distortion function index
enum DFunc
{
  DF_DEFAULT  = 0,
  DF_SSE      = 1,      ///< general size SSE
  DF_SSE4     = 2,      ///<   4xM SSE
  DF_SSE8     = 3,      ///<   8xM SSE
  DF_SSE16    = 4,      ///<  16xM SSE
  DF_SSE32    = 5,      ///<  32xM SSE
  DF_SSE64    = 6,      ///<  64xM SSE
  DF_SSE16N   = 7,      ///< 16NxM SSE
  
  DF_SAD      = 8,      ///< general size SAD
  DF_SAD4     = 9,      ///<   4xM SAD
  DF_SAD8     = 10,     ///<   8xM SAD
  DF_SAD16    = 11,     ///<  16xM SAD
  DF_SAD32    = 12,     ///<  32xM SAD
  DF_SAD64    = 13,     ///<  64xM SAD
  DF_SAD16N   = 14,     ///< 16NxM SAD
  
  DF_SADS     = 15,     ///< general size SAD with step
  DF_SADS4    = 16,     ///<   4xM SAD with step
  DF_SADS8    = 17,     ///<   8xM SAD with step
  DF_SADS16   = 18,     ///<  16xM SAD with step
  DF_SADS32   = 19,     ///<  32xM SAD with step
  DF_SADS64   = 20,     ///<  64xM SAD with step
  DF_SADS16N  = 21,     ///< 16NxM SAD with step
  
  DF_HADS     = 22,     ///< general size Hadamard with step
  DF_HADS4    = 23,     ///<   4xM HAD with step
  DF_HADS8    = 24,     ///<   8xM HAD with step
  DF_HADS16   = 25,     ///<  16xM HAD with step
  DF_HADS32   = 26,     ///<  32xM HAD with step
  DF_HADS64   = 27,     ///<  64xM HAD with step
  DF_HADS16N  = 28,     ///< 16NxM HAD with step
  
#if AMP_SAD
  DF_SAD12    = 43,
  DF_SAD24    = 44,
  DF_SAD48    = 45,

  DF_SADS12   = 46,
  DF_SADS24   = 47,
  DF_SADS48   = 48,

  DF_SSE_FRAME = 50     ///< Frame-based SSE
#else
  DF_SSE_FRAME = 33     ///< Frame-based SSE
#endif
};

/// index for SBAC based RD optimization
enum CI_IDX
{
  CI_CURR_BEST = 0,     ///< best mode index
  CI_NEXT_BEST,         ///< next best index
  CI_TEMP_BEST,         ///< temporal index
  CI_CHROMA_INTRA,      ///< chroma intra index
  CI_QT_TRAFO_TEST,
  CI_QT_TRAFO_ROOT,
  CI_NUM,               ///< total number
};

/// motion vector predictor direction used in AMVP
enum MVP_DIR
{
  MD_LEFT = 0,          ///< MVP of left block
  MD_ABOVE,             ///< MVP of above block
  MD_ABOVE_RIGHT,       ///< MVP of above right block
  MD_BELOW_LEFT,        ///< MVP of below left block
  MD_ABOVE_LEFT         ///< MVP of above left block
};

/// motion vector prediction mode used in AMVP
enum AMVP_MODE
{
  AM_NONE = 0,          ///< no AMVP mode
  AM_EXPL,              ///< explicit signalling of motion vector index
};

/// coefficient scanning type used in ACS
enum COEFF_SCAN_TYPE
{
  SCAN_ZIGZAG = 0,      ///< typical zigzag scan
  SCAN_HOR,             ///< horizontal first scan
  SCAN_VER,              ///< vertical first scan
  SCAN_DIAG              ///< up-right diagonal scan
};

//! \}

#endif
