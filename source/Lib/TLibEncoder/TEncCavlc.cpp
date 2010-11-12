/* ====================================================================================================================

  The copyright in this software is being made available under the License included below.
  This software may be subject to other third party and   contributor rights, including patent rights, and no such
  rights are granted under this license.

  Copyright (c) 2010, SAMSUNG ELECTRONICS CO., LTD. and BRITISH BROADCASTING CORPORATION
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted only for
  the purpose of developing standards within the Joint Collaborative Team on Video Coding and for testing and
  promoting such standards. The following conditions are required to be met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
      the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
      the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of SAMSUNG ELECTRONICS CO., LTD. nor the name of the BRITISH BROADCASTING CORPORATION
      may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * ====================================================================================================================
*/

/** \file     TEncCavlc.cpp
    \brief    CAVLC encoder class
*/

#include "TEncCavlc.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCavlc::TEncCavlc()
{
  m_pcBitIf           = NULL;
  m_bRunLengthCoding  = false;   //  m_bRunLengthCoding  = !rcSliceHeader.isIntra();
  m_uiCoeffCost       = 0;
  m_bAlfCtrl = false;
  m_uiMaxAlfCtrlDepth = 0;

  m_bAdaptFlag        = true;    // adaptive VLC table

#if LCEC_STAT
  m_uiBitHLS                 = 0;
  m_uiBitMVPId               = 0;
  m_uiBitPartSize            = 0;
  m_uiBitPredMode            = 0;
  m_uiBitMergeFlag           = 0;
  m_uiBitMergeIndex          = 0;
  m_uiBitIntraFiltFlag       = 0;
  m_uiBitAlfCtrlFlag         = 0;
  m_uiBitAlfCtrlDepth        = 0;
  m_uiBitSkipFlag            = 0;
  m_uiBitCurrSplitFlag       = 0;
  m_uiBitTransformSubdivFlag = 0;
  m_uiBitQtCbf               = 0;
  m_uiBitPlanarVlc           = 0;
  m_uiBitIntraDir            = 0;
  m_uiBitIRefFrmIdx          = 0;  
  m_uiBitMVD                 = 0;
  m_uiBitDeltaQP             = 0;
  m_uiBitCbf                 = 0;
  m_uiBitAlfFlag             = 0;
  m_uiBitAlfUvlc             = 0;
  m_uiBitAlfSvlc             = 0;
  m_uiBitMVPIdx              = 0;
  m_uiBitPlanarInfo          = 0;
  m_uiBitInterDir            = 0;
  m_uiBitMI                  = 0;
  m_uiBitSF                  = 0;
  m_uiBitCoeff               = 0;
  m_uiBitCbp                 = 0;
#endif
}

TEncCavlc::~TEncCavlc()
{
}

#if LCEC_STAT
#define NUM_PASSES 2
Void TEncCavlc::statistics(Bool bResetFlag, UInt uiPrintVar)
{
  Int i,j;
  if (bResetFlag)
  {
    m_uiBitHLS                 = 0;
    m_uiBitMVPId               = 0;
    m_uiBitPartSize            = 0;
    m_uiBitPredMode            = 0;
    
    m_uiBitMergeFlag           = 0; 
    m_uiBitMergeIndex          = 0; 
    m_uiBitIntraFiltFlag       = 0;
    m_uiBitAlfCtrlFlag         = 0;
    m_uiBitAlfCtrlDepth        = 0;
    
    m_uiBitSkipFlag            = 0;
    m_uiBitCurrSplitFlag       = 0; 
    m_uiBitTransformSubdivFlag = 0;
    m_uiBitQtCbf               = 0; 
    
    m_uiBitPlanarVlc           = 0;
    m_uiBitIntraDir            = 0;
    m_uiBitIRefFrmIdx          = 0;
    m_uiBitMVD                 = 0;
    m_uiBitDeltaQP             = 0;
    
    m_uiBitCbf                 = 0; 
    m_uiBitCbp                 = 0;
    m_uiBitAlfFlag             = 0;
    m_uiBitAlfUvlc             = 0;
    m_uiBitAlfSvlc             = 0;
    m_uiBitMVPIdx              = 0;
    
    m_uiBitPlanarInfo          = 0;
    m_uiBitInterDir            = 0;
    m_uiBitMI                  = 0;
    m_uiBitSF                  = 0;
    m_uiBitCoeff               = 0;
  }


  if (uiPrintVar)
  {
    Int i;
    FILE *fp;
   
    UInt uiTotalBits = 0;
              
    /* Divide some of the variables by by number of passes */
    m_uiBitMVPId = m_uiBitMVPId/NUM_PASSES;
    m_uiBitPartSize = m_uiBitPartSize/NUM_PASSES;
    m_uiBitPredMode = m_uiBitPredMode/NUM_PASSES;    
    m_uiBitMergeFlag = m_uiBitMergeFlag/NUM_PASSES;
    m_uiBitMergeIndex = m_uiBitMergeIndex/NUM_PASSES;
    m_uiBitIntraFiltFlag = m_uiBitIntraFiltFlag/NUM_PASSES;    
    m_uiBitSkipFlag = m_uiBitSkipFlag/NUM_PASSES;
    m_uiBitCurrSplitFlag = m_uiBitCurrSplitFlag/NUM_PASSES;
    m_uiBitTransformSubdivFlag = m_uiBitTransformSubdivFlag/NUM_PASSES;
    m_uiBitQtCbf = m_uiBitQtCbf/NUM_PASSES;   
    m_uiBitPlanarVlc = m_uiBitPlanarVlc/NUM_PASSES;
    m_uiBitIntraDir = m_uiBitIntraDir/NUM_PASSES;
    m_uiBitIRefFrmIdx = m_uiBitIRefFrmIdx/NUM_PASSES;
    m_uiBitMVD = m_uiBitMVD/NUM_PASSES;
    m_uiBitDeltaQP = m_uiBitDeltaQP/NUM_PASSES;
    m_uiBitCbf = m_uiBitCbf/NUM_PASSES;
    m_uiBitCbp = m_uiBitCbp/NUM_PASSES;
    m_uiBitMVPIdx = m_uiBitMVPIdx/NUM_PASSES;
    m_uiBitPlanarInfo = m_uiBitPlanarInfo/NUM_PASSES;
    m_uiBitInterDir = m_uiBitInterDir/NUM_PASSES;
    m_uiBitMI = m_uiBitMI/NUM_PASSES;
    m_uiBitCoeff = m_uiBitCoeff/NUM_PASSES;


    /* Calculate total bit usage */
    uiTotalBits += m_uiBitHLS;           
    uiTotalBits += m_uiBitMVPId;
    uiTotalBits += m_uiBitPartSize;
    uiTotalBits += m_uiBitPredMode;
    
    uiTotalBits += m_uiBitMergeFlag;
    uiTotalBits += m_uiBitMergeIndex;
    uiTotalBits += m_uiBitIntraFiltFlag;
    uiTotalBits += m_uiBitAlfCtrlFlag;
    uiTotalBits += m_uiBitAlfCtrlDepth;
    
    uiTotalBits += m_uiBitSkipFlag;
    uiTotalBits += m_uiBitCurrSplitFlag;
    uiTotalBits += m_uiBitTransformSubdivFlag;
    uiTotalBits += m_uiBitQtCbf;   
   
    uiTotalBits += m_uiBitPlanarVlc;
    uiTotalBits += m_uiBitIntraDir;
    uiTotalBits += m_uiBitIRefFrmIdx;
    uiTotalBits += m_uiBitMVD;
    uiTotalBits += m_uiBitDeltaQP;

    uiTotalBits += m_uiBitCbf;
    uiTotalBits += m_uiBitCbp;
    uiTotalBits += m_uiBitAlfFlag;
    uiTotalBits += m_uiBitAlfUvlc;  
    uiTotalBits += m_uiBitAlfSvlc;
    uiTotalBits += m_uiBitMVPIdx;

    uiTotalBits += m_uiBitPlanarInfo;
    uiTotalBits += m_uiBitInterDir;
    uiTotalBits += m_uiBitMI;
    uiTotalBits += m_uiBitSF;
    uiTotalBits += m_uiBitCoeff;

    /* Printout statistics */
    printf("\n");
    printf("m_uiBitHLS =                 %12d %6.1f\n",m_uiBitHLS,100.0*(float)m_uiBitHLS/(float)uiTotalBits);
    printf("m_uiBitMVPId =               %12d %6.1f\n",m_uiBitMVPId,100.0*(float)m_uiBitMVPId/(float)uiTotalBits);
    printf("m_uiBitPartSize =            %12d %6.1f\n",m_uiBitPartSize,100.0*(float)m_uiBitPartSize/(float)uiTotalBits);
    printf("m_uiBitPredMode =            %12d %6.1f\n",m_uiBitPredMode,100.0*(float)m_uiBitPredMode/(float)uiTotalBits);
    
    printf("m_uiBitMergeFlag =           %12d %6.1f\n",m_uiBitMergeFlag,100.0*(float)m_uiBitMergeFlag/(float)uiTotalBits);
    printf("m_uiBitMergeIndex =          %12d %6.1f\n",m_uiBitMergeIndex,100.0*(float)m_uiBitMergeIndex/(float)uiTotalBits);
    printf("m_uiBitIntraFiltFlag =       %12d %6.1f\n",m_uiBitIntraFiltFlag,100.0*(float)m_uiBitIntraFiltFlag/(float)uiTotalBits);
    printf("m_uiBitAlfCtrlFlag =         %12d %6.1f\n",m_uiBitAlfCtrlFlag,100.0*(float)m_uiBitAlfCtrlFlag/(float)uiTotalBits);
    printf("m_uiBitAlfCtrlDepth =        %12d %6.1f\n",m_uiBitAlfCtrlDepth,100.0*(float)m_uiBitAlfCtrlDepth/(float)uiTotalBits);
    
    printf("m_uiBitSkipFlag =            %12d %6.1f\n",m_uiBitSkipFlag,100.0*(float)m_uiBitSkipFlag/(float)uiTotalBits);
    printf("m_uiBitCurrSplitFlag  =      %12d %6.1f\n",m_uiBitCurrSplitFlag,100.0*(float)m_uiBitCurrSplitFlag/(float)uiTotalBits);
    printf("m_uiBitTransformSubdivFlag = %12d %6.1f\n",m_uiBitTransformSubdivFlag,100.0*(float)m_uiBitTransformSubdivFlag/(float)uiTotalBits);
    printf("m_uiBitQtCbf =               %12d %6.1f\n",m_uiBitQtCbf,100.0*(float)m_uiBitQtCbf/(float)uiTotalBits);
    
    printf("m_uiBitPlanarVlc =           %12d %6.1f\n",m_uiBitPlanarVlc,100.0*(float)m_uiBitPlanarVlc/(float)uiTotalBits);
    printf("m_uiBitIntraDir =            %12d %6.1f\n",m_uiBitIntraDir,100.0*(float)m_uiBitIntraDir/(float)uiTotalBits);
    printf("m_uiBitIRefFrmIdx =          %12d %6.1f\n",m_uiBitIRefFrmIdx,100.0*(float)m_uiBitIRefFrmIdx/(float)uiTotalBits);
    printf("m_uiBitMVD =                 %12d %6.1f\n",m_uiBitMVD,100.0*(float)m_uiBitMVD/(float)uiTotalBits);
    printf("m_uiBitDeltaQP =             %12d %6.1f\n",m_uiBitDeltaQP,100.0*(float)m_uiBitDeltaQP/(float)uiTotalBits);
    
    printf("m_uiBitCbf =                 %12d %6.1f\n",m_uiBitCbf,100.0*(float)m_uiBitCbf/(float)uiTotalBits);
    printf("m_uiBitCbp =                 %12d %6.1f\n",m_uiBitCbp,100.0*(float)m_uiBitCbp/(float)uiTotalBits);
    printf("m_uiBitAlfFlag =             %12d %6.1f\n",m_uiBitAlfFlag,100.0*(float)m_uiBitAlfFlag/(float)uiTotalBits);
    printf("m_uiBitAlfUvlc =             %12d %6.1f\n",m_uiBitAlfUvlc,100.0*(float)m_uiBitAlfUvlc/(float)uiTotalBits);
    printf("m_uiBitAlfSvlc =             %12d %6.1f\n",m_uiBitAlfSvlc,100.0*(float)m_uiBitAlfSvlc/(float)uiTotalBits);
    printf("m_uiBitMVPIdx =              %12d %6.1f\n",m_uiBitMVPIdx,100.0*(float)m_uiBitMVPIdx/(float)uiTotalBits);
    
    printf("m_uiBitPlanarInfo =          %12d %6.1f\n",m_uiBitPlanarInfo,100.0*(float)m_uiBitPlanarInfo/(float)uiTotalBits);
    printf("m_uiBitInterDir =            %12d %6.1f\n",m_uiBitInterDir,100.0*(float)m_uiBitInterDir/(float)uiTotalBits);
    printf("m_uiBitMI =                  %12d %6.1f\n",m_uiBitMI,100.0*(float)m_uiBitMI/(float)uiTotalBits);
    printf("m_uiBitSF =                  %12d %6.1f\n",m_uiBitSF,100.0*(float)m_uiBitSF/(float)uiTotalBits);
    printf("m_uiBitCoeff =               %12d %6.1f\n",m_uiBitCoeff,100.0*(float)m_uiBitCoeff/(float)uiTotalBits);

    printf("uiTotalBits =                %12d\n",uiTotalBits);

  } //if uiPrintFlag
}
#endif

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncCavlc::resetEntropy()
{
  m_bRunLengthCoding = ! m_pcSlice->isIntra();
  m_uiRun = 0;
  ::memcpy(m_uiLPTableE8, g_auiLPTableE8, 10*128*sizeof(UInt));
  ::memcpy(m_uiLPTableD8, g_auiLPTableD8, 10*128*sizeof(UInt));
  ::memcpy(m_uiLPTableE4, g_auiLPTableE4, 3*32*sizeof(UInt));
  ::memcpy(m_uiLPTableD4, g_auiLPTableD4, 3*32*sizeof(UInt));
  ::memcpy(m_uiLastPosVlcIndex, g_auiLastPosVlcIndex, 10*sizeof(UInt));

  ::memcpy(m_uiCBPTableE, g_auiCBPTableE, 2*8*sizeof(UInt));
  ::memcpy(m_uiCBPTableD, g_auiCBPTableD, 2*8*sizeof(UInt));
  m_uiCbpVlcIdx[0] = 0;
  m_uiCbpVlcIdx[1] = 0;

#if QC_BLK_CBP
  ::memcpy(m_uiBlkCBPTableE, g_auiBlkCBPTableE, 2*15*sizeof(UInt));
  ::memcpy(m_uiBlkCBPTableD, g_auiBlkCBPTableD, 2*15*sizeof(UInt));
  m_uiBlkCbpVlcIdx = 0;
#endif

  ::memcpy(m_uiMI1TableE, g_auiMI1TableE, 8*sizeof(UInt));
  ::memcpy(m_uiMI1TableD, g_auiMI1TableD, 8*sizeof(UInt));
  ::memcpy(m_uiMI2TableE, g_auiMI2TableE, 15*sizeof(UInt));
  ::memcpy(m_uiMI2TableD, g_auiMI2TableD, 15*sizeof(UInt));

#if MS_NO_BACK_PRED_IN_B0
  if ( m_pcSlice->getNoBackPredFlag() )
  {
    ::memcpy(m_uiMI1TableE, g_auiMI1TableENoL1, 8*sizeof(UInt));
    ::memcpy(m_uiMI1TableD, g_auiMI1TableDNoL1, 8*sizeof(UInt));
    ::memcpy(m_uiMI2TableE, g_auiMI2TableENoL1, 15*sizeof(UInt));
    ::memcpy(m_uiMI2TableD, g_auiMI2TableDNoL1, 15*sizeof(UInt));
  }
#endif

  m_uiMITableVlcIdx = 0;  
}

UInt* TEncCavlc::GetLP8Table()
{   
  return &m_uiLPTableE8[0][0];
}

UInt* TEncCavlc::GetLP4Table()
{   
  return &m_uiLPTableE4[0][0];
}

#if LCEC_STAT
Void TEncCavlc::codePPS( TComPPS* pcPPS )
{
  
#if HHI_NAL_UNIT_SYNTAX
  // uiFirstByte
  xWriteCode( NAL_REF_IDC_PRIORITY_HIGHEST, 2);
  xWriteCode( 0, 1);
  xWriteCode( NAL_UNIT_PPS, 5);
  m_uiBitHLS += 8;
#endif
  return;
}



