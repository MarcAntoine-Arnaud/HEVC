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


#include "CommonDef.h"
#include "TComList.h"

class TComPic;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// SPS class
class TComSPS
{
private:
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
#if E057_INTRA_PCM
  UInt        m_uiPCMLog2MinSize;
#endif
  Bool        m_bUseALF;
  Bool        m_bUseDQP;
  Bool        m_bUseLDC;
  Bool        m_bUsePAD;
  Bool        m_bUseMRG; // SOPH:

#if LM_CHROMA 
  Bool        m_bUseLMChroma; // JL:
#endif

#if DCM_COMB_LIST
  Bool        m_bUseLComb;
  Bool        m_bLCMod;
#endif
  
#if HHI_RMP_SWITCH
  Bool        m_bUseRMP;
#endif
  // Parameter
  AMVP_MODE   m_aeAMVPMode[MAX_CU_DEPTH];
  UInt        m_uiBitDepth;
  UInt        m_uiBitIncrement;

#if E057_INTRA_PCM && E192_SPS_PCM_BIT_DEPTH_SYNTAX
  UInt        m_uiPCMBitDepthLuma;
  UInt        m_uiPCMBitDepthChroma;
#endif
#if E057_INTRA_PCM && E192_SPS_PCM_FILTER_DISABLE_SYNTAX
  Bool        m_bPCMFilterDisableFlag;
#endif

  // Max physical transform size
  UInt        m_uiMaxTrSize;
  
  Int m_iAMPAcc[MAX_CU_DEPTH];

#if MTK_NONCROSS_INLOOP_FILTER
  Bool        m_bLFCrossSliceBoundaryFlag;
#endif
#if MTK_SAO
  Bool        m_bUseSAO; 
#endif

  Bool        m_bTemporalIdNestingFlag; // temporal_id_nesting_flag

public:
  TComSPS();
  virtual ~TComSPS();
  
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
#if E057_INTRA_PCM
  Void setPCMLog2MinSize  ( UInt u ) { m_uiPCMLog2MinSize = u;      }
  UInt getPCMLog2MinSize  ()         { return  m_uiPCMLog2MinSize;  }
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
  
#if DCM_COMB_LIST
  Void setUseLComb    (Bool b)   { m_bUseLComb = b;         }
  Bool getUseLComb    ()         { return m_bUseLComb;      }
  Void setLCMod       (Bool b)   { m_bLCMod = b;     }
  Bool getLCMod       ()         { return m_bLCMod;  }
#endif

#if HHI_RMP_SWITCH
  Bool getUseRMP     ()         { return m_bUseRMP; }
  Void setUseRMP     ( Bool b ) { m_bUseRMP = b;    }
#endif
  
#if LM_CHROMA 
  Bool getUseLMChroma ()         { return m_bUseLMChroma;        }
  Void setUseLMChroma ( Bool b ) { m_bUseLMChroma  = b;          }
#endif

  // AMVP mode (for each depth)
  AMVP_MODE getAMVPMode ( UInt uiDepth ) { assert(uiDepth < g_uiMaxCUDepth);  return m_aeAMVPMode[uiDepth]; }
  Void      setAMVPMode ( UInt uiDepth, AMVP_MODE eMode) { assert(uiDepth < g_uiMaxCUDepth);  m_aeAMVPMode[uiDepth] = eMode; }
  
  // Bit-depth
  UInt      getBitDepth     ()         { return m_uiBitDepth;     }
  Void      setBitDepth     ( UInt u ) { m_uiBitDepth = u;        }
  UInt      getBitIncrement ()         { return m_uiBitIncrement; }
  Void      setBitIncrement ( UInt u ) { m_uiBitIncrement = u;    }

#if MTK_NONCROSS_INLOOP_FILTER
  Void      setLFCrossSliceBoundaryFlag     ( Bool   bValue  )    { m_bLFCrossSliceBoundaryFlag = bValue; }
  Bool      getLFCrossSliceBoundaryFlag     ()                    { return m_bLFCrossSliceBoundaryFlag;   } 
#endif

#if MTK_SAO
  Void setUseSAO                  (Bool bVal)  {m_bUseSAO = bVal;}
  Bool getUseSAO                  ()           {return m_bUseSAO;}
#endif

  UInt      getMaxTLayers()                           { return m_uiMaxTLayers; }
  Void      setMaxTLayers( UInt uiMaxTLayers )        { assert( uiMaxTLayers <= MAX_TLAYER ); m_uiMaxTLayers = uiMaxTLayers; }

