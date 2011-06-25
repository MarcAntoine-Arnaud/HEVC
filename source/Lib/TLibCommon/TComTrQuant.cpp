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

/** \file     TComTrQuant.cpp
    \brief    transform and quantization class
*/

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "TComTrQuant.h"
#include "TComPic.h"
#include "ContextTables.h"

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define RDOQ_CHROMA                 1           ///< use of RDOQ in chroma

// ====================================================================================================================
// Tables
// ====================================================================================================================

// RDOQ parameter


#if CAVLC_COEF_LRG_BLK
static const int VLClength[14][128] =
#else
static const int VLClength[13][128] =
#endif
{
  { 1, 2, 3, 4, 5, 6, 7, 9, 9,11,11,11,11,13,13,13,13,13,13,13,13,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19},
  { 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,10,10,10,10,12,12,12,12,12,12,12,12,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18},
  { 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,11,11,11,11,11,11,11,11,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17},
  { 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9,10,10,10,10,10,10,10,10,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16},
  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13},
  { 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,32,32,33,33,34,34,35,35,36,36,37,37,38,38,39,39,40,40,41,41,42,42,43,43,44,44,45,45,46,46,47,47,48,48,49,49,50,50,51,51,52,52,53,53,54,54,55,55,56,56,57,57,58,58,59,59,60,60,61,61,62,62,63,63,64,64,65,65},
  { 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,30,30,30,30,31,31,31,31,32,32,32,32,33,33,33,33,34,34,34,34},
  { 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,16,16,16,16,16,16,16,16,17,17,17,17,17,17,17,17,18,18,18,18,18,18,18,18,19,19,19,19,19,19,19,19},
  { 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,13,13,13,13,13},
  { 1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,15},

  { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
  { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}
#if CAVLC_COEF_LRG_BLK
  ,{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12}
#endif
};

// ====================================================================================================================
// Qp class member functions
// ====================================================================================================================

QpParam::QpParam()
{
}

// ====================================================================================================================
// TComTrQuant class member functions
// ====================================================================================================================

TComTrQuant::TComTrQuant()
{
  m_cQP.clear();
  
  // allocate temporary buffers
  m_plTempCoeff  = new Long[ MAX_CU_SIZE*MAX_CU_SIZE ];
  
  // allocate bit estimation class  (for RDOQ)
  m_pcEstBitsSbac = new estBitsSbacStruct;
}

TComTrQuant::~TComTrQuant()
{
  // delete temporary buffers
  if ( m_plTempCoeff )
  {
    delete [] m_plTempCoeff;
    m_plTempCoeff = NULL;
  }
  
  // delete bit estimation class
  if ( m_pcEstBitsSbac ) delete m_pcEstBitsSbac;
}

/// Including Chroma QP Parameter setting
Void TComTrQuant::setQPforQuant( Int iQP, Bool bLowpass, SliceType eSliceType, TextType eTxtType)
{
  iQP = max( min( iQP, 51 ), 0 );
  
  if(eTxtType != TEXT_LUMA) //Chroma
  {
    iQP  = g_aucChromaScale[ iQP ];
  }
  
  m_cQP.setQpParam( iQP, bLowpass, eSliceType );
}

#if MATRIX_MULT
/** NxN forward transform (2D) using brute force matrix multiplication (3 nested loops)
 *  \param block pointer to input data (residual)
 *  \param coeff pointer to output data (transform coefficients)
 *  \param uiStride stride of input data
 *  \param uiTrSize transform size (uiTrSize x uiTrSize)
 *  \param uiMode is Intra Prediction mode used in Mode-Dependent DCT/DST only
 */
#if INTRA_DST_TYPE_7
void xTr(Pel *block, Long *coeff, UInt uiStride, UInt uiTrSize, UInt uiMode)
#else
void xTr(Pel *block, Long *coeff, UInt uiStride, UInt uiTrSize)
#endif
{
  Int i,j,k,iSum;
  Int tmp[32*32];
  const short *iT;
  UInt uiLog2TrSize = g_aucConvertToBit[ uiTrSize ] + 2;

  if (uiTrSize==4)
  {
    iT  = g_aiT4[0];
  }
  else if (uiTrSize==8)
  {
    iT = g_aiT8[0];
  }
  else if (uiTrSize==16)
  {
    iT = g_aiT16[0];
  }
  else if (uiTrSize==32)
  {
    iT = g_aiT32[0];
  }
  else
  {
    assert(0);
  }

#if FULL_NBIT
  int shift_1st = uiLog2TrSize - 1 + g_uiBitDepth - 8; // log2(N) - 1 + g_uiBitDepth - 8
#else
  int shift_1st = uiLog2TrSize - 1 + g_uiBitIncrement; // log2(N) - 1 + g_uiBitIncrement
#endif

  int add_1st = 1<<(shift_1st-1);
  int shift_2nd = uiLog2TrSize + 6;
  int add_2nd = 1<<(shift_2nd-1);

  /* Horizontal transform */

#if INTRA_DST_TYPE_7
  if (uiTrSize==4)
  {
    if (uiMode != REG_DCT && g_aucDCTDSTMode_Hor[uiMode])
    {
      iT  =  g_as_DST_MAT_4[0];
    }
  }
#endif
  for (i=0; i<uiTrSize; i++)
  {
    for (j=0; j<uiTrSize; j++)
    {
      iSum = 0;
      for (k=0; k<uiTrSize; k++)
      {
        iSum += iT[i*uiTrSize+k]*block[j*uiStride+k];
      }
      tmp[i*uiTrSize+j] = (iSum + add_1st)>>shift_1st;
    }
  }
/* Vertical transform */
#if INTRA_DST_TYPE_7
  if (uiTrSize==4)
  {
    if (uiMode != REG_DCT && g_aucDCTDSTMode_Vert[uiMode])
    {
      iT  =  g_as_DST_MAT_4[0];
    }
    else
    {
      iT  = g_aiT4[0];
    }
  }
#endif
  for (i=0; i<uiTrSize; i++)
  {                 
    for (j=0; j<uiTrSize; j++)
    {
      iSum = 0;
      for (k=0; k<uiTrSize; k++)
      {
        iSum += iT[i*uiTrSize+k]*tmp[j*uiTrSize+k];        
      }
      coeff[i*uiTrSize+j] = (iSum + add_2nd)>>shift_2nd; 
    }
  }  
}

/** NxN inverse transform (2D) using brute force matrix multiplication (3 nested loops)
 *  \param coeff pointer to input data (transform coefficients)
 *  \param block pointer to output data (residual)
 *  \param uiStride stride of output data
 *  \param uiTrSize transform size (uiTrSize x uiTrSize)
 *  \param uiMode is Intra Prediction mode used in Mode-Dependent DCT/DST only
 */
#if INTRA_DST_TYPE_7
void xITr(Long *coeff, Pel *block, UInt uiStride, UInt uiTrSize, UInt uiMode)
#else
void xITr(Long *coeff, Pel *block, UInt uiStride, UInt uiTrSize)
#endif
{
  int i,j,k,iSum;
  Int tmp[32*32];
  const short *iT;
  if (uiTrSize==4)
  {
    iT  = g_aiT4[0];
  }
  else if (uiTrSize==8)
  {
    iT = g_aiT8[0];
  }
  else if (uiTrSize==16)
  {
    iT = g_aiT16[0];
  }
  else if (uiTrSize==32)
  {
    iT = g_aiT32[0];
  }
  else
  {
    assert(0);
  }
  int shift_1st = SHIFT_INV_1ST;
  int add_1st = 1<<(shift_1st-1);  
#if FULL_NBIT
  int shift_2nd = SHIFT_INV_2ND - ((short)g_uiBitDepth - 8);
#else
  int shift_2nd = SHIFT_INV_2ND - g_uiBitIncrement;
#endif
  int add_2nd = 1<<(shift_2nd-1);
#if INTRA_DST_TYPE_7
  if (uiTrSize==4)
  {
    if (uiMode != REG_DCT && g_aucDCTDSTMode_Vert[uiMode] ) // Check for DCT or DST
    {
      iT  =  g_as_DST_MAT_4[0];
    }
  }
#endif
  /* Horizontal transform */
  for (i=0; i<uiTrSize; i++)
  {    
    for (j=0; j<uiTrSize; j++)
    {
      iSum = 0;
      for (k=0; k<uiTrSize; k++)
      {        
        iSum += iT[k*uiTrSize+i]*coeff[k*uiTrSize+j]; 
      }
      tmp[i*uiTrSize+j] = (iSum + add_1st)>>shift_1st;
    }
  }   
#if INTRA_DST_TYPE_7
  if (uiTrSize==4)
  {
    if (uiMode != REG_DCT && g_aucDCTDSTMode_Hor[uiMode] )   // Check for DCT or DST
    {
      iT  =  g_as_DST_MAT_4[0];
    }
    else  
    {
      iT  = g_aiT4[0];
    }
  }
#endif
  /* Vertical transform */
  for (i=0; i<uiTrSize; i++)
  {   
    for (j=0; j<uiTrSize; j++)
    {
      iSum = 0;
      for (k=0; k<uiTrSize; k++)
      {        
        iSum += iT[k*uiTrSize+j]*tmp[i*uiTrSize+k];
      }
      block[i*uiStride+j] = (iSum + add_2nd)>>shift_2nd;
    }
  }
}

#else //MATRIX_MULT

/** 4x4 forward transform implemented using partial butterfly structure (1D)
 *  \param block input data (residual)
 *  \param coeff output data (transform coefficients)
 *  \param shift specifies right shift after 1D transform
 */
void partialButterfly4(short block[4][4],short coeff[4][4],int shift)
{
  int j;  
  int E[2],O[2];
  int add = 1<<(shift-1);

  for (j=0; j<4; j++)
  {    
    /* E and O */
    E[0] = block[j][0] + block[j][3];
    O[0] = block[j][0] - block[j][3];
    E[1] = block[j][1] + block[j][2];
    O[1] = block[j][1] - block[j][2];

    coeff[0][j] = (g_aiT4[0][0]*E[0] + g_aiT4[0][1]*E[1] + add)>>shift;
    coeff[2][j] = (g_aiT4[2][0]*E[0] + g_aiT4[2][1]*E[1] + add)>>shift;
    coeff[1][j] = (g_aiT4[1][0]*O[0] + g_aiT4[1][1]*O[1] + add)>>shift;
    coeff[3][j] = (g_aiT4[3][0]*O[0] + g_aiT4[3][1]*O[1] + add)>>shift;
  }
}

#if INTRA_DST_TYPE_7
// Fast DST Algorithm. Full matrix multiplication for DST and Fast DST algorithm 
// give identical results
void fastForwardDst(short block[4][4],short coeff[4][4],int shift)  // input block, output coeff
{
  int i, c[4];
  int rnd_factor = 1<<(shift-1);
  for (i=0; i<4; i++)
  {
    // Intermediate Variables
    c[0] = block[i][0] + block[i][3];
    c[1] = block[i][1] + block[i][3];
    c[2] = block[i][0] - block[i][1];
    c[3] = 74* block[i][2];
    
    coeff[0][i] =  ( 29 * c[0] + 55 * c[1]         + c[3]               + rnd_factor ) >> shift;
    coeff[1][i] =  ( 74 * (block[i][0]+ block[i][1] - block[i][3])      + rnd_factor ) >> shift;
    coeff[2][i] =  ( 29 * c[2] + 55 * c[0]         - c[3]               + rnd_factor ) >> shift;
    coeff[3][i] =  ( 55 * c[2] - 29 * c[1]         + c[3]               + rnd_factor ) >> shift;
  }
}
void fastInverseDst(short tmp[4][4],short block[4][4],int shift)  // input tmp, output block
{
  int i, c[4];
  int rnd_factor = 1<<(shift-1);
  for (i=0; i<4; i++)
  {  
    // Intermediate Variables
    c[0] = tmp[0][i] + tmp[2][i];
    c[1] = tmp[2][i] + tmp[3][i];
    c[2] = tmp[0][i] - tmp[3][i];
    c[3] = 74* tmp[1][i];

    block[i][0] =  ( 29 * c[0] + 55 * c[1]     + c[3]               + rnd_factor ) >> shift;
    block[i][1] =  ( 55 * c[2] - 29 * c[1]     + c[3]               + rnd_factor ) >> shift;
    block[i][2] =  ( 74 * (tmp[0][i] - tmp[2][i]  + tmp[3][i])      + rnd_factor ) >> shift;
    block[i][3] =  ( 55 * c[0] + 29 * c[2]     - c[3]               + rnd_factor ) >> shift;
  }
}
#endif 
/** 4x4 forward transform (2D)
 *  \param block input data (residual)
 *  \param coeff output data (transform coefficients)
 *  \param uiMode is Intra Prediction mode used in Mode-Dependent DCT/DST only
 */
#if INTRA_DST_TYPE_7
void xTr4(short block[4][4],short coeff[4][4],UInt uiMode)
#else
void xTr4(short block[4][4],short coeff[4][4])
#endif
{
#if FULL_NBIT
  int shift_1st = 1 + g_uiBitDepth - 8; // log2(4) - 1 + g_uiBitDepth - 8
#else
  int shift_1st = 1 + g_uiBitIncrement; // log2(4) - 1 + g_uiBitIncrement
#endif
  int shift_2nd = 8;                    // log2(4) + 6
  short tmp[4][4]; 
#if INTRA_DST_TYPE_7
  if (uiMode != REG_DCT && g_aucDCTDSTMode_Hor[uiMode])// Check for DCT or DST
  {
    fastForwardDst(block,tmp,shift_1st); // Forward DST BY FAST ALGORITHM, block input, tmp output
  }
  else  
  {
    partialButterfly4(block,tmp,shift_1st);
  }
#else
  partialButterfly4(block,tmp,shift_1st);
#endif

#if INTRA_DST_TYPE_7
  if (uiMode != REG_DCT && g_aucDCTDSTMode_Vert[uiMode] )   // Check for DCT or DST
  {
    fastForwardDst(tmp,coeff,shift_2nd); // Forward DST BY FAST ALGORITHM, tmp input, coeff output
  }
  else  
  {
    partialButterfly4(tmp,coeff,shift_2nd);
  }   
#else
  partialButterfly4(tmp,coeff,shift_2nd);
#endif
}

/** 4x4 inverse transform implemented using partial butterfly structure (1D)
 *  \param coeff input data (transform coefficients)
 *  \param block output data (residual)
 *  \param shift specifies right shift after 1D transform
 */
void partialButterflyInverse4(short tmp[4][4],short block[4][4],int shift)
{
  int j;    
  int E[2],O[2];
  int add = 1<<(shift-1);

  for (j=0; j<4; j++)
  {    
    /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */    
    O[0] = g_aiT4[1][0]*tmp[1][j] + g_aiT4[3][0]*tmp[3][j];
    O[1] = g_aiT4[1][1]*tmp[1][j] + g_aiT4[3][1]*tmp[3][j];
    E[0] = g_aiT4[0][0]*tmp[0][j] + g_aiT4[2][0]*tmp[2][j];
    E[1] = g_aiT4[0][1]*tmp[0][j] + g_aiT4[2][1]*tmp[2][j];
    
    /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */ 
    block[j][0] = (E[0] + O[0] + add)>>shift;
    block[j][1] = (E[1] + O[1] + add)>>shift;
    block[j][2] = (E[1] - O[1] + add)>>shift;
    block[j][3] = (E[0] - O[0] + add)>>shift;
  }
}

/** 4x4 inverse transform (2D)
 *  \param coeff input data (transform coefficients)
 *  \param block output data (residual)
 *  \param uiMode is Intra Prediction mode used in Mode-Dependent DCT/DST only
 */
#if INTRA_DST_TYPE_7
void xITr4(short coeff[4][4],short block[4][4], UInt uiMode)
#else
void xITr4(short coeff[4][4],short block[4][4])
#endif
{
  int shift_1st = SHIFT_INV_1ST;
#if FULL_NBIT
  int shift_2nd = SHIFT_INV_2ND - ((short)g_uiBitDepth - 8);
#else
  int shift_2nd = SHIFT_INV_2ND - g_uiBitIncrement;
#endif
  short tmp[4][4];
  
#if INTRA_DST_TYPE_7
  if (uiMode != REG_DCT && g_aucDCTDSTMode_Vert[uiMode] )    // Check for DCT or DST
  {
    fastInverseDst(coeff,tmp,shift_1st);    // Inverse DST by FAST Algorithm, coeff input, tmp output
  }
  else
  {
    partialButterflyInverse4(coeff,tmp,shift_1st);    
  } 
#else
  partialButterflyInverse4(coeff,tmp,shift_1st);
#endif
#if INTRA_DST_TYPE_7
  if (uiMode != REG_DCT && g_aucDCTDSTMode_Hor[uiMode] )    // Check for DCT or DST
  {
    fastInverseDst(tmp,block,shift_2nd); // Inverse DST by FAST Algorithm, tmp input, coeff output
  }
  else
  {
    partialButterflyInverse4(tmp,block,shift_2nd);
  }   
#else
  partialButterflyInverse4(tmp,block,shift_2nd);
#endif
}