Void TEncCavlc::codeSPS( TComSPS* pcSPS )
{
  
#if HHI_NAL_UNIT_SYNTAX
  // uiFirstByte
  xWriteCode( NAL_REF_IDC_PRIORITY_HIGHEST, 2);
  xWriteCode( 0, 1);
  xWriteCode( NAL_UNIT_SPS, 5);
  m_uiBitHLS += 8;
#endif
  // Structure
  m_uiBitHLS += xWriteUvlc  ( pcSPS->getWidth () );
  m_uiBitHLS += xWriteUvlc  ( pcSPS->getHeight() );

  m_uiBitHLS += xWriteUvlc  ( pcSPS->getPad (0) );
  m_uiBitHLS += xWriteUvlc  ( pcSPS->getPad (1) );

  assert( pcSPS->getMaxCUWidth() == pcSPS->getMaxCUHeight() );
  m_uiBitHLS += xWriteUvlc  ( pcSPS->getMaxCUWidth()   );
  m_uiBitHLS += xWriteUvlc  ( pcSPS->getMaxCUDepth()-g_uiAddCUDepth );
  
  m_uiBitHLS += xWriteUvlc( pcSPS->getQuadtreeTULog2MinSize() - 2 );
  m_uiBitHLS += xWriteUvlc( pcSPS->getQuadtreeTULog2MaxSize() - pcSPS->getQuadtreeTULog2MinSize() );
  m_uiBitHLS += xWriteUvlc( pcSPS->getQuadtreeTUMaxDepthInter() - 1 );
  m_uiBitHLS += xWriteUvlc( pcSPS->getQuadtreeTUMaxDepthIntra() - 1 );
  
  // Tools
  xWriteFlag  ( (pcSPS->getUseALF ()) ? 1 : 0 );
  xWriteFlag  ( (pcSPS->getUseDQP ()) ? 1 : 0 );
  xWriteFlag  ( 0 ); // TODO: was WPG
  xWriteFlag  ( (pcSPS->getUseLDC ()) ? 1 : 0 );
  xWriteFlag  ( (pcSPS->getUseQBO ()) ? 1 : 0 );
  m_uiBitHLS += 5;
#if HHI_MRG
  xWriteFlag  ( (pcSPS->getUseMRG ()) ? 1 : 0 ); // SOPH:
  m_uiBitHLS += 1;
#endif
  xWriteFlag  ( (pcSPS->getUseAMP ()) ? 1 : 0 );
  m_uiBitHLS += 1;
#if HHI_RMP_SWITCH
  xWriteFlag  ( (pcSPS->getUseRMP ()) ? 1 : 0 );
  m_uiBitHLS += 1;
#endif
  
  // write number of taps for DIF
  m_uiBitHLS += xWriteUvlc  ( (pcSPS->getDIFTap ()>>1)-2 ); // 4, 6, 8, 10, 12
  m_uiBitHLS += xWriteUvlc  ( 0 ); // TODO: remove? was chroma IF length

  // AMVP mode for each depth
  for (Int i = 0; i < pcSPS->getMaxCUDepth(); i++)
  {
    xWriteFlag( pcSPS->getAMVPMode(i) ? 1 : 0);
    m_uiBitHLS += 1;
  }

  // Bit-depth information
  m_uiBitHLS += xWriteUvlc( pcSPS->getBitDepth() - 8 );
  m_uiBitHLS += xWriteUvlc( pcSPS->getBitIncrement() );

  m_uiBitHLS += xWriteCode( 0, 8); // TODO: remove? was balanced CPUs

}

Void TEncCavlc::codeSliceHeader         ( TComSlice* pcSlice )
{
  
#if HHI_NAL_UNIT_SYNTAX
  // here someone can add an appropriated NalRefIdc type 
  xWriteCode( NAL_REF_IDC_PRIORITY_HIGHEST, 2);
  xWriteCode( 0, 1);
  xWriteCode( NAL_UNIT_CODED_SLICE, 5);
  m_uiBitHLS += 8;
#endif
  m_uiBitHLS += xWriteCode  (pcSlice->getPOC(), 10 );   //  9 == SPS->Log2MaxFrameNum
  m_uiBitHLS += xWriteUvlc  (pcSlice->getSliceType() );
  m_uiBitHLS += xWriteSvlc  (pcSlice->getSliceQp() );
  
  xWriteFlag  (pcSlice->getSymbolMode() > 0 ? 1 : 0);
  m_uiBitHLS += 1;
  if( pcSlice->getSymbolMode() > 0 )
  {
    xWriteFlag( pcSlice->getSymbolMode() > 1 ? 1 : 0 );
    m_uiBitHLS += 1;
    if( pcSlice->getSymbolMode() )
    {
      xWriteFlag( pcSlice->getMultiCodeword() ? 1 : 0 );
      m_uiBitHLS += 1;
    }
    if( pcSlice->getSymbolMode() == 2 && ! pcSlice->getMultiCodeword() )
    {
      m_uiBitHLS += xWriteUvlc( pcSlice->getMaxPIPEDelay() >> 6 );
    }
  }
  else
  {
    xWriteFlag( 0 ); // TODO: remove? v2v related
    m_uiBitHLS += 1;
  }

  if (!pcSlice->isIntra())
  {
    xWriteFlag  (pcSlice->isReferenced() ? 1 : 0);
    m_uiBitHLS += 1;
#ifdef ROUNDING_CONTROL_BIPRED
	xWriteFlag  (pcSlice->isRounding() ? 1 : 0);
  m_uiBitHLS += 1;
#endif
  }

  xWriteFlag  (pcSlice->getLoopFilterDisable());
  m_uiBitHLS += 1;

  if (!pcSlice->isIntra())
  {
    m_uiBitHLS += xWriteCode  ((pcSlice->getNumRefIdx( REF_PIC_LIST_0 )) -pcSlice->getAddRefCnt(REF_PIC_LIST_0), 3 );
  }
  else
  {
    pcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
  }
  if (pcSlice->isInterB())
  {
    m_uiBitHLS += xWriteCode  ((pcSlice->getNumRefIdx( REF_PIC_LIST_1 )) -pcSlice->getAddRefCnt(REF_PIC_LIST_1), 3 );
  }
  else
  {
    pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
  }

  xWriteFlag  (pcSlice->getDRBFlag() ? 1 : 0 );
  m_uiBitHLS += 1;
  if ( !pcSlice->getDRBFlag() )
  {
    m_uiBitHLS += xWriteCode  (pcSlice->getERBIndex(), 2);
  }

  m_uiBitHLS += xWriteUvlc  ( pcSlice->getInterpFilterType() );

#if AMVP_NEIGH_COL
  if ( pcSlice->getSliceType() == B_SLICE )
  {
    xWriteFlag( pcSlice->getColDir() );
    m_uiBitHLS += 1;
  }
#endif
}

Void TEncCavlc::codeTerminatingBit      ( UInt uilsLast )
{
  xWriteFlag( uilsLast );
  m_uiBitHLS += 1;
}

Void TEncCavlc::codeSliceFinish ()
{
  if ( m_bRunLengthCoding && m_uiRun)
  {
    m_uiBitHLS += xWriteUvlc(m_uiRun);
  }
}
#else

Void TEncCavlc::codePPS( TComPPS* pcPPS )
{
#if HHI_NAL_UNIT_SYNTAX
  // uiFirstByte
  xWriteCode( NAL_REF_IDC_PRIORITY_HIGHEST, 2);
  xWriteCode( 0, 1);
  xWriteCode( NAL_UNIT_PPS, 5);
#endif
  return;
}



Void TEncCavlc::codeSPS( TComSPS* pcSPS )
{
#if HHI_NAL_UNIT_SYNTAX
  // uiFirstByte
  xWriteCode( NAL_REF_IDC_PRIORITY_HIGHEST, 2);
  xWriteCode( 0, 1);
  xWriteCode( NAL_UNIT_SPS, 5);
#endif
  // Structure
  xWriteUvlc  ( pcSPS->getWidth () );
  xWriteUvlc  ( pcSPS->getHeight() );

  xWriteUvlc  ( pcSPS->getPad (0) );
  xWriteUvlc  ( pcSPS->getPad (1) );

  assert( pcSPS->getMaxCUWidth() == pcSPS->getMaxCUHeight() );
  xWriteUvlc  ( pcSPS->getMaxCUWidth()   );
  xWriteUvlc  ( pcSPS->getMaxCUDepth()-g_uiAddCUDepth );
  
  xWriteUvlc( pcSPS->getQuadtreeTULog2MinSize() - 2 );
  xWriteUvlc( pcSPS->getQuadtreeTULog2MaxSize() - pcSPS->getQuadtreeTULog2MinSize() );
  xWriteUvlc( pcSPS->getQuadtreeTUMaxDepthInter() - 1 );
  xWriteUvlc( pcSPS->getQuadtreeTUMaxDepthIntra() - 1 );
  
  
  // Tools
  xWriteFlag  ( (pcSPS->getUseALF ()) ? 1 : 0 );
  xWriteFlag  ( (pcSPS->getUseDQP ()) ? 1 : 0 );
  xWriteFlag  ( 0 ); // TODO: was WPG
  xWriteFlag  ( (pcSPS->getUseLDC ()) ? 1 : 0 );
  xWriteFlag  ( (pcSPS->getUseQBO ()) ? 1 : 0 );
#if HHI_MRG
  xWriteFlag  ( (pcSPS->getUseMRG ()) ? 1 : 0 ); // SOPH:
#endif

  xWriteFlag  ( 0 ); // TODO: remove? was AMP
#if HHI_RMP_SWITCH
  xWriteFlag  ( (pcSPS->getUseRMP()) ? 1 : 0 );
#endif

  // write number of taps for DIF
  xWriteUvlc  ( (pcSPS->getDIFTap ()>>1)-2 ); // 4, 6, 8, 10, 12
  xWriteUvlc  ( 0 ); // TODO: remove? was chroma IF length

  // AMVP mode for each depth
  for (Int i = 0; i < pcSPS->getMaxCUDepth(); i++)
  {
    xWriteFlag( pcSPS->getAMVPMode(i) ? 1 : 0);
  }

  // Bit-depth information
  xWriteUvlc( pcSPS->getBitDepth() - 8 );
  xWriteUvlc( pcSPS->getBitIncrement() );

  xWriteCode( 0, 8); // TODO: remove? was balanced CPUs

}

