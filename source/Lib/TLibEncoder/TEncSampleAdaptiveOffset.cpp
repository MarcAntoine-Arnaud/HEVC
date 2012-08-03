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
#if SAO_ENCODING_CHOICE_CHROMA
  m_depthSaoRate[0][0] = 0;
  m_depthSaoRate[0][1] = 0;
  m_depthSaoRate[0][2] = 0;
  m_depthSaoRate[0][3] = 0;
  m_depthSaoRate[1][0] = 0;
  m_depthSaoRate[1][1] = 0;
  m_depthSaoRate[1][2] = 0;
  m_depthSaoRate[1][3] = 0;
#endif
}
TEncSampleAdaptiveOffset::~TEncSampleAdaptiveOffset()
{

}
// ====================================================================================================================
// Constants
// ====================================================================================================================


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
#if PICTURE_SAO_RDO_FIX
Void TEncSampleAdaptiveOffset::rdoSaoOnePart(SAOQTPart *psQTPart, Int iPartIdx, Double dLambda, Int yCbCr)
#else
Void TEncSampleAdaptiveOffset::rdoSaoOnePart(SAOQTPart *psQTPart, Int iPartIdx, Double dLambda)
#endif
{
  Int iTypeIdx;
  Int iNumTotalType = MAX_NUM_SAO_TYPE;
  SAOQTPart*  pOnePart = &(psQTPart[iPartIdx]);

  Int64 iEstDist;
  Int iClassIdx;
  Int uiShift = g_uiBitIncrement << 1;
  UInt uiDepth = pOnePart->PartLevel;

  m_iDistOrg [iPartIdx] =  0;

  Double  bestRDCostTableBo = MAX_DOUBLE;
  Int     bestClassTableBo    = 0;
  Int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
  Double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];

#if PICTURE_SAO_RDO_FIX
  Int addr;
  Int allowMergeLeft;
  Int allowMergeUp;
  Int iFrameWidthInCU = m_pcPic->getFrameWidthInCU();
  SaoLcuParam  saoLcuParamRdo;
#endif

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

#if PICTURE_SAO_RDO_FIX
    if (iTypeIdx == -1)
    {      
      for (Int ry = pOnePart->StartCUY; ry<= pOnePart->EndCUY; ry++)
      {
        for (Int rx = pOnePart->StartCUX; rx <= pOnePart->EndCUX; rx++)
        {
          addr = ry * iFrameWidthInCU + rx;          

          // get bits for iTypeIdx = -1
          allowMergeLeft = 1;
          allowMergeUp   = 1;
          if (rx != 0)
          { 
            // check tile id and slice id 
            if ( (m_pcPic->getPicSym()->getTileIdxMap(addr-1) != m_pcPic->getPicSym()->getTileIdxMap(addr)) || (m_pcPic->getCU(addr-1)->getSlice()->getSliceIdx() != m_pcPic->getCU(addr)->getSlice()->getSliceIdx()))
            {
              allowMergeLeft = 0;
            }
          }
          if (ry!=0)
          {
            if ( (m_pcPic->getPicSym()->getTileIdxMap(addr-m_iNumCuInWidth) != m_pcPic->getPicSym()->getTileIdxMap(addr)) || (m_pcPic->getCU(addr-m_iNumCuInWidth)->getSlice()->getSliceIdx() != m_pcPic->getCU(addr)->getSlice()->getSliceIdx()))
            {
              allowMergeUp = 0;
            }
          }

          // reset
          resetSaoUnit(&saoLcuParamRdo);

          // set merge flag
          saoLcuParamRdo.mergeUpFlag   = 1;
          saoLcuParamRdo.mergeLeftFlag = 1;

          if (ry == pOnePart->StartCUY) 
          {
            saoLcuParamRdo.mergeUpFlag = 0;
          } 
          
          if (rx == pOnePart->StartCUX) 
          {
            saoLcuParamRdo.mergeLeftFlag = 0;
          }

          m_pcEntropyCoder->encodeSaoUnitInterleaving(yCbCr, 1, rx, ry,  &saoLcuParamRdo, 1,  1,  allowMergeLeft, allowMergeUp);

        }
      }
    }
#else
    m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoTypeIdx(iTypeIdx+1);
#endif

    if (iTypeIdx>=0)
    {
      iEstDist = estSaoTypeDist(iPartIdx, iTypeIdx, uiShift, dLambda, currentDistortionTableBo, currentRdCostTableBo);
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
        for(iClassIdx = bestClassTableBo; iClassIdx < bestClassTableBo+SAO_BO_LEN; iClassIdx++)
        {
          iEstDist += currentDistortionTableBo[iClassIdx];
        }
      }

#if PICTURE_SAO_RDO_FIX      
      for (Int ry = pOnePart->StartCUY; ry<= pOnePart->EndCUY; ry++)
      {
        for (Int rx = pOnePart->StartCUX; rx <= pOnePart->EndCUX; rx++)
        {
          addr = ry * iFrameWidthInCU + rx;          

          // get bits for iTypeIdx = -1
          allowMergeLeft = 1;
          allowMergeUp   = 1;
          if (rx != 0)
          { 
            // check tile id and slice id 
            if ( (m_pcPic->getPicSym()->getTileIdxMap(addr-1) != m_pcPic->getPicSym()->getTileIdxMap(addr)) || (m_pcPic->getCU(addr-1)->getSlice()->getSliceIdx() != m_pcPic->getCU(addr)->getSlice()->getSliceIdx()))
            {
              allowMergeLeft = 0;
            }
          }
          if (ry!=0)
          {
            if ( (m_pcPic->getPicSym()->getTileIdxMap(addr-m_iNumCuInWidth) != m_pcPic->getPicSym()->getTileIdxMap(addr)) || (m_pcPic->getCU(addr-m_iNumCuInWidth)->getSlice()->getSliceIdx() != m_pcPic->getCU(addr)->getSlice()->getSliceIdx()))
            {
              allowMergeUp = 0;
            }
          }

          // reset
          resetSaoUnit(&saoLcuParamRdo);
          
          // set merge flag
          saoLcuParamRdo.mergeUpFlag   = 1;
          saoLcuParamRdo.mergeLeftFlag = 1;

          if (ry == pOnePart->StartCUY) 
          {
            saoLcuParamRdo.mergeUpFlag = 0;
          } 
          
          if (rx == pOnePart->StartCUX) 
          {
            saoLcuParamRdo.mergeLeftFlag = 0;
          }

          // set type and offsets
          saoLcuParamRdo.typeIdx = iTypeIdx;
#if SAO_TYPE_CODING
          saoLcuParamRdo.subTypeIdx = (iTypeIdx==SAO_BO)?bestClassTableBo:0;
#else
          saoLcuParamRdo.bandPosition = (iTypeIdx==SAO_BO)?bestClassTableBo:0;
#endif        
          saoLcuParamRdo.length = m_iNumClass[iTypeIdx];
          for (iClassIdx = 0; iClassIdx < saoLcuParamRdo.length; iClassIdx++)
          {
#if SAO_TYPE_CODING
            saoLcuParamRdo.offset[iClassIdx] = (Int)m_iOffset[iPartIdx][iTypeIdx][iClassIdx+saoLcuParamRdo.subTypeIdx+1];
#else
            saoLcuParamRdo.offset[iClassIdx] = (Int)m_iOffset[iPartIdx][iTypeIdx][iClassIdx+saoLcuParamRdo.bandPosition+1];
#endif            
          }

          m_pcEntropyCoder->encodeSaoUnitInterleaving(yCbCr, 1, rx, ry,  &saoLcuParamRdo, 1,  1,  allowMergeLeft, allowMergeUp);

        }
      }
#else // #if PICTURE_SAO_RDO_FIX 

      SaoLcuParam  saoLcuParamRdo;   
      resetSaoUnit(&saoLcuParamRdo);
      saoLcuParamRdo.typeIdx = iTypeIdx;
#if SAO_TYPE_CODING
      saoLcuParamRdo.subTypeIdx = (iTypeIdx==SAO_BO)?bestClassTableBo:0;
#else
      saoLcuParamRdo.bandPosition = (iTypeIdx==SAO_BO)?bestClassTableBo:0;
#endif
      saoLcuParamRdo.length = m_iNumClass[iTypeIdx];
      for (iClassIdx = 0; iClassIdx < saoLcuParamRdo.length; iClassIdx++)
      {
#if SAO_TYPE_CODING
        saoLcuParamRdo.offset[iClassIdx] = (Int)m_iOffset[iPartIdx][iTypeIdx][iClassIdx+saoLcuParamRdo.subTypeIdx+1];
#else
        saoLcuParamRdo.offset[iClassIdx] = (Int)m_iOffset[iPartIdx][iTypeIdx][iClassIdx+saoLcuParamRdo.bandPosition+1];
#endif
      }
      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
      m_pcRDGoOnSbacCoder->resetBits();
#if SAO_TYPE_SHARING
      m_pcEntropyCoder->encodeSaoOffset(&saoLcuParamRdo, iPartIdx);
#else
      m_pcEntropyCoder->encodeSaoOffset(&saoLcuParamRdo);
