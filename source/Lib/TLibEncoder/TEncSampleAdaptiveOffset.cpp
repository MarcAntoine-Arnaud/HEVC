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

/** \file     TEncSampleAdaptiveOffset.cpp
 \brief       estimation part of sample adaptive offset class
 */
#include "TEncSampleAdaptiveOffset.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//! \ingroup TLibEncoder
//! \{

TEncSampleAdaptiveOffset::TEncSampleAdaptiveOffset()
{
  m_pcEntropyCoder = NULL;
  m_pppcRDSbacCoder = NULL;
  m_pcRDGoOnSbacCoder = NULL;
  m_pppcBinCoderCABAC = NULL;            
  m_iCount = NULL;     
  m_iOffset = NULL;      
  m_iOffsetOrg = NULL;  
  m_iRate = NULL;       
  m_iDist = NULL;        
  m_dCost = NULL;        
  m_dCostPartBest = NULL; 
  m_iDistOrg = NULL;      
  m_iTypePartBest = NULL; 
}
TEncSampleAdaptiveOffset::~TEncSampleAdaptiveOffset()
{

}

// ====================================================================================================================
// Tables
// ====================================================================================================================
inline Double xRoundIbdi2(Double x)
{
#if FULL_NBIT
  Int bitDepthMinus8 = g_uiBitDepth - 8;
  return ((x)>0) ? (Int)(((Int)(x)+(1<<(bitDepthMinus8-1)))/(1<<bitDepthMinus8)) : ((Int)(((Int)(x)-(1<<(bitDepthMinus8-1)))/(1<<bitDepthMinus8)));
#else
  return ((x)>0) ? (Int)(((Int)(x)+(1<<(g_uiBitIncrement-1)))/(1<<g_uiBitIncrement)) : ((Int)(((Int)(x)-(1<<(g_uiBitIncrement-1)))/(1<<g_uiBitIncrement)));
#endif
}

/** rounding with IBDI
 * \param  x
 */
inline Double xRoundIbdi(Double x)
{
#if FULL_NBIT
  return (g_uiBitDepth > 8 ? xRoundIbdi2((x)) : ((x)>=0 ? ((Int)((x)+0.5)) : ((Int)((x)-0.5)))) ;
#else
  return (g_uiBitIncrement >0 ? xRoundIbdi2((x)) : ((x)>=0 ? ((Int)((x)+0.5)) : ((Int)((x)-0.5)))) ;
#endif
}

/** process SAO for one partition
 * \param  *psQTPart, iPartIdx, dLambda
 */
Void TEncSampleAdaptiveOffset::rdoSaoOnePart(SAOQTPart *psQTPart, Int iPartIdx, Double dLambda)
{
  Int iTypeIdx;
  Int iNumTotalType = MAX_NUM_SAO_TYPE;
  SAOQTPart*  pOnePart = &(psQTPart[iPartIdx]);

  Int64 iEstDist;
  Int64 iOffsetOrg;
  Int64 iOffset;
  Int64 iCount;
  Int iClassIdx;
  Int uiShift = g_uiBitIncrement << 1;
  UInt uiDepth = pOnePart->partLevel;

  m_iDistOrg [iPartIdx] =  0;

  Double  bestRDCostTableBo = MAX_DOUBLE;
  Int     bestClassTableBo    = 0;
  Int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
  Double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];

  for (iTypeIdx=-1; iTypeIdx<iNumTotalType; iTypeIdx++)
  {
    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
      m_pcRDGoOnSbacCoder->resetBits();
    }
    else
    {
      m_pcEntropyCoder->resetEntropy();
      m_pcEntropyCoder->resetBits();
    }

    iEstDist = 0;

    m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoTypeIdx(iTypeIdx+1);

    if (iTypeIdx>=0)
    {

      for(iClassIdx=1; iClassIdx < ( (iTypeIdx < SAO_BO) ?  MAX_NUM_SAO_OFFSETS+1 : SAO_MAX_BO_CLASSES+1); iClassIdx++)
      {
        if( iTypeIdx == SAO_BO)
        {
          currentDistortionTableBo[iClassIdx-1] = 0;
          currentRdCostTableBo[iClassIdx-1] = dLambda;
        }
        if(m_iCount [iPartIdx][iTypeIdx][iClassIdx])
        {
#if FULL_NBIT
          m_iOffset[iPartIdx][iTypeIdx][iClassIdx] = (Int64) xRoundIbdi((Double)(m_iOffsetOrg[iPartIdx][iTypeIdx][iClassIdx]<<g_uiBitDepth-8) / (Double)(m_iCount [iPartIdx][iTypeIdx][iClassIdx]<<m_uiSaoBitIncrease));
#else
          m_iOffset[iPartIdx][iTypeIdx][iClassIdx] = (Int64) xRoundIbdi((Double)(m_iOffsetOrg[iPartIdx][iTypeIdx][iClassIdx]<<g_uiBitIncrement) / (Double)(m_iCount [iPartIdx][iTypeIdx][iClassIdx]<<m_uiSaoBitIncrease));
#endif
          m_iOffset[iPartIdx][iTypeIdx][iClassIdx] = Clip3(-m_iOffsetTh, m_iOffsetTh-1, (Int)m_iOffset[iPartIdx][iTypeIdx][iClassIdx]);

          if (iTypeIdx < 4)
          {
            if ( m_iOffset[iPartIdx][iTypeIdx][iClassIdx]<0 && iClassIdx<3 )
            {
              m_iOffset[iPartIdx][iTypeIdx][iClassIdx] = 0;
            }
            if ( m_iOffset[iPartIdx][iTypeIdx][iClassIdx]>0 && iClassIdx>=3)
            {
              m_iOffset[iPartIdx][iTypeIdx][iClassIdx] = 0;
            }
          }
          {
            //Clean up, best_q_offset.
            Int64 iIterOffset, iTempOffset;
            Int64 iTempDist, iTempRate;
            Double dTempCost, dTempMinCost;
            UInt uiLength, uiTemp;

            iIterOffset = m_iOffset[iPartIdx][iTypeIdx][iClassIdx];
            m_iOffset[iPartIdx][iTypeIdx][iClassIdx] = 0;
            dTempMinCost = dLambda; // Assuming sending quantized value 0 results in zero offset and sending the value zero needs 1 bit. entropy coder can be used to measure the exact rate here. 

            while (iIterOffset != 0)
            {
              // Calculate the bits required for signalling the offset
              uiLength = 1;
              uiTemp = (UInt)((iIterOffset <= 0) ? ( (-iIterOffset<<1) + 1 ) : (iIterOffset<<1));
              while( 1 != uiTemp )
              {
                uiTemp >>= 1;
                uiLength += 2;
              }
              iTempRate = (uiLength >> 1) + ((uiLength+1) >> 1);

              // Do the dequntization before distorion calculation
              iTempOffset    =  iIterOffset << m_uiSaoBitIncrease;
              iTempDist  = (( m_iCount [iPartIdx][iTypeIdx][iClassIdx]*iTempOffset*iTempOffset-m_iOffsetOrg[iPartIdx][iTypeIdx][iClassIdx]*iTempOffset*2 ) >> uiShift);

              dTempCost = ((Double)iTempDist + dLambda * (Double) iTempRate);
              if(dTempCost < dTempMinCost)
              {
                dTempMinCost = dTempCost;
                m_iOffset[iPartIdx][iTypeIdx][iClassIdx] = iIterOffset;
                if(iTypeIdx == SAO_BO)
                {
                  currentDistortionTableBo[iClassIdx-1] = (Int) iTempDist;
                  currentRdCostTableBo[iClassIdx-1] = dTempCost;
                }
              }
              iIterOffset = (iIterOffset > 0) ? (iIterOffset-1):(iIterOffset+1);
            }

          }
        }
        else
        {
          m_iOffsetOrg[iPartIdx][iTypeIdx][iClassIdx] = 0;
          m_iOffset[iPartIdx][iTypeIdx][iClassIdx] = 0;
        }
        if( iTypeIdx != SAO_BO )
        {
          iCount     =  m_iCount [iPartIdx][iTypeIdx][iClassIdx];
          iOffset    =  m_iOffset[iPartIdx][iTypeIdx][iClassIdx] << m_uiSaoBitIncrease;
          iOffsetOrg =  m_iOffsetOrg[iPartIdx][iTypeIdx][iClassIdx];
          iEstDist   += (( iCount*iOffset*iOffset-iOffsetOrg*iOffset*2 ) >> uiShift);
          if (iTypeIdx < 4)
          {
            if (iClassIdx<3)
            {
              m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoUvlc((Int)m_iOffset[iPartIdx][iTypeIdx][iClassIdx]);
            }
            else
            {
              m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoUvlc((Int)-m_iOffset[iPartIdx][iTypeIdx][iClassIdx]);
            }
          }
          else
          {
            m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoSvlc((Int)m_iOffset[iPartIdx][iTypeIdx][iClassIdx]);
          }
        }
      }

      if( iTypeIdx == SAO_BO )
      {
        // Estimate Best Position
        Double currentRDCost = 0.0;

        for(Int i=0; i< SAO_MAX_BO_CLASSES -SAO_BO_LEN +1; i++)
        {
          currentRDCost = 0.0;
          for(UInt uj = i; uj < i+SAO_BO_LEN; uj++)
          {
            currentRDCost += currentRdCostTableBo[uj];
          }

          if( currentRDCost < bestRDCostTableBo)
          {
            bestRDCostTableBo = currentRDCost;
            bestClassTableBo  = i;
          }
        }

        // Re code all Offsets
        // Code Center
        m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoUflc( (UInt) (bestClassTableBo) );

        for(iClassIdx = bestClassTableBo; iClassIdx < bestClassTableBo+SAO_BO_LEN; iClassIdx++)
        {
          m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoSvlc((Int)m_iOffset[iPartIdx][iTypeIdx][iClassIdx+1]);
          iEstDist += currentDistortionTableBo[iClassIdx];
        }
      }

      m_iDist[iPartIdx][iTypeIdx] = iEstDist;
      m_iRate[iPartIdx][iTypeIdx] = m_pcEntropyCoder->getNumberOfWrittenBits();

      m_dCost[iPartIdx][iTypeIdx] = (Double)((Double)m_iDist[iPartIdx][iTypeIdx] + dLambda * (Double) m_iRate[iPartIdx][iTypeIdx]);

      if(m_dCost[iPartIdx][iTypeIdx] < m_dCostPartBest[iPartIdx])
      {
        m_iDistOrg [iPartIdx] = 0;
        m_dCostPartBest[iPartIdx] = m_dCost[iPartIdx][iTypeIdx];
        m_iTypePartBest[iPartIdx] = iTypeIdx;
        if( m_bUseSBACRD )
          m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[pOnePart->partLevel][CI_TEMP_BEST] );
      }
    }
    else
    {
      if(m_iDistOrg[iPartIdx] < m_dCostPartBest[iPartIdx] )
      {
        m_dCostPartBest[iPartIdx] = (Double) m_iDistOrg[iPartIdx] + m_pcEntropyCoder->getNumberOfWrittenBits()*dLambda ; 
        m_iTypePartBest[iPartIdx] = -1;
        if( m_bUseSBACRD )
          m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[pOnePart->partLevel][CI_TEMP_BEST] );
      }
    }
  }

  pOnePart->processed = true;
  pOnePart->split     = false;
  pOnePart->minDist   =        m_iTypePartBest[iPartIdx] >= 0 ? m_iDist[iPartIdx][m_iTypePartBest[iPartIdx]] : m_iDistOrg[iPartIdx];
  pOnePart->minRate   = (Int) (m_iTypePartBest[iPartIdx] >= 0 ? m_iRate[iPartIdx][m_iTypePartBest[iPartIdx]] : 0);
  pOnePart->minCost   = pOnePart->minDist + dLambda * pOnePart->minRate;
  pOnePart->typeIdx  = m_iTypePartBest[iPartIdx];
  if (pOnePart->typeIdx != -1)
  {
    //     pOnePart->bEnableFlag =  1;
    pOnePart->length = MAX_NUM_SAO_OFFSETS;
    Int minIndex = 0;
    if( pOnePart->typeIdx == SAO_BO )
    {
      pOnePart->bandPosition = bestClassTableBo;
      minIndex = pOnePart->bandPosition;
    }
    for (Int i=0; i< pOnePart->length ; i++)
    {
      pOnePart->offset[i] = (Int) m_iOffset[iPartIdx][pOnePart->typeIdx][minIndex+i+1];
    }

  }
  else
  {
    //     pOnePart->bEnableFlag = 0;
    pOnePart->length     = 0;
  }
}