/** 8x8 forward transform implemented using partial butterfly structure (1D)
 *  \param block input data (residual)
 *  \param coeff output data (transform coefficients)
 *  \param shift specifies right shift after 1D transform
 */
void partialButterfly8(short block[8][8],short coeff[8][8],int shift)
{
  int j,k;  
  int E[4],O[4];
  int EE[2],EO[2];
  int add = 1<<(shift-1);

  for (j=0; j<8; j++)
  {    
    /* E and O*/
    for (k=0;k<4;k++)
    {
      E[k] = block[j][k] + block[j][7-k];
      O[k] = block[j][k] - block[j][7-k];
    }    
    /* EE and EO */
    EE[0] = E[0] + E[3];    
    EO[0] = E[0] - E[3];
    EE[1] = E[1] + E[2];
    EO[1] = E[1] - E[2];

    coeff[0][j] = (g_aiT8[0][0]*EE[0] + g_aiT8[0][1]*EE[1] + add)>>shift;
    coeff[4][j] = (g_aiT8[4][0]*EE[0] + g_aiT8[4][1]*EE[1] + add)>>shift; 
    coeff[2][j] = (g_aiT8[2][0]*EO[0] + g_aiT8[2][1]*EO[1] + add)>>shift;
    coeff[6][j] = (g_aiT8[6][0]*EO[0] + g_aiT8[6][1]*EO[1] + add)>>shift; 

    coeff[1][j] = (g_aiT8[1][0]*O[0] + g_aiT8[1][1]*O[1] + g_aiT8[1][2]*O[2] + g_aiT8[1][3]*O[3] + add)>>shift;
    coeff[3][j] = (g_aiT8[3][0]*O[0] + g_aiT8[3][1]*O[1] + g_aiT8[3][2]*O[2] + g_aiT8[3][3]*O[3] + add)>>shift;
    coeff[5][j] = (g_aiT8[5][0]*O[0] + g_aiT8[5][1]*O[1] + g_aiT8[5][2]*O[2] + g_aiT8[5][3]*O[3] + add)>>shift;
    coeff[7][j] = (g_aiT8[7][0]*O[0] + g_aiT8[7][1]*O[1] + g_aiT8[7][2]*O[2] + g_aiT8[7][3]*O[3] + add)>>shift;
  }
}

/** 8x8 forward transform (2D)
 *  \param block input data (residual)
 *  \param coeff  output data (transform coefficients)
 */
void xTr8(short block[8][8],short coeff[8][8])
{
#if FULL_NBIT
  int shift_1st = 2 + g_uiBitDepth - 8; // log2(8) - 1 + g_uiBitDepth - 8
#else
  int shift_1st = 2 + g_uiBitIncrement; // log2(8) - 1 + g_uiBitIncrement
#endif
  int shift_2nd = 9;                    // log2(8) + 6
  short tmp[8][8]; 

  partialButterfly8(block,tmp,shift_1st);
  partialButterfly8(tmp,coeff,shift_2nd);
}

/** 8x8 inverse transform implemented using partial butterfly structure (1D)
 *  \param coeff input data (transform coefficients)
 *  \param block output data (residual)
 *  \param shift specifies right shift after 1D transform
 */
void partialButterflyInverse8(short tmp[8][8],short block[8][8],int shift)
{
  int j,k;    
  int E[4],O[4];
  int EE[2],EO[2];
  int add = 1<<(shift-1);

  for (j=0; j<8; j++)
  {    
    /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
    for (k=0;k<4;k++)
    {
      O[k] = g_aiT8[ 1][k]*tmp[ 1][j] + g_aiT8[ 3][k]*tmp[ 3][j] + g_aiT8[ 5][k]*tmp[ 5][j] + g_aiT8[ 7][k]*tmp[ 7][j];
    }
   
    EO[0] = g_aiT8[2][0]*tmp[2][j] + g_aiT8[6][0]*tmp[6][j];
    EO[1] = g_aiT8[2][1]*tmp[2][j] + g_aiT8[6][1]*tmp[6][j];
    EE[0] = g_aiT8[0][0]*tmp[0][j] + g_aiT8[4][0]*tmp[4][j];
    EE[1] = g_aiT8[0][1]*tmp[0][j] + g_aiT8[4][1]*tmp[4][j];

    /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */ 
    E[0] = EE[0] + EO[0];
    E[3] = EE[0] - EO[0];
    E[1] = EE[1] + EO[1];
    E[2] = EE[1] - EO[1];
    for (k=0;k<4;k++)
    {
      block[j][k] = (E[k] + O[k] + add)>>shift;
      block[j][k+4] = (E[3-k] - O[3-k] + add)>>shift;
    }        
  }
}

/** 8x8 inverse transform (2D)
 *  \param coeff input data (transform coefficients)
 *  \param block output data (residual)
 */
void xITr8(short coeff[8][8],short block[8][8])
{
  int shift_1st = SHIFT_INV_1ST;
#if FULL_NBIT
  int shift_2nd = SHIFT_INV_2ND - ((short)g_uiBitDepth - 8);
#else
  int shift_2nd = SHIFT_INV_2ND - g_uiBitIncrement;
#endif
  short tmp[8][8];
  
  partialButterflyInverse8(coeff,tmp,shift_1st);
  partialButterflyInverse8(tmp,block,shift_2nd);
}

/** 16x16 forward transform implemented using partial butterfly structure (1D)
 *  \param block input data (residual)
 *  \param coeff output data (transform coefficients)
 *  \param shift specifies right shift after 1D transform
 */
void partialButterfly16(short block[16][16],short coeff[16][16],int shift)
{
  int j,k;
  int E[8],O[8];
  int EE[4],EO[4];
  int EEE[2],EEO[2];
  int add = 1<<(shift-1);

  for (j=0; j<16; j++)
  {    
    /* E and O*/
    for (k=0;k<8;k++)
    {
      E[k] = block[j][k] + block[j][15-k];
      O[k] = block[j][k] - block[j][15-k];
    } 
    /* EE and EO */
    for (k=0;k<4;k++)
    {
      EE[k] = E[k] + E[7-k];
      EO[k] = E[k] - E[7-k];
    }
    /* EEE and EEO */
    EEE[0] = EE[0] + EE[3];    
    EEO[0] = EE[0] - EE[3];
    EEE[1] = EE[1] + EE[2];
    EEO[1] = EE[1] - EE[2];

    coeff[ 0][j] = (g_aiT16[ 0][0]*EEE[0] + g_aiT16[ 0][1]*EEE[1] + add)>>shift;        
    coeff[ 8][j] = (g_aiT16[ 8][0]*EEE[0] + g_aiT16[ 8][1]*EEE[1] + add)>>shift;    
    coeff[ 4][j] = (g_aiT16[ 4][0]*EEO[0] + g_aiT16[ 4][1]*EEO[1] + add)>>shift;        
    coeff[12][j] = (g_aiT16[12][0]*EEO[0] + g_aiT16[12][1]*EEO[1] + add)>>shift;

    for (k=2;k<16;k+=4)
    {
      coeff[k][j] = (g_aiT16[k][0]*EO[0] + g_aiT16[k][1]*EO[1] + g_aiT16[k][2]*EO[2] + g_aiT16[k][3]*EO[3] + add)>>shift;      
    }
    
    for (k=1;k<16;k+=2)
    {
      coeff[k][j] = (g_aiT16[k][0]*O[0] + g_aiT16[k][1]*O[1] + g_aiT16[k][2]*O[2] + g_aiT16[k][3]*O[3] + 
                     g_aiT16[k][4]*O[4] + g_aiT16[k][5]*O[5] + g_aiT16[k][6]*O[6] + g_aiT16[k][7]*O[7] + add)>>shift;
    }

  }
}

/** 16x16 forward transform (2D)
 *  \param block input data (residual)
 *  \param coeff output data (transform coefficients)
 */
void xTr16(short block[16][16],short coeff[16][16])
{
 #if FULL_NBIT
  int shift_1st = 3 + g_uiBitDepth - 8; // log2(16) - 1 + g_uiBitDepth - 8
#else
  int shift_1st = 3 + g_uiBitIncrement; // log2(16) - 1 + g_uiBitIncrement
#endif
  int shift_2nd = 10;                   // log2(16) + 6
  short tmp[16][16]; 

  partialButterfly16(block,tmp,shift_1st);
  partialButterfly16(tmp,coeff,shift_2nd);
}

/** 16x16 inverse transform implemented using partial butterfly structure (1D)
 *  \param coeff input data (transform coefficients)
 *  \param block output data (residual)
 *  \param shift specifies right shift after 1D transform
 */
void partialButterflyInverse16(short tmp[16][16],short block[16][16],int shift)
{
  int j,k;  
  int E[8],O[8];
  int EE[4],EO[4];
  int EEE[2],EEO[2];
  int add = 1<<(shift-1);

  for (j=0; j<16; j++)
  {    
    /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
    for (k=0;k<8;k++)
    {
      O[k] = g_aiT16[ 1][k]*tmp[ 1][j] + g_aiT16[ 3][k]*tmp[ 3][j] + g_aiT16[ 5][k]*tmp[ 5][j] + g_aiT16[ 7][k]*tmp[ 7][j] + 
             g_aiT16[ 9][k]*tmp[ 9][j] + g_aiT16[11][k]*tmp[11][j] + g_aiT16[13][k]*tmp[13][j] + g_aiT16[15][k]*tmp[15][j];
    }
    for (k=0;k<4;k++)
    {
      EO[k] = g_aiT16[ 2][k]*tmp[ 2][j] + g_aiT16[ 6][k]*tmp[ 6][j] + g_aiT16[10][k]*tmp[10][j] + g_aiT16[14][k]*tmp[14][j];
    }
    EEO[0] = g_aiT16[4][0]*tmp[4][j] + g_aiT16[12][0]*tmp[12][j];
    EEE[0] = g_aiT16[0][0]*tmp[0][j] + g_aiT16[ 8][0]*tmp[ 8][j];
    EEO[1] = g_aiT16[4][1]*tmp[4][j] + g_aiT16[12][1]*tmp[12][j];
    EEE[1] = g_aiT16[0][1]*tmp[0][j] + g_aiT16[ 8][1]*tmp[ 8][j];

    /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */ 
    for (k=0;k<2;k++)
    {
      EE[k] = EEE[k] + EEO[k];
      EE[k+2] = EEE[1-k] - EEO[1-k];
    }    
    for (k=0;k<4;k++)
    {
      E[k] = EE[k] + EO[k];
      E[k+4] = EE[3-k] - EO[3-k];
    }    
    for (k=0;k<8;k++)
    {
      block[j][k] = (E[k] + O[k] + add)>>shift;
      block[j][k+8] = (E[7-k] - O[7-k] + add)>>shift;
    }        
  }
}

/** 16x16 inverse transform (2D)
 *  \param coeff input data (transform coefficients)
 *  \param block output data (residual)
 */
void xITr16(short coeff[16][16],short block[16][16])
{
  int shift_1st = SHIFT_INV_1ST;
#if FULL_NBIT
  int shift_2nd = SHIFT_INV_2ND - ((short)g_uiBitDepth - 8);
#else
  int shift_2nd = SHIFT_INV_2ND - g_uiBitIncrement;
#endif
  short tmp[16][16];
  
  partialButterflyInverse16(coeff,tmp,shift_1st);
  partialButterflyInverse16(tmp,block,shift_2nd);
}

/** 32x32 forward transform implemented using partial butterfly structure (1D)
 *  \param block input data (residual)
 *  \param coeff output data (transform coefficients)
 *  \param shift specifies right shift after 1D transform
 */
void partialButterfly32(short block[32][32],short coeff[32][32],int shift)
{
  int j,k;
  int E[16],O[16];
  int EE[8],EO[8];
  int EEE[4],EEO[4];
  int EEEE[2],EEEO[2];
  int add = 1<<(shift-1);

  for (j=0; j<32; j++)
  {    
    /* E and O*/
    for (k=0;k<16;k++)
    {
      E[k] = block[j][k] + block[j][31-k];
      O[k] = block[j][k] - block[j][31-k];
    } 
    /* EE and EO */
    for (k=0;k<8;k++)
    {
      EE[k] = E[k] + E[15-k];
      EO[k] = E[k] - E[15-k];
    }
    /* EEE and EEO */
    for (k=0;k<4;k++)
    {
      EEE[k] = EE[k] + EE[7-k];
      EEO[k] = EE[k] - EE[7-k];
    }
    /* EEEE and EEEO */
    EEEE[0] = EEE[0] + EEE[3];    
    EEEO[0] = EEE[0] - EEE[3];
    EEEE[1] = EEE[1] + EEE[2];
    EEEO[1] = EEE[1] - EEE[2];

    coeff[ 0][j] = (g_aiT32[ 0][0]*EEEE[0] + g_aiT32[ 0][1]*EEEE[1] + add)>>shift;
    coeff[16][j] = (g_aiT32[16][0]*EEEE[0] + g_aiT32[16][1]*EEEE[1] + add)>>shift;
    coeff[ 8][j] = (g_aiT32[ 8][0]*EEEO[0] + g_aiT32[ 8][1]*EEEO[1] + add)>>shift; 
    coeff[24][j] = (g_aiT32[24][0]*EEEO[0] + g_aiT32[24][1]*EEEO[1] + add)>>shift;
    for (k=4;k<32;k+=8)
    {
      coeff[k][j] = (g_aiT32[k][0]*EEO[0] + g_aiT32[k][1]*EEO[1] + g_aiT32[k][2]*EEO[2] + g_aiT32[k][3]*EEO[3] + add)>>shift;
    }       
    for (k=2;k<32;k+=4)
    {
      coeff[k][j] = (g_aiT32[k][0]*EO[0] + g_aiT32[k][1]*EO[1] + g_aiT32[k][2]*EO[2] + g_aiT32[k][3]*EO[3] + 
                     g_aiT32[k][4]*EO[4] + g_aiT32[k][5]*EO[5] + g_aiT32[k][6]*EO[6] + g_aiT32[k][7]*EO[7] + add)>>shift;
    }       
    for (k=1;k<32;k+=2)
    {
      coeff[k][j] = (g_aiT32[k][ 0]*O[ 0] + g_aiT32[k][ 1]*O[ 1] + g_aiT32[k][ 2]*O[ 2] + g_aiT32[k][ 3]*O[ 3] + 
                     g_aiT32[k][ 4]*O[ 4] + g_aiT32[k][ 5]*O[ 5] + g_aiT32[k][ 6]*O[ 6] + g_aiT32[k][ 7]*O[ 7] +
                     g_aiT32[k][ 8]*O[ 8] + g_aiT32[k][ 9]*O[ 9] + g_aiT32[k][10]*O[10] + g_aiT32[k][11]*O[11] + 
                     g_aiT32[k][12]*O[12] + g_aiT32[k][13]*O[13] + g_aiT32[k][14]*O[14] + g_aiT32[k][15]*O[15] + add)>>shift;
    }
  }
}

/** 32x32 forward transform (2D)
 *  \param block input data (residual)
 *  \param coeff output data (transform coefficients)
 */
void xTr32(short block[32][32],short coeff[32][32])
{
 #if FULL_NBIT
  int shift_1st = 4 + g_uiBitDepth - 8; // log2(32) - 1 + g_uiBitDepth - 8
#else
  int shift_1st = 4 + g_uiBitIncrement; // log2(32) - 1 + g_uiBitIncrement
#endif
  int shift_2nd = 11;                   // log2(32) + 6
  short tmp[32][32]; 

  partialButterfly32(block,tmp,shift_1st);
  partialButterfly32(tmp,coeff,shift_2nd);
}

/** 32x32 inverse transform implemented using partial butterfly structure (1D)
 *  \param coeff input data (transform coefficients)
 *  \param block output data (residual)
 *  \param shift specifies right shift after 1D transform
 */