Void TEncCavlc::codeSliceHeader         ( TComSlice* pcSlice )
{
#if HHI_NAL_UNIT_SYNTAX
  // here someone can add an appropriated NalRefIdc type 
  xWriteCode( NAL_REF_IDC_PRIORITY_HIGHEST, 2);
  xWriteCode( 0, 1);
  xWriteCode( NAL_UNIT_CODED_SLICE, 5);
#endif
  xWriteCode  (pcSlice->getPOC(), 10 );   //  9 == SPS->Log2MaxFrameNum
  xWriteUvlc  (pcSlice->getSliceType() );
  xWriteSvlc  (pcSlice->getSliceQp() );
  
  xWriteFlag  (pcSlice->getSymbolMode() > 0 ? 1 : 0);

  if (!pcSlice->isIntra())
  {
    xWriteFlag  (pcSlice->isReferenced() ? 1 : 0);
#ifdef ROUNDING_CONTROL_BIPRED
	xWriteFlag  (pcSlice->isRounding() ? 1 : 0);
#endif
  }

  xWriteFlag  (pcSlice->getLoopFilterDisable());

  if (!pcSlice->isIntra())
  {
    xWriteCode  ((pcSlice->getNumRefIdx( REF_PIC_LIST_0 )), 3 );
  }
  else
  {
    pcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
  }
  if (pcSlice->isInterB())
  {
    xWriteCode  ((pcSlice->getNumRefIdx( REF_PIC_LIST_1 )), 3 );
  }
  else
  {
    pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
  }

  xWriteFlag  (pcSlice->getDRBFlag() ? 1 : 0 );
  if ( !pcSlice->getDRBFlag() )
  {
    xWriteCode  (pcSlice->getERBIndex(), 2);
  }

  xWriteUvlc  ( pcSlice->getInterpFilterType() );

#if AMVP_NEIGH_COL
  if ( pcSlice->getSliceType() == B_SLICE )
  {
    xWriteFlag( pcSlice->getColDir() );
  }
#endif
}

Void TEncCavlc::codeTerminatingBit      ( UInt uilsLast )
{
#if !BUGFIX102
  xWriteFlag( uilsLast );
#endif
}

Void TEncCavlc::codeSliceFinish ()
{
  if ( m_bRunLengthCoding && m_uiRun)
  {
    xWriteUvlc(m_uiRun);
  }
}
#endif //LCEC_STAT

Void TEncCavlc::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iSymbol = pcCU->getMVPIdx(eRefList, uiAbsPartIdx);
  Int iNum    = pcCU->getMVPNum(eRefList, uiAbsPartIdx);

#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitMVPIdx += xWriteUnaryMaxSymbol(iSymbol, iNum-1);
  else
#endif
  xWriteUnaryMaxSymbol(iSymbol, iNum-1);
}

Void TEncCavlc::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  PartSize eSize         = pcCU->getPartitionSize( uiAbsPartIdx );

  if ( pcCU->getSlice()->isInterB() && pcCU->isIntra( uiAbsPartIdx ) )
  {
    xWriteFlag( 0 );
#if HHI_RMP_SWITCH
    if( pcCU->getSlice()->getSPS()->getUseRMP() )
#endif
    {
      xWriteFlag( 0 );
      xWriteFlag( 0 );
    }
#if HHI_DISABLE_INTER_NxN_SPLIT
    if( pcCU->getWidth( uiAbsPartIdx ) == 8 )
    {
      xWriteFlag( 0 );
    }
#else
    xWriteFlag( 0 );
#endif
    xWriteFlag( (eSize == SIZE_2Nx2N? 0 : 1) );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitPartSize += 5;
#endif
    return;
  }

  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
    xWriteFlag( eSize == SIZE_2Nx2N? 1 : 0 );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitPartSize += 1;
#endif
    return;
  }

  switch(eSize)
  {
  case SIZE_2Nx2N:
    {
      xWriteFlag( 1 );
#if LCEC_STAT
      if (m_bAdaptFlag)
        m_uiBitPartSize += 1;
#endif
      break;
    }
  case SIZE_2NxN:
    {
      xWriteFlag( 0 );
      xWriteFlag( 1 );
#if LCEC_STAT
      if (m_bAdaptFlag)
        m_uiBitPartSize += 2;
#endif
      break;
    }
  case SIZE_Nx2N:
    {
      xWriteFlag( 0 );
      xWriteFlag( 0 );
      xWriteFlag( 1 );
#if LCEC_STAT
      if (m_bAdaptFlag)
        m_uiBitPartSize += 3;
#endif
      break;
    }
  case SIZE_NxN:
    {
#if HHI_DISABLE_INTER_NxN_SPLIT
      if( pcCU->getWidth( uiAbsPartIdx ) == 8 )
#endif
      {
        xWriteFlag( 0 );
#if HHI_RMP_SWITCH
        if( pcCU->getSlice()->getSPS()->getUseRMP())
#endif
        {
          xWriteFlag( 0 );
          xWriteFlag( 0 );
        }
#if LCEC_STAT
      if (m_bAdaptFlag)
        m_uiBitPartSize += 3;
#endif
        if (pcCU->getSlice()->isInterB())
        {
          xWriteFlag( 1 );
        }
#if LCEC_STAT
        if (m_bAdaptFlag)
          m_uiBitPartSize += 1;
#endif
      }
      break;
    }
  default:
    {
      assert(0);
    }
  }
}

Void TEncCavlc::codePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  Int iPredMode = pcCU->getPredictionMode( uiAbsPartIdx );

#if HHI_MRG && !SAMSUNG_MRG_SKIP_DIRECT
  if ( !pcCU->getSlice()->getSPS()->getUseMRG() )
  {
    xWriteFlag( iPredMode == MODE_SKIP ? 0 : 1 );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitPredMode += 1;
#endif
  }
#else
  xWriteFlag( iPredMode == MODE_SKIP ? 0 : 1 );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitPredMode += 1;
#endif
#endif

  if ( pcCU->getSlice()->isInterB() )
  {
    return;
  }

  if ( iPredMode != MODE_SKIP )
  {
    xWriteFlag( iPredMode == MODE_INTER ? 0 : 1 );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitPredMode += 1;
#endif
  }
}

#if HHI_MRG
Void TEncCavlc::codeMergeFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiSymbol = pcCU->getMergeFlag( uiAbsPartIdx ) ? 1 : 0;
  xWriteFlag( uiSymbol );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitMergeFlag += 1;
#endif
}

Void TEncCavlc::codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiSymbol = pcCU->getMergeIndex( uiAbsPartIdx ) ? 1 : 0;
  xWriteFlag( uiSymbol );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitMergeIndex += 1;
#endif
}
#endif

Void TEncCavlc::codeAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{  
  if (!m_bAlfCtrl)
    return;

  if( pcCU->getDepth(uiAbsPartIdx) > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, m_uiMaxAlfCtrlDepth))
  {
    return;
  }

  // get context function is here
  UInt uiSymbol = pcCU->getAlfCtrlFlag( uiAbsPartIdx ) ? 1 : 0;

  xWriteFlag( uiSymbol );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitAlfCtrlFlag += 1;
#endif
}

Void TEncCavlc::codeAlfCtrlDepth()
{  
  if (!m_bAlfCtrl)
    return;

  UInt uiDepth = m_uiMaxAlfCtrlDepth;

#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitAlfCtrlDepth += xWriteUnaryMaxSymbol(uiDepth, g_uiMaxCUDepth-1);
  else
#endif
  xWriteUnaryMaxSymbol(uiDepth, g_uiMaxCUDepth-1);
}

Void TEncCavlc::codeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  UInt uiSymbol = pcCU->isSkipped( uiAbsPartIdx ) ? 1 : 0;
  xWriteFlag( uiSymbol );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitSkipFlag += 1;
#endif
}

Void TEncCavlc::codeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    return;

  UInt uiCurrSplitFlag = ( pcCU->getDepth( uiAbsPartIdx ) > uiDepth ) ? 1 : 0;

  xWriteFlag( uiCurrSplitFlag );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitCurrSplitFlag += 1;
#endif
  return;
}

Void TEncCavlc::codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  xWriteFlag( uiSymbol );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitTransformSubdivFlag += 1;
#endif
}

Void TEncCavlc::codeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  UInt uiCbf = pcCU->getCbf( uiAbsPartIdx, eType, uiTrDepth );
  xWriteFlag( uiCbf );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitQtCbf += 1;
#endif
}

Void TEncCavlc::codeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiCbf = pcCU->getQtRootCbf( uiAbsPartIdx );
  xWriteFlag( uiCbf ? 1 : 0 );
}

Void TEncCavlc::codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiDir         = pcCU->getLumaIntraDir( uiAbsPartIdx );
  Int  iMostProbable = pcCU->getMostProbableIntraDirLuma( uiAbsPartIdx );



  if (uiDir == iMostProbable)
  {
    xWriteFlag( 1 );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitIntraDir += 1;
#endif
  }
  else{
    xWriteFlag( 0 );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitIntraDir += 1;
#endif
    uiDir = uiDir > iMostProbable ? uiDir - 1 : uiDir;
    Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
    if ( g_aucIntraModeBitsAng[iIntraIdx] < 6 )
    {
      xWriteFlag( uiDir & 0x01 ? 1 : 0 );
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) xWriteFlag( uiDir & 0x02 ? 1 : 0 );
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) xWriteFlag( uiDir & 0x04 ? 1 : 0 );
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 4 ) xWriteFlag( uiDir & 0x08 ? 1 : 0 );

