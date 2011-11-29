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

/** \file     TComSlice.h
    \brief    slice header and SPS class (header)
*/

#ifndef __TCOMSLICE__
#define __TCOMSLICE__

#include <cstring>
#include "CommonDef.h"
#include "TComRom.h"
#include "TComList.h"

//! \ingroup TLibCommon
//! \{

class TComPic;


// ====================================================================================================================
// Constants
// ====================================================================================================================

#if F747_APS
/// max number of supported APS in software
#define MAX_NUM_SUPPORTED_APS 1
#endif

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// SPS class
class TComSPS
{
private:
  Int         m_SPSId;
  Int         m_ProfileIdc;
  Int         m_LevelIdc;

  UInt        m_uiMaxTLayers;           // maximum number of temporal layers

  // Structure
  UInt        m_uiWidth;
  UInt        m_uiHeight;
  Int         m_aiPad[2];
  UInt        m_uiMaxCUWidth;
  UInt        m_uiMaxCUHeight;
  UInt        m_uiMaxCUDepth;
  UInt        m_uiMinTrDepth;
  UInt        m_uiMaxTrDepth;
  
  // Tool list
  UInt        m_uiQuadtreeTULog2MaxSize;
  UInt        m_uiQuadtreeTULog2MinSize;
  UInt        m_uiQuadtreeTUMaxDepthInter;
  UInt        m_uiQuadtreeTUMaxDepthIntra;
  UInt        m_uiPCMLog2MinSize;
#if DISABLE_4x4_INTER
  Bool        m_bDisInter4x4;
#endif    
  Bool        m_bUseALF;
  Bool        m_bUseDQP;
  Bool        m_bUseLDC;
  Bool        m_bUsePAD;
  Bool        m_bUseMRG; // SOPH:

  Bool        m_bUseLMChroma; // JL:

  Bool        m_bUseLComb;
  Bool        m_bLCMod;
  
  // Parameter
  AMVP_MODE   m_aeAMVPMode[MAX_CU_DEPTH];
  UInt        m_uiBitDepth;
  UInt        m_uiBitIncrement;

#if E192_SPS_PCM_BIT_DEPTH_SYNTAX
  UInt        m_uiPCMBitDepthLuma;
  UInt        m_uiPCMBitDepthChroma;
#endif
#if E192_SPS_PCM_FILTER_DISABLE_SYNTAX
  Bool        m_bPCMFilterDisableFlag;
#endif

  // Max physical transform size
  UInt        m_uiMaxTrSize;
  
  Int m_iAMPAcc[MAX_CU_DEPTH];

  Bool        m_bLFCrossSliceBoundaryFlag;
#if SAO
  Bool        m_bUseSAO; 
#endif

#if TILES
  Int      m_iUniformSpacingIdr;
  Int      m_iTileBoundaryIndependenceIdr;
  Int      m_iNumColumnsMinus1;
  UInt*    m_puiColumnWidth;
  Int      m_iNumRowsMinus1;
  UInt*    m_puiRowHeight;
#endif
  
  Bool        m_bTemporalIdNestingFlag; // temporal_id_nesting_flag

#if REF_SETTING_FOR_LD
  Bool        m_bUseNewRefSetting;
  UInt        m_uiMaxNumRefFrames;
#endif

public:
  TComSPS();
  virtual ~TComSPS();

  Int  getSPSId       ()         { return m_SPSId;          }
  Void setSPSId       (Int i)    { m_SPSId = i;             }
  Int  getProfileIdc  ()         { return m_ProfileIdc;     }
  Void setProfileIdc  (Int i)    { m_ProfileIdc = i;        }
  Int  getLevelIdc    ()         { return m_LevelIdc;       }
  Void setLevelIdc    (Int i)    { m_LevelIdc = i;          }
  
  // structure
  Void setWidth       ( UInt u ) { m_uiWidth = u;           }
  UInt getWidth       ()         { return  m_uiWidth;       }
  Void setHeight      ( UInt u ) { m_uiHeight = u;          }
  UInt getHeight      ()         { return  m_uiHeight;      }
  Void setMaxCUWidth  ( UInt u ) { m_uiMaxCUWidth = u;      }
  UInt getMaxCUWidth  ()         { return  m_uiMaxCUWidth;  }
  Void setMaxCUHeight ( UInt u ) { m_uiMaxCUHeight = u;     }
  UInt getMaxCUHeight ()         { return  m_uiMaxCUHeight; }
  Void setMaxCUDepth  ( UInt u ) { m_uiMaxCUDepth = u;      }
  UInt getMaxCUDepth  ()         { return  m_uiMaxCUDepth;  }
  Void setPCMLog2MinSize  ( UInt u ) { m_uiPCMLog2MinSize = u;      }
  UInt getPCMLog2MinSize  ()         { return  m_uiPCMLog2MinSize;  }
#if DISABLE_4x4_INTER
  Bool getDisInter4x4()         { return m_bDisInter4x4;        }
  Void setDisInter4x4      ( Bool b ) { m_bDisInter4x4  = b;          }
#endif
  Void setMinTrDepth  ( UInt u ) { m_uiMinTrDepth = u;      }
  UInt getMinTrDepth  ()         { return  m_uiMinTrDepth;  }
  Void setMaxTrDepth  ( UInt u ) { m_uiMaxTrDepth = u;      }
  UInt getMaxTrDepth  ()         { return  m_uiMaxTrDepth;  }
  Void setQuadtreeTULog2MaxSize( UInt u ) { m_uiQuadtreeTULog2MaxSize = u;    }
  UInt getQuadtreeTULog2MaxSize()         { return m_uiQuadtreeTULog2MaxSize; }
  Void setQuadtreeTULog2MinSize( UInt u ) { m_uiQuadtreeTULog2MinSize = u;    }
  UInt getQuadtreeTULog2MinSize()         { return m_uiQuadtreeTULog2MinSize; }
  Void setQuadtreeTUMaxDepthInter( UInt u ) { m_uiQuadtreeTUMaxDepthInter = u;    }
  Void setQuadtreeTUMaxDepthIntra( UInt u ) { m_uiQuadtreeTUMaxDepthIntra = u;    }
  UInt getQuadtreeTUMaxDepthInter()         { return m_uiQuadtreeTUMaxDepthInter; }
  UInt getQuadtreeTUMaxDepthIntra()         { return m_uiQuadtreeTUMaxDepthIntra; }
  Void setPad         (Int iPad[2]) { m_aiPad[0] = iPad[0]; m_aiPad[1] = iPad[1]; }
  Void setPadX        ( Int  u ) { m_aiPad[0] = u; }
  Void setPadY        ( Int  u ) { m_aiPad[1] = u; }
  Int  getPad         ( Int  u ) { assert(u < 2); return m_aiPad[u];}
  Int* getPad         ( )        { return m_aiPad; }
  