/** Run partition tree disable
 */
Void TEncSampleAdaptiveOffset::disablePartTree(SAOQTPart *psQTPart, Int iPartIdx)
{
  SAOQTPart*  pOnePart= &(psQTPart[iPartIdx]);
  pOnePart->split      = false;
  pOnePart->length     =  0;
  pOnePart->typeIdx   = -1;

  if (pOnePart->partLevel < m_uiMaxSplitLevel)
  {
    for (Int i=0; i<NUM_DOWN_PART; i++)
    {
      disablePartTree(psQTPart, pOnePart->downPartsIdx[i]);
    }
  }
}

/** Run quadtree decision function
 * \param  iPartIdx, pcPicOrg, pcPicDec, pcPicRest, &dCostFinal
 */
Void TEncSampleAdaptiveOffset::runQuadTreeDecision(SAOQTPart *psQTPart, Int iPartIdx, Double &dCostFinal, Int iMaxLevel, Double dLambda)
{
  SAOQTPart*  pOnePart = &(psQTPart[iPartIdx]);

  UInt uiDepth = pOnePart->partLevel;
  UInt uhNextDepth = uiDepth+1;

  if (iPartIdx == 0)
  {
    dCostFinal = 0;
  }

  //SAO for this part
  if(!pOnePart->processed)
  {
    rdoSaoOnePart (psQTPart, iPartIdx, dLambda);
  }

  //SAO for sub 4 parts
  if (pOnePart->partLevel < iMaxLevel)
  {
    Double      dCostNotSplit = dLambda + pOnePart->minCost;
    Double      dCostSplit    = dLambda;

    for (Int i=0; i< NUM_DOWN_PART ;i++)
    {
      if( m_bUseSBACRD )  
      {
        if ( 0 == i) //initialize RD with previous depth buffer
        {
          m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
        }
        else
        {
          m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
        }
      }  
      runQuadTreeDecision(psQTPart, pOnePart->downPartsIdx[i], dCostFinal, iMaxLevel, dLambda);
      dCostSplit += dCostFinal;
      if( m_bUseSBACRD )
      {
        m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_TEMP_BEST]);
      }
    }

    if(dCostSplit < dCostNotSplit)
    {
      dCostFinal = dCostSplit;
      pOnePart->split      = true;
      pOnePart->length     =  0;
      pOnePart->typeIdx   = -1;
      if( m_bUseSBACRD )
      {
        m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
      }
    }
    else
    {
      dCostFinal = dCostNotSplit;
      pOnePart->split = false;
      for (Int i=0; i<NUM_DOWN_PART; i++)
      {
        disablePartTree(psQTPart, pOnePart->downPartsIdx[i]);
      }
      if( m_bUseSBACRD )
      {
        m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);
      }
    }
  }
  else
  {
    dCostFinal = pOnePart->minCost;
  }
}

/** delete allocated memory of TEncSampleAdaptiveOffset class.
 */
Void TEncSampleAdaptiveOffset::destroyEncBuffer()
{
  for (Int i=0;i<m_iNumTotalParts;i++)
  {
    for (Int j=0;j<MAX_NUM_SAO_TYPE;j++)
    {
      if (m_iCount [i][j])
      {
        delete [] m_iCount [i][j]; 
      }
      if (m_iOffset[i][j])
      {
        delete [] m_iOffset[i][j]; 
      }
      if (m_iOffsetOrg[i][j])
      {
        delete [] m_iOffsetOrg[i][j]; 
      }
    }
    if (m_iRate[i])
    {
      delete [] m_iRate[i];
    }
    if (m_iDist[i])
    {
      delete [] m_iDist[i]; 
    }
    if (m_dCost[i])
    {
      delete [] m_dCost[i]; 
    }
    if (m_iCount [i])
    {
      delete [] m_iCount [i]; 
    }
    if (m_iOffset[i])
    {
      delete [] m_iOffset[i]; 
    }
    if (m_iOffsetOrg[i])
    {
      delete [] m_iOffsetOrg[i]; 
    }

  }
  if (m_iDistOrg)
  {
    delete [] m_iDistOrg ; m_iDistOrg = NULL;
  }
  if (m_dCostPartBest)
  {
    delete [] m_dCostPartBest ; m_dCostPartBest = NULL;
  }
  if (m_iTypePartBest)
  {
    delete [] m_iTypePartBest ; m_iTypePartBest = NULL;
  }
  if (m_iRate)
  {
    delete [] m_iRate ; m_iRate = NULL;
  }
  if (m_iDist)
  {
    delete [] m_iDist ; m_iDist = NULL;
  }
  if (m_dCost)
  {
    delete [] m_dCost ; m_dCost = NULL;
  }
  if (m_iCount)
  {
    delete [] m_iCount  ; m_iCount = NULL;
  }
  if (m_iOffset)
  {
    delete [] m_iOffset ; m_iOffset = NULL;
  }
  if (m_iOffsetOrg)
  {
    delete [] m_iOffsetOrg ; m_iOffsetOrg = NULL;
  }

  Int iMaxDepth = 4;
  Int iDepth;
  for ( iDepth = 0; iDepth < iMaxDepth+1; iDepth++ )
  {
    for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
    {
      delete m_pppcRDSbacCoder[iDepth][iCIIdx];
      delete m_pppcBinCoderCABAC[iDepth][iCIIdx];
    }
  }

  for ( iDepth = 0; iDepth < iMaxDepth+1; iDepth++ )
  {
    delete [] m_pppcRDSbacCoder[iDepth];
    delete [] m_pppcBinCoderCABAC[iDepth];
  }

  delete [] m_pppcRDSbacCoder;
  delete [] m_pppcBinCoderCABAC;
}