#if LCEC_STAT
      if (m_bAdaptFlag)
      {
        m_uiBitIntraDir += 1;
        if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) m_uiBitIntraDir += 1;
        if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) m_uiBitIntraDir += 1;
        if ( g_aucIntraModeBitsAng[iIntraIdx] > 4 ) m_uiBitIntraDir += 1;
      }
#endif

    }
    else
    if (uiDir < 31){ // uiDir is here 0...32, 5 bits for uiDir 0...30, 31 is an escape code for coding one more bit for 31 and 32
      xWriteFlag( uiDir & 0x01 ? 1 : 0 );
      xWriteFlag( uiDir & 0x02 ? 1 : 0 );
      xWriteFlag( uiDir & 0x04 ? 1 : 0 );
      xWriteFlag( uiDir & 0x08 ? 1 : 0 );
      xWriteFlag( uiDir & 0x10 ? 1 : 0 );
#if LCEC_STAT
      if (m_bAdaptFlag)
        m_uiBitIntraDir += 5;
#endif
    }
    else{
      xWriteFlag( 1 );
      xWriteFlag( 1 );
      xWriteFlag( 1 );
      xWriteFlag( 1 );
      xWriteFlag( 1 );
      xWriteFlag( uiDir == 32 ? 1 : 0 );
#if LCEC_STAT
      if (m_bAdaptFlag)
        m_uiBitIntraDir += 6;
#endif
    }
  }
}

Void TEncCavlc::codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIntraDirChroma = pcCU->getChromaIntraDir   ( uiAbsPartIdx );

  if ( 0 == uiIntraDirChroma )
  {
    xWriteFlag( 0 );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitIntraDir += 1;
#endif
  }
  else
  {
    xWriteFlag( 1 );
#if LCEC_STAT
    if (m_bAdaptFlag){
      m_uiBitIntraDir += 1;
      m_uiBitIntraDir += xWriteUnaryMaxSymbol( uiIntraDirChroma - 1, 3 );
    }
    else
#endif
    xWriteUnaryMaxSymbol( uiIntraDirChroma - 1, 3 );
  }

  return;
}

Void TEncCavlc::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiInterDir = pcCU->getInterDir   ( uiAbsPartIdx );
  uiInterDir--;

  if(pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2)
  {
    Int x,cx,y,cy;
    Int iRefFrame0,iRefFrame1;
    UInt uiIndex;

    UInt *m_uiMITableE;
    UInt *m_uiMITableD;
    {      
      m_uiMITableE = m_uiMI1TableE;
      m_uiMITableD = m_uiMI1TableD;
      if (uiInterDir==0)
      { 
        iRefFrame0 = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx );
        uiIndex = iRefFrame0;
      }
      else if (uiInterDir==1)
      {
        iRefFrame1 = pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx );
        uiIndex = 2 + iRefFrame1;
      }
      else
      {
        iRefFrame0 = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx );
        iRefFrame1 = pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx );
        uiIndex = 4 + 2*iRefFrame0 + iRefFrame1;
      }
    }
    
    x = uiIndex;
    
    cx = m_uiMITableE[x];
   
    /* Adapt table */
    UInt vlcn = g_auiMITableVlcNum[m_uiMITableVlcIdx];    
    if ( m_bAdaptFlag )
    {        
      cy = Max(0,cx-1);
      y = m_uiMITableD[cy];
      m_uiMITableD[cy] = x;
      m_uiMITableD[cx] = y;
      m_uiMITableE[x] = cy;
      m_uiMITableE[y] = cx;   
	    m_uiMITableVlcIdx += cx == m_uiMITableVlcIdx ? 0 : (cx < m_uiMITableVlcIdx ? -1 : 1);
    }

#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitMI += xWriteVlc( vlcn, cx );
    else
#endif
	  xWriteVlc( vlcn, cx );

    return;
  }

  xWriteFlag( ( uiInterDir == 2 ? 1 : 0 ));
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitInterDir += 1;
#endif
#if MS_NO_BACK_PRED_IN_B0
  if ( pcCU->getSlice()->getNoBackPredFlag() )
  {
    assert( uiInterDir != 1 );
    return;
  }
#endif
  if ( uiInterDir < 2 )
  {
    xWriteFlag( uiInterDir );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitInterDir += 1;
#endif
  }

  return;
}

Void TEncCavlc::codeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );

    if (pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2)
    {
      return;
    }

  xWriteFlag( ( iRefFrame == 0 ? 0 : 1 ) );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitIRefFrmIdx += 1;
#endif
  if ( iRefFrame > 0 )
  {
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitIRefFrmIdx += xWriteUnaryMaxSymbol( iRefFrame - 1, pcCU->getSlice()->getNumRefIdx( eRefList )-2 );
    else
#endif
    xWriteUnaryMaxSymbol( iRefFrame - 1, pcCU->getSlice()->getNumRefIdx( eRefList )-2 );
  }
  return;
}

Void TEncCavlc::codeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  TComCUMvField* pcCUMvField = pcCU->getCUMvField( eRefList );
  Int iHor = pcCUMvField->getMvd( uiAbsPartIdx ).getHor();
  Int iVer = pcCUMvField->getMvd( uiAbsPartIdx ).getVer();

  UInt uiAbsPartIdxL, uiAbsPartIdxA;
  Int iHorPred, iVerPred;

  TComDataCU* pcCUL   = pcCU->getPULeft ( uiAbsPartIdxL, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  TComDataCU* pcCUA   = pcCU->getPUAbove( uiAbsPartIdxA, pcCU->getZorderIdxInCU() + uiAbsPartIdx );

  TComCUMvField* pcCUMvFieldL = ( pcCUL == NULL || pcCUL->isIntra( uiAbsPartIdxL ) ) ? NULL : pcCUL->getCUMvField( eRefList );
  TComCUMvField* pcCUMvFieldA = ( pcCUA == NULL || pcCUA->isIntra( uiAbsPartIdxA ) ) ? NULL : pcCUA->getCUMvField( eRefList );
  iHorPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsHor() ) +
       ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsHor() );
  iVerPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsVer() ) +
       ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsVer() );

#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitMVD += xWriteSvlc( iHor);
  else
    xWriteSvlc( iHor);
  if (m_bAdaptFlag)      
    m_uiBitMVD += xWriteSvlc( iVer);
  else
    xWriteSvlc( iVer);
#else
  xWriteSvlc( iHor );
  xWriteSvlc( iVer );
#endif

  return;
}

Void TEncCavlc::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getSlice()->getSliceQp();

#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitDeltaQP += 1;
#endif

  if ( iDQp == 0 )
  {
    xWriteFlag( 0 );
  }
  else
  {
    xWriteFlag( 1 );
#if LCEC_STAT
    if (m_bAdaptFlag)      
      m_uiBitDeltaQP += xWriteSvlc( iDQp );
    else
#endif
    xWriteSvlc( iDQp );
  }

  return;
}

#if LCEC_CBP_YUV_ROOT
Void TEncCavlc::codeCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  if (eType == TEXT_ALL)
  {
    Int n,x,cx,y,cy;
    UInt uiCBFY = pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, 0);
    UInt uiCBFU = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, 0);
    UInt uiCBFV = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, 0);
    UInt uiCBP = (uiCBFV<<2) + (uiCBFU<<1) + (uiCBFY<<0);

    /* Start adaptation */
    n = pcCU->isIntra( uiAbsPartIdx ) ? 0 : 1;
    x = uiCBP;
    cx = m_uiCBPTableE[n][x];
    
    UInt vlcn = g_auiCbpVlcNum[n][m_uiCbpVlcIdx[n]];

    if ( m_bAdaptFlag )
    {                
        
        cy = Max(0,cx-1);
        y = m_uiCBPTableD[n][cy];
        m_uiCBPTableD[n][cy] = x;
        m_uiCBPTableD[n][cx] = y;
        m_uiCBPTableE[n][x] = cy;
        m_uiCBPTableE[n][y] = cx;
        m_uiCbpVlcIdx[n] += cx == m_uiCbpVlcIdx[n] ? 0 : (cx < m_uiCbpVlcIdx[n] ? -1 : 1);
    }
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitCbp += xWriteVlc( vlcn, cx );
    else
#endif
    xWriteVlc( vlcn, cx );
  }
}