  // physical transform
  Void setMaxTrSize   ( UInt u ) { m_uiMaxTrSize = u;       }
  UInt getMaxTrSize   ()         { return  m_uiMaxTrSize;   }
  
  // Tool list
  Bool getUseALF      ()         { return m_bUseALF;        }
  Bool getUseDQP      ()         { return m_bUseDQP;        }
  
  Bool getUseLDC      ()         { return m_bUseLDC;        }
  Bool getUsePAD      ()         { return m_bUsePAD;        }
  Bool getUseMRG      ()         { return m_bUseMRG;        } // SOPH:
  
  Void setUseALF      ( Bool b ) { m_bUseALF  = b;          }
  Void setUseDQP      ( Bool b ) { m_bUseDQP   = b;         }
  
  Void setUseLDC      ( Bool b ) { m_bUseLDC   = b;         }
  Void setUsePAD      ( Bool b ) { m_bUsePAD   = b;         }
  Void setUseMRG      ( Bool b ) { m_bUseMRG  = b;          } // SOPH:
  
  Void setUseLComb    (Bool b)   { m_bUseLComb = b;         }
  Bool getUseLComb    ()         { return m_bUseLComb;      }
  Void setLCMod       (Bool b)   { m_bLCMod = b;     }
  Bool getLCMod       ()         { return m_bLCMod;  }

  Bool getUseLMChroma ()         { return m_bUseLMChroma;        }
  Void setUseLMChroma ( Bool b ) { m_bUseLMChroma  = b;          }

  // AMVP mode (for each depth)
  AMVP_MODE getAMVPMode ( UInt uiDepth ) { assert(uiDepth < g_uiMaxCUDepth);  return m_aeAMVPMode[uiDepth]; }
  Void      setAMVPMode ( UInt uiDepth, AMVP_MODE eMode) { assert(uiDepth < g_uiMaxCUDepth);  m_aeAMVPMode[uiDepth] = eMode; }
  
#if AMP
  // AMP accuracy
  Int       getAMPAcc   ( UInt uiDepth ) { return m_iAMPAcc[uiDepth]; }
  Void      setAMPAcc   ( UInt uiDepth, Int iAccu ) { assert( uiDepth < g_uiMaxCUDepth);  m_iAMPAcc[uiDepth] = iAccu; }
#endif  

  // Bit-depth
  UInt      getBitDepth     ()         { return m_uiBitDepth;     }
  Void      setBitDepth     ( UInt u ) { m_uiBitDepth = u;        }
  UInt      getBitIncrement ()         { return m_uiBitIncrement; }
  Void      setBitIncrement ( UInt u ) { m_uiBitIncrement = u;    }

  Void      setLFCrossSliceBoundaryFlag     ( Bool   bValue  )    { m_bLFCrossSliceBoundaryFlag = bValue; }
  Bool      getLFCrossSliceBoundaryFlag     ()                    { return m_bLFCrossSliceBoundaryFlag;   } 

#if SAO
  Void setUseSAO                  (Bool bVal)  {m_bUseSAO = bVal;}
  Bool getUseSAO                  ()           {return m_bUseSAO;}
#endif

  UInt      getMaxTLayers()                           { return m_uiMaxTLayers; }
  Void      setMaxTLayers( UInt uiMaxTLayers )        { assert( uiMaxTLayers <= MAX_TLAYER ); m_uiMaxTLayers = uiMaxTLayers; }

