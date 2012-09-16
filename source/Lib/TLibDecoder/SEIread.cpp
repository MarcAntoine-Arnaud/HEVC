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

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/SEI.h"
#include "SyntaxElementParser.h"
#include "SEIread.h"

//! \ingroup TLibDecoder
//! \{

#if ENC_DEC_TRACE
Void  xTraceSEIHeader()
{
  fprintf( g_hTrace, "=========== SEI message ===========\n");
}

Void  xTraceSEIMessageType(SEI::PayloadType payloadType)
{
  switch (payloadType)
  {
  case SEI::PICTURE_DIGEST:
    fprintf( g_hTrace, "=========== Decoded picture hash SEI message ===========\n");
    break;
  case SEI::USER_DATA_UNREGISTERED:
    fprintf( g_hTrace, "=========== User Data Unregistered SEI message ===========\n");
    break;
  default:
    fprintf( g_hTrace, "=========== Unknown SEI message ===========\n");
    break;
  }
}
#endif

/**
 * unmarshal a single SEI message from bitstream bs
 */
void SEIReader::parseSEImessage(TComInputBitstream* bs, SEImessages& seis)
{
  setBitstream(bs);

  assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  do
  {
    xReadSEImessage(seis);
    /* SEI messages are an integer number of bytes, something has failed
    * in the parsing if bitstream not byte-aligned */
    assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  } while (0x80 != m_pcBitstream->peekBits(8));
  assert(m_pcBitstream->getNumBitsLeft() == 8); /* rsbp_trailing_bits */
}

Void SEIReader::xReadSEImessage(SEImessages& seis)
{
#if ENC_DEC_TRACE
  xTraceSEIHeader();
#endif
  Int payloadType = 0;
  UInt val = 0;

  do
  {
    READ_CODE (8, val, "payload_type");
    payloadType += val;
  } while (val==0xFF);

  UInt payloadSize = 0;
  do
  {
    READ_CODE (8, val, "payload_size");
    payloadSize += val;
  } while (val==0xFF);

#if ENC_DEC_TRACE
  xTraceSEIMessageType((SEI::PayloadType)payloadType);
#endif

  switch (payloadType)
  {
  case SEI::USER_DATA_UNREGISTERED:
    seis.user_data_unregistered = new SEIuserDataUnregistered;
    xParseSEIuserDataUnregistered(*seis.user_data_unregistered, payloadSize);
    break;
  case SEI::PICTURE_DIGEST:
    seis.picture_digest = new SEIpictureDigest;
    xParseSEIpictureDigest(*seis.picture_digest, payloadSize);
    break;
  default:
    assert(!"Unhandled SEI message");
  }
}

/**
 * parse bitstream bs and unpack a user_data_unregistered SEI message
 * of payloasSize bytes into sei.
 */
Void SEIReader::xParseSEIuserDataUnregistered(SEIuserDataUnregistered &sei, unsigned payloadSize)
{
  assert(payloadSize >= 16);
  UInt val;

  for (UInt i = 0; i < 16; i++)
  {
    READ_CODE (8, val, "uuid_iso_iec_11578");
    sei.uuid_iso_iec_11578[i] = val;
  }

  sei.userDataLength = payloadSize - 16;
  if (!sei.userDataLength)
  {
    sei.userData = 0;
    return;
  }

  sei.userData = new UChar[sei.userDataLength];
  for (UInt i = 0; i < sei.userDataLength; i++)
  {
    READ_CODE (8, val, "user_data" );
    sei.userData[i] = val;
  }
}

/**
 * parse bitstream bs and unpack a picture_digest SEI message
 * of payloadSize bytes into sei.
 */
Void SEIReader::xParseSEIpictureDigest(SEIpictureDigest& sei, unsigned payloadSize)
{
  UInt val;
  READ_CODE (8, val, "hash_type");
  sei.method = static_cast<SEIpictureDigest::Method>(val);
  for(int yuvIdx = 0; yuvIdx < 3; yuvIdx++)
  {
    if(SEIpictureDigest::MD5 == sei.method)
    {
      for (unsigned i = 0; i < 16; i++)
      {
        READ_CODE(8, val, "picture_md5");
        sei.digest[yuvIdx][i] = val;
      }
    }
    else if(SEIpictureDigest::CRC == sei.method)
    {
      READ_CODE(16, val, "picture_crc");
      sei.digest[yuvIdx][0] = val >> 8 & 0xFF;
      sei.digest[yuvIdx][1] = val & 0xFF;
    }
    else if(SEIpictureDigest::CHECKSUM == sei.method)
    {
      READ_CODE(32, val, "picture_checksum");
      sei.digest[yuvIdx][0] = (val>>24) & 0xff;
      sei.digest[yuvIdx][1] = (val>>16) & 0xff;
      sei.digest[yuvIdx][2] = (val>>8)  & 0xff;
      sei.digest[yuvIdx][3] =  val      & 0xff;
    }
  }
}

//! \}