Void TEncCavlc::codeBlockCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiQPartNum, Bool bRD )
{
  UInt uiCbf0 = pcCU->getCbf   ( uiAbsPartIdx, eType, uiTrDepth );
  UInt uiCbf1 = pcCU->getCbf   ( uiAbsPartIdx + uiQPartNum, eType, uiTrDepth );
  UInt uiCbf2 = pcCU->getCbf   ( uiAbsPartIdx + uiQPartNum*2, eType, uiTrDepth );
  UInt uiCbf3 = pcCU->getCbf   ( uiAbsPartIdx + uiQPartNum*3, eType, uiTrDepth );
  UInt uiCbf = (uiCbf0<<3) | (uiCbf1<<2) | (uiCbf2<<1) | uiCbf3;

  assert(uiTrDepth > 0);

#if QC_BLK_CBP
  if(bRD && uiCbf==0)
  {
    xWriteCode(0, 4); 
    return;
  }

  assert(uiCbf > 0);

  uiCbf --;

  Int x,cx,y,cy;

  UInt n = (pcCU->isIntra(uiAbsPartIdx) && eType == TEXT_LUMA)? 0:1;
  cx = m_uiBlkCBPTableE[n][uiCbf];
  x = uiCbf;
  UInt vlcn = (n==0)?g_auiBlkCbpVlcNum[m_uiBlkCbpVlcIdx]:11;

  if ( m_bAdaptFlag )
  {                
    cy = Max(0,cx-1);
    y = m_uiBlkCBPTableD[n][cy];
    m_uiBlkCBPTableD[n][cy] = x;
    m_uiBlkCBPTableD[n][cx] = y;
    m_uiBlkCBPTableE[n][x] = cy;
    m_uiBlkCBPTableE[n][y] = cx;
    if(n==0)
      m_uiBlkCbpVlcIdx += cx == m_uiBlkCbpVlcIdx ? 0 : (cx < m_uiBlkCbpVlcIdx ? -1 : 1);

  }

  xWriteVlc( vlcn, cx );
  return;
#else
  xWriteCode(uiCbf, 4);
#endif
}
#endif

Void TEncCavlc::codeCoeffNxN    ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType, Bool bRD )
{
  if ( uiWidth > m_pcSlice->getSPS()->getMaxTrSize() ) {
    uiWidth  = m_pcSlice->getSPS()->getMaxTrSize();
    uiHeight = m_pcSlice->getSPS()->getMaxTrSize();
  }
  UInt uiSize   = uiWidth*uiHeight;

  // point to coefficient
  TCoeff* piCoeff = pcCoef;
  UInt uiNumSig = 0;
  UInt uiScanning;

  // compute number of significant coefficients
  UInt  uiPart = 0;
  xCheckCoeff(piCoeff, uiWidth, 0, uiNumSig, uiPart );

  if ( bRD ) {
    UInt uiTempDepth = uiDepth - pcCU->getDepth( uiAbsPartIdx );
    pcCU->setCbfSubParts( ( uiNumSig ? 1 : 0 ) << uiTempDepth, eTType, uiAbsPartIdx, uiDepth );
    codeCbf( pcCU, uiAbsPartIdx, eTType, uiTempDepth );
  }

  if ( uiNumSig == 0 ) {
    return;
  }

  // initialize scan
  const UInt*  pucScan;
  //UInt uiConvBit = g_aucConvertToBit[ Min(8,uiWidth)    ];
  UInt uiConvBit = g_aucConvertToBit[ pcCU->isIntra( uiAbsPartIdx ) ? uiWidth : Min(8,uiWidth)    ];
  pucScan        = g_auiFrameScanXY [ uiConvBit + 1 ];

  TCoeff scoeff[64];
  Int iBlockType;
  UInt uiCodeDCCoef = 0;
  TCoeff dcCoeff = 0;
  if (pcCU->isIntra(uiAbsPartIdx))
  {
    UInt uiAbsPartIdxL, uiAbsPartIdxA;
    TComDataCU* pcCUL   = pcCU->getPULeft (uiAbsPartIdxL, pcCU->getZorderIdxInCU() + uiAbsPartIdx);
    TComDataCU* pcCUA   = pcCU->getPUAbove(uiAbsPartIdxA, pcCU->getZorderIdxInCU() + uiAbsPartIdx);
    if (pcCUL == NULL && pcCUA == NULL)
    {
#if 1
      uiCodeDCCoef = 1;
      xWriteVlc((eTType == TEXT_LUMA ? 3 : 1) , abs(piCoeff[0]));
      if (piCoeff[0] != 0)
      {
        UInt sign = (piCoeff[0] < 0) ? 1 : 0;
        xWriteFlag(sign);
      }
      dcCoeff = piCoeff[0];
      piCoeff[0] = 1;
#else
        xWriteFlag(1);
#endif
    }
  }

  if( uiSize == 2*2 )
  {
    // hack: re-use 4x4 coding
    ::memset( scoeff, 0, 16*sizeof(TCoeff) );
    for (uiScanning=0; uiScanning<4; uiScanning++)
    {
      scoeff[15-uiScanning] = piCoeff[ pucScan[ uiScanning ] ];
    }
    iBlockType = pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType();

    xCodeCoeff4x4( scoeff, iBlockType );
  }
  else if ( uiSize == 4*4 )
  {
    for (uiScanning=0; uiScanning<16; uiScanning++)
    {
      scoeff[15-uiScanning] = piCoeff[ pucScan[ uiScanning ] ];
    }
    iBlockType = pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType();

    xCodeCoeff4x4( scoeff, iBlockType );
  }
  else if ( uiSize == 8*8 )
  {
    for (uiScanning=0; uiScanning<64; uiScanning++)
    {
      scoeff[63-uiScanning] = piCoeff[ pucScan[ uiScanning ] ];
    }
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
      iBlockType = eTType-2;
    else
    iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );

    xCodeCoeff8x8( scoeff, iBlockType );
  }
  else
  {
    if(!pcCU->isIntra( uiAbsPartIdx ))
    {
      for (uiScanning=0; uiScanning<64; uiScanning++)
      {
        scoeff[63-uiScanning] = piCoeff[(pucScan[uiScanning]/8)*uiWidth + (pucScan[uiScanning]%8)];      
	    }
      if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V) 
        iBlockType = eTType-2;
      else
        iBlockType = 5 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
      xCodeCoeff8x8( scoeff, iBlockType );
      return;
    }    

    if(pcCU->isIntra( uiAbsPartIdx ))
    {
      for (uiScanning=0; uiScanning<64; uiScanning++)
      {
        scoeff[63-uiScanning] = piCoeff[ pucScan[ uiScanning ] ];
      }

      if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V) 
        iBlockType = eTType-2;
      else
        iBlockType = 5 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
      xCodeCoeff8x8( scoeff, iBlockType );
    }
//#endif
  }

  if (uiCodeDCCoef == 1)
  {
    piCoeff[0] = dcCoeff;
  }
}

Void TEncCavlc::codeAlfFlag( UInt uiCode )
{
  
  xWriteFlag( uiCode );
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitAlfFlag += 1;
#endif
}

#if TSB_ALF_HEADER
Void TEncCavlc::codeAlfFlagNum( UInt uiCode, UInt minValue )
{
  UInt uiLength = 0;
  UInt maxValue = (minValue << (this->getMaxAlfCtrlDepth()*2));
  assert((uiCode>=minValue)&&(uiCode<=maxValue));
  UInt temp = maxValue - minValue;
  for(UInt i=0; i<32; i++)
  {
    if(temp&0x1)
    {
      uiLength = i+1;
    }
    temp = (temp >> 1);
  }
  if(uiLength)
  {
    xWriteCode( uiCode - minValue, uiLength );
  }
}

Void TEncCavlc::codeAlfCtrlFlag( UInt uiSymbol )
{
  xWriteFlag( uiSymbol );
}
#endif

Void TEncCavlc::codeAlfUvlc( UInt uiCode )
{
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitAlfUvlc += xWriteUvlc( uiCode );
  else
#endif
  xWriteUvlc( uiCode );
}

Void TEncCavlc::codeAlfSvlc( Int iCode )
{
#if LCEC_STAT
  if (m_bAdaptFlag)
    m_uiBitAlfSvlc += xWriteSvlc( iCode );
  else
#endif
  xWriteSvlc( iCode );
}