  Bool      getTemporalIdNestingFlag()                { return m_bTemporalIdNestingFlag; }
  Void      setTemporalIdNestingFlag( Bool bValue )   { m_bTemporalIdNestingFlag = bValue; }
#if E192_SPS_PCM_BIT_DEPTH_SYNTAX
  UInt      getPCMBitDepthLuma     ()         { return m_uiPCMBitDepthLuma;     }
  Void      setPCMBitDepthLuma     ( UInt u ) { m_uiPCMBitDepthLuma = u;        }
  UInt      getPCMBitDepthChroma   ()         { return m_uiPCMBitDepthChroma;   }
  Void      setPCMBitDepthChroma   ( UInt u ) { m_uiPCMBitDepthChroma = u;      }
#endif
#if E192_SPS_PCM_FILTER_DISABLE_SYNTAX
  Void      setPCMFilterDisableFlag     ( Bool   bValue  )    { m_bPCMFilterDisableFlag = bValue; }
  Bool      getPCMFilterDisableFlag     ()                    { return m_bPCMFilterDisableFlag;   } 
#endif

#if REF_SETTING_FOR_LD
  Void      setUseNewRefSetting    ( Bool b ) { m_bUseNewRefSetting = b;    }
  Bool      getUseNewRefSetting    ()         { return m_bUseNewRefSetting; }
  Void      setMaxNumRefFrames     ( UInt u ) { m_uiMaxNumRefFrames = u;    }
  UInt      getMaxNumRefFrames     ()         { return m_uiMaxNumRefFrames; }
#endif
#if TILES
  Void     setUniformSpacingIdr             ( Int i )           { m_iUniformSpacingIdr = i; }
  Int      getUniformSpacingIdr             ()                  { return m_iUniformSpacingIdr; }
  Void     setTileBoundaryIndependenceIdr   ( Int i )           { m_iTileBoundaryIndependenceIdr = i; }
  Int      getTileBoundaryIndependenceIdr   ()                  { return m_iTileBoundaryIndependenceIdr; }
  Void     setNumColumnsMinus1              ( Int i )           { m_iNumColumnsMinus1 = i; }
  Int      getNumColumnsMinus1              ()                  { return m_iNumColumnsMinus1; }
  Void     setColumnWidth ( UInt* columnWidth )
  {
    if( m_iUniformSpacingIdr == 0 && m_iNumColumnsMinus1 > 0 )
    {
      m_puiColumnWidth = new UInt[ m_iNumColumnsMinus1 ];

      for(Int i=0; i<m_iNumColumnsMinus1; i++)
      {
        m_puiColumnWidth[i] = columnWidth[i];
     }
    }
  }
  UInt     getColumnWidth  (UInt columnIdx) { return *( m_puiColumnWidth + columnIdx ); }
  Void     setNumRowsMinus1( Int i )        { m_iNumRowsMinus1 = i; }
  Int      getNumRowsMinus1()               { return m_iNumRowsMinus1; }
  Void     setRowHeight    ( UInt* rowHeight )
  {
    if( m_iUniformSpacingIdr == 0 && m_iNumRowsMinus1 > 0 )
    {
      m_puiRowHeight = new UInt[ m_iNumRowsMinus1 ];

      for(Int i=0; i<m_iNumRowsMinus1; i++)
      {
        m_puiRowHeight[i] = rowHeight[i];
      }
    }
  }
  UInt     getRowHeight           (UInt rowIdx)    { return *( m_puiRowHeight + rowIdx ); }
#endif
};

/// PPS class
class TComPPS
{
private:
  Int         m_PPSId;                    // pic_parameter_set_id
  Int         m_SPSId;                    // seq_parameter_set_id
  Bool        m_bConstrainedIntraPred;    // constrained_intra_pred_flag
 
  // access channel
  TComSPS*    m_pcSPS;
  UInt        m_uiMaxCuDQPDepth;
  UInt        m_uiMinCuDQPSize;

  UInt        m_uiNumTlayerSwitchingFlags;            // num_temporal_layer_switching_point_flags
  Bool        m_abTLayerSwitchingFlag[ MAX_TLAYER ];  // temporal_layer_switching_point_flag

#if FINE_GRANULARITY_SLICES
  Int         m_iSliceGranularity;
#endif

#if !F747_APS
  Bool        m_bSharedPPSInfoEnabled;  //!< Shared info. in PPS is enabled/disabled
  ALFParam    m_cSharedAlfParam;        //!< Shared ALF parameters in PPS 
#endif

#if WEIGHT_PRED
  Bool        m_bUseWeightPred;           // Use of Weighting Prediction (P_SLICE)
  UInt        m_uiBiPredIdc;              // Use of Weighting Bi-Prediction (B_SLICE)
#endif

#if TILES
  Int      m_iColumnRowInfoPresent;
  Int      m_iUniformSpacingIdr;
  Int      m_iTileBoundaryIndependenceIdr;
  Int      m_iNumColumnsMinus1;
  UInt*    m_puiColumnWidth;
  Int      m_iNumRowsMinus1;
  UInt*    m_puiRowHeight;
#endif
  
#if OL_USE_WPP
  Int      m_iEntropyCodingMode; // !!! in PPS now, but also remains in slice header!
  Int      m_iEntropyCodingSynchro;
  Bool     m_bCabacIstateReset;
  Int      m_iNumSubstreams;
#endif

public:
  TComPPS();
  virtual ~TComPPS();
  
  Int       getPPSId ()      { return m_PPSId; }
  Void      setPPSId (Int i) { m_PPSId = i; }
  Int       getSPSId ()      { return m_SPSId; }
  Void      setSPSId (Int i) { m_SPSId = i; }
  
#if FINE_GRANULARITY_SLICES
  Int       getSliceGranularity()        { return m_iSliceGranularity; }
  Void      setSliceGranularity( Int i ) { m_iSliceGranularity = i;    }
#endif
  Bool      getConstrainedIntraPred ()         { return  m_bConstrainedIntraPred; }
  Void      setConstrainedIntraPred ( Bool b ) { m_bConstrainedIntraPred = b;     }