  Bool      getTemporalIdNestingFlag()                { return m_bTemporalIdNestingFlag; }
  Void      setTemporalIdNestingFlag( Bool bValue )   { m_bTemporalIdNestingFlag = bValue; }
#if E057_INTRA_PCM && E192_SPS_PCM_BIT_DEPTH_SYNTAX
  UInt      getPCMBitDepthLuma     ()         { return m_uiPCMBitDepthLuma;     }
  Void      setPCMBitDepthLuma     ( UInt u ) { m_uiPCMBitDepthLuma = u;        }
  UInt      getPCMBitDepthChroma   ()         { return m_uiPCMBitDepthChroma;   }
  Void      setPCMBitDepthChroma   ( UInt u ) { m_uiPCMBitDepthChroma = u;      }
#endif
#if E057_INTRA_PCM && E192_SPS_PCM_FILTER_DISABLE_SYNTAX
  Void      setPCMFilterDisableFlag     ( Bool   bValue  )    { m_bPCMFilterDisableFlag = bValue; }
  Bool      getPCMFilterDisableFlag     ()                    { return m_bPCMFilterDisableFlag;   } 
#endif
};

/// PPS class
class TComPPS
{
private:
#if CONSTRAINED_INTRA_PRED
  Bool        m_bConstrainedIntraPred;    //  constrained_intra_pred_flag
#endif
 
#if SUB_LCU_DQP
  // access channel
  TComSPS*    m_pcSPS;
  UInt        m_uiMaxCuDQPDepth;
  UInt        m_uiMinCuDQPSize;
#endif

  UInt        m_uiNumTlayerSwitchingFlags;            // num_temporal_layer_switching_point_flags
  Bool        m_abTLayerSwitchingFlag[ MAX_TLAYER ];  // temporal_layer_switching_point_flag

public:
  TComPPS();
  virtual ~TComPPS();
  
#if CONSTRAINED_INTRA_PRED
  Bool      getConstrainedIntraPred ()         { return  m_bConstrainedIntraPred; }
  Void      setConstrainedIntraPred ( Bool b ) { m_bConstrainedIntraPred = b;     }
#endif

  UInt      getNumTLayerSwitchingFlags()                                  { return m_uiNumTlayerSwitchingFlags; }
  Void      setNumTLayerSwitchingFlags( UInt uiNumTlayerSwitchingFlags )  { assert( uiNumTlayerSwitchingFlags < MAX_TLAYER ); m_uiNumTlayerSwitchingFlags = uiNumTlayerSwitchingFlags; }

  Bool      getTLayerSwitchingFlag( UInt uiTLayer )                       { assert( uiTLayer < MAX_TLAYER ); return m_abTLayerSwitchingFlag[ uiTLayer ]; }
  Void      setTLayerSwitchingFlag( UInt uiTLayer, Bool bValue )          { m_abTLayerSwitchingFlag[ uiTLayer ] = bValue; }

#if SUB_LCU_DQP
  Void      setSPS              ( TComSPS* pcSPS ) { m_pcSPS = pcSPS; }
  TComSPS*  getSPS              ()         { return m_pcSPS;          }
  Void      setMaxCuDQPDepth    ( UInt u ) { m_uiMaxCuDQPDepth = u;   }
  UInt      getMaxCuDQPDepth    ()         { return m_uiMaxCuDQPDepth;}
  Void      setMinCuDQPSize     ( UInt u ) { m_uiMinCuDQPSize = u;    }
  UInt      getMinCuDQPSize     ()         { return m_uiMinCuDQPSize; }
#endif
};

/// slice header class
class TComSlice
{
  
private:
  //  Bitstream writing
  Int         m_iPOC;
#if DCM_DECODING_REFRESH
  NalUnitType m_eNalUnitType;         ///< Nal unit type for the slice
#endif
  SliceType   m_eSliceType;
  Int         m_iSliceQp;
  Int         m_iSymbolMode;
  Bool        m_bLoopFilterDisable;
  
  Bool        m_bDRBFlag;             //  flag for future usage as reference buffer
  ERBIndex    m_eERBIndex;            //  flag for future usage as reference buffer
#if DCM_COMB_LIST
  Int         m_aiNumRefIdx   [3];    //  for multiple reference of current slice

