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

/** \file     TComLoopFilter.cpp
    \brief    deblocking filter
*/

#include "TComLoopFilter.h"
#include "TComSlice.h"
#include "TComMv.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define   EDGE_VER    0
#define   EDGE_HOR    1
#define   QpUV(iQpY)  ( g_aucChromaScale[ max( min( (iQpY), MAX_QP ), MIN_QP ) ] )

#define DEFAULT_INTRA_TC_OFFSET 2 ///< Default intra TC offset

// ====================================================================================================================
// Tables
// ====================================================================================================================

const UChar tctable_8x8[56] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,5,5,6,6,7,8,9,9,10,10,11,11,12,12,13,13,14,14
};

const UChar betatable_8x8[52] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,7,8,9,10,11,12,13,14,15,16,17,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64
};

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComLoopFilter::TComLoopFilter()
: m_uiNumPartitions( 0 )
{
  m_uiDisableDeblockingFilterIdc = 0;
#if NONCROSS_TILE_IN_LOOP_FILTERING
  m_bLFCrossTileBoundary = true;
#endif
}

TComLoopFilter::~TComLoopFilter()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

#if NONCROSS_TILE_IN_LOOP_FILTERING
Void TComLoopFilter::setCfg( UInt uiDisableDblkIdc, Int iBetaOffset_div2, Int iTcOffset_div2, Bool bLFCrossTileBoundary)
#else
Void TComLoopFilter::setCfg( UInt uiDisableDblkIdc, Int iAlphaOffset, Int iBetaOffset)
#endif
{
  m_uiDisableDeblockingFilterIdc  = uiDisableDblkIdc;
#if NONCROSS_TILE_IN_LOOP_FILTERING
  m_bLFCrossTileBoundary = bLFCrossTileBoundary;
#endif

  m_betaOffsetDiv2 = iBetaOffset_div2;
  m_tcOffsetDiv2 = iTcOffset_div2;
}

Void TComLoopFilter::create( UInt uiMaxCUDepth )
{
  m_uiNumPartitions = 1 << ( uiMaxCUDepth<<1 );
  for( UInt uiDir = 0; uiDir < 2; uiDir++ )
  {
    m_aapucBS       [uiDir] = new UChar[m_uiNumPartitions];
    for( UInt uiPlane = 0; uiPlane < 3; uiPlane++ )
    {
      m_aapbEdgeFilter[uiDir][uiPlane] = new Bool [m_uiNumPartitions];
    }
  }
  
}

Void TComLoopFilter::destroy()
{
  for( UInt uiDir = 0; uiDir < 2; uiDir++ )
  {
    delete [] m_aapucBS       [uiDir];
    for( UInt uiPlane = 0; uiPlane < 3; uiPlane++ )
    {
      delete [] m_aapbEdgeFilter[uiDir][uiPlane];
    }
  }
  
}

/**
 - call deblocking function for every CU
 .
 \param  pcPic   picture class (TComPic) pointer
 */
Void TComLoopFilter::loopFilterPic( TComPic* pcPic )
{
  if (m_uiDisableDeblockingFilterIdc == 1)
  {
    return;
  }
  
  // Horizontal filtering
  for ( UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame(); uiCUAddr++ )
  {
    TComDataCU* pcCU = pcPic->getCU( uiCUAddr );

    ::memset( m_aapucBS       [EDGE_VER], 0, sizeof( UChar ) * m_uiNumPartitions );
    for( Int iPlane = 0; iPlane < 3; iPlane++ )
    {
      ::memset( m_aapbEdgeFilter[EDGE_VER][iPlane], 0, sizeof( bool  ) * m_uiNumPartitions );
    }

    // CU-based deblocking
    xDeblockCU( pcCU, 0, 0, EDGE_VER );
  }

  // Vertical filtering
  for ( UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame(); uiCUAddr++ )
  {
    TComDataCU* pcCU = pcPic->getCU( uiCUAddr );

      ::memset( m_aapucBS       [EDGE_HOR], 0, sizeof( UChar ) * m_uiNumPartitions );
    for( Int iPlane = 0; iPlane < 3; iPlane++ )
    {
      ::memset( m_aapbEdgeFilter[EDGE_HOR][iPlane], 0, sizeof( bool  ) * m_uiNumPartitions );
    }

    // CU-based deblocking
    xDeblockCU( pcCU, 0, 0, EDGE_HOR );
  }
}


// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - Deblocking filter process in CU-based (the same function as conventional's)
 .
 \param Edge          the direction of the edge in block boundary (horizonta/vertical), which is added newly
*/
Void TComLoopFilter::xDeblockCU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Int Edge )
{
  TComPic* pcPic     = pcCU->getPic();
  UInt uiCurNumParts = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiQNumParts   = uiCurNumParts>>2;
  
  if( pcCU->getDepth(uiAbsZorderIdx) > uiDepth )
  {
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx+=uiQNumParts )
    {
      UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
      {
        xDeblockCU( pcCU, uiAbsZorderIdx, uiDepth+1, Edge );
      }
    }
    return;
  }
  
  xSetLoopfilterParam( pcCU, uiAbsZorderIdx );
  
#if NSQT
  xSetEdgefilterTU   ( pcCU, g_auiZscanToRaster[ uiAbsZorderIdx ], uiAbsZorderIdx, uiDepth );
#else
  xSetEdgefilterTU   ( pcCU, uiAbsZorderIdx, uiDepth );