void partialButterflyInverse32(short tmp[32][32],short block[32][32],int shift)
{
  int j,k;  
  int E[16],O[16];
  int EE[8],EO[8];
  int EEE[4],EEO[4];
  int EEEE[2],EEEO[2];
  int add = 1<<(shift-1);

  for (j=0; j<32; j++)
  {    
    /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
    for (k=0;k<16;k++)
    {
      O[k] = g_aiT32[ 1][k]*tmp[ 1][j] + g_aiT32[ 3][k]*tmp[ 3][j] + g_aiT32[ 5][k]*tmp[ 5][j] + g_aiT32[ 7][k]*tmp[ 7][j] + 
             g_aiT32[ 9][k]*tmp[ 9][j] + g_aiT32[11][k]*tmp[11][j] + g_aiT32[13][k]*tmp[13][j] + g_aiT32[15][k]*tmp[15][j] + 
             g_aiT32[17][k]*tmp[17][j] + g_aiT32[19][k]*tmp[19][j] + g_aiT32[21][k]*tmp[21][j] + g_aiT32[23][k]*tmp[23][j] + 
             g_aiT32[25][k]*tmp[25][j] + g_aiT32[27][k]*tmp[27][j] + g_aiT32[29][k]*tmp[29][j] + g_aiT32[31][k]*tmp[31][j];
    }
    for (k=0;k<8;k++)
    {
      EO[k] = g_aiT32[ 2][k]*tmp[ 2][j] + g_aiT32[ 6][k]*tmp[ 6][j] + g_aiT32[10][k]*tmp[10][j] + g_aiT32[14][k]*tmp[14][j] + 
              g_aiT32[18][k]*tmp[18][j] + g_aiT32[22][k]*tmp[22][j] + g_aiT32[26][k]*tmp[26][j] + g_aiT32[30][k]*tmp[30][j];
    }
    for (k=0;k<4;k++)
    {
      EEO[k] = g_aiT32[4][k]*tmp[4][j] + g_aiT32[12][k]*tmp[12][j] + g_aiT32[20][k]*tmp[20][j] + g_aiT32[28][k]*tmp[28][j];
    }
    EEEO[0] = g_aiT32[8][0]*tmp[8][j] + g_aiT32[24][0]*tmp[24][j];
    EEEO[1] = g_aiT32[8][1]*tmp[8][j] + g_aiT32[24][1]*tmp[24][j];
    EEEE[0] = g_aiT32[0][0]*tmp[0][j] + g_aiT32[16][0]*tmp[16][j];    
    EEEE[1] = g_aiT32[0][1]*tmp[0][j] + g_aiT32[16][1]*tmp[16][j];

    /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
    EEE[0] = EEEE[0] + EEEO[0];
    EEE[3] = EEEE[0] - EEEO[0];
    EEE[1] = EEEE[1] + EEEO[1];
    EEE[2] = EEEE[1] - EEEO[1];    
    for (k=0;k<4;k++)
    {
      EE[k] = EEE[k] + EEO[k];
      EE[k+4] = EEE[3-k] - EEO[3-k];
    }    
    for (k=0;k<8;k++)
    {
      E[k] = EE[k] + EO[k];
      E[k+8] = EE[7-k] - EO[7-k];
    }    
    for (k=0;k<16;k++)
    {
      block[j][k] = (E[k] + O[k] + add)>>shift;
      block[j][k+16] = (E[15-k] - O[15-k] + add)>>shift;
    }        
  }
}

/** 32x32 inverse transform (2D)
 *  \param coeff input data (transform coefficients)
 *  \param block output data (residual)
 */
void xITr32(short coeff[32][32],short block[32][32])
{
  int shift_1st = SHIFT_INV_1ST;
#if FULL_NBIT
  int shift_2nd = SHIFT_INV_2ND - ((short)g_uiBitDepth - 8);
#else
  int shift_2nd = SHIFT_INV_2ND - g_uiBitIncrement;
#endif
  short tmp[32][32];
  
  partialButterflyInverse32(coeff,tmp,shift_1st);
  partialButterflyInverse32(tmp,block,shift_2nd);
}
#endif //MATRIX_MULT

UInt TComTrQuant::xCountVlcBits(UInt uiTableNumber, UInt uiCodeNumber)
{
  UInt uiLength = 0;
  UInt uiCode = 0;

  if ( uiCodeNumber < 128 )
  {
    uiLength=VLClength[uiTableNumber][uiCodeNumber];
  }
  else
  {
    if ( uiTableNumber < 10 )
    {
      if ( uiTableNumber < 5 )
      {
        uiCode = uiCodeNumber - (6 * (1 << uiTableNumber)) + (1 << uiTableNumber);
        uiLength = ( 6 - uiTableNumber ) + 1 + 2 * xLeadingZeros(uiCode);
      }
      else if ( uiTableNumber < 8 )
      {
        uiLength = 1 + (uiTableNumber - 4) + (uiCodeNumber >> (uiTableNumber - 4));
      }
      else if ( uiTableNumber == 9 )
      {
        uiLength = 5 + ((uiCodeNumber + 5) >> 4);
      }
    }
    else
    {
      if ( uiTableNumber == 10 )
      {
        uiCode = uiCodeNumber + 1;
        uiLength = 1 + 2 * xLeadingZeros(uiCode);
      }
#if CAVLC_COEF_LRG_BLK
      else if (uiTableNumber == 12)
      {
        uiLength = 7+(uiCodeNumber>>6);
      }
      else if(uiTableNumber == 13)
      {
        uiLength = 5+(uiCodeNumber>>4);
      }
#endif
    }
  }
  return uiLength;
}


#if CAVLC_COEF_LRG_BLK
Int TComTrQuant::bitCountRDOQ(Int coeff, Int pos, Int nTab, Int lastCoeffFlag,Int levelMode,Int run, Int maxrun, Int vlc_adaptive, Int N, 
                              UInt uiTr1, Int iSum_big_coef, Int iBlockType, TComDataCU* pcCU, const UInt **pLumaRunTr1)
#else
Int TComTrQuant::bitCountRDOQ(Int coeff, Int pos, Int nTab, Int lastCoeffFlag,Int levelMode,Int run, Int maxrun, Int vlc_adaptive, Int N, 
                              UInt uiTr1, Int iSum_big_coef, Int iBlockType, TComDataCU* pcCU)
#endif
{
  UInt cn, n, level, lev;
  Int vlc,x,cx,vlcNum,bits;
  static const int vlcTable4[3] = {2,2,2};             // Y4x4I,Y4x4P,Y4x4B,
  
  Int sign = coeff < 0 ? 1 : 0;
  
  if ( N==4 )
  {
    n = max(0, nTab - 2);
  }
  else
  {
    n = nTab;
  }
  
  UInt uiModZeroCoding = 0;
  const UInt *uiVlcTableTemp;
  
#if CAVLC_COEF_LRG_BLK
  uiModZeroCoding = (m_uiRDOQOffset==1 || N>8)? 1:0;
  int tmprun = min(maxrun,28);
  
  if( N<=8 )
  {
    uiVlcTableTemp = (nTab==2 || nTab==5)? g_auiVlcTable8x8Intra:g_auiVlcTable8x8Inter;
  }
  else
  {
    uiVlcTableTemp = (nTab==5)? g_auiVlcTable16x16Intra:g_auiVlcTable16x16Inter;
  }
#else
  if( nTab == 2 || nTab == 5 )
  {
    uiVlcTableTemp = g_auiVlcTable8x8Intra;
  }
  else
  {
    uiVlcTableTemp = g_auiVlcTable8x8Inter;
  }
#endif
  
  level = abs(coeff);
  lev = (level == 1) ? 0 : 1;
  
  if ( level )
  {
    if ( lastCoeffFlag == 1 )
    {     
      x = pos + (level == 1 ? 0 : N * N);
      if( N == 4 )
      {
        cx = m_uiLPTableE4[(n << 5) + x];
        vlcNum = vlcTable4[n];
      }
      else
      {
#if CAVLC_COEF_LRG_BLK
        cx = xLastLevelInd(lev, pos, N);
#else
        cx = m_uiLPTableE8[(n << 7) + x];
#endif
        vlcNum = g_auiLastPosVlcNum[n][min(16u,m_uiLastPosVlcIndex[n])];
      }
      bits=xCountVlcBits( vlcNum, cx );
      
      if ( level > 1 )
      {
        bits += xCountVlcBits( 0, 2 * (level - 2) + sign );
      }
      else
      {
        bits++;
      }
      
    }
    else
    { // Level !=0  && lastCoeffFlag==0
      if ( !levelMode )
      {   
#if CAVLC_COEF_LRG_BLK
        if(nTab == 2 || nTab == 5)
        {
          cn = xRunLevelInd(lev, run, maxrun, pLumaRunTr1[uiTr1][tmprun]);
        }
        else
        {
          cn = xRunLevelIndInter(lev, run, maxrun);
        }
        vlc = uiVlcTableTemp[tmprun];
#else
        if ( N == 4 )
        {
          // 4x4
          if ( nTab == 2 )
          {
            cn = xRunLevelInd(lev, run, maxrun, g_auiLumaRunTr14x4[uiTr1][maxrun]);
          }
          else
          {
#if RUNLEVEL_TABLE_CUT
            cn = xRunLevelIndInter(lev, run, maxrun);
#else
            cn = g_auiLumaRun8x8[maxrun][lev][run];
#endif
          }
          vlc = uiVlcTableTemp[maxrun];
        }
        else
        {
          // 8x8
          if(nTab == 2 || nTab == 5)
          {
            cn = xRunLevelInd(lev, run, maxrun, g_auiLumaRunTr18x8[uiTr1][min(maxrun,28)]);
          }
          else
          {
#if RUNLEVEL_TABLE_CUT
            cn = xRunLevelIndInter(lev, run, maxrun);
#else
            cn = g_auiLumaRun8x8[min(maxrun,28)][lev][run];
#endif
          }
          vlc = uiVlcTableTemp[min(maxrun,28)];
        }
#endif
        bits = xCountVlcBits( vlc, cn );
        if ( level > 1 )
        {
          bits += xCountVlcBits( 0, 2 * (level - 2) + sign );
        }
        else
        {
          bits++;
        } 
        
      }
      else
      { // Level !=0  && lastCoeffFlag==0 && levelMode
        bits = (xCountVlcBits( vlc_adaptive, level ) + 1);
      }
    }
  }
  else
  {
    if (levelMode)
    {
      bits=xCountVlcBits( vlc_adaptive, level );
    }
    else
    {                        
      if ( pos == 0 && lastCoeffFlag == 0)
      {  
#if CAVLC_COEF_LRG_BLK
        vlc = uiVlcTableTemp[tmprun];
        if(nTab == 2 || nTab == 5)
        {
          cn = xRunLevelInd(0, run + 1, maxrun, pLumaRunTr1[uiTr1][tmprun]);
        }
        else
        {
          cn = xRunLevelIndInter(0, run + 1, maxrun);
        }
#else
        if ( N == 4 )
        {
          // 4x4
          vlc = uiVlcTableTemp[maxrun];
          if ( nTab == 2 )
          {
            cn = xRunLevelInd(0, run + 1, maxrun, g_auiLumaRunTr14x4[uiTr1][maxrun]);
          }
          else
          {
#if RUNLEVEL_TABLE_CUT
            cn = xRunLevelIndInter(0, run+1, maxrun);
#else
            cn = g_auiLumaRun8x8[maxrun][0][run + 1];
#endif
          }
        }
        else
        {
          // 8x8
          vlc = uiVlcTableTemp[min(maxrun, 28)];
          if(nTab == 2 || nTab == 5)
          {
            cn = xRunLevelInd(0, run + 1, maxrun, g_auiLumaRunTr18x8[uiTr1][min(maxrun, 28)]);
          }
          else
          {
#if RUNLEVEL_TABLE_CUT
            cn = xRunLevelIndInter(0, run+1, maxrun);
#else
            cn = g_auiLumaRun8x8[min(maxrun, 28)][0][run + 1];
#endif
          }
        }
#endif
        bits=xCountVlcBits( vlc, cn );
      }
      else
      {
        bits = 0;
        
        if ( pos > 0 && uiModZeroCoding == 1 )
        {
          Int iSum_big_coefTemp, levelModeTemp, maxrunTemp;
          UInt uiTr1Temp;
          
          if ( lastCoeffFlag == 0 )
          {
#if CAVLC_COEF_LRG_BLK
            vlc = uiVlcTableTemp[tmprun];
            if(nTab == 2 || nTab == 5)
            {
              cn = xRunLevelInd(0, run + 1, maxrun, pLumaRunTr1[uiTr1][tmprun]);
            }
            else
            {
              cn = xRunLevelIndInter(0, run + 1, maxrun);
            }
#else
            if ( N == 4 )
            {
              // 4x4
              vlc = uiVlcTableTemp[maxrun];
              if ( nTab == 2 )
              {
                cn = xRunLevelInd(0, run + 1, maxrun, g_auiLumaRunTr14x4[uiTr1][maxrun]);
              }
              else
              {
#if RUNLEVEL_TABLE_CUT
                cn = xRunLevelIndInter(0, run+1, maxrun);
#else
                cn = g_auiLumaRun8x8[maxrun][0][run + 1];
#endif
              }
            }
            else
            {
              // 8x8
              vlc = uiVlcTableTemp[min(maxrun, 28)];
              if(nTab == 2 || nTab == 5)
              {
                cn = xRunLevelInd(0, run + 1, maxrun, g_auiLumaRunTr18x8[uiTr1][min(maxrun, 28)]);
              }
              else
              {
#if RUNLEVEL_TABLE_CUT
                cn = xRunLevelIndInter(0, run+1, maxrun);
#else
                cn = g_auiLumaRun8x8[min(maxrun,28)][0][run + 1];
#endif
              }
            }
#endif
          }
          else
          {
            x = (pos - 1);
            if( N == 4 )
            {
              cn = m_uiLPTableE4[(n << 5) + x];
              vlc = vlcTable4[n];
            }
            else
            {
#if CAVLC_COEF_LRG_BLK
              cn = xLastLevelInd(lev, pos, N);
#else
              cn = m_uiLPTableE8[(n << 7) + x];
#endif
              vlc = g_auiLastPosVlcNum[n][min(16u, m_uiLastPosVlcIndex[n])];
            }
          }
          bits+=xCountVlcBits( vlc, cn );
          
          // Next coeff is 1 with run=0
          
          iSum_big_coefTemp = iSum_big_coef;
          levelModeTemp = levelMode;
          Int switch_thr[10] = {49,49,0,49,49,0,49,49,49,49};
          
          if ( N > 4 )
          { 
            if ( level > 1 )
            {
              iSum_big_coefTemp += level;
              if ((N * N - pos - 1) > switch_thr[iBlockType] || iSum_big_coefTemp > 2) levelModeTemp = 1;
            }
          }
          else
          {
            if ( level > 1 ) levelModeTemp = 1;
          }
          
          if ( levelModeTemp == 1 )
          {
            bits-=xCountVlcBits( vlc_adaptive, 1);
          }
          else
          {
            maxrunTemp = pos - 1;
            uiTr1Temp = uiTr1;
            
            if ( uiTr1Temp > 0 && level < 2 )
            {
              uiTr1Temp++;
              uiTr1Temp = min(unsigned(MAX_TR1), uiTr1Temp);
            }
            else
            {
              uiTr1Temp=0;
            }
            
#if CAVLC_COEF_LRG_BLK
            vlc = uiVlcTableTemp[min(maxrunTemp,28)];
            if(nTab == 2 || nTab == 5)
            {
              cn = xRunLevelInd(0, 0, maxrunTemp, pLumaRunTr1[uiTr1Temp][min(maxrunTemp,28)]);
            }
            else
            {
              cn = xRunLevelIndInter(0, 0, maxrunTemp);
            }
#else
            if ( N == 4 )
            {
              // 4x4
              vlc = uiVlcTableTemp[maxrunTemp];
              if ( nTab == 2 )
              {
                cn = xRunLevelInd(0, 0, maxrunTemp, g_auiLumaRunTr14x4[uiTr1Temp][maxrunTemp]);
              }
              else
              {
#if RUNLEVEL_TABLE_CUT
                cn = xRunLevelIndInter(0, 0, maxrunTemp);
#else
                cn = g_auiLumaRun8x8[maxrunTemp][0][0];
#endif
              }
            }
            else
            {
              // 8x8
              vlc = uiVlcTableTemp[min(maxrunTemp,28)];
              if(nTab == 2 || nTab == 5)
              {
                cn = xRunLevelInd(0, 0, maxrunTemp, g_auiLumaRunTr18x8[uiTr1Temp][min(maxrunTemp,28)]);
              }
              else
              {
#if RUNLEVEL_TABLE_CUT
                cn = xRunLevelIndInter(0, 0, maxrunTemp);
#else
                cn = g_auiLumaRun8x8[min(maxrunTemp,28)][0][0];
#endif
              }
            }
#endif
            bits -= xCountVlcBits( vlc, cn );
          }
        } // if ( pos > 0 && uiModZeroCoding == 1 )
      } 
    }
  }
  return bits;
}