/** create Encoder Buffer for SAO
 * \param 
 */
Void TEncSampleAdaptiveOffset::createEncBuffer()
{
  m_iDistOrg = new Int64 [m_iNumTotalParts]; 
  m_dCostPartBest = new Double [m_iNumTotalParts]; 
  m_iTypePartBest = new Int [m_iNumTotalParts]; 

  m_iRate = new Int64* [m_iNumTotalParts];
  m_iDist = new Int64* [m_iNumTotalParts];
  m_dCost = new Double*[m_iNumTotalParts];

  m_iCount  = new Int64 **[m_iNumTotalParts];
  m_iOffset = new Int64 **[m_iNumTotalParts];
  m_iOffsetOrg = new Int64 **[m_iNumTotalParts];

  for (Int i=0;i<m_iNumTotalParts;i++)
  {
    m_iRate[i] = new Int64  [MAX_NUM_SAO_TYPE];
    m_iDist[i] = new Int64  [MAX_NUM_SAO_TYPE]; 
    m_dCost[i] = new Double [MAX_NUM_SAO_TYPE]; 

    m_iCount [i] = new Int64 *[MAX_NUM_SAO_TYPE]; 
    m_iOffset[i] = new Int64 *[MAX_NUM_SAO_TYPE]; 
    m_iOffsetOrg[i] = new Int64 *[MAX_NUM_SAO_TYPE]; 

    for (Int j=0;j<MAX_NUM_SAO_TYPE;j++)
    {
      m_iCount [i][j]   = new Int64 [MAX_NUM_SAO_CLASS]; 
      m_iOffset[i][j]   = new Int64 [MAX_NUM_SAO_CLASS]; 
      m_iOffsetOrg[i][j]= new Int64 [MAX_NUM_SAO_CLASS]; 
    }
  }

  Int iMaxDepth = 4;
  m_pppcRDSbacCoder = new TEncSbac** [iMaxDepth+1];
#if FAST_BIT_EST
  m_pppcBinCoderCABAC = new TEncBinCABACCounter** [iMaxDepth+1];
#else
  m_pppcBinCoderCABAC = new TEncBinCABAC** [iMaxDepth+1];
#endif

  for ( Int iDepth = 0; iDepth < iMaxDepth+1; iDepth++ )
  {
    m_pppcRDSbacCoder[iDepth] = new TEncSbac* [CI_NUM];
#if FAST_BIT_EST
    m_pppcBinCoderCABAC[iDepth] = new TEncBinCABACCounter* [CI_NUM];
#else
    m_pppcBinCoderCABAC[iDepth] = new TEncBinCABAC* [CI_NUM];
#endif
    for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
    {
      m_pppcRDSbacCoder[iDepth][iCIIdx] = new TEncSbac;
#if FAST_BIT_EST
      m_pppcBinCoderCABAC [iDepth][iCIIdx] = new TEncBinCABACCounter;
#else
      m_pppcBinCoderCABAC [iDepth][iCIIdx] = new TEncBinCABAC;
#endif
      m_pppcRDSbacCoder   [iDepth][iCIIdx]->init( m_pppcBinCoderCABAC [iDepth][iCIIdx] );
    }
  }
}

/** Start SAO encoder
 * \param pcPic, pcEntropyCoder, pppcRDSbacCoder, pcRDGoOnSbacCoder 
 */
Void TEncSampleAdaptiveOffset::startSaoEnc( TComPic* pcPic, TEncEntropy* pcEntropyCoder, TEncSbac*** pppcRDSbacCoder, TEncSbac* pcRDGoOnSbacCoder)
{
  if( pcRDGoOnSbacCoder )
    m_bUseSBACRD = true;
  else
    m_bUseSBACRD = false;

  m_pcPic = pcPic;
  m_pcEntropyCoder = pcEntropyCoder;

  m_pcRDGoOnSbacCoder = pcRDGoOnSbacCoder;
  m_pcEntropyCoder->resetEntropy();
  m_pcEntropyCoder->resetBits();

  if( m_bUseSBACRD )
  {
    m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[0][CI_NEXT_BEST]);
    m_pppcRDSbacCoder[0][CI_CURR_BEST]->load( m_pppcRDSbacCoder[0][CI_NEXT_BEST]);
  }
}

/** End SAO encoder
 */
Void TEncSampleAdaptiveOffset::endSaoEnc()
{
  m_pcPic = NULL;
  m_pcEntropyCoder = NULL;
}

inline int xSign(int x)
{
  return ((x >> 31) | ((int)( (((unsigned int) -x)) >> 31)));
}

/** Calculate SAO statistics for non-cross-slice or non-cross-tile processing
 * \param  pRecStart to-be-filtered block buffer pointer
 * \param  pOrgStart original block buffer pointer
 * \param  stride picture buffer stride
 * \param  ppStat statistics buffer
 * \param  ppCount counter buffer
 * \param  width block width
 * \param  height block height
 * \param  pbBorderAvail availabilities of block border pixels
 */
