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

/** \file     TAppDecTop.cpp
    \brief    Decoder application class
*/

#include <list>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "TAppDecTop.h"
#include "../../Lib/TLibDecoder/AnnexBread.h"
#include "../../Lib/TLibDecoder/NALread.h"

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppDecTop::TAppDecTop()
{
  ::memset (m_abDecFlag, 0, sizeof (m_abDecFlag));
  m_iPOCLastDisplay  = -1;
}

Void TAppDecTop::create()
{
}

Void TAppDecTop::destroy()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 - create internal class
 - initialize internal class
 - until the end of the bitstream, call decoding function in TDecTop class
 - delete allocated buffers
 - destroy internal class
 .
 */
Void TAppDecTop::decode()
{
  UInt                uiPOC;
  TComList<TComPic*>* pcListPic = NULL;

  ifstream bitstreamFile(m_pchBitstreamFile, ifstream::in | ifstream::binary);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", m_pchBitstreamFile);
    exit(EXIT_FAILURE);
  }

  InputByteStream bytestream(bitstreamFile);

  // create & initialize internal classes
  xCreateDecLib();
  xInitDecLib  ();
  m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.

  // main decoder loop
  bool recon_opened = false; // reconstruction file not yet opened. (must be performed after SPS is seen)

  while (!!bitstreamFile)
  {
    /* location serves to work around a design fault in the decoder, whereby
     * the process of reading a new slice that is the first slice of a new frame
     * requires the TDecTop::decode() method to be called again with the same
     * nal unit. */
    streampos location = bitstreamFile.tellg();
    AnnexBStats stats = AnnexBStats();

    vector<uint8_t> nalUnit;
    byteStreamNALUnit(bytestream, nalUnit, stats);

    // call actual decoding function
    bool bNewPicture = false;
    if (nalUnit.empty())
      /* this can happen if the following occur:
       *  - empty input file
       *  - two back-to-back start_code_prefixes
       *  - start_code_prefix immediately followed by EOF
       */
      fprintf(stderr, "Warning: Attempt to decode an empty NAL unit\n");
    else
    {
      InputNALUnit nalu;
      read(nalu, nalUnit);
      bNewPicture = m_cTDecTop.decode(nalu, m_iSkipFrame, m_iPOCLastDisplay);
      if (bNewPicture)
      {
        bitstreamFile.clear();
        /* location points to the current nalunit payload[1] due to the
         * need for the annexB parser to read three extra bytes.
         * [1] except for the first NAL unit in the file
         *     (but bNewPicture doesn't happen then) */
        bitstreamFile.seekg(location-streamoff(3));
        bytestream.reset();
      }
    }
    if (bNewPicture || !bitstreamFile)
    {
      m_cTDecTop.executeDeblockAndAlf(uiPOC, pcListPic, m_iSkipFrame, m_iPOCLastDisplay);
    }

    if( pcListPic )
    {
      if ( m_pchReconFile && !recon_opened )
      {
        if ( m_outputBitDepth == 0 )
          m_outputBitDepth = g_uiBitDepth + g_uiBitIncrement;

        m_cTVideoIOYuvReconFile.open( m_pchReconFile, true, m_outputBitDepth, g_uiBitDepth + g_uiBitIncrement ); // write mode
        recon_opened = true;
      }
      // write reconstuction to file
      xWriteOutput( pcListPic );
    }
  }
  
  // delete buffers
  m_cTDecTop.deletePicBuffer();
  
  // destroy internal classes
  xDestroyDecLib();
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TAppDecTop::xCreateDecLib()
{
  // create decoder class
  m_cTDecTop.create();
}

Void TAppDecTop::xDestroyDecLib()
{
  if ( m_pchReconFile )
  {
    m_cTVideoIOYuvReconFile. close();
  }
  
  // destroy decoder class
  m_cTDecTop.destroy();
}

Void TAppDecTop::xInitDecLib()
{
  // initialize decoder class
  m_cTDecTop.init();
  m_cTDecTop.setPictureDigestEnabled(m_pictureDigestEnabled);
}

/** \param pcListPic list of pictures to be written to file
    \param bFirst    first picture?
    \todo            DYN_REF_FREE should be revised
 */
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic )
{
  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();
  
  while (iterPic != pcListPic->end())
  {
    TComPic* pcPic = *(iterPic);
    
    if ( pcPic->getReconMark() && pcPic->getPOC() == (m_iPOCLastDisplay + 1) )
    {
      // write to file
      if ( m_pchReconFile )
      {
        m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(), pcPic->getSlice(0)->getSPS()->getPad() );
      }
      
      // update POC of display order
      m_iPOCLastDisplay = pcPic->getPOC();
      
      // erase non-referenced picture in the reference picture list after display
      if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
      {
#if !DYN_REF_FREE
        pcPic->setReconMark(false);
        
        // mark it should be extended later
        pcPic->getPicYuvRec()->setBorderExtension( false );
        
#else
        pcPic->destroy();
        pcListPic->erase( iterPic );
        iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
        continue;
#endif
      }
    }
    
    iterPic++;
  }
}