Int TComTrQuant::xCodeCoeffCountBitsLast(TCoeff* scoeff, levelDataStruct* levelData, Int nTab, UInt N)
{
  Int i, prevCoeffInd, lastPosMin, iRate;
  Int done,last_pos;
#if CAVLC_COEF_LRG_BLK
  Int run_done, maxrun,run, bitsLast[1024], bitsRun[1024], bitsLastPrev;
  quantLevelStruct quantCoeffInfo[1024];
#else
  Int run_done, maxrun,run, bitsLast[256], bitsRun[256], bitsLastPrev;
  quantLevelStruct quantCoeffInfo[256];
#endif
  UInt last_pos_init, bitsLevel, sign, lev, cn, vlc, uiBitShift=15, uiNoCoeff=N*N, absLevel;
  Int n;
  double lagrMin, lagr, lagrPrev;
  UInt uiLumaRunNoTr14x4[15]={2, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2};
  UInt uiLumaRunNoTr18x8[29]={2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 9, 13};


  if ( N == 4 )
  {
    n = max(0, nTab - 2);
  }
  else
  {
    n = nTab;
  }

  /* Do the last coefficient first */
  i = uiNoCoeff - 1;
  done = 0;

  while ( !done && i >= 0 )
  {
    if (scoeff[i])
    {
      done = 1;
    }
    else
    {
      i--;
    }
  }
  if (i == -1)
  {
    return(-1);
  }

  last_pos = last_pos_init = i;
  prevCoeffInd = i;

  i--;

  if ( i >= 0 )
  {
    /* Go into run mode */
    run_done = 0;
    while ( !run_done )
    {
      maxrun = i;

      run = 0;
      done = 0;
      while ( !done )
      {
        if ( !scoeff[i] )
        {
          run++;
        }
        else
        {
          quantCoeffInfo[prevCoeffInd].run=run;
          quantCoeffInfo[prevCoeffInd].maxrun=maxrun;
          quantCoeffInfo[prevCoeffInd].nextLev=(abs(scoeff[i]) == 1) ? 0 : 1;
          quantCoeffInfo[prevCoeffInd].nexLevelVal=scoeff[i];

          prevCoeffInd = i;

          run = 0;
          done = 1;
        }
        if (i == 0)
        {
          quantCoeffInfo[prevCoeffInd].run=run;
          quantCoeffInfo[prevCoeffInd].maxrun=maxrun;
          quantCoeffInfo[prevCoeffInd].nextLev=0;
          quantCoeffInfo[prevCoeffInd].nexLevelVal=0;

          done = 1;
          run_done = 1;
        }
        i--;
      }
    }
  }

#if CAVLC_COEF_LRG_BLK
  const UInt *vlcTableIntra = (N<=8)? g_auiVlcTable8x8Intra:g_auiVlcTable16x16Intra;
  const UInt *vlcTableInter = (N<=8)? g_auiVlcTable8x8Inter:g_auiVlcTable16x16Inter;
  const UInt *pLumaRunTr1 = (N==4)? uiLumaRunNoTr14x4:uiLumaRunNoTr18x8;
#endif
  for (i = last_pos_init; i >= 0; i--)
  {
    if (scoeff[i])
    {
      bitsLast[i] = bitsRun[i] = 0; 

      last_pos = i;
      {
        int x,cx,vlcNum;
        int vlcTable[3] = {2,2,2};

        bitsLevel=0;
        absLevel = abs(scoeff[i]);
        sign = (scoeff[i] < 0) ? 1 : 0;
        lev = (absLevel == 1) ? 0 : 1;

        if (absLevel > 1)
        {
          bitsLevel=xCountVlcBits( 0, 2*(absLevel-2)+sign );
        }
        else
        {
          bitsLevel++;
        }

        x = uiNoCoeff*lev + last_pos;

        if ( uiNoCoeff == 16 )
        {
          cx = m_uiLPTableE4[n * 32 + x];
          vlcNum = vlcTable[n];
        }
        else
        {
#if CAVLC_COEF_LRG_BLK
          cx = xLastLevelInd(lev, last_pos, N);
#else
          cx = m_uiLPTableE8[n * 128 + x];
#endif
          vlcNum = g_auiLastPosVlcNum[n][min(16u,m_uiLastPosVlcIndex[n])];
        }
        bitsLast[i]=bitsLevel + xCountVlcBits( vlcNum, cx);
      }

      bitsRun[i]=0;

      if ( i > 0 )
      {
        bitsLevel = 0;
        if ( quantCoeffInfo[i].nexLevelVal != 0 )
        {
          absLevel = abs(quantCoeffInfo[i].nexLevelVal);
          sign = (quantCoeffInfo[i].nexLevelVal < 0) ? 1 : 0;
          lev = (absLevel == 1) ? 0 : 1;

          if (absLevel > 1)
          {
            bitsLevel = xCountVlcBits( 0, 2 * (absLevel - 2) + sign );
          }
          else
          {
            bitsLevel++;
          }
        }

        bitsRun[i] = bitsLevel;
        run = quantCoeffInfo[i].run;
        maxrun = quantCoeffInfo[i].maxrun;

#if CAVLC_COEF_LRG_BLK 
        Int tmprun = min(maxrun,28);
        if(nTab == 2 || nTab == 5)
        {
          vlc = vlcTableIntra[tmprun]; 
          cn = xRunLevelInd(quantCoeffInfo[i].nextLev, run, maxrun, pLumaRunTr1[tmprun]);
        }
        else
        {
          vlc = vlcTableInter[tmprun]; 
          cn = xRunLevelIndInter(quantCoeffInfo[i].nextLev, run, maxrun);
        }
        bitsRun[i] += xCountVlcBits( vlc, cn );
#else
        if ( uiNoCoeff == 16 )
        {
          if ( nTab == 2 )
          {
            vlc = g_auiVlcTable8x8Intra[maxrun];        
            cn = xRunLevelInd(quantCoeffInfo[i].nextLev, run, maxrun, uiLumaRunNoTr14x4[maxrun]);
          }
          else
          {
            vlc = g_auiVlcTable8x8Inter[maxrun];           
#if RUNLEVEL_TABLE_CUT
            cn = xRunLevelIndInter(quantCoeffInfo[i].nextLev, run, maxrun);
#else
            cn = g_auiLumaRun8x8[maxrun][quantCoeffInfo[i].nextLev][run];
#endif
          }
          bitsRun[i] += xCountVlcBits( vlc, cn );
        }
        else
        {
          if(nTab == 2 || nTab == 5)
          {
            vlc = g_auiVlcTable8x8Intra[min(maxrun,28)];          
            cn = xRunLevelInd(quantCoeffInfo[i].nextLev, run, maxrun, uiLumaRunNoTr18x8[min(maxrun,28)]);
          }
          else
          {
            vlc = g_auiVlcTable8x8Inter[min(maxrun,28)];           
#if RUNLEVEL_TABLE_CUT
            cn = xRunLevelIndInter(quantCoeffInfo[i].nextLev, run, maxrun); 
#else
            cn = g_auiLumaRun8x8[min(maxrun,28)][quantCoeffInfo[i].nextLev][run];
#endif
          }
          bitsRun[i] += xCountVlcBits( vlc, cn );
        }
#endif
      }
    }
  }

  lagrMin=0; lastPosMin=-1; 
  for (i=0; i<uiNoCoeff; i++)
  {
    if ( scoeff[i] != 0 )
    {
      lagrMin += levelData[i].errLevel[0];
    }
  }

  UInt first=1; 

  bitsLastPrev=0; lagrPrev=lagrMin;
  for (i=0; i<uiNoCoeff; i++)
  {
    if (scoeff[i])
    {
      iRate = (bitsRun[i] + bitsLast[i] - bitsLastPrev) << uiBitShift;
      lagr = lagrPrev-levelData[i].errLevel[0] + levelData[i].errLevel[levelData[i].quantInd] + m_dLambda*iRate;
      bitsLastPrev = bitsLast[i];
      lagrPrev = lagr;

      if ( lagr < lagrMin || abs(scoeff[i]) > 1 || first == 1)
      {
        lagrMin = lagr;
        lastPosMin =i;
        first = 0;
      }
    }
  }

  return(lastPosMin);
}
    
static levelDataStruct slevelData  [ MAX_CU_SIZE*MAX_CU_SIZE ];
Void TComTrQuant::xRateDistOptQuant_LCEC(TComDataCU* pcCU, Long* pSrcCoeff, TCoeff*& pDstCoeff, UInt uiWidth, UInt uiHeight, UInt& uiAbsSum, TextType eTType, 
                                         UInt uiAbsPartIdx )
{
  Int     i, j;
  Int     iShift = 0;
  Double  err, lagr, lagrMin;
  Double  fTemp = 0.0;
  Int     iQuantCoeff;
  
  Int     q_bits;
  Int     iShiftQBits, iSign, iRate, lastPosMin, iBlockType;
  UInt    uiBitShift = SCALE_BITS, uiScanPos, levelInd;
  Int     levelBest, iLevel;
  
  levelDataStruct* levelData = &slevelData[0];
  
  Int     iPos, iScanning;
  
#if CAVLC_COEF_LRG_BLK
  static TCoeff sQuantCoeff[1024];
#else
  static TCoeff sQuantCoeff[256];
#endif
  
  q_bits    = m_cQP.m_iBits;
  
  UInt noCoeff=(uiWidth < 8 ? 16 : 64);
#if CAVLC_COEF_LRG_BLK
  UInt maxBlSize = (eTType==TEXT_LUMA)? 32:8;
  UInt uiBlSize = min(uiWidth,maxBlSize);
  noCoeff = uiBlSize*uiBlSize;
#endif
  
  UInt uiLog2TrSize = g_aucConvertToBit[ uiWidth ] + 2;
  UInt uiQ = g_auiQ[m_cQP.rem()];
  
#if FULL_NBIT
  UInt uiBitDepth = g_uiBitDepth;
#else
  UInt uiBitDepth = g_uiBitDepth + g_uiBitIncrement;
#endif
  Int iTransformShift = MAX_TR_DYNAMIC_RANGE - uiBitDepth - uiLog2TrSize;  // Represents scaling through forward transform
  double dErrScale = (double)(1<<uiBitShift);                              // Compensate for scaling of bitcount in Lagrange cost function
  dErrScale = dErrScale*pow(2.0,-2.0*iTransformShift);                     // Compensate for scaling through forward transform
  dErrScale = dErrScale/(double)(uiQ*uiQ);                                 // Compensate for qp-dependent multiplier applied before calculating the Lagrange cost function
  dErrScale = dErrScale/(double)(1<<(2*g_uiBitIncrement));                   // Compensate for Lagrange multiplier that is tuned towards 8-bit input
  
  q_bits = QUANT_SHIFT + m_cQP.m_iPer + iTransformShift;                   // Right shift of non-RDOQ quantizer;  level = (coeff*uiQ + offset)>>q_bits
  
  iShift = uiLog2TrSize;
  if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
  {
    iBlockType = eTType-2;
  }
  else
  {
    iBlockType = (uiWidth < 16 ? 2 : 5) + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() ); 
  }
  memset(&pDstCoeff[0],0,uiWidth*uiHeight*sizeof(TCoeff)); 
  
  iShiftQBits = (1 <<( q_bits - 1));
  
  
#if QC_MDCS
#if CAVLC_COEF_LRG_BLK
  UInt uiLog2BlkSize = g_aucConvertToBit[ pcCU->isIntra( uiAbsPartIdx ) ? uiWidth : uiBlSize   ] + 2;
#else
  UInt uiLog2BlkSize = g_aucConvertToBit[ pcCU->isIntra( uiAbsPartIdx ) ? uiWidth : min(8,uiWidth)    ] + 2;
#endif
  const UInt uiScanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, eTType==TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
#else
  const UInt* pucScan;
  if( !pcCU->isIntra(uiAbsPartIdx ))
  {
#if CAVLC_COEF_LRG_BLK
    pucScan = g_auiFrameScanXY[ g_aucConvertToBit[ uiBlSize ] + 1];
#else
    pucScan = g_auiFrameScanXY[ g_aucConvertToBit[ min(uiWidth, 8) ] + 1];
#endif
  }
  else
  {
    pucScan = g_auiFrameScanXY[ g_aucConvertToBit[ uiWidth ] + 1];
  }
#endif //QC_MDCS
  
  
  UInt uiShift_local = iShift;
  UInt uiRes_local = (uiWidth - 1);
  UInt uiWidth_local = uiShift_local;
  
#if CAVLC_COEF_LRG_BLK
  if(!pcCU->isIntra(uiAbsPartIdx) && uiWidth > maxBlSize)
  {
    uiShift_local = g_aucConvertToBit[ maxBlSize ] + 2;;
    uiRes_local = maxBlSize - 1; 
  }
#else
  if( !pcCU->isIntra(uiAbsPartIdx) && uiWidth >= 16 )
  {
    uiShift_local = 3;
    uiRes_local = 7; 
  }
#endif
  
  Int iAddRDOQ = 0;
  /* Code below is consistent with JCTVC-E243 but could preferably be replaced with iAddRDOQ = 171 << (q_bits-9); */
  if (q_bits>=15)
  {
    iAddRDOQ = (uiWidth<16 ? 10922 : 10880) << (q_bits-15);
  }
  else
  {
    iAddRDOQ = (uiWidth<16 ? 10922 : 10880) >> (15-q_bits);
  }
  if (m_uiRDOQOffset==1)
    iAddRDOQ=iShiftQBits;
  
  for (iScanning=noCoeff-1; iScanning>=0; iScanning--) 
  {
#if QC_MDCS
    iPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][iScanning];
#else
    iPos = pucScan[iScanning];
#endif //QC_MDCS
    j = iPos >> uiShift_local;
    i = iPos &  uiRes_local;
    iPos = (j << uiWidth_local) + i;
    
    levelDataStruct *psLevelData = &levelData[iScanning];
    psLevelData->levelDouble = abs(pSrcCoeff[iPos]) * uiQ;          
    iQuantCoeff = (Int)((psLevelData->levelDouble + iAddRDOQ) >> q_bits);
    
    psLevelData->levelQ   = (Long)( psLevelData->levelDouble >> q_bits );
    psLevelData->lowerInt = ( ( psLevelData->levelDouble - (psLevelData->levelQ << q_bits) ) < iShiftQBits ) ? true : false;
    
    iSign = pSrcCoeff[iPos] < 0 ? -1 : 1;
    sQuantCoeff[iScanning] = iQuantCoeff*iSign;
    
    fTemp = dErrScale;
    psLevelData->level[0] = 0;
    err = (Double)(psLevelData->levelDouble);
    psLevelData->errLevel[0] = err * err * fTemp;
    
    if ( !psLevelData->levelQ )
    {
      if ( psLevelData->lowerInt )
      {
        psLevelData->noLevels = 1;
      }
      else
      {
        psLevelData->level[1] = 1;
        psLevelData->noLevels = 2;
      }
      if (iQuantCoeff==0)
        psLevelData->quantInd=0;
      else
        psLevelData->quantInd=1;
      
    }
    else if ( psLevelData->lowerInt )
    {
      psLevelData->level[1] = psLevelData->levelQ;
      psLevelData->noLevels = 2;
      
      if ( psLevelData->levelQ > 1 )
      {
        psLevelData->noLevels++;
        psLevelData->level[2] = 1;
      }
      
      psLevelData->quantInd = 1;
    }
    else
    {
      psLevelData->level[1] = psLevelData->levelQ;
      psLevelData->level[2] = psLevelData->levelQ + 1;
      psLevelData->noLevels = 3;
      
      if ( psLevelData->levelQ > 1 )
      {
        psLevelData->noLevels++;
        psLevelData->level[3] = 1;
      }
      if ( iQuantCoeff == psLevelData->level[1] )
        psLevelData->quantInd = 1;
      else
        psLevelData->quantInd = 2;
    }
    
    for ( levelInd = 1; levelInd < psLevelData->noLevels; levelInd++ )
    {
      err = (Double)((psLevelData->level[levelInd] << q_bits) - psLevelData->levelDouble);
      psLevelData->errLevel[levelInd] = err * err * fTemp;
      psLevelData->level[levelInd] *= iSign;
    }
  }
  