Void TEncSampleAdaptiveOffset::calcSaoStatsBlock( Pel* pRecStart, Pel* pOrgStart, Int stride, Int64** ppStats, Int64** ppCount, UInt width, UInt height, Bool* pbBorderAvail)
{
  Int64 *stats, *count;
  Int classIdx, posShift, startX, endX, startY, endY, signLeft,signRight,signDown,signDown1;
  Pel *pOrg, *pRec;
  UInt edgeType;
  Int x, y;

  //--------- Band offset-----------//
  stats = ppStats[SAO_BO];
  count = ppCount[SAO_BO];
  pOrg   = pOrgStart;
  pRec   = pRecStart;
  for (y=0; y< height; y++)
  {
    for (x=0; x< width; x++)
    {
      classIdx = m_ppLumaTableBo[pRec[x]];
      if (classIdx)
      {
        stats[classIdx] += (pOrg[x] - pRec[x]); 
        count[classIdx] ++;
      }
    }
    pOrg += stride;
    pRec += stride;
  }
  //---------- Edge offset 0--------------//
  stats = ppStats[SAO_EO_0];
  count = ppCount[SAO_EO_0];
  pOrg   = pOrgStart;
  pRec   = pRecStart;


  startX = (pbBorderAvail[SGU_L]) ? 0 : 1;
  endX   = (pbBorderAvail[SGU_R]) ? width : (width -1);
  for (y=0; y< height; y++)
  {
    signLeft = xSign(pRec[startX] - pRec[startX-1]);
    for (x=startX; x< endX; x++)
    {
      signRight =  xSign(pRec[x] - pRec[x+1]); 
      edgeType =  signRight + signLeft + 2;
      signLeft  = -signRight;

      stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
      count[m_auiEoTable[edgeType]] ++;
    }
    pRec  += stride;
    pOrg += stride;
  }

  //---------- Edge offset 1--------------//
  stats = ppStats[SAO_EO_1];
  count = ppCount[SAO_EO_1];
  pOrg   = pOrgStart;
  pRec   = pRecStart;

  startY = (pbBorderAvail[SGU_T]) ? 0 : 1;
  endY   = (pbBorderAvail[SGU_B]) ? height : height-1;
  if (!pbBorderAvail[SGU_T])
  {
    pRec  += stride;
    pOrg  += stride;
  }

  for (x=0; x< width; x++)
  {
    m_iUpBuff1[x] = xSign(pRec[x] - pRec[x-stride]);
  }
  for (y=startY; y<endY; y++)
  {
    for (x=0; x< width; x++)
    {
      signDown     =  xSign(pRec[x] - pRec[x+stride]); 
      edgeType    =  signDown + m_iUpBuff1[x] + 2;
      m_iUpBuff1[x] = -signDown;

      stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
      count[m_auiEoTable[edgeType]] ++;
    }
    pOrg += stride;
    pRec += stride;
  }
  //---------- Edge offset 2--------------//
  stats = ppStats[SAO_EO_2];
  count = ppCount[SAO_EO_2];
  pOrg   = pOrgStart;
  pRec   = pRecStart;

  posShift= stride + 1;

  startX = (pbBorderAvail[SGU_L]) ? 0 : 1 ;
  endX   = (pbBorderAvail[SGU_R]) ? width : (width-1);

  //prepare 2nd line upper sign
  pRec += stride;
  for (x=startX; x< endX+1; x++)
  {
    m_iUpBuff1[x] = xSign(pRec[x] - pRec[x- posShift]);
  }

  //1st line
  pRec -= stride;
  if(pbBorderAvail[SGU_TL])
  {
    x= 0;
    edgeType      =  xSign(pRec[x] - pRec[x- posShift]) - m_iUpBuff1[x+1] + 2;
    stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
    count[m_auiEoTable[edgeType]] ++;
  }
  if(pbBorderAvail[SGU_T])
  {
    for(x= 1; x< endX; x++)
    {
      edgeType      =  xSign(pRec[x] - pRec[x- posShift]) - m_iUpBuff1[x+1] + 2;
      stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
      count[m_auiEoTable[edgeType]] ++;
    }
  }
  pRec   += stride;
  pOrg   += stride;

  //middle lines
  for (y= 1; y< height-1; y++)
  {
    for (x=startX; x<endX; x++)
    {
      signDown1      =  xSign(pRec[x] - pRec[x+ posShift]) ;
      edgeType      =  signDown1 + m_iUpBuff1[x] + 2;
      stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
      count[m_auiEoTable[edgeType]] ++;

      m_iUpBufft[x+1] = -signDown1; 
    }
    m_iUpBufft[startX] = xSign(pRec[stride+startX] - pRec[startX-1]);

    ipSwap     = m_iUpBuff1;
    m_iUpBuff1 = m_iUpBufft;
    m_iUpBufft = ipSwap;

    pRec  += stride;
    pOrg  += stride;
  }

  //last line
  if(pbBorderAvail[SGU_B])
  {
    for(x= startX; x< width-1; x++)
    {
      edgeType =  xSign(pRec[x] - pRec[x+ posShift]) + m_iUpBuff1[x] + 2;
      stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
      count[m_auiEoTable[edgeType]] ++;
    }
  }
  if(pbBorderAvail[SGU_BR])
  {
    x= width -1;
    edgeType =  xSign(pRec[x] - pRec[x+ posShift]) + m_iUpBuff1[x] + 2;
    stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
    count[m_auiEoTable[edgeType]] ++;
  }

  //---------- Edge offset 3--------------//

  stats = ppStats[SAO_EO_3];
  count = ppCount[SAO_EO_3];
  pOrg   = pOrgStart;
  pRec   = pRecStart;

  posShift     = stride - 1;
  startX = (pbBorderAvail[SGU_L]) ? 0 : 1;
  endX   = (pbBorderAvail[SGU_R]) ? width : (width -1);

  //prepare 2nd line upper sign
  pRec += stride;
  for (x=startX-1; x< endX; x++)
  {
    m_iUpBuff1[x] = xSign(pRec[x] - pRec[x- posShift]);
  }


  //first line
  pRec -= stride;
  if(pbBorderAvail[SGU_T])
  {
    for(x= startX; x< width -1; x++)
    {
      edgeType = xSign(pRec[x] - pRec[x- posShift]) -m_iUpBuff1[x-1] + 2;
      stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
      count[m_auiEoTable[edgeType]] ++;
    }
  }
  if(pbBorderAvail[SGU_TR])
  {
    x= width-1;
    edgeType = xSign(pRec[x] - pRec[x- posShift]) -m_iUpBuff1[x-1] + 2;
    stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
    count[m_auiEoTable[edgeType]] ++;
  }
  pRec  += stride;
  pOrg  += stride;

  //middle lines
  for (y= 1; y< height-1; y++)
  {
    for(x= startX; x< endX; x++)
    {
      signDown1      =  xSign(pRec[x] - pRec[x+ posShift]) ;
      edgeType      =  signDown1 + m_iUpBuff1[x] + 2;

      stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
      count[m_auiEoTable[edgeType]] ++;
      m_iUpBuff1[x-1] = -signDown1; 

    }
    m_iUpBuff1[endX-1] = xSign(pRec[endX-1 + stride] - pRec[endX]);

    pRec  += stride;
    pOrg  += stride;
  }

  //last line
  if(pbBorderAvail[SGU_BL])
  {
    x= 0;
    edgeType = xSign(pRec[x] - pRec[x+ posShift]) + m_iUpBuff1[x] + 2;
    stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
    count[m_auiEoTable[edgeType]] ++;

  }
  if(pbBorderAvail[SGU_B])
  {
    for(x= 1; x< endX; x++)
    {
      edgeType = xSign(pRec[x] - pRec[x+ posShift]) + m_iUpBuff1[x] + 2;
      stats[m_auiEoTable[edgeType]] += (pOrg[x] - pRec[x]);
      count[m_auiEoTable[edgeType]] ++;
    }
  }
}

/** Calculate SAO statistics for current LCU
 * \param  iAddr,  iPartIdx,  iYCbCr
 */
Void TEncSampleAdaptiveOffset::calcSaoStatsCu(Int iAddr, Int iPartIdx, Int iYCbCr)
{
  if(!m_bUseNIF)
  {
    calcSaoStatsCuOrg( iAddr, iPartIdx, iYCbCr);
  }
  else
  {
    Int64** ppStats = m_iOffsetOrg[iPartIdx];
    Int64** ppCount = m_iCount    [iPartIdx];

    //parameters
    Int  isChroma = (iYCbCr != 0)? 1:0;
    Int  stride   = (iYCbCr != 0)?(m_pcPic->getCStride()):(m_pcPic->getStride());
    Pel* pPicOrg = getPicYuvAddr (m_pcPic->getPicYuvOrg(), iYCbCr);
    Pel* pPicRec  = getPicYuvAddr(m_pcYuvTmp, iYCbCr);

    std::vector<NDBFBlockInfo>& vFilterBlocks = *(m_pcPic->getCU(iAddr)->getNDBFilterBlocks());

    //variables
    UInt  xPos, yPos, width, height;
    Bool* pbBorderAvail;
    UInt  posOffset;

    for(Int i=0; i< vFilterBlocks.size(); i++)
    {
      xPos        = vFilterBlocks[i].posX   >> isChroma;
      yPos        = vFilterBlocks[i].posY   >> isChroma;
      width       = vFilterBlocks[i].width  >> isChroma;
      height      = vFilterBlocks[i].height >> isChroma;
      pbBorderAvail = vFilterBlocks[i].isBorderAvailable;

      posOffset = (yPos* stride) + xPos;

      calcSaoStatsBlock(pPicRec+ posOffset, pPicOrg+ posOffset, stride, ppStats, ppCount,width, height, pbBorderAvail);
    }
  }

}

/** Calculate SAO statistics for current LCU without non-crossing slice
 * \param  iAddr,  iPartIdx,  iYCbCr
 */