#endif
  xSetEdgefilterPU   ( pcCU, uiAbsZorderIdx );
  
  Int iDir = Edge;
  {
    for( UInt uiPartIdx = uiAbsZorderIdx; uiPartIdx < uiAbsZorderIdx + uiCurNumParts; uiPartIdx++ )
    {
#if BS_DISABLE_INSIDE_8x8
      UInt uiBSCheck = ((iDir == EDGE_VER && uiPartIdx%2 == 0) || (iDir == EDGE_HOR && (uiPartIdx-((uiPartIdx>>2)<<2))/2 == 0));
      if ( m_aapbEdgeFilter[iDir][0][uiPartIdx] && uiBSCheck )
#else
      if ( m_aapbEdgeFilter[iDir][0][uiPartIdx] )
#endif
      {
        xGetBoundaryStrengthSingle ( pcCU, uiAbsZorderIdx, iDir, uiPartIdx );
      }
    }
  }
  
  UInt uiPelsInPart = g_uiMaxCUWidth >> g_uiMaxCUDepth;
  UInt PartIdxIncr = DEBLOCK_SMALLEST_BLOCK / uiPelsInPart ? DEBLOCK_SMALLEST_BLOCK / uiPelsInPart : 1 ;
  
  UInt uiSizeInPU = pcPic->getNumPartInWidth()>>(uiDepth);
  
  {
    for ( UInt iEdge = 0; iEdge < uiSizeInPU ; iEdge+=PartIdxIncr)
    {
      xEdgeFilterLuma     ( pcCU, uiAbsZorderIdx, uiDepth, iDir, iEdge );
      if ( (iEdge % ( (DEBLOCK_SMALLEST_BLOCK<<1)/uiPelsInPart ) ) == 0 )
      {
        xEdgeFilterChroma   ( pcCU, uiAbsZorderIdx, uiDepth, iDir, iEdge );
      }
    }
  }
}


#if NSQT
Void TComLoopFilter::xSetEdgefilterMultiple( TComDataCU* pcCU, UInt uiScanIdx, UInt uiDepth, Int iDir, Int iEdgeIdx, Bool bValue,UInt uiWidthInBaseUnits, UInt uiHeightInBaseUnits)
{  
  if ( uiWidthInBaseUnits == 0 )
  {
    uiWidthInBaseUnits  = pcCU->getPic()->getNumPartInWidth () >> uiDepth;
  }
  if ( uiHeightInBaseUnits == 0 )
  {
    uiHeightInBaseUnits = pcCU->getPic()->getNumPartInHeight() >> uiDepth;
  }
  const UInt uiNumElem = iDir == 0 ? uiHeightInBaseUnits : uiWidthInBaseUnits;
  assert( uiNumElem > 0 );
  assert( uiWidthInBaseUnits > 0 );
  assert( uiHeightInBaseUnits > 0 );
  for( UInt ui = 0; ui < uiNumElem; ui++ )
  {
    UInt uiBsIdx;
    if ( uiWidthInBaseUnits == uiHeightInBaseUnits )
    {
      uiBsIdx = xCalcBsIdx( pcCU, uiScanIdx, iDir, iEdgeIdx, ui, true );
    }
    else
    {
      uiBsIdx = xCalcBsIdx( pcCU, uiScanIdx, iDir, iEdgeIdx, ui, false );
    }
    m_aapbEdgeFilter[iDir][0][uiBsIdx] = bValue;
    m_aapbEdgeFilter[iDir][1][uiBsIdx] = bValue;
    m_aapbEdgeFilter[iDir][2][uiBsIdx] = bValue;
    if (iEdgeIdx == 0)
    {
      m_aapucBS[iDir][uiBsIdx] = bValue;
    }
  }
}
#else
Void TComLoopFilter::xSetEdgefilterMultiple( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Int iDir, Int iEdgeIdx, Bool bValue )
{
  const UInt uiWidthInBaseUnits  = pcCU->getPic()->getNumPartInWidth () >> uiDepth;
  const UInt uiHeightInBaseUnits = pcCU->getPic()->getNumPartInHeight() >> uiDepth;
  const UInt uiNumElem = iDir == 0 ? uiHeightInBaseUnits : uiWidthInBaseUnits;
  assert( uiNumElem > 0 );
  for( UInt ui = 0; ui < uiNumElem; ui++ )
  {
    const UInt uiBsIdx = xCalcBsIdx( pcCU, uiAbsZorderIdx, iDir, iEdgeIdx, ui );
    m_aapbEdgeFilter[iDir][0][uiBsIdx] = bValue;
    m_aapbEdgeFilter[iDir][1][uiBsIdx] = bValue;
    m_aapbEdgeFilter[iDir][2][uiBsIdx] = bValue;
    if (iEdgeIdx == 0)
    {
      m_aapucBS[iDir][uiBsIdx] = bValue;
    }
  }
}
#endif