#if CAVLC_COEF_LRG_BLK==0
  UInt uiNum;
  if ( uiWidth == 4 )
    uiNum = 4;
  else
    uiNum = 8;
#endif
  
  // Last Position
#if CAVLC_COEF_LRG_BLK
  lastPosMin = xCodeCoeffCountBitsLast(sQuantCoeff, levelData, iBlockType, uiBlSize);
#else
  lastPosMin = xCodeCoeffCountBitsLast(sQuantCoeff, levelData, iBlockType, uiNum);
#endif
  memset(&sQuantCoeff[lastPosMin+1],0,sizeof(TCoeff) * (noCoeff - (lastPosMin + 1)));
  
  
  Int  iLpFlag = 1; 
  Int  iLevelMode = 0;
  Int  iRun = 0;
  Int  iVlc_adaptive = 0;
  Int  iMaxrun = 0;
  Int  iSum_big_coef = 0;
  
  
  UInt uiTr1=0;
  UInt absBestLevel;
  
  Int atable[5] = {4,6,14,28,0xfffffff};
  Int switch_thr[10] = {49,49,0,49,49,0,49,49,49,49};
  
  Int iRateMin=0, levelStart;
  Double lagrCoded=0, lagrNotCoded=0;
#if CAVLC_COEF_LRG_BLK
  const UInt **pLumaRunTr1 = (uiWidth==4)? g_pLumaRunTr14x4:g_pLumaRunTr18x8;
  UInt coeffBlkSize = (uiWidth==4)? 4:(noCoeff==64)? 8:(noCoeff==256)? 16:32;
#endif
  
  for (iScanning = lastPosMin; iScanning>=0; iScanning--)
  {
    uiScanPos = iScanning;
    levelStart = (iScanning == lastPosMin) ? 1 : 0;
    
    sQuantCoeff[uiScanPos] = levelBest = 0;
    levelDataStruct *psLevelData = &levelData[uiScanPos];
    if ( psLevelData->noLevels >1 || iScanning == 0 )
    {
      lagrMin = 0; iRateMin = 0;
      for (levelInd = levelStart; levelInd < psLevelData->noLevels; levelInd++)
      {
        lagr = psLevelData->errLevel[levelInd];
        iLevel=psLevelData->level[levelInd];
        
#if CAVLC_COEF_LRG_BLK
        iRate = bitCountRDOQ(iLevel,uiScanPos,iBlockType,iLpFlag,iLevelMode,iRun,iMaxrun,iVlc_adaptive,
                             coeffBlkSize,uiTr1, iSum_big_coef, iBlockType, pcCU, pLumaRunTr1)<<uiBitShift;
#else
        if ( uiWidth == 4 )
        {
          iRate = bitCountRDOQ(iLevel,uiScanPos,iBlockType,iLpFlag,iLevelMode,iRun,iMaxrun,iVlc_adaptive,4,uiTr1, 
                               iSum_big_coef,iBlockType, pcCU)<<uiBitShift;
        }
        else
        {
          iRate = bitCountRDOQ(iLevel,uiScanPos,iBlockType,iLpFlag,iLevelMode,iRun,iMaxrun,iVlc_adaptive,8,uiTr1, 
                               iSum_big_coef, iBlockType, pcCU) << uiBitShift;
        }
#endif
        lagr += m_dLambda * iRate; 
        
        if ( lagr < lagrMin || levelInd == levelStart)
        {
          lagrMin = lagr;
          iRateMin = iRate;
          levelBest = iLevel;
        }
      }
    }
    
    if ( levelBest != 0 )
    {
      lagrCoded += lagrMin;
      lagrNotCoded += psLevelData->errLevel[0];
    }
    if ( uiScanPos == 0 && levelBest == 0 )
    {
      lagrCoded += m_dLambda * iRateMin;
    }
    
    sQuantCoeff[uiScanPos] = levelBest;
    
    absBestLevel = abs(levelBest);
    if ( levelBest != 0 )
    {  
      if ( uiWidth > 4 )
      { 
        if ( !iLpFlag && absBestLevel > 1 )
        {
          iSum_big_coef += absBestLevel;
          if ((noCoeff - uiScanPos - 1) > switch_thr[iBlockType] || iSum_big_coef > 2) iLevelMode = 1; 
        }
      }
      else
      {
        if ( absBestLevel > 1 ) iLevelMode = 1;
      }
      
      if ( iLpFlag == 1 )
      {
        uiTr1 = (absBestLevel > 1) ? 0 : 1;
      }
      else
      {
        if ( uiTr1 == 0 || absBestLevel >= 2 )
        { 
          uiTr1 = 0;
        }
        else if ( uiTr1 < MAX_TR1 )
        {
          uiTr1++;
        }
      }
      iMaxrun = iScanning - 1;
      iLpFlag = 0;
      iRun = 0;
      if ( iLevelMode && (absBestLevel > atable[iVlc_adaptive])) iVlc_adaptive++;        
    }
    else
    {
      iRun += 1;         
    }
  }
  
  if (lastPosMin >= 0 && lagrCoded > lagrNotCoded)
  {
    for (iScanning = lastPosMin; iScanning>=0; iScanning--)
    {
      sQuantCoeff[iScanning] = 0;
    }
  }
  
#if CAVLC_COEF_LRG_BLK 
  if ((!pcCU->isIntra(uiAbsPartIdx) && uiWidth > maxBlSize))
  {
    for (iScanning=noCoeff-1; iScanning>=0; iScanning--) 
    {
#if QC_MDCS
      iPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][iScanning];
#else
      iPos = pucScan[iScanning];
#endif //QC_MDCS
      j = iPos >>  (g_aucConvertToBit[ maxBlSize ] + 2);
      i = iPos & (maxBlSize-1);
      iPos = (j<<(g_aucConvertToBit[ uiWidth ] + 2))+i;
      pDstCoeff[iPos] = sQuantCoeff[iScanning];
      uiAbsSum += abs(sQuantCoeff[iScanning]);
    }
  }
#else
  if ((!pcCU->isIntra(uiAbsPartIdx) && uiWidth >= 16))
  {
    for (iScanning = noCoeff - 1; iScanning >= 0; iScanning--) 
    {
#if QC_MDCS
      iPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][iScanning];
#else
      iPos = pucScan[iScanning];
#endif //QC_MDCS
      j = iPos >> 3;
      i = iPos & 0x7;
      iPos = uiWidth * j + i;
      pDstCoeff[iPos] = sQuantCoeff[iScanning];
      uiAbsSum += abs(sQuantCoeff[iScanning]);
    }
  }
#endif
  else
  {
    for (iScanning = noCoeff - 1; iScanning >= 0; iScanning--) 
    {
#if QC_MDCS
      iPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][iScanning];
#else
      iPos = pucScan[iScanning];
#endif //QC_MDCS
      pDstCoeff[iPos] = sQuantCoeff[iScanning];
      uiAbsSum += abs(sQuantCoeff[iScanning]);
    }
  }
}

Void TComTrQuant::xQuantLTR  (TComDataCU* pcCU, Long* pSrc, TCoeff*& pDes, Int iWidth, Int iHeight, UInt& uiAcSum, TextType eTType, UInt uiAbsPartIdx )
{
  Long*   piCoef    = pSrc;
  TCoeff* piQCoef   = pDes;
  Int   iAdd = 0;
  
  if ( m_bUseRDOQ && (eTType == TEXT_LUMA || RDOQ_CHROMA) )
  {
    if ( m_iSymbolMode == 0)
      xRateDistOptQuant_LCEC(pcCU, piCoef, pDes, iWidth, iHeight, uiAcSum, eTType, uiAbsPartIdx );
    else
      xRateDistOptQuant( pcCU, piCoef, pDes, iWidth, iHeight, uiAcSum, eTType, uiAbsPartIdx );
  }
  else
  {
    const UInt*  pucScan;
    UInt uiConvBit = g_aucConvertToBit[ iWidth ];
    pucScan        = g_auiFrameScanXY [ uiConvBit + 1 ];

    UInt uiLog2TrSize = g_aucConvertToBit[ iWidth ] + 2;
    UInt uiQ = g_auiQ[m_cQP.rem()];

#if FULL_NBIT
    UInt uiBitDepth = g_uiBitDepth;
#else
    UInt uiBitDepth = g_uiBitDepth + g_uiBitIncrement;
#endif
    UInt iTransformShift = MAX_TR_DYNAMIC_RANGE - uiBitDepth - uiLog2TrSize;  // Represents scaling through forward transform
    Int iQBits = QUANT_SHIFT + m_cQP.m_iPer + iTransformShift;                // Right shift of non-RDOQ quantizer;  level = (coeff*uiQ + offset)>>q_bits

    iAdd = (pcCU->getSlice()->getSliceType()==I_SLICE ? 171 : 85) << (iQBits-9);

    for( Int n = 0; n < iWidth*iHeight; n++ )
    {
      Long iLevel;
      Int  iSign;
      UInt uiBlockPos = pucScan[n]; 
      iLevel  = (Long) piCoef[uiBlockPos];
      iSign   = (iLevel < 0 ? -1: 1);      

      iLevel = (abs(iLevel) * uiQ + iAdd ) >> iQBits;
#if CAVLC_COEF_LRG_BLK
      if (pcCU->isIntra( uiAbsPartIdx ))
      {
        if (m_iSymbolMode == 0 && n>=64 && eTType != TEXT_LUMA)
        {
          iLevel = 0;
        }
      }
      else   
      {
        if (m_iSymbolMode == 0 && ((uiBlockPos%iWidth)>=8 || (uiBlockPos/iWidth)>=8) && eTType != TEXT_LUMA)
        {
          iLevel = 0;
        }
      }
#else
      if (m_iSymbolMode == 0 && iWidth>8)
      {
        /* Two methods of limiting number of encoded coefficients to 8x8 for intra and inter respectively */
        if (pcCU->isIntra( uiAbsPartIdx ))
        {
          if(n>=64) iLevel = 0;
        }
        else
        {
          if ((uiBlockPos%iWidth)>=8 || (uiBlockPos/iWidth)>=8) iLevel = 0;
        }
      }
#endif
      uiAcSum += iLevel;
      iLevel *= iSign;        
      piQCoef[uiBlockPos] = iLevel;
    } // for n
  } //if RDOQ
  //return;

}

Void TComTrQuant::xDeQuantLTR( TCoeff* pSrc, Long*& pDes, Int iWidth, Int iHeight )
{
  
  TCoeff* piQCoef   = pSrc;
  Long*   piCoef    = pDes;
  
  if ( iWidth > (Int)m_uiMaxTrSize )
  {
    iWidth  = m_uiMaxTrSize;
    iHeight = m_uiMaxTrSize;
  }
  
  Int iShift,iAdd,iCoeffQ;
  UInt uiQ;
  UInt uiLog2TrSize = g_aucConvertToBit[ iWidth ] + 2;

#if FULL_NBIT
  UInt uiBitDepth = g_uiBitDepth;
#else
  UInt uiBitDepth = g_uiBitDepth + g_uiBitIncrement;
#endif
  UInt iTransformShift = MAX_TR_DYNAMIC_RANGE - uiBitDepth - uiLog2TrSize; 
  iShift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - iTransformShift;
  iAdd = 1 << (iShift-1);
  uiQ = g_auiIQ[m_cQP.m_iRem];

  for( Int n = 0; n < iWidth*iHeight; n++ )
  {
    iCoeffQ = ((piQCoef[n]*(Int)uiQ << m_cQP.m_iPer)+iAdd)>>iShift;
    piCoef[n] = Clip3(-32768,32767,iCoeffQ);
  } 
}

Void TComTrQuant::init( UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxTrSize, Int iSymbolMode, UInt *aTableLP4, UInt *aTableLP8, UInt *aTableLastPosVlcIndex,
                       Bool bUseRDOQ,  Bool bEnc )
{
  m_uiMaxTrSize  = uiMaxTrSize;
  m_bEnc         = bEnc;
  m_bUseRDOQ     = bUseRDOQ;
  m_uiLPTableE8 = aTableLP8;
  m_uiLPTableE4 = aTableLP4;
  m_uiLastPosVlcIndex=aTableLastPosVlcIndex;
  m_iSymbolMode = iSymbolMode;  
}

Void TComTrQuant::xQuant( TComDataCU* pcCU, Long* pSrc, TCoeff*& pDes, Int iWidth, Int iHeight, UInt& uiAcSum, TextType eTType, UInt uiAbsPartIdx )
{
  xQuantLTR(pcCU, pSrc, pDes, iWidth, iHeight, uiAcSum, eTType, uiAbsPartIdx );
}

Void TComTrQuant::xDeQuant( TCoeff* pSrc, Long*& pDes, Int iWidth, Int iHeight )
{
  xDeQuantLTR( pSrc, pDes, iWidth, iHeight );
}

#if INTRA_DST_TYPE_7
Void TComTrQuant::transformNxN( TComDataCU* pcCU, Pel* pcResidual, UInt uiStride, TCoeff*& rpcCoeff, UInt uiWidth, UInt uiHeight, UInt& uiAbsSum, TextType eTType, UInt uiAbsPartIdx )
{
  UInt uiMode;  //luma intra pred
  if(eTType == TEXT_LUMA && pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA )
    uiMode = pcCU->getLumaIntraDir( uiAbsPartIdx );
  else
    uiMode = REG_DCT;

  uiAbsSum = 0;
  assert( (pcCU->getSlice()->getSPS()->getMaxTrSize() >= uiWidth) );

  xT( uiMode, pcResidual, uiStride, m_plTempCoeff, uiWidth );
  xQuant( pcCU, m_plTempCoeff, rpcCoeff, uiWidth, uiHeight, uiAbsSum, eTType, uiAbsPartIdx );
}
#else
Void TComTrQuant::transformNxN( TComDataCU* pcCU, Pel* pcResidual, UInt uiStride, TCoeff*& rpcCoeff, UInt uiWidth, UInt uiHeight, UInt& uiAbsSum, TextType eTType, UInt uiAbsPartIdx )
{
  uiAbsSum = 0;
  
  assert( (pcCU->getSlice()->getSPS()->getMaxTrSize() >= uiWidth) );
  
  xT( pcResidual, uiStride, m_plTempCoeff, uiWidth );
  xQuant( pcCU, m_plTempCoeff, rpcCoeff, uiWidth, uiHeight, uiAbsSum, eTType, uiAbsPartIdx );
}
#endif


#if INTRA_DST_TYPE_7
Void TComTrQuant::invtransformNxN( TextType eText,UInt uiMode, Pel*& rpcResidual, UInt uiStride, TCoeff* pcCoeff, UInt uiWidth, UInt uiHeight )
{
  xDeQuant( pcCoeff, m_plTempCoeff, uiWidth, uiHeight);
  xIT( uiMode, m_plTempCoeff, rpcResidual, uiStride, uiWidth);
}
#else
Void TComTrQuant::invtransformNxN( Pel*& rpcResidual, UInt uiStride, TCoeff* pcCoeff, UInt uiWidth, UInt uiHeight )
{
  xDeQuant( pcCoeff, m_plTempCoeff, uiWidth, uiHeight);
  xIT( m_plTempCoeff, rpcResidual, uiStride, uiWidth );
}
#endif