Void TEncSampleAdaptiveOffset::calcSaoStatsCuOrg(Int iAddr, Int iPartIdx, Int iYCbCr)
{
  Int x,y;
  TComDataCU *pTmpCu = m_pcPic->getCU(iAddr);
  TComSPS *pTmpSPS =  m_pcPic->getSlice(0)->getSPS();

  Pel* pOrg;
  Pel* pRec;
  Int iStride;
  Int iLcuWidth  = pTmpSPS->getMaxCUHeight();
  Int iLcuHeight = pTmpSPS->getMaxCUWidth();
  UInt uiLPelX   = pTmpCu->getCUPelX();
  UInt uiTPelY   = pTmpCu->getCUPelY();
  UInt uiRPelX;
  UInt uiBPelY;
  Int64* iStats;
  Int64* iCount;
  Int iClassIdx;
  Int iPicWidthTmp;
  Int iPicHeightTmp;
  Int iStartX;
  Int iStartY;
  Int iEndX;
  Int iEndY;

  Int iIsChroma = (iYCbCr!=0)? 1:0;
  Int iNumSkipLine = iIsChroma? 2:4;
  if (m_saoInterleavingFlag == 0)
  {
    iNumSkipLine = 0;
  }

  iPicWidthTmp  = m_iPicWidth  >> iIsChroma;
  iPicHeightTmp = m_iPicHeight >> iIsChroma;
  iLcuWidth     = iLcuWidth    >> iIsChroma;
  iLcuHeight    = iLcuHeight   >> iIsChroma;
  uiLPelX       = uiLPelX      >> iIsChroma;
  uiTPelY       = uiTPelY      >> iIsChroma;
  uiRPelX       = uiLPelX + iLcuWidth  ;
  uiBPelY       = uiTPelY + iLcuHeight ;
  uiRPelX       = uiRPelX > iPicWidthTmp  ? iPicWidthTmp  : uiRPelX;
  uiBPelY       = uiBPelY > iPicHeightTmp ? iPicHeightTmp : uiBPelY;
  iLcuWidth     = uiRPelX - uiLPelX;
  iLcuHeight    = uiBPelY - uiTPelY;

  iStride    =  (iYCbCr == 0)? m_pcPic->getStride(): m_pcPic->getCStride();

//if(iSaoType == BO_0 || iSaoType == BO_1)
  {
    iStats = m_iOffsetOrg[iPartIdx][SAO_BO];
    iCount = m_iCount    [iPartIdx][SAO_BO];

    pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
    pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

    iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight : iLcuHeight-iNumSkipLine;
    for (y=0; y<iEndY; y++)
    {
      for (x=0; x<iLcuWidth; x++)
      {
        iClassIdx = m_ppLumaTableBo[pRec[x]];
        if (iClassIdx)
        {
          iStats[iClassIdx] += (pOrg[x] - pRec[x]); 
          iCount[iClassIdx] ++;
        }
      }
      pOrg += iStride;
      pRec += iStride;
    }
  }

  Int iSignLeft;
  Int iSignRight;
  Int iSignDown;
  Int iSignDown1;
  Int iSignDown2;

  UInt uiEdgeType;

//if (iSaoType == EO_0  || iSaoType == EO_1 || iSaoType == EO_2 || iSaoType == EO_3)
  {
  //if (iSaoType == EO_0)
    {
      iStats = m_iOffsetOrg[iPartIdx][SAO_EO_0];
      iCount = m_iCount    [iPartIdx][SAO_EO_0];

      pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
      pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

      iStartX = (uiLPelX == 0) ? 1 : 0;
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth;
      for (y=0; y<iLcuHeight-iNumSkipLine; y++)
      {
        iSignLeft = xSign(pRec[iStartX] - pRec[iStartX-1]);
        for (x=iStartX; x< iEndX; x++)
        {
          iSignRight =  xSign(pRec[x] - pRec[x+1]); 
          uiEdgeType =  iSignRight + iSignLeft + 2;
          iSignLeft  = -iSignRight;

          iStats[m_auiEoTable[uiEdgeType]] += (pOrg[x] - pRec[x]);
          iCount[m_auiEoTable[uiEdgeType]] ++;
        }
        pOrg += iStride;
        pRec += iStride;
      }
    }

  //if (iSaoType == EO_1)
    {
      iStats = m_iOffsetOrg[iPartIdx][SAO_EO_1];
      iCount = m_iCount    [iPartIdx][SAO_EO_1];

      pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
      pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

      iStartY = (uiTPelY == 0) ? 1 : 0;
      iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight-iNumSkipLine;
      if (uiTPelY == 0)
      {
        pOrg += iStride;
        pRec += iStride;
      }

      for (x=0; x< iLcuWidth; x++)
      {
        m_iUpBuff1[x] = xSign(pRec[x] - pRec[x-iStride]);
      }
      for (y=iStartY; y<iEndY; y++)
      {
        for (x=0; x<iLcuWidth; x++)
        {
          iSignDown     =  xSign(pRec[x] - pRec[x+iStride]); 
          uiEdgeType    =  iSignDown + m_iUpBuff1[x] + 2;
          m_iUpBuff1[x] = -iSignDown;

          iStats[m_auiEoTable[uiEdgeType]] += (pOrg[x] - pRec[x]);
          iCount[m_auiEoTable[uiEdgeType]] ++;
        }
        pOrg += iStride;
        pRec += iStride;
      }
    }
  //if (iSaoType == EO_2)
    {
      iStats = m_iOffsetOrg[iPartIdx][SAO_EO_2];
      iCount = m_iCount    [iPartIdx][SAO_EO_2];

      pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
      pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

      iStartX = (uiLPelX == 0) ? 1 : 0;
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth;

      iStartY = (uiTPelY == 0) ? 1 : 0;
      iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight-iNumSkipLine;
      if (uiTPelY == 0)
      {
        pOrg += iStride;
        pRec += iStride;
      }

      for (x=iStartX; x<iEndX; x++)
      {
        m_iUpBuff1[x] = xSign(pRec[x] - pRec[x-iStride-1]);
      }
      for (y=iStartY; y<iEndY; y++)
      {
        iSignDown2 = xSign(pRec[iStride+iStartX] - pRec[iStartX-1]);
        for (x=iStartX; x<iEndX; x++)
        {
          iSignDown1      =  xSign(pRec[x] - pRec[x+iStride+1]) ;
          uiEdgeType      =  iSignDown1 + m_iUpBuff1[x] + 2;
          m_iUpBufft[x+1] = -iSignDown1; 
          iStats[m_auiEoTable[uiEdgeType]] += (pOrg[x] - pRec[x]);
          iCount[m_auiEoTable[uiEdgeType]] ++;
        }
        m_iUpBufft[iStartX] = iSignDown2;
        ipSwap     = m_iUpBuff1;
        m_iUpBuff1 = m_iUpBufft;
        m_iUpBufft = ipSwap;

        pRec += iStride;
        pOrg += iStride;
      }
    } 
  //if (iSaoType == EO_3  )
    {
      iStats = m_iOffsetOrg[iPartIdx][SAO_EO_3];
      iCount = m_iCount    [iPartIdx][SAO_EO_3];

      pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
      pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

      iStartX = (uiLPelX == 0) ? 1 : 0;
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth;

      iStartY = (uiTPelY == 0) ? 1 : 0;
      iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight-iNumSkipLine;
      if (iStartY == 1)
      {
        pOrg += iStride;
        pRec += iStride;
      }

      for (x=iStartX-1; x<iEndX; x++)
      {
        m_iUpBuff1[x] = xSign(pRec[x] - pRec[x-iStride+1]);
      }

      for (y=iStartY; y<iEndY; y++)
      {
        for (x=iStartX; x<iEndX; x++)
        {
          iSignDown1      =  xSign(pRec[x] - pRec[x+iStride-1]) ;
          uiEdgeType      =  iSignDown1 + m_iUpBuff1[x] + 2;
          m_iUpBuff1[x-1] = -iSignDown1; 
          iStats[m_auiEoTable[uiEdgeType]] += (pOrg[x] - pRec[x]);
          iCount[m_auiEoTable[uiEdgeType]] ++;
        }
        m_iUpBuff1[iEndX-1] = xSign(pRec[iEndX-1 + iStride] - pRec[iEndX]);

        pRec += iStride;
        pOrg += iStride;
      } 
    } 
  }
}

/** get SAO statistics
 * \param  *psQTPart,  iYCbCr
 */
Void TEncSampleAdaptiveOffset::getSaoStats(SAOQTPart *psQTPart, Int iYCbCr)
{
  Int iLevelIdx, iPartIdx, iTypeIdx, iClassIdx;
  Int i;
  Int iNumTotalType = MAX_NUM_SAO_TYPE;
  Int LcuIdxX;
  Int LcuIdxY;
  Int iAddr;
  Int iFrameWidthInCU = m_pcPic->getFrameWidthInCU();
  Int iDownPartIdx;
  Int iPartStart;
  Int iPartEnd;
  SAOQTPart*  pOnePart; 

  if (m_uiMaxSplitLevel == 0)
  {
    iPartIdx = 0;
    pOnePart = &(psQTPart[iPartIdx]);
    for (LcuIdxY = pOnePart->startCUY; LcuIdxY<= pOnePart->endCUY; LcuIdxY++)
    {
      for (LcuIdxX = pOnePart->startCUX; LcuIdxX<= pOnePart->endCUX; LcuIdxX++)
      {
        iAddr = LcuIdxY*iFrameWidthInCU + LcuIdxX;
        calcSaoStatsCu(iAddr, iPartIdx, iYCbCr);
      }
    }
  }
  else
  {
    for(iPartIdx=m_aiNumCulPartsLevel[m_uiMaxSplitLevel-1]; iPartIdx<m_aiNumCulPartsLevel[m_uiMaxSplitLevel]; iPartIdx++)
    {
      pOnePart = &(psQTPart[iPartIdx]);
      for (LcuIdxY = pOnePart->startCUY; LcuIdxY<= pOnePart->endCUY; LcuIdxY++)
      {
        for (LcuIdxX = pOnePart->startCUX; LcuIdxX<= pOnePart->endCUX; LcuIdxX++)
        {
          iAddr = LcuIdxY*iFrameWidthInCU + LcuIdxX;
          calcSaoStatsCu(iAddr, iPartIdx, iYCbCr);
        }
      }
    }
    for (iLevelIdx = m_uiMaxSplitLevel-1; iLevelIdx>=0; iLevelIdx-- )
    {
      iPartStart = (iLevelIdx > 0) ? m_aiNumCulPartsLevel[iLevelIdx-1] : 0;
      iPartEnd   = m_aiNumCulPartsLevel[iLevelIdx];

      for(iPartIdx = iPartStart; iPartIdx < iPartEnd; iPartIdx++)
      {
        pOnePart = &(psQTPart[iPartIdx]);
        for (i=0; i< NUM_DOWN_PART; i++)
        {
          iDownPartIdx = pOnePart->downPartsIdx[i];
          for (iTypeIdx=0; iTypeIdx<iNumTotalType; iTypeIdx++)
          {
            for (iClassIdx=0; iClassIdx< (iTypeIdx < SAO_BO ? MAX_NUM_SAO_OFFSETS : SAO_MAX_BO_CLASSES) +1; iClassIdx++)
            {
              m_iOffsetOrg[iPartIdx][iTypeIdx][iClassIdx] += m_iOffsetOrg[iDownPartIdx][iTypeIdx][iClassIdx];
              m_iCount [iPartIdx][iTypeIdx][iClassIdx]    += m_iCount [iDownPartIdx][iTypeIdx][iClassIdx];
            }
          }
        }
      }
    }
  }
}