Void TEncCavlc::estBit( estBitsSbacStruct* pcEstBitsCabac, UInt uiCTXIdx, TextType eTType )
{
  // printf("error : no VLC mode support in this version\n");
  return;
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

#if LCEC_STAT
UInt TEncCavlc::xWriteCode     ( UInt uiCode, UInt uiLength )
#else
Void TEncCavlc::xWriteCode     ( UInt uiCode, UInt uiLength )
#endif
{
  assert ( uiLength > 0 );
  m_pcBitIf->write( uiCode, uiLength );
#if LCEC_STAT
  return uiLength;
#endif
}

#if LCEC_STAT
UInt TEncCavlc::xWriteUvlc     ( UInt uiCode )
#else
Void TEncCavlc::xWriteUvlc     ( UInt uiCode )
#endif
{
  UInt uiLength = 1;
  UInt uiTemp = ++uiCode;

  assert ( uiTemp );

  while( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }

  m_pcBitIf->write( uiCode, uiLength );
#if LCEC_STAT
  return uiLength;
#endif
}

#if LCEC_STAT
UInt TEncCavlc::xWriteSvlc     ( Int iCode )
{
  UInt uiCode;
  UInt uiNumBits;

  uiCode = xConvertToUInt( iCode );
  uiNumBits = xWriteUvlc( uiCode );
  return uiNumBits;
}
#else
Void TEncCavlc::xWriteSvlc     ( Int iCode )
{
  UInt uiCode;

  uiCode = xConvertToUInt( iCode );
  xWriteUvlc( uiCode );
}
#endif

Void TEncCavlc::xWriteFlag( UInt uiCode )
{
  m_pcBitIf->write( uiCode, 1 );
}

Void TEncCavlc::xCheckCoeff( TCoeff* pcCoef, UInt uiSize, UInt uiDepth, UInt& uiNumofCoeff, UInt& uiPart )
{
  UInt ui = uiSize>>uiDepth;
  if( uiPart == 0 )
  {
    if( ui <= 4 )
    {
      UInt x, y;
      TCoeff* pCeoff = pcCoef;
      for( y=0 ; y<ui ; y++ )
      {
        for( x=0 ; x<ui ; x++ )
        {
          if( pCeoff[x] != 0 )
          {
            uiNumofCoeff++;
          }
        }
        pCeoff += uiSize;
      }
    }
    else
    {
      xCheckCoeff( pcCoef,                            uiSize, uiDepth+1, uiNumofCoeff, uiPart ); uiPart++; //1st Part
      xCheckCoeff( pcCoef             + (ui>>1),      uiSize, uiDepth+1, uiNumofCoeff, uiPart ); uiPart++; //2nd Part
      xCheckCoeff( pcCoef + (ui>>1)*uiSize,           uiSize, uiDepth+1, uiNumofCoeff, uiPart ); uiPart++; //3rd Part
      xCheckCoeff( pcCoef + (ui>>1)*uiSize + (ui>>1), uiSize, uiDepth+1, uiNumofCoeff, uiPart );           //4th Part
    }
  }
  else
  {
    UInt x, y;
    TCoeff* pCeoff = pcCoef;
    for( y=0 ; y<ui ; y++ )
    {
      for( x=0 ; x<ui ; x++ )
      {
        if( pCeoff[x] != 0 )
        {
          uiNumofCoeff++;
        }
      }
      pCeoff += uiSize;
    }
  }
}

#if LCEC_STAT
UInt TEncCavlc::xWriteUnaryMaxSymbol( UInt uiSymbol, UInt uiMaxSymbol )
#else
Void TEncCavlc::xWriteUnaryMaxSymbol( UInt uiSymbol, UInt uiMaxSymbol )
#endif
{
#if LCEC_STAT
  UInt uiNumBits = 0;
#endif
  xWriteFlag( uiSymbol ? 1 : 0 );
#if LCEC_STAT
  uiNumBits +=1;
#endif
  if ( uiSymbol == 0 )
  {
#if LCEC_STAT
    return uiNumBits;
#else
    return;
#endif
  }

  Bool bCodeLast = ( uiMaxSymbol > uiSymbol );

  while( --uiSymbol )
  {
    xWriteFlag( 1 );
#if LCEC_STAT
    uiNumBits +=1;
#endif
  }
  if( bCodeLast )
  {
    xWriteFlag( 0 );
#if LCEC_STAT
    uiNumBits +=1;
#endif
  }
#if LCEC_STAT
  return uiNumBits;
#else
  return;
#endif
}

#if LCEC_STAT
UInt TEncCavlc::xWriteExGolombLevel( UInt uiSymbol )
{
  UInt uiNumBits = 0;
#else
Void TEncCavlc::xWriteExGolombLevel( UInt uiSymbol )
{
#endif
  if( uiSymbol )
  {
    xWriteFlag( 1 );
#if LCEC_STAT
    uiNumBits += 1;
#endif
    UInt uiCount = 0;
    Bool bNoExGo = (uiSymbol < 13);

    while( --uiSymbol && ++uiCount < 13 )
    {
      xWriteFlag( 1 );
#if LCEC_STAT
      uiNumBits += 1;
#endif
    }
    if( bNoExGo )
    {
      xWriteFlag( 0 );
#if LCEC_STAT
      uiNumBits += 1;
#endif
    }
    else
    {
#if LCEC_STAT     
      uiNumBits += xWriteEpExGolomb( uiSymbol, 0 );
#else
      xWriteEpExGolomb( uiSymbol, 0 );
#endif
    }
  }
  else
  {
    xWriteFlag( 0 );
#if LCEC_STAT
    uiNumBits += 1;
#endif
  }

#if LCEC_STAT
  return uiNumBits;
#else
  return;
#endif
}

#if LCEC_STAT
UInt TEncCavlc::xWriteEpExGolomb( UInt uiSymbol, UInt uiCount )
{
  UInt uiNumBits = 0;
#else
Void TEncCavlc::xWriteEpExGolomb( UInt uiSymbol, UInt uiCount )
{
#endif
  while( uiSymbol >= (UInt)(1<<uiCount) )
  {
    xWriteFlag( 1 );
#if LCEC_STAT    
    uiNumBits += 1;
#endif
    uiSymbol -= 1<<uiCount;
    uiCount  ++;
  }
  xWriteFlag( 0 );
#if LCEC_STAT    
  uiNumBits += 1;
#endif
  while( uiCount-- )
  {
    xWriteFlag( (uiSymbol>>uiCount) & 1 );
#if LCEC_STAT    
    uiNumBits += 1;
#endif
  }
#if LCEC_STAT
  return uiNumBits;
#else
  return;
#endif
}

UInt TEncCavlc::xLeadingZeros(UInt uiCode)
{
  UInt uiCount = 0;
  Int iDone = 0;

  if (uiCode)
  {
    while (!iDone)
    {
      uiCode >>= 1;
      if (!uiCode) iDone = 1;
      else uiCount++;
    }
  }
  return uiCount;
}

#if LCEC_STAT
UInt TEncCavlc::xWriteVlc(UInt uiTableNumber, UInt uiCodeNumber)
{
  UInt uiNumBits = 0;
#else
Void TEncCavlc::xWriteVlc(UInt uiTableNumber, UInt uiCodeNumber)
{
#endif
#if QC_BLK_CBP
  assert( uiTableNumber<=11 );
#else
  assert( uiTableNumber<=10 );
#endif

  UInt uiTemp;
  UInt uiLength = 0;
  UInt uiCode = 0;

  if ( uiTableNumber < 5 )
  {
    if ((Int)uiCodeNumber < (6 * (1 << uiTableNumber)))
    {
      uiTemp = 1<<uiTableNumber;
      uiCode = uiTemp+uiCodeNumber%uiTemp;
      uiLength = 1+uiTableNumber+(uiCodeNumber>>uiTableNumber);
    }
    else
    {
      uiCode = uiCodeNumber - (6 * (1 << uiTableNumber)) + (1 << uiTableNumber);
      uiLength = (6-uiTableNumber)+1+2*xLeadingZeros(uiCode);
    }
  }
  else if (uiTableNumber < 8)
  {
    uiTemp = 1<<(uiTableNumber-4);
    uiCode = uiTemp+uiCodeNumber%uiTemp;
    uiLength = 1+(uiTableNumber-4)+(uiCodeNumber>>(uiTableNumber-4));
  }
  else if (uiTableNumber == 8)
  {
    assert( uiCodeNumber<=2 );
    if (uiCodeNumber == 0)
    {
      uiCode = 1;
      uiLength = 1;
    }
    else if (uiCodeNumber == 1)
    {
      uiCode = 1;
      uiLength = 2;
    }
    else if (uiCodeNumber == 2)
    {
      uiCode = 0;
      uiLength = 2;
    }
  }
  else if (uiTableNumber == 9)
  {
    if (uiCodeNumber == 0)
    {
      uiCode = 4;
      uiLength = 3;
    }
    else if (uiCodeNumber == 1)
    {
      uiCode = 10;
      uiLength = 4;
    }
    else if (uiCodeNumber == 2)
    {
      uiCode = 11;
      uiLength = 4;
    }
    else if (uiCodeNumber < 11)
    {
      uiCode = uiCodeNumber+21;
      uiLength = 5;
    }
    else
    {
      uiTemp = 1<<4;
      uiCode = uiTemp+(uiCodeNumber+5)%uiTemp;
      uiLength = 5+((uiCodeNumber+5)>>4);
    }
  }
  else if (uiTableNumber == 10)
  {
    uiCode = uiCodeNumber+1;
    uiLength = 1+2*xLeadingZeros(uiCode);
  }
#if QC_BLK_CBP
  else if (uiTableNumber == 11)
  {
    if (uiCodeNumber == 0)
    {
      uiCode = 0;
      uiLength = 3;
    }
    else
    {
      uiCode = uiCodeNumber + 1;
      uiLength = 4;
    }
  }
#endif
  xWriteCode(uiCode, uiLength);
#if LCEC_STAT
  return uiLength;
#endif
}

Void TEncCavlc::xCodeCoeff4x4(TCoeff* scoeff, Int n )
{
  Int i;
  UInt cn;
  Int level,vlc,sign,done,last_pos,start;
  Int run_done,maxrun,run,lev;
  Int tmprun, vlc_adaptive=0;
  static const int atable[5] = {4,6,14,28,0xfffffff};
  Int tmp;

  /* Do the last coefficient first */
  i = 0;
  done = 0;

  while (!done && i < 16)
  {
    if (scoeff[i])
    {
      done = 1;
    }
    else
    {
      i++;
    }
  }
  if (i == 16)
  {
    return;
  }

  last_pos = 15-i;
  level = abs(scoeff[i]);
  lev = (level == 1) ? 0 : 1;

  {
    int x,y,cx,cy,vlcNum;
    int vlcTable[3] = {2,2,2};

    x = 16*lev + last_pos;
    
    cx = m_uiLPTableE4[n][x];
    vlcNum = vlcTable[n];

#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitCoeff += xWriteVlc( vlcNum, cx );
    else
#endif
    xWriteVlc( vlcNum, cx );

    if ( m_bAdaptFlag )
    {
     
      cy = Max( 0, cx-1 );
      y = m_uiLPTableD4[n][cy];
      m_uiLPTableD4[n][cy] = x;
      m_uiLPTableD4[n][cx] = y;
      m_uiLPTableE4[n][x] = cy;
      m_uiLPTableE4[n][y] = cx;
    }
  }

  sign = (scoeff[i] < 0) ? 1 : 0;
  if (level > 1)
  {
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitCoeff += xWriteVlc( 0, 2*(level-2)+sign );
    else
#endif
    xWriteVlc( 0, 2*(level-2)+sign );
  }
  else
  {
    xWriteFlag( sign );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitCoeff += 1;
#endif
  }
  i++;

  if (i < 16)
  {
    /* Go into run mode */
    run_done = 0;
    while (!run_done)
    {
      maxrun = 15-i;
      tmprun = maxrun;
      if (maxrun > 27)
      {
        vlc = 3;
        tmprun = 28;
      }
      else
      {
        vlc = g_auiVlcTable8x8[maxrun];
      }

      run = 0;
      done = 0;
      while (!done)
      {
        if (!scoeff[i])
        {
          run++;
        }
        else
        {
          level = abs(scoeff[i]);
          lev = (level == 1) ? 0 : 1;
          if (maxrun > 27)
          {
            cn = g_auiLumaRun8x8[28][lev][run];
          }
          else
          {
            cn = g_auiLumaRun8x8[maxrun][lev][run];
          }
#if LCEC_STAT
          if (m_bAdaptFlag)
            m_uiBitCoeff += xWriteVlc( vlc, cn );
          else
#endif
          xWriteVlc( vlc, cn );

          sign = (scoeff[i] < 0) ? 1 : 0;
          if (level > 1)
          {
#if LCEC_STAT
            if (m_bAdaptFlag)
              m_uiBitCoeff += xWriteVlc( 0, 2*(level-2)+sign );
            else
#endif
            xWriteVlc( 0, 2*(level-2)+sign );
            run_done = 1;
          }
          else
          {
            xWriteFlag( sign );
#if LCEC_STAT
            if (m_bAdaptFlag)
              m_uiBitCoeff += 1;
#endif
          }

          run = 0;
          done = 1;
        }
        if (i == 15)
        {
          done = 1;
          run_done = 1;
          if (run)
          {
            if (maxrun > 27)
            {
              cn = g_auiLumaRun8x8[28][0][run];
            }
            else
            {
              cn = g_auiLumaRun8x8[maxrun][0][run];
            }
#if LCEC_STAT
            if (m_bAdaptFlag)
              m_uiBitCoeff += xWriteVlc( vlc, cn );
            else
#endif
            xWriteVlc( vlc, cn );
          }
        }
        i++;
      }
    }
  }

  /* Code the rest in level mode */
  start = i;
  for ( i=start; i<16; i++ )
  {
    tmp = abs(scoeff[i]);
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitCoeff += xWriteVlc( vlc_adaptive, tmp );
    else
#endif
    xWriteVlc( vlc_adaptive, tmp );
    if (scoeff[i])
    {
      sign = (scoeff[i] < 0) ? 1 : 0;
      xWriteFlag( sign );
#if LCEC_STAT
      if (m_bAdaptFlag)
        m_uiBitCoeff += 1;
#endif
    }
    if ( tmp > atable[vlc_adaptive] )
    {
      vlc_adaptive++;
    }
  }
  return;
}

Void TEncCavlc::xCodeCoeff8x8( TCoeff* scoeff, Int n )
{
  int i;
  unsigned int cn;
  int level,vlc,sign,done,last_pos,start;
  int run_done,maxrun,run,lev;
  int tmprun,vlc_adaptive=0;
  static const int atable[5] = {4,6,14,28,0xfffffff};
  int tmp;

  static const int switch_thr[10] = {49,49,0,49,49,0,49,49,49,49};
  int sum_big_coef = 0;


  /* Do the last coefficient first */
  i = 0;
  done = 0;
  while (!done && i < 64)
  {
    if (scoeff[i])
    {
      done = 1;
    }
    else
    {
      i++;
    }
  }
  if (i == 64)
  {
    return;
  }

  last_pos = 63-i;
  level = abs(scoeff[i]);
  lev = (level == 1) ? 0 : 1;

  {
    int x,y,cx,cy,vlcNum;
    x = 64*lev + last_pos;
    
    cx = m_uiLPTableE8[n][x];
    // ADAPT_VLC_NUM
    vlcNum = g_auiLastPosVlcNum[n][Min(16,m_uiLastPosVlcIndex[n])];
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitCoeff += xWriteVlc( vlcNum, cx );
    else
#endif
    xWriteVlc( vlcNum, cx );

    if ( m_bAdaptFlag )
    {
        
        // ADAPT_VLC_NUM
        m_uiLastPosVlcIndex[n] += cx == m_uiLastPosVlcIndex[n] ? 0 : (cx < m_uiLastPosVlcIndex[n] ? -1 : 1);
        cy = Max(0,cx-1);
        y = m_uiLPTableD8[n][cy];
        m_uiLPTableD8[n][cy] = x;
        m_uiLPTableD8[n][cx] = y;
        m_uiLPTableE8[n][x] = cy;
        m_uiLPTableE8[n][y] = cx;
    }
  }

  sign = (scoeff[i] < 0) ? 1 : 0;
  if (level > 1)
  {
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitCoeff += xWriteVlc( 0, 2*(level-2)+sign );
    else
#endif
    xWriteVlc( 0, 2*(level-2)+sign );
  }
  else
  {
    xWriteFlag( sign );
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitCoeff += 1;
#endif
  }
  i++;

  if (i < 64)
  {
    /* Go into run mode */
    run_done = 0;
    while ( !run_done )
    {
      maxrun = 63-i;
      tmprun = maxrun;
      if (maxrun > 27)
      {
        vlc = 3;
        tmprun = 28;
      }
      else
      {
        vlc = g_auiVlcTable8x8[maxrun];
      }

      run = 0;
      done = 0;
      while (!done)
      {
        if (!scoeff[i])
        {
          run++;
        }
        else
        {
          level = abs(scoeff[i]);
          lev = (level == 1) ? 0 : 1;
          if (maxrun > 27)
          {
            cn = g_auiLumaRun8x8[28][lev][run];
          }
          else
          {
            cn = g_auiLumaRun8x8[maxrun][lev][run];
          }
#if LCEC_STAT
          if (m_bAdaptFlag)
            m_uiBitCoeff += xWriteVlc( vlc, cn );
          else
#endif
          xWriteVlc( vlc, cn );

          sign = (scoeff[i] < 0) ? 1 : 0;
          if (level > 1)
          {
#if LCEC_STAT
            if (m_bAdaptFlag)
              m_uiBitCoeff += xWriteVlc( 0, 2*(level-2)+sign );
            else
#endif
            xWriteVlc( 0, 2*(level-2)+sign );

            sum_big_coef += level;
            if (i > switch_thr[n] || sum_big_coef > 2)
            {
              run_done = 1;
            }
          }
          else
          {
            xWriteFlag( sign );
#if LCEC_STAT
            if (m_bAdaptFlag)
              m_uiBitCoeff += 1;
#endif
          }
          run = 0;
          done = 1;
        }
        if (i == 63)
        {
          done = 1;
          run_done = 1;
          if (run)
          {
            if (maxrun > 27)
            {
              cn = g_auiLumaRun8x8[28][0][run];
            }
            else
            {
              cn = g_auiLumaRun8x8[maxrun][0][run];
            }
#if LCEC_STAT
            if (m_bAdaptFlag)
              m_uiBitCoeff += xWriteVlc( vlc, cn );
            else
#endif
            xWriteVlc( vlc, cn );
          }
        }
        i++;
      }
    }
  }

  /* Code the rest in level mode */
  start = i;
  for ( i=start; i<64; i++ )
  {
    tmp = abs(scoeff[i]);
#if LCEC_STAT
    if (m_bAdaptFlag)
      m_uiBitCoeff += xWriteVlc( vlc_adaptive, tmp );
    else
#endif
    xWriteVlc( vlc_adaptive, tmp );
    if (scoeff[i])
    {
      sign = (scoeff[i] < 0) ? 1 : 0;
      xWriteFlag( sign );
#if LCEC_STAT
      if (m_bAdaptFlag)
        m_uiBitCoeff += 1;
#endif
    }
    if (tmp>atable[vlc_adaptive])
    {
      vlc_adaptive++;
    }
  }

  return;
}