#if NSQT
Void TComLoopFilter::xSetEdgefilterTU( TComDataCU* pcCU, UInt uiRasterIdx, UInt uiAbsZorderIdx, UInt uiDepth )
#else
Void TComLoopFilter::xSetEdgefilterTU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth )
#endif
{
  if( pcCU->getTransformIdx( uiAbsZorderIdx ) + pcCU->getDepth( uiAbsZorderIdx) > uiDepth )
  {
    const UInt uiCurNumParts = pcCU->getPic()->getNumPartInCU() >> (uiDepth<<1);
    const UInt uiQNumParts   = uiCurNumParts>>2;
#if NSQT
    const UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> ( uiDepth + 1 ) ] + 2;
    if( !pcCU->isIntra( uiAbsZorderIdx ) && pcCU->getTransformIdx( uiAbsZorderIdx ) && uiLog2TrSize < pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && pcCU->getWidth( uiAbsZorderIdx ) > 8 && pcCU->getSlice()->getSPS()->getUseNSQT() &&
      ( pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_Nx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_2NxN || 
      ( pcCU->getPartitionSize( uiAbsZorderIdx ) >= SIZE_2NxnU && pcCU->getPartitionSize( uiAbsZorderIdx ) <= SIZE_nRx2N ) ) )
    {
      TComPic* const pcPic = pcCU->getPic();
      const UInt uiLCUWidthInBaseUnits = pcPic->getNumPartInWidth();

      if( ( ( 1 << uiLog2TrSize ) < ( pcCU->getSlice()->getSPS()->getMaxTrSize() >> 1 ) && pcCU->getDepth( uiAbsZorderIdx ) == uiDepth ) ||
        ( 1 << uiLog2TrSize ) == ( pcCU->getSlice()->getSPS()->getMaxTrSize() >> 1 ) ) 
      {
        const UInt iBaseUnitIdx = uiLCUWidthInBaseUnits >> ( uiDepth + 2 );
        UInt offset = ( pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_Nx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_nLx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_nRx2N ) ? iBaseUnitIdx : iBaseUnitIdx * uiLCUWidthInBaseUnits;
        for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx += uiQNumParts )
        {
          xSetEdgefilterTU( pcCU, uiRasterIdx+uiPartIdx * offset, uiAbsZorderIdx, uiDepth+1 );
        }
      }
      else
      {
        if( ( 1 << uiLog2TrSize ) < DEBLOCK_SMALLEST_BLOCK )
        {
          return;
        }
        UInt uiTrWidth  = ( pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_Nx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_nLx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_nRx2N ) ? 1 << ( uiLog2TrSize - 4 + 1) : 1 << ( uiLog2TrSize - 2 + 1 );
        UInt uiTrHeight = ( pcCU->getPartitionSize( uiAbsZorderIdx  )== SIZE_Nx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_nLx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_nRx2N ) ? 1 << ( uiLog2TrSize - 2 + 1) : 1 << ( uiLog2TrSize - 4 + 1 );
        xSetEdgefilterTU( pcCU, uiRasterIdx,                                                  uiAbsZorderIdx, uiDepth + 1 ); uiAbsZorderIdx += uiQNumParts;
        xSetEdgefilterTU( pcCU, uiRasterIdx + uiTrWidth,                                      uiAbsZorderIdx, uiDepth + 1 ); uiAbsZorderIdx += uiQNumParts;
        xSetEdgefilterTU( pcCU, uiRasterIdx + uiTrHeight * uiLCUWidthInBaseUnits,             uiAbsZorderIdx, uiDepth + 1 ); uiAbsZorderIdx += uiQNumParts;
        xSetEdgefilterTU( pcCU, uiRasterIdx + uiTrHeight * uiLCUWidthInBaseUnits + uiTrWidth, uiAbsZorderIdx, uiDepth + 1 );
      }
    }
    else
#endif 
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx+=uiQNumParts )
    {
#if NSQT
      xSetEdgefilterTU( pcCU,uiRasterIdx, uiAbsZorderIdx, uiDepth + 1 );
#else
      xSetEdgefilterTU( pcCU, uiAbsZorderIdx, uiDepth+1 );
#endif
    }
    return;
  }

#if NSQT
  const UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth ] + 2;
  if( !pcCU->isIntra( uiAbsZorderIdx ) && pcCU->getTransformIdx( uiAbsZorderIdx ) && uiLog2TrSize<pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && pcCU->getWidth( uiAbsZorderIdx ) > 8 && pcCU->getSlice()->getSPS()->getUseNSQT() &&
    ( pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_Nx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_2NxN || 
    ( pcCU->getPartitionSize( uiAbsZorderIdx ) >= SIZE_2NxnU && pcCU->getPartitionSize( uiAbsZorderIdx ) <= SIZE_nRx2N ) ) )
  {
    if( ( 1 << uiLog2TrSize ) < DEBLOCK_SMALLEST_BLOCK )
    {
      return;
    }
    
    UInt uiWidthInBaseUnits  = pcCU->getPic()->getNumPartInWidth () >> uiDepth;
    UInt uiHeightInBaseUnits = pcCU->getPic()->getNumPartInWidth () >> uiDepth;

    if( pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_Nx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_nLx2N || pcCU->getPartitionSize( uiAbsZorderIdx ) == SIZE_nRx2N )
    {
      uiWidthInBaseUnits  >>= 1;
      uiHeightInBaseUnits <<= 1;
    }
    else
    {
      uiWidthInBaseUnits  <<= 1;
      uiHeightInBaseUnits >>= 1;
    }
    xSetEdgefilterMultiple( pcCU, uiRasterIdx, uiDepth, EDGE_VER, 0, m_stLFCUParam.bInternalEdge, uiWidthInBaseUnits, uiHeightInBaseUnits );
    xSetEdgefilterMultiple( pcCU, uiRasterIdx, uiDepth, EDGE_HOR, 0, m_stLFCUParam.bInternalEdge, uiWidthInBaseUnits, uiHeightInBaseUnits );
  }
  else
  {
#endif
  xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, 0, m_stLFCUParam.bInternalEdge );
  xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, 0, m_stLFCUParam.bInternalEdge );
#if NSQT
  }
#endif
}