/** reset offset statistics 
 * \param 
 */
Void TEncSampleAdaptiveOffset::resetStats()
{
  for (Int i=0;i<m_iNumTotalParts;i++)
  {
    m_dCostPartBest[i] = MAX_DOUBLE;
    m_iTypePartBest[i] = -1;
    m_iDistOrg[i] = 0;
    for (Int j=0;j<MAX_NUM_SAO_TYPE;j++)
    {
      m_iDist[i][j] = 0;
      m_iRate[i][j] = 0;
      m_dCost[i][j] = 0;
      for (Int k=0;k<MAX_NUM_SAO_CLASS;k++)
      {
        m_iCount [i][j][k] = 0;
        m_iOffset[i][j][k] = 0;
        m_iOffsetOrg[i][j][k] = 0;
      }  
    }
  }
}

#if SAO_CHROMA_LAMBDA 
/** Sample adaptive offset process
 * \param pcSaoParam
 * \param dLambdaLuma
 * \param dLambdaChroma
 */
Void TEncSampleAdaptiveOffset::SAOProcess(SAOParam *pcSaoParam, Double dLambdaLuma, Double dLambdaChroma)
#else
/** Sample adaptive offset process
 * \param dLambda
 */
Void TEncSampleAdaptiveOffset::SAOProcess(SAOParam *pcSaoParam, Double dLambda)
#endif
{
#if FULL_NBIT
  m_uiSaoBitIncrease = g_uiBitDepth + (g_uiBitDepth-8) - min((Int)(g_uiBitDepth + (g_uiBitDepth-8)), 10);
#else
  m_uiSaoBitIncrease = g_uiBitDepth + g_uiBitIncrement - min((Int)(g_uiBitDepth + g_uiBitIncrement), 10);
#endif

  if(m_bUseNIF)
  {
    m_pcPic->getPicYuvRec()->copyToPic(m_pcYuvTmp);
  }

  const Int iOffsetBitRange8Bit = 4;
  Int iOffsetBitDepth = g_uiBitDepth + g_uiBitIncrement - m_uiSaoBitIncrease;
  Int iOffsetBitRange = iOffsetBitRange8Bit + (iOffsetBitDepth - 8);
  Double dCostFinal = 0;
  Double lambdaRdo = 0;

  m_iOffsetTh = 1 << (iOffsetBitRange - 1);
  resetSAOParam(pcSaoParam);
  resetStats();

  if ( m_saoInterleavingFlag)
  {
    rdoSaoUnitAll(pcSaoParam, dLambdaLuma, dLambdaChroma);
  }
  else
  {
    pcSaoParam->saoFlag[0] = 1;
    for (Int compIdx=0;compIdx<3;compIdx++)
    {
      if (pcSaoParam->saoFlag[0])
      {
        dCostFinal = 0;
        lambdaRdo = (compIdx==0 ? dLambdaLuma: dLambdaChroma);
        resetStats();
        getSaoStats(pcSaoParam->saoPart[compIdx], compIdx);
        runQuadTreeDecision(pcSaoParam->saoPart[compIdx], 0, dCostFinal, m_uiMaxSplitLevel, lambdaRdo);
        pcSaoParam->saoFlag[compIdx] = dCostFinal < 0 ? 1:0;
        if(pcSaoParam->saoFlag[compIdx])
        {
          convertQT2SaoUnit(pcSaoParam, 0, compIdx);
          assignSaoUnitSyntax(pcSaoParam->saoLcuParam[compIdx],  pcSaoParam->saoPart[compIdx], pcSaoParam->oneUnitFlag[compIdx]);
        }
      }
    }
  }
  for (Int compIdx=0;compIdx<3;compIdx++)
  {
    if (pcSaoParam->saoFlag[compIdx])
    {
      processSaoUnitAll( pcSaoParam->saoLcuParam[compIdx], pcSaoParam->oneUnitFlag[compIdx], compIdx);
    }
  }
}
Void TEncSampleAdaptiveOffset::checkMerge(SaoLcuParam * saoUnitCurr, SaoLcuParam * saoUnitCheck, Int dir)
{
  Int i ;
  Int countDiff = 0;
  if (saoUnitCurr->partIdx != saoUnitCheck->partIdx)
  {
    if (saoUnitCurr->typeIdx !=-1)
    {
      if (saoUnitCurr->typeIdx == saoUnitCheck->typeIdx)
      {
        for (i=0;i<saoUnitCurr->length;i++)
        {
          countDiff += (saoUnitCurr->offset[i] != saoUnitCheck->offset[i]);
        }
        countDiff += (saoUnitCurr->bandPosition != saoUnitCheck->bandPosition);
        if (countDiff ==0)
        {
          saoUnitCurr->partIdx = saoUnitCheck->partIdx;
          if (dir == 1)
          {
            saoUnitCurr->mergeUpFlag = 1;
            saoUnitCurr->mergeLeftFlag = 0;
          }
          else
          {
            saoUnitCurr->mergeUpFlag = 0;
            saoUnitCurr->mergeLeftFlag = 1;
          }
        }
      }
    }
    else
    {
      if (saoUnitCurr->typeIdx == saoUnitCheck->typeIdx)
      {
        saoUnitCurr->partIdx = saoUnitCheck->partIdx;
        if (dir == 1)
        {
          saoUnitCurr->mergeUpFlag = 1;
          saoUnitCurr->mergeLeftFlag = 0;
        }
        else
        {
          saoUnitCurr->mergeUpFlag = 0;
          saoUnitCurr->mergeLeftFlag = 1;
        }
      }
    }
  }
}
/** Assign SAO unit syntax from picture-based algorithm
 * \param saoLcuParam
 * \param saoPart
 * \param oneUnitFlag
 */
Void TEncSampleAdaptiveOffset::assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, Bool &oneUnitFlag)
{
  if (saoPart->split == 0)
  {
    oneUnitFlag = 1;
  }
  else
  {
    Int i,j, addr, addrUp, addrLeft,  idx, idxUp, idxLeft,  idxCount;
    Int run;
    Int runPartBeginAddr=0;
    Int runPart;
    Int runPartPrevious;

    oneUnitFlag = 0;

    idxCount = -1;
    saoLcuParam[0].mergeUpFlag = 0;
    saoLcuParam[0].mergeLeftFlag = 0;

    for (j=0;j<m_iNumCuInHeight;j++)
    {
      run = 0;
      runPartPrevious = -1;
      for (i=0;i<m_iNumCuInWidth;i++)
      {
        addr     = i + j*m_iNumCuInWidth;
        addrLeft = (addr%m_iNumCuInWidth == 0) ? -1 : addr - 1;
        addrUp   = (addr<m_iNumCuInWidth)      ? -1 : addr - m_iNumCuInWidth;
        idx      = saoLcuParam[addr].partIdxTmp;
        idxLeft  = (addrLeft == -1) ? -1 : saoLcuParam[addrLeft].partIdxTmp;
        idxUp    = (addrUp == -1)   ? -1 : saoLcuParam[addrUp].partIdxTmp;

        if(idx!=idxLeft && idx!=idxUp)
        {
          saoLcuParam[addr].mergeUpFlag   = 0; idxCount++;
          saoLcuParam[addr].mergeLeftFlag = 0;
          saoLcuParam[addr].partIdx = idxCount;
        }
        else if (idx==idxLeft)
        {
          saoLcuParam[addr].mergeUpFlag   = 1;
          saoLcuParam[addr].mergeLeftFlag = 1;
          saoLcuParam[addr].partIdx = saoLcuParam[addrLeft].partIdx;
        }
        else if (idx==idxUp)
        {
          saoLcuParam[addr].mergeUpFlag   = 1;
          saoLcuParam[addr].mergeLeftFlag = 0;
          saoLcuParam[addr].partIdx = saoLcuParam[addrUp].partIdx;
        }
        if (addrUp != -1)
        {
          checkMerge(&saoLcuParam[addr], &saoLcuParam[addrUp], 1);
        }
        if (addrLeft != -1)
        {
          checkMerge(&saoLcuParam[addr], &saoLcuParam[addrLeft], 0);
        }
        runPart = saoLcuParam[addr].partIdx;
        if (runPart == runPartPrevious)
        {
          run ++;
          saoLcuParam[addr].run = -1;
          saoLcuParam[runPartBeginAddr].run = run;
        }
        else
        {
          runPartBeginAddr = addr;
          run = 0;
          saoLcuParam[addr].run = run;
        }
        runPartPrevious = runPart;
      }
    }

    for (j=0;j<m_iNumCuInHeight;j++)
    {
      for (i=0;i<m_iNumCuInWidth;i++)
      {
        addr      =  i + j*m_iNumCuInWidth;
        addrLeft =  (addr%m_iNumCuInWidth == 0) ? -1 : addr - 1;
        addrUp   =  (addr<m_iNumCuInWidth)      ? -1 : addr - m_iNumCuInWidth;

        if (saoLcuParam[addr].run == -1)
        {
          if (addrLeft != -1)
          {
            saoLcuParam[addr].run = saoLcuParam[addrLeft].run-1;
          }
        }
        if (addrUp>=0)
        {
          saoLcuParam[addr].runDiff = saoLcuParam[addr].run - saoLcuParam[addrUp].run;
        }
        else
        {
          saoLcuParam[addr].runDiff = saoLcuParam[addr].run ;
        }
      }
    }
  }
}
/** rate distortion optimization of SAO all units
 * \param saoParam
 * \param lambdaLuma
 * \param lambdaChroma
 */