Void TComTrQuant::invRecurTransformNxN( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eTxt, Pel*& rpcResidual, UInt uiAddr, UInt uiStride, UInt uiWidth, UInt uiHeight, UInt uiMaxTrMode, UInt uiTrMode, TCoeff* rpcCoeff )
{
  if( !pcCU->getCbf(uiAbsPartIdx, eTxt, uiTrMode) )
    return;
  
  UInt uiLumaTrMode, uiChromaTrMode;
  pcCU->convertTransIdx( uiAbsPartIdx, pcCU->getTransformIdx( uiAbsPartIdx ), uiLumaTrMode, uiChromaTrMode );
  const UInt uiStopTrMode = eTxt == TEXT_LUMA ? uiLumaTrMode : uiChromaTrMode;
  
  assert(1); // as long as quadtrees are not used for residual transform
  
  if( uiTrMode == uiStopTrMode )
  {
    UInt uiDepth      = pcCU->getDepth( uiAbsPartIdx ) + uiTrMode;
    UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth ] + 2;
    if( eTxt != TEXT_LUMA && uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
    {
      UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
      if( ( uiAbsPartIdx % uiQPDiv ) != 0 )
      {
        return;
      }
      uiWidth  <<= 1;
      uiHeight <<= 1;
    }
    Pel* pResi = rpcResidual + uiAddr;
#if INTRA_DST_TYPE_7
    invtransformNxN( eTxt, REG_DCT, pResi, uiStride, rpcCoeff, uiWidth, uiHeight );
#else
    invtransformNxN( pResi, uiStride, rpcCoeff, uiWidth, uiHeight );
#endif
  }
  else
  {
    uiTrMode++;
    uiWidth  >>= 1;
    uiHeight >>= 1;
    UInt uiAddrOffset = uiHeight * uiStride;
    UInt uiCoefOffset = uiWidth * uiHeight;
    UInt uiPartOffset = pcCU->getTotalNumPart() >> (uiTrMode<<1);
    invRecurTransformNxN( pcCU, uiAbsPartIdx, eTxt, rpcResidual, uiAddr                         , uiStride, uiWidth, uiHeight, uiMaxTrMode, uiTrMode, rpcCoeff ); rpcCoeff += uiCoefOffset; uiAbsPartIdx += uiPartOffset;
    invRecurTransformNxN( pcCU, uiAbsPartIdx, eTxt, rpcResidual, uiAddr + uiWidth               , uiStride, uiWidth, uiHeight, uiMaxTrMode, uiTrMode, rpcCoeff ); rpcCoeff += uiCoefOffset; uiAbsPartIdx += uiPartOffset;
    invRecurTransformNxN( pcCU, uiAbsPartIdx, eTxt, rpcResidual, uiAddr + uiAddrOffset          , uiStride, uiWidth, uiHeight, uiMaxTrMode, uiTrMode, rpcCoeff ); rpcCoeff += uiCoefOffset; uiAbsPartIdx += uiPartOffset;
    invRecurTransformNxN( pcCU, uiAbsPartIdx, eTxt, rpcResidual, uiAddr + uiAddrOffset + uiWidth, uiStride, uiWidth, uiHeight, uiMaxTrMode, uiTrMode, rpcCoeff );
  }
}

// ------------------------------------------------------------------------------------------------
// Logical transform
// ------------------------------------------------------------------------------------------------

/** Wrapper function between HM interface and core NxN forward transform (2D) 
 *  \param piBlkResi input data (residual)
 *  \param psCoeff output data (transform coefficients)
 *  \param uiStride stride of input residual data
 *  \param iSize transform size (iSize x iSize)
 *  \param uiMode is Intra Prediction mode used in Mode-Dependent DCT/DST only
 */
#if INTRA_DST_TYPE_7
Void TComTrQuant::xT( UInt uiMode, Pel* piBlkResi, UInt uiStride, Long* psCoeff, Int iSize )
#else
Void TComTrQuant::xT( Pel* piBlkResi, UInt uiStride, Long* psCoeff, Int iSize )
#endif
{
#if MATRIX_MULT  
#if INTRA_DST_TYPE_7
  xTr(piBlkResi,psCoeff,uiStride,(UInt)iSize,uiMode);
#else
  xTr(piBlkResi,psCoeff,uiStride,(UInt)iSize);
#endif
#else
  Int j,k;
  if (iSize==4)
  {   
    short block[4][4];   
    short coeff[4][4];
    for (j=0; j<4; j++)
    {    
      memcpy(block[j],piBlkResi+j*uiStride,4*sizeof(short));      
    }
#if INTRA_DST_TYPE_7
    xTr4(block,coeff,uiMode);
#else
    xTr4(block,coeff);     
#endif
    for (j=0; j<4; j++)
    {    
      for (k=0; k<4; k++)
      {        
        psCoeff[j*4+k] = coeff[j][k];
      }    
    }    
  }
  else if (iSize==8)
  {
    short block[8][8];
    short coeff[8][8];

    for (j=0; j<8; j++)
    {    
      memcpy(block[j],piBlkResi+j*uiStride,8*sizeof(short));
    }

    xTr8(block,coeff);       
    for (j=0; j<8; j++)
    {    
      for (k=0; k<8; k++)
      {        
        psCoeff[j*8+k] = coeff[j][k];
      }    
    }
  }
  else if (iSize==16)
  {   
    short block[16][16];
    short coeff[16][16];

    for (j=0; j<16; j++)
    {    
      memcpy(block[j],piBlkResi+j*uiStride,16*sizeof(short));
    }
    xTr16(block,coeff);       
    for (j=0; j<16; j++)
    {    
      for (k=0; k<16; k++)
      {        
        psCoeff[j*16+k] = coeff[j][k];
      }    
    }
  }
  else if (iSize==32)
  {   
    short block[32][32];
    short coeff[32][32];

    for (j=0; j<32; j++)
    {    
      memcpy(block[j],piBlkResi+j*uiStride,32*sizeof(short));
    }
    xTr32(block,coeff);       
    for (j=0; j<32; j++)
    {    
      for (k=0; k<32; k++)
      {        
        psCoeff[j*32+k] = coeff[j][k];
      }    
    }
  }
#endif  
}

/** Wrapper function between HM interface and core NxN inverse transform (2D) 
 *  \param plCoef input data (transform coefficients)
 *  \param pResidual output data (residual)
 *  \param uiStride stride of input residual data
 *  \param iSize transform size (iSize x iSize)
 *  \param uiMode is Intra Prediction mode used in Mode-Dependent DCT/DST only
 */
#if INTRA_DST_TYPE_7
Void TComTrQuant::xIT( UInt uiMode, Long* plCoef, Pel* pResidual, UInt uiStride, Int iSize )
#else
Void TComTrQuant::xIT( Long* plCoef, Pel* pResidual, UInt uiStride, Int iSize )
#endif
{
#if MATRIX_MULT  
#if INTRA_DST_TYPE_7
  xITr(plCoef,pResidual,uiStride,(UInt)iSize,uiMode);
#else
  xITr(plCoef,pResidual,uiStride,(UInt)iSize);
#endif
#else
  Int j,k;
  if (iSize==4)
  {    
    short block[4][4];
    short coeff[4][4];

    for (j=0; j<4; j++)
    {    
      for (k=0; k<4; k++)
      {        
        coeff[j][k] = (short)plCoef[j*4+k];
      }    
    }
#if INTRA_DST_TYPE_7
    xITr4(coeff,block,uiMode);
#else
    xITr4(coeff,block);       
#endif
    for (j=0; j<4; j++)
    {    
      memcpy(pResidual+j*uiStride,block[j],4*sizeof(short));
    }    
  }
  else if (iSize==8)
  {
    short block[8][8];
    short coeff[8][8];

    for (j=0; j<8; j++)
    {    
      for (k=0; k<8; k++)
      {        
        coeff[j][k] = (short)plCoef[j*8+k];
      }    
    }
    xITr8(coeff,block);       
    for (j=0; j<8; j++)
    {    
      memcpy(pResidual+j*uiStride,block[j],8*sizeof(short));
    }
  }
  else if (iSize==16)
  {
    short block[16][16];
    short coeff[16][16];

    for (j=0; j<16; j++)
    {    
      for (k=0; k<16; k++)
      {        
        coeff[j][k] = (short)plCoef[j*16+k];
      }    
    }
    xITr16(coeff,block);       
    for (j=0; j<16; j++)
    {    
      memcpy(pResidual+j*uiStride,block[j],16*sizeof(short));
    }
  }

  else if (iSize==32)
  {
    short block[32][32];
    short coeff[32][32];

    for (j=0; j<32; j++)
    {    
      for (k=0; k<32; k++)
      {        
        coeff[j][k] = (short)plCoef[j*32+k];
      }    
    }
    xITr32(coeff,block);       
    for (j=0; j<32; j++)
    {    
      memcpy(pResidual+j*uiStride,block[j],32*sizeof(short));
    }   
  }
#endif  
}
    
#if QC_MDCS
UInt TComTrQuant::getCurrLineNum(UInt uiScanIdx, UInt uiPosX, UInt uiPosY)
{
  UInt uiLineNum = 0;
  
  switch (uiScanIdx)
  {
    case SCAN_ZIGZAG:
      uiLineNum = uiPosY + uiPosX;
      break;
    case SCAN_HOR:
      uiLineNum = uiPosY;
      break;
    case SCAN_VER:
      uiLineNum = uiPosX;
      break;
  }
  
  return uiLineNum;
}
#endif

/** RDOQ with CABAC
 * \param pcCU pointer to coding unit structure
 * \param plSrcCoeff pointer to input buffer
 * \param piDstCoeff reference to pointer to output buffer
 * \param uiWidth block width
 * \param uiHeight block height
 * \param uiAbsSum reference to absolute sum of quantized transform coefficient
 * \param eTType plane type / luminance or chrominance
 * \param uiAbsPartIdx absolute partition index
 * \returns Void
 * Rate distortion optimized quantization for entropy
 * coding engines using probability models like CABAC
 */
Void TComTrQuant::xRateDistOptQuant                 ( TComDataCU*                     pcCU,
                                                      Long*                           plSrcCoeff,
                                                      TCoeff*&                        piDstCoeff,
                                                      UInt                            uiWidth,
                                                      UInt                            uiHeight,
                                                      UInt&                           uiAbsSum,
                                                      TextType                        eTType,
                                                      UInt                            uiAbsPartIdx )
{
  Int    iQBits      = m_cQP.m_iBits;
  Double dTemp       = 0;
  
  UInt uiLog2TrSize = g_aucConvertToBit[ uiWidth ] + 2;
  UInt uiQ = g_auiQ[m_cQP.rem()];

#if FULL_NBIT
  UInt uiBitDepth = g_uiBitDepth;
#else
  UInt uiBitDepth = g_uiBitDepth + g_uiBitIncrement;  
#endif
  Int iTransformShift = MAX_TR_DYNAMIC_RANGE - uiBitDepth - uiLog2TrSize;  // Represents scaling through forward transform
  double dErrScale = (double)(1<<SCALE_BITS);                              // Compensate for scaling of bitcount in Lagrange cost function
  dErrScale = dErrScale*pow(2.0,-2.0*iTransformShift);                     // Compensate for scaling through forward transform
  dErrScale = dErrScale/(double)(uiQ*uiQ);                                 // Compensate for qp-dependent multiplier applied before calculating the Lagrange cost function
  dErrScale = dErrScale/(double)(1<<(2*g_uiBitIncrement));                   // Compensate for Lagrange multiplier that is tuned towards 8-bit input

  iQBits = QUANT_SHIFT + m_cQP.m_iPer + iTransformShift;                   // Right shift of non-RDOQ quantizer;  level = (coeff*uiQ + offset)>>q_bits

#if E253
  UInt       uiGoRiceParam       = 0;
#endif
#if PCP_SIGMAP_SIMPLE_LAST
  UInt       uiLastScanPos       = 0;
#else
  UInt       uiMaxLineNum        = 0;
#endif
  Double     d64BlockUncodedCost = 0;
  const UInt uiLog2BlkSize       = g_aucConvertToBit[ uiWidth ] + 2;
  const UInt uiMaxNumCoeff       = 1 << ( uiLog2BlkSize << 1 );
  const UInt uiNum4x4Blk         = max<UInt>( 1, uiMaxNumCoeff >> 4 );
#if QC_MDCS
  const UInt uiScanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, eTType==TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
#endif //QC_MDCS
  
  Int  piCoeff      [ MAX_CU_SIZE * MAX_CU_SIZE ];
  Long plLevelDouble[ MAX_CU_SIZE * MAX_CU_SIZE ];
#if E253
  UInt puiEstParams [ 32*32*3 ];

  ::memset( piDstCoeff,    0, sizeof(TCoeff) *   uiMaxNumCoeff        );
  ::memset( piCoeff,       0, sizeof(Int)    *   uiMaxNumCoeff        );
  ::memset( plLevelDouble, 0, sizeof(Long)   *   uiMaxNumCoeff        );
  ::memset( puiEstParams,  0, sizeof(UInt)   *   uiMaxNumCoeff * 3    );

  UInt *puiOneCtx    = puiEstParams;
  UInt *puiAbsCtx    = puiEstParams +   uiMaxNumCoeff;
  UInt *puiAbsGoRice = puiEstParams + ( uiMaxNumCoeff << 1 );
#else
  UInt puiOneCtx    [ MAX_CU_SIZE * MAX_CU_SIZE ];
  UInt puiAbsCtx    [ MAX_CU_SIZE * MAX_CU_SIZE ];
    
  ::memset( piDstCoeff,    0, sizeof(TCoeff) *   uiMaxNumCoeff        );
  ::memset( piCoeff,       0, sizeof(Int)    *   uiMaxNumCoeff        );
  ::memset( plLevelDouble, 0, sizeof(Long)   *   uiMaxNumCoeff        );
  ::memset( puiOneCtx,     0, sizeof(UInt)   *   uiMaxNumCoeff        );
  ::memset( puiAbsCtx,     0, sizeof(UInt)   *   uiMaxNumCoeff        );
#endif

  //===== quantization =====
  for( UInt uiScanPos = 0; uiScanPos < uiMaxNumCoeff; uiScanPos++ )
  {
#if QC_MDCS
    UInt    uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanPos];  
#else
    UInt    uiBlkPos = g_auiFrameScanXY[ uiLog2BlkSize-1 ][ uiScanPos ];
#endif

    Long lLevelDouble = plSrcCoeff[ uiBlkPos ];

#if !PCP_SIGMAP_SIMPLE_LAST
    UInt    uiPosY   = uiBlkPos >> uiLog2BlkSize;
    UInt    uiPosX   = uiBlkPos - ( uiPosY << uiLog2BlkSize );
#endif
    dTemp = dErrScale;  
    lLevelDouble = abs(lLevelDouble * (Long)uiQ);   
    plLevelDouble[ uiBlkPos ] = lLevelDouble;
    UInt uiMaxAbsLevel = (lLevelDouble + (1L << (iQBits - 1))) >> iQBits;
    
    Double dErr          = Double( lLevelDouble );
    d64BlockUncodedCost += dErr * dErr * dTemp;
    
    piCoeff[ uiBlkPos ] = plSrcCoeff[ uiBlkPos ] > 0 ? uiMaxAbsLevel : -Int( uiMaxAbsLevel );
    
    if ( uiMaxAbsLevel > 0 )
    {
#if PCP_SIGMAP_SIMPLE_LAST
      uiLastScanPos = uiScanPos;
#else
#if QC_MDCS
      UInt uiLineNum = getCurrLineNum(uiScanIdx, uiPosX, uiPosY);
#else
      UInt uiLineNum = uiPosY + uiPosX;
#endif //QC_MDCS
      
      if( uiLineNum > uiMaxLineNum )
      {
        uiMaxLineNum = uiLineNum;
      }
#endif
    }    
  }
  
#if PCP_SIGMAP_SIMPLE_LAST
  uiLastScanPos++;
#endif

  //===== estimate context models =====
  if ( uiNum4x4Blk > 1 )
  {
    Bool bFirstBlock  = true;
    UInt uiNumOne = 0;
    
    for( UInt uiSubBlk = 0; uiSubBlk < uiNum4x4Blk; uiSubBlk++ )
    {
      UInt uiCtxSet    = 0;
      UInt uiSubNumSig = 0;
      UInt uiSubPosX   = 0;
      UInt uiSubPosY   = 0;
#if E253
      uiGoRiceParam    = 0;
#endif

      uiSubPosX = g_auiFrameScanX[ uiLog2BlkSize - 3 ][ uiSubBlk ] << 2;
      uiSubPosY = g_auiFrameScanY[ uiLog2BlkSize - 3 ][ uiSubBlk ] << 2;
      
      Int offset = uiSubPosX + (uiSubPosY << uiLog2BlkSize);
      Int* piCurr = &piCoeff[ offset ];
      
      for( UInt uiY = 0; uiY < 4; uiY++ )
      {
        for( UInt uiX = 0; uiX < 4; uiX++ )
        {
          if( piCurr[ uiY * uiWidth + uiX ] )
          {
            uiSubNumSig++;
          }
        }
      }
      
      if( uiSubNumSig > 0 )
      {
        Int c1 = 1;
        Int c2 = 0;
        
        if( bFirstBlock )
        {
          bFirstBlock = false;
          uiCtxSet = 5;
        }
        else
        {
          uiCtxSet = ( uiNumOne >> 2 ) + 1;
          uiNumOne = 0;
        }
        
        for( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos & 3;
          UInt  uiIndex   = (uiPosY << uiLog2BlkSize) + uiPosX + offset;
          
          if( piCoeff[ uiIndex ]  )
          {
            puiOneCtx[ uiIndex ] = 5 * uiCtxSet + c1;
            puiAbsCtx[ uiIndex ] = 5 * uiCtxSet + c2;
#if E253
            puiAbsGoRice[ uiIndex ] = uiGoRiceParam;
#endif            
            UInt uiAbs = abs( piCoeff[ uiIndex ] );
            
            if( uiAbs > 1 )
            {
              c1 = 0;
              c2 += (c2 < 4);
              uiNumOne++;
#if E253
              if( uiAbs > 2 )
              {
                uiGoRiceParam = g_aauiGoRiceUpdate[ uiGoRiceParam ][ min<UInt>( uiAbs - 3, 15 ) ];
              }
#endif
            }
            else if( c1 & 3 )
            {
              c1++; // Increment c1 if equal to 1, 2 or 3
            }
          }
        }
      }
    }
  }
  else
  {
    Int c1 = 1;
    Int c2 = 0;
    
    for ( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
    {
      UInt uiIndex = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
      
      if( piCoeff[ uiIndex ]  )
      {
        puiOneCtx[ uiIndex ] = c1;
        puiAbsCtx[ uiIndex ] = c2;
#if E253
        puiAbsGoRice[ uiIndex ] = uiGoRiceParam;
#endif
        UInt uiAbs = abs( piCoeff[ uiIndex ] );
        
        if( uiAbs > 1 )
        {
          c1 = 0;
          c2 += (c2 < 4);
#if E253
          if( uiAbs > 2 )
          {
            uiGoRiceParam = g_aauiGoRiceUpdate[ uiGoRiceParam ][ min<UInt>( uiAbs - 3, 15 ) ];
          }
#endif
        }
        else if( c1 & 3 )
        {
          c1++; // Increment c1 if equal to 1, 2 or 3
        }
      }
    }
  }
  
  Int     ui16CtxCbf        = 0;
  UInt    uiBestLastIdxP1   = 0;
  Double  d64BestCost       = 0;
  Double  d64BaseCost       = 0;
  Double  d64CodedCost      = 0;
  Double  d64UncodedCost    = 0;

  if( !pcCU->isIntra( uiAbsPartIdx ) && eTType == TEXT_LUMA && pcCU->getTransformIdx( uiAbsPartIdx ) == 0 )
  {
    ui16CtxCbf  = pcCU->getCtxQtRootCbf( uiAbsPartIdx );
    d64BestCost = d64BlockUncodedCost + xGetICost( m_pcEstBitsSbac->blockRootCbpBits[ ui16CtxCbf ][ 0 ] );
    d64BaseCost = d64BestCost - xGetICost( m_pcEstBitsSbac->blockRootCbpBits[ ui16CtxCbf ][ 0 ] ) + xGetICost( m_pcEstBitsSbac->blockRootCbpBits[ ui16CtxCbf ][ 1 ] );
  }
  else
  {
    ui16CtxCbf  = pcCU->getCtxQtCbf( uiAbsPartIdx, eTType, pcCU->getTransformIdx( uiAbsPartIdx ) );
    ui16CtxCbf  = ( eTType ? eTType - 1 : eTType ) * NUM_QT_CBF_CTX + ui16CtxCbf;
    d64BestCost = d64BlockUncodedCost + xGetICost( m_pcEstBitsSbac->blockCbpBits[ ui16CtxCbf ][ 0 ] );
    d64BaseCost = d64BestCost - xGetICost( m_pcEstBitsSbac->blockCbpBits[ ui16CtxCbf ][ 0 ] ) + xGetICost( m_pcEstBitsSbac->blockCbpBits[ ui16CtxCbf ][ 1 ] );
  }
  
#if PCP_SIGMAP_SIMPLE_LAST
  Double  d64CostLast        = 0;
  TCoeff  iLastCoeffLevel    = 0;
  UInt    uiBestNonZeroLevel = 0;
  UInt    uiBestLastBlkPos   = 0;

  for( UInt uiScanPos = 0; uiScanPos < uiLastScanPos; uiScanPos++ )
  {
#if QC_MDCS
    UInt   uiBlkPos     = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanPos];
#else
    UInt   uiBlkPos     = g_auiFrameScanXY[ uiLog2BlkSize-1 ][ uiScanPos ];
#endif
    UInt   uiPosY       = uiBlkPos >> uiLog2BlkSize;
    UInt   uiPosX       = uiBlkPos - ( uiPosY << uiLog2BlkSize );

    dTemp = dErrScale;

    UShort  uiCtxSig       = getSigCtxInc( piDstCoeff, uiPosX, uiPosY, uiLog2BlkSize, uiWidth );
    UInt    uiMaxAbsLevel  = abs( piCoeff[ uiBlkPos ] );
#if E253
    UInt    uiLevel        = xGetCodedLevel( d64UncodedCost, d64CodedCost, d64CostLast, uiBestNonZeroLevel, plLevelDouble[ uiBlkPos ], uiMaxAbsLevel, uiCtxSig, puiOneCtx[ uiBlkPos ], puiAbsCtx[ uiBlkPos ], puiAbsGoRice[ uiBlkPos ], iQBits, dTemp );
#else
    UInt    uiLevel        = xGetCodedLevel( d64UncodedCost, d64CodedCost, d64CostLast, uiBestNonZeroLevel, plLevelDouble[ uiBlkPos ], uiMaxAbsLevel, uiCtxSig, puiOneCtx[ uiBlkPos ], puiAbsCtx[ uiBlkPos ], iQBits, dTemp );
#endif
    piDstCoeff[ uiBlkPos ] = plSrcCoeff[ uiBlkPos ] < 0 ? -Int( uiLevel ) : uiLevel;
    d64BaseCost           -= d64UncodedCost;

    if( uiBestNonZeroLevel != 0 )
    {
      d64CostLast        += d64BaseCost;
#if QC_MDCS
      d64CostLast        += uiScanIdx == SCAN_VER ? xGetRateLast( uiPosY, uiPosX ) : xGetRateLast( uiPosX, uiPosY );
#else
      d64CostLast        += xGetRateLast( uiPosX, uiPosY );
#endif
      if( d64CostLast < d64BestCost )
      {
        d64BestCost       = d64CostLast;
        uiBestLastIdxP1   = uiScanPos + 1;
        uiBestLastBlkPos  = uiBlkPos;
        iLastCoeffLevel   = plSrcCoeff[ uiBlkPos ] < 0 ? -Int( uiBestNonZeroLevel ) : uiBestNonZeroLevel;
      }
    }
    d64BaseCost           += d64CodedCost;
  }
  if( uiBestLastBlkPos > 0)
  {
    piDstCoeff[ uiBestLastBlkPos ] = iLastCoeffLevel;
  }
#else
  for( UInt uiScanPos = 0; uiScanPos < uiMaxNumCoeff; uiScanPos++ )
  {
#if QC_MDCS
    UInt uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanPos];  
#else
    UInt   uiBlkPos     = g_auiFrameScanXY[ uiLog2BlkSize-1 ][ uiScanPos ];
#endif //QC_MDCS
    UInt   uiPosY       = uiBlkPos >> uiLog2BlkSize;
    UInt   uiPosX       = uiBlkPos - ( uiPosY << uiLog2BlkSize );
    
#if QC_MDCS
    UInt uiLineNum = getCurrLineNum(uiScanIdx, uiPosX, uiPosY);
    if ( uiLineNum > uiMaxLineNum )
    {
      break;
    }
#else 
    if( uiPosY + uiPosX > uiMaxLineNum )
    {
      break;
    }
#endif //QC_MDCS
    
    dTemp = dErrScale;
    
    UShort  uiCtxSig                = getSigCtxInc( piDstCoeff, uiPosX, uiPosY, uiLog2BlkSize, uiWidth );
    Bool    bLastScanPos            = ( uiScanPos == uiMaxNumCoeff - 1 );
#if E253
    UInt    uiLevel                 = xGetCodedLevel( d64UncodedCost, d64CodedCost, plLevelDouble[ uiBlkPos ], abs( piCoeff[ uiBlkPos ] ), bLastScanPos, uiCtxSig, puiOneCtx[ uiBlkPos ], puiAbsCtx[ uiBlkPos ], puiAbsGoRice[ uiBlkPos ], iQBits, dTemp );
#else
    UInt    uiLevel                 = xGetCodedLevel( d64UncodedCost, d64CodedCost, plLevelDouble[ uiBlkPos ], abs( piCoeff[ uiBlkPos ] ), bLastScanPos, uiCtxSig, puiOneCtx[ uiBlkPos ], puiAbsCtx[ uiBlkPos ], iQBits, dTemp );
#endif
    piDstCoeff[ uiBlkPos ]          = plSrcCoeff[ uiBlkPos ] < 0 ? -Int( uiLevel ) : uiLevel;
    d64BaseCost                    -= d64UncodedCost;
    d64BaseCost                    += d64CodedCost;
    
    if( uiLevel )
    {
      //----- check for last flag -----
      UShort  uiCtxLast             = getLastCtxInc( uiPosX, uiPosY, uiLog2BlkSize );
      Double  d64CostLastZero       = xGetICost( m_pcEstBitsSbac->lastBits[ uiCtxLast ][ 0 ] );
      Double  d64CostLastOne        = xGetICost( m_pcEstBitsSbac->lastBits[ uiCtxLast ][ 1 ] );
      Double  d64CurrIsLastCost     = d64BaseCost + d64CostLastOne;
      d64BaseCost                  += d64CostLastZero;
      
      if( d64CurrIsLastCost < d64BestCost )
      {
        d64BestCost       = d64CurrIsLastCost;
        uiBestLastIdxP1   = uiScanPos + 1;
      }
    }
  }
#endif
  
  //===== clean uncoded coefficients =====
  {
    for( UInt uiScanPos = 0; uiScanPos < uiMaxNumCoeff; uiScanPos++ )
    {
#if QC_MDCS
      UInt uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanPos];  
#else
      UInt uiBlkPos = g_auiFrameScanXY[ uiLog2BlkSize-1 ][ uiScanPos ];
#endif //QC_MDCS
      
      if( uiScanPos < uiBestLastIdxP1 )
      {
        uiAbsSum += abs( piDstCoeff[ uiBlkPos ] );
      }
      else
      {
        piDstCoeff[ uiBlkPos ] = 0;
      }      
    }
  }
}

#if !SONY_SIG_CTX
/** Context derivation process of coeff_abs_significant_flag
 * \param pcCoeff pointer to prior coded transform coefficients
 * \param uiPosX column of current scan position
 * \param uiPosY row of current scan position
 * \param uiLog2BlkSize log2 value of block size
 * \param uiStride stride of the block
 * \returns ctxInc for current scan position
 */
UInt TComTrQuant::getSigCtxInc    ( TCoeff*                         pcCoeff,
                                    const UInt                      uiPosX,
                                    const UInt                      uiPosY,
                                    const UInt                      uiLog2BlkSize,
                                    const UInt                      uiStride )
{
  UInt  uiCtxInc  = 0;
  UInt  uiSizeM1  = ( 1 << uiLog2BlkSize ) - 1;
  if( uiLog2BlkSize <= 3 )
  {
    UInt  uiShift = uiLog2BlkSize > 2 ? uiLog2BlkSize - 2 : 0;
    uiCtxInc      = ( ( uiPosY >> uiShift ) << 2 ) + ( uiPosX >> uiShift );
  }
  else if( uiPosX <= 1 && uiPosY <= 1 )
  {
    uiCtxInc            = ( uiPosY << 1 ) + uiPosX;
  }
  else if( uiPosY == 0 )
  {
    const int*  pData   = &pcCoeff[ uiPosX + uiPosY * uiStride ];
    int         iStride =  uiStride;
    UInt        uiCnt   = ( pData[         -1 ] ? 1 : 0 );
    uiCnt              += ( pData[         -2 ] ? 1 : 0 );
    uiCnt              += ( pData[ iStride -2 ] ? 1 : 0 );
    if( ! (uiPosX & 1) )
    {
      uiCnt            += ( pData[ iStride -1 ] ? 1 : 0 );
    }
    uiCtxInc            = 4 + ( ( uiCnt + 1 ) >> 1 );
  }
  else if( uiPosX == 0 )
  {
    const int*  pData   = &pcCoeff[ uiPosX + uiPosY * uiStride ];
    int         iStride =  uiStride;
    int         iStride2=  iStride << 1;
    UInt        uiCnt   = ( pData[  -iStride  ] ? 1 : 0 );
    uiCnt              += ( pData[  -iStride2 ] ? 1 : 0 );
    uiCnt              += ( pData[ 1-iStride2 ] ? 1 : 0 );
    if( uiPosY & 1 )
    {
      uiCnt            += ( pData[ 1-iStride  ] ? 1 : 0 );
    }
    uiCtxInc            = 7 + ( ( uiCnt + 1 ) >> 1 );
  }
  else
  {
    const int*  pData   = &pcCoeff[ uiPosX + uiPosY * uiStride ];
    int         iStride =  uiStride;
    int         iStride2=  iStride << 1;
    UInt        uiCnt   = ( pData[    -iStride  ] ? 1 : 0 );
    uiCnt              += ( pData[ -1           ] ? 1 : 0 );
    uiCnt              += ( pData[ -1 -iStride  ] ? 1 : 0 );
    if( uiPosX > 1 )
    {
      uiCnt            += ( pData[ -2           ] ? 1 : 0 );
      uiCnt            += ( pData[ -2 -iStride  ] ? 1 : 0 );
      if( uiPosY < uiSizeM1 )
      {
        uiCnt          += ( pData[ -2 +iStride  ] ? 1 : 0 );
      }
    }
    if( uiPosY > 1 )
    {
      uiCnt            += ( pData[    -iStride2 ] ? 1 : 0 );
      uiCnt            += ( pData[ -1 -iStride2 ] ? 1 : 0 );
      if( uiPosX < uiSizeM1 )
      {
        uiCnt          += ( pData[  1 -iStride2 ] ? 1 : 0 );
      }
    }
    if( (uiPosX + uiPosY) & 1 )
    {
      if( uiPosX < uiSizeM1 )
      {
        uiCnt          += ( pData[  1 -iStride  ] ? 1 : 0 );
      }
    }
    else
    {
      if( uiPosY < uiSizeM1 )
      {
        uiCnt          += ( pData[ -1 +iStride  ] ? 1 : 0 );
      }
    }
    uiCtxInc      = 10 + min<UInt>( 4, ( uiCnt + 1 ) >> 1 );
  }
  return uiCtxInc;
}
#else
/** Context derivation process of coeff_abs_significant_flag
 * \param pcCoeff pointer to prior coded transform coefficients
 * \param uiPosX column of current scan position
 * \param uiPosY row of current scan position
 * \param uiLog2BlkSize log2 value of block size
 * \param uiStride stride of the block
 * \returns ctxInc for current scan position
 */