  UInt      getNumTLayerSwitchingFlags()                                  { return m_uiNumTlayerSwitchingFlags; }
  Void      setNumTLayerSwitchingFlags( UInt uiNumTlayerSwitchingFlags )  { assert( uiNumTlayerSwitchingFlags < MAX_TLAYER ); m_uiNumTlayerSwitchingFlags = uiNumTlayerSwitchingFlags; }

  Bool      getTLayerSwitchingFlag( UInt uiTLayer )                       { assert( uiTLayer < MAX_TLAYER ); return m_abTLayerSwitchingFlag[ uiTLayer ]; }
  Void      setTLayerSwitchingFlag( UInt uiTLayer, Bool bValue )          { m_abTLayerSwitchingFlag[ uiTLayer ] = bValue; }

  Void      setSPS              ( TComSPS* pcSPS ) { m_pcSPS = pcSPS; }
  TComSPS*  getSPS              ()         { return m_pcSPS;          }
  Void      setMaxCuDQPDepth    ( UInt u ) { m_uiMaxCuDQPDepth = u;   }
  UInt      getMaxCuDQPDepth    ()         { return m_uiMaxCuDQPDepth;}
  Void      setMinCuDQPSize     ( UInt u ) { m_uiMinCuDQPSize = u;    }
  UInt      getMinCuDQPSize     ()         { return m_uiMinCuDQPSize; }

#if !F747_APS
  ///  set shared PPS info enabled/disabled
  Void      setSharedPPSInfoEnabled(Bool b) {m_bSharedPPSInfoEnabled = b;   }
  /// get shared PPS info enabled/disabled flag
  Bool      getSharedPPSInfoEnabled()       {return m_bSharedPPSInfoEnabled;}
  /// get shared ALF parameters in PPS
  ALFParam* getSharedAlfParam()             {return &m_cSharedAlfParam;     }
#endif

#if WEIGHT_PRED
  Bool getUseWP                     ()          { return m_bUseWeightPred;  }
  UInt getWPBiPredIdc               ()          { return m_uiBiPredIdc;     }

  Void setUseWP                     ( Bool b )  { m_bUseWeightPred = b;     }
  Void setWPBiPredIdc               ( UInt u )  { m_uiBiPredIdc = u;        }
#endif

#if TILES
  Void     setColumnRowInfoPresent          ( Int i )           { m_iColumnRowInfoPresent = i; }
  Int      getColumnRowInfoPresent          ()                  { return m_iColumnRowInfoPresent; }
  Void     setUniformSpacingIdr             ( Int i )           { m_iUniformSpacingIdr = i; }
  Int      getUniformSpacingIdr             ()                  { return m_iUniformSpacingIdr; }
  Void     setTileBoundaryIndependenceIdr   ( Int i )           { m_iTileBoundaryIndependenceIdr = i; }
  Int      getTileBoundaryIndependenceIdr   ()                  { return m_iTileBoundaryIndependenceIdr; }
  Void     setNumColumnsMinus1              ( Int i )           { m_iNumColumnsMinus1 = i; }
  Int      getNumColumnsMinus1              ()                  { return m_iNumColumnsMinus1; }
  Void     setColumnWidth ( UInt* columnWidth )
  {
    if( m_iUniformSpacingIdr == 0 && m_iNumColumnsMinus1 > 0 )
    {
      m_puiColumnWidth = new UInt[ m_iNumColumnsMinus1 ];

      for(Int i=0; i<m_iNumColumnsMinus1; i++)
      {
        m_puiColumnWidth[i] = columnWidth[i];
      }
    }
  }
  UInt     getColumnWidth  (UInt columnIdx) { return *( m_puiColumnWidth + columnIdx ); }
  Void     setNumRowsMinus1( Int i )        { m_iNumRowsMinus1 = i; }
  Int      getNumRowsMinus1()               { return m_iNumRowsMinus1; }
  Void     setRowHeight    ( UInt* rowHeight )
  {
    if( m_iUniformSpacingIdr == 0 && m_iNumRowsMinus1 > 0 )
    {
      m_puiRowHeight = new UInt[ m_iNumRowsMinus1 ];

      for(Int i=0; i<m_iNumRowsMinus1; i++)
      {
        m_puiRowHeight[i] = rowHeight[i];
      }
    }
  }
  UInt     getRowHeight           (UInt rowIdx)    { return *( m_puiRowHeight + rowIdx ); }
#endif
#if OL_USE_WPP
  Void     setEntropyCodingMode(Int iEntropyCodingMode)       { m_iEntropyCodingMode = iEntropyCodingMode; }
  Int      getEntropyCodingMode()                             { return m_iEntropyCodingMode; }
  Void     setEntropyCodingSynchro(Int iEntropyCodingSynchro) { m_iEntropyCodingSynchro = iEntropyCodingSynchro; }
  Int      getEntropyCodingSynchro()                          { return m_iEntropyCodingSynchro; }
  Void     setCabacIstateReset(Bool bCabacIstateReset)        { m_bCabacIstateReset = bCabacIstateReset; }
  Bool     getCabacIstateReset()                              { return m_bCabacIstateReset; }
  Void     setNumSubstreams(Int iNumSubstreams)               { m_iNumSubstreams = iNumSubstreams; }
  Int      getNumSubstreams()                                 { return m_iNumSubstreams; }
#endif
};

#if F747_APS
/// APS class
class TComAPS
{
public:
  TComAPS();
  virtual ~TComAPS();