Void TEncSampleAdaptiveOffset::rdoSaoUnitAll(SAOParam *saoParam, Double lambdaLuma, Double lambdaChroma)
{

  Int ry;
  Int rx;
  Int frameHeightInCU = saoParam->numUnitInHeight;
  Int frameWidthInCU  = saoParam->numUnitInWidth;
  Int j, k;
  Int addr = 0;
  Int addrUp = -1;
  Int addrLeft = -1;
  Int compIdx = 0;
  Double dLambdaComp;

  saoParam->saoFlag[0] = true;
  saoParam->saoFlag[1] = true;
  saoParam->saoFlag[2] = true;
  saoParam->oneUnitFlag[0]  = false;
  saoParam->oneUnitFlag[1] = false;
  saoParam->oneUnitFlag[2] = false;


  for (ry = 0; ry< frameHeightInCU; ry++)
  {
    for (rx = 0; rx< frameWidthInCU; rx++)
    {
      addr     = rx   + frameWidthInCU*ry;
      addrUp   = addr < frameWidthInCU ? -1: addr - frameWidthInCU;
      addrLeft = rx == 0               ? -1: addr-1 ;
      // reset stats Y, Cb, Cr
      for ( compIdx=0;compIdx<3;compIdx++)
      {
        for ( j=0;j<MAX_NUM_SAO_TYPE;j++)
        {
          for ( k=0;k< MAX_NUM_SAO_CLASS;k++)
          {
            m_iCount    [compIdx][j][k] = 0;
            m_iOffset   [compIdx][j][k] = 0;
            m_iOffsetOrg[compIdx][j][k] = 0;
          }  
        }
        saoParam->saoLcuParam[compIdx][addr].typeIdx       =  -1;
        saoParam->saoLcuParam[compIdx][addr].mergeUpFlag   = 0;
        saoParam->saoLcuParam[compIdx][addr].run           = 0;
        saoParam->saoLcuParam[compIdx][addr].runDiff       = 0;
        saoParam->saoLcuParam[compIdx][addr].mergeLeftFlag = 0;
        saoParam->saoLcuParam[compIdx][addr].bandPosition  = 0;
        dLambdaComp = compIdx==0 ? lambdaLuma : lambdaChroma;
        calcSaoStatsCu(addr, compIdx,  compIdx);
        rdoSaoUnit (saoParam, addr, addrUp, addrLeft, compIdx,  (compIdx==0 ? lambdaLuma : lambdaChroma) );
        if (compIdx!=0)
        {
          if ( saoParam->saoLcuParam[compIdx][0].typeIdx == -1 )
          {
            saoParam->saoFlag[compIdx] = false;
          }
        }
      }
    }
  }

}
/** rate distortion optimization of SAO unit 
 * \param saoParam
 * \param addr
 * \param addrUp
 * \param addrLeft
 * \param compIdx
 * \param lambda
 */
Void TEncSampleAdaptiveOffset::rdoSaoUnit(SAOParam *saoParam, Int addr, Int addrUp, Int addrLeft, Int compIdx, Double lambda)
{
  Int typeIdx;

  Int64  estDist;
  Int64  offsetOrg;
  Int64  offset;
  Int64  count;
  Int    classIdx;
  Int    shift = g_uiBitIncrement << 1;
  Int64  minDist;
  Int64  minRate;
  Double minCost = MAX_DOUBLE;
  SaoLcuParam*  saoLcuParamTmp = new SaoLcuParam;   
  SaoLcuParam*  saoLcuParam      = NULL;   
  SaoLcuParam*  saoLcuParamUp    = NULL; 
  SaoLcuParam*  saoLcuParamLeft  = NULL;
  Int iOffsetTh = m_iOffsetTh;

  saoLcuParam = &(saoParam->saoLcuParam[compIdx][addr]);
  if (addrUp>=0)
  {
    saoLcuParamUp = &(saoParam->saoLcuParam[compIdx][addrUp]);
  }
  if (addrLeft>=0)
  {
    saoLcuParamLeft = &(saoParam->saoLcuParam[compIdx][addrLeft]);
  }

  saoLcuParam->mergeUpFlag   = 0;
  saoLcuParam->mergeLeftFlag = 0;
  saoLcuParam->run    = 0;
  saoLcuParam->runDiff= 0;


  m_iTypePartBest[compIdx] = -1;
  m_dCostPartBest[compIdx] = 0;
  m_iDistOrg[compIdx] = 0;

  Double  bestRDCostTableBo = MAX_DOUBLE;
  Int     bestClassTableBo    = 0;
  Int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
  Double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];
  Int     bestClassTableBoMerge = 0;

  for (typeIdx=-1; typeIdx<MAX_NUM_SAO_TYPE; typeIdx++)
  {
    if (m_saoInterleavingFlag)
    {
//       if(compIdx>0 && typeIdx>3 )
//       {
//         continue;
//       }
      if (compIdx>0 )
      {
        iOffsetTh = 2<<g_uiBitIncrement;
      }
      else
      {
        iOffsetTh = m_iOffsetTh;
      }
    }

    estDist = 0;
    if (typeIdx>=0)
    {
      for(classIdx=1; classIdx < ( (typeIdx < SAO_BO) ?  MAX_NUM_SAO_OFFSETS+1 : SAO_MAX_BO_CLASSES+1); classIdx++)
      {
        if( typeIdx == SAO_BO)
        {
          currentDistortionTableBo[classIdx-1] = 0;
          currentRdCostTableBo[classIdx-1] = lambda;
        }
        if(m_iCount [compIdx][typeIdx][classIdx])
        {
          m_iOffset[compIdx][typeIdx][classIdx] = (Int64) xRoundIbdi((Double)(m_iOffsetOrg[compIdx][typeIdx][classIdx]<<g_uiBitIncrement) / (Double)(m_iCount [compIdx][typeIdx][classIdx]<<m_uiSaoBitIncrease));
          m_iOffset[compIdx][typeIdx][classIdx] = Clip3(-iOffsetTh, iOffsetTh, (Int)m_iOffset[compIdx][typeIdx][classIdx]);
          if (typeIdx < 4)
          {
            if ( m_iOffset[compIdx][typeIdx][classIdx]<0 && classIdx<3 )
            {
              m_iOffset[compIdx][typeIdx][classIdx] = 0;
            }
            if ( m_iOffset[compIdx][typeIdx][classIdx]>0 && classIdx>=3)
            {
              m_iOffset[compIdx][typeIdx][classIdx] = 0;
            }
          }
          {
            //Clean up, best_q_offset.
            Int64 iterOffset, tempOffset;
            Int64 tempDist, tempRate;
            Double tempCost, tempMinCost;
            UInt length, temp;

            iterOffset = m_iOffset[compIdx][typeIdx][classIdx];
            m_iOffset[compIdx][typeIdx][classIdx] = 0;
            tempMinCost = lambda; // Assuming sending quantized value 0 results in zero offset and sending the value zero needs 1 bit. entropy coder can be used to measure the exact rate here. 

            while (iterOffset != 0)
            {
              // Calculate the bits required for signalling the offset
              length = 1;
              temp = (UInt)((iterOffset <= 0) ? ( (-iterOffset<<1) + 1 ) : (iterOffset<<1));
              while( 1 != temp )
              {
                temp >>= 1;
                length += 2;
              }
              tempRate = (length >> 1) + ((length+1) >> 1);

              // Do the dequntization before distorion calculation
              tempOffset    =  iterOffset << m_uiSaoBitIncrease;
              tempDist  = (( m_iCount [compIdx][typeIdx][classIdx]*tempOffset*tempOffset-m_iOffsetOrg[compIdx][typeIdx][classIdx]*tempOffset*2 ) >> shift);
              tempCost = ((Double)tempDist + lambda * (Double) tempRate);
              if(tempCost < tempMinCost)
              {
                tempMinCost = tempCost;
                m_iOffset[compIdx][typeIdx][classIdx] = iterOffset;
                if(typeIdx == SAO_BO)
                {
                  currentDistortionTableBo[classIdx-1] = (Int) tempDist;
                  currentRdCostTableBo[classIdx-1] = tempCost;
                }
              }
              iterOffset = (iterOffset > 0) ? (iterOffset-1):(iterOffset+1);
            }
          }

        }
        else
        {
          m_iOffsetOrg[compIdx][typeIdx][classIdx] = 0;
          m_iOffset[compIdx][typeIdx][classIdx] = 0;
        }
        if( typeIdx != SAO_BO )
        {
          count     =  m_iCount [compIdx][typeIdx][classIdx];
          offset    =  m_iOffset[compIdx][typeIdx][classIdx] << m_uiSaoBitIncrease;
          offsetOrg =  m_iOffsetOrg[compIdx][typeIdx][classIdx];
          estDist   += (( count*offset*offset-offsetOrg*offset*2 ) >> shift);
        }
      }
      if (typeIdx != SAO_BO)
      {
        m_pcEntropyCoder->resetEntropy();
        m_pcEntropyCoder->resetBits();

        saoLcuParamTmp->bandPosition = 0;
        saoLcuParamTmp->typeIdx = typeIdx;
        for (Int i=0;i<SAO_EO_LEN; i++)
        {
          saoLcuParamTmp->offset[i] = (Int)m_iOffset[compIdx][typeIdx][i+1];
        }
        m_pcEntropyCoder->encodeSaoOffset(saoLcuParamTmp);
        m_iRate[compIdx][typeIdx] = m_pcEntropyCoder->getNumberOfWrittenBits();
      }

      if( typeIdx == SAO_BO )
      {
        m_pcEntropyCoder->resetEntropy();
        m_pcEntropyCoder->resetBits();

        m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoTypeIdx(typeIdx+1);
        // Estimate Best Position
        Double currentRDCost = 0.0;

        for(Int i=0; i< SAO_MAX_BO_CLASSES -SAO_BO_LEN +1; i++)
        {
          currentRDCost = 0.0;
          for(UInt uj = i; uj < i+SAO_BO_LEN; uj++)
          {
            currentRDCost += currentRdCostTableBo[uj];
          }
          if( currentRDCost < bestRDCostTableBo)
          {
            bestRDCostTableBo = currentRDCost;
            bestClassTableBo  = i;
          }
        }
        // Re code all Offsets
        // Code Center
        m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoUflc( (UInt) (bestClassTableBo) );

        for(classIdx = bestClassTableBo; classIdx < bestClassTableBo+SAO_BO_LEN; classIdx++)
        {
          m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoSvlc((Int)m_iOffset[compIdx][typeIdx][classIdx+1]);
          estDist += currentDistortionTableBo[classIdx];
        }
        m_iRate[compIdx][typeIdx] = m_pcEntropyCoder->getNumberOfWrittenBits();
      }

      m_iDist[compIdx][typeIdx] = estDist;
      m_dCost[compIdx][typeIdx] = (Double)((Double)m_iDist[compIdx][typeIdx] + lambda * (Double) m_iRate[compIdx][typeIdx]);

      if(m_dCost[compIdx][typeIdx] < m_dCostPartBest[compIdx])
      {
        m_iDistOrg [compIdx] = 0;
        m_dCostPartBest[compIdx] = m_dCost[compIdx][typeIdx];
        m_iTypePartBest[compIdx] = typeIdx;
      }
    }
    else
    {
      if(m_iDistOrg[compIdx] < m_dCostPartBest[compIdx])
      {
        m_dCostPartBest[compIdx] = (Double) m_iDistOrg[compIdx] + m_pcEntropyCoder->getNumberOfWrittenBits()*lambda ; 
        m_iTypePartBest[compIdx] = -1;
      }
    }
  }

  // LEFT merge
  if (addrLeft>=0) 
  {
    checkMergeSaoUnit( saoLcuParam,  saoLcuParamLeft, compIdx, 1, lambda);
  } 
  //UP merge
  if (addrUp>=0) 
  {
    checkMergeSaoUnit( saoLcuParam,  saoLcuParamUp, compIdx, 0, lambda);
  } 

  minDist   =        m_iTypePartBest[compIdx] >= 0 ? m_iDist[compIdx][m_iTypePartBest[compIdx]] : m_iDistOrg[compIdx];
  minRate   = (Int) (m_iTypePartBest[compIdx] >= 0 ? m_iRate[compIdx][m_iTypePartBest[compIdx]] : 0);
  minCost   = minDist + lambda * minRate;
  saoLcuParam->typeIdx  = m_iTypePartBest[compIdx];
  if (saoLcuParam->typeIdx != -1)
  {
    saoLcuParam->length = MAX_NUM_SAO_OFFSETS;
    Int minIndex = 0;
    if( saoLcuParam->typeIdx == SAO_BO )
    {
      if (!((saoLcuParam->mergeUpFlag )||(saoLcuParam->mergeLeftFlag))) 
      {
        saoLcuParam->bandPosition = bestClassTableBo;
      }
      minIndex = saoLcuParam->bandPosition;
    }
    for (Int i=0; i< saoLcuParam->length ; i++)
    {
      saoLcuParam->offset[i] = (Int) m_iOffset[compIdx][saoLcuParam->typeIdx][minIndex+i+1];
    }
  }
  else
  {
    saoLcuParam->length = 0;
  }

  if (addrUp>=0)
  {
    if (saoLcuParam->typeIdx == -1 && saoLcuParamUp->typeIdx == -1)
    {
      saoLcuParam->mergeUpFlag   = 1;
      saoLcuParam->mergeLeftFlag = 0;
    }
  }
  if (addrLeft>=0)
  {
    if (saoLcuParam->typeIdx == -1 && saoLcuParamLeft->typeIdx == -1)
    {
      saoLcuParam->mergeUpFlag   = 0;
      saoLcuParam->mergeLeftFlag = 1;
    }
  }
  delete saoLcuParamTmp;
}