Void TComLoopFilter::xSetEdgefilterPU( TComDataCU* pcCU, UInt uiAbsZorderIdx )
{
  const UInt uiDepth = pcCU->getDepth( uiAbsZorderIdx );
  const UInt uiWidthInBaseUnits  = pcCU->getPic()->getNumPartInWidth () >> uiDepth;
  const UInt uiHeightInBaseUnits = pcCU->getPic()->getNumPartInHeight() >> uiDepth;
  const UInt uiHWidthInBaseUnits  = uiWidthInBaseUnits  >> 1;
  const UInt uiHHeightInBaseUnits = uiHeightInBaseUnits >> 1;
  const UInt uiQWidthInBaseUnits  = uiWidthInBaseUnits  >> 2;
  const UInt uiQHeightInBaseUnits = uiHeightInBaseUnits >> 2;
  
  xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, 0, m_stLFCUParam.bLeftEdge );
  xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, 0, m_stLFCUParam.bTopEdge );
  
  switch ( pcCU->getPartitionSize( uiAbsZorderIdx ) )
  {
    case SIZE_2Nx2N:
    {
      break;
    }
    case SIZE_2NxN:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, uiHHeightInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    case SIZE_Nx2N:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, uiHWidthInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    case SIZE_NxN:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, uiHWidthInBaseUnits, m_stLFCUParam.bInternalEdge );
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, uiHHeightInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
  case SIZE_2NxnU:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, uiQHeightInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
  case SIZE_2NxnD:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, uiHeightInBaseUnits - uiQHeightInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
  case SIZE_nLx2N:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, uiQWidthInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
  case SIZE_nRx2N:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, uiWidthInBaseUnits - uiQWidthInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    default:
    {
      assert(0);
      break;
    }
  }
}


Void TComLoopFilter::xSetLoopfilterParam( TComDataCU* pcCU, UInt uiAbsZorderIdx )
{
  UInt uiX           = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsZorderIdx ] ];
  UInt uiY           = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsZorderIdx ] ];
  
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;

  m_stLFCUParam.bInternalEdge = m_uiDisableDeblockingFilterIdc ? false : true ;
  
  if ( (uiX == 0) || (m_uiDisableDeblockingFilterIdc == 1) )
  {
    m_stLFCUParam.bLeftEdge = false;
  }
  else
  {
    m_stLFCUParam.bLeftEdge = true;
  }
  if ( m_stLFCUParam.bLeftEdge )
  {
#if NONCROSS_TILE_IN_LOOP_FILTERING
    pcTempCU = pcCU->getPULeft( uiTempPartIdx, uiAbsZorderIdx, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false, !m_bLFCrossTileBoundary);
#else
    pcTempCU = pcCU->getPULeft( uiTempPartIdx, uiAbsZorderIdx, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false );
#endif
    if ( pcTempCU )
    {
      m_stLFCUParam.bLeftEdge = true;
    }
    else
    {
      m_stLFCUParam.bLeftEdge = false;
    }
  }
  
  if ( (uiY == 0 ) || (m_uiDisableDeblockingFilterIdc == 1) )
  {
    m_stLFCUParam.bTopEdge = false;
  }
  else
  {
    m_stLFCUParam.bTopEdge = true;
  }
  if ( m_stLFCUParam.bTopEdge )
  {
#if NONCROSS_TILE_IN_LOOP_FILTERING
    pcTempCU = pcCU->getPUAbove( uiTempPartIdx, uiAbsZorderIdx, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false , false, false, !m_bLFCrossTileBoundary);
#else
    pcTempCU = pcCU->getPUAbove( uiTempPartIdx, uiAbsZorderIdx, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false );
#endif
    if ( pcTempCU )
    {
      m_stLFCUParam.bTopEdge = true;
    }
    else
    {
      m_stLFCUParam.bTopEdge = false;
    }
  }
}