#endif
#endif // #if PICTURE_SAO_RDO_FIX 

      m_iDist[iPartIdx][iTypeIdx] = iEstDist;
      m_iRate[iPartIdx][iTypeIdx] = m_pcEntropyCoder->getNumberOfWrittenBits();

      m_dCost[iPartIdx][iTypeIdx] = (Double)((Double)m_iDist[iPartIdx][iTypeIdx] + dLambda * (Double) m_iRate[iPartIdx][iTypeIdx]);

      if(m_dCost[iPartIdx][iTypeIdx] < m_dCostPartBest[iPartIdx])
      {
        m_iDistOrg [iPartIdx] = 0;
        m_dCostPartBest[iPartIdx] = m_dCost[iPartIdx][iTypeIdx];
        m_iTypePartBest[iPartIdx] = iTypeIdx;
        if( m_bUseSBACRD )
          m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[pOnePart->PartLevel][CI_TEMP_BEST] );
      }
    }
    else
    {
      if(m_iDistOrg[iPartIdx] < m_dCostPartBest[iPartIdx] )
      {
        m_dCostPartBest[iPartIdx] = (Double) m_iDistOrg[iPartIdx] + m_pcEntropyCoder->getNumberOfWrittenBits()*dLambda ; 
        m_iTypePartBest[iPartIdx] = -1;
        if( m_bUseSBACRD )
          m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[pOnePart->PartLevel][CI_TEMP_BEST] );
      }
    }
  }

  pOnePart->bProcessed = true;
  pOnePart->bSplit     = false;
  pOnePart->iMinDist   =        m_iTypePartBest[iPartIdx] >= 0 ? m_iDist[iPartIdx][m_iTypePartBest[iPartIdx]] : m_iDistOrg[iPartIdx];
  pOnePart->iMinRate   = (Int) (m_iTypePartBest[iPartIdx] >= 0 ? m_iRate[iPartIdx][m_iTypePartBest[iPartIdx]] : 0);
  pOnePart->dMinCost   = pOnePart->iMinDist + dLambda * pOnePart->iMinRate;
  pOnePart->iBestType  = m_iTypePartBest[iPartIdx];
  if (pOnePart->iBestType != -1)
  {
    //     pOnePart->bEnableFlag =  1;
    pOnePart->iLength = m_iNumClass[pOnePart->iBestType];
    Int minIndex = 0;
    if( pOnePart->iBestType == SAO_BO )
    {
#if SAO_TYPE_CODING
      pOnePart->subTypeIdx = bestClassTableBo;
      minIndex = pOnePart->subTypeIdx;
#else
      pOnePart->bandPosition = bestClassTableBo;
      minIndex = pOnePart->bandPosition;
#endif
    }
    for (Int i=0; i< pOnePart->iLength ; i++)
    {
      pOnePart->iOffset[i] = (Int) m_iOffset[iPartIdx][pOnePart->iBestType][minIndex+i+1];
    }

  }
  else
  {
    //     pOnePart->bEnableFlag = 0;
    pOnePart->iLength     = 0;
  }
}

/** Run partition tree disable
 */
Void TEncSampleAdaptiveOffset::disablePartTree(SAOQTPart *psQTPart, Int iPartIdx)
{
  SAOQTPart*  pOnePart= &(psQTPart[iPartIdx]);
  pOnePart->bSplit      = false;
  pOnePart->iLength     =  0;
  pOnePart->iBestType   = -1;

  if (pOnePart->PartLevel < m_uiMaxSplitLevel)
  {
    for (Int i=0; i<NUM_DOWN_PART; i++)
    {
      disablePartTree(psQTPart, pOnePart->DownPartsIdx[i]);
    }
  }
}

/** Run quadtree decision function
 * \param  iPartIdx, pcPicOrg, pcPicDec, pcPicRest, &dCostFinal
 */
#if PICTURE_SAO_RDO_FIX
Void TEncSampleAdaptiveOffset::runQuadTreeDecision(SAOQTPart *psQTPart, Int iPartIdx, Double &dCostFinal, Int iMaxLevel, Double dLambda, Int yCbCr)
#else
Void TEncSampleAdaptiveOffset::runQuadTreeDecision(SAOQTPart *psQTPart, Int iPartIdx, Double &dCostFinal, Int iMaxLevel, Double dLambda)
#endif
{
  SAOQTPart*  pOnePart = &(psQTPart[iPartIdx]);

  UInt uiDepth = pOnePart->PartLevel;
  UInt uhNextDepth = uiDepth+1;

  if (iPartIdx == 0)
  {
    dCostFinal = 0;
  }

  //SAO for this part
  if(!pOnePart->bProcessed)
  {
#if PICTURE_SAO_RDO_FIX
    rdoSaoOnePart (psQTPart, iPartIdx, dLambda, yCbCr);
#else
    rdoSaoOnePart (psQTPart, iPartIdx, dLambda);
#endif
  }

  //SAO for sub 4 parts
  if (pOnePart->PartLevel < iMaxLevel)
  {
    Double      dCostNotSplit = dLambda + pOnePart->dMinCost;
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
#if PICTURE_SAO_RDO_FIX
      runQuadTreeDecision(psQTPart, pOnePart->DownPartsIdx[i], dCostFinal, iMaxLevel, dLambda, yCbCr);
#else
      runQuadTreeDecision(psQTPart, pOnePart->DownPartsIdx[i], dCostFinal, iMaxLevel, dLambda);
#endif
      dCostSplit += dCostFinal;
      if( m_bUseSBACRD )
      {
        m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_TEMP_BEST]);
      }
    }

    if(dCostSplit < dCostNotSplit)
    {
      dCostFinal = dCostSplit;
      pOnePart->bSplit      = true;
      pOnePart->iLength     =  0;
      pOnePart->iBestType   = -1;
      if( m_bUseSBACRD )
      {
        m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
      }
    }
    else
    {
      dCostFinal = dCostNotSplit;
      pOnePart->bSplit = false;
      for (Int i=0; i<NUM_DOWN_PART; i++)
      {
        disablePartTree(psQTPart, pOnePart->DownPartsIdx[i]);
      }
      if( m_bUseSBACRD )
      {
        m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);
      }
    }
  }
  else
  {
    dCostFinal = pOnePart->dMinCost;
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
#if SAO_LCU_BOUNDARY
  Int iNumLcu = m_iNumCuInWidth * m_iNumCuInHeight;

  for (Int i=0;i<iNumLcu;i++)
  {
    for (Int j=0;j<3;j++)
    {
      for (Int k=0;k<MAX_NUM_SAO_TYPE;k++)
      {
        if (m_iCount_PreDblk [i][j][k])
        {
          delete [] m_iCount_PreDblk [i][j][k]; 
        }
        if (m_iOffsetOrg_PreDblk[i][j][k])
        {
          delete [] m_iOffsetOrg_PreDblk[i][j][k];
        }
      }
      if (m_iCount_PreDblk [i][j])
      {
        delete [] m_iCount_PreDblk [i][j]; 
      }
      if (m_iOffsetOrg_PreDblk[i][j])
      {
        delete [] m_iOffsetOrg_PreDblk[i][j]; 
      }
    }
    if (m_iCount_PreDblk [i])
    {
      delete [] m_iCount_PreDblk [i]; 
    }
    if (m_iOffsetOrg_PreDblk[i])
    {
      delete [] m_iOffsetOrg_PreDblk[i]; 
    }
  }
  if (m_iCount_PreDblk)
  {
    delete [] m_iCount_PreDblk  ; m_iCount_PreDblk = NULL;
  }
  if (m_iOffsetOrg_PreDblk)
  {
    delete [] m_iOffsetOrg_PreDblk ; m_iOffsetOrg_PreDblk = NULL;
  }
#endif

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
#if SAO_LCU_BOUNDARY
  Int iNumLcu = m_iNumCuInWidth * m_iNumCuInHeight;
  m_iCount_PreDblk  = new Int64 ***[iNumLcu];
  m_iOffsetOrg_PreDblk = new Int64 ***[iNumLcu];
  for (Int i=0; i<iNumLcu; i++)
  {
    m_iCount_PreDblk[i]  = new Int64 **[3];
    m_iOffsetOrg_PreDblk[i] = new Int64 **[3];

    for (Int j=0;j<3;j++)
    {
      m_iCount_PreDblk [i][j] = new Int64 *[MAX_NUM_SAO_TYPE]; 
      m_iOffsetOrg_PreDblk[i][j] = new Int64 *[MAX_NUM_SAO_TYPE]; 

      for (Int k=0;k<MAX_NUM_SAO_TYPE;k++)
      {
        m_iCount_PreDblk [i][j][k]   = new Int64 [MAX_NUM_SAO_CLASS]; 
        m_iOffsetOrg_PreDblk[i][j][k]= new Int64 [MAX_NUM_SAO_CLASS]; 
      }
    }
  }
#endif

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
    m_bUseSBACRD = true;
  m_pcPic = pcPic;
  m_pcEntropyCoder = pcEntropyCoder;

  m_pcRDGoOnSbacCoder = pcRDGoOnSbacCoder;
  m_pcEntropyCoder->setEntropyCoder(m_pcRDGoOnSbacCoder, pcPic->getSlice(0));
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
      classIdx = m_lumaTableBo[pRec[x]];
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
  Int numSkipLine = iIsChroma? 2:4;
  if (m_saoLcuBasedOptimization == 0)
  {
    numSkipLine = 0;
  }

#if SAO_SKIP_RIGHT
  Int numSkipLineRight = iIsChroma? 3:5;
  if (m_saoLcuBasedOptimization == 0)
  {
    numSkipLineRight = 0;
  }
#endif

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
#if SAO_LCU_BOUNDARY
    if( m_saoLcuBasedOptimization && m_saoLcuBoundary )
    {
      numSkipLine = iIsChroma? 1:3;
      numSkipLineRight = iIsChroma? 2:4;
    }
#endif
    iStats = m_iOffsetOrg[iPartIdx][SAO_BO];
    iCount = m_iCount    [iPartIdx][SAO_BO];

    pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
    pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

#if SAO_SKIP_RIGHT
    iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth : iLcuWidth-numSkipLineRight;
#endif

    iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight : iLcuHeight-numSkipLine;
    for (y=0; y<iEndY; y++)
    {
#if SAO_SKIP_RIGHT
      for (x=0; x<iEndX; x++)
#else
      for (x=0; x<iLcuWidth; x++)
#endif
      {
        iClassIdx = m_lumaTableBo[pRec[x]];
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
#if SAO_LCU_BOUNDARY
      if( m_saoLcuBasedOptimization && m_saoLcuBoundary )
      {
        numSkipLine = iIsChroma? 1:3;
        numSkipLineRight = iIsChroma? 3:5;
      }
#endif
      iStats = m_iOffsetOrg[iPartIdx][SAO_EO_0];
      iCount = m_iCount    [iPartIdx][SAO_EO_0];

      pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
      pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

      iStartX = (uiLPelX == 0) ? 1 : 0;
#if SAO_SKIP_RIGHT
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth-numSkipLineRight;
#else
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth;
#endif
      for (y=0; y<iLcuHeight-numSkipLine; y++)
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
#if SAO_LCU_BOUNDARY
      if( m_saoLcuBasedOptimization && m_saoLcuBoundary )
      {
        numSkipLine = iIsChroma? 2:4;
        numSkipLineRight = iIsChroma? 2:4;
      }
#endif
      iStats = m_iOffsetOrg[iPartIdx][SAO_EO_1];
      iCount = m_iCount    [iPartIdx][SAO_EO_1];

      pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
      pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

      iStartY = (uiTPelY == 0) ? 1 : 0;
#if SAO_SKIP_RIGHT
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth : iLcuWidth-numSkipLineRight;
#endif
      iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight-numSkipLine;
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
#if SAO_SKIP_RIGHT
        for (x=0; x<iEndX; x++)
#else
        for (x=0; x<iLcuWidth; x++)
#endif
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
#if SAO_LCU_BOUNDARY
      if( m_saoLcuBasedOptimization && m_saoLcuBoundary )
      {
        numSkipLine = iIsChroma? 2:4;
        numSkipLineRight = iIsChroma? 3:5;
      }
#endif
      iStats = m_iOffsetOrg[iPartIdx][SAO_EO_2];
      iCount = m_iCount    [iPartIdx][SAO_EO_2];

      pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
      pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

      iStartX = (uiLPelX == 0) ? 1 : 0;
#if SAO_SKIP_RIGHT
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth-numSkipLineRight;
#else
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth;
#endif

      iStartY = (uiTPelY == 0) ? 1 : 0;
      iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight-numSkipLine;
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
#if SAO_LCU_BOUNDARY
      if( m_saoLcuBasedOptimization && m_saoLcuBoundary )
      {
        numSkipLine = iIsChroma? 2:4;
        numSkipLineRight = iIsChroma? 3:5;
      }
#endif
      iStats = m_iOffsetOrg[iPartIdx][SAO_EO_3];
      iCount = m_iCount    [iPartIdx][SAO_EO_3];

      pOrg = getPicYuvAddr(m_pcPic->getPicYuvOrg(), iYCbCr, iAddr);
      pRec = getPicYuvAddr(m_pcPic->getPicYuvRec(), iYCbCr, iAddr);

      iStartX = (uiLPelX == 0) ? 1 : 0;
#if SAO_SKIP_RIGHT
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth-numSkipLineRight;
#else
      iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth;
#endif

      iStartY = (uiTPelY == 0) ? 1 : 0;
      iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight-numSkipLine;
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


#if SAO_LCU_BOUNDARY
Void TEncSampleAdaptiveOffset::calcSaoStatsCu_BeforeDblk( TComPic* pcPic )
//(Int iAddr, Int iPartIdx, Int iYCbCr)
{
  Int iAddr, iYCbCr;
  Int x,y;
  TComSPS *pTmpSPS =  pcPic->getSlice(0)->getSPS();

  Pel* pOrg;
  Pel* pRec;
  Int iStride;
  Int iLcuWidth  = pTmpSPS->getMaxCUHeight();
  Int iLcuHeight = pTmpSPS->getMaxCUWidth();
  UInt uiRPelX;
  UInt uiBPelY;
  Int64* iStats;
  Int64* iCount;
  Int iClassIdx;
  Int iPicWidthTmp = 0;
  Int iPicHeightTmp = 0;
  Int iStartX;
  Int iStartY;
  Int iEndX;
  Int iEndY;
  Int iFirstX, iFirstY;

  Int idxY;
  Int idxX;
  Int frameHeightInCU = m_iNumCuInHeight;
  Int frameWidthInCU  = m_iNumCuInWidth;
  Int j, k;

  Int iIsChroma;
  Int numSkipLine, numSkipLineRight;

  UInt uiLPelX, uiTPelY;
  TComDataCU *pTmpCu;

  for (idxY = 0; idxY< frameHeightInCU; idxY++)
  {
    for (idxX = 0; idxX< frameWidthInCU; idxX++)
    {
      iLcuWidth  = pTmpSPS->getMaxCUHeight();
      iLcuHeight = pTmpSPS->getMaxCUWidth();
      iAddr     = idxX  + frameWidthInCU*idxY;
      pTmpCu = pcPic->getCU(iAddr);
      uiLPelX   = pTmpCu->getCUPelX();
      uiTPelY   = pTmpCu->getCUPelY();
      for( iYCbCr = 0; iYCbCr < 3; iYCbCr++ )
      {
        iIsChroma = (iYCbCr!=0)? 1:0;

        for ( j=0;j<MAX_NUM_SAO_TYPE;j++)
        {
          for ( k=0;k< MAX_NUM_SAO_CLASS;k++)
          {
            m_iCount_PreDblk    [iAddr][iYCbCr][j][k] = 0;
            m_iOffsetOrg_PreDblk[iAddr][iYCbCr][j][k] = 0;
          }  
        }
        if( iYCbCr == 0 )
        {
          iPicWidthTmp  = m_iPicWidth;
          iPicHeightTmp = m_iPicHeight;
        }
        else if( iYCbCr == 1 )
        {
          iPicWidthTmp  = m_iPicWidth  >> iIsChroma;
          iPicHeightTmp = m_iPicHeight >> iIsChroma;
          iLcuWidth     = iLcuWidth    >> iIsChroma;
          iLcuHeight    = iLcuHeight   >> iIsChroma;
          uiLPelX       = uiLPelX      >> iIsChroma;
          uiTPelY       = uiTPelY      >> iIsChroma;
        }
        uiRPelX       = uiLPelX + iLcuWidth  ;
        uiBPelY       = uiTPelY + iLcuHeight ;
        uiRPelX       = uiRPelX > iPicWidthTmp  ? iPicWidthTmp  : uiRPelX;
        uiBPelY       = uiBPelY > iPicHeightTmp ? iPicHeightTmp : uiBPelY;
        iLcuWidth     = uiRPelX - uiLPelX;
        iLcuHeight    = uiBPelY - uiTPelY;

        iStride    =  (iYCbCr == 0)? pcPic->getStride(): pcPic->getCStride();

        //if(iSaoType == BO)

        numSkipLine = iIsChroma? 1:3;
        numSkipLineRight = iIsChroma? 2:4;

        iStats = m_iOffsetOrg_PreDblk[iAddr][iYCbCr][SAO_BO];
        iCount = m_iCount_PreDblk[iAddr][iYCbCr][SAO_BO];

        pOrg = getPicYuvAddr(pcPic->getPicYuvOrg(), iYCbCr, iAddr);
        pRec = getPicYuvAddr(pcPic->getPicYuvRec(), iYCbCr, iAddr);

        iStartX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth : iLcuWidth-numSkipLineRight;
        iStartY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight : iLcuHeight-numSkipLine;

        for (y=0; y<iLcuHeight; y++)
        {
          for (x=0; x<iLcuWidth; x++)
          {
            if( x < iStartX && y < iStartY )
              continue;

            iClassIdx = m_lumaTableBo[pRec[x]];
            if (iClassIdx)
            {
              iStats[iClassIdx] += (pOrg[x] - pRec[x]); 
              iCount[iClassIdx] ++;
            }
          }
          pOrg += iStride;
          pRec += iStride;
        }

        Int iSignLeft;
        Int iSignRight;
        Int iSignDown;
        Int iSignDown1;
        Int iSignDown2;

        UInt uiEdgeType;

        //if (iSaoType == EO_0)

        numSkipLine = iIsChroma? 1:3;
        numSkipLineRight = iIsChroma? 3:5;

        iStats = m_iOffsetOrg_PreDblk[iAddr][iYCbCr][SAO_EO_0];
        iCount = m_iCount_PreDblk[iAddr][iYCbCr][SAO_EO_0];

        pOrg = getPicYuvAddr(pcPic->getPicYuvOrg(), iYCbCr, iAddr);
        pRec = getPicYuvAddr(pcPic->getPicYuvRec(), iYCbCr, iAddr);

        iStartX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth-numSkipLineRight;
        iStartY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight : iLcuHeight-numSkipLine;
        iFirstX   = (uiLPelX == 0) ? 1 : 0;
        iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth;

        for (y=0; y<iLcuHeight; y++)
        {
          iSignLeft = xSign(pRec[iFirstX] - pRec[iFirstX-1]);
          for (x=iFirstX; x< iEndX; x++)
          {
            iSignRight =  xSign(pRec[x] - pRec[x+1]); 
            uiEdgeType =  iSignRight + iSignLeft + 2;
            iSignLeft  = -iSignRight;

            if( x < iStartX && y < iStartY )
              continue;

            iStats[m_auiEoTable[uiEdgeType]] += (pOrg[x] - pRec[x]);
            iCount[m_auiEoTable[uiEdgeType]] ++;
          }
          pOrg += iStride;
          pRec += iStride;
        }

        //if (iSaoType == EO_1)

        numSkipLine = iIsChroma? 2:4;
        numSkipLineRight = iIsChroma? 2:4;

        iStats = m_iOffsetOrg_PreDblk[iAddr][iYCbCr][SAO_EO_1];
        iCount = m_iCount_PreDblk[iAddr][iYCbCr][SAO_EO_1];

        pOrg = getPicYuvAddr(pcPic->getPicYuvOrg(), iYCbCr, iAddr);
        pRec = getPicYuvAddr(pcPic->getPicYuvRec(), iYCbCr, iAddr);

        iStartX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth : iLcuWidth-numSkipLineRight;
        iStartY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight-numSkipLine;
        iFirstY = (uiTPelY == 0) ? 1 : 0;
        iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight;
        if (iFirstY == 1)
        {
          pOrg += iStride;
          pRec += iStride;
        }

        for (x=0; x< iLcuWidth; x++)
        {
          m_iUpBuff1[x] = xSign(pRec[x] - pRec[x-iStride]);
        }
        for (y=iFirstY; y<iEndY; y++)
        {
          for (x=0; x<iLcuWidth; x++)
          {
            iSignDown     =  xSign(pRec[x] - pRec[x+iStride]); 
            uiEdgeType    =  iSignDown + m_iUpBuff1[x] + 2;
            m_iUpBuff1[x] = -iSignDown;

            if( x < iStartX && y < iStartY )
              continue;

            iStats[m_auiEoTable[uiEdgeType]] += (pOrg[x] - pRec[x]);
            iCount[m_auiEoTable[uiEdgeType]] ++;
          }
          pOrg += iStride;
          pRec += iStride;
        }

        //if (iSaoType == EO_2)

        numSkipLine = iIsChroma? 2:4;
        numSkipLineRight = iIsChroma? 3:5;

        iStats = m_iOffsetOrg_PreDblk[iAddr][iYCbCr][SAO_EO_2];
        iCount = m_iCount_PreDblk[iAddr][iYCbCr][SAO_EO_2];

        pOrg = getPicYuvAddr(pcPic->getPicYuvOrg(), iYCbCr, iAddr);
        pRec = getPicYuvAddr(pcPic->getPicYuvRec(), iYCbCr, iAddr);

        iStartX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth-numSkipLineRight;
        iStartY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight-numSkipLine;
        iFirstX   = (uiLPelX == 0) ? 1 : 0;
        iFirstY = (uiTPelY == 0) ? 1 : 0;
        iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth;
        iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight;
        if (iFirstY == 1)
        {
          pOrg += iStride;
          pRec += iStride;
        }

        for (x=iFirstX; x<iEndX; x++)
        {
          m_iUpBuff1[x] = xSign(pRec[x] - pRec[x-iStride-1]);
        }
        for (y=iFirstY; y<iEndY; y++)
        {
          iSignDown2 = xSign(pRec[iStride+iStartX] - pRec[iStartX-1]);
          for (x=iFirstX; x<iEndX; x++)
          {
            iSignDown1      =  xSign(pRec[x] - pRec[x+iStride+1]) ;
            uiEdgeType      =  iSignDown1 + m_iUpBuff1[x] + 2;
            m_iUpBufft[x+1] = -iSignDown1; 

            if( x < iStartX && y < iStartY )
              continue;

            iStats[m_auiEoTable[uiEdgeType]] += (pOrg[x] - pRec[x]);
            iCount[m_auiEoTable[uiEdgeType]] ++;
          }
          m_iUpBufft[iFirstX] = iSignDown2;
          ipSwap     = m_iUpBuff1;
          m_iUpBuff1 = m_iUpBufft;
          m_iUpBufft = ipSwap;

          pRec += iStride;
          pOrg += iStride;
        }

        //if (iSaoType == EO_3)

        numSkipLine = iIsChroma? 2:4;
        numSkipLineRight = iIsChroma? 3:5;

        iStats = m_iOffsetOrg_PreDblk[iAddr][iYCbCr][SAO_EO_3];
        iCount = m_iCount_PreDblk[iAddr][iYCbCr][SAO_EO_3];

        pOrg = getPicYuvAddr(pcPic->getPicYuvOrg(), iYCbCr, iAddr);
        pRec = getPicYuvAddr(pcPic->getPicYuvRec(), iYCbCr, iAddr);

        iStartX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth-numSkipLineRight;
        iStartY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight-numSkipLine;
        iFirstX   = (uiLPelX == 0) ? 1 : 0;
        iFirstY = (uiTPelY == 0) ? 1 : 0;
        iEndX   = (uiRPelX == iPicWidthTmp) ? iLcuWidth-1 : iLcuWidth;
        iEndY   = (uiBPelY == iPicHeightTmp) ? iLcuHeight-1 : iLcuHeight;
        if (iFirstY == 1)
        {
          pOrg += iStride;
          pRec += iStride;
        }

        for (x=iFirstX-1; x<iEndX; x++)
        {
          m_iUpBuff1[x] = xSign(pRec[x] - pRec[x-iStride+1]);
        }

        for (y=iFirstY; y<iEndY; y++)
        {
          for (x=iFirstX; x<iEndX; x++)
          {
            iSignDown1      =  xSign(pRec[x] - pRec[x+iStride-1]) ;
            uiEdgeType      =  iSignDown1 + m_iUpBuff1[x] + 2;
            m_iUpBuff1[x-1] = -iSignDown1; 

            if( x < iStartX && y < iStartY )
              continue;

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
}
#endif


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
    for (LcuIdxY = pOnePart->StartCUY; LcuIdxY<= pOnePart->EndCUY; LcuIdxY++)
    {
      for (LcuIdxX = pOnePart->StartCUX; LcuIdxX<= pOnePart->EndCUX; LcuIdxX++)
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
      for (LcuIdxY = pOnePart->StartCUY; LcuIdxY<= pOnePart->EndCUY; LcuIdxY++)
      {
        for (LcuIdxX = pOnePart->StartCUX; LcuIdxX<= pOnePart->EndCUX; LcuIdxX++)
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
          iDownPartIdx = pOnePart->DownPartsIdx[i];
          for (iTypeIdx=0; iTypeIdx<iNumTotalType; iTypeIdx++)
          {
            for (iClassIdx=0; iClassIdx< (iTypeIdx < SAO_BO ? m_iNumClass[iTypeIdx] : SAO_MAX_BO_CLASSES) +1; iClassIdx++)
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
#if SAO_ENCODING_CHOICE
Void TEncSampleAdaptiveOffset::SAOProcess(SAOParam *pcSaoParam, Double dLambdaLuma, Double dLambdaChroma, Int depth)
#else
Void TEncSampleAdaptiveOffset::SAOProcess(SAOParam *pcSaoParam, Double dLambdaLuma, Double dLambdaChroma)
#endif
#else
/** Sample adaptive offset process
 * \param dLambda
 */
Void TEncSampleAdaptiveOffset::SAOProcess(SAOParam *pcSaoParam, Double dLambda)
#endif
{

  m_eSliceType          =  m_pcPic->getSlice(0)->getSliceType();
  m_iPicNalReferenceIdc = (m_pcPic->getSlice(0)->isReferenced() ? 1 :0);

#if SAO_CHROMA_LAMBDA 
  m_dLambdaLuma    = dLambdaLuma;
  m_dLambdaChroma  = dLambdaChroma;
#else
  m_dLambdaLuma    = dLambda;
  m_dLambdaChroma  = dLambda;
#endif

  if(m_bUseNIF)
  {
    m_pcPic->getPicYuvRec()->copyToPic(m_pcYuvTmp);
  }

#if FULL_NBIT
  m_uiSaoBitIncrease = g_uiBitDepth + (g_uiBitDepth-8) - min((Int)(g_uiBitDepth + (g_uiBitDepth-8)), 10);
#else
  m_uiSaoBitIncrease = g_uiBitDepth + g_uiBitIncrement - min((Int)(g_uiBitDepth + g_uiBitIncrement), 10);
#endif
  
#if FULL_NBIT
  m_iOffsetTh = 1 << ( min((Int)(g_uiBitDepth + (g_uiBitDepth-8)-5),5) );
#else
  m_iOffsetTh = 1 << ( min((Int)(g_uiBitDepth + g_uiBitIncrement-5),5) );
#endif
  resetSAOParam(pcSaoParam);
#if SAO_LCU_BOUNDARY
  if( !m_saoLcuBasedOptimization || !m_saoLcuBoundary )
  {
    resetStats();
  }
#else
  resetStats();
#endif
#if !SAO_TYPE_SHARING
  Int iY  = 0;
#endif
  Double dCostFinal = 0;
  if ( m_saoLcuBasedOptimization)
  {
#if SAO_ENCODING_CHOICE
    rdoSaoUnitAll(pcSaoParam, dLambdaLuma, dLambdaChroma, depth);
#else
    rdoSaoUnitAll(pcSaoParam, dLambdaLuma, dLambdaChroma);
#endif
  }
  else
  {
#if SAO_TYPE_SHARING
    pcSaoParam->bSaoFlag[0] = 1;
    pcSaoParam->bSaoFlag[1] = 0;
    dCostFinal = 0;
    Double lambdaRdo =  dLambdaLuma;
    resetStats();
    getSaoStats(pcSaoParam->psSaoPart[0], 0);
#if PICTURE_SAO_RDO_FIX
    runQuadTreeDecision(pcSaoParam->psSaoPart[0], 0, dCostFinal, m_uiMaxSplitLevel, lambdaRdo, 0);
#else
    runQuadTreeDecision(pcSaoParam->psSaoPart[0], 0, dCostFinal, m_uiMaxSplitLevel, lambdaRdo);
#endif
    pcSaoParam->bSaoFlag[0] = dCostFinal < 0 ? 1:0;
    if(pcSaoParam->bSaoFlag[0])
    {
      convertQT2SaoUnit(pcSaoParam, 0, 0);
      assignSaoUnitSyntax(pcSaoParam->saoLcuParam[0],  pcSaoParam->psSaoPart[0], pcSaoParam->oneUnitFlag[0], 0);
    }
#else
    pcSaoParam->bSaoFlag[0] = 1;
    pcSaoParam->bSaoFlag[1] = 0;
    pcSaoParam->bSaoFlag[2] = 0;
    for (Int compIdx=0;compIdx<3;compIdx++)
    {
      if (pcSaoParam->bSaoFlag[iY])
      {
        dCostFinal = 0;
        Double lambdaRdo = (compIdx==0 ? dLambdaLuma: dLambdaChroma);
        resetStats();
        getSaoStats(pcSaoParam->psSaoPart[compIdx], compIdx);
#if PICTURE_SAO_RDO_FIX
        runQuadTreeDecision(pcSaoParam->psSaoPart[compIdx], 0, dCostFinal, m_uiMaxSplitLevel, lambdaRdo, compIdx);        
#else
        runQuadTreeDecision(pcSaoParam->psSaoPart[compIdx], 0, dCostFinal, m_uiMaxSplitLevel, lambdaRdo);
#endif

        pcSaoParam->bSaoFlag[compIdx] = dCostFinal < 0 ? 1:0;
        if(pcSaoParam->bSaoFlag[compIdx])
        {
          convertQT2SaoUnit(pcSaoParam, 0, compIdx);
          assignSaoUnitSyntax(pcSaoParam->saoLcuParam[compIdx],  pcSaoParam->psSaoPart[compIdx], pcSaoParam->oneUnitFlag[compIdx], compIdx);
        }
      }
    }
#endif
  }
#if SAO_TYPE_SHARING
  if (pcSaoParam->bSaoFlag[0])
  {
    processSaoUnitAll( pcSaoParam->saoLcuParam[0], pcSaoParam->oneUnitFlag[0], 0);
  }
  if (pcSaoParam->bSaoFlag[1])
  {
    processSaoUnitAll( pcSaoParam->saoLcuParam[1], pcSaoParam->oneUnitFlag[1], 1);
    processSaoUnitAll( pcSaoParam->saoLcuParam[2], pcSaoParam->oneUnitFlag[2], 2);
  }
#else
  for (Int compIdx=0;compIdx<3;compIdx++)
  {
    if (pcSaoParam->bSaoFlag[compIdx])
    {
      processSaoUnitAll( pcSaoParam->saoLcuParam[compIdx], pcSaoParam->oneUnitFlag[compIdx], compIdx);
    }
  }
#endif
}
/** Check merge SAO unit
 * \param saoUnitCurr current SAO unit 
 * \param saoUnitCheck SAO unit tobe check
 * \param dir direction
 */
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
#if SAO_TYPE_CODING
        countDiff += (saoUnitCurr->subTypeIdx != saoUnitCheck->subTypeIdx);
#else
        countDiff += (saoUnitCurr->bandPosition != saoUnitCheck->bandPosition);
#endif
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
 * \param saoLcuParam SAO LCU parameters
 * \param saoPart SAO part
 * \param oneUnitFlag SAO one unit flag
 * \param iYCbCr color component Index
 */
Void TEncSampleAdaptiveOffset::assignSaoUnitSyntax(SaoLcuParam* saoLcuParam,  SAOQTPart* saoPart, Bool &oneUnitFlag, Int yCbCr)
{
  if (saoPart->bSplit == 0)
  {
    oneUnitFlag = 1;
  }
  else
  {
    Int i,j, addr, addrUp, addrLeft,  idx, idxUp, idxLeft,  idxCount;

    oneUnitFlag = 0;

    idxCount = -1;
    saoLcuParam[0].mergeUpFlag = 0;
    saoLcuParam[0].mergeLeftFlag = 0;

    for (j=0;j<m_iNumCuInHeight;j++)
    {
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
      }
    }
  }
}
/** rate distortion optimization of all SAO units
 * \param saoParam SAO parameters
 * \param lambda 
 * \param lambdaChroma
 */
#if SAO_ENCODING_CHOICE
Void TEncSampleAdaptiveOffset::rdoSaoUnitAll(SAOParam *saoParam, Double lambda, Double lambdaChroma, Int depth)
#else
Void TEncSampleAdaptiveOffset::rdoSaoUnitAll(SAOParam *saoParam, Double lambda, Double lambdaChroma)
#endif
{

  Int idxY;
  Int idxX;
  Int frameHeightInCU = saoParam->numCuInHeight;
  Int frameWidthInCU  = saoParam->numCuInWidth;
  Int j, k;
  Int addr = 0;
  Int addrUp = -1;
  Int addrLeft = -1;
  Int compIdx = 0;
#if !SAO_TYPE_SHARING
  Double lambdaComp;
#endif
#if SAO_SINGLE_MERGE 
  SaoLcuParam mergeSaoParam[3][2];
  Double compDistortion[3];
#endif

  saoParam->bSaoFlag[0] = true;
  saoParam->bSaoFlag[1] = true;
#if !SAO_TYPE_SHARING
  saoParam->bSaoFlag[2] = true;
#endif
  saoParam->oneUnitFlag[0] = false;
  saoParam->oneUnitFlag[1] = false;
  saoParam->oneUnitFlag[2] = false;

#if SAO_ENCODING_CHOICE
#if SAO_ENCODING_CHOICE_CHROMA
  Int numNoSao[2];
  numNoSao[0] = 0;// Luma 
  numNoSao[1] = 0;// Chroma 
  if( depth > 0 && m_depthSaoRate[0][depth-1] > SAO_ENCODING_RATE )
  {
    saoParam->bSaoFlag[0] = false;
  }
  if( depth > 0 && m_depthSaoRate[1][depth-1] > SAO_ENCODING_RATE_CHROMA )
  {
    saoParam->bSaoFlag[1] = false;
#if !SAO_TYPE_SHARING
    saoParam->bSaoFlag[2] = false;
#endif
  }
#else
  Int numNoSao = 0;

  if( depth > 0 && m_depth0SaoRate > SAO_ENCODING_RATE )
  {
    saoParam->bSaoFlag[0] = false;
    saoParam->bSaoFlag[1] = false;
#if !SAO_TYPE_SHARING
    saoParam->bSaoFlag[2] = false;
#endif
  }
#endif
#endif

  for (idxY = 0; idxY< frameHeightInCU; idxY++)
  {
    for (idxX = 0; idxX< frameWidthInCU; idxX++)
    {
      addr     = idxX  + frameWidthInCU*idxY;
      addrUp   = addr < frameWidthInCU ? -1:idxX   + frameWidthInCU*(idxY-1);
      addrLeft = idxX == 0               ? -1:idxX-1 + frameWidthInCU*idxY;
#if SAO_SINGLE_MERGE
      Int allowMergeLeft = 1;
      Int allowMergeUp   = 1;
      UInt rate;
      Double bestCost, mergeCost;
      if (idxX!=0)
      { 
        // check tile id and slice id 
        if ( (m_pcPic->getPicSym()->getTileIdxMap(addr-1) != m_pcPic->getPicSym()->getTileIdxMap(addr)) || (m_pcPic->getCU(addr-1)->getSlice()->getSliceIdx() != m_pcPic->getCU(addr)->getSlice()->getSliceIdx()))
        {
          allowMergeLeft = 0;
        }
      }
      else
      {
        allowMergeLeft = 0;
      }
      if (idxY!=0)
      {
        if ( (m_pcPic->getPicSym()->getTileIdxMap(addr-m_iNumCuInWidth) != m_pcPic->getPicSym()->getTileIdxMap(addr)) || (m_pcPic->getCU(addr-m_iNumCuInWidth)->getSlice()->getSliceIdx() != m_pcPic->getCU(addr)->getSlice()->getSliceIdx()))
        {
          allowMergeUp = 0;
        }
      }
      else
      {
        allowMergeUp = 0;
      }

      compDistortion[0] = 0; 
      compDistortion[1] = 0; 
      compDistortion[2] = 0;
      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
      if (allowMergeLeft)
      {
#if SAO_MERGE_ONE_CTX
        m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(0); 
#else
        m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMergeLeft(0, 0); 
#endif
      }
      if (allowMergeUp)
      {
#if SAO_MERGE_ONE_CTX
        m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(0);
#else
        m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMergeUp(0);
#endif
      }
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[0][CI_TEMP_BEST] );
#endif
      // reset stats Y, Cb, Cr
      for ( compIdx=0;compIdx<3;compIdx++)
      {
        for ( j=0;j<MAX_NUM_SAO_TYPE;j++)
        {
          for ( k=0;k< MAX_NUM_SAO_CLASS;k++)
          {
#if SAO_LCU_BOUNDARY
            m_iOffset   [compIdx][j][k] = 0;
            if( m_saoLcuBasedOptimization && m_saoLcuBoundary ){
              m_iCount    [compIdx][j][k] = m_iCount_PreDblk    [addr][compIdx][j][k];
              m_iOffsetOrg[compIdx][j][k] = m_iOffsetOrg_PreDblk[addr][compIdx][j][k];
            }
            else
            {
              m_iCount    [compIdx][j][k] = 0;
              m_iOffsetOrg[compIdx][j][k] = 0;
            }
#else
            m_iCount    [compIdx][j][k] = 0;
            m_iOffset   [compIdx][j][k] = 0;
            m_iOffsetOrg[compIdx][j][k] = 0;
#endif
          }  
        }
        saoParam->saoLcuParam[compIdx][addr].typeIdx       =  -1;
        saoParam->saoLcuParam[compIdx][addr].mergeUpFlag   = 0;
        saoParam->saoLcuParam[compIdx][addr].mergeLeftFlag = 0;
#if SAO_TYPE_CODING
        saoParam->saoLcuParam[compIdx][addr].subTypeIdx    = 0;
#else
        saoParam->saoLcuParam[compIdx][addr].bandPosition  = 0;
#endif
#if !SAO_TYPE_SHARING
       lambdaComp = compIdx==0 ? lambda : lambdaChroma;
#endif
#if SAO_ENCODING_CHOICE
#if SAO_TYPE_SHARING
  if( (compIdx ==0 && saoParam->bSaoFlag[0])|| (compIdx >0 && saoParam->bSaoFlag[1]) )
#else
  if( saoParam->bSaoFlag[compIdx] )
#endif
#endif
        {
          calcSaoStatsCu(addr, compIdx,  compIdx);

#if SAO_TYPE_SHARING
       }
      }
      saoComponentParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft, 0,  lambda, &mergeSaoParam[0][0], &compDistortion[0]);
      sao2ChromaParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft, lambdaChroma, &mergeSaoParam[1][0], &mergeSaoParam[2][0], &compDistortion[0]);
#else
#if SAO_SINGLE_MERGE
          saoComponentParamDist(allowMergeLeft, allowMergeUp, saoParam, addr, addrUp, addrLeft, compIdx,  lambdaComp, &mergeSaoParam[compIdx][0], &compDistortion[0]);
#else
          rdoSaoUnit (idxX, idxY, saoParam, addr, addrUp, addrLeft, compIdx,  lambdaComp);
#endif
#if SAO_ENCODING_CHOICE
#if !SAO_SINGLE_MERGE
#if SAO_ENCODING_CHOICE_CHROMA
          if( saoParam->saoLcuParam[compIdx][addr].typeIdx == -1)
          {
            numNoSao[compIdx>0?1:0]++;
          }
#else
          if( depth == 0 && saoParam->saoLcuParam[compIdx][addr].typeIdx == -1)
          {
            numNoSao++;
          }
#endif
#endif
        }
#endif
#if !REMOVE_SAO_LCU_ENC_CONSTRAINTS_3
        if (compIdx!=0)
        {
          if ( saoParam->saoLcuParam[compIdx][0].typeIdx == -1 )
          {
            saoParam->bSaoFlag[compIdx] = false;
          }
        }
#endif
      }
#endif
#if SAO_SINGLE_MERGE   
#if SAO_TYPE_SHARING
     if( saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1] )
#else
     if( saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1] || saoParam->bSaoFlag[2])
#endif
      {
        // Cost of new SAO_params
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
        m_pcRDGoOnSbacCoder->resetBits();
        if (allowMergeLeft)
        {
#if SAO_MERGE_ONE_CTX
          m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(0); 
#else
          m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMergeLeft(0, 0); 
#endif
        }
        if (allowMergeUp)
        {
#if SAO_MERGE_ONE_CTX
          m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(0);
#else
          m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMergeUp(0);
#endif
        }
        for ( compIdx=0;compIdx<3;compIdx++)
        {
#if SAO_TYPE_SHARING
        if( (compIdx ==0 && saoParam->bSaoFlag[0]) || (compIdx >0 && saoParam->bSaoFlag[1]))
#else
        if( saoParam->bSaoFlag[compIdx])
#endif
          {
#if SAO_TYPE_SHARING
           m_pcEntropyCoder->encodeSaoOffset(&saoParam->saoLcuParam[compIdx][addr], compIdx);
#else
           m_pcEntropyCoder->encodeSaoOffset(&saoParam->saoLcuParam[compIdx][addr]);
#endif
          }
        }

        rate = m_pcEntropyCoder->getNumberOfWrittenBits();
        bestCost = compDistortion[0] + (Double)rate;
        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[0][CI_TEMP_BEST]);

        // Cost of Merge
        for(Int mergeUp=0; mergeUp<2; ++mergeUp)
        {
          if ( (allowMergeLeft && (mergeUp==0)) || (allowMergeUp && (mergeUp==1)) )
          {
            m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
            m_pcRDGoOnSbacCoder->resetBits();
            if (allowMergeLeft)
            {
#if SAO_MERGE_ONE_CTX
              m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(1-mergeUp); 
#else
              m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMergeLeft(1-mergeUp, 0); 
#endif
            }
            if ( allowMergeUp && (mergeUp==1) )
            {
#if SAO_MERGE_ONE_CTX
              m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMerge(1); 
#else
              m_pcEntropyCoder->m_pcEntropyCoderIf->codeSaoMergeUp(1); 
#endif
            }

            rate = m_pcEntropyCoder->getNumberOfWrittenBits();
            mergeCost = compDistortion[mergeUp+1] + (Double)rate;
            if (mergeCost < bestCost)
            {
              bestCost = mergeCost;
              m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[0][CI_TEMP_BEST]);              
              for ( compIdx=0;compIdx<3;compIdx++)
              {
                mergeSaoParam[compIdx][mergeUp].mergeLeftFlag = 1-mergeUp;
                mergeSaoParam[compIdx][mergeUp].mergeUpFlag = mergeUp;
#if SAO_TYPE_SHARING
                if( (compIdx==0 && saoParam->bSaoFlag[0]) || (compIdx>0 && saoParam->bSaoFlag[1]))
#else
                if( saoParam->bSaoFlag[compIdx])
#endif
                {
                  copySaoUnit(&saoParam->saoLcuParam[compIdx][addr], &mergeSaoParam[compIdx][mergeUp] );             
                }
              }
            }
          }
        }
#if SAO_ENCODING_CHOICE
#if SAO_ENCODING_CHOICE_CHROMA
if( saoParam->saoLcuParam[0][addr].typeIdx == -1)
{
  numNoSao[0]++;
}
#if SAO_TYPE_SHARING
if( saoParam->saoLcuParam[1][addr].typeIdx == -1)
{
  numNoSao[1]+=2;
}
#else
if( saoParam->saoLcuParam[1][addr].typeIdx == -1)
{
  numNoSao[1]++;
}
if( saoParam->saoLcuParam[2][addr].typeIdx == -1)
{
  numNoSao[1]++;
}
#endif
#else
        for ( compIdx=0;compIdx<3;compIdx++)
        {
          if( depth == 0 && saoParam->saoLcuParam[compIdx][addr].typeIdx == -1)
          {
            numNoSao++;
          }
        }
#endif
#endif
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_TEMP_BEST]);
        m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
      }
#endif
    }
  }
#if SAO_ENCODING_CHOICE
#if SAO_ENCODING_CHOICE_CHROMA
m_depthSaoRate[0][depth] = numNoSao[0]/((Double) frameHeightInCU*frameWidthInCU);
m_depthSaoRate[1][depth] = numNoSao[1]/((Double) frameHeightInCU*frameWidthInCU*2);
#else
  if( depth == 0)
  {
    // update SAO Rate
    m_depth0SaoRate = numNoSao/((Double) frameHeightInCU*frameWidthInCU*3);
  }
#endif
#endif

}
/** rate distortion optimization of SAO unit 
 * \param saoParam SAO parameters
 * \param addr address 
 * \param addrUp above address
 * \param addrLeft left address 
 * \param yCbCr color component index
 * \param lambda 
 */
inline Int64 TEncSampleAdaptiveOffset::estSaoTypeDist(Int compIdx, Int typeIdx, Int shift, Double lambda, Int *currentDistortionTableBo, Double *currentRdCostTableBo)
{
  Int64 estDist = 0;
  Int classIdx;
  for(classIdx=1; classIdx < ( (typeIdx < SAO_BO) ?  m_iNumClass[typeIdx]+1 : SAO_MAX_BO_CLASSES+1); classIdx++)
  {
    if( typeIdx == SAO_BO)
    {
      currentDistortionTableBo[classIdx-1] = 0;
      currentRdCostTableBo[classIdx-1] = lambda;
    }
    if(m_iCount [compIdx][typeIdx][classIdx])
    {
#if FULL_NBIT
      m_iOffset[compIdx][typeIdx][classIdx] = (Int64) xRoundIbdi((Double)(m_iOffsetOrg[compIdx][typeIdx][classIdx]<<g_uiBitDepth-8)   / (Double)(m_iCount [compIdx][typeIdx][classIdx]<<m_uiSaoBitIncrease));
#else
      m_iOffset[compIdx][typeIdx][classIdx] = (Int64) xRoundIbdi((Double)(m_iOffsetOrg[compIdx][typeIdx][classIdx]<<g_uiBitIncrement) / (Double)(m_iCount [compIdx][typeIdx][classIdx]<<m_uiSaoBitIncrease));
#endif
      m_iOffset[compIdx][typeIdx][classIdx] = Clip3(-m_iOffsetTh+1, m_iOffsetTh-1, (Int)m_iOffset[compIdx][typeIdx][classIdx]);
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
      m_iOffset[compIdx][typeIdx][classIdx] = estIterOffset( typeIdx, classIdx, lambda, m_iOffset[compIdx][typeIdx][classIdx], m_iCount [compIdx][typeIdx][classIdx], m_iOffsetOrg[compIdx][typeIdx][classIdx], shift, m_uiSaoBitIncrease, currentDistortionTableBo, currentRdCostTableBo );
    }
    else
    {
      m_iOffsetOrg[compIdx][typeIdx][classIdx] = 0;
      m_iOffset[compIdx][typeIdx][classIdx] = 0;
    }
    if( typeIdx != SAO_BO )
    {
      estDist   += estSaoDist( m_iCount [compIdx][typeIdx][classIdx], m_iOffset[compIdx][typeIdx][classIdx] << m_uiSaoBitIncrease, m_iOffsetOrg[compIdx][typeIdx][classIdx], shift);
    }

  }
  return estDist;
}

inline Int64 TEncSampleAdaptiveOffset::estSaoDist(Int64 count, Int64 offset, Int64 offsetOrg, Int shift)
{
  return (( count*offset*offset-offsetOrg*offset*2 ) >> shift);
}
inline Int64 TEncSampleAdaptiveOffset::estIterOffset(Int typeIdx, Int classIdx, double lambda, Int64 offsetInput, Int64 count, Int64 offsetOrg, Int shift, Int bitIncrease, Int *currentDistortionTableBo, Double *currentRdCostTableBo )
{
  //Clean up, best_q_offset.
  Int64 iterOffset, tempOffset;
  Int64 tempDist, tempRate;
  Double tempCost, tempMinCost;
  Int64 offsetOutput = 0;
  iterOffset = offsetInput;
  // Assuming sending quantized value 0 results in zero offset and sending the value zero needs 1 bit. entropy coder can be used to measure the exact rate here. 
  tempMinCost = lambda; 
  while (iterOffset != 0)
  {
    // Calculate the bits required for signalling the offset
    tempRate = (typeIdx == SAO_BO) ? (abs((Int)iterOffset)+2) : (abs((Int)iterOffset)+1);
    if (abs((Int)iterOffset)==m_iOffsetTh-1) 
    {  
      tempRate --;
    }
    // Do the dequntization before distorion calculation
    tempOffset  = iterOffset << bitIncrease;
    tempDist    = estSaoDist( count, tempOffset, offsetOrg, shift);
    tempCost    = ((Double)tempDist + lambda * (Double) tempRate);
    if(tempCost < tempMinCost)
    {
      tempMinCost = tempCost;
      offsetOutput = iterOffset;
      if(typeIdx == SAO_BO)
      {
        currentDistortionTableBo[classIdx-1] = (Int) tempDist;
        currentRdCostTableBo[classIdx-1] = tempCost;
      }
    }
    iterOffset = (iterOffset > 0) ? (iterOffset-1):(iterOffset+1);
  }
  return offsetOutput;
}


#if SAO_SINGLE_MERGE
Void TEncSampleAdaptiveOffset::saoComponentParamDist(Int allowMergeLeft, Int allowMergeUp, SAOParam *saoParam, Int addr, Int addrUp, Int addrLeft, Int yCbCr, Double lambda, SaoLcuParam *compSaoParam, Double *compDistortion)
{
  Int typeIdx;

  Int64 estDist;
  Int classIdx;
  Int shift = g_uiBitIncrement << 1;
  Int64 bestDist;

  SaoLcuParam*  saoLcuParam = &(saoParam->saoLcuParam[yCbCr][addr]);
  SaoLcuParam*  saoLcuParamNeighbor = NULL; 

  resetSaoUnit(saoLcuParam);
  resetSaoUnit(&compSaoParam[0]);
  resetSaoUnit(&compSaoParam[1]);


  Double dCostPartBest = MAX_DOUBLE;

  Double  bestRDCostTableBo = MAX_DOUBLE;
  Int     bestClassTableBo    = 0;
  Int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
  Double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];


  SaoLcuParam   saoLcuParamRdo;   
  Double   estRate = 0;

  resetSaoUnit(&saoLcuParamRdo);

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_TEMP_BEST]);
  m_pcRDGoOnSbacCoder->resetBits();
#if SAO_TYPE_SHARING
 m_pcEntropyCoder->encodeSaoOffset(&saoLcuParamRdo, yCbCr);
#else
 m_pcEntropyCoder->encodeSaoOffset(&saoLcuParamRdo);
#endif
  
  dCostPartBest = m_pcEntropyCoder->getNumberOfWrittenBits()*lambda ; 
  copySaoUnit(saoLcuParam, &saoLcuParamRdo );
  bestDist = 0;
  


  for (typeIdx=0; typeIdx<MAX_NUM_SAO_TYPE; typeIdx++)
  {
    estDist = estSaoTypeDist(yCbCr, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);

    if( typeIdx == SAO_BO )
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
      estDist = 0;
      for(classIdx = bestClassTableBo; classIdx < bestClassTableBo+SAO_BO_LEN; classIdx++)
      {
        estDist += currentDistortionTableBo[classIdx];
      }
    }
    resetSaoUnit(&saoLcuParamRdo);
    saoLcuParamRdo.length = m_iNumClass[typeIdx];
    saoLcuParamRdo.typeIdx = typeIdx;
    saoLcuParamRdo.mergeLeftFlag = 0;
    saoLcuParamRdo.mergeUpFlag   = 0;
#if SAO_TYPE_CODING
    saoLcuParamRdo.subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
#else
    saoLcuParamRdo.bandPosition = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
#endif
    for (classIdx = 0; classIdx < saoLcuParamRdo.length; classIdx++)
    {
#if SAO_TYPE_CODING
      saoLcuParamRdo.offset[classIdx] = (Int)m_iOffset[yCbCr][typeIdx][classIdx+saoLcuParamRdo.subTypeIdx+1];
#else
      saoLcuParamRdo.offset[classIdx] = (Int)m_iOffset[yCbCr][typeIdx][classIdx+saoLcuParamRdo.bandPosition+1];
#endif
    }
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_TEMP_BEST]);
    m_pcRDGoOnSbacCoder->resetBits();
#if SAO_TYPE_SHARING
    m_pcEntropyCoder->encodeSaoOffset(&saoLcuParamRdo, yCbCr);
#else
    m_pcEntropyCoder->encodeSaoOffset(&saoLcuParamRdo);
#endif

    estRate = m_pcEntropyCoder->getNumberOfWrittenBits();
    m_dCost[yCbCr][typeIdx] = (Double)((Double)estDist + lambda * (Double) estRate);

    if(m_dCost[yCbCr][typeIdx] < dCostPartBest)
    {
      dCostPartBest = m_dCost[yCbCr][typeIdx];
      copySaoUnit(saoLcuParam, &saoLcuParamRdo );
      bestDist = estDist;       
    }
  }
  compDistortion[0] += ((Double)bestDist/lambda);
  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_TEMP_BEST]);
#if SAO_TYPE_SHARING
 m_pcEntropyCoder->encodeSaoOffset(saoLcuParam, yCbCr);
#else
 m_pcEntropyCoder->encodeSaoOffset(saoLcuParam);
