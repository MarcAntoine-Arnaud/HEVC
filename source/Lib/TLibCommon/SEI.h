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

#pragma once

//! \ingroup TLibCommon
//! \{
#if BUFFERING_PERIOD_AND_TIMING_SEI
class TComSPS;
#endif

/**
 * Abstract class representing an SEI message with lightweight RTTI.
 */
class SEI
{
public:
  enum PayloadType
  {
#if BUFFERING_PERIOD_AND_TIMING_SEI
    BUFFERING_PERIOD       = 0,
    PICTURE_TIMING         = 1,
#endif
    USER_DATA_UNREGISTERED = 5,
#if RECOVERY_POINT_SEI
    RECOVERY_POINT         = 6,
#endif
#if ACTIVE_PARAMETER_SETS_SEI_MESSAGE 
    ACTIVE_PARAMETER_SETS = 131, 
#endif 
    DECODED_PICTURE_HASH   = 256,
  };
  
  SEI() {}
  virtual ~SEI() {}
  
  virtual PayloadType payloadType() const = 0;
};

class SEIuserDataUnregistered : public SEI
{
public:
  PayloadType payloadType() const { return USER_DATA_UNREGISTERED; }

  SEIuserDataUnregistered()
    : userData(0)
    {}

  virtual ~SEIuserDataUnregistered()
  {
    delete userData;
  }

  unsigned char uuid_iso_iec_11578[16];
  unsigned userDataLength;
  unsigned char *userData;
};

class SEIDecodedPictureHash : public SEI
{
public:
  PayloadType payloadType() const { return DECODED_PICTURE_HASH; }

  SEIDecodedPictureHash() {}
  virtual ~SEIDecodedPictureHash() {}
  
  enum Method
  {
    MD5,
    CRC,
    CHECKSUM,
    RESERVED,
  } method;

  unsigned char digest[3][16];
};

#if ACTIVE_PARAMETER_SETS_SEI_MESSAGE  
class SEIActiveParameterSets : public SEI 
{
public:
  PayloadType payloadType() const { return ACTIVE_PARAMETER_SETS; }

  SEIActiveParameterSets() 
    :activeSPSIdPresentFlag(1)
    ,activeParamSetSEIExtensionFlag(0)
  {}
  virtual ~SEIActiveParameterSets() {}

  Int activeVPSId; 
  Int activeSPSIdPresentFlag;
  Int activeSeqParamSetId; 
  Int activeParamSetSEIExtensionFlag; 
};
#endif 

#if BUFFERING_PERIOD_AND_TIMING_SEI
class SEIBufferingPeriod : public SEI
{
public:
  PayloadType payloadType() const { return BUFFERING_PERIOD; }

  SEIBufferingPeriod()
  :m_sps (NULL)
  {}
  virtual ~SEIBufferingPeriod() {}

  UInt m_seqParameterSetId;
  Bool m_altCpbParamsPresentFlag;
  UInt m_initialCpbRemovalDelay         [MAX_CPB_CNT][2];
  UInt m_initialCpbRemovalDelayOffset   [MAX_CPB_CNT][2];
  UInt m_initialAltCpbRemovalDelay      [MAX_CPB_CNT][2];
  UInt m_initialAltCpbRemovalDelayOffset[MAX_CPB_CNT][2];
  TComSPS* m_sps;
};
class SEIPictureTiming : public SEI
{
public:
  PayloadType payloadType() const { return PICTURE_TIMING; }

  SEIPictureTiming()
  : m_numNalusInDuMinus1      (NULL)
  , m_duCpbRemovalDelayMinus1 (NULL)
  , m_sps                     (NULL)
  {}
  virtual ~SEIPictureTiming()
  {
    if( m_numNalusInDuMinus1 != NULL )
    {
      delete m_numNalusInDuMinus1;
    }
    if( m_duCpbRemovalDelayMinus1  != NULL )
    {
      delete m_duCpbRemovalDelayMinus1;
    }
  }

  UInt  m_auCpbRemovalDelay;
  UInt  m_picDpbOutputDelay;
  UInt  m_numDecodingUnitsMinus1;
  Bool  m_duCommonCpbRemovalDelayFlag;
  UInt  m_duCommonCpbRemovalDelayMinus1;
  UInt* m_numNalusInDuMinus1;
  UInt* m_duCpbRemovalDelayMinus1;
  TComSPS* m_sps;
};
#endif
#if RECOVERY_POINT_SEI
class SEIRecoveryPoint : public SEI
{
public:
  PayloadType payloadType() const { return RECOVERY_POINT; }

  SEIRecoveryPoint() {}
  virtual ~SEIRecoveryPoint() {}

  Int  m_recoveryPocCnt;
  Bool m_exactMatchingFlag;
  Bool m_brokenLinkFlag;
};
#endif
/**
 * A structure to collate all SEI messages.  This ought to be replaced
 * with a list of std::list<SEI*>.  However, since there is only one
 * user of the SEI framework, this will do initially */
class SEImessages
{
public:
  SEImessages()
    : user_data_unregistered(0)
#if ACTIVE_PARAMETER_SETS_SEI_MESSAGE  
    , active_parameter_sets(0)
#endif 
    , picture_digest(0)
#if BUFFERING_PERIOD_AND_TIMING_SEI
    , buffering_period(0)
    , picture_timing(0)
#endif
#if RECOVERY_POINT_SEI
    , recovery_point(0)
#endif
    {}

  ~SEImessages()
  {
    delete user_data_unregistered;
#if ACTIVE_PARAMETER_SETS_SEI_MESSAGE  
    delete active_parameter_sets; 
#endif 
    delete picture_digest;
#if BUFFERING_PERIOD_AND_TIMING_SEI
    delete buffering_period;
    delete picture_timing;
#endif
#if RECOVERY_POINT_SEI
    delete recovery_point;
#endif
  }

  SEIuserDataUnregistered* user_data_unregistered;
#if ACTIVE_PARAMETER_SETS_SEI_MESSAGE  
  SEIActiveParameterSets* active_parameter_sets; 
#endif 
  SEIDecodedPictureHash* picture_digest;
#if BUFFERING_PERIOD_AND_TIMING_SEI
  SEIBufferingPeriod* buffering_period;
  SEIPictureTiming* picture_timing;
  TComSPS* m_pSPS;
#endif
#if RECOVERY_POINT_SEI
  SEIRecoveryPoint* recovery_point;
#endif
};

//! \}
