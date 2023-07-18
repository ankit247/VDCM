/* Copyright (c) 2015-2018 Qualcomm Technologies, Inc. All Rights Reserved
 * 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The above license is used as a license under copyright only. Please reference VESA Policy #200D for patent licensing terms.
 */

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

/** \file       Bitstream.h
\brief      Bitstream handling
*/

#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <vector>
#include <stdint.h>
#include "TypeDef.h"

class BitstreamInput {
protected:
    std::vector<uint8_t> *m_fifo;   // FIFO for storage of complete bytes
    UInt m_fifoIdx;                 // FIFO read index
    UInt m_numHeldBits;             // number of bits held for current byte
    UChar m_heldBits;               // value of bits helf for current byte
    UInt m_numBitsRead;             // number of bits read from bitstream

public:
    BitstreamInput ();
    BitstreamInput (std::vector<uint8_t>* buf);
    ~BitstreamInput () { delete m_fifo; };
    Char* GetByteStream () const;
    Void Read (UInt uiNumberOfBits, UInt& ruiBits);
    UInt Read (UInt numberOfBits) { UInt tmp; Read (numberOfBits, tmp); return tmp; }
    Void BytesFromStream (std::fstream* in, UInt numBytes);
    Void DeleteFifo ();
    Void Clear ();
};

class BitstreamOutput {
protected:
    std::vector<uint8_t> *m_fifo;   // FIFO for storage of complete bytes
    UInt m_numHeldBits;             // number of bits not flushed to bytestream.
    UChar m_heldBits;               // the bits held and not flushed to bytestream.
                                    // this value is always msb-aligned, bigendian.

public:
    BitstreamOutput ();
    ~BitstreamOutput () { delete m_fifo; };

    // methods
    Int GetNumBitsUntilByteAligned () { return (8 - m_numHeldBits) & 0x7; }
    Char* GetByteStream () const;
    UInt GetByteStreamLength ();
    Void Clear ();
    Void WriteAlignZero ();
    Void WriteByteAlignment ();
    Void Write (UInt uiBits, UInt uiNumberOfBits);
    Void CopyBufferOut (BitstreamOutput* outBuffer);
};

#endif // __BITSTREAM_H__