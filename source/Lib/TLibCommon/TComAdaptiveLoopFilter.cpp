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

/** \file     TComAdaptiveLoopFilter.cpp
    \brief    adaptive loop filter class
*/

#include "TComAdaptiveLoopFilter.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Tables
// ====================================================================================================================

//Shape0: Star5x5
Int TComAdaptiveLoopFilter::weightsShape0Sym[10] = 
{
  2,    2,    2,    
  2, 2, 2,        
  2, 2, 1, 1
};


#if G212_CROSS9x9_VB
//Shape1: Cross9x9
Int TComAdaptiveLoopFilter::weightsShape1Sym[10] = 
{ 
              2,
              2,
              2,
              2,
  2, 2, 2, 2, 1, 
              1
};

#else
//Shape1: Cross11x5
Int TComAdaptiveLoopFilter::weightsShape1Sym[9] = 
{                      
  2,
  2,
  2, 2, 2, 2, 2, 1, 1
};
#endif


Int* TComAdaptiveLoopFilter::weightsTabShapes[NUM_ALF_FILTER_SHAPE] =
{
  weightsShape0Sym, weightsShape1Sym
};

Int TComAdaptiveLoopFilter::m_sqrFiltLengthTab[NUM_ALF_FILTER_SHAPE] =
{
#if G212_CROSS9x9_VB

#if ALF_DC_OFFSET_REMOVAL
  9, 9
#else
  10, 10
#endif


#else
#if ALF_DC_OFFSET_REMOVAL
   9, 8
#else
  10, 9
#endif
#endif
};

// Shape0
Int depthIntShape0Sym[10] = 
{
  1,    3,    1,
  3, 4, 3, 
  3, 4, 5, 5                 
};
// Shape1
#if G212_CROSS9x9_VB
Int depthIntShape1Sym[10] = 
{
              5,
              6,
              7,
              8,
  5, 6, 7, 8, 9, 
              9  
};

#else
Int depthIntShape1Sym[9] = 
{
  9,
  10,
  6, 7, 8, 9,10,11,11                        
};
#endif



Int* pDepthIntTabShapes[NUM_ALF_FILTER_SHAPE] =
{ 
  depthIntShape0Sym, depthIntShape1Sym
};


// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

const AlfCUCtrlInfo& AlfCUCtrlInfo::operator= (const AlfCUCtrlInfo& src)
{
  this->cu_control_flag = src.cu_control_flag;
  this->alf_max_depth   = src.alf_max_depth;
  this->num_alf_cu_flag = src.num_alf_cu_flag;
  this->alf_cu_flag     = src.alf_cu_flag;
  return *this;
}

TComAdaptiveLoopFilter::TComAdaptiveLoopFilter()
{
  m_pcTempPicYuv = NULL;
#if !NONCROSS_TILE_IN_LOOP_FILTERING
  m_pSlice       = NULL;
#endif
  m_iSGDepth     = 0;
#if NONCROSS_TILE_IN_LOOP_FILTERING
  m_pcPic        = NULL;
  m_ppSliceAlfLCUs = NULL;
  m_pvpAlfLCU    = NULL;
  m_pvpSliceTileAlfLCU = NULL;
#else
  m_piSliceSUMap = NULL;
#endif
}

Void TComAdaptiveLoopFilter:: xError(const char *text, int code)
{
  fprintf(stderr, "%s\n", text);
  exit(code);
}

Void TComAdaptiveLoopFilter:: no_mem_exit(const char *where)
{
  char errortext[200];
  sprintf(errortext, "Could not allocate memory: %s",where);
  xError (errortext, 100);
}

Void TComAdaptiveLoopFilter::initMatrix_Pel(Pel ***m2D, int d1, int d2)
{
  int i;
  
  if(!(*m2D = (Pel **) calloc(d1, sizeof(Pel *))))
  {
    FATAL_ERROR_0("initMatrix_Pel: memory allocation problem\n", -1);
  }
  if(!((*m2D)[0] = (Pel *) calloc(d1 * d2, sizeof(Pel))))
  {
    FATAL_ERROR_0("initMatrix_Pel: memory allocation problem\n", -1);
  }
  
  for(i = 1; i < d1; i++)
  {
    (*m2D)[i] = (*m2D)[i-1] + d2;
  }
}

Void TComAdaptiveLoopFilter::initMatrix_int(int ***m2D, int d1, int d2)
{
  int i;
  
  if(!(*m2D = (int **) calloc(d1, sizeof(int *))))
  {
    FATAL_ERROR_0("initMatrix_int: memory allocation problem\n", -1);
  }
  if(!((*m2D)[0] = (int *) calloc(d1 * d2, sizeof(int))))
  {
    FATAL_ERROR_0("initMatrix_int: memory allocation problem\n", -1);
  }
  
  for(i = 1; i < d1; i++)
  {
    (*m2D)[i] = (*m2D)[i-1] + d2;
  }
}

Void TComAdaptiveLoopFilter::destroyMatrix_int(int **m2D)
{
  if(m2D)
  {
    if(m2D[0])
    {
      free(m2D[0]);
    }
    else
    {
      FATAL_ERROR_0("destroyMatrix_int: memory free problem\n", -1);
    }
    free(m2D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix_int: memory free problem\n", -1);
  }
}

Void TComAdaptiveLoopFilter::destroyMatrix_Pel(Pel **m2D)
{
  if(m2D)
  {
    if(m2D[0])
    {
      free(m2D[0]);
    }
    else
    {
      FATAL_ERROR_0("destroyMatrix_Pel: memory free problem\n", -1);
    }
    free(m2D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix_Pel: memory free problem\n", -1);
  }
}

Void TComAdaptiveLoopFilter::initMatrix_double(double ***m2D, int d1, int d2)
{
  int i;
  
  if(!(*m2D = (double **) calloc(d1, sizeof(double *))))
  {
    FATAL_ERROR_0("initMatrix_double: memory allocation problem\n", -1);
  }
  if(!((*m2D)[0] = (double *) calloc(d1 * d2, sizeof(double))))
  {
    FATAL_ERROR_0("initMatrix_double: memory allocation problem\n", -1);
  }
  
  for(i = 1; i < d1; i++)
  {
    (*m2D)[i] = (*m2D)[i-1] + d2;
  }
}

Void TComAdaptiveLoopFilter::initMatrix3D_double(double ****m3D, int d1, int d2, int d3)
{
  int  j;
  
  if(!((*m3D) = (double ***) calloc(d1, sizeof(double **))))
  {
    FATAL_ERROR_0("initMatrix3D_double: memory allocation problem\n", -1);
  }
  
  for(j = 0; j < d1; j++)
  {
    initMatrix_double((*m3D) + j, d2, d3);
  }
}


Void TComAdaptiveLoopFilter::initMatrix4D_double(double *****m4D, int d1, int d2, int d3, int d4)
{
  int  j;
  
  if(!((*m4D) = (double ****) calloc(d1, sizeof(double ***))))
  {
    FATAL_ERROR_0("initMatrix4D_double: memory allocation problem\n", -1);
  }
  
  for(j = 0; j < d1; j++)
  {
    initMatrix3D_double((*m4D) + j, d2, d3, d4);
  }
}


Void TComAdaptiveLoopFilter::destroyMatrix_double(double **m2D)
{
  if(m2D)
  {
    if(m2D[0])
    {
      free(m2D[0]);
    }
    else
    {
      FATAL_ERROR_0("destroyMatrix_double: memory free problem\n", -1);
    }
    free(m2D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix_double: memory free problem\n", -1);
  }
}

Void TComAdaptiveLoopFilter::destroyMatrix3D_double(double ***m3D, int d1)
{
  int i;
  
  if(m3D)
  {
    for(i = 0; i < d1; i++)
    {
      destroyMatrix_double(m3D[i]);
    }
    free(m3D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix3D_double: memory free problem\n", -1);
  }
}


Void TComAdaptiveLoopFilter::destroyMatrix4D_double(double ****m4D, int d1, int d2)
{
  int  j;
  
  if(m4D)
  {
    for(j = 0; j < d1; j++)
    {
      destroyMatrix3D_double(m4D[j], d2);
    }
    free(m4D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix4D_double: memory free problem\n", -1);
  }
}

Void TComAdaptiveLoopFilter::create( Int iPicWidth, Int iPicHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxCUDepth )
{
  if ( !m_pcTempPicYuv )
  {
    m_pcTempPicYuv = new TComPicYuv;
    m_pcTempPicYuv->create( iPicWidth, iPicHeight, uiMaxCUWidth, uiMaxCUHeight, uiMaxCUDepth );
  }
  m_img_height = iPicHeight;
  m_img_width = iPicWidth;
  for(Int i=0; i< NUM_ALF_CLASS_METHOD; i++)
  {
    initMatrix_Pel(&(m_varImgMethods[i]), m_img_height, m_img_width);
  }
  initMatrix_int(&m_filterCoeffSym, NO_VAR_BINS, ALF_MAX_NUM_COEF);
  UInt uiNumLCUsInWidth   = m_img_width  / uiMaxCUWidth;
  UInt uiNumLCUsInHeight  = m_img_height / uiMaxCUHeight;

  uiNumLCUsInWidth  += ( m_img_width % uiMaxCUWidth ) ? 1 : 0;
  uiNumLCUsInHeight += ( m_img_height % uiMaxCUHeight ) ? 1 : 0;

  m_uiNumCUsInFrame = uiNumLCUsInWidth* uiNumLCUsInHeight; 

  createRegionIndexMap(m_varImgMethods[ALF_RA], m_img_width, m_img_height);
}

Void TComAdaptiveLoopFilter::destroy()
{
  if ( m_pcTempPicYuv )
  {
    m_pcTempPicYuv->destroy();
    delete m_pcTempPicYuv;
  }
  for(Int i=0; i< NUM_ALF_CLASS_METHOD; i++)
  {
    destroyMatrix_Pel(m_varImgMethods[i]);
  }
  destroyMatrix_int(m_filterCoeffSym);
}

// --------------------------------------------------------------------------------------------------------------------
// allocate / free / copy functions
// --------------------------------------------------------------------------------------------------------------------

Void TComAdaptiveLoopFilter::allocALFParam(ALFParam* pAlfParam)
{
  pAlfParam->alf_flag = 0;
  pAlfParam->coeff_chroma = new Int[ALF_MAX_NUM_COEF];
  ::memset(pAlfParam->coeff_chroma, 0, sizeof(Int)*ALF_MAX_NUM_COEF );
  pAlfParam->coeffmulti = new Int*[NO_VAR_BINS];
  for (int i=0; i<NO_VAR_BINS; i++)
  {
    pAlfParam->coeffmulti[i] = new Int[ALF_MAX_NUM_COEF];
    ::memset(pAlfParam->coeffmulti[i],        0, sizeof(Int)*ALF_MAX_NUM_COEF );
  }
#if G665_ALF_COEFF_PRED
  pAlfParam->nbSPred = new Int[NO_VAR_BINS];
  ::memset(pAlfParam->nbSPred, 0, sizeof(Int)*NO_VAR_BINS);
#endif
  pAlfParam->filterPattern = new Int[NO_VAR_BINS];
  ::memset(pAlfParam->filterPattern, 0, sizeof(Int)*NO_VAR_BINS);
  pAlfParam->alf_pcr_region_flag = 0;
}

Void TComAdaptiveLoopFilter::freeALFParam(ALFParam* pAlfParam)
{
  assert(pAlfParam != NULL);
  if (pAlfParam->coeff_chroma != NULL)
  {
    delete[] pAlfParam->coeff_chroma;
    pAlfParam->coeff_chroma = NULL;
  }
  for (int i=0; i<NO_VAR_BINS; i++)
  {
    delete[] pAlfParam->coeffmulti[i];
    pAlfParam->coeffmulti[i] = NULL;
  }
  delete[] pAlfParam->coeffmulti;
  pAlfParam->coeffmulti = NULL;

#if G665_ALF_COEFF_PRED
  delete[] pAlfParam->nbSPred;
  pAlfParam->nbSPred = NULL;
#endif

  delete[] pAlfParam->filterPattern;
  pAlfParam->filterPattern = NULL;
}


Void TComAdaptiveLoopFilter::copyALFParam(ALFParam* pDesAlfParam, ALFParam* pSrcAlfParam)
{
  pDesAlfParam->alf_flag = pSrcAlfParam->alf_flag;
  pDesAlfParam->chroma_idc = pSrcAlfParam->chroma_idc;
  pDesAlfParam->num_coeff = pSrcAlfParam->num_coeff;
  pDesAlfParam->filter_shape_chroma = pSrcAlfParam->filter_shape_chroma;
  pDesAlfParam->num_coeff_chroma = pSrcAlfParam->num_coeff_chroma;
  pDesAlfParam->alf_pcr_region_flag = pSrcAlfParam->alf_pcr_region_flag;
  ::memcpy(pDesAlfParam->coeff_chroma, pSrcAlfParam->coeff_chroma, sizeof(Int)*ALF_MAX_NUM_COEF);
  pDesAlfParam->filter_shape = pSrcAlfParam->filter_shape;
  ::memcpy(pDesAlfParam->filterPattern, pSrcAlfParam->filterPattern, sizeof(Int)*NO_VAR_BINS);
  pDesAlfParam->startSecondFilter = pSrcAlfParam->startSecondFilter;

  //Coeff send related
  pDesAlfParam->filters_per_group = pSrcAlfParam->filters_per_group; //this can be updated using codedVarBins
  pDesAlfParam->predMethod = pSrcAlfParam->predMethod;
#if G665_ALF_COEFF_PRED
  ::memcpy(pDesAlfParam->nbSPred, pSrcAlfParam->nbSPred, sizeof(Int)*NO_VAR_BINS);
#endif
  for (int i=0; i<NO_VAR_BINS; i++)
  {
    ::memcpy(pDesAlfParam->coeffmulti[i], pSrcAlfParam->coeffmulti[i], sizeof(Int)*ALF_MAX_NUM_COEF);
  }
}

// --------------------------------------------------------------------------------------------------------------------
// prediction of filter coefficients
// --------------------------------------------------------------------------------------------------------------------

Void TComAdaptiveLoopFilter::predictALFCoeffChroma( ALFParam* pAlfParam )
{
  Int i, sum, pred, N;
  const Int* pFiltMag = NULL;
  pFiltMag = weightsTabShapes[pAlfParam->filter_shape_chroma];
#if ALF_DC_OFFSET_REMOVAL
  N = pAlfParam->num_coeff_chroma;
#else
  N = pAlfParam->num_coeff_chroma - 1;
#endif
  sum=0;
  for(i=0; i<N;i++)
  {
    sum+=pFiltMag[i]*pAlfParam->coeff_chroma[i];
  }
  pred=(1<<ALF_NUM_BIT_SHIFT)-(sum-pAlfParam->coeff_chroma[N-1]);
  pAlfParam->coeff_chroma[N-1]=pred-pAlfParam->coeff_chroma[N-1];
}

// --------------------------------------------------------------------------------------------------------------------
// interface function for actual ALF process
// --------------------------------------------------------------------------------------------------------------------

/**
 \param [in, out] pcPic         picture (TComPic) class (input/output)
 \param [in] pcAlfParam    ALF parameter
 \param [in,out] vAlfCUCtrlParam ALF CU control parameters
 \todo   for temporal buffer, it uses residual picture buffer, which may not be safe. Make it be safer.
 */
Void TComAdaptiveLoopFilter::ALFProcess(TComPic* pcPic, ALFParam* pcAlfParam, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam)
{
  assert(m_uiNumSlicesInPic == vAlfCUCtrlParam.size());
  if(!pcAlfParam->alf_flag)
  {
    return;
  }

#if G212_CROSS9x9_VB
  m_lcuHeight     = pcPic->getSlice(0)->getSPS()->getMaxCUHeight();
  m_lineIdxPadBot = m_lcuHeight - 4 - 4;             // DFRegion, Vertical Taps
  m_lineIdxPadTop = m_lcuHeight - 4;                 // DFRegion

  m_lcuHeightChroma     = m_lcuHeight>>1;
  m_lineIdxPadBotChroma = m_lcuHeightChroma - 2 - 4; // DFRegion, Vertical Taps
  m_lineIdxPadTopChroma = m_lcuHeightChroma - 2 ;    // DFRegion
#endif 

  TComPicYuv* pcPicYuvRec    = pcPic->getPicYuvRec();
  TComPicYuv* pcPicYuvExtRec = m_pcTempPicYuv;
#if !NONCROSS_TILE_IN_LOOP_FILTERING
  if(!m_bUseNonCrossALF)
  {
#endif
    pcPicYuvRec   ->copyToPic          ( pcPicYuvExtRec );
    pcPicYuvExtRec->setBorderExtension ( false );
    pcPicYuvExtRec->extendPicBorder    ();
#if !NONCROSS_TILE_IN_LOOP_FILTERING
  }
#endif

  if(m_uiNumSlicesInPic == 1)
  {
    AlfCUCtrlInfo* pcAlfCtrlParam = &(vAlfCUCtrlParam[0]);
    if(pcAlfCtrlParam->cu_control_flag)
    {
      UInt idx = 0;
      for(UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame(); uiCUAddr++)
      {
        TComDataCU *pcCU = pcPic->getCU(uiCUAddr);
        setAlfCtrlFlags(pcAlfCtrlParam, pcCU, 0, 0, idx);
      }
    }
  }
  else
  {
    transferCtrlFlagsFromAlfParam(vAlfCUCtrlParam);
  }
  xALFLuma(pcPic, pcAlfParam, vAlfCUCtrlParam, pcPicYuvExtRec, pcPicYuvRec);
  if(pcAlfParam->chroma_idc)
  {
    predictALFCoeffChroma(pcAlfParam);

#if G214_ALF_CONSTRAINED_COEFF
    checkFilterCoeffValue(pcAlfParam->coeff_chroma, pcAlfParam->num_coeff_chroma, true );
#endif

    xALFChroma( pcAlfParam, pcPicYuvExtRec, pcPicYuvRec);
  }
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

#if G214_ALF_CONSTRAINED_COEFF
/** 
 \param filter         filter coefficient
 \param filterLength   filter length
 \param isChroma       1: chroma, 0: luma
 */
Void TComAdaptiveLoopFilter::checkFilterCoeffValue( Int *filter, Int filterLength, Bool isChroma )
{
  Int maxValueNonCenter = 1 * (1 << ALF_NUM_BIT_SHIFT) - 1;
  Int minValueNonCenter = 0 - 1 * (1 << ALF_NUM_BIT_SHIFT);

  Int maxValueCenter    = 2 * (1 << ALF_NUM_BIT_SHIFT) - 1;
  Int minValueCenter    = 0 ; 

#if !ALF_DC_OFFSET_REMOVAL
  Int pelDepth          = g_uiBitIncrement + g_uiBitDepth - (isChroma ? 2 : 0) ;
  Int maxValueOffset    = ( 1 << (pelDepth + ALF_NUM_BIT_SHIFT))  -1;  
  Int minValueOffset    = 0 - ( 1 << (pelDepth + ALF_NUM_BIT_SHIFT) ) ;
#endif

#if ALF_DC_OFFSET_REMOVAL
  for(Int i = 0; i < filterLength-1; i++)
#else
  for(Int i = 0; i < filterLength-2; i++)
#endif
  {
    filter[i] = Clip3(minValueNonCenter, maxValueNonCenter, filter[i]);
  }

#if ALF_DC_OFFSET_REMOVAL
  filter[filterLength-1] = Clip3(minValueCenter, maxValueCenter, filter[filterLength-1]);
#else
  filter[filterLength-2] = Clip3(minValueCenter, maxValueCenter, filter[filterLength-2]);
  filter[filterLength-1] = Clip3(minValueOffset, maxValueOffset, filter[filterLength-1]);
#endif
}
#endif


Void TComAdaptiveLoopFilter::xALFLuma(TComPic* pcPic, ALFParam* pcAlfParam, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam,TComPicYuv* pcPicDec, TComPicYuv* pcPicRest)
{
  Int    LumaStride = pcPicDec->getStride();
  Pel* pDec = pcPicDec->getLumaAddr();
  Pel* pRest = pcPicRest->getLumaAddr();

  decodeFilterSet(pcAlfParam, m_varIndTab, m_filterCoeffSym);

  m_uiVarGenMethod = pcAlfParam->alf_pcr_region_flag;
  m_varImg         = m_varImgMethods[m_uiVarGenMethod];
#if G609_NEW_BA_SUB
  calcVar(m_varImg, pRest, LumaStride, pcAlfParam->alf_pcr_region_flag);
#endif

  if(!m_bUseNonCrossALF)
  {
#if !G609_NEW_BA_SUB
    calcVar(0, 0, m_varImg, pDec, VAR_SIZE, m_img_height, m_img_width, LumaStride);
#endif
    Bool bCUCtrlEnabled = false;
    for(UInt s=0; s< m_uiNumSlicesInPic; s++)
    {
#if NONCROSS_TILE_IN_LOOP_FILTERING
      if(!pcPic->getValidSlice(s))
      {
        continue;
      }
#else
      if(m_uiNumSlicesInPic > 1)
      {
        if(!m_pSlice[s].isValidSlice()) 
        {
          continue;
        }
      }
#endif
      if( vAlfCUCtrlParam[s].cu_control_flag == 1)
      {
        bCUCtrlEnabled = true;
      }
    }

    if(bCUCtrlEnabled)  
    {
      xCUAdaptive(pcPic, pcAlfParam->filter_shape, pRest, pDec, LumaStride);
    }  
    else
    {
      filterLuma(pRest, pDec, LumaStride, 0, m_img_height-1, 0, m_img_width-1,  pcAlfParam->filter_shape, m_filterCoeffSym, m_varIndTab, m_varImg);
    }
  }
  else
  {
#if NONCROSS_TILE_IN_LOOP_FILTERING
    Pel* pTemp = m_pcSliceYuvTmp->getLumaAddr();
#endif
    for(UInt s=0; s< m_uiNumSlicesInPic; s++)
    {
#if NONCROSS_TILE_IN_LOOP_FILTERING
      if(!pcPic->getValidSlice(s)) 
      {
        continue;
      }
      std::vector< std::vector<AlfLCUInfo*> > & vpSliceTileAlfLCU = m_pvpSliceTileAlfLCU[s];

      for(Int t=0; t< (Int)vpSliceTileAlfLCU.size(); t++)
      {
        std::vector<AlfLCUInfo*> & vpAlfLCU = vpSliceTileAlfLCU[t];

        copyRegion(vpAlfLCU, pTemp, pDec, LumaStride);
        extendRegionBorder(vpAlfLCU, pTemp, LumaStride);
#if !G609_NEW_BA_SUB
        if(m_uiVarGenMethod != ALF_RA)
        {
          calcVarforOneSlice(vpAlfLCU, m_varImg, pDec, VAR_SIZE, LumaStride);
        }
#endif
        if(vAlfCUCtrlParam[s].cu_control_flag == 1)
        {
          xCUAdaptiveRegion(vpAlfLCU, pTemp, pRest, LumaStride, pcAlfParam->filter_shape, m_filterCoeffSym, m_varIndTab, m_varImg);
        }
        else
        {
          filterLumaRegion(vpAlfLCU, pTemp, pRest, LumaStride, pcAlfParam->filter_shape, m_filterCoeffSym, m_varIndTab, m_varImg);
        }
      }
#else
      CAlfSlice* pSlice = &(m_pSlice[s]);

      if(!pSlice->isValidSlice()) 
      {
        continue;
      }

      pSlice->copySliceLuma((Pel*)pDec, (Pel*)pRest, LumaStride);
      pSlice->extendSliceBorderLuma((Pel*)pDec, LumaStride);
#if !G609_NEW_BA_SUB
      if(m_uiVarGenMethod != ALF_RA)
      {
        calcVarforOneSlice(pSlice, m_varImg, pDec, VAR_SIZE, LumaStride);
      }
#endif
      xFilterOneSlice(pSlice, pDec, pRest, LumaStride, pcAlfParam);
#endif
    }
  }
}

Void TComAdaptiveLoopFilter::decodeFilterSet(ALFParam* pcAlfParam, Int* varIndTab, Int** filterCoeff)
{
  // reconstruct merge table
  memset(m_varIndTab, 0, NO_VAR_BINS * sizeof(Int));
  if(pcAlfParam->filters_per_group > 1)
  {
    for(Int i = 1; i < NO_VAR_BINS; ++i)
    {
      if(pcAlfParam->filterPattern[i])
      {
        varIndTab[i] = varIndTab[i-1] + 1;
      }
      else
      {
        varIndTab[i] = varIndTab[i-1];
      }
    }
  }
#if G665_ALF_COEFF_PRED
  predictALFCoeffLuma( pcAlfParam);
#endif
  // reconstruct filter sets
  reconstructFilterCoeffs( pcAlfParam, filterCoeff);

}


Void TComAdaptiveLoopFilter::filterLuma(Pel *pImgRes, Pel *pImgPad, Int stride, 
  Int ypos, Int yposEnd, Int xpos, Int xposEnd, 
  Int filtNo, Int** filterSet, Int* mergeTable, Pel** ppVarImg)
{
  static Int numBitsMinus1= (Int)ALF_NUM_BIT_SHIFT;
  static Int offset       = (1<<( (Int)ALF_NUM_BIT_SHIFT-1));
  static Int shiftHeight  = (Int)(log((double)VAR_SIZE_H)/log(2.0));
  static Int shiftWidth   = (Int)(log((double)VAR_SIZE_W)/log(2.0));

  Pel *pImgPad1,*pImgPad2,*pImgPad3,*pImgPad4;
  Pel *pVar;
  Int i, j, pixelInt;
  Int *coef = NULL;

  pImgPad    += (ypos*stride);
  pImgRes    += (ypos*stride);

#if G212_CROSS9x9_VB
  Int yLineInLCU;
  Int paddingLine;
  Int varInd = 0;
  Int newCenterCoeff[4][NO_VAR_BINS];

  for(i=0; i< 4; i++)
  {
    ::memset(&(newCenterCoeff[i][0]), 0, sizeof(Int)*NO_VAR_BINS);
  }

  if(filtNo == ALF_CROSS9x9)
  {
    for (i=0; i<NO_VAR_BINS; i++)
    {
      coef = filterSet[i];
      //VB line 1
      newCenterCoeff[0][i] = coef[8] + ((coef[0] + coef[1] + coef[2] + coef[3])<<1);
      //VB line 2 
      newCenterCoeff[1][i] = coef[8] + ((coef[0] + coef[1] + coef[2])<<1);
      //VB line 3 
      newCenterCoeff[2][i] = coef[8] + ((coef[0] + coef[1])<<1);
      //VB line 4 
      newCenterCoeff[3][i] = coef[8] + ((coef[0])<<1);
    }
  }
#endif


  switch(filtNo)
  {
  case ALF_STAR5x5:
    {
      for(i= ypos; i<= yposEnd; i++)
      {

#if G212_CROSS9x9_VB
        yLineInLCU = i % m_lcuHeight;   

        if (yLineInLCU<m_lineIdxPadBot || i-yLineInLCU+m_lcuHeight >= m_img_height)
        {
          pImgPad1 = pImgPad +   stride;
          pImgPad2 = pImgPad -   stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
        }
        else if (yLineInLCU<m_lineIdxPadTop)
        {
          paddingLine = - yLineInLCU + m_lineIdxPadTop - 1;
          pImgPad1 = pImgPad + min(paddingLine, 1)*stride;
          pImgPad2 = pImgPad -   stride;
          pImgPad3 = pImgPad + min(paddingLine, 2)*stride;
          pImgPad4 = pImgPad - 2*stride;
        }
        else
        {
          paddingLine = yLineInLCU - m_lineIdxPadTop ;
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - min(paddingLine, 1)*stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - min(paddingLine, 2)*stride;
        } 
#else 
        pImgPad1 = pImgPad +   stride;
        pImgPad2 = pImgPad -   stride;
        pImgPad3 = pImgPad + 2*stride;
        pImgPad4 = pImgPad - 2*stride;
#endif

        pVar = ppVarImg[i>>shiftHeight] + (xpos>>shiftWidth);

#if G212_CROSS9x9_VB
        if ( (yLineInLCU == m_lineIdxPadTop || yLineInLCU == m_lineIdxPadTop-1) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd; j++)
          {
            pImgRes[j] = pImgPad[j];
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTop+1 || yLineInLCU == m_lineIdxPadTop-2) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              coef = filterSet[mergeTable[*(pVar++)]];
            }

#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = coef[9];
#endif

            pixelInt += coef[0]* (pImgPad3[j+2]+pImgPad4[j-2]);
            pixelInt += coef[1]* (pImgPad3[j  ]+pImgPad4[j  ]);
            pixelInt += coef[2]* (pImgPad3[j-2]+pImgPad4[j+2]);

            pixelInt += coef[3]* (pImgPad1[j+1]+pImgPad2[j-1]);
            pixelInt += coef[4]* (pImgPad1[j  ]+pImgPad2[j  ]);
            pixelInt += coef[5]* (pImgPad1[j-1]+pImgPad2[j+1]);

            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[8]* (pImgPad[j  ]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = ( Clip( pixelInt ) + pImgPad[j] ) >> 1;
          }
        }
        else
        {
#endif

        for(j= xpos; j<= xposEnd ; j++)
        {
          if (j % VAR_SIZE_W==0) 
          {
            coef = filterSet[mergeTable[*(pVar++)]];
          }

#if ALF_DC_OFFSET_REMOVAL
          pixelInt  = 0;
#else
          pixelInt  = coef[9];
#endif

          pixelInt += coef[0]* (pImgPad3[j+2]+pImgPad4[j-2]);
          pixelInt += coef[1]* (pImgPad3[j  ]+pImgPad4[j  ]);
          pixelInt += coef[2]* (pImgPad3[j-2]+pImgPad4[j+2]);

          pixelInt += coef[3]* (pImgPad1[j+1]+pImgPad2[j-1]);
          pixelInt += coef[4]* (pImgPad1[j  ]+pImgPad2[j  ]);
          pixelInt += coef[5]* (pImgPad1[j-1]+pImgPad2[j+1]);

          pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
          pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
          pixelInt += coef[8]* (pImgPad[j  ]);

          pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);
          pImgRes[j] = Clip( pixelInt );
        }

#if G212_CROSS9x9_VB
        }
#endif

        pImgPad += stride;
        pImgRes += stride;
      }
    }
    break;
#if G212_CROSS9x9_VB
  case ALF_CROSS9x9:
    {
      Pel *pImgPad5, *pImgPad6, *pImgPad7, *pImgPad8;

      for(i= ypos; i<= yposEnd; i++)
      {
        yLineInLCU = i % m_lcuHeight;   

        if (yLineInLCU<m_lineIdxPadBot || i-yLineInLCU+m_lcuHeight >= m_img_height)
        {
          pImgPad1 = pImgPad +   stride;
          pImgPad2 = pImgPad -   stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - 3*stride;
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - 4*stride;
        }
        else if (yLineInLCU<m_lineIdxPadTop)
        {
          paddingLine = - yLineInLCU + m_lineIdxPadTop - 1;
          pImgPad1 = pImgPad + min(paddingLine, 1)*stride;
          pImgPad2 = pImgPad -   stride;
          pImgPad3 = pImgPad + min(paddingLine, 2)*stride;
          pImgPad4 = pImgPad - 2*stride;
          pImgPad5 = pImgPad + min(paddingLine, 3)*stride;
          pImgPad6 = pImgPad - 3*stride;
          pImgPad7 = pImgPad + min(paddingLine, 4)*stride;
          pImgPad8 = pImgPad - 4*stride;
        }
        else
        {
          paddingLine = yLineInLCU - m_lineIdxPadTop ;
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - min(paddingLine, 1)*stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - min(paddingLine, 2)*stride;
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - min(paddingLine, 3)*stride;
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - min(paddingLine, 4)*stride;
        } 

        pVar = ppVarImg[i>>shiftHeight] + (xpos>>shiftWidth);

        if ( (yLineInLCU == m_lineIdxPadTop || yLineInLCU == m_lineIdxPadTop-1) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              varInd = *(pVar++);
              coef = filterSet[mergeTable[varInd]];
            }

#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = coef[9]; 
#endif

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[0][mergeTable[varInd]]* (pImgPad[j]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTop+1 || yLineInLCU == m_lineIdxPadTop-2) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              varInd = *(pVar++);
              coef = filterSet[mergeTable[varInd]];
            }

#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = coef[9]; 
#endif

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[1][mergeTable[varInd]]* (pImgPad[j]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTop+2 || yLineInLCU == m_lineIdxPadTop-3) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              varInd = *(pVar++);
              coef = filterSet[mergeTable[varInd]];
            }

#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = coef[9]; 
#endif

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[2][mergeTable[varInd]]* (pImgPad[j  ]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTop+3 || yLineInLCU == m_lineIdxPadTop-4) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              varInd = *(pVar++);
              coef = filterSet[mergeTable[varInd]];
            }

#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = coef[9]; 
#endif

            pixelInt += coef[1]* (pImgPad5[j]+pImgPad6[j]);

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[3][mergeTable[varInd]]* (pImgPad[j  ]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
        else
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              coef = filterSet[mergeTable[*(pVar++)]];
            }

#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = coef[9]; 
#endif

            pixelInt += coef[0]* (pImgPad7[j]+pImgPad8[j]);

            pixelInt += coef[1]* (pImgPad5[j]+pImgPad6[j]);

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[8]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
        pImgPad += stride;
        pImgRes += stride;
      }
    }
#else
  case ALF_CROSS11x5:
    {
      for(i= ypos; i<= yposEnd; i++)
      {
        pImgPad1 = pImgPad +   stride;
        pImgPad2 = pImgPad -   stride;
        pImgPad3 = pImgPad + 2*stride;
        pImgPad4 = pImgPad - 2*stride;

        pVar = ppVarImg[i>>shiftHeight] + (xpos>>shiftWidth);
        for(j= xpos; j<= xposEnd ; j++)
        {
          if (j % VAR_SIZE_W==0) 
          {
            coef = filterSet[mergeTable[*(pVar++)]];
          }

#if ALF_DC_OFFSET_REMOVAL
          pixelInt  = 0;
#else
          pixelInt  = coef[8]; 
#endif

          pixelInt += coef[0]* (pImgPad3[j]+pImgPad4[j]);

          pixelInt += coef[1]* (pImgPad1[j]+pImgPad2[j]);

          pixelInt += coef[2]* (pImgPad[j+5]+pImgPad[j-5]);
          pixelInt += coef[3]* (pImgPad[j+4]+pImgPad[j-4]);
          pixelInt += coef[4]* (pImgPad[j+3]+pImgPad[j-3]);
          pixelInt += coef[5]* (pImgPad[j+2]+pImgPad[j-2]);
          pixelInt += coef[6]* (pImgPad[j+1]+pImgPad[j-1]);
          pixelInt += coef[7]* (pImgPad[j  ]);

          pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);
          pImgRes[j] = Clip( pixelInt );
        }
        pImgPad += stride;
        pImgRes += stride;

      }
    }
#endif
    break;
  default:
    {
      printf("Not a supported filter shape\n");
      assert(0);
      exit(1);
    }
  }
}



Void TComAdaptiveLoopFilter::xCUAdaptive(TComPic* pcPic, Int filtNo, Pel *imgYFilt, Pel *imgYRec, Int Stride)
{
  // for every CU, call CU-adaptive ALF process
  for( UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = pcPic->getCU( uiCUAddr );
    xSubCUAdaptive(pcCU, filtNo, imgYFilt, imgYRec, 0, 0, Stride);
  }
}

Void TComAdaptiveLoopFilter::xSubCUAdaptive(TComDataCU* pcCU, Int filtNo, Pel *imgYFilt, Pel *imgYRec, UInt uiAbsPartIdx, UInt uiDepth, Int Stride)
{
  TComPic* pcPic = pcCU->getPic();

  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;

  // check picture boundary
  if ( ( uiRPelX >= m_img_width ) || ( uiBPelY >= m_img_height ) )
  {
    bBoundary = true;
  }

  if ( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < (g_uiMaxCUDepth-g_uiAddCUDepth) ) ) || bBoundary )
  {
    UInt uiQNumParts = ( pcPic->getNumPartInCU() >> (uiDepth<<1) )>>2;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

      if( ( uiLPelX < m_img_width ) && ( uiTPelY < m_img_height ) )
        xSubCUAdaptive(pcCU, filtNo, imgYFilt, imgYRec, uiAbsPartIdx, uiDepth+1, Stride);
    }
    return;
  }

  if ( pcCU->getAlfCtrlFlag(uiAbsPartIdx) )
  {
    filterLuma(imgYFilt, imgYRec, Stride, uiTPelY, min(uiBPelY,(unsigned int)(m_img_height-1)), uiLPelX, min(uiRPelX,(unsigned int)(m_img_width-1))
      ,filtNo, m_filterCoeffSym, m_varIndTab, m_varImg);
  }
}

#if G665_ALF_COEFF_PRED
/** Predict ALF luma filter coefficients. Centre coefficient is always predicted. Left neighbour is predicted according to flag.
 */
Void TComAdaptiveLoopFilter::predictALFCoeffLuma(ALFParam* pcAlfParam)
{
  Int sum, coeffPred, ind;
  const Int* pFiltMag = NULL;
  pFiltMag = weightsTabShapes[pcAlfParam->filter_shape];
  for(ind = 0; ind < pcAlfParam->filters_per_group; ++ind)
  {
    sum = 0;
#if ALF_DC_OFFSET_REMOVAL
    for(Int i = 0; i < pcAlfParam->num_coeff-2; i++)
#else
    for(Int i = 0; i < pcAlfParam->num_coeff-3; i++)
#endif
    {
      sum +=  pFiltMag[i]*pcAlfParam->coeffmulti[ind][i];
    }
    if(pcAlfParam->nbSPred[ind]==0)
    {
      if((pcAlfParam->predMethod==0)|(ind==0))
      {
        coeffPred = ((1<<ALF_NUM_BIT_SHIFT)-sum) >> 2;
      }
      else
      {
        coeffPred = (0-sum) >> 2;
      }
#if ALF_DC_OFFSET_REMOVAL
      pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-2] = coeffPred + pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-2];
#else
      pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-3] = coeffPred + pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-3];
#endif
    }
#if ALF_DC_OFFSET_REMOVAL
    sum += pFiltMag[pcAlfParam->num_coeff-2]*pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-2];
#else
    sum += pFiltMag[pcAlfParam->num_coeff-3]*pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-3];
#endif
    if((pcAlfParam->predMethod==0)|(ind==0))
    {
      coeffPred = (1<<ALF_NUM_BIT_SHIFT)-sum;
    }
    else
    {
      coeffPred = -sum;
    }
#if ALF_DC_OFFSET_REMOVAL
    pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-1] = coeffPred + pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-1];
#else
    pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-2] = coeffPred + pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-2];
#endif
  }
}
#endif

Void TComAdaptiveLoopFilter::reconstructFilterCoeffs(ALFParam* pcAlfParam,int **pfilterCoeffSym)
{
  int i, ind;

  // Copy non zero filters in filterCoeffTmp
  for(ind = 0; ind < pcAlfParam->filters_per_group; ++ind)
  {
    for(i = 0; i < pcAlfParam->num_coeff; i++)
    {
      pfilterCoeffSym[ind][i] = pcAlfParam->coeffmulti[ind][i];
    }
  }
  // Undo prediction
  for(ind = 1; ind < pcAlfParam->filters_per_group; ++ind)
  {
    if(pcAlfParam->predMethod)
    {
      // Prediction
      for(i = 0; i < pcAlfParam->num_coeff; ++i)
      {
        pfilterCoeffSym[ind][i] = (int)(pfilterCoeffSym[ind][i] + pfilterCoeffSym[ind - 1][i]);
      }
    }
  }

#if G214_ALF_CONSTRAINED_COEFF
  for(ind = 0; ind < pcAlfParam->filters_per_group; ind++)
  {
    checkFilterCoeffValue(pfilterCoeffSym[ind], pcAlfParam->num_coeff, false );
  }
#endif

}

static Pel Clip_post(int high, int val)
{
  return (Pel)(((val > high)? high: val));
}

#if G609_NEW_BA_SUB
/** Calculate ALF grouping indices for block-based (BA) mode
 * \param [out] imgYvar grouping indices buffer
 * \param [in] imgYpad picture buffer
 * \param [in] stride picture stride size
 * \param [in] adaptationMode  ALF_BA or ALF_RA mode
 */
Void TComAdaptiveLoopFilter::calcVar(Pel **imgYvar, Pel *imgYpad, Int stride, Int adaptationMode)
{
  if(adaptationMode == ALF_RA) 
  {
    return;
  }
  static Int shiftH = (Int)(log((double)VAR_SIZE_H)/log(2.0));
  static Int shiftW = (Int)(log((double)VAR_SIZE_W)/log(2.0));
  static Int varmax = (Int)NO_VAR_BINS-1;
  static Int step1  = (Int)((Int)(NO_VAR_BINS)/3) - 1;  
  static Int th[NO_VAR_BINS] = {0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4}; 
  
  Int i, j, avgvar, vertical, horizontal,direction, yoffset;
  Pel *pimgYpad, *pimgYpadup, *pimgYpaddown;

  for(i = 0; i < m_img_height - 3; i=i+4)
  {
    yoffset      = ((i)*stride) + stride;
    pimgYpad     = &imgYpad [yoffset];
    pimgYpadup   = &imgYpad [yoffset + stride];
    pimgYpaddown = &imgYpad [yoffset - stride];

    for(j = 0; j < m_img_width - 3 ; j=j+4)
    {
      // Compute at sub-sample by 2
      vertical   =  abs((pimgYpad[j+1]<<1  ) - pimgYpaddown[j+1]   - pimgYpadup[j+1]);
      horizontal =  abs((pimgYpad[j+1]<<1  ) - pimgYpad    [j+2]   - pimgYpad  [j  ]);

      vertical   += abs((pimgYpad[j+2]<<1  ) - pimgYpaddown[j+2]   - pimgYpadup[j+2]);
      horizontal += abs((pimgYpad[j+2]<<1  ) - pimgYpad    [j+3]   - pimgYpad  [j+1]);

      vertical   += abs((pimgYpad[j+1+stride]<<1) - pimgYpaddown[j+1+stride] - pimgYpadup[j+1+stride]);
      horizontal += abs((pimgYpad[j+1+stride]<<1) - pimgYpad    [j+2+stride] - pimgYpad  [j+stride  ]);

      vertical   += abs((pimgYpad[j+2+stride]<<1) - pimgYpaddown[j+2+stride] - pimgYpadup[j+2+stride]);
      horizontal += abs((pimgYpad[j+2+stride]<<1) - pimgYpad    [j+3+stride] - pimgYpad  [j+1+stride]);

      direction = 0;
      if (vertical > 2*horizontal) 
      {
        direction = 1; //vertical
      }
      if (horizontal > 2*vertical)
      {
        direction = 2; //horizontal
      }

      avgvar = (vertical + horizontal) >> 2;
      avgvar = (Pel) Clip_post(varmax, avgvar >>(g_uiBitIncrement+1));
      avgvar = th[avgvar];
      avgvar = Clip_post(step1, (Int) avgvar ) + (step1+1)*direction;
      imgYvar[(i )>>shiftH][(j)>>shiftW] = avgvar;
    }
  }
}

#else
Void TComAdaptiveLoopFilter::calcVar(int ypos, int xpos, Pel **imgY_var, Pel *imgY_pad, int fl, int img_height, int img_width, int img_stride)
{
  if(m_uiVarGenMethod == ALF_RA)
  {
    return;
  }
  static Int shift_h     = (Int)(log((double)VAR_SIZE_H)/log(2.0));
  static Int shift_w     = (Int)(log((double)VAR_SIZE_W)/log(2.0));

  Int start_height = ypos;
  Int start_width  = xpos;
  Int end_height   = ypos + img_height;
  Int end_width    = xpos + img_width;
  Int i, j;
  Int fl2plusOne= (VAR_SIZE<<1)+1; //3
  Int var_max= NO_VAR_BINS-1;
  Int avg_var;
  Int vertical, horizontal;
  Int direction;
  Int step1 = NO_VAR_BINS/3 - 1;
  Int th[NO_VAR_BINS] = {0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4}; 

  for(i = 1+start_height+1; i < end_height + fl2plusOne; i=i+4)
  {
    Int yoffset = ((i-fl-1) * img_stride) -fl-1;
    Pel *p_imgY_pad = &imgY_pad[yoffset];
    Pel *p_imgY_pad_up   = &imgY_pad[yoffset + img_stride];
    Pel *p_imgY_pad_down = &imgY_pad[yoffset - img_stride];
    for(j = 1+start_width+1; j < end_width +fl2plusOne; j=j+4)
    {
      // Compute at sub-sample by 2
      vertical = abs((p_imgY_pad[j]<<1) - p_imgY_pad_down[j] - p_imgY_pad_up[j]);
      horizontal = abs((p_imgY_pad[j]<<1) - p_imgY_pad[j+1] - p_imgY_pad[j-1]);

      vertical += abs((p_imgY_pad[j+2]<<1) - p_imgY_pad_down[j+2] - p_imgY_pad_up[j+2]);
      horizontal += abs((p_imgY_pad[j+2]<<1) - p_imgY_pad[j+1+2] - p_imgY_pad[j-1+2]);

      vertical += abs((p_imgY_pad[j+2*img_stride]<<1) - p_imgY_pad_down[j+2*img_stride] - p_imgY_pad_up[j+2*img_stride]);
      horizontal += abs((p_imgY_pad[j+2*img_stride]<<1) - p_imgY_pad[j+1+2*img_stride] - p_imgY_pad[j-1+2*img_stride]);

      vertical += abs((p_imgY_pad[j+2+2*img_stride]<<1) - p_imgY_pad_down[j+2+2*img_stride] - p_imgY_pad_up[j+2+2*img_stride]);
      horizontal += abs((p_imgY_pad[j+2+2*img_stride]<<1) - p_imgY_pad[j+1+2+2*img_stride] - p_imgY_pad[j-1+2+2*img_stride]);

      direction = 0;
      if (vertical > 2*horizontal) direction = 1; //vertical
      if (horizontal > 2*vertical) direction = 2; //horizontal
      avg_var = (vertical + horizontal)>>2;
      avg_var = (Pel) Clip_post(var_max, avg_var>>(g_uiBitIncrement+1));
      avg_var = th[avg_var];
      avg_var = Clip_post(step1, (Int) avg_var ) + (step1+1)*direction;
      imgY_var[(i - 1)>>shift_h][(j - 1)>>shift_w] = avg_var;
    }
  }
}
#endif

Void TComAdaptiveLoopFilter::createRegionIndexMap(Pel **imgYVar, Int imgWidth, Int imgHeight)
{
  int varStepSizeWidth = VAR_SIZE_W;
  int varStepSizeHeight = VAR_SIZE_H;
  int shiftHeight = (int)(log((double)varStepSizeHeight)/log(2.0));
  int shiftWidth = (int)(log((double)varStepSizeWidth)/log(2.0));

  int i, j;
  int regionTable[NO_VAR_BINS] = {0, 1, 4, 5, 15, 2, 3, 6, 14, 11, 10, 7, 13, 12,  9,  8}; 
  int xInterval;
  int yInterval;
  int yIndex;
  int yIndexOffset;
  int yStartLine;
  int yEndLine;

  xInterval = ((( (imgWidth+63)/64) + 1) / 4 * 64)>>shiftWidth;  
  yInterval = ((((imgHeight+63)/64) + 1) / 4 * 64)>>shiftHeight;

  for (yIndex = 0; yIndex < 4 ; yIndex++)
  {
    yIndexOffset = yIndex * 4;
    yStartLine = yIndex * yInterval;
    yEndLine   = (yIndex == 3) ? imgHeight>>shiftHeight : (yStartLine+yInterval);

    for(i = yStartLine; i < yEndLine ; i++)
    {
      for(j = 0; j < xInterval ; j++)
      {
        imgYVar[i][j] = regionTable[yIndexOffset+0];     
      }

      for(j = xInterval; j < xInterval*2 ; j++)
      {
        imgYVar[i][j] = regionTable[yIndexOffset+1];     
      }

      for(j = xInterval*2; j < xInterval*3 ; j++)
      {
        imgYVar[i][j] = regionTable[yIndexOffset+2];     
      }

      for(j = xInterval*3; j < imgWidth>>shiftWidth ; j++)
      {
        imgYVar[i][j] = regionTable[yIndexOffset+3];     
      }
    }
  }

}

// --------------------------------------------------------------------------------------------------------------------
// ALF for chroma
// --------------------------------------------------------------------------------------------------------------------

/** 
 \param pcPicDec    picture before ALF
 \param pcPicRest   picture after  ALF
 \param qh          filter coefficient
 \param iTap        filter tap
 \param iColor      0 for Cb and 1 for Cr
 */
Void TComAdaptiveLoopFilter::filterChroma(Pel *pImgRes, Pel *pImgPad, Int stride, 
                                          Int ypos, Int yposEnd, Int xpos, Int xposEnd, 
                                          Int filtNo, Int* coef)
{
  static Int numBitsMinus1= (Int)ALF_NUM_BIT_SHIFT;
  static Int offset       = (1<<( (Int)ALF_NUM_BIT_SHIFT-1));
#if !ALF_DC_OFFSET_REMOVAL
  Int iShift = g_uiBitDepth + g_uiBitIncrement - 8;
#endif

  Pel *pImgPad1,*pImgPad2,*pImgPad3,*pImgPad4;
  Int i, j, pixelInt;

  pImgPad    += (ypos*stride);
  pImgRes    += (ypos*stride);

#if G212_CROSS9x9_VB
  Int imgHeightChroma = m_img_height>>1;
  Int yLineInLCU;
  Int paddingline;
  Int newCenterCoeff[4];

  ::memset(newCenterCoeff, 0, sizeof(Int)*4);
  if (filtNo == ALF_CROSS9x9)
  {
    //VB line 1
    newCenterCoeff[0] = coef[8] + ((coef[0] + coef[1] + coef[2] + coef[3])<<1);
    //VB line 2 
    newCenterCoeff[1] = coef[8] + ((coef[0] + coef[1] + coef[2])<<1);
    //VB line 3 
    newCenterCoeff[2] = coef[8] + ((coef[0] + coef[1])<<1);
    //VB line 4 
    newCenterCoeff[3] = coef[8] + ((coef[0])<<1);
  }
#endif

  switch(filtNo)
  {
  case ALF_STAR5x5:
    {
      for(i= ypos; i<= yposEnd; i++)
      {
#if G212_CROSS9x9_VB
        yLineInLCU = i % m_lcuHeightChroma;

        if (yLineInLCU < m_lineIdxPadBotChroma || i-yLineInLCU+m_lcuHeightChroma >= imgHeightChroma )
        {
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
        }
        else if (yLineInLCU < m_lineIdxPadTopChroma)
        {
          paddingline = - yLineInLCU + m_lineIdxPadTopChroma - 1;
          pImgPad1 = pImgPad + min(paddingline, 1)*stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + min(paddingline, 2)*stride;
          pImgPad4 = pImgPad - 2*stride;
        }
        else
        {
          paddingline = yLineInLCU - m_lineIdxPadTopChroma ;
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - min(paddingline, 1)*stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - min(paddingline, 2)*stride;
        }
#else
        pImgPad1 = pImgPad +   stride;
        pImgPad2 = pImgPad -   stride;
        pImgPad3 = pImgPad + 2*stride;
        pImgPad4 = pImgPad - 2*stride;
#endif

#if G212_CROSS9x9_VB
        if ( (yLineInLCU == m_lineIdxPadTopChroma || yLineInLCU == m_lineIdxPadTopChroma-1) && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            pImgRes[j] = pImgPad[j];
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTopChroma+1 || yLineInLCU == m_lineIdxPadTopChroma-2) && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = (coef[9] << iShift); 
#endif

            pixelInt += coef[0]* (pImgPad3[j+2]+pImgPad4[j-2]);
            pixelInt += coef[1]* (pImgPad3[j  ]+pImgPad4[j  ]);
            pixelInt += coef[2]* (pImgPad3[j-2]+pImgPad4[j+2]);

            pixelInt += coef[3]* (pImgPad1[j+1]+pImgPad2[j-1]);
            pixelInt += coef[4]* (pImgPad1[j  ]+pImgPad2[j  ]);
            pixelInt += coef[5]* (pImgPad1[j-1]+pImgPad2[j+1]);

            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[8]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = (Clip( pixelInt ) + pImgPad[j]) >> 1;
          }
        }
        else
        {
#endif

        for(j= xpos; j<= xposEnd ; j++)
        {
#if ALF_DC_OFFSET_REMOVAL
          pixelInt  = 0;
#else
          pixelInt  = (coef[9] << iShift); 
#endif

          pixelInt += coef[0]* (pImgPad3[j+2]+pImgPad4[j-2]);
          pixelInt += coef[1]* (pImgPad3[j  ]+pImgPad4[j  ]);
          pixelInt += coef[2]* (pImgPad3[j-2]+pImgPad4[j+2]);

          pixelInt += coef[3]* (pImgPad1[j+1]+pImgPad2[j-1]);
          pixelInt += coef[4]* (pImgPad1[j  ]+pImgPad2[j  ]);
          pixelInt += coef[5]* (pImgPad1[j-1]+pImgPad2[j+1]);

          pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
          pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
          pixelInt += coef[8]* (pImgPad[j  ]);

          pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

          pImgRes[j] = Clip( pixelInt );
        }

#if G212_CROSS9x9_VB
        }
#endif
        pImgPad += stride;
        pImgRes += stride;
      }
    }
    break;
#if G212_CROSS9x9_VB
  case ALF_CROSS9x9:
    {
      Pel *pImgPad5, *pImgPad6, *pImgPad7, *pImgPad8;

      for(i= ypos; i<= yposEnd; i++)
      {
        yLineInLCU = i % m_lcuHeightChroma;

        if (yLineInLCU<2)
        {
          paddingline = yLineInLCU + 2 ;
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - min(paddingline, 3)*stride;
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - min(paddingline, 4)*stride;
        }
        else if (yLineInLCU < m_lineIdxPadBotChroma || i-yLineInLCU+m_lcuHeightChroma >= imgHeightChroma )
        {
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - 3*stride;
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - 4*stride;
        }
        else if (yLineInLCU < m_lineIdxPadTopChroma)
        {
          paddingline = - yLineInLCU + m_lineIdxPadTopChroma - 1;
          pImgPad1 = pImgPad + min(paddingline, 1)*stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + min(paddingline, 2)*stride;
          pImgPad4 = pImgPad - 2*stride;
          pImgPad5 = pImgPad + min(paddingline, 3)*stride;
          pImgPad6 = pImgPad - 3*stride;
          pImgPad7 = pImgPad + min(paddingline, 4)*stride;
          pImgPad8 = pImgPad - 4*stride;
        }
        else
        {
          paddingline = yLineInLCU - m_lineIdxPadTopChroma ;
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - min(paddingline, 1)*stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - min(paddingline, 2)*stride;
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - min(paddingline, 3)*stride;
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - min(paddingline, 4)*stride;
        }

        if ( (yLineInLCU == m_lineIdxPadTopChroma || yLineInLCU == m_lineIdxPadTopChroma-1) && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = (coef[9] << iShift); 
#endif

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[0]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTopChroma+1 || yLineInLCU == m_lineIdxPadTopChroma-2) && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = (coef[9] << iShift); 
#endif
            
            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[1]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == 0 && i>0) || (yLineInLCU == m_lineIdxPadTopChroma-3 && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma) )
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = (coef[9] << iShift); 
#endif

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[2]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == 1 && i>1) || (yLineInLCU == m_lineIdxPadTopChroma-4 && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma) ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = (coef[9] << iShift); 
#endif

            pixelInt += coef[1]* (pImgPad5[j]+pImgPad6[j]);

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[3]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
        else
        {          
          for(j= xpos; j<= xposEnd ; j++)
          {
#if ALF_DC_OFFSET_REMOVAL
            pixelInt  = 0;
#else
            pixelInt  = (coef[9] << iShift); 
#endif

            pixelInt += coef[0]* (pImgPad7[j]+pImgPad8[j]);

            pixelInt += coef[1]* (pImgPad5[j]+pImgPad6[j]);

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[8]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
        pImgPad += stride;
        pImgRes += stride;

      }
    }

#else
  case ALF_CROSS11x5:
    {
      for(i= ypos; i<= yposEnd; i++)
      {
        pImgPad1 = pImgPad +   stride;
        pImgPad2 = pImgPad -   stride;
        pImgPad3 = pImgPad + 2*stride;
        pImgPad4 = pImgPad - 2*stride;

        for(j= xpos; j<= xposEnd ; j++)
        {
#if ALF_DC_OFFSET_REMOVAL
          pixelInt  = 0;
#else
          pixelInt  = (coef[8] << iShift); 
#endif

          pixelInt += coef[0]* (pImgPad3[j]+pImgPad4[j]);

          pixelInt += coef[1]* (pImgPad1[j]+pImgPad2[j]);

          pixelInt += coef[2]* (pImgPad[j+5]+pImgPad[j-5]);
          pixelInt += coef[3]* (pImgPad[j+4]+pImgPad[j-4]);
          pixelInt += coef[4]* (pImgPad[j+3]+pImgPad[j-3]);
          pixelInt += coef[5]* (pImgPad[j+2]+pImgPad[j-2]);
          pixelInt += coef[6]* (pImgPad[j+1]+pImgPad[j-1]);
          pixelInt += coef[7]* (pImgPad[j  ]);

          pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

          pImgRes[j] = Clip( pixelInt );
        }
        pImgPad += stride;
        pImgRes += stride;

      }
    }
#endif
    break;
  default:
    {
      printf("Not a supported filter shape\n");
      assert(0);
      exit(1);
    }
  }

}

#if NONCROSS_TILE_IN_LOOP_FILTERING
/** Chroma filtering for multi-slice picture
 * \param componentID slice parameters
 * \param pcPicDecYuv original picture
 * \param pcPicRestYuv picture before filtering
 * \param coeff filter coefficients
 * \param filtNo  filter shape
 * \param chromaFormatShift size adjustment for chroma (1 for 4:2:0 format)
 */
Void TComAdaptiveLoopFilter::xFilterChromaSlices(Int componentID, TComPicYuv* pcPicDecYuv, TComPicYuv* pcPicRestYuv, Int *coeff, Int filtNo, Int chromaFormatShift)
{
  Pel* pPicDec   = (componentID == ALF_Cb)?(    pcPicDecYuv->getCbAddr()):(    pcPicDecYuv->getCrAddr());
  Pel* pPicSlice = (componentID == ALF_Cb)?(m_pcSliceYuvTmp->getCbAddr()):(m_pcSliceYuvTmp->getCrAddr());
  Pel* pRest     = (componentID == ALF_Cb)?(   pcPicRestYuv->getCbAddr()):(   pcPicRestYuv->getCrAddr());
  Int  stride    = pcPicDecYuv->getCStride();

  for(UInt s=0; s< m_uiNumSlicesInPic; s++)
  {
    if(!m_pcPic->getValidSlice(s))
    {
      continue;
    }
    std::vector< std::vector<AlfLCUInfo*> > & vpSliceTileAlfLCU = m_pvpSliceTileAlfLCU[s];

    for(Int t=0; t< (Int)vpSliceTileAlfLCU.size(); t++)
    {
      std::vector<AlfLCUInfo*> & vpAlfLCU = vpSliceTileAlfLCU[t];

      copyRegion(vpAlfLCU, pPicSlice, pPicDec, stride, chromaFormatShift);
      extendRegionBorder(vpAlfLCU, pPicSlice, stride, chromaFormatShift);
      filterChromaRegion(vpAlfLCU, pPicSlice, pRest, stride, coeff, filtNo, chromaFormatShift);
    }
  }
}
#endif

#if NONCROSS_TILE_IN_LOOP_FILTERING
/** Chroma filtering for one component in multi-slice picture
 * \param componentID slice parameters
 * \param pcPicDecYuv original picture
 * \param pcPicRestYuv picture before filtering
 * \param shape  filter shape
 * \param pCoeff filter coefficients
 */
Void TComAdaptiveLoopFilter::xFilterChromaOneCmp(Int componentID, TComPicYuv *pDecYuv, TComPicYuv *pRestYuv, Int shape, Int *pCoeff)
#else
Void TComAdaptiveLoopFilter::xFilterChromaOneCmp(Pel *pDec, Pel *pRest, Int iStride, Int shape, Int *pCoeff)
#endif
{
  Int chromaFormatShift = 1;
  if(!m_bUseNonCrossALF)
  {
#if NONCROSS_TILE_IN_LOOP_FILTERING
    Pel* pDec    = (componentID == ALF_Cb)?(pDecYuv->getCbAddr()): (pDecYuv->getCrAddr());
    Pel* pRest   = (componentID == ALF_Cb)?(pRestYuv->getCbAddr()):(pRestYuv->getCrAddr());
    Int  iStride = pDecYuv->getCStride();
#endif
    filterChroma(pRest, pDec, iStride, 0, (Int)(m_img_height>>1) -1, 0, (Int)(m_img_width>>1)-1, shape, pCoeff);
  }
  else
  {
#if NONCROSS_TILE_IN_LOOP_FILTERING
    xFilterChromaSlices(componentID, pDecYuv, pRestYuv, pCoeff, shape, chromaFormatShift);
#else
    for(UInt s=0; s< m_uiNumSlicesInPic; s++)
    {
      CAlfSlice* pSlice = &(m_pSlice[s]);
      if(!pSlice->isValidSlice()) 
      {
        continue;
      }

      pSlice->copySliceChroma(pDec, pRest, iStride);
      pSlice->extendSliceBorderChroma(pDec, iStride);
      xFilterOneChromaSlice(pSlice, pDec, pRest, iStride, pCoeff, shape, chromaFormatShift);
    }
#endif
  }
}

/** Chroma filtering for  multi-slice picture
 * \param pcAlfParam ALF parameters
 * \param pcPicDec to-be-filtered picture buffer 
 * \param pcPicRest filtered picture buffer
 */
Void TComAdaptiveLoopFilter::xALFChroma(ALFParam* pcAlfParam, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest)
{
  if((pcAlfParam->chroma_idc>>1)&0x01)
  {
#if NONCROSS_TILE_IN_LOOP_FILTERING
    xFilterChromaOneCmp(ALF_Cb, pcPicDec, pcPicRest, pcAlfParam->filter_shape_chroma, pcAlfParam->coeff_chroma);
#else
    Pel* pDec  = pcPicDec->getCbAddr();
    Pel* pRest = pcPicRest->getCbAddr();
    xFilterChromaOneCmp(pDec, pRest, pcPicRest->getCStride(), pcAlfParam->filter_shape_chroma, pcAlfParam->coeff_chroma);
#endif
  }

  if(pcAlfParam->chroma_idc&0x01)
  {
#if NONCROSS_TILE_IN_LOOP_FILTERING
    xFilterChromaOneCmp(ALF_Cr, pcPicDec, pcPicRest, pcAlfParam->filter_shape_chroma, pcAlfParam->coeff_chroma);
#else
    Pel* pDec  = pcPicDec->getCrAddr();
    Pel* pRest = pcPicRest->getCrAddr();
    xFilterChromaOneCmp(pDec, pRest, pcPicRest->getCStride(), pcAlfParam->filter_shape_chroma, pcAlfParam->coeff_chroma);
#endif
  }
}

Void TComAdaptiveLoopFilter::setAlfCtrlFlags(AlfCUCtrlInfo* pAlfParam, TComDataCU *pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt &idx)
{
  TComPic* pcPic = pcCU->getPic();
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiQNumParts      = uiCurNumParts>>2;
  
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  
  if( ( uiRPelX >= pcCU->getSlice()->getSPS()->getWidth() ) || ( uiBPelY >= pcCU->getSlice()->getSPS()->getHeight() ) )
  {
    bBoundary = true;
  }
  
  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary )
  {
    UInt uiIdx = uiAbsPartIdx;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];
      
      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
      {
        setAlfCtrlFlags(pAlfParam, pcCU, uiIdx, uiDepth+1, idx);
      }
      uiIdx += uiQNumParts;
    }
    
    return;
  }
  
  if( uiDepth <= pAlfParam->alf_max_depth || pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, pAlfParam->alf_max_depth))
  {
    if (uiDepth > pAlfParam->alf_max_depth)
    {
      pcCU->setAlfCtrlFlagSubParts(pAlfParam->alf_cu_flag[idx], uiAbsPartIdx, pAlfParam->alf_max_depth);
    }
    else
    {
      pcCU->setAlfCtrlFlagSubParts(pAlfParam->alf_cu_flag[idx], uiAbsPartIdx, uiDepth );
    }
    idx++;
  }
}

#if NONCROSS_TILE_IN_LOOP_FILTERING
/** Initialize the variables for one ALF LCU
 * \param rAlfLCU to-be-initialized ALF LCU
 * \param sliceID slice index
 * \param tileID tile index
 * \param pcCU CU data pointer
 * \param maxNumSUInLCU maximum number of SUs in one LCU
 */
Void TComAdaptiveLoopFilter::InitAlfLCUInfo(AlfLCUInfo& rAlfLCU, Int sliceID, Int tileID, TComDataCU* pcCU, UInt maxNumSUInLCU)
{
  //pcCU
  rAlfLCU.pcCU     = pcCU;
  //sliceID
  rAlfLCU.sliceID = sliceID;
  //tileID
  rAlfLCU.tileID  = tileID;

  //numSGU, vpAlfBLock;
  std::vector<NDBFBlockInfo>& vNDBFBlock = *(pcCU->getNDBFilterBlocks());
  rAlfLCU.vpAlfBlock.clear();
  rAlfLCU.numSGU = 0;
  for(Int i=0; i< vNDBFBlock.size(); i++)
  {
    if( vNDBFBlock[i].sliceID == sliceID)
    {
      rAlfLCU.vpAlfBlock.push_back( &(vNDBFBlock[i])  );
      rAlfLCU.numSGU ++;
    }
  }
  //startSU
  rAlfLCU.startSU = rAlfLCU.vpAlfBlock.front()->startSU;
  //endSU
  rAlfLCU.endSU   = rAlfLCU.vpAlfBlock.back()->endSU;
  //bAllSUsInLCUInSameSlice
  rAlfLCU.bAllSUsInLCUInSameSlice = (rAlfLCU.startSU == 0)&&( rAlfLCU.endSU == maxNumSUInLCU -1);
}

/** create and initialize variables for picture ALF processing
 * \param pcPic picture-level data pointer
 * \param numSlicesInPic number of slices in picture
 */
Void TComAdaptiveLoopFilter::createPicAlfInfo(TComPic* pcPic, Int numSlicesInPic)
{
  m_uiNumSlicesInPic = numSlicesInPic;
  m_iSGDepth         = pcPic->getSliceGranularityForNDBFilter();

  m_bUseNonCrossALF = ( pcPic->getIndependentSliceBoundaryForNDBFilter() || pcPic->getIndependentTileBoundaryForNDBFilter());
  m_pcPic = pcPic;

  if(m_uiNumSlicesInPic > 1 || m_bUseNonCrossALF)
  {
    m_ppSliceAlfLCUs = new AlfLCUInfo*[m_uiNumSlicesInPic];
    m_pvpAlfLCU = new std::vector< AlfLCUInfo* >[m_uiNumSlicesInPic];
    m_pvpSliceTileAlfLCU = new std::vector< std::vector< AlfLCUInfo* > >[m_uiNumSlicesInPic];

    for(Int s=0; s< m_uiNumSlicesInPic; s++)
    {
      m_ppSliceAlfLCUs[s] = NULL;
      if(!pcPic->getValidSlice(s))
      {
        continue;
      }

      std::vector< TComDataCU* >& vSliceLCUPointers = pcPic->getOneSliceCUDataForNDBFilter(s);
      Int                         numLCU           = (Int)vSliceLCUPointers.size();

      //create Alf LCU info
      m_ppSliceAlfLCUs[s] = new AlfLCUInfo[numLCU];
      for(Int i=0; i< numLCU; i++)
      {
        TComDataCU* pcCU       = vSliceLCUPointers[i];
        Int         currTileID = pcPic->getPicSym()->getTileIdxMap(pcCU->getAddr());

        InitAlfLCUInfo(m_ppSliceAlfLCUs[s][i], s, currTileID, pcCU, pcPic->getNumPartInCU());
      }

      //distribute Alf LCU info pointers to slice container
      std::vector< AlfLCUInfo* >&    vpSliceAlfLCU     = m_pvpAlfLCU[s]; 
      vpSliceAlfLCU.reserve(numLCU);
      vpSliceAlfLCU.resize(0);
      std::vector< std::vector< AlfLCUInfo* > > &vpSliceTileAlfLCU = m_pvpSliceTileAlfLCU[s];
      Int prevTileID = -1;
      Int numValidTilesInSlice = 0;

      for(Int i=0; i< numLCU; i++)
      {
        AlfLCUInfo* pcAlfLCU = &(m_ppSliceAlfLCUs[s][i]);

        //container of Alf LCU pointers for slice processing
        vpSliceAlfLCU.push_back( pcAlfLCU);

        if(pcAlfLCU->tileID != prevTileID)
        {
          if(prevTileID == -1 || pcPic->getIndependentTileBoundaryForNDBFilter())
          {
            prevTileID = pcAlfLCU->tileID;
            numValidTilesInSlice ++;
            vpSliceTileAlfLCU.resize(numValidTilesInSlice);
          }
        }
        //container of Alf LCU pointers for tile processing 
        vpSliceTileAlfLCU[numValidTilesInSlice-1].push_back(pcAlfLCU);
      }

      assert( vpSliceAlfLCU.size() == numLCU);
    }
 
    if(m_bUseNonCrossALF)
    {
      m_pcSliceYuvTmp = pcPic->getYuvPicBufferForIndependentBoundaryProcessing();
    }

  }


}

/** Destroy ALF slice units
 */
Void TComAdaptiveLoopFilter::destroyPicAlfInfo()
{
  if(m_bUseNonCrossALF)
  {
    for(Int s=0; s< m_uiNumSlicesInPic; s++)
    {
      if(m_ppSliceAlfLCUs[s] != NULL)
      {
        delete[] m_ppSliceAlfLCUs[s];
        m_ppSliceAlfLCUs[s] = NULL;
      }
    }
    delete[] m_ppSliceAlfLCUs;
    m_ppSliceAlfLCUs = NULL;

    delete[] m_pvpAlfLCU;
    m_pvpAlfLCU = NULL;

    delete[] m_pvpSliceTileAlfLCU;
    m_pvpSliceTileAlfLCU = NULL;
  }
}

/** ALF for cu-on/off-controlled region
 * \param vpAlfLCU ALF LCU information container
 * \param imgDec to-be-filtered picture buffer
 * \param imgRest filtered picture buffer
 * \param stride picture buffer stride size
 * \param filtNo filter shape
 * \param filterCoeff filter coefficients
 * \param mergeTable merge table for filter set
 * \param varImg BA index 
 */
Void TComAdaptiveLoopFilter::xCUAdaptiveRegion(std::vector<AlfLCUInfo*> &vpAlfLCU, Pel* imgDec, Pel* imgRest, Int stride, Int filtNo, Int** filterCoeff, Int* mergeTable, Pel** varImg)
{
  UInt SUWidth   = m_pcPic->getMinCUWidth();
  UInt SUHeight  = m_pcPic->getMinCUHeight();
  UInt idx, startSU, endSU, currSU, LCUX, LCUY, LPelX, TPelY;
  TComDataCU* pcCU;

  for(idx=0; idx< vpAlfLCU.size(); idx++)
  {
    AlfLCUInfo&    rAlfLCU = *(vpAlfLCU[idx]);
    pcCU                   = rAlfLCU.pcCU;
    startSU              = rAlfLCU.startSU;
    endSU                = rAlfLCU.endSU;
    LCUX                 = pcCU->getCUPelX();
    LCUY                 = pcCU->getCUPelY();

    for(currSU= startSU; currSU<= endSU; currSU++)
    {
      LPelX   = LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[currSU] ];
      TPelY   = LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[currSU] ];
      if( !( LPelX < m_img_width )  || !( TPelY < m_img_height )  )
      {
        continue;
      }
      if(pcCU->getAlfCtrlFlag(currSU))
      {
        filterLuma(imgRest, imgDec, stride, TPelY, TPelY+ SUHeight-1, LPelX, LPelX+ SUWidth-1,  filtNo, m_filterCoeffSym, m_varIndTab, m_varImg);
      }
    }
  }

}

/** ALF for "non" cu-on/off-controlled region
 * \param vpAlfLCU ALF LCU information container
 * \param imgDec to-be-filtered picture buffer
 * \param imgRest filtered picture buffer
 * \param stride picture buffer stride size
 * \param filtNo filter shape
 * \param filterCoeff filter coefficients
 * \param mergeTable merge table for filter set
 * \param varImg BA index 
 */
Void TComAdaptiveLoopFilter::filterLumaRegion(std::vector<AlfLCUInfo*> &vpAlfLCU, Pel* imgDec, Pel* imgRest, Int stride, Int filtNo, Int** filterCoeff, Int* mergeTable, Pel** varImg)
{

  Int height, width;
  Int ypos, xpos;

  for(Int i=0; i< vpAlfLCU.size(); i++)
  {
    AlfLCUInfo& rAlfLCU = *(vpAlfLCU[i]); 
    for(UInt j=0; j< rAlfLCU.numSGU; j++)
    {
      ypos   = (Int)(rAlfLCU[j].posY  );
      xpos   = (Int)(rAlfLCU[j].posX  );
      height = (Int)(rAlfLCU[j].height);
      width  = (Int)(rAlfLCU[j].width );

      filterLuma(imgRest, imgDec, stride, ypos, ypos+ height-1, xpos, xpos+ width-1,  filtNo, filterCoeff, mergeTable, varImg);
    }
  }
}


#else

/** create ALF slice units
 * \param pcPic picture related parameters
 */
Void TComAdaptiveLoopFilter::createSlice(TComPic* pcPic)
{
  UInt uiMaxNumSUInLCU = pcPic->getNumPartInCU();
  UInt uiNumLCUInPic   = pcPic->getNumCUsInFrame();

  m_piSliceSUMap = new Int[uiMaxNumSUInLCU * uiNumLCUInPic];
  for(UInt i=0; i< (uiMaxNumSUInLCU * uiNumLCUInPic); i++ )
  {
    m_piSliceSUMap[i] = -1;
  }

  for( UInt uiCUAddr = 0; uiCUAddr < uiNumLCUInPic ; uiCUAddr++ )
  {
    TComDataCU* pcCU = pcPic->getCU( uiCUAddr );
    pcCU->setSliceSUMap(m_piSliceSUMap + (uiCUAddr* uiMaxNumSUInLCU)); 
  }

  m_pSlice = new CAlfSlice[m_uiNumSlicesInPic];
  for(Int i=0; i< m_uiNumSlicesInPic; i++)
  {
    m_pSlice[i].init(pcPic, m_iSGDepth, m_piSliceSUMap);
  }
}

/** Destroy ALF slice units
 */
Void TComAdaptiveLoopFilter::destroySlice()
{
  if(m_pSlice != NULL)
  {
    delete[] m_pSlice;
    m_pSlice = NULL;
  }

  if(m_piSliceSUMap != NULL)
  {
    delete[] m_piSliceSUMap;
    m_piSliceSUMap = NULL;
  }
}
#endif
#if !G609_NEW_BA_SUB
/** Calculate ALF grouping indices for one slice
 * \param pSlice slice variables
 * \param imgY_var grouping indices buffer
 * \param imgY_pad padded picture buffer
 * \param pad_size (max. filter tap)/2
 * \param fl  VAR_SIZE
 * \param img_stride picture buffer stride
 */
#if NONCROSS_TILE_IN_LOOP_FILTERING
Void TComAdaptiveLoopFilter::calcVarforOneSlice(std::vector<AlfLCUInfo*> &vpAlfLCU, Pel **imgY_var, Pel *imgY_pad, Int fl, Int img_stride)
#else
Void TComAdaptiveLoopFilter::calcVarforOneSlice(CAlfSlice* pSlice, Pel **imgY_var, Pel *imgY_pad, Int fl, Int img_stride)
#endif
{  
  Int iHeight, iWidth;
  Int ypos, xpos;

#if NONCROSS_TILE_IN_LOOP_FILTERING
  for(Int i=0; i< vpAlfLCU.size(); i++)
  {
    AlfLCUInfo& cAlfLCU = *(vpAlfLCU[i]);
    for(Int j=0; j< cAlfLCU.numSGU; j++)
#else
  for(Int i=0; i< pSlice->getNumLCUs(); i++)
  { 
    CAlfLCU& cAlfLCU = (*pSlice)[i];
    for(Int j=0; j< cAlfLCU.getNumSGU(); j++)
#endif
    {
      ypos    = (Int)cAlfLCU[j].posY;
      xpos    = (Int)cAlfLCU[j].posX;
      iHeight = (Int)cAlfLCU[j].height;
      iWidth  = (Int)cAlfLCU[j].width;
      calcVar(ypos, xpos, imgY_var, imgY_pad, fl, iHeight, iWidth, img_stride);
    }
  }
}
#endif

/** Perform ALF for one chroma region
 * \param vpAlfLCU ALF LCU data container
 * \param pDec to-be-filtered picture buffer
 * \param pRest filtered picture buffer
 * \param stride picture buffer stride
 * \param coeff  filter coefficients
 * \param filtNo filter shape
 * \param chromaFormatShift chroma component size adjustment (1 for 4:2:0)
 */
#if NONCROSS_TILE_IN_LOOP_FILTERING
Void TComAdaptiveLoopFilter::filterChromaRegion(std::vector<AlfLCUInfo*> &vpAlfLCU, Pel* pDec, Pel* pRest, Int stride, Int *coeff, Int filtNo, Int chromaFormatShift)
#else
Void TComAdaptiveLoopFilter::xFilterOneChromaSlice(CAlfSlice* pSlice, Pel* pDec, Pel* pRest, Int stride, Int *coeff, Int filtNo, Int chromaFormatShift)
#endif
{
  Int height, width;
  Int ypos, xpos;

#if NONCROSS_TILE_IN_LOOP_FILTERING
  for(Int i=0; i< vpAlfLCU.size(); i++)
  {
    AlfLCUInfo& cAlfLCU = *(vpAlfLCU[i]);
    for(Int j=0; j< cAlfLCU.numSGU; j++)
#else
  for(Int i=0; i< pSlice->getNumLCUs(); i++)
  { 
    CAlfLCU& cAlfLCU = (*pSlice)[i];
    for(Int j=0; j< cAlfLCU.getNumSGU(); j++)
#endif
    {
      ypos   = (Int)(cAlfLCU[j].posY   >> chromaFormatShift);
      xpos   = (Int)(cAlfLCU[j].posX   >> chromaFormatShift);
      height = (Int)(cAlfLCU[j].height >> chromaFormatShift);
      width  = (Int)(cAlfLCU[j].width  >> chromaFormatShift);

      filterChroma(pRest, pDec, stride, ypos, ypos+ height -1, xpos, xpos+ width-1, filtNo, coeff);
    }
  }
}

#if !NONCROSS_TILE_IN_LOOP_FILTERING
/** Perform ALF for one luma slice
 * \param pSlice slice variables
 * \param pDec picture buffer before filtering
 * \param pRest picture buffer after filtering
 * \param iStride stride size of picture buffer
 * \param pcAlfParam ALF parameters
 */
Void TComAdaptiveLoopFilter::xFilterOneSlice(CAlfSlice* pSlice, Pel* pDec, Pel* pRest, Int iStride, ALFParam* pcAlfParam)
{
  if(pSlice->getCUCtrlEnabled())
  {
    TComPic* pcPic       = pSlice->getPic();
    UInt     uiNumLCU    = pSlice->getNumLCUs();
    UInt     uiSUWidth   = pcPic->getMinCUWidth();
    UInt     uiSUHeight  = pcPic->getMinCUHeight();
    UInt idx, uiStartSU, uiEndSU, uiCurrSU, uiLCUX, uiLCUY, uiLPelX, uiTPelY;
    TComDataCU* pcCU;

    for(idx=0; idx< uiNumLCU; idx++)
    {
      CAlfLCU&    cAlfLCU    = (*pSlice)[idx];

      pcCU                   = cAlfLCU.getCU();
      uiStartSU              = cAlfLCU.getStartSU();
      uiEndSU                = cAlfLCU.getEndSU();
      uiLCUX                 = pcCU->getCUPelX();
      uiLCUY                 = pcCU->getCUPelY();

      for(uiCurrSU= uiStartSU; uiCurrSU<= uiEndSU; uiCurrSU++)
      {
        uiLPelX   = uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiCurrSU] ];
        uiTPelY   = uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiCurrSU] ];
        if( !( uiLPelX < m_img_width )  || !( uiTPelY < m_img_height )  )
        {
          continue;
        }
        if(pcCU->getAlfCtrlFlag(uiCurrSU))
        {
          filterLuma(pRest, pDec, iStride, uiTPelY, uiTPelY+ uiSUHeight-1, uiLPelX, uiLPelX+ uiSUWidth-1,  pcAlfParam->filter_shape, m_filterCoeffSym, m_varIndTab, m_varImg);
        }
      }
    }
  }  
  else
  {
    UInt uiNumLCU = pSlice->getNumLCUs();
    Int  iTPelY,  iLPelX;
    Int  iWidth, iHeight;

    for(UInt idx=0; idx< uiNumLCU; idx++)
    {
      CAlfLCU& cAlfLCU = (*pSlice)[idx];
      for(UInt i=0; i< cAlfLCU.getNumSGU(); i++)
      {
        iTPelY = (Int)cAlfLCU[i].posY;
        iLPelX = (Int)cAlfLCU[i].posX;
        iHeight= (Int)cAlfLCU[i].height;
        iWidth = (Int)cAlfLCU[i].width;
        filterLuma(pRest, pDec, iStride, iTPelY, iTPelY+ iHeight-1, iLPelX, iLPelX+ iWidth-1,  pcAlfParam->filter_shape, m_filterCoeffSym, m_varIndTab, m_varImg);
      }
    }
  }
}
#endif
/** Copy ALF CU control flags from ALF parameters for slices
 * \param [in] vAlfParamSlices ALF CU control parameters
 */
Void TComAdaptiveLoopFilter::transferCtrlFlagsFromAlfParam(std::vector<AlfCUCtrlInfo>& vAlfParamSlices)
{
  assert(m_uiNumSlicesInPic == vAlfParamSlices.size());

  for(UInt s=0; s< m_uiNumSlicesInPic; s++)
  {
    AlfCUCtrlInfo& rAlfParam = vAlfParamSlices[s];
#if NONCROSS_TILE_IN_LOOP_FILTERING
    transferCtrlFlagsFromAlfParamOneSlice( m_pvpAlfLCU[s], 
#else
    transferCtrlFlagsFromAlfParamOneSlice(s, 
#endif
      (rAlfParam.cu_control_flag ==1)?true:false, 
      rAlfParam.alf_max_depth, 
      rAlfParam.alf_cu_flag
      );
  }
}
/** Copy ALF CU control flags from ALF CU control parameters for one slice
 * \param [in] vpAlfLCU ALF LCU data container 
 * \param [in] bCUCtrlEnabled true for ALF CU control enabled
 * \param [in] iAlfDepth ALF CU control depth
 * \param [in] vCtrlFlags ALF CU control flags
 */
#if NONCROSS_TILE_IN_LOOP_FILTERING
Void TComAdaptiveLoopFilter::transferCtrlFlagsFromAlfParamOneSlice(std::vector< AlfLCUInfo* > &vpAlfLCU, Bool bCUCtrlEnabled, Int iAlfDepth, std::vector<UInt>& vCtrlFlags)
#else
Void TComAdaptiveLoopFilter::transferCtrlFlagsFromAlfParamOneSlice(UInt s, Bool bCUCtrlEnabled, Int iAlfDepth, std::vector<UInt>& vCtrlFlags)
#endif
{
#if !NONCROSS_TILE_IN_LOOP_FILTERING
  CAlfSlice& cSlice   = m_pSlice[s];
#endif

#if NONCROSS_TILE_IN_LOOP_FILTERING
  if(!bCUCtrlEnabled)
  {
    for(Int idx=0; idx< vpAlfLCU.size(); idx++)
    {
      AlfLCUInfo& cAlfLCU = *(vpAlfLCU[idx]);
      if( cAlfLCU.bAllSUsInLCUInSameSlice)
      {
        cAlfLCU.pcCU->setAlfCtrlFlagSubParts(1, 0, 0);
      }
      else
      {
        for(UInt uiCurrSU= cAlfLCU.startSU; uiCurrSU<= cAlfLCU.endSU; uiCurrSU++)
        {
          cAlfLCU.pcCU->setAlfCtrlFlag(uiCurrSU, 1);
        }
      }
    }
    return;
  }
#else
  cSlice.setCUCtrlEnabled(bCUCtrlEnabled);
  if(!bCUCtrlEnabled)
  {
    cSlice.setCUCtrlDepth(-1);
    return;
  }

  cSlice.setCUCtrlDepth(iAlfDepth);
#endif

  UInt uiNumCtrlFlags = 0;
#if NONCROSS_TILE_IN_LOOP_FILTERING
  for(Int idx=0; idx< vpAlfLCU.size(); idx++)
  {
    AlfLCUInfo& cAlfLCU = *(vpAlfLCU[idx]);
#else
  for(UInt idx=0; idx< cSlice.getNumLCUs(); idx++)
  {
    CAlfLCU& cAlfLCU = cSlice[idx];
#endif

#if NONCROSS_TILE_IN_LOOP_FILTERING
    uiNumCtrlFlags += (UInt)getCtrlFlagsFromAlfParam(&cAlfLCU, iAlfDepth, &(vCtrlFlags[uiNumCtrlFlags]) );
#else
    cAlfLCU.getCtrlFlagsFromAlfParam(iAlfDepth, &(vCtrlFlags[uiNumCtrlFlags]) );
    uiNumCtrlFlags += cAlfLCU.getNumCtrlFlags();
#endif
  }
#if !NONCROSS_TILE_IN_LOOP_FILTERING
  cSlice.setNumCtrlFlags(uiNumCtrlFlags);
#endif
}

#if NONCROSS_TILE_IN_LOOP_FILTERING
/** Copy region pixels
 * \param vpAlfLCU ALF LCU data container
 * \param pPicDst destination picture buffer
 * \param pPicSrc source picture buffer
 * \param stride stride size of picture buffer
 * \param formatShift region size adjustment according to component size
 */
Void TComAdaptiveLoopFilter::copyRegion(std::vector< AlfLCUInfo* > &vpAlfLCU, Pel* pPicDst, Pel* pPicSrc, Int stride, Int formatShift)
{
#if G212_CROSS9x9_VB
  Int extSize = 4;
#else
  Int extSize = 5;
#endif
  Int posX, posY, width, height, offset;
  Pel *pPelDst, *pPelSrc;
  
  for(Int idx =0; idx < vpAlfLCU.size(); idx++)
  {
    AlfLCUInfo& cAlfLCU = *(vpAlfLCU[idx]);
    for(Int n=0; n < cAlfLCU.numSGU; n++)
    {
      NDBFBlockInfo& rSGU = cAlfLCU[n];

      posX     = (Int)(rSGU.posX   >> formatShift);
      posY     = (Int)(rSGU.posY   >> formatShift);
      width    = (Int)(rSGU.width  >> formatShift);
      height   = (Int)(rSGU.height >> formatShift);
      offset   = ( (posY- extSize) * stride)+ (posX -extSize);
      pPelDst  = pPicDst + offset;    
      pPelSrc  = pPicSrc + offset;    

      for(Int j=0; j< (height + (extSize<<1)); j++)
      {
        ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*(width + (extSize<<1)));
        pPelDst += stride;
        pPelSrc += stride;
      }
    }
  }
}

/** Extend region boundary 
 * \param [in] vpAlfLCU ALF LCU data container
 * \param [in,out] pPelSrc picture buffer
 * \param [in] stride stride size of picture buffer
 * \param [in] formatShift region size adjustment according to component size
 */
Void TComAdaptiveLoopFilter::extendRegionBorder(std::vector< AlfLCUInfo* > &vpAlfLCU, Pel* pPelSrc, Int stride, Int formatShift)
{
#if G212_CROSS9x9_VB
  UInt extSize = 4;
#else
  UInt extSize = 5;
#endif
  UInt width, height;
  UInt posX, posY;
  Pel* pPel;
  Bool* pbAvail;
  for(Int idx = 0; idx < vpAlfLCU.size(); idx++)
  {
    AlfLCUInfo& rAlfLCU = *(vpAlfLCU[idx]);
    for(Int n =0; n < rAlfLCU.numSGU; n++)
    {
      NDBFBlockInfo& rSGU = rAlfLCU[n];

      posX     = rSGU.posX >> formatShift;
      posY     = rSGU.posY >> formatShift;
      width    = rSGU.width >> formatShift;
      height   = rSGU.height >> formatShift;
      pbAvail  = rSGU.isBorderAvailable;    
      pPel     = pPelSrc + (posY * stride)+ posX;    
      extendBorderCoreFunction(pPel, stride, pbAvail, width, height, extSize);
    }
  }
}

/** Core function for extending slice/tile boundary 
 * \param [in, out] pPel processing block pointer
 * \param [in] stride picture buffer stride
 * \param [in] pbAvail neighboring blocks availabilities
 * \param [in] width block width
 * \param [in] height block height
 * \param [in] extSize boundary extension size
 */
Void TComAdaptiveLoopFilter::extendBorderCoreFunction(Pel* pPel, Int stride, Bool* pbAvail, UInt width, UInt height, UInt extSize)
{
  Pel* pPelDst;
  Pel* pPelSrc;
  Int i, j;

  for(Int pos =0; pos < NUM_SGU_BORDER; pos++)
  {
    if(pbAvail[pos])
    {
      continue;
    }

    switch(pos)
    {
    case SGU_L:
      {
        pPelDst = pPel - extSize;
        pPelSrc = pPel;
        for(j=0; j< height; j++)
        {
          for(i=0; i< extSize; i++)
          {
            pPelDst[i] = *pPelSrc;
          }
          pPelDst += stride;
          pPelSrc += stride;
        }
      }
      break;
    case SGU_R:
      {
        pPelDst = pPel + width;
        pPelSrc = pPelDst -1;
        for(j=0; j< height; j++)
        {
          for(i=0; i< extSize; i++)
          {
            pPelDst[i] = *pPelSrc;
          }
          pPelDst += stride;
          pPelSrc += stride;
        }

      }
      break;
    case SGU_T:
      {
        pPelSrc = pPel;
        pPelDst = pPel - stride;
        for(j=0; j< extSize; j++)
        {
          ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*width);
          pPelDst -= stride;
        }
      }
      break;
    case SGU_B:
      {
        pPelDst = pPel + height*stride;
        pPelSrc = pPelDst - stride;
        for(j=0; j< extSize; j++)
        {
          ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*width);
          pPelDst += stride;
        }

      }
      break;
    case SGU_TL:
      {
        if( (!pbAvail[SGU_T]) && (!pbAvail[SGU_L]))
        {
          pPelSrc = pPel  - extSize;
          pPelDst = pPelSrc - stride;
          for(j=0; j< extSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*extSize);
            pPelDst -= stride;
          }         
        }
      }
      break;
    case SGU_TR:
      {
        if( (!pbAvail[SGU_T]) && (!pbAvail[SGU_R]))
        {
          pPelSrc = pPel + width;
          pPelDst = pPelSrc - stride;
          for(j=0; j< extSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*extSize);
            pPelDst -= stride;
          }

        }

      }
      break;
    case SGU_BL:
      {
        if( (!pbAvail[SGU_B]) && (!pbAvail[SGU_L]))
        {
          pPelDst = pPel + height*stride; pPelDst-= extSize;
          pPelSrc = pPelDst - stride;
          for(j=0; j< extSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*extSize);
            pPelDst += stride;
          }

        }
      }
      break;
    case SGU_BR:
      {
        if( (!pbAvail[SGU_B]) && (!pbAvail[SGU_R]))
        {
          pPelDst = pPel + height*stride; pPelDst += width;
          pPelSrc = pPelDst - stride;
          for(j=0; j< extSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*extSize);
            pPelDst += stride;
          }
        }
      }
      break;
    default:
      {
        printf("Not a legal neighboring availability\n");
        assert(0);
        exit(-1);
      }
    }
  }
}

/** Assign ALF on/off-control flags from ALF parameters to CU data
 * \param [in] pcAlfLCU processing ALF LCU data pointer
 * \param [in] alfDepth ALF on/off-control depth
 * \param [in] pFlags on/off-control flags buffer in ALF parameters
 */
Int TComAdaptiveLoopFilter::getCtrlFlagsFromAlfParam(AlfLCUInfo* pcAlfLCU, Int alfDepth, UInt* pFlags)
{

  const UInt startSU               = pcAlfLCU->startSU;
  const UInt endSU                 = pcAlfLCU->endSU;
  const Bool bAllSUsInLCUInSameSlice = pcAlfLCU->bAllSUsInLCUInSameSlice;
  const UInt maxNumSUInLCU         = m_pcPic->getNumPartInCU();

  TComDataCU* pcCU = pcAlfLCU->pcCU;
  Int   numCUCtrlFlags = 0;

  UInt  currSU, CUDepth, setDepth, ctrlNumSU;
  UInt  alfFlag;

  currSU = startSU;
  if( bAllSUsInLCUInSameSlice ) 
  {
    while(currSU < maxNumSUInLCU)
    {
      //depth of this CU
      CUDepth = pcCU->getDepth(currSU);

      //choose the min. depth for ALF
      setDepth   = (alfDepth < CUDepth)?(alfDepth):(CUDepth);
      ctrlNumSU = maxNumSUInLCU >> (setDepth << 1);

      alfFlag= pFlags[numCUCtrlFlags];

      pcCU->setAlfCtrlFlagSubParts(alfFlag, currSU, (UInt)setDepth);

      numCUCtrlFlags++;
      currSU += ctrlNumSU;
    }
    return numCUCtrlFlags;
  }


  UInt  LCUX    = pcCU->getCUPelX();
  UInt  LCUY    = pcCU->getCUPelY();

  Bool  bFirst, bValidCU;
  UInt  idx, LPelXSU, TPelYSU;

  bFirst= true;
  while(currSU <= endSU)
  {
    //check picture boundary
    while(!( LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[currSU] ] < m_img_width  ) || 
          !( LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[currSU] ] < m_img_height )
      )
    {
      currSU++;
      if(currSU >= maxNumSUInLCU || currSU > endSU)
      {
        break;
      }
    }

    if(currSU >= maxNumSUInLCU || currSU > endSU)
    {
      break;
    }

    //depth of this CU
    CUDepth = pcCU->getDepth(currSU);

    //choose the min. depth for ALF
    setDepth   = (alfDepth < CUDepth)?(alfDepth):(CUDepth);
    ctrlNumSU = maxNumSUInLCU >> (setDepth << 1);

    if(bFirst)
    {
      if(currSU !=0 )
      {
        currSU = ((UInt)(currSU/ctrlNumSU))* ctrlNumSU;
      }
      bFirst = false;
    }

    //alf flag for this CU
    alfFlag= pFlags[numCUCtrlFlags];

    bValidCU = false;
    for(idx = currSU; idx < currSU + ctrlNumSU; idx++)
    {
      if(idx < startSU || idx > endSU)
      {
        continue;
      }

      LPelXSU   = LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[idx] ];
      TPelYSU   = LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[idx] ];

      if( !( LPelXSU < m_img_width )  || !( TPelYSU < m_img_height )  )
      {
        continue;
      }

      bValidCU = true;
      pcCU->setAlfCtrlFlag(idx, alfFlag);
    }

    if(bValidCU)
    {
      numCUCtrlFlags++;
    }

    currSU += ctrlNumSU;
  }

  return numCUCtrlFlags;
}

#else


//-------------- CAlfLCU -----------------//

/** Create ALF LCU unit perform slice processing
 * \param iSliceID slice ID
 * \param pcPic picture parameters
 * \param uiCUAddr LCU raster scan address
 * \param uiStartSU starting SU z-scan address of current LCU unit
 * \param uiEndSU ending SU z-scan address of current LCU unit
 * \param iSGDepth Slice granularity
 */
Void CAlfLCU::create(Int iSliceID, TComPic* pcPic, UInt uiCUAddr, UInt uiStartSU, UInt uiEndSU, Int iSGDepth)
{
  m_iSliceID       = iSliceID;
  m_pcPic          = pcPic;
  m_uiCUAddr       = uiCUAddr;
  m_pcCU           = pcPic->getCU(m_uiCUAddr);
  m_uiStartSU      = uiStartSU;

  UInt uiLCUX      = m_pcCU->getCUPelX();
  UInt uiLCUY      = m_pcCU->getCUPelY();
  UInt uiPicWidth  = m_pcCU->getSlice()->getSPS()->getWidth();
  UInt uiPicHeight = m_pcCU->getSlice()->getSPS()->getHeight();
  UInt uiMaxNumSUInLCUWidth = m_pcPic->getNumPartInWidth();
  UInt uiMAxNumSUInLCUHeight= m_pcPic->getNumPartInHeight();
  UInt uiMaxNumSUInLCU      = uiMaxNumSUInLCUWidth*uiMAxNumSUInLCUHeight;
  UInt uiMaxNumSUInSGU      = uiMaxNumSUInLCU >> (iSGDepth << 1);
  UInt uiCurrSU, uiLPelX, uiTPelY;

  //create CU ctrl flag buffer
  m_puiCUCtrlFlag = new UInt[uiMaxNumSUInLCU];
  ::memset(m_puiCUCtrlFlag, 0, sizeof(UInt)*uiMaxNumSUInLCU);
  m_iNumCUCtrlFlags = -1;

  //find number of SGU
  uiCurrSU   = m_uiStartSU;
  m_uiNumSGU = 0;
  while(uiCurrSU <= uiEndSU)
  {
    uiLPelX = uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiCurrSU] ];
    uiTPelY = uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiCurrSU] ];

    if(( uiLPelX < uiPicWidth ) && ( uiTPelY < uiPicHeight ))
    {
      m_uiNumSGU ++;
    }

    uiCurrSU += uiMaxNumSUInSGU;
  }

  m_pSGU= new AlfSGUInfo[m_uiNumSGU];       

  //initialize SGU parameters
  uiCurrSU   = m_uiStartSU;
  UInt uiSGUID = 0;
  Int* piCUSliceMap = m_pcCU->getSliceSUMap();
  while(uiCurrSU <= uiEndSU)
  {
    uiLPelX = uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiCurrSU] ];
    uiTPelY = uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiCurrSU] ];

    while(!( uiLPelX < uiPicWidth ) || !( uiTPelY < uiPicHeight ))
    {
      uiCurrSU += uiMaxNumSUInSGU;
      if(uiCurrSU >= uiMaxNumSUInLCU || uiCurrSU > uiEndSU)
      {
        break;
      }
      uiLPelX = uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiCurrSU] ];
      uiTPelY = uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiCurrSU] ];
    }

    if(uiCurrSU >= uiMaxNumSUInLCU || uiCurrSU > uiEndSU)
    {
      break;
    }

    AlfSGUInfo& rSGU = m_pSGU[uiSGUID];
    rSGU.sliceID = m_iSliceID;
    rSGU.posY    = uiTPelY;
    rSGU.posX    = uiLPelX;
    rSGU.startSU = uiCurrSU;
    UInt uiLastValidSU  = uiCurrSU;
    UInt uiIdx, uiLPelX_su, uiTPelY_su;
    for(uiIdx = uiCurrSU; uiIdx < uiCurrSU + uiMaxNumSUInSGU; uiIdx++)
    {
      if(uiIdx > uiEndSU)
      {
        break;        
      }
      uiLPelX_su   = uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY_su   = uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];
      if( !(uiLPelX_su < uiPicWidth ) || !( uiTPelY_su < uiPicHeight ))
      {
        continue;
      }
      piCUSliceMap[uiIdx] = m_iSliceID;
      uiLastValidSU = uiIdx;
    }
    rSGU.endSU = uiLastValidSU;

    UInt rTLSU = g_auiZscanToRaster[ rSGU.startSU ];
    UInt rBRSU = g_auiZscanToRaster[ rSGU.endSU   ];
    rSGU.widthSU  = (rBRSU % uiMaxNumSUInLCUWidth) - (rTLSU % uiMaxNumSUInLCUWidth)+ 1;
    rSGU.heightSU = (UInt)(rBRSU / uiMaxNumSUInLCUWidth) - (UInt)(rTLSU / uiMaxNumSUInLCUWidth)+ 1;
    rSGU.width    = rSGU.widthSU  * m_pcPic->getMinCUWidth();
    rSGU.height   = rSGU.heightSU * m_pcPic->getMinCUHeight();

    uiCurrSU += uiMaxNumSUInSGU;
    uiSGUID ++;
  }
  assert(uiSGUID == m_uiNumSGU);

  m_uiEndSU = m_pSGU[m_uiNumSGU-1].endSU;
}


/** Destroy ALF LCU unit
 */
Void CAlfLCU::destroy()
{
  if(m_pSGU != NULL)
  {
    delete[] m_pSGU; 
    m_pSGU = NULL;
  }
  if(m_puiCUCtrlFlag != NULL)
  {
    delete[] m_puiCUCtrlFlag;
    m_puiCUCtrlFlag = NULL;
  }
}


/** Set the neighboring availabilities for one slice granularity unit
 * \param uiNumLCUInPicWidth number of LCUs in picture width
 * \param uiNumLCUInPicHeight number of LCUs in picture height
 * \param uiNumSUInLCUWidth max. number of SUs in one LCU
 * \param uiNumSUInLCUHeight max. number of SUs in one LCU
 * \param piSliceIDMap slice ID map (picture)
 */
Void CAlfLCU::setSGUBorderAvailability(UInt uiNumLCUInPicWidth, UInt uiNumLCUInPicHeight, UInt uiNumSUInLCUWidth, UInt uiNumSUInLCUHeight,Int* piSliceIDMap)
{
  UInt uiPicWidth  = m_pcCU->getSlice()->getSPS()->getWidth();
  UInt uiPicHeight = m_pcCU->getSlice()->getSPS()->getHeight();
  UInt uiNumSUInLCU = uiNumSUInLCUWidth*uiNumSUInLCUHeight;
  UInt uiLCUOffset  = m_uiCUAddr*uiNumSUInLCU;
  Int* piSliceIDMapLCU = piSliceIDMap + uiLCUOffset;

  UInt uiLPelX, uiTPelY;
  UInt uiWidth, uiHeight;
  Bool bPicRBoundary, bPicBBoundary, bPicTBoundary, bPicLBoundary;
  Bool bLCURBoundary= false, bLCUBBoundary= false, bLCUTBoundary= false, bLCULBoundary= false;

  Bool* pbAvailBorder;
  Bool* pbAvail;
  UInt rTLSU, rBRSU, uiWidthSU, uiHeightSU;
  UInt zRefSU;
  Int* piRefID;
  Int* piRefMapLCU;

  UInt rTRefSU= 0, rBRefSU= 0, rLRefSU= 0, rRRefSU= 0;
  Int* piRRefMapLCU= NULL;
  Int* piLRefMapLCU= NULL;
  Int* piTRefMapLCU= NULL;
  Int* piBRefMapLCU= NULL;

  for(Int i=0; i< m_uiNumSGU; i++)
  {
    AlfSGUInfo& rSGU = m_pSGU[i];
    uiLPelX = rSGU.posX;
    uiTPelY = rSGU.posY;
    uiWidth = rSGU.width;
    uiHeight= rSGU.height;

    rTLSU     = g_auiZscanToRaster[ rSGU.startSU ];
    rBRSU     = g_auiZscanToRaster[ rSGU.endSU   ];
    uiWidthSU = rSGU.widthSU;
    uiHeightSU= rSGU.heightSU;

    pbAvailBorder = rSGU.isBorderAvailable;

    bPicTBoundary= (uiTPelY == 0                       )?(true):(false);
    bPicLBoundary= (uiLPelX == 0                       )?(true):(false);
    bPicRBoundary= (!(uiLPelX+ uiWidth < uiPicWidth )  )?(true):(false);
    bPicBBoundary= (!(uiTPelY + uiHeight < uiPicHeight))?(true):(false);

    //       SGU_L
    pbAvail = &(pbAvailBorder[SGU_L]);
    if(bPicLBoundary)
    {
      *pbAvail = false;
    }
    else
    {
      bLCULBoundary = (rTLSU % uiNumSUInLCUWidth == 0)?(true):(false);
      if(bLCULBoundary)
      {
        rLRefSU     = rTLSU + uiNumSUInLCUWidth -1;
        zRefSU      = g_auiRasterToZscan[rLRefSU];
        piRefMapLCU = piLRefMapLCU= (piSliceIDMapLCU - uiNumSUInLCU);
      }
      else
      {
        zRefSU   = g_auiRasterToZscan[rTLSU - 1];
        piRefMapLCU  = piSliceIDMapLCU;
      }
      piRefID = piRefMapLCU + zRefSU;
      *pbAvail = (*piRefID == m_iSliceID)?(true):(false);
    }

    //       SGU_R
    pbAvail = &(pbAvailBorder[SGU_R]);
    if(bPicRBoundary)
    {
      *pbAvail = false;
    }
    else
    {
      bLCURBoundary = ( (rTLSU+ uiWidthSU) % uiNumSUInLCUWidth == 0)?(true):(false);
      if(bLCURBoundary)
      {
        rRRefSU      = rTLSU + uiWidthSU - uiNumSUInLCUWidth;
        zRefSU       = g_auiRasterToZscan[rRRefSU];
        piRefMapLCU  = piRRefMapLCU= (piSliceIDMapLCU + uiNumSUInLCU);
      }
      else
      {
        zRefSU       = g_auiRasterToZscan[rTLSU + uiWidthSU];
        piRefMapLCU  = piSliceIDMapLCU;
      }
      piRefID = piRefMapLCU + zRefSU;
      *pbAvail = (*piRefID == m_iSliceID)?(true):(false);
    }

    //       SGU_T
    pbAvail = &(pbAvailBorder[SGU_T]);
    if(bPicTBoundary)
    {
      *pbAvail = false;
    }
    else
    {
      bLCUTBoundary = ( (UInt)(rTLSU / uiNumSUInLCUWidth)== 0)?(true):(false);
      if(bLCUTBoundary)
      {
        rTRefSU      = uiNumSUInLCU - (uiNumSUInLCUWidth - rTLSU);
        zRefSU       = g_auiRasterToZscan[rTRefSU];
        piRefMapLCU  = piTRefMapLCU= (piSliceIDMapLCU - (uiNumLCUInPicWidth*uiNumSUInLCU));
      }
      else
      {
        zRefSU       = g_auiRasterToZscan[rTLSU - uiNumSUInLCUWidth];
        piRefMapLCU  = piSliceIDMapLCU;
      }
      piRefID = piRefMapLCU + zRefSU;
      *pbAvail = (*piRefID == m_iSliceID)?(true):(false);
    }

    //       SGU_B
    pbAvail = &(pbAvailBorder[SGU_B]);
    if(bPicBBoundary)
    {
      *pbAvail = false;
    }
    else
    {
      bLCUBBoundary = ( (UInt)(rBRSU / uiNumSUInLCUWidth) == (uiNumSUInLCUHeight-1) )?(true):(false);
      if(bLCUBBoundary)
      {
        rBRefSU      = rTLSU % uiNumSUInLCUWidth;
        zRefSU       = g_auiRasterToZscan[rBRefSU];
        piRefMapLCU  = piBRefMapLCU= (piSliceIDMapLCU + (uiNumLCUInPicWidth*uiNumSUInLCU));
      }
      else
      {
        zRefSU       = g_auiRasterToZscan[rTLSU + (uiHeightSU*uiNumSUInLCUWidth)];
        piRefMapLCU  = piSliceIDMapLCU;
      }
      piRefID = piRefMapLCU + zRefSU;
      *pbAvail = (*piRefID == m_iSliceID)?(true):(false);
    }

    //       SGU_TL
    pbAvail = &(pbAvailBorder[SGU_TL]);
    if(bPicTBoundary || bPicLBoundary)
    {
      *pbAvail = false;
    }
    else
    {
      if(bLCUTBoundary && bLCULBoundary)
      {
        zRefSU       = uiNumSUInLCU -1;
        piRefMapLCU  = piSliceIDMapLCU - ( (uiNumLCUInPicWidth+1)*uiNumSUInLCU);
      }
      else if(bLCUTBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rTRefSU- 1];
        piRefMapLCU  = piTRefMapLCU;
      }
      else if(bLCULBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rLRefSU- uiNumSUInLCUWidth ];
        piRefMapLCU  = piLRefMapLCU;
      }
      else //inside LCU
      {
        zRefSU       = g_auiRasterToZscan[ rTLSU - uiNumSUInLCUWidth -1];
        piRefMapLCU  = piSliceIDMapLCU;
      }
      piRefID = piRefMapLCU + zRefSU;
      *pbAvail = (*piRefID == m_iSliceID)?(true):(false);
    }

    //       SGU_TR
    pbAvail = &(pbAvailBorder[SGU_TR]);
    if(bPicTBoundary || bPicRBoundary)
    {
      *pbAvail = false;
    }
    else
    {
      if(bLCUTBoundary && bLCURBoundary)
      {
        zRefSU      = g_auiRasterToZscan[uiNumSUInLCU - uiNumSUInLCUWidth];
        piRefMapLCU  = piSliceIDMapLCU - ( (uiNumLCUInPicWidth-1)*uiNumSUInLCU);        
      }
      else if(bLCUTBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rTRefSU+ uiWidthSU];
        piRefMapLCU  = piTRefMapLCU;
      }
      else if(bLCURBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rRRefSU- uiNumSUInLCUWidth ];
        piRefMapLCU  = piRRefMapLCU;
      }
      else //inside LCU
      {
        zRefSU       = g_auiRasterToZscan[ rTLSU - uiNumSUInLCUWidth +uiWidthSU];
        piRefMapLCU  = piSliceIDMapLCU;
      }
      piRefID = piRefMapLCU + zRefSU;
      *pbAvail = (*piRefID == m_iSliceID)?(true):(false);
    }

    //       SGU_BL
    pbAvail = &(pbAvailBorder[SGU_BL]);
    if(bPicBBoundary || bPicLBoundary)
    {
      *pbAvail = false;
    }
    else
    {
      if(bLCUBBoundary && bLCULBoundary)
      {
        zRefSU      = g_auiRasterToZscan[uiNumSUInLCUWidth - 1];
        piRefMapLCU  = piSliceIDMapLCU + ( (uiNumLCUInPicWidth-1)*uiNumSUInLCU);        
      }
      else if(bLCUBBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rBRefSU - 1];
        piRefMapLCU  = piBRefMapLCU;
      }
      else if(bLCULBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rLRefSU+ uiHeightSU*uiNumSUInLCUWidth ];
        piRefMapLCU  = piLRefMapLCU;
      }
      else //inside LCU
      {
        zRefSU       = g_auiRasterToZscan[ rTLSU + uiHeightSU*uiNumSUInLCUWidth -1];
        piRefMapLCU  = piSliceIDMapLCU;
      }
      piRefID = piRefMapLCU + zRefSU;
      *pbAvail = (*piRefID == m_iSliceID)?(true):(false);
    }

    //       SGU_BR
    pbAvail = &(pbAvailBorder[SGU_BR]);
    if(bPicBBoundary || bPicRBoundary)
    {
      *pbAvail = false;
    }
    else
    {
      if(bLCUBBoundary && bLCURBoundary)
      {
        zRefSU = 0;
        piRefMapLCU = piSliceIDMapLCU+ ( (uiNumLCUInPicWidth+1)*uiNumSUInLCU);
      }
      else if(bLCUBBoundary)
      {
        zRefSU      = g_auiRasterToZscan[ rBRefSU + uiWidthSU];
        piRefMapLCU = piBRefMapLCU;
      }
      else if(bLCURBoundary)
      {
        zRefSU      = g_auiRasterToZscan[ rRRefSU + (uiHeightSU*uiNumSUInLCUWidth)];
        piRefMapLCU = piRRefMapLCU;
      }
      else //inside LCU
      {
        zRefSU      = g_auiRasterToZscan[ rTLSU + (uiHeightSU*uiNumSUInLCUWidth)+ uiWidthSU];
        piRefMapLCU = piSliceIDMapLCU;
      }
      piRefID = piRefMapLCU + zRefSU;
      *pbAvail = (*piRefID == m_iSliceID)?(true):(false);
    }
  }

}


/** Extend slice boundary border
 * \param [in,out] pPel starting pixel position in picture buffer
 * \param [in] iStride stride size of picture buffer
 * \param [in] pbAvail neighboring availabilities for current processing block
 * \param [in] uiWidth pixel width of current processing block
 * \param [in] uiHeight pixel height of current processing block
 * \param [in] uiExtSize extension size
 */
Void CAlfLCU::extendBorderCoreFunction(Pel* pPel, Int iStride, Bool* pbAvail, UInt uiWidth, UInt uiHeight, UInt uiExtSize)
{
  Pel* pPelDst;
  Pel* pPelSrc;
  Int i, j;

  for(Int pos =0; pos < NUM_SGU_BORDER; pos++)
  {
    if(pbAvail[pos])
    {
      continue;
    }

    switch(pos)
    {
    case SGU_L:
      {
        pPelDst = pPel - uiExtSize;
        pPelSrc = pPel;
        for(j=0; j< uiHeight; j++)
        {
          for(i=0; i< uiExtSize; i++)
          {
            pPelDst[i] = *pPelSrc;
          }
          pPelDst += iStride;
          pPelSrc += iStride;
        }
      }
      break;
    case SGU_R:
      {
        pPelDst = pPel + uiWidth;
        pPelSrc = pPelDst -1;
        for(j=0; j< uiHeight; j++)
        {
          for(i=0; i< uiExtSize; i++)
          {
            pPelDst[i] = *pPelSrc;
          }
          pPelDst += iStride;
          pPelSrc += iStride;
        }

      }
      break;
    case SGU_T:
      {
        pPelSrc = pPel;
        pPelDst = pPel - iStride;
        for(j=0; j< uiExtSize; j++)
        {
          ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*uiWidth);
          pPelDst -= iStride;
        }
      }
      break;
    case SGU_B:
      {
        pPelDst = pPel + uiHeight*iStride;
        pPelSrc = pPelDst - iStride;
        for(j=0; j< uiExtSize; j++)
        {
          ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*uiWidth);
          pPelDst += iStride;
        }
      }
      break;
    case SGU_TL:
      {
        if( (!pbAvail[SGU_T]) && (!pbAvail[SGU_L]))
        {
          pPelSrc = pPel  - uiExtSize;
          pPelDst = pPelSrc - iStride;
          for(j=0; j< uiExtSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*uiExtSize);
            pPelDst -= iStride;
          }         
        }
      }
      break;
    case SGU_TR:
      {
        if( (!pbAvail[SGU_T]) && (!pbAvail[SGU_R]))
        {
          pPelSrc = pPel + uiWidth;
          pPelDst = pPelSrc - iStride;
          for(j=0; j< uiExtSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*uiExtSize);
            pPelDst -= iStride;
          }
        }

      }
      break;
    case SGU_BL:
      {
        if( (!pbAvail[SGU_B]) && (!pbAvail[SGU_L]))
        {
          pPelDst = pPel + uiHeight*iStride; pPelDst-= uiExtSize;
          pPelSrc = pPelDst - iStride;
          for(j=0; j< uiExtSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*uiExtSize);
            pPelDst += iStride;
          }
        }
      }
      break;
    case SGU_BR:
      {
        if( (!pbAvail[SGU_B]) && (!pbAvail[SGU_R]))
        {
          pPelDst = pPel + uiHeight*iStride; pPelDst += uiWidth;
          pPelSrc = pPelDst - iStride;
          for(j=0; j< uiExtSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*uiExtSize);
            pPelDst += iStride;
          }
        }
      }
      break;
    default:
      {
        printf("Not a legal neighboring availability\n");
        assert(0);
        exit(-1);
      }

    }

  }

}


/** Extend slice boundary border for one luma LCU
 * \param [in, out] pImg picture buffer
 * \param [in] iStride stride size of picture buffer
 */
Void CAlfLCU::extendLumaBorder(Pel* pImg, Int iStride)
{
#if G212_CROSS9x9_VB
  UInt uiExtSize = 4;
#else
  UInt uiExtSize = 5;
#endif
  UInt uiWidth, uiHeight;
  UInt posX, posY;
  Pel* pPel;
  Bool* pbAvail;

  for(Int n =0; n < m_uiNumSGU; n++)
  {
    AlfSGUInfo& rSGU = m_pSGU[n];
    posX     = rSGU.posX;
    posY     = rSGU.posY;
    uiWidth  = rSGU.width;
    uiHeight = rSGU.height;
    pbAvail  = rSGU.isBorderAvailable;    
    pPel     = pImg + (posY * iStride)+ posX;    
    extendBorderCoreFunction(pPel, iStride, pbAvail, uiWidth, uiHeight, uiExtSize);
  }
}


/** Extend slice boundary border for one chroma LCU
* \param [in, out] pImg picture buffer
* \param [in] iStride stride size of picture buffer
 */
Void CAlfLCU::extendChromaBorder(Pel* pImg, Int iStride)
{
#if G212_CROSS9x9_VB
  UInt uiExtSize = 4;
#else
  UInt uiExtSize = 5;
#endif
  UInt uiWidth, uiHeight;
  UInt posX, posY;
  Pel* pPel;
  Bool* pbAvail;

  for(Int n =0; n < m_uiNumSGU; n++)
  {
    AlfSGUInfo& rSGU = m_pSGU[n];
    posX     = rSGU.posX >> 1;
    posY     = rSGU.posY >> 1;
    uiWidth  = rSGU.width >> 1;
    uiHeight = rSGU.height >> 1;
    pbAvail  = rSGU.isBorderAvailable;    
    pPel     = pImg + (posY * iStride)+ posX;    
    extendBorderCoreFunction(pPel, iStride, pbAvail, uiWidth, uiHeight, uiExtSize);
  }
}


/** Copy one luma LCU
 * \param pImgDst destination picture buffer
 * \param pImgSrc souce picture buffer
 * \param iStride stride size of picture buffer
 */
Void CAlfLCU::copyLuma(Pel* pImgDst, Pel* pImgSrc, Int iStride)
{
  UInt uiWidth, uiHeight;
  UInt posX, posY;
  UInt uiOffset;
  Pel* pPelSrc;
  Pel* pPelDst;

  for(Int n =0; n < m_uiNumSGU; n++)
  {
    AlfSGUInfo& rSGU = m_pSGU[n];
    posX     = rSGU.posX;
    posY     = rSGU.posY;
    uiWidth  = rSGU.width;
    uiHeight = rSGU.height;
    uiOffset = (posY * iStride)+ posX;

    pPelDst   = pImgDst + uiOffset;    
    pPelSrc   = pImgSrc + uiOffset;    

    for(Int j=0; j< uiHeight; j++)
    {
      ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*uiWidth);
      pPelDst += iStride;
      pPelSrc += iStride;
    }

  }
}

/** Copy one chroma LCU
 * \param pImgDst destination picture buffer
 * \param pImgSrc souce picture buffer
 * \param iStride stride size of picture buffer
 */
Void CAlfLCU::copyChroma(Pel* pImgDst, Pel* pImgSrc, Int iStride)
{
  UInt uiWidth, uiHeight;
  UInt posX, posY;
  UInt uiOffset;
  Pel* pPelSrc;
  Pel* pPelDst;

  for(Int n =0; n < m_uiNumSGU; n++)
  {
    AlfSGUInfo& rSGU = m_pSGU[n];
    posX     = rSGU.posX >>1;
    posY     = rSGU.posY >>1;
    uiWidth  = rSGU.width >>1;
    uiHeight = rSGU.height >>1;
    uiOffset = (posY * iStride)+ posX;

    pPelDst   = pImgDst + uiOffset;    
    pPelSrc   = pImgSrc + uiOffset;    

    for(Int j=0; j< uiHeight; j++)
    {
      ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*uiWidth);
      pPelDst += iStride;
      pPelSrc += iStride;
    }
  }
}


/** Copy ALF CU control flags from ALF parameters
 * \param iAlfDepth ALF CU control depth
 * \param puiFlags ALF CU control flags
 */
Void CAlfLCU::getCtrlFlagsFromAlfParam(Int iAlfDepth, UInt* puiFlags)
{
  UInt  uiMaxNumSUInLCU     = m_pcPic->getNumPartInCU();
  Bool  bAllSUsInSameSlice  = (m_uiStartSU == 0)&&( m_uiEndSU == uiMaxNumSUInLCU -1);

  UInt  uiCurrSU, iCUDepth, iSetDepth, uiCtrlNumSU;
  UInt  uiAlfFlag;

  uiCurrSU = m_uiStartSU;
  m_iNumCUCtrlFlags = 0;

  if(bAllSUsInSameSlice) 
  {
    while(uiCurrSU < uiMaxNumSUInLCU)
    {
      //depth of this CU
      iCUDepth = m_pcCU->getDepth(uiCurrSU);

      //choose the min. depth for ALF
      iSetDepth   = (iAlfDepth < iCUDepth)?(iAlfDepth):(iCUDepth);
      uiCtrlNumSU = uiMaxNumSUInLCU >> (iSetDepth << 1);

      uiAlfFlag= puiFlags[m_iNumCUCtrlFlags];

      m_pcCU->setAlfCtrlFlagSubParts(uiAlfFlag, uiCurrSU, (UInt)iSetDepth);

      m_iNumCUCtrlFlags++;
      uiCurrSU += uiCtrlNumSU;
    }
    ::memcpy(m_puiCUCtrlFlag, puiFlags, sizeof(UInt)*m_iNumCUCtrlFlags);
    return;
  }


  UInt  uiLCUX              = m_pcCU->getCUPelX();
  UInt  uiLCUY              = m_pcCU->getCUPelY();
  UInt  uiPicWidth          = m_pcCU->getSlice()->getSPS()->getWidth();
  UInt  uiPicHeight         = m_pcCU->getSlice()->getSPS()->getHeight();

  Bool  bFirst, bValidCU;
  UInt uiIdx, uiLPelX_su, uiTPelY_su;

  bFirst= true;
  while(uiCurrSU <= m_uiEndSU)
  {
    //check picture boundary
    while(!( uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiCurrSU] ] < uiPicWidth  ) || 
          !( uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiCurrSU] ] < uiPicHeight )
         )
    {
      uiCurrSU++;
      if(uiCurrSU >= uiMaxNumSUInLCU || uiCurrSU > m_uiEndSU)
      {
        break;
      }
    }

    if(uiCurrSU >= uiMaxNumSUInLCU || uiCurrSU > m_uiEndSU)
    {
      break;
    }

    //depth of this CU
    iCUDepth = m_pcCU->getDepth(uiCurrSU);

    //choose the min. depth for ALF
    iSetDepth   = (iAlfDepth < iCUDepth)?(iAlfDepth):(iCUDepth);
    uiCtrlNumSU = uiMaxNumSUInLCU >> (iSetDepth << 1);

    if(bFirst)
    {
      if(uiCurrSU !=0 )
      {
        uiCurrSU = ((UInt)(uiCurrSU/uiCtrlNumSU))* uiCtrlNumSU;
      }
      bFirst = false;
    }

    //alf flag for this CU
    uiAlfFlag= puiFlags[m_iNumCUCtrlFlags];

    bValidCU = false;
    for(uiIdx = uiCurrSU; uiIdx < uiCurrSU + uiCtrlNumSU; uiIdx++)
    {
      if(uiIdx < m_uiStartSU || uiIdx > m_uiEndSU)
      {
        continue;
      }

      uiLPelX_su   = uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY_su   = uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];

      if( !( uiLPelX_su < uiPicWidth )  || !( uiTPelY_su < uiPicHeight )  )
      {
        continue;
      }

      bValidCU = true;
      m_pcCU->setAlfCtrlFlag(uiIdx, uiAlfFlag);
    }

    if(bValidCU)
    {
      m_puiCUCtrlFlag[m_iNumCUCtrlFlags] = uiAlfFlag;
      m_iNumCUCtrlFlags++;
    }

    uiCurrSU += uiCtrlNumSU;
  }
}