Void TComLoopFilter::xGetBoundaryStrengthSingle ( TComDataCU* pcCU, UInt uiAbsZorderIdx, Int iDir, UInt uiAbsPartIdx )
{
  TComSlice* const pcSlice = pcCU->getSlice();
  
  const UInt uiPartQ = uiAbsPartIdx;
  TComDataCU* const pcCUQ = pcCU;
  
  UInt uiPartP;
  TComDataCU* pcCUP;
  UInt uiBs;
  
  //-- Calculate Block Index
  if (iDir == EDGE_VER)
  {
#if NONCROSS_TILE_IN_LOOP_FILTERING
    pcCUP = pcCUQ->getPULeft(uiPartP, uiPartQ, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false, !m_bLFCrossTileBoundary);
#else
    pcCUP = pcCUQ->getPULeft(uiPartP, uiPartQ, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false);
#endif
  }
  else  // (iDir == EDGE_HOR)
  {
#if NONCROSS_TILE_IN_LOOP_FILTERING
    pcCUP = pcCUQ->getPUAbove(uiPartP, uiPartQ, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false, false, false, !m_bLFCrossTileBoundary);
#else
    pcCUP = pcCUQ->getPUAbove(uiPartP, uiPartQ, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false);
#endif
  }
  
  //-- Set BS for Intra MB : BS = 4 or 3
  if ( pcCUP->isIntra(uiPartP) || pcCUQ->isIntra(uiPartQ) )
  {
    uiBs = 2;
  }
  
  //-- Set BS for not Intra MB : BS = 2 or 1 or 0
  if ( !pcCUP->isIntra(uiPartP) && !pcCUQ->isIntra(uiPartQ) )
  {
    if ( m_aapucBS[iDir][uiAbsPartIdx] && (pcCUQ->getCbf( uiPartQ, TEXT_LUMA, pcCUQ->getTransformIdx(uiPartQ)) != 0 || pcCUP->getCbf( uiPartP, TEXT_LUMA, pcCUP->getTransformIdx(uiPartP) ) != 0) )
    {
      uiBs = 1;
    }
    else
    {
      if (iDir == EDGE_HOR)
      {
#if NONCROSS_TILE_IN_LOOP_FILTERING
        pcCUP = pcCUQ->getPUAbove(uiPartP, uiPartQ, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false, true, false, !m_bLFCrossTileBoundary);
#else
        pcCUP = pcCUQ->getPUAbove(uiPartP, uiPartQ, !pcCU->getSlice()->getSPS()->getLFCrossSliceBoundaryFlag(), false, true);
#endif
      }
      if (pcSlice->isInterB())
      {
        Int iRefIdx;
        Int *piRefP0, *piRefP1, *piRefQ0, *piRefQ1;
        iRefIdx = pcCUP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartP);
        piRefP0 = (iRefIdx < 0) ? NULL :  (Int*) pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx);
        iRefIdx = pcCUP->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiPartP);
        piRefP1 = (iRefIdx < 0) ? NULL :  (Int*) pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx);
        iRefIdx = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartQ);
        piRefQ0 = (iRefIdx < 0) ? NULL :  (Int*) pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx);
        iRefIdx = pcCUQ->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiPartQ);
        piRefQ1 = (iRefIdx < 0) ? NULL :  (Int*) pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx);
        
        
        TComMv pcMvP0 = pcCUP->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartP);
        TComMv pcMvP1 = pcCUP->getCUMvField(REF_PIC_LIST_1)->getMv(uiPartP);
        TComMv pcMvQ0 = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartQ);
        TComMv pcMvQ1 = pcCUQ->getCUMvField(REF_PIC_LIST_1)->getMv(uiPartQ);
        
        if ( ((piRefP0==piRefQ0)&&(piRefP1==piRefQ1)) || ((piRefP0==piRefQ1)&&(piRefP1==piRefQ0)) )
        {
          uiBs = 0;
          if ( piRefP0 != piRefP1 )   // Different L0 & L1
          {
            if ( piRefP0 == piRefQ0 )
            {
              pcMvP0 -= pcMvQ0;   pcMvP1 -= pcMvQ1;
              uiBs = (pcMvP0.getAbsHor() >= 4) | (pcMvP0.getAbsVer() >= 4) |
              (pcMvP1.getAbsHor() >= 4) | (pcMvP1.getAbsVer() >= 4);
            }
            else
            {
              pcMvP0 -= pcMvQ1;   pcMvP1 -= pcMvQ0;
              uiBs = (pcMvP0.getAbsHor() >= 4) | (pcMvP0.getAbsVer() >= 4) |
              (pcMvP1.getAbsHor() >= 4) | (pcMvP1.getAbsVer() >= 4);
            }
          }
          else    // Same L0 & L1
          {
            TComMv pcMvSub0 = pcMvP0 - pcMvQ0;
            TComMv pcMvSub1 = pcMvP1 - pcMvQ1;
            pcMvP0 -= pcMvQ1;   pcMvP1 -= pcMvQ0;
            uiBs = ( (pcMvP0.getAbsHor() >= 4) | (pcMvP0.getAbsVer() >= 4) |
                    (pcMvP1.getAbsHor() >= 4) | (pcMvP1.getAbsVer() >= 4) ) &&
            ( (pcMvSub0.getAbsHor() >= 4) | (pcMvSub0.getAbsVer() >= 4) |
             (pcMvSub1.getAbsHor() >= 4) | (pcMvSub1.getAbsVer() >= 4) );
          }
        }
        else // for all different Ref_Idx
        {
          uiBs = 1;
        }
      }
      else  // pcSlice->isInterP()
      {
        Int iRefIdx;
        Int *piRefP0, *piRefQ0;
        iRefIdx = pcCUP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartP);
        piRefP0 = (iRefIdx < 0) ? NULL :  (Int*) pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx);
        iRefIdx = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartQ);
        piRefQ0 = (iRefIdx < 0) ? NULL :  (Int*) pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx);
        TComMv pcMvP0 = pcCUP->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartP);
        TComMv pcMvQ0 = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartQ);
        
        pcMvP0 -= pcMvQ0;
        uiBs = (piRefP0 != piRefQ0) | (pcMvP0.getAbsHor() >= 4) | (pcMvP0.getAbsVer() >= 4);
      }
    }   // enf of "if( one of BCBP == 0 )"
  }   // enf of "if( not Intra )"
  
  m_aapucBS[iDir][uiAbsPartIdx] = uiBs;
}