  Void      setAPSID      (Int iID)   {m_apsID = iID;            }  //!< set APS ID 
  Int       getAPSID      ()          {return m_apsID;           }  //!< get APS ID
  Void      setSaoEnabled (Bool bVal) {m_bSaoEnabled = bVal;     }  //!< set SAO enabled/disabled in APS
  Bool      getSaoEnabled ()          {return m_bSaoEnabled;     }  //!< get SAO enabled/disabled in APS
  Void      setAlfEnabled (Bool bVal) {m_bAlfEnabled = bVal;     }  //!< set ALF enabled/disabled in APS
  Bool      getAlfEnabled ()          {return m_bAlfEnabled;     }  //!< get ALF enabled/disabled in APS

  ALFParam* getAlfParam   ()          {return m_pAlfParam;       }  //!< get ALF parameters in APS
  SAOParam* getSaoParam   ()          {return m_pSaoParam;       }  //!< get SAO parameters in APS

  Void      createSaoParam();   //!< create SAO parameter object
  Void      destroySaoParam();  //!< destroy SAO parameter object

  Void      createAlfParam();   //!< create ALF parameter object
  Void      destroyAlfParam();  //!< destroy ALF parameter object
  Void      setCABACForAPS(Bool bVal) {m_bCABACForAPS = bVal;    }  //!< set CABAC enabled/disabled in APS
  Bool      getCABACForAPS()          {return m_bCABACForAPS;    }  //!< get CABAC enabled/disabled in APS
  Void      setCABACinitIDC(Int iVal) {m_CABACinitIDC = iVal;    }  //!< set CABAC initial IDC number for APS coding
  Int       getCABACinitIDC()         {return m_CABACinitIDC;    }  //!< get CABAC initial IDC number for APS coding
  Void      setCABACinitQP(Int iVal)  {m_CABACinitQP = iVal;     }  //!< set CABAC initial QP value for APS coding
  Int       getCABACinitQP()          {return m_CABACinitQP;     }  //!< get CABAC initial QP value for APS coding

private:
  Int         m_apsID;        //!< APS ID
  Bool        m_bSaoEnabled;  //!< SAO enabled/disabled in APS (true for enabled)
  Bool        m_bAlfEnabled;  //!< ALF enabled/disabled in APS (true for enabled)
  SAOParam*   m_pSaoParam;    //!< SAO parameter object pointer 
  ALFParam*   m_pAlfParam;    //!< ALF parameter object pointer
  Bool        m_bCABACForAPS; //!< CABAC coding enabled/disabled for APS (true for enabling CABAC)
  Int         m_CABACinitIDC; //!< CABAC initial IDC number for APS coding
  Int         m_CABACinitQP;  //!< CABAC initial QP value for APS coding

public:
  TComAPS& operator= (const TComAPS& src);  //!< "=" operator for APS object
};
#endif


#if WEIGHT_PRED
typedef struct {
  // Explicit weighted prediction parameters parsed in slice header,
  // or Implicit weighted prediction parameters (8 bits depth values).
  Bool        bPresentFlag;
  UInt        uiLog2WeightDenom;
  Int         iWeight;
  Int         iOffset;

  // Weighted prediction scaling values built from above parameters (bitdepth scaled):
  Int         w, o, offset, shift, round;
} wpScalingParam;

typedef struct {
  Int64 iAC;
  Int64 iDC;
} wpACDCParam;
#endif

/// slice header class
class TComSlice
{
  
private:
  //  Bitstream writing
#if F747_APS
  Int         m_iAPSId; //!< APS ID in slice header
#endif
  Int         m_iPPSId;               ///< picture parameter set ID
  Int         m_iPOC;
  NalUnitType m_eNalUnitType;         ///< Nal unit type for the slice
  SliceType   m_eSliceType;
  Int         m_iSliceQp;
  Int         m_iSymbolMode;
  Bool        m_bLoopFilterDisable;
  
  Bool        m_bDRBFlag;             //  flag for future usage as reference buffer
  ERBIndex    m_eERBIndex;            //  flag for future usage as reference buffer
  Int         m_aiNumRefIdx   [3];    //  for multiple reference of current slice

  Int         m_iRefIdxOfLC[2][MAX_NUM_REF_LC];
  Int         m_eListIdFromIdxOfLC[MAX_NUM_REF_LC];
  Int         m_iRefIdxFromIdxOfLC[MAX_NUM_REF_LC];
  Int         m_iRefIdxOfL1FromRefIdxOfL0[MAX_NUM_REF_LC];
  Int         m_iRefIdxOfL0FromRefIdxOfL1[MAX_NUM_REF_LC];
  Bool        m_bRefPicListModificationFlagLC;
  Bool        m_bRefPicListCombinationFlag;

  Bool        m_bCheckLDC;

  //  Data
  Int         m_iSliceQpDelta;
  TComPic*    m_apcRefPicList [2][MAX_NUM_REF];
  Int         m_aiRefPOCList  [2][MAX_NUM_REF];
  Int         m_iDepth;
  
  // referenced slice?
  Bool        m_bRefenced;
  
  // access channel
  TComSPS*    m_pcSPS;
  TComPPS*    m_pcPPS;
  TComPic*    m_pcPic;
#if F747_APS
  TComAPS*    m_pcAPS;  //!< pointer to APS parameter object
#endif

  UInt        m_uiColDir;  // direction to get colocated CUs
  
#if ALF_CHROMA_LAMBDA || SAO_CHROMA_LAMBDA
  Double      m_dLambdaLuma;
  Double      m_dLambdaChroma;
#else
  Double      m_dLambda;
#endif

  Bool        m_abEqualRef  [2][MAX_NUM_REF][MAX_NUM_REF];
  