/** Copy ALF CU control flags
 * \param iAlfDepth ALF CU control depth
 */
Void CAlfLCU::getCtrlFlagsFromCU(Int iAlfDepth)
{
  UInt  uiMaxNumSUInLCU     = m_pcPic->getNumPartInCU();
  Bool  bAllSUsInSameSlice  = (m_uiStartSU == 0)&&( m_uiEndSU == uiMaxNumSUInLCU -1);

  UInt  uiCurrSU, iCUDepth, iSetDepth, uiCtrlNumSU;
  UInt* puiFlag;

  uiCurrSU = m_uiStartSU;
  m_iNumCUCtrlFlags = 0;
  puiFlag = m_puiCUCtrlFlag;

  if(bAllSUsInSameSlice)
  {
    while(uiCurrSU < uiMaxNumSUInLCU)
    {
      //depth of this CU
      iCUDepth = m_pcCU->getDepth(uiCurrSU);

      //choose the min. depth for ALF
      iSetDepth   = (iAlfDepth < iCUDepth)?(iAlfDepth):(iCUDepth);
      uiCtrlNumSU = uiMaxNumSUInLCU >> (iSetDepth << 1);

      *puiFlag = m_pcCU->getAlfCtrlFlag(uiCurrSU); 

      puiFlag++;
      m_iNumCUCtrlFlags++;

      uiCurrSU += uiCtrlNumSU;
    }

    return;
  }


  UInt  uiLCUX              = m_pcCU->getCUPelX();
  UInt  uiLCUY              = m_pcCU->getCUPelY();
  UInt  uiPicWidth          = m_pcCU->getSlice()->getSPS()->getWidth();
  UInt  uiPicHeight         = m_pcCU->getSlice()->getSPS()->getHeight();

  Bool  bFirst, bValidCU;
  UInt uiIdx, uiLPelX_su, uiTPelY_su;

  bFirst= true;
  while(uiCurrSU <= m_uiEndSU)
  {
    //check picture boundary
    while(!( uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiCurrSU] ] < uiPicWidth  ) || 
          !( uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiCurrSU] ] < uiPicHeight )
         )
    {
      uiCurrSU++;

      if(uiCurrSU >= uiMaxNumSUInLCU || uiCurrSU > m_uiEndSU)
      {
        break;
      }
    }

    if(uiCurrSU >= uiMaxNumSUInLCU || uiCurrSU > m_uiEndSU)
    {
      break;
    }

    //depth of this CU
    iCUDepth = m_pcCU->getDepth(uiCurrSU);

    //choose the min. depth for ALF
    iSetDepth   = (iAlfDepth < iCUDepth)?(iAlfDepth):(iCUDepth);
    uiCtrlNumSU = uiMaxNumSUInLCU >> (iSetDepth << 1);

    if(bFirst)
    {
      if(uiCurrSU !=0 )
      {
        uiCurrSU = ((UInt)(uiCurrSU/uiCtrlNumSU))* uiCtrlNumSU;
      }
      bFirst = false;
    }

    bValidCU = false;
    for(uiIdx = uiCurrSU; uiIdx < uiCurrSU + uiCtrlNumSU; uiIdx++)
    {
      if(uiIdx < m_uiStartSU || uiIdx > m_uiEndSU)
      {
        continue;
      }

      uiLPelX_su   = uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY_su   = uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];

      if( !( uiLPelX_su < uiPicWidth )  || !( uiTPelY_su < uiPicHeight )  )
      {
        continue;
      }

      bValidCU = true;
    }

    if(bValidCU)
    {
      *puiFlag = m_pcCU->getAlfCtrlFlag(uiCurrSU); 

      puiFlag++;
      m_iNumCUCtrlFlags++;
    }

    uiCurrSU += uiCtrlNumSU;
  }
}


//



//-------------- CAlfSlice -----------------//

/** Initialize one ALF slice unit
 * \param pcPic picture parameters
 * \param iSGDepth slice granularity
 * \param piSliceSUMap slice ID map
 */
Void CAlfSlice::init(TComPic* pcPic, Int iSGDepth, Int* piSliceSUMap)
{
  m_pcPic          = pcPic;
  m_iSGDepth       = iSGDepth;
  m_piSliceSUMap   = piSliceSUMap;
  m_bCUCtrlEnabled = false;
  m_iCUCtrlDepth   = -1;
  m_bValidSlice    = true;
}

/** Create one ALF slice unit
 * \param iSliceID slice ID
 * \param uiStartAddr starting address of the current processing slice
 * \param uiEndAddr ending address of the current processing slice
 */
Void CAlfSlice::create(Int iSliceID, UInt uiStartAddr, UInt uiEndAddr)
{
  m_iSliceID    = iSliceID;
  UInt uiNumSUInLCUHeight = m_pcPic->getNumPartInHeight();
  UInt uiNumSUInLCUWidth  = m_pcPic->getNumPartInWidth();
  UInt uiNumSUInLCU = uiNumSUInLCUHeight * uiNumSUInLCUWidth;

  //start LCU and SU address
#if TILES
  m_uiStartLCU             = m_pcPic->getPicSym()->getPicSCUAddr(uiStartAddr) / uiNumSUInLCU;
#else
  m_uiStartLCU             = uiStartAddr / uiNumSUInLCU;
#endif
  m_uiFirstCUInStartLCU    = uiStartAddr % uiNumSUInLCU;

  //check if the star SU is out of picture boundary
  UInt uiPicWidth  = m_pcPic->getCU(m_uiStartLCU)->getSlice()->getSPS()->getWidth();
  UInt uiPicHeight = m_pcPic->getCU(m_uiStartLCU)->getSlice()->getSPS()->getHeight();
  UInt uiLCUX      = m_pcPic->getCU(m_uiStartLCU)->getCUPelX();
  UInt uiLCUY      = m_pcPic->getCU(m_uiStartLCU)->getCUPelY();
  UInt uiLPelX     = uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[m_uiFirstCUInStartLCU] ];
  UInt uiTPelY     = uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[m_uiFirstCUInStartLCU] ];
  UInt uiCurrSU    = m_uiFirstCUInStartLCU;
  Bool bMoveToNextLCU = false;

#if TILES
  m_uiStartLCU             = uiStartAddr / uiNumSUInLCU;
#endif

  m_uiEndLCU               = uiEndAddr   / uiNumSUInLCU;
  m_uiLastCUInEndLCU       = uiEndAddr   % uiNumSUInLCU;   
  m_bValidSlice            = true;

  Bool bSliceInOneLCU      = (m_uiStartLCU == m_uiEndLCU);

  while(!( uiLPelX < uiPicWidth ) || !( uiTPelY < uiPicHeight ))
  {
    uiCurrSU ++;

    if(bSliceInOneLCU)
    {
      if(uiCurrSU > m_uiLastCUInEndLCU)
      {
        m_bValidSlice = false;
        break;
      }
    }

    if(uiCurrSU >= uiNumSUInLCU )
    {
      bMoveToNextLCU = true;
      break;
    }

    uiLPelX = uiLCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiCurrSU] ];
    uiTPelY = uiLCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiCurrSU] ];

  }

  if(!m_bValidSlice)
  {
    return;
  }

  if(uiCurrSU != m_uiFirstCUInStartLCU)
  {
    if(!bMoveToNextLCU)
    {
      m_uiFirstCUInStartLCU = uiCurrSU;
    }
    else
    {
      m_uiStartLCU++;
      m_uiFirstCUInStartLCU = 0;
      assert(m_uiStartLCU < m_pcPic->getNumCUsInFrame());
    }

    assert(m_uiStartLCU*uiNumSUInLCU + m_uiFirstCUInStartLCU < uiEndAddr);

  }


  m_uiNumLCUs              = m_uiEndLCU - m_uiStartLCU +1;

  m_pcAlfLCU = new CAlfLCU[m_uiNumLCUs];
  for(UInt uiAddr= m_uiStartLCU; uiAddr <=  m_uiEndLCU; uiAddr++)
  {
    UInt uiStartSU = (uiAddr == m_uiStartLCU)?(m_uiFirstCUInStartLCU):(0);
    UInt uiEndSU   = (uiAddr == m_uiEndLCU  )?(m_uiLastCUInEndLCU   ):(uiNumSUInLCU -1);

#if TILES
    m_pcAlfLCU[uiAddr - m_uiStartLCU].create(m_iSliceID, m_pcPic, m_pcPic->getPicSym()->getCUOrderMap(uiAddr), uiStartSU, uiEndSU, m_iSGDepth);
#else
    m_pcAlfLCU[uiAddr - m_uiStartLCU].create(m_iSliceID, m_pcPic, uiAddr, uiStartSU, uiEndSU, m_iSGDepth);
#endif
  }


  UInt uiNumLCUInPicWidth = m_pcPic->getFrameWidthInCU();
  UInt uiNumLCUInPicHeight= m_pcPic->getFrameHeightInCU();
  for(UInt i= 0; i <  m_uiNumLCUs; i++)
  {
    m_pcAlfLCU[i].setSGUBorderAvailability(uiNumLCUInPicWidth, uiNumLCUInPicHeight,uiNumSUInLCUWidth, uiNumSUInLCUHeight, m_piSliceSUMap);
  }
}

/** Destroy one ALF slice unit
 */
Void CAlfSlice::destroy()
{

  if(m_pcAlfLCU != NULL)
  {
    delete[] m_pcAlfLCU;
    m_pcAlfLCU = NULL;
  }

}

/** Extend slice boundary for one luma slice
 * \param [in,out] pPelSrc picture buffer
 * \param [in] iStride stride size of picture buffer
 */
Void CAlfSlice::extendSliceBorderLuma(Pel* pPelSrc, Int iStride)
{
  for(UInt idx = 0; idx < m_uiNumLCUs; idx++)
  {
    m_pcAlfLCU[idx].extendLumaBorder(pPelSrc, iStride);
  }
}

/** Extend slice boundary for one chroma slice
* \param [in,out] pPelSrc picture buffer
* \param [in] iStride stride size of picture buffer
 */
Void CAlfSlice::extendSliceBorderChroma(Pel* pPelSrc, Int iStride)
{
  for(UInt idx = 0; idx < m_uiNumLCUs; idx++)
  {
    m_pcAlfLCU[idx].extendChromaBorder(pPelSrc, iStride);
  }

}

/** Copy one luma slice
 * \param pPicDst destination picture buffer
 * \param pPicSrc source picture buffer
 * \param iStride stride size of picture buffer
 */
Void CAlfSlice::copySliceLuma(Pel* pPicDst, Pel* pPicSrc, Int iStride )
{
  for(UInt idx = 0; idx < m_uiNumLCUs; idx++)
  {
    m_pcAlfLCU[idx].copyLuma(pPicDst, pPicSrc, iStride);
  }
}

/** Copy one chroma slice
 * \param pPicDst destination picture buffer
 * \param pPicSrc source picture buffer
 * \param iStride stride size of picture buffer
 */
Void CAlfSlice::copySliceChroma(Pel* pPicDst, Pel* pPicSrc, Int iStride )
{

  for(UInt idx = 0; idx < m_uiNumLCUs; idx++)
  {
    m_pcAlfLCU[idx].copyChroma(pPicDst, pPicSrc, iStride);
  }

}

/** Copy ALF CU Control Flags for one slice
 */
Void CAlfSlice::getCtrlFlagsForOneSlice()
{
  assert(m_bCUCtrlEnabled);

  m_iNumCUCtrlFlags = 0;
  for(UInt idx =0; idx < m_uiNumLCUs; idx++)
  {
    m_pcAlfLCU[idx].getCtrlFlagsFromCU(m_iCUCtrlDepth);
    m_iNumCUCtrlFlags += m_pcAlfLCU[idx].getNumCtrlFlags();
  }
}
#endif

#if E192_SPS_PCM_FILTER_DISABLE_SYNTAX
/** PCM LF disable process. 
 * \param pcPic picture (TComPic) pointer
 * \returns Void
 *
 * \note Replace filtered sample values of PCM mode blocks with the transmitted and reconstructed ones.
 */
Void TComAdaptiveLoopFilter::PCMLFDisableProcess (TComPic* pcPic)
{
  xPCMRestoration(pcPic);
}

/** Picture-level PCM restoration. 
 * \param pcPic picture (TComPic) pointer
 * \returns Void
 */
Void TComAdaptiveLoopFilter::xPCMRestoration(TComPic* pcPic)
{
#if MAX_PCM_SIZE
  Bool  bPCMFilter = (pcPic->getSlice(0)->getSPS()->getUsePCM() && pcPic->getSlice(0)->getSPS()->getPCMFilterDisableFlag())? true : false;
#else
  Bool  bPCMFilter = (pcPic->getSlice(0)->getSPS()->getPCMFilterDisableFlag() && ((1<<pcPic->getSlice(0)->getSPS()->getPCMLog2MinSize()) <= g_uiMaxCUWidth))? true : false;
#endif

  if(bPCMFilter)
  {
    for( UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame() ; uiCUAddr++ )
    {
      TComDataCU* pcCU = pcPic->getCU(uiCUAddr);

      xPCMCURestoration(pcCU, 0, 0); 
    } 
  }
}

/** PCM CU restoration. 
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx part index
 * \param uiDepth CU depth
 * \returns Void
 */
Void TComAdaptiveLoopFilter::xPCMCURestoration ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth )
{
  TComPic* pcPic     = pcCU->getPic();
  UInt uiCurNumParts = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiQNumParts   = uiCurNumParts>>2;

  // go to sub-CU
  if( pcCU->getDepth(uiAbsZorderIdx) > uiDepth )
  {
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx+=uiQNumParts )
    {
      UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
        xPCMCURestoration( pcCU, uiAbsZorderIdx, uiDepth+1 );
    }
    return;
  }

  // restore PCM samples
  if (pcCU->getIPCMFlag(uiAbsZorderIdx))
  {
    xPCMSampleRestoration (pcCU, uiAbsZorderIdx, uiDepth, TEXT_LUMA    );
    xPCMSampleRestoration (pcCU, uiAbsZorderIdx, uiDepth, TEXT_CHROMA_U);
    xPCMSampleRestoration (pcCU, uiAbsZorderIdx, uiDepth, TEXT_CHROMA_V);
  }
}

/** PCM sample restoration. 
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx part index
 * \param uiDepth CU depth
 * \param ttText texture component type
 * \returns Void
 */
Void TComAdaptiveLoopFilter::xPCMSampleRestoration (TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, TextType ttText)
{
  TComPicYuv* pcPicYuvRec = pcCU->getPic()->getPicYuvRec();
  Pel* piSrc;
  Pel* piPcm;
  UInt uiStride;
  UInt uiWidth;
  UInt uiHeight;
#if E192_SPS_PCM_BIT_DEPTH_SYNTAX
  UInt uiPcmLeftShiftBit; 
#endif
  UInt uiX, uiY;
  UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
  UInt uiLumaOffset   = uiMinCoeffSize*uiAbsZorderIdx;
  UInt uiChromaOffset = uiLumaOffset>>2;

  if( ttText == TEXT_LUMA )
  {
    piSrc = pcPicYuvRec->getLumaAddr( pcCU->getAddr(), uiAbsZorderIdx);
    piPcm = pcCU->getPCMSampleY() + uiLumaOffset;
    uiStride  = pcPicYuvRec->getStride();
    uiWidth  = (g_uiMaxCUWidth >> uiDepth);
    uiHeight = (g_uiMaxCUHeight >> uiDepth);
#if E192_SPS_PCM_BIT_DEPTH_SYNTAX
    uiPcmLeftShiftBit = g_uiBitDepth + g_uiBitIncrement - pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();
#endif
  }
  else
  {
    if( ttText == TEXT_CHROMA_U )
    {
      piSrc = pcPicYuvRec->getCbAddr( pcCU->getAddr(), uiAbsZorderIdx );
      piPcm = pcCU->getPCMSampleCb() + uiChromaOffset;
    }
    else
    {
      piSrc = pcPicYuvRec->getCrAddr( pcCU->getAddr(), uiAbsZorderIdx );
      piPcm = pcCU->getPCMSampleCr() + uiChromaOffset;
    }

    uiStride = pcPicYuvRec->getCStride();
    uiWidth  = ((g_uiMaxCUWidth >> uiDepth)/2);
    uiHeight = ((g_uiMaxCUWidth >> uiDepth)/2);
#if E192_SPS_PCM_BIT_DEPTH_SYNTAX
    uiPcmLeftShiftBit = g_uiBitDepth + g_uiBitIncrement - pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();
#endif
  }

  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
    {
#if E192_SPS_PCM_BIT_DEPTH_SYNTAX
      piSrc[uiX] = (piPcm[uiX] << uiPcmLeftShiftBit);
#else
      if(g_uiBitIncrement > 0)
      {
        piSrc[uiX] = (piPcm[uiX] << g_uiBitIncrement);
      }
      else
      {
        piSrc[uiX] = piPcm[uiX];
      }
#endif
    }
    piPcm += uiWidth;
    piSrc += uiStride;
  }
}
#endif
//! \}
