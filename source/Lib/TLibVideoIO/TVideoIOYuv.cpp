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

/** \file     TVideoIOYuv.cpp
    \brief    YUV file I/O class
*/

#include <cstdlib>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

#include "TVideoIOYuv.h"

using namespace std;

/**
 * Multiply all pixels in #img by \f$ 2^{#shiftbits} \f$.
 *
 * @param stride  distance between vertically adjacent pixels of #img.
 * @param width   width of active area in #img.
 * @param height  height of active area in #img.
 */
static void scalePlane(Pel* img, unsigned int stride, unsigned int width, unsigned int height,
                       unsigned int shiftbits)
{
  for (unsigned int y = 0; y < height; y++)
  {
    for (unsigned int x = 0; x < width; x++)
    {
      img[x] <<= shiftbits;
    }
    img += stride;
  }
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 \param pchFile    file name string
 \param bWriteMode file open mode
 \param fileBitDepth     bit-depth of input/output file data.
 \param internalBitDepth bit-depth to scale image data to/from when reading/writing.
 */
Void TVideoIOYuv::open( char* pchFile, Bool bWriteMode, unsigned int fileBitDepth, unsigned int internalBitDepth )
{
  m_bitdepthShift = internalBitDepth - fileBitDepth;
  m_fileBitdepth = fileBitDepth;
  assert(m_bitdepthShift >= 0);

  if ( bWriteMode )
  {
    m_cHandle.open( pchFile, ios::binary | ios::out );
    
    if( m_cHandle.fail() )
    {
      printf("\nfailed to write reconstructed YUV file\n");
      exit(0);
    }
  }
  else
  {
    m_cHandle.open( pchFile, ios::binary | ios::in );
    
    if( m_cHandle.fail() )
    {
      printf("\nfailed to open Input YUV file\n");
      exit(0);
    }
  }
  
  return;
}

Void TVideoIOYuv::close()
{
  m_cHandle.close();
}

Bool TVideoIOYuv::isEof()
{
  return m_cHandle.eof();
}

/**
 * Read \f$ #width * #height \f$ pixels from #fd into #dst, optionally
 * padding the left and right edges by edge-extension.  Input may be
 * either 8bit or 16bit little-endian lsb-aligned words.
 *
 * @param dst     destination image
 * @param is16bit true if input file carries > 8bit data, false otherwise.
 * @param stride  distance between vertically adjacent pixels of #dst.
 * @param width   width of active area in #dst.
 * @param height  height of active area in #dst.
 * @param pad_x   length of horizontal padding.
 * @param pad_y   length of vertical padding.
 */
static void readPlane(Pel* dst, istream& fd, bool is16bit,
                      unsigned int stride,
                      unsigned int width, unsigned int height,
                      unsigned int pad_x, unsigned int pad_y)
{
  int read_len = width * (is16bit ? 2 : 1);
  unsigned char *buf = new unsigned char[read_len];
  for (int y = 0; y < height; y++)
  {
    fd.read(reinterpret_cast<char*>(buf), read_len);
    if (!is16bit) {
      for (int x = 0; x < width; x++) {
        dst[x] = buf[x];
      }
    }
    else {
      for (int x = 0; x < width; x++) {
        dst[x] = (buf[2*x+1] << 8) | buf[2*x];
      }
    }

    for (int x = width; x < width + pad_x; x++)
    {
      dst[x] = dst[width - 1];
    }
    dst += stride;
  }
  for (int y = height; y < height + pad_y; y++)
  {
    for (int x = width; x < width + pad_x; x++)
    {
      dst[x] = dst[x - stride];
    }
    dst += stride;
  }
  delete[] buf;
}

/**
 * Write \f$ #width * #height \f$ pixels info #fd from #src.
 *
 * @param src     source image
 * @param is16bit true if input file carries > 8bit data, false otherwise.
 * @param stride  distance between vertically adjacent pixels of #src.
 * @param width   width of active area in #src.
 * @param height  height of active area in #src.
 */
static void writePlane(ostream& fd, Pel* src, bool is16bit,
                       unsigned int stride,
                       unsigned int width, unsigned int height)
{
  int write_len = width * (is16bit ? 2 : 1);
  unsigned char *buf = new unsigned char[write_len];
  for (int y = 0; y < height; y++)
  {
    if (!is16bit) {
      for (int x = 0; x < width; x++) {
        buf[x] = src[x];
      }
    }
    else {
      for (int x = 0; x < width; x++) {
        buf[2*x] = src[x] & 0xff;
        buf[2*x+1] = (src[x] >> 8) & 0xff;
      }
    }

    fd.write(reinterpret_cast<char*>(buf), write_len);
    src += stride;
  }
  delete[] buf;
}

/**
 * Read one Y'CbCr frame, performing any required input scaling to change
 * from the bitdepth of the input file to the internal bit-depth.
 *
 \param rpcPicYuv      input picture YUV buffer class pointer
 \param aiPad[2]       source padding size, aiPad[0] = horizontal, aiPad[1] = vertical
 */
Void TVideoIOYuv::read ( TComPicYuv*&  rpcPicYuv, Int aiPad[2] )
{
  // check end-of-file
  if ( isEof() ) return;
  
  Int   iStride = rpcPicYuv->getStride();
  
  // compute actual YUV width & height excluding padding size
  unsigned int pad_h = aiPad[0];
  unsigned int pad_v = aiPad[1];
  unsigned int width_full = rpcPicYuv->getWidth();
  unsigned int height_full = rpcPicYuv->getHeight();
  unsigned int width  = width_full - pad_h;
  unsigned int height = height_full - pad_v;
  bool is16bit = m_fileBitdepth > 8;

  readPlane(rpcPicYuv->getLumaAddr(), m_cHandle, is16bit, iStride, width, height, pad_h, pad_v);
  scalePlane(rpcPicYuv->getLumaAddr(), iStride, width_full, height_full, m_bitdepthShift);

  iStride >>= 1;
  width_full >>= 1;
  height_full >>= 1;
  width >>= 1;
  height >>= 1;
  pad_h >>= 1;
  pad_v >>= 1;

  readPlane(rpcPicYuv->getCbAddr(), m_cHandle, is16bit, iStride, width, height, pad_h, pad_v);
  scalePlane(rpcPicYuv->getCbAddr(), iStride, width_full, height_full, m_bitdepthShift);

  readPlane(rpcPicYuv->getCrAddr(), m_cHandle, is16bit, iStride, width, height, pad_h, pad_v);
  scalePlane(rpcPicYuv->getCrAddr(), iStride, width_full, height_full, m_bitdepthShift);
}

/**
 * Write one Y'CbCr frame. No bit-depth conversion is performed, #pcPicYuv is
 * assumed to be at TVideoIO::m_fileBitdepth depth.
 *
 \param pcPicYuv     input picture YUV buffer class pointer
 \param aiPad[2]     source padding size, aiPad[0] = horizontal, aiPad[1] = vertical
 */
Void TVideoIOYuv::write( TComPicYuv* pcPicYuv, Int aiPad[2] )
{
  // compute actual YUV frame size excluding padding size
  Int   iStride = pcPicYuv->getStride();
  unsigned int width  = pcPicYuv->getWidth() - aiPad[0];
  unsigned int height = pcPicYuv->getHeight() - aiPad[1];
  bool is16bit = m_fileBitdepth > 8;

  writePlane(m_cHandle, pcPicYuv->getLumaAddr(), is16bit, iStride, width, height);

  width >>= 1;
  height >>= 1;
  iStride >>= 1;
  writePlane(m_cHandle, pcPicYuv->getCbAddr(), is16bit, iStride, width, height);
  writePlane(m_cHandle, pcPicYuv->getCrAddr(), is16bit, iStride, width, height);
}