  Bool        m_bNoBackPredFlag;
  Bool        m_bRefIdxCombineCoding;

  UInt        m_uiTLayer;
  Bool        m_bTLayerSwitchingFlag;

  UInt        m_uiSliceMode;
  UInt        m_uiSliceArgument;
  UInt        m_uiSliceCurStartCUAddr;
  UInt        m_uiSliceCurEndCUAddr;
  UInt        m_uiSliceIdx;
  UInt        m_uiEntropySliceMode;
  UInt        m_uiEntropySliceArgument;
  UInt        m_uiEntropySliceCurStartCUAddr;
  UInt        m_uiEntropySliceCurEndCUAddr;
  Bool        m_bNextSlice;
  Bool        m_bNextEntropySlice;
  UInt        m_uiSliceBits;
#if FINE_GRANULARITY_SLICES
  UInt        m_uiEntropySliceCounter;
  Bool        m_bFinalized;
#endif

#if WEIGHT_PRED
  wpScalingParam  m_weightPredTable[2][MAX_NUM_REF][3]; // [REF_PIC_LIST_0 or REF_PIC_LIST_1][refIdx][0:Y, 1:U, 2:V]
  wpACDCParam    m_weightACDCParam[3];                 // [0:Y, 1:U, 2:V]
#endif

#if TILES_DECODER
  UInt        *m_uiTileByteLocation;
  UInt        m_uiTileCount;
  Int         m_iTileMarkerFlag;
  UInt        m_uiTileOffstForMultES;
#endif

#if OL_USE_WPP
  UInt*       m_puiSubstreamSizes;
#endif

public:
  TComSlice();
  virtual ~TComSlice();
  
  Void      initSlice       ();
  
  Void      setSPS          ( TComSPS* pcSPS ) { m_pcSPS = pcSPS; }
  TComSPS*  getSPS          () { return m_pcSPS; }
  
  Void      setPPS          ( TComPPS* pcPPS )         { assert(pcPPS!=NULL); m_pcPPS = pcPPS; m_iPPSId = pcPPS->getPPSId(); }
  TComPPS*  getPPS          () { return m_pcPPS; }

  Void      setPPSId        ( Int PPSId )         { m_iPPSId = PPSId; }
  Int       getPPSId        () { return m_iPPSId; }
#if F747_APS
  Void      setAPS          ( TComAPS* pcAPS ) { m_pcAPS = pcAPS; } //!< set APS pointer
  TComAPS*  getAPS          ()                 { return m_pcAPS;  } //!< get APS pointer
  Void      setAPSId        ( Int Id)          { m_iAPSId =Id;    } //!< set APS ID
  Int       getAPSId        ()                 { return m_iAPSId; } //!< get APS ID
#endif
  
  SliceType getSliceType    ()                          { return  m_eSliceType;         }
  Int       getPOC          ()                          { return  m_iPOC;           }
  Int       getSliceQp      ()                          { return  m_iSliceQp;           }
  Int       getSliceQpDelta ()                          { return  m_iSliceQpDelta;      }
  Bool      getDRBFlag      ()                          { return  m_bDRBFlag;           }
  ERBIndex  getERBIndex     ()                          { return  m_eERBIndex;          }
  Int       getSymbolMode   ()                          { return  m_iSymbolMode;        }
  Bool      getLoopFilterDisable()                      { return  m_bLoopFilterDisable; }
  Int       getNumRefIdx        ( RefPicList e )                { return  m_aiNumRefIdx[e];             }
  TComPic*  getPic              ()                              { return  m_pcPic;                      }
  TComPic*  getRefPic           ( RefPicList e, Int iRefIdx)    { return  m_apcRefPicList[e][iRefIdx];  }
  Int       getRefPOC           ( RefPicList e, Int iRefIdx)    { return  m_aiRefPOCList[e][iRefIdx];   }
  Int       getDepth            ()                              { return  m_iDepth;                     }
  UInt      getColDir           ()                              { return  m_uiColDir;                   }
  Bool      getCheckLDC     ()                                  { return m_bCheckLDC; }

  Int       getRefIdxOfLC       (RefPicList e, Int iRefIdx)     { return m_iRefIdxOfLC[e][iRefIdx];           }
  Int       getListIdFromIdxOfLC(Int iRefIdx)                   { return m_eListIdFromIdxOfLC[iRefIdx];       }
  Int       getRefIdxFromIdxOfLC(Int iRefIdx)                   { return m_iRefIdxFromIdxOfLC[iRefIdx];       }
  Int       getRefIdxOfL0FromRefIdxOfL1(Int iRefIdx)            { return m_iRefIdxOfL0FromRefIdxOfL1[iRefIdx];}
  Int       getRefIdxOfL1FromRefIdxOfL0(Int iRefIdx)            { return m_iRefIdxOfL1FromRefIdxOfL0[iRefIdx];}
  Bool      getRefPicListModificationFlagLC()                   {return m_bRefPicListModificationFlagLC;}
  Void      setRefPicListModificationFlagLC(Bool bflag)         {m_bRefPicListModificationFlagLC=bflag;}     
  Bool      getRefPicListCombinationFlag()                      {return m_bRefPicListCombinationFlag;}
  Void      setRefPicListCombinationFlag(Bool bflag)            {m_bRefPicListCombinationFlag=bflag;}     
  Void      setListIdFromIdxOfLC(Int  iRefIdx, UInt uiVal)      { m_eListIdFromIdxOfLC[iRefIdx]=uiVal; }
  Void      setRefIdxFromIdxOfLC(Int  iRefIdx, UInt uiVal)      { m_iRefIdxFromIdxOfLC[iRefIdx]=uiVal; }
  Void      setRefIdxOfLC       (RefPicList e, Int iRefIdx, Int RefIdxLC)     { m_iRefIdxOfLC[e][iRefIdx]=RefIdxLC;}