Void TComLoopFilter::xEdgeFilterLuma( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Int iDir, Int iEdge  )
{
  TComPicYuv* pcPicYuvRec = pcCU->getPic()->getPicYuvRec();
  Pel* piSrc    = pcPicYuvRec->getLumaAddr( pcCU->getAddr(), uiAbsZorderIdx );
  Pel* piTmpSrc = piSrc;

  Int  iStride = pcPicYuvRec->getStride();
  Int iQP = 0;
  Int iQP_P = 0;
  Int iQP_Q = 0;
  UInt uiNumParts = pcCU->getPic()->getNumPartInWidth()>>uiDepth;
  
  UInt  uiPelsInPart = g_uiMaxCUWidth >> g_uiMaxCUDepth;
  UInt  PartIdxIncr = DEBLOCK_SMALLEST_BLOCK / uiPelsInPart ? DEBLOCK_SMALLEST_BLOCK / uiPelsInPart : 1;
  UInt  uiBlocksInPart = uiPelsInPart / DEBLOCK_SMALLEST_BLOCK ? uiPelsInPart / DEBLOCK_SMALLEST_BLOCK : 1;
  UInt  uiBsAbsIdx = 0, uiBs = 0;
  Int   iOffset, iSrcStep;

#if MAX_PCM_SIZE
  Bool  bPCMFilter = (pcCU->getSlice()->getSPS()->getUsePCM() && pcCU->getSlice()->getSPS()->getPCMFilterDisableFlag())? true : false;
#else
  Bool  bPCMFilter = (pcCU->getSlice()->getSPS()->getPCMFilterDisableFlag() && ((1<<pcCU->getSlice()->getSPS()->getPCMLog2MinSize()) <= g_uiMaxCUWidth))? true : false;
#endif
  Bool  bPartPNoFilter = false;
  Bool  bPartQNoFilter = false; 
  UInt  uiPartPIdx = 0;
  UInt  uiPartQIdx = 0;
  TComDataCU* pcCUP = pcCU; 
  TComDataCU* pcCUQ = pcCU;

  if (iDir == EDGE_VER)
  {
    iOffset = 1;
    iSrcStep = iStride;
    piTmpSrc += iEdge*uiPelsInPart;
  }
  else  // (iDir == EDGE_HOR)
  {
    iOffset = iStride;
    iSrcStep = 1;
    piTmpSrc += iEdge*uiPelsInPart*iStride;
  }
  
  for ( UInt iIdx = 0; iIdx < uiNumParts; iIdx+=PartIdxIncr )
  {
    
    uiBs = 0;
    for (UInt iIdxInside = 0; iIdxInside<PartIdxIncr; iIdxInside++)
    {
      uiBsAbsIdx = xCalcBsIdx( pcCU, uiAbsZorderIdx, iDir, iEdge, iIdx+iIdxInside);
      if (uiBs < m_aapucBS[iDir][uiBsAbsIdx])
      {
        uiBs = m_aapucBS[iDir][uiBsAbsIdx];
      }
    }
    
    if ( uiBs )
    {
      iQP_Q = pcCU->getQP( uiBsAbsIdx );
      uiPartQIdx = uiBsAbsIdx;
      // Derive neighboring PU index
      if (iDir == EDGE_VER)
      {
        pcCUP = pcCUQ->getPULeft (uiPartPIdx, uiPartQIdx);
      }
      else  // (iDir == EDGE_HOR)
      {
        pcCUP = pcCUQ->getPUAbove(uiPartPIdx, uiPartQIdx);
      }
      if (!pcCUP)
      {
        return;
      }

      iQP_P = pcCUP->getQP(uiPartPIdx);

      if(pcCU->getIPCMFlag(uiPartQIdx)) 
      {
        iQP_Q = 0; 
      }
      if(pcCUP->getIPCMFlag(uiPartPIdx)) 
      {
        iQP_P = 0; 
      }

      iQP = (iQP_P + iQP_Q + 1) >> 1;
    Int iBitdepthScale = (1<<(g_uiBitIncrement+g_uiBitDepth-8));
    
    Int iIndexTC = Clip3(0, MAX_QP+DEFAULT_INTRA_TC_OFFSET, Int(iQP + DEFAULT_INTRA_TC_OFFSET*(uiBs-1) + (m_tcOffsetDiv2 << 1)));
    Int iIndexB = Clip3(0, MAX_QP, iQP + (m_betaOffsetDiv2 << 1));

    Int iTc =  tctable_8x8[iIndexTC]*iBitdepthScale;
    Int iBeta = betatable_8x8[iIndexB]*iBitdepthScale;
    Int iSideThreshold = (iBeta+(iBeta>>1))>>3;
    Int iThrCut = iTc*10;
    
    
    for (UInt iBlkIdx = 0; iBlkIdx< uiBlocksInPart; iBlkIdx ++)
    {
        Int dp0 = xCalcDP( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+0), iOffset);
        Int dq0 = xCalcDQ( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+0), iOffset);
        Int dp3 = xCalcDP( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+3), iOffset);
        Int dq3 = xCalcDQ( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+3), iOffset);
        Int d0 = dp0 + dq0;
        Int d3 = dp3 + dq3;

        Int dp4 = xCalcDP( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+4), iOffset);
        Int dq4 = xCalcDQ( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+4), iOffset);
        Int dp7 = xCalcDP( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+7), iOffset);
        Int dq7 = xCalcDQ( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+7), iOffset);
        Int d4 = dp4 + dq4;
        Int d7 = dp7 + dq7;

        if (bPCMFilter)
        {
          // Check if each of PUs is I_PCM
          bPartPNoFilter = (pcCUP->getIPCMFlag(uiPartPIdx));
          bPartQNoFilter = (pcCUQ->getIPCMFlag(uiPartQIdx));
        }
        if (d0+d3 < iBeta)
        {
          Bool bFilterP = (dp0+dp3 < iSideThreshold);
          Bool bFilterQ = (dq0+dq3 < iSideThreshold);

          Bool sw =  xUseStrongFiltering( iOffset, 2*d0, iBeta, iTc , piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+0))
                  && xUseStrongFiltering( iOffset, 2*d3, iBeta, iTc , piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+3));

          for ( Int i = 0; i < DEBLOCK_SMALLEST_BLOCK/2; i++)
          {
            xPelFilterLuma( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+i), iOffset, d0+d3, iBeta, iTc, sw, bPartPNoFilter, bPartQNoFilter, iThrCut, bFilterP, bFilterQ);
          }
        }
        if (d4+d7 < iBeta)
        {
          Bool bFilterP = (dp4+dp7 < iSideThreshold);
          Bool bFilterQ = (dq4+dq7 < iSideThreshold);

          Bool sw =  xUseStrongFiltering( iOffset, 2*d4, iBeta, iTc , piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+4))
                  && xUseStrongFiltering( iOffset, 2*d7, iBeta, iTc , piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+7));
          for ( Int i = DEBLOCK_SMALLEST_BLOCK/2; i < DEBLOCK_SMALLEST_BLOCK; i++)
          {
            xPelFilterLuma( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*DEBLOCK_SMALLEST_BLOCK+i), iOffset, d4+d7, iBeta, iTc, sw, bPartPNoFilter, bPartQNoFilter, iThrCut, bFilterP, bFilterQ);
          }
        }
      }
    }
  }
}