UInt TComTrQuant::getSigCtxInc    ( TCoeff*                         pcCoeff,
                                    const UInt                      uiPosX,
                                    const UInt                      uiPosY,
                                    const UInt                      uiLog2BlkSize,
                                    const UInt                      uiStride )
{
#if SIMPLE_CONTEXT_SIG
  if ( uiLog2BlkSize == 2)
  {
    return 4 * uiPosY + uiPosX;
  }
  
  if ( uiLog2BlkSize == 3 )
  {
    return 15 + 4 * (uiPosY >> 1) + (uiPosX >> 1);
  }
  
  if( uiPosX <= 1 && uiPosY <= 1 )
  {
    return 31 + 2 * uiPosY + uiPosX + ((uiLog2BlkSize > 4) ? 15 : 0);
  }
  
  const Int *pData = pcCoeff + uiPosX + (uiPosY << uiLog2BlkSize);
  Int iStride = uiStride;

  if( uiPosY == 0 )
  {
    return 31 + 4 + (pData[-1] != 0) + (pData[-2] != 0);
  }
  
  if( uiPosX == 0 )
  {
    return 31 + 7 + (pData[-iStride] != 0) + (pData[-2*iStride] != 0);
  }
  
  UInt cnt = (pData[-1] != 0) + (pData[-iStride] != 0) + (pData[-iStride-1] != 0);
  if( uiPosX > 1 )
  {
    cnt += pData[-2] != 0;
  }
  if ( uiPosY > 1 && cnt < 4)
  {
    cnt += pData[-2*iStride] != 0;
  }
  return 31 + 10 + cnt;
#else
  UInt  uiCtxInc  = 0;
  
  if( uiLog2BlkSize <= 3 )
  {
    UInt  uiShift = uiLog2BlkSize > 2 ? uiLog2BlkSize - 2 : 0;
    uiCtxInc      = ( ( uiPosY >> uiShift ) << 2 ) + ( uiPosX >> uiShift );
  }
  else if( uiPosX <= 1 && uiPosY <= 1 )
  {
    uiCtxInc            = ( uiPosY << 1 ) + uiPosX;
  }
  else if( uiPosY == 0 )
  {
    const int*  pData   = &pcCoeff[ uiPosX + uiPosY * uiStride ];
    UInt        uiCnt   = ( pData[         -1 ] ? 1 : 0 );
    uiCnt              += ( pData[         -2 ] ? 1 : 0 );
    uiCtxInc            = 4 + uiCnt;
  }
  else if( uiPosX == 0 )
  {
    const int*  pData   = &pcCoeff[ uiPosX + uiPosY * uiStride ];
    int         iStride =  uiStride;
    int         iStride2=  iStride << 1;
    UInt        uiCnt   = ( pData[  -iStride  ] ? 1 : 0 );
    uiCnt              += ( pData[  -iStride2 ] ? 1 : 0 );
    uiCtxInc            = 7 + uiCnt;
  }
  else
  {
    const int*  pData   = &pcCoeff[ uiPosX + uiPosY * uiStride ];
    int         iStride =  uiStride;
    int         iStride2=  iStride << 1;
    UInt        uiCnt   = ( pData[ -1 -iStride  ] ? 1 : 0 );
    uiCnt              += ( pData[    -iStride  ] ? 1 : 0 );
    uiCnt              += ( pData[ -1           ] ? 1 : 0 );
    if( uiPosX > 1 )
    {
      uiCnt          += ( pData[ -2           ] ? 1 : 0 );
    }
    if ( uiPosY > 1 )
    {
      uiCnt          += ( pData[    -iStride2 ] ? 1 : 0 );
    }
    uiCtxInc            = 10 + min<UInt>( 4, uiCnt);
  }
  return uiCtxInc;
#endif
}
#endif

#if !PCP_SIGMAP_SIMPLE_LAST
/** Context derivation of coeff_abs_last_significant_flag
 * \param uiPosX column of current scan position
 * \param uiPosY row of current scan position
 * \param uiLog2BlkSize log2 value of block size
 * \returns ctxInc for current scan position
 */
UInt TComTrQuant::getLastCtxInc   ( const UInt                      uiPosX,
                                    const UInt                      uiPosY,
                                    const UInt                      uiLog2BlkSize )
{
  if( uiLog2BlkSize <= 2 )
  {
    return ( uiPosY << 2 ) + uiPosX;
  }
  else
  {
    return ( uiPosX + uiPosY ) >> ( uiLog2BlkSize - 3 );
  }
}
#endif

#if E253 && PCP_SIGMAP_SIMPLE_LAST // only valid if both tools are enabled
/** Get the best level in RD sense
 * \param rd64UncodedCost reference to uncoded cost
 * \param rd64CodedCost reference to current coded cost
 * \param rd64CodedLastCost reference to coded cost of coefficient without the significance cost
 * \param uiBestNonZeroLevel !!! not available yet
 * \param lLevelDouble reference to unscaled quantized level
 * \param uiMaxAbsLevel scaled quantized level
 * \param ui16CtxNumSig current ctxInc for coeff_abs_significant_flag
 * \param ui16CtxNumOne current ctxInc for coeff_abs_level_greater1 (1st bin of coeff_abs_level_minus1 in AVC)
 * \param ui16CtxNumAbs current ctxInc for coeff_abs_level_greater2 (remaining bins of coeff_abs_level_minus1 in AVC)
 * \param ui16AbsGoRice current Rice parameter for coeff_abs_level_minus3
 * \param iQBits quantization step size
 * \param dTemp correction factor
 * \param ui16CtxBase current global offset for coeff_abs_level_greater1 and coeff_abs_level_greater2
 * \returns best quantized transform level for given scan position
 * This method calculates the best quantized transform level for a given scan position.
 */
#else
/** Get the best level in RD sense
 * \param rd64UncodedCost reference to uncoded cost
 * \param rd64CodedCost reference to current coded cost
 * \param lLevelDouble reference to unscaled quantized level
 * \param uiMaxAbsLevel scaled quantized level
 * \param bLastScanPos last scan position
 * \param ui16CtxNumSig current ctxInc for coeff_abs_significant_flag
 * \param ui16CtxNumOne current ctxInc for coeff_abs_level_greater1 (1st bin of coeff_abs_level_minus1 in AVC)
 * \param ui16CtxNumAbs current ctxInc for coeff_abs_level_minus2 (remaining bins of coeff_abs_level_minus1 in AVC)
 * \param iQBits quantization step size
 * \param dTemp correction factor
 * \param ui16CtxBase current global offset for coeff_abs_level_greater1 and coeff_abs_level_minus2
 * \returns best quantized transform level for given scan position
 * This method calculates the best quantized transform level for a given scan position.
 */
#endif
__inline UInt TComTrQuant::xGetCodedLevel  ( Double&                         rd64UncodedCost,
                                             Double&                         rd64CodedCost,
#if PCP_SIGMAP_SIMPLE_LAST
                                             Double&                         rd64CodedLastCost,
                                             UInt&                           ruiBestNonZeroLevel,
                                             Long                            lLevelDouble,
                                             UInt                            uiMaxAbsLevel,
#else
                                             Long                            lLevelDouble,
                                             UInt                            uiMaxAbsLevel,
                                             bool                            bLastScanPos,
#endif
                                             UShort                          ui16CtxNumSig,
                                             UShort                          ui16CtxNumOne,
                                             UShort                          ui16CtxNumAbs,
#if E253
                                             UShort                          ui16AbsGoRice,
#endif
                                             Int                             iQBits,
                                             Double                          dTemp
                                            ) const
{
  UInt   uiBestAbsLevel = 0;
  Double dErr1          = Double( lLevelDouble );
  
  rd64UncodedCost = dErr1 * dErr1 * dTemp;
#if PCP_SIGMAP_SIMPLE_LAST
  rd64CodedCost   = rd64UncodedCost + xGetRateSigCoef( 0, ui16CtxNumSig );

  ruiBestNonZeroLevel   = 0;
  if( uiMaxAbsLevel )
  {
    UInt uiAbsLevel     = uiMaxAbsLevel;
    ruiBestNonZeroLevel = uiMaxAbsLevel;
    Double dErr         = Double( lLevelDouble - Long( uiAbsLevel << iQBits ) );
#if E253
    rd64CodedLastCost   = dErr * dErr * dTemp + xGetICRateCost( uiAbsLevel, ui16CtxNumOne, ui16CtxNumAbs, ui16AbsGoRice );
#else
    rd64CodedLastCost   = dErr * dErr * dTemp + xGetICRateCost( uiAbsLevel, ui16CtxNumOne, ui16CtxNumAbs );
#endif
  }
  else
  {
    return uiBestAbsLevel;
  }

  UInt uiAbsLevel = ( uiMaxAbsLevel > 1 ? uiMaxAbsLevel - 1 : 1 );
  if( uiAbsLevel != uiMaxAbsLevel )
  {
    Double dErr        = Double( lLevelDouble - Long( uiAbsLevel << iQBits ) );
#if E253
    Double dCurrCost   = dErr * dErr * dTemp + xGetICRateCost( uiAbsLevel, ui16CtxNumOne, ui16CtxNumAbs, ui16AbsGoRice );
#else
    Double dCurrCost   = dErr * dErr * dTemp + xGetICRateCost( uiAbsLevel, ui16CtxNumOne, ui16CtxNumAbs );
#endif
    if( dCurrCost < rd64CodedLastCost )
    {
      ruiBestNonZeroLevel = uiAbsLevel;
      rd64CodedLastCost   = dCurrCost;
    }  
  }

  Double dCurrCost = rd64CodedLastCost + xGetRateSigCoef( 1, ui16CtxNumSig );

  if( dCurrCost < rd64CodedCost )
  {
    uiBestAbsLevel  = ruiBestNonZeroLevel;
    rd64CodedCost   = dCurrCost;
  }
#else
#if E253
  rd64CodedCost   = rd64UncodedCost + xGetICRateCost( 0, bLastScanPos, ui16CtxNumSig, ui16CtxNumOne, ui16CtxNumAbs, ui16AbsGoRice );
#else
  rd64CodedCost   = rd64UncodedCost + xGetICRateCost( 0, bLastScanPos, ui16CtxNumSig, ui16CtxNumOne, ui16CtxNumAbs );
#endif

  UInt uiMinAbsLevel = ( uiMaxAbsLevel > 1 ? uiMaxAbsLevel - 1 : 1 );
  for( UInt uiAbsLevel = uiMaxAbsLevel; uiAbsLevel >= uiMinAbsLevel ; uiAbsLevel-- )
  {
    Double i64Delta  = Double( lLevelDouble  - Long( uiAbsLevel << iQBits ) );
    Double dErr      = Double( i64Delta );
#if E253
    Double dCurrCost = dErr * dErr * dTemp + xGetICRateCost( uiAbsLevel, bLastScanPos, ui16CtxNumSig, ui16CtxNumOne, ui16CtxNumAbs, ui16AbsGoRice );
#else
    Double dCurrCost = dErr * dErr * dTemp + xGetICRateCost( uiAbsLevel, bLastScanPos, ui16CtxNumSig, ui16CtxNumOne, ui16CtxNumAbs );
#endif

    if( dCurrCost < rd64CodedCost )
    {
      uiBestAbsLevel  = uiAbsLevel;
      rd64CodedCost   = dCurrCost;
    }
  }
#endif
  return uiBestAbsLevel;
}

#if E253 && PCP_SIGMAP_SIMPLE_LAST // only valid if both tools are enabled
/** Calculates the cost for specific absolute transform level
 * \param uiAbsLevel scaled quantized level
 * \param bLastScanPos last scan position
 * \param ui16CtxNumSig current ctxInc for coeff_abs_significant_flag
 * \param ui16CtxNumOne current ctxInc for coeff_abs_level_greater1 (1st bin of coeff_abs_level_minus1 in AVC)
 * \param ui16CtxNumAbs current ctxInc for coeff_abs_level_greater2 (remaining bins of coeff_abs_level_minus1 in AVC)
 * \param ui16AbsGoRice Rice parameter for coeff_abs_level_minus3
 * \param ui16CtxBase current global offset for coeff_abs_level_greater1 and coeff_abs_level_greater2
 * \returns cost of given absolute transform level
 */
#endif
__inline Double TComTrQuant::xGetICRateCost  ( UInt                            uiAbsLevel,
#if !PCP_SIGMAP_SIMPLE_LAST
                                               Bool                            bLastScanPos,
                                               UShort                          ui16CtxNumSig,
#endif
                                               UShort                          ui16CtxNumOne,
                                               UShort                          ui16CtxNumAbs
#if E253
                                               ,UShort                          ui16AbsGoRice
#endif
                                              ) const
{
#if PCP_SIGMAP_SIMPLE_LAST
  Double iRate = xGetIEPRate();
#else
  if( uiAbsLevel == 0 )
  {
    Double iRate = 0;
    if( !bLastScanPos )
    {
      iRate += m_pcEstBitsSbac->significantBits[ ui16CtxNumSig ][ 0 ];
    }
    return xGetICost( iRate );
  }
  Double iRate = xGetIEPRate();
  if( !bLastScanPos )
  {
    iRate += m_pcEstBitsSbac->significantBits[ ui16CtxNumSig ][ 1 ];
  }
#endif
  if( uiAbsLevel == 1 )
  {
    iRate += m_pcEstBitsSbac->m_greaterOneBits[ ui16CtxNumOne ][ 0 ];
  }
#if E253
  else if( uiAbsLevel == 2 )
  {
    iRate += m_pcEstBitsSbac->m_greaterOneBits[ ui16CtxNumOne ][ 1 ];
    iRate += m_pcEstBitsSbac->m_levelAbsBits[ ui16CtxNumAbs ][ 0 ];
  }
  else
  {
    UInt uiSymbol     = uiAbsLevel - 3;
    UInt uiMaxVlc     = g_auiGoRiceRange[ ui16AbsGoRice ];
    Bool bExpGolomb   = ( uiSymbol > uiMaxVlc );

    if( bExpGolomb )
    {
      uiAbsLevel  = uiSymbol - uiMaxVlc;
      int iEGS    = 1;  for( UInt uiMax = 2; uiAbsLevel >= uiMax; uiMax <<= 1, iEGS += 2 );
      iRate      += iEGS << 15;
      uiSymbol    = min<UInt>( uiSymbol, ( uiMaxVlc + 1 ) );
    }

    UShort ui16PrefLen = UShort( uiSymbol >> ui16AbsGoRice ) + 1;
    UShort ui16NumBins = min<UInt>( ui16PrefLen, g_auiGoRicePrefixLen[ ui16AbsGoRice ] ) + ui16AbsGoRice;

    iRate += ui16NumBins << 15;
    iRate += m_pcEstBitsSbac->m_greaterOneBits[ ui16CtxNumOne ][ 1 ];
    iRate += m_pcEstBitsSbac->m_levelAbsBits[ ui16CtxNumAbs ][ 1 ];
  }
#else
  else if( uiAbsLevel < 15 )
  {
    iRate += m_pcEstBitsSbac->m_greaterOneBits[ ui16CtxNumOne ][ 1 ];
    iRate += m_pcEstBitsSbac->m_levelAbsBits[ ui16CtxNumAbs ][ 0 ];
    iRate += m_pcEstBitsSbac->m_levelAbsBits[ ui16CtxNumAbs ][ 1 ] * (int)( uiAbsLevel - 2 );
  }
  else
  {
    uiAbsLevel -= 14;
    int iEGS    = 1;  for( UInt uiMax = 2; uiAbsLevel >= uiMax; uiMax <<= 1, iEGS += 2 );
    iRate += m_pcEstBitsSbac->m_greaterOneBits[ ui16CtxNumOne ][ 1 ];
    iRate += m_pcEstBitsSbac->m_levelAbsBits[ ui16CtxNumAbs ][ 1 ] * 13;
    iRate += xGetIEPRate() * iEGS;
  }
#endif
  return xGetICost( iRate );
}

#if PCP_SIGMAP_SIMPLE_LAST
/** Calculates the cost of signaling the last significant coefficient in the block
 * \param uiPosX X coordinate of the last significant coefficient
 * \param uiPosY Y coordinate of the last significant coefficient
 * \returns cost of last significant coefficient
 */
__inline Double TComTrQuant::xGetRateLast   ( UInt                            uiPosX,
                                              UInt                            uiPosY ) const
{
  return xGetICost( m_pcEstBitsSbac->lastXBits[ uiPosX ] + m_pcEstBitsSbac->lastYBits[ uiPosY ] );
}

 /** Calculates the cost for specific absolute transform level
 * \param uiAbsLevel scaled quantized level
 * \param ui16CtxNumOne current ctxInc for coeff_abs_level_greater1 (1st bin of coeff_abs_level_minus1 in AVC)
 * \param ui16CtxNumAbs current ctxInc for coeff_abs_level_greater2 (remaining bins of coeff_abs_level_minus1 in AVC)
 * \param ui16CtxBase current global offset for coeff_abs_level_greater1 and coeff_abs_level_greater2
 * \returns cost of given absolute transform level
 */
__inline Double TComTrQuant::xGetRateSigCoef  ( UShort                          uiSignificance,
                                                UShort                          ui16CtxNumSig ) const
{
  return xGetICost( m_pcEstBitsSbac->significantBits[ ui16CtxNumSig ][ uiSignificance ] );
}
#endif

/** Get the cost for a specific rate
 * \param dRate rate of a bit
 * \returns cost at the specific rate
 */
__inline Double TComTrQuant::xGetICost        ( Double                          dRate         ) const
{
  return m_dLambda * dRate;
}

/** Get the cost of an equal probable bit
 * \returns cost of equal probable bit
 */
__inline Double TComTrQuant::xGetIEPRate      (                                               ) const
{
  return 32768;
}