/** check merge SAO unit 
 * \param saoLcuParam
 * \param saoLcuParamCheck
 * \param compIdx
 * \param mergeLeft
 * \param lambda
 */
Void TEncSampleAdaptiveOffset::checkMergeSaoUnit( SaoLcuParam*  saoLcuParam, SaoLcuParam*  saoLcuParamCheck, Int compIdx, Int mergeLeft, Double lambda)
{
  Int64  count=0;
  Int64  offset=0;
  Int64  offsetOrg=0;
  Int64  iEstDist = 0;
  Int    classIdx=0;
  Int    typeIdx=-1;
  Int    mergeOffset [33];
  Int64  mergeDist;
  Int    mergeRate;
  Double mergeCost;
  Int shift = g_uiBitIncrement << 1;

  if (saoLcuParamCheck->typeIdx>=0) //new
  {
    m_pcEntropyCoder->resetEntropy();
    m_pcEntropyCoder->resetBits();

    iEstDist = 0;
    typeIdx = saoLcuParamCheck->typeIdx;
    m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoFlag(1);
    if (saoLcuParamCheck->typeIdx == SAO_BO)
    {
      for(classIdx = saoLcuParamCheck->bandPosition+1; classIdx < saoLcuParamCheck->bandPosition+SAO_BO_LEN+1; classIdx++)
      {
        mergeOffset[classIdx] = saoLcuParamCheck->offset[classIdx-1-saoLcuParamCheck->bandPosition];

        count     =  m_iCount [compIdx][typeIdx][classIdx];
        offset    =  mergeOffset[classIdx];
        offsetOrg =  m_iOffsetOrg[compIdx][typeIdx][classIdx];
        iEstDist   += (( count*offset*offset-offsetOrg*offset*2 ) >> shift);
      }
    }
    else
    {
      for(classIdx=1; classIdx < MAX_NUM_SAO_OFFSETS+1; classIdx++)
      {
        mergeOffset[classIdx] = saoLcuParamCheck->offset[classIdx-1];
        count     =  m_iCount [compIdx][typeIdx][classIdx];
        offset    =  mergeOffset[classIdx];
        offsetOrg =  m_iOffsetOrg[compIdx][typeIdx][classIdx];
        iEstDist   += (( count*offset*offset-offsetOrg*offset*2 ) >> shift);
      }
    }
    mergeDist = iEstDist;
    mergeRate = m_pcEntropyCoder->getNumberOfWrittenBits();
    mergeCost = (Double)((Double)mergeDist + lambda * (Double) mergeRate) ;
    if(mergeCost < m_dCostPartBest[compIdx] && mergeCost < 0)
    {
      m_dCostPartBest[compIdx] = mergeCost;
      m_iTypePartBest[compIdx] = typeIdx;
      if (typeIdx == SAO_BO)
      {
        saoLcuParam->bandPosition   = saoLcuParamCheck->bandPosition;
        for(classIdx = saoLcuParamCheck->bandPosition+1; classIdx < saoLcuParamCheck->bandPosition+SAO_BO_LEN+1; classIdx++)
        {
          m_iOffset[compIdx][typeIdx][classIdx] = mergeOffset[classIdx];
        }
      }
      else  
      {
        for(classIdx=1; classIdx < MAX_NUM_SAO_OFFSETS+1; classIdx++)
        {
          m_iOffset[compIdx][typeIdx][classIdx] = mergeOffset[classIdx];
        }
      }
      saoLcuParam->mergeUpFlag   = !mergeLeft;
      saoLcuParam->mergeLeftFlag = mergeLeft;
    }
  }
} 

//! \}