  Void      setReferenced(Bool b)                               { m_bRefenced = b; }
  Bool      isReferenced()                                      { return m_bRefenced; }
  
  Void      setPOC              ( Int i )                       { m_iPOC              = i;      }
  Void      setNalUnitType      ( NalUnitType e )               { m_eNalUnitType      = e;      }
  NalUnitType getNalUnitType    ()                              { return m_eNalUnitType;        }
  Void      decodingRefreshMarking(UInt& uiPOCCDR, Bool& bRefreshPending, TComList<TComPic*>& rcListPic);
  Void      setSliceType        ( SliceType e )                 { m_eSliceType        = e;      }
  Void      setSliceQp          ( Int i )                       { m_iSliceQp          = i;      }
  Void      setSliceQpDelta     ( Int i )                       { m_iSliceQpDelta     = i;      }
  Void      setDRBFlag          ( Bool b )                      { m_bDRBFlag = b;               }
  Void      setERBIndex         ( ERBIndex e )                  { m_eERBIndex = e;              }
  Void      setSymbolMode       ( Int b  )                      { m_iSymbolMode       = b;      }
  Void      setLoopFilterDisable( Bool b )                      { m_bLoopFilterDisable= b;      }
  
  Void      setRefPic           ( TComPic* p, RefPicList e, Int iRefIdx ) { m_apcRefPicList[e][iRefIdx] = p; }
  Void      setRefPOC           ( Int i, RefPicList e, Int iRefIdx ) { m_aiRefPOCList[e][iRefIdx] = i; }
  Void      setNumRefIdx        ( RefPicList e, Int i )         { m_aiNumRefIdx[e]    = i;      }
  Void      setPic              ( TComPic* p )                  { m_pcPic             = p;      }
  Void      setDepth            ( Int iDepth )                  { m_iDepth            = iDepth; }
  
  Void      setRefPicList       ( TComList<TComPic*>& rcListPic );
  Void      setRefPOCList       ();
  Void      setColDir           ( UInt uiDir ) { m_uiColDir = uiDir; }
  Void      setCheckLDC         ( Bool b )                      { m_bCheckLDC = b; }
  
  Bool      isIntra         ()                          { return  m_eSliceType == I_SLICE;  }
  Bool      isInterB        ()                          { return  m_eSliceType == B_SLICE;  }
  Bool      isInterP        ()                          { return  m_eSliceType == P_SLICE;  }
  
#if ALF_CHROMA_LAMBDA || SAO_CHROMA_LAMBDA  
  Void      setLambda( Double d, Double e ) { m_dLambdaLuma = d; m_dLambdaChroma = e;}
  Double    getLambdaLuma() { return m_dLambdaLuma;        }
  Double    getLambdaChroma() { return m_dLambdaChroma;        }
#else
  Void      setLambda( Double d ) { m_dLambda = d; }
  Double    getLambda() { return m_dLambda;        }
#endif
  
  Void      initEqualRef();
  Bool      isEqualRef  ( RefPicList e, Int iRefIdx1, Int iRefIdx2 )
  {
    if (iRefIdx1 < 0 || iRefIdx2 < 0) return false;
    return m_abEqualRef[e][iRefIdx1][iRefIdx2];
  }
  
  Void setEqualRef( RefPicList e, Int iRefIdx1, Int iRefIdx2, Bool b)
  {
    m_abEqualRef[e][iRefIdx1][iRefIdx2] = m_abEqualRef[e][iRefIdx2][iRefIdx1] = b;
  }
  
  static Void      sortPicList         ( TComList<TComPic*>& rcListPic );
  
  Bool getNoBackPredFlag() { return m_bNoBackPredFlag; }
  Void setNoBackPredFlag( Bool b ) { m_bNoBackPredFlag = b; }
  Bool getRefIdxCombineCoding() { return m_bRefIdxCombineCoding; }
  Void setRefIdxCombineCoding( Bool b ) { m_bRefIdxCombineCoding = b; }
  Void generateCombinedList       ();

  UInt getTLayer             ()                            { return m_uiTLayer;                      }
  Void setTLayer             ( UInt uiTLayer )             { m_uiTLayer = uiTLayer;                  }

  Bool getTLayerSwitchingFlag()                            { return m_bTLayerSwitchingFlag;          }
  Void setTLayerSwitchingFlag( Bool bValue )               { m_bTLayerSwitchingFlag = bValue;        }

  Void setTLayerInfo( UInt uiTLayer );
  Void decodingMarking( TComList<TComPic*>& rcListPic, Int iGOPSIze, Int& iMaxRefPicNum ); 
  Void decodingTLayerSwitchingMarking( TComList<TComPic*>& rcListPic );

#if REF_SETTING_FOR_LD
  Int getActualRefNumber( TComList<TComPic*>& rcListPic );
  Void decodingRefMarkingForLD( TComList<TComPic*>& rcListPic, Int iMaxNumRefFrames, Int iCurrentPOC );
#endif