  Int         m_iRefIdxOfLC[2][MAX_NUM_REF_LC];
  Int         m_eListIdFromIdxOfLC[MAX_NUM_REF_LC];
  Int         m_iRefIdxFromIdxOfLC[MAX_NUM_REF_LC];
  Int         m_iRefIdxOfL1FromRefIdxOfL0[MAX_NUM_REF_LC];
  Int         m_iRefIdxOfL0FromRefIdxOfL1[MAX_NUM_REF_LC];
  Bool        m_bRefPicListModificationFlagLC;
  Bool        m_bRefPicListCombinationFlag;
#else
  Int         m_aiNumRefIdx   [2];    //  for multiple reference of current slice
#endif  

  //  Data
  Int         m_iSliceQpDelta;
  TComPic*    m_apcRefPicList [2][MAX_NUM_REF];
  Int         m_aiRefPOCList  [2][MAX_NUM_REF];
  Int         m_iDepth;
  
  // referenced slice?
  Bool        m_bRefenced;
#ifdef ROUNDING_CONTROL_BIPRED
  Bool        m_bRounding;
#endif
  
  // access channel
  TComSPS*    m_pcSPS;
  TComPPS*    m_pcPPS;
  TComPic*    m_pcPic;
  
  UInt        m_uiColDir;  // direction to get colocated CUs
  
  Double      m_dLambda;
  
  Bool        m_abEqualRef  [2][MAX_NUM_REF][MAX_NUM_REF];
  
  Bool        m_bNoBackPredFlag;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  Bool        m_bRefIdxCombineCoding;
#endif

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
  
public:
  TComSlice();
  virtual ~TComSlice();
  
  Void      initSlice       ();
  
  Void      setSPS          ( TComSPS* pcSPS ) { m_pcSPS = pcSPS; }
  TComSPS*  getSPS          () { return m_pcSPS; }
  
  Void      setPPS          ( TComPPS* pcPPS ) { m_pcPPS = pcPPS; }
  TComPPS*  getPPS          () { return m_pcPPS; }
  
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
  
#if DCM_COMB_LIST 
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
#endif

  Void      setReferenced(Bool b)                               { m_bRefenced = b; }
  Bool      isReferenced()                                      { return m_bRefenced; }
#ifdef ROUNDING_CONTROL_BIPRED
  Void      setRounding(Bool bRound)                            { m_bRounding = bRound; }
  Bool      isRounding()                                        { return m_bRounding; }
#endif
  
  Void      setPOC              ( Int i )                       { m_iPOC              = i;      }
#if DCM_DECODING_REFRESH
  Void      setNalUnitType      ( NalUnitType e )               { m_eNalUnitType      = e;      }
  NalUnitType getNalUnitType    ()                              { return m_eNalUnitType;        }
  Void      decodingRefreshMarking(UInt& uiPOCCDR, Bool& bRefreshPending, TComList<TComPic*>& rcListPic);
#endif
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
  
  Bool      isIntra         ()                          { return  m_eSliceType == I_SLICE;  }
  Bool      isInterB        ()                          { return  m_eSliceType == B_SLICE;  }
  Bool      isInterP        ()                          { return  m_eSliceType == P_SLICE;  }
  
  Void      setLambda( Double d ) { m_dLambda = d; }
  Double    getLambda() { return m_dLambda;        }
  
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
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  Bool getRefIdxCombineCoding() { return m_bRefIdxCombineCoding; }
  Void setRefIdxCombineCoding( Bool b ) { m_bRefIdxCombineCoding = b; }
#endif
#if DCM_COMB_LIST
  Void      generateCombinedList       ();
#endif

  UInt      getTLayer             ()                            { return m_uiTLayer;                      }
  Void      setTLayer             ( UInt uiTLayer )             { m_uiTLayer = uiTLayer;                  }

  Bool      getTLayerSwitchingFlag()                            { return m_bTLayerSwitchingFlag;          }
  Void      setTLayerSwitchingFlag( Bool bValue )               { m_bTLayerSwitchingFlag = bValue;        }

  Void      setTLayerInfo( UInt uiTLayer );
  Void      decodingMarking( TComList<TComPic*>& rcListPic, Int iGOPSIze, Int& iMaxRefPicNum ); 
  Void      decodingTLayerSwitchingMarking( TComList<TComPic*>& rcListPic );

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
  
protected:
  TComPic*  xGetRefPic  (TComList<TComPic*>& rcListPic,
                         Bool                bDRBFlag,
                         ERBIndex            eERBIndex,
                         UInt                uiPOCCurr,
                         RefPicList          eRefPicList,
                         UInt                uiNthRefPic,
                         UInt                uiTLayer );
};// END CLASS DEFINITION TComSlice


#endif // __TCOMSLICE__