Void TComLoopFilter::xEdgeFilterChroma( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Int iDir, Int iEdge )
{
  TComPicYuv* pcPicYuvRec = pcCU->getPic()->getPicYuvRec();
  Int         iStride     = pcPicYuvRec->getCStride();
  Pel*        piSrcCb     = pcPicYuvRec->getCbAddr( pcCU->getAddr(), uiAbsZorderIdx );
  Pel*        piSrcCr     = pcPicYuvRec->getCrAddr( pcCU->getAddr(), uiAbsZorderIdx );
  Int iQP = 0;
  Int iQP_P = 0;
  Int iQP_Q = 0;

  UInt  uiPelsInPartChroma = g_uiMaxCUWidth >> (g_uiMaxCUDepth+1);
  
  Int   iOffset, iSrcStep;
  
  const UInt uiLCUWidthInBaseUnits = pcCU->getPic()->getNumPartInWidth();
  
#if MAX_PCM_SIZE
  Bool  bPCMFilter = (pcCU->getSlice()->getSPS()->getUsePCM() && pcCU->getSlice()->getSPS()->getPCMFilterDisableFlag())? true : false;
#else
  Bool  bPCMFilter = (pcCU->getSlice()->getSPS()->getPCMFilterDisableFlag() && ((1<<pcCU->getSlice()->getSPS()->getPCMLog2MinSize()) <= g_uiMaxCUWidth))? true : false;
#endif
  Bool  bPartPNoFilter = false;
  Bool  bPartQNoFilter = false; 
  UInt  uiPartPIdx;
  UInt  uiPartQIdx;
  TComDataCU* pcCUP; 
  TComDataCU* pcCUQ = pcCU;

  // Vertical Position
  UInt uiEdgeNumInLCUVert = g_auiZscanToRaster[uiAbsZorderIdx]%uiLCUWidthInBaseUnits + iEdge;
  UInt uiEdgeNumInLCUHor = g_auiZscanToRaster[uiAbsZorderIdx]/uiLCUWidthInBaseUnits + iEdge;
  
  if ( ( (uiEdgeNumInLCUVert%(DEBLOCK_SMALLEST_BLOCK/uiPelsInPartChroma))&&(iDir==0) ) || ( (uiEdgeNumInLCUHor%(DEBLOCK_SMALLEST_BLOCK/uiPelsInPartChroma))&& iDir ) )
  {
    return;
  }
  
  UInt  uiNumParts = pcCU->getPic()->getNumPartInWidth()>>uiDepth;
  
  UInt  uiBsAbsIdx;
  UChar ucBs;
  
  Pel* piTmpSrcCb = piSrcCb;
  Pel* piTmpSrcCr = piSrcCr;
  
  
  if (iDir == EDGE_VER)
  {
    iOffset   = 1;
    iSrcStep  = iStride;
    piTmpSrcCb += iEdge*uiPelsInPartChroma;
    piTmpSrcCr += iEdge*uiPelsInPartChroma;
  }
  else  // (iDir == EDGE_HOR)
  {
    iOffset   = iStride;
    iSrcStep  = 1;
    piTmpSrcCb += iEdge*iStride*uiPelsInPartChroma;
    piTmpSrcCr += iEdge*iStride*uiPelsInPartChroma;
  }
  
  for ( UInt iIdx = 0; iIdx < uiNumParts; iIdx++ )
  {
    ucBs = 0;
    
    uiBsAbsIdx = xCalcBsIdx( pcCU, uiAbsZorderIdx, iDir, iEdge, iIdx);
    ucBs = m_aapucBS[iDir][uiBsAbsIdx];
    
    if ( ucBs > 1)
    {
      iQP_Q = pcCU->getQP( uiBsAbsIdx );
      uiPartQIdx = uiBsAbsIdx;
      // Derive neighboring PU index
      if (iDir == EDGE_VER)
      {
        pcCUP = pcCUQ->getPULeft (uiPartPIdx, uiPartQIdx);
      }
      else  // (iDir == EDGE_HOR)
      {
        pcCUP = pcCUQ->getPUAbove(uiPartPIdx, uiPartQIdx);
      }
      if (!pcCUP)
      {
        return;
      }

      iQP_P = pcCUP->getQP(uiPartPIdx);

      if(pcCU->getIPCMFlag(uiPartQIdx)) 
      {
        iQP_Q = 0; 
      }
      if(pcCUP->getIPCMFlag(uiPartPIdx)) 
      {
        iQP_P = 0; 
      }

      iQP = QpUV((iQP_P + iQP_Q + 1) >> 1);
    Int iBitdepthScale = (1<<(g_uiBitIncrement+g_uiBitDepth-8));
    
    Int iIndexTC = Clip3(0, MAX_QP+DEFAULT_INTRA_TC_OFFSET, iQP + DEFAULT_INTRA_TC_OFFSET*(ucBs - 1) + (m_tcOffsetDiv2 << 1));
    Int iTc =  tctable_8x8[iIndexTC]*iBitdepthScale;
    
      if(bPCMFilter)
      {
        // Check if each of PUs is IPCM
        bPartPNoFilter = (pcCUP->getIPCMFlag(uiPartPIdx));
        bPartQNoFilter = (pcCUQ->getIPCMFlag(uiPartQIdx));
      }

      for ( UInt uiStep = 0; uiStep < uiPelsInPartChroma; uiStep++ )
      {
        xPelFilterChroma( piTmpSrcCb + iSrcStep*(uiStep+iIdx*uiPelsInPartChroma), iOffset, iTc , bPartPNoFilter, bPartQNoFilter);
        xPelFilterChroma( piTmpSrcCr + iSrcStep*(uiStep+iIdx*uiPelsInPartChroma), iOffset, iTc , bPartPNoFilter, bPartQNoFilter);
      }
    }
  }
}