  Void setSliceMode                     ( UInt uiMode )     { m_uiSliceMode = uiMode;                     }
  UInt getSliceMode                     ()                  { return m_uiSliceMode;                       }
  Void setSliceArgument                 ( UInt uiArgument ) { m_uiSliceArgument = uiArgument;             }
  UInt getSliceArgument                 ()                  { return m_uiSliceArgument;                   }
  Void setSliceCurStartCUAddr           ( UInt uiAddr )     { m_uiSliceCurStartCUAddr = uiAddr;           }
  UInt getSliceCurStartCUAddr           ()                  { return m_uiSliceCurStartCUAddr;             }
  Void setSliceCurEndCUAddr             ( UInt uiAddr )     { m_uiSliceCurEndCUAddr = uiAddr;             }
  UInt getSliceCurEndCUAddr             ()                  { return m_uiSliceCurEndCUAddr;               }
  Void setSliceIdx                      ( UInt i)           { m_uiSliceIdx = i;                           }
  UInt getSliceIdx                      ()                  { return  m_uiSliceIdx;                       }
  Void copySliceInfo                    (TComSlice *pcSliceSrc);
  Void setEntropySliceMode              ( UInt uiMode )     { m_uiEntropySliceMode = uiMode;              }
  UInt getEntropySliceMode              ()                  { return m_uiEntropySliceMode;                }
  Void setEntropySliceArgument          ( UInt uiArgument ) { m_uiEntropySliceArgument = uiArgument;      }
  UInt getEntropySliceArgument          ()                  { return m_uiEntropySliceArgument;            }
  Void setEntropySliceCurStartCUAddr    ( UInt uiAddr )     { m_uiEntropySliceCurStartCUAddr = uiAddr;    }
  UInt getEntropySliceCurStartCUAddr    ()                  { return m_uiEntropySliceCurStartCUAddr;      }
  Void setEntropySliceCurEndCUAddr      ( UInt uiAddr )     { m_uiEntropySliceCurEndCUAddr = uiAddr;      }
  UInt getEntropySliceCurEndCUAddr      ()                  { return m_uiEntropySliceCurEndCUAddr;        }
  Void setNextSlice                     ( Bool b )          { m_bNextSlice = b;                           }
  Bool isNextSlice                      ()                  { return m_bNextSlice;                        }
  Void setNextEntropySlice              ( Bool b )          { m_bNextEntropySlice = b;                    }
  Bool isNextEntropySlice               ()                  { return m_bNextEntropySlice;                 }
  Void setSliceBits                     ( UInt uiVal )      { m_uiSliceBits = uiVal;                      }
  UInt getSliceBits                     ()                  { return m_uiSliceBits;                       }  
#if FINE_GRANULARITY_SLICES
  Void setEntropySliceCounter           ( UInt uiVal )      { m_uiEntropySliceCounter = uiVal;            }
  UInt getEntropySliceCounter           ()                  { return m_uiEntropySliceCounter;             }
  Void setFinalized                     ( Bool uiVal )      { m_bFinalized = uiVal;                       }
  Bool getFinalized                     ()                  { return m_bFinalized;                        }
#endif
#if WEIGHT_PRED
  Void  setWpScaling    ( wpScalingParam  wp[2][MAX_NUM_REF][3] ) { memcpy(m_weightPredTable, wp, sizeof(wpScalingParam)*2*MAX_NUM_REF*3); }
  Void  getWpScaling    ( RefPicList e, Int iRefIdx, wpScalingParam *&wp);

  Void  resetWpScaling  (wpScalingParam  wp[2][MAX_NUM_REF][3]);
  Void  initWpScaling    (wpScalingParam  wp[2][MAX_NUM_REF][3]);
  Void  initWpScaling   ();
  inline Bool applyWP   () { return( (m_eSliceType==P_SLICE && m_pcPPS->getUseWP()) || (m_eSliceType==B_SLICE && m_pcPPS->getWPBiPredIdc()) ); }
  
  Void  setWpAcDcParam  ( wpACDCParam wp[3] ) { memcpy(m_weightACDCParam, wp, sizeof(wpACDCParam)*3); }
  Void  getWpAcDcParam  ( wpACDCParam *&wp );
  Void  initWpAcDcParam ();
#endif
#if TILES_DECODER
  Void setTileLocationCount             ( UInt uiCount )      { m_uiTileCount = uiCount;                  }
  UInt getTileLocationCount             ()                    { return m_uiTileCount;                     }
  Void setTileLocation                  ( Int i, UInt uiLOC ) { m_uiTileByteLocation[i] = uiLOC;          }
  UInt getTileLocation                  ( Int i )             { return m_uiTileByteLocation[i];           }
  Void setTileMarkerFlag                ( Int iFlag )         { m_iTileMarkerFlag = iFlag;                }
  Int  getTileMarkerFlag                ()                    { return m_iTileMarkerFlag;                 }
  Void setTileOffstForMultES            (UInt uiOffset )      { m_uiTileOffstForMultES = uiOffset;        }
  UInt getTileOffstForMultES            ()                    { return m_uiTileOffstForMultES;            }
#endif
#if OL_USE_WPP
  Void allocSubstreamSizes              ( UInt uiNumSubstreams );
  UInt* getSubstreamSizes               ()                  { return m_puiSubstreamSizes; }
#endif
protected:
  TComPic*  xGetRefPic  (TComList<TComPic*>& rcListPic,
                         Bool                bDRBFlag,
                         ERBIndex            eERBIndex,
                         UInt                uiPOCCurr,
                         RefPicList          eRefPicList,
                         UInt                uiNthRefPic,
                         UInt                uiTLayer );
};// END CLASS DEFINITION TComSlice

//! \}

#endif // __TCOMSLICE__