#endif 
  m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[0][CI_TEMP_BEST] );


  // merge left or merge up

  for (Int idxNeighbor=0;idxNeighbor<2;idxNeighbor++) 
  {
    saoLcuParamNeighbor = NULL;
    if (allowMergeLeft && addrLeft>=0 && idxNeighbor ==0)
    {
      saoLcuParamNeighbor = &(saoParam->saoLcuParam[yCbCr][addrLeft]);
    }
    else if (allowMergeUp && addrUp>=0 && idxNeighbor ==1)
    {
      saoLcuParamNeighbor = &(saoParam->saoLcuParam[yCbCr][addrUp]);
    }
    if (saoLcuParamNeighbor!=NULL)
    {
      estDist = 0;
      typeIdx = saoLcuParamNeighbor->typeIdx;
      if (typeIdx>=0) 
      {
#if SAO_TYPE_CODING
        Int mergeBandPosition = (typeIdx == SAO_BO)?saoLcuParamNeighbor->subTypeIdx:0;
#else
        Int mergeBandPosition = (typeIdx == SAO_BO)?saoLcuParamNeighbor->bandPosition:0;
#endif        
        Int   merge_iOffset;
        for(classIdx = 0; classIdx < m_iNumClass[typeIdx]; classIdx++)
        {
          merge_iOffset = saoLcuParamNeighbor->offset[classIdx];
          estDist   += estSaoDist(m_iCount [yCbCr][typeIdx][classIdx+mergeBandPosition+1], merge_iOffset, m_iOffsetOrg[yCbCr][typeIdx][classIdx+mergeBandPosition+1],  shift);
        }
      }
      else
      {
        estDist = 0;
      }

      copySaoUnit(&compSaoParam[idxNeighbor], saoLcuParamNeighbor );
      compSaoParam[idxNeighbor].mergeUpFlag   = idxNeighbor;
      compSaoParam[idxNeighbor].mergeLeftFlag = !idxNeighbor;

      compDistortion[idxNeighbor+1] += ((Double)estDist/lambda);
    } 
  } 
}
#if SAO_TYPE_SHARING
Void TEncSampleAdaptiveOffset::sao2ChromaParamDist(Int allowMergeLeft, Int allowMergeUp, SAOParam *saoParam, Int addr, Int addrUp, Int addrLeft, Double lambda, SaoLcuParam *crSaoParam, SaoLcuParam *cbSaoParam, Double *distortion)
{
  Int typeIdx;

  Int64 estDist[2];
  Int classIdx;
  Int shift = g_uiBitIncrement << 1;
  Int64 bestDist = 0;

  SaoLcuParam*  saoLcuParam[2] = {&(saoParam->saoLcuParam[1][addr]), &(saoParam->saoLcuParam[2][addr])};
  SaoLcuParam*  saoLcuParamNeighbor[2] = {NULL, NULL}; 
  SaoLcuParam*  saoMergeParam[2][2];
  saoMergeParam[0][0] = &crSaoParam[0];
  saoMergeParam[0][1] = &crSaoParam[1]; 
  saoMergeParam[1][0] = &cbSaoParam[0];
  saoMergeParam[1][1] = &cbSaoParam[1];

  resetSaoUnit(saoLcuParam[0]);
  resetSaoUnit(saoLcuParam[1]);
  resetSaoUnit(saoMergeParam[0][0]);
  resetSaoUnit(saoMergeParam[0][1]);
  resetSaoUnit(saoMergeParam[1][0]);
  resetSaoUnit(saoMergeParam[1][1]);


  Double costPartBest = MAX_DOUBLE;

  Double  bestRDCostTableBo;
  Int     bestClassTableBo[2]    = {0, 0};
  Int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
  Double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];

  SaoLcuParam   saoLcuParamRdo[2];   
  Double   estRate = 0;

  resetSaoUnit(&saoLcuParamRdo[0]);
  resetSaoUnit(&saoLcuParamRdo[1]);

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_TEMP_BEST]);
  m_pcRDGoOnSbacCoder->resetBits();
  m_pcEntropyCoder->encodeSaoOffset(&saoLcuParamRdo[0], 1);
  m_pcEntropyCoder->encodeSaoOffset(&saoLcuParamRdo[1], 2);
  
  costPartBest = m_pcEntropyCoder->getNumberOfWrittenBits()*lambda ; 
  copySaoUnit(saoLcuParam[0], &saoLcuParamRdo[0] );
  copySaoUnit(saoLcuParam[1], &saoLcuParamRdo[1] );

  for (typeIdx=0; typeIdx<MAX_NUM_SAO_TYPE; typeIdx++)
  {
    if( typeIdx == SAO_BO )
    {
      // Estimate Best Position
      for(Int compIdx = 0; compIdx < 2; compIdx++)
      {
        Double currentRDCost = 0.0;
        bestRDCostTableBo = MAX_DOUBLE;
        estDist[compIdx] = estSaoTypeDist(compIdx+1, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
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
            bestClassTableBo[compIdx]  = i;
          }
        }

        // Re code all Offsets
        // Code Center
        estDist[compIdx] = 0;
        for(classIdx = bestClassTableBo[compIdx]; classIdx < bestClassTableBo[compIdx]+SAO_BO_LEN; classIdx++)
        {
          estDist[compIdx] += currentDistortionTableBo[classIdx];
        }
      }
    }
    else
    {
      estDist[0] = estSaoTypeDist(1, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
      estDist[1] = estSaoTypeDist(2, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);
    }

    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_TEMP_BEST]);
    m_pcRDGoOnSbacCoder->resetBits();

    for(Int compIdx = 0; compIdx < 2; compIdx++)
    {
      resetSaoUnit(&saoLcuParamRdo[compIdx]);
      saoLcuParamRdo[compIdx].length = m_iNumClass[typeIdx];
      saoLcuParamRdo[compIdx].typeIdx = typeIdx;
      saoLcuParamRdo[compIdx].mergeLeftFlag = 0;
      saoLcuParamRdo[compIdx].mergeUpFlag   = 0;
#if SAO_TYPE_CODING
      saoLcuParamRdo[compIdx].subTypeIdx = (typeIdx == SAO_BO) ? bestClassTableBo[compIdx] : 0;
#else
      saoLcuParamRdo[compIdx].bandPosition = (typeIdx == SAO_BO) ? bestClassTableBo[compIdx] : 0;
#endif
      for (classIdx = 0; classIdx < saoLcuParamRdo[compIdx].length; classIdx++)
      {
#if SAO_TYPE_CODING
        saoLcuParamRdo[compIdx].offset[classIdx] = (Int)m_iOffset[compIdx+1][typeIdx][classIdx+saoLcuParamRdo[compIdx].subTypeIdx+1];
#else
        saoLcuParamRdo[compIdx].offset[classIdx] = (Int)m_iOffset[compIdx+1][typeIdx][classIdx+saoLcuParamRdo[compIdx].bandPosition+1];
#endif
      }

      m_pcEntropyCoder->encodeSaoOffset(&saoLcuParamRdo[compIdx], compIdx+1);
    }
    estRate = m_pcEntropyCoder->getNumberOfWrittenBits();
    m_dCost[1][typeIdx] = (Double)((Double)(estDist[0] + estDist[1])  + lambda * (Double) estRate);
    
    if(m_dCost[1][typeIdx] < costPartBest)
    {
      costPartBest = m_dCost[1][typeIdx];
      copySaoUnit(saoLcuParam[0], &saoLcuParamRdo[0] );
      copySaoUnit(saoLcuParam[1], &saoLcuParamRdo[1] );
      bestDist = (estDist[0]+estDist[1]);       
    }
  }

  distortion[0] += ((Double)bestDist/lambda);
  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_TEMP_BEST]);
  m_pcEntropyCoder->encodeSaoOffset(saoLcuParam[0], 1);
  m_pcEntropyCoder->encodeSaoOffset(saoLcuParam[1], 2);
  m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[0][CI_TEMP_BEST] );

  // merge left or merge up

  for (Int idxNeighbor=0;idxNeighbor<2;idxNeighbor++) 
  {
    for(Int compIdx = 0; compIdx < 2; compIdx++)
    {
      saoLcuParamNeighbor[compIdx] = NULL;
      if (allowMergeLeft && addrLeft>=0 && idxNeighbor ==0)
      {
        saoLcuParamNeighbor[compIdx] = &(saoParam->saoLcuParam[compIdx+1][addrLeft]);
      }
      else if (allowMergeUp && addrUp>=0 && idxNeighbor ==1)
      {
        saoLcuParamNeighbor[compIdx] = &(saoParam->saoLcuParam[compIdx+1][addrUp]);
      }
      if (saoLcuParamNeighbor[compIdx]!=NULL)
      {
        estDist[compIdx] = 0;
        typeIdx = saoLcuParamNeighbor[compIdx]->typeIdx;
        if (typeIdx>=0) 
        {
#if SAO_TYPE_CODING
          Int mergeBandPosition = (typeIdx == SAO_BO)?saoLcuParamNeighbor[compIdx]->subTypeIdx:0;
#else
          Int mergeBandPosition = (typeIdx == SAO_BO)?saoLcuParamNeighbor[compIdx]->bandPosition:0;
#endif
          Int   merge_iOffset;
          for(classIdx = 0; classIdx < m_iNumClass[typeIdx]; classIdx++)
          {
            merge_iOffset = saoLcuParamNeighbor[compIdx]->offset[classIdx];
            estDist[compIdx]   += estSaoDist(m_iCount [compIdx+1][typeIdx][classIdx+mergeBandPosition+1], merge_iOffset, m_iOffsetOrg[compIdx+1][typeIdx][classIdx+mergeBandPosition+1],  shift);
          }
        }
        else
        {
          estDist[compIdx] = 0;
        }

        copySaoUnit(saoMergeParam[compIdx][idxNeighbor], saoLcuParamNeighbor[compIdx] );
        saoMergeParam[compIdx][idxNeighbor]->mergeUpFlag   = idxNeighbor;
        saoMergeParam[compIdx][idxNeighbor]->mergeLeftFlag = !idxNeighbor;
        distortion[idxNeighbor+1] += ((Double)estDist[compIdx]/lambda);
      }
    } 
  } 
}
#endif
#else
Void TEncSampleAdaptiveOffset::rdoSaoUnit(Int rx, Int ry, SAOParam *saoParam, Int addr, Int addrUp, Int addrLeft, Int yCbCr, Double lambda)
{
  Int typeIdx;

  Int64 estDist;
  Int classIdx;
  Int shift = g_uiBitIncrement << 1;
  //   Double dAreaWeight =  0;
  SaoLcuParam*  saoLcuParam = NULL;   
  SaoLcuParam*  saoLcuParamNeighbor = NULL; 
  Int   merge_iOffset [33];
  Double merge_dCost;
  saoLcuParam = &(saoParam->saoLcuParam[yCbCr][addr]);
  saoLcuParam->mergeUpFlag   = 0;
  saoLcuParam->mergeLeftFlag = 0;

  m_iTypePartBest[yCbCr] = -1;
  m_dCostPartBest[yCbCr] = MAX_DOUBLE;

  Double  bestRDCostTableBo = MAX_DOUBLE;
  Int     bestClassTableBo    = 0;
  Int     currentDistortionTableBo[MAX_NUM_SAO_CLASS];
  Double  currentRdCostTableBo[MAX_NUM_SAO_CLASS];
  Int     bestClassTableBoMerge = 0;

  SaoLcuParam   saoLcuParamRdo;   
  Double   estRate = 0;

  resetSaoUnit(saoLcuParam);
  resetSaoUnit(&saoLcuParamRdo);

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
  m_pcRDGoOnSbacCoder->resetBits();
  Int allowMergeLeft = 1;
  Int allowMergeUp   = 1;
  if (rx!=0)
  { 
    // check tile id and slice id 
    if ( (m_pcPic->getPicSym()->getTileIdxMap(addr-1) != m_pcPic->getPicSym()->getTileIdxMap(addr)) || (m_pcPic->getCU(addr-1)->getSlice()->getSliceIdx() != m_pcPic->getCU(addr)->getSlice()->getSliceIdx()))
    {
      allowMergeLeft = 0;
    }
  }
  if (ry!=0)
  {
    if ( (m_pcPic->getPicSym()->getTileIdxMap(addr-m_iNumCuInWidth) != m_pcPic->getPicSym()->getTileIdxMap(addr)) || (m_pcPic->getCU(addr-m_iNumCuInWidth)->getSlice()->getSliceIdx() != m_pcPic->getCU(addr)->getSlice()->getSliceIdx()))
    {
      allowMergeUp = 0;
    }
  }
  m_pcEntropyCoder->encodeSaoUnitInterleaving(yCbCr, 1, rx, ry,  &saoLcuParamRdo, 1,  1,  allowMergeLeft, allowMergeUp);
  m_dCostPartBest[yCbCr] = m_pcEntropyCoder->getNumberOfWrittenBits()*lambda ; 
  m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[0][CI_TEMP_BEST] );

  for (typeIdx=0; typeIdx<MAX_NUM_SAO_TYPE; typeIdx++)
  {
    estDist = estSaoTypeDist(yCbCr, typeIdx, shift, lambda, currentDistortionTableBo, currentRdCostTableBo);

    if( typeIdx == SAO_BO )
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
      estDist = 0;
      for(classIdx = bestClassTableBo; classIdx < bestClassTableBo+SAO_BO_LEN; classIdx++)
      {
        estDist += currentDistortionTableBo[classIdx];
      }
    }
    resetSaoUnit(&saoLcuParamRdo);
    saoLcuParamRdo.length = m_iNumClass[typeIdx];
    saoLcuParamRdo.typeIdx = typeIdx;
    saoLcuParamRdo.mergeLeftFlag = 0;
    saoLcuParamRdo.mergeUpFlag   = 0;
    saoLcuParamRdo.bandPosition = (typeIdx == SAO_BO) ? bestClassTableBo : 0;
    for (classIdx = 0; classIdx < saoLcuParamRdo.length; classIdx++)
    {
      saoLcuParamRdo.offset[classIdx] = (Int)m_iOffset[yCbCr][typeIdx][classIdx+saoLcuParamRdo.bandPosition+1];
    }
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
    m_pcRDGoOnSbacCoder->resetBits();
    m_pcEntropyCoder->encodeSaoUnitInterleaving(yCbCr, 1, rx, ry,  &saoLcuParamRdo, 1,  1,  allowMergeLeft, allowMergeUp);
    estRate = m_pcEntropyCoder->getNumberOfWrittenBits();
    m_dCost[yCbCr][typeIdx] = (Double)((Double)estDist + lambda * (Double) estRate);

    if(m_dCost[yCbCr][typeIdx] < m_dCostPartBest[yCbCr])
    {
      m_dCostPartBest[yCbCr] = m_dCost[yCbCr][typeIdx];
      m_iTypePartBest[yCbCr] = typeIdx;
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[0][CI_TEMP_BEST] );
    }
  }

  // merge left or merge up

  for (Int idxNeighbor=0;idxNeighbor<2;idxNeighbor++) 
  {
    saoLcuParamNeighbor = NULL;
    if (addrLeft>=0 && idxNeighbor ==0)
    {
      saoLcuParamNeighbor = &(saoParam->saoLcuParam[yCbCr][addrLeft]);
    }
    else if (addrUp>=0 && idxNeighbor ==1)
    {
      saoLcuParamNeighbor = &(saoParam->saoLcuParam[yCbCr][addrUp]);
    }
    if (saoLcuParamNeighbor!=NULL)
    {
        Int mergeBandPosition = 0;
        estDist = 0;
        typeIdx = saoLcuParamNeighbor->typeIdx;
        if (saoLcuParamNeighbor->typeIdx>=0) //new
        {
          mergeBandPosition = (saoLcuParamNeighbor->typeIdx == SAO_BO)?saoLcuParamNeighbor->bandPosition:0;
          for(classIdx = mergeBandPosition+1; classIdx < mergeBandPosition+m_iNumClass[typeIdx]+1; classIdx++)
          {
            merge_iOffset[classIdx] = saoLcuParamNeighbor->offset[classIdx-1-saoLcuParamNeighbor->bandPosition];
            estDist   += estSaoDist(m_iCount [yCbCr][typeIdx][classIdx], merge_iOffset[classIdx], m_iOffsetOrg[yCbCr][typeIdx][classIdx],  shift);
          }
        }
        else
        {
          saoLcuParamRdo.typeIdx = -1;
          saoLcuParamRdo.length  = 0;
          estDist = 0;
        }
        m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
        m_pcRDGoOnSbacCoder->resetBits();
        saoLcuParamRdo.mergeUpFlag   = idxNeighbor;
        saoLcuParamRdo.mergeLeftFlag = !idxNeighbor;
        m_pcEntropyCoder->encodeSaoUnitInterleaving(yCbCr, 1, rx, ry,  &saoLcuParamRdo, 1,  1,  allowMergeLeft, allowMergeUp);
        estRate = m_pcEntropyCoder->getNumberOfWrittenBits();

        merge_dCost = (Double)((Double)estDist + m_dLambdaLuma * estRate) ;

        if(merge_dCost < m_dCostPartBest[yCbCr])
        {
          m_dCostPartBest[yCbCr] = merge_dCost;
          m_iTypePartBest[yCbCr] = typeIdx;
          if (typeIdx>=0)
          {
            bestClassTableBoMerge   = mergeBandPosition;
            for(classIdx = mergeBandPosition+1; classIdx < mergeBandPosition+m_iNumClass[typeIdx]+1; classIdx++)
            {
              m_iOffset[yCbCr][typeIdx][classIdx] = merge_iOffset[classIdx];
            }
          }
          saoLcuParam->mergeUpFlag   = idxNeighbor;
          saoLcuParam->mergeLeftFlag = !idxNeighbor;
          m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[0][CI_TEMP_BEST] );
        }
    }
  } 

  saoLcuParam->typeIdx  = m_iTypePartBest[yCbCr];
  if (saoLcuParam->typeIdx != -1)
  {
    saoLcuParam->length = m_iNumClass[saoLcuParam->typeIdx];
    Int minIndex = 0;
    if( saoLcuParam->typeIdx == SAO_BO )
    {
      if ((saoLcuParam->mergeUpFlag )||(saoLcuParam->mergeLeftFlag)) 
      {
        saoLcuParam->bandPosition = bestClassTableBoMerge;
      }
      else
      {
        saoLcuParam->bandPosition = bestClassTableBo;
      }
      minIndex = saoLcuParam->bandPosition;
    }
    for (Int i=0; i< saoLcuParam->length ; i++)
    {
      saoLcuParam->offset[i] = (Int) m_iOffset[yCbCr][saoLcuParam->typeIdx][minIndex+i+1];
    }
  }
  else
  {
    saoLcuParam->length = 0;
  }

  m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[0][CI_TEMP_BEST] );
  m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[0][CI_CURR_BEST] );
}
#endif

//! \}
