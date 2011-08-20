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

#include <vector>
#include <algorithm>
#include <ostream>

#include "../TLibCommon/NAL.h"
#include "../TLibCommon/TComBitStream.h"
#include "NALwrite.h"

using namespace std;

static const char emulation_prevention_three_byte[] = {3};

/**
 * write @nalu@ to bytestream @out@, performing RBSP anti startcode
 * emulation as required.  @nalu@.m_RBSPayload must be byte aligned.
 */
void write(ostream& out, const OutputNALUnit& nalu)
{
  TComOutputBitstream bsNALUHeader;

  bsNALUHeader.write(0,1); // forbidden_zero_flag
  bsNALUHeader.write(nalu.m_RefIDC, 2); // nal_ref_idc
  bsNALUHeader.write(nalu.m_UnitType, 5); // nal_unit_type

  switch (nalu.m_UnitType)
  {
  case NAL_UNIT_CODED_SLICE:
  case NAL_UNIT_CODED_SLICE_IDR:
  case NAL_UNIT_CODED_SLICE_CDR:
    bsNALUHeader.write(nalu.m_TemporalID, 3); // temporal_id
    bsNALUHeader.write(nalu.m_OutputFlag, 1); // output_flag
    bsNALUHeader.write(1, 4); // reserved_one_4bits
    break;
  default: break;
  }

  out.write(bsNALUHeader.getByteStream(), bsNALUHeader.getByteStreamLength());

  /* write out rsbp_byte's, inserting any required
   * emulation_prevention_three_byte's */
  /* 7.4.1 ...
   * emulation_prevention_three_byte is a byte equal to 0x03. When an
   * emulation_prevention_three_byte is present in the NAL unit, it shall be
   * discarded by the decoding process.
   * The last byte of the NAL unit shall not be equal to 0x00.
   * Within the NAL unit, the following three-byte sequences shall not occur at
   * any byte-aligned position:
   *  - 0x000000
   *  - 0x000001
   *  - 0x000002
   * Within the NAL unit, any four-byte sequence that starts with 0x000003
   * other than the following sequences shall not occur at any byte-aligned
   * position:
   *  - 0x00000300
   *  - 0x00000301
   *  - 0x00000302
   *  - 0x00000303
   */
  const vector<uint8_t>& rbsp = nalu.m_Bitstream.getFIFO();

  for (vector<uint8_t>::const_iterator it = rbsp.begin(); it != rbsp.end();)
  {
    /* 1) find the next emulated 00 00 {00,01,02,03}
     * 2a) if not found, write all remaining bytes out, stop.
     * 2b) otherwise, write all non-emulated bytes out
     * 3) insert emulation_prevention_three_byte
     */
    vector<uint8_t>::const_iterator found = it;
    do
    {
      /* NB, end()-1, prevents finding a trailing two byte sequence */
      found = search_n(found, rbsp.end()-1, 2, 0);
      found++;
      /* if not found, found == end, otherwise found = second zero byte */
      if (found == rbsp.end())
        break;
      if (*(++found) <= 3)
        break;
    } while (true);

    /* found is the first byte that shouldn't be written out this time */
    out.write((char*)&*it, found - it);
    it = found;
    if (found != rbsp.end())
    {
      out.write(emulation_prevention_three_byte, 1);
    }
  }

  /* 7.4.1.1
   * ... when the last byte of the RBSP data is equal to 0x00 (which can
   * only occur when the RBSP ends in a cabac_zero_word), a final byte equal
   * to 0x03 is appended to the end of the data.
   */
  if (rbsp.back() == 0x00)
  {
    out.write(emulation_prevention_three_byte, 1);
  }
}

/**
 * Write rbsp_trailing_bits to @bs@ causing it to become byte-aligned
 */
void writeRBSPTrailingBits(TComOutputBitstream& bs)
{
  bs.write( 1, 1 );
  bs.writeAlignZero();
}