/**
 - Deblocking for the luminance component with strong or weak filter
 .
 \param piSrc           pointer to picture data
 \param iOffset         offset value for picture data
 \param d               d value
 \param beta            beta value
 \param tc              tc value
 \param sw              decision strong/weak filter
 \param bPartPNoFilter  indicator to disable filtering on partP
 \param bPartQNoFilter  indicator to disable filtering on partQ
 \param iThrCut         threshold value for weak filter decision
 \param bFilterSecondP  decision weak filter/no filter for partP
 \param bFilterSecondQ  decision weak filter/no filter for partQ
*/
__inline Void TComLoopFilter::xPelFilterLuma( Pel* piSrc, Int iOffset, Int d, Int beta, Int tc , Bool sw, Bool bPartPNoFilter, Bool bPartQNoFilter, Int iThrCut, Bool bFilterSecondP, Bool bFilterSecondQ)
{
  Int delta;
  
  Pel m4  = piSrc[0];
  Pel m3  = piSrc[-iOffset];
  Pel m5  = piSrc[ iOffset];
  Pel m2  = piSrc[-iOffset*2];
  Pel m6  = piSrc[ iOffset*2];
  Pel m1  = piSrc[-iOffset*3];
  Pel m7  = piSrc[ iOffset*3];
  Pel m0  = piSrc[-iOffset*4];

  if (sw)
  {
    piSrc[-iOffset] = ( m1 + 2*m2 + 2*m3 + 2*m4 + m5 + 4) >> 3;
    piSrc[0] = ( m2 + 2*m3 + 2*m4 + 2*m5 + m6 + 4) >> 3;
    
    piSrc[-iOffset*2] = ( m1 + m2 + m3 + m4 + 2)>>2;
    piSrc[ iOffset] = ( m3 + m4 + m5 + m6 + 2)>>2;
    
    piSrc[-iOffset*3] = ( 2*m0 + 3*m1 + m2 + m3 + m4 + 4 )>>3;
    piSrc[ iOffset*2] = ( m3 + m4 + m5 + 3*m6 + 2*m7 +4 )>>3;
  }
  else
  {
    /* Weak filter */
    delta = (9*(m4-m3) -3*(m5-m2) + 8)>>4 ;

    if ( abs(delta) < iThrCut )
    {
      delta = Clip3(-tc, tc, delta);        
      piSrc[-iOffset] = Clip((m3+delta));
      piSrc[0] = Clip((m4-delta));

      Int tc2 = tc>>1;
      if(bFilterSecondP)
      {
        Int delta1 = Clip3(-tc2, tc2, (( ((m1+m3+1)>>1)- m2+delta)>>1));
        piSrc[-iOffset*2] = Clip((m2+delta1));
      }
      if(bFilterSecondQ)
      {
        Int delta2 = Clip3(-tc2, tc2, (( ((m6+m4+1)>>1)- m5-delta)>>1));
        piSrc[ iOffset] = Clip((m5+delta2));
      }
    }
  }

  if(bPartPNoFilter)
  {
    piSrc[-iOffset] = m3;
    piSrc[-iOffset*2] = m2;
    piSrc[-iOffset*3] = m1;
  }
  if(bPartQNoFilter)
  {
    piSrc[0] = m4;
    piSrc[ iOffset] = m5;
    piSrc[ iOffset*2] = m6;
  }
}

/**
 - Deblocking of one line/column for the chrominance component
 .
 \param piSrc           pointer to picture data
 \param iOffset         offset value for picture data
 \param tc              tc value
 \param bPartPNoFilter  indicator to disable filtering on partP
 \param bPartQNoFilter  indicator to disable filtering on partQ
 */
__inline Void TComLoopFilter::xPelFilterChroma( Pel* piSrc, Int iOffset, Int tc, Bool bPartPNoFilter, Bool bPartQNoFilter)
{
  int delta;
  
  Pel m4  = piSrc[0];
  Pel m3  = piSrc[-iOffset];
  Pel m5  = piSrc[ iOffset];
  Pel m2  = piSrc[-iOffset*2];
  
  delta = Clip3(-tc,tc, (((( m4 - m3 ) << 2 ) + m2 - m5 + 4 ) >> 3) );
  piSrc[-iOffset] = Clip(m3+delta);
  piSrc[0] = Clip(m4-delta);

  if(bPartPNoFilter)
  {
    piSrc[-iOffset] = m3;
  }
  if(bPartQNoFilter)
  {
    piSrc[0] = m4;
  }
}

/**
 - Decision between strong and weak filter
 .
 \param offset         offset value for picture data
 \param d               d value
 \param beta            beta value
 \param tc              tc value
 \param piSrc           pointer to picture data
 */
__inline Bool TComLoopFilter::xUseStrongFiltering( Int offset, Int d, Int beta, Int tc, Pel* piSrc)
{
  Pel m4  = piSrc[0];
  Pel m3  = piSrc[-offset];
  Pel m7  = piSrc[ offset*3];
  Pel m0  = piSrc[-offset*4];

  Int d_strong = abs(m0-m3) + abs(m7-m4);

  return ( (d_strong < (beta>>3)) && (d<(beta>>2)) && ( abs(m3-m4) < ((tc*5+1)>>1)) );
}

__inline Int TComLoopFilter::xCalcDP( Pel* piSrc, Int iOffset)
{
  return abs( piSrc[-iOffset*3] - 2*piSrc[-iOffset*2] + piSrc[-iOffset] ) ;
}
  
__inline Int TComLoopFilter::xCalcDQ( Pel* piSrc, Int iOffset)
{
  return abs( piSrc[0] - 2*piSrc[iOffset] + piSrc[iOffset*2] );
}
//! \}
