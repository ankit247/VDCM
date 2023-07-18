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

/** \file       Bitstream.cpp
\brief      Bitstream handling
*/

#include "Bitstream.h"
#include <fstream>

/**
    BitstreamInput default constructor
    */
BitstreamInput::BitstreamInput ()
    : m_fifoIdx (0)
    , m_heldBits (0)
    , m_numHeldBits (0)
    , m_numBitsRead (0) {
    m_fifo = new std::vector<uint8_t>;
    Clear ();
}

/**
    BitstreamInput constructor

    @param buf byte-buffer to use as input bitstream
    */
BitstreamInput::BitstreamInput (std::vector<uint8_t>* buf)
    : m_fifo (buf)
    , m_fifoIdx (0)
    , m_heldBits (0)
    , m_numHeldBits (0)
    , m_numBitsRead (0) {
}

Void BitstreamInput::BytesFromStream (std::fstream* in, UInt numBytes) {
    Char temp;
    for (UInt idx = 0; idx < numBytes; idx++) {
        in->read (&temp, 1);
        m_fifo->push_back (temp);
    }
}

/**
    Read from bitstream

    @param uiNumberOfBits number of bits to read
    @param ruiBits output of read operation
    */
Void BitstreamInput::Read (UInt uiNumberOfBits, UInt& ruiBits) {
    if (uiNumberOfBits > 32) {
        fprintf (stderr, "BitstreamInput::read(): max read length of 32 bits per call.\n");
        exit (EXIT_FAILURE);
    }

    m_numBitsRead += uiNumberOfBits;

    /* NB, bits are extracted from the MSB of each byte. */
    UInt retval = 0;
    if (uiNumberOfBits <= m_numHeldBits) {
        /* n=1, len(H)=7:   -VHH HHHH, shift_down=6, mask=0xfe
        * n=3, len(H)=7:   -VVV HHHH, shift_down=4, mask=0xf8
        */
        retval = m_heldBits >> (m_numHeldBits - uiNumberOfBits);
        retval &= ~(0xff << uiNumberOfBits);
        m_numHeldBits -= uiNumberOfBits;
        ruiBits = retval;
        return;
    }

    /* all num_held_bits will go into retval
    *   => need to mask leftover bits from previous extractions
    *   => align retval with top of extracted word */
    /* n=5, len(H)=3: ---- -VVV, mask=0x07, shift_up=5-3=2,
    * n=9, len(H)=3: ---- -VVV, mask=0x07, shift_up=9-3=6 */
    uiNumberOfBits -= m_numHeldBits;
    retval = m_heldBits & ~(0xff << m_numHeldBits);
    retval <<= uiNumberOfBits;

    /* number of whole bytes that need to be loaded to form retval */
    /* n=32, len(H)=0, load 4bytes, shift_down=0
    * n=32, len(H)=1, load 4bytes, shift_down=1
    * n=31, len(H)=1, load 4bytes, shift_down=1+1
    * n=8,  len(H)=0, load 1byte,  shift_down=0
    * n=8,  len(H)=3, load 1byte,  shift_down=3
    * n=5,  len(H)=1, load 1byte,  shift_down=1+3
    */
    UInt aligned_word = 0;
    UInt num_bytes_to_load = (uiNumberOfBits - 1) >> 3;
    if (m_fifoIdx + num_bytes_to_load >= m_fifo->size ()) {
        fprintf (stderr, "BitstreamInput::read(): FIFO underflow.\n");
        exit (EXIT_FAILURE);
    }

    switch (num_bytes_to_load) {
    case 3: aligned_word = (*m_fifo)[m_fifoIdx++] << 24;
    case 2: aligned_word |= (*m_fifo)[m_fifoIdx++] << 16;
    case 1: aligned_word |= (*m_fifo)[m_fifoIdx++] << 8;
    case 0: aligned_word |= (*m_fifo)[m_fifoIdx++];
    }

    /* resolve remainder bits */
    UInt next_num_held_bits = (32 - uiNumberOfBits) % 8;

    /* copy required part of aligned_word into retval */
    retval |= aligned_word >> next_num_held_bits;

    /* store held bits */
    m_numHeldBits = next_num_held_bits;
    m_heldBits = (UChar)aligned_word;

    ruiBits = retval;
}

/**
    BitstreamOutput constructor
    */
BitstreamOutput::BitstreamOutput () {
    m_fifo = new std::vector<uint8_t>;
    Clear ();
}

/**
    Get access to bitstream data

    @return front of bitstream buffer
    */
Char* BitstreamOutput::GetByteStream () const {
    return (Char*)&m_fifo->front ();
}

/**
Get access to bitstream data

@return front of bitstream buffer
*/
Char* BitstreamInput::GetByteStream () const {
    return (Char*)&m_fifo->front ();
}

/**
    Get length of bitstream (in bytes)

    @return length of bitstream
    */
UInt BitstreamOutput::GetByteStreamLength () {
    return UInt (m_fifo->size ());
}

/**
    Clear FIFO and reset bits held
    */
Void BitstreamOutput::Clear () {
    m_fifo->clear ();
    m_heldBits = 0;
    m_numHeldBits = 0;
}

/**
    Clear FIFO and reset bits held
    */
Void BitstreamInput::Clear () {
    m_fifo->clear ();
    m_fifoIdx = 0;
    m_numBitsRead = 0;
    m_heldBits = 0;
    m_numHeldBits = 0;
}

/**
    Write to bitstream, align to nearest byte
    */
Void BitstreamOutput::WriteByteAlignment () {
    Write (1, 1);
    WriteAlignZero ();
}

/**
    Write held bits to bitstream
    */
Void BitstreamOutput::WriteAlignZero () {
    if (0 == m_numHeldBits) {
        return;
    }
    m_fifo->push_back (m_heldBits);
    m_heldBits = 0;
    m_numHeldBits = 0;
}

/**
    Write to bitstream

    @param uiBits data to be written to bitstream
    @param uiNumberOfBits number of bits to write
    */
Void BitstreamOutput::Write (UInt uiBits, UInt uiNumberOfBits) {
    if (uiNumberOfBits > 32) {
        fprintf (stderr, "BitstreamOutput::write(): exceed max write of 32 bits.\n");
        exit (EXIT_FAILURE);
    }

    /* any modulo 8 remainder of num_total_bits cannot be written this time,
    * and will be held until next time. */
    UInt num_total_bits = uiNumberOfBits + m_numHeldBits;
    UInt next_num_held_bits = num_total_bits % 8;

    /* form a byte aligned word (write_bits), by concatenating any held bits
    * with the new bits, discarding the bits that will form the next_held_bits.
    * eg: H = held bits, V = n new bits        /---- next_held_bits
    * len(H)=7, len(V)=1: ... ---- HHHH HHHV . 0000 0000, next_num_held_bits=0
    * len(H)=7, len(V)=2: ... ---- HHHH HHHV . V000 0000, next_num_held_bits=1
    * if total_bits < 8, the value of v_ is not used */
    UChar next_held_bits = (UChar)(uiBits << (8 - next_num_held_bits));

    if (!(num_total_bits >> 3)) {
        /* insufficient bits accumulated to write out, append new_held_bits to
        * current held_bits */
        /* NB, this requires that v only contains 0 in bit positions {31..n} */
        m_heldBits |= next_held_bits;
        m_numHeldBits = next_num_held_bits;
        return;
    }

    /* topword serves to justify held_bits to align with the msb of uiBits */
    UInt topword = (uiNumberOfBits - next_num_held_bits) & ~((1 << 3) - 1);
    UInt write_bits = (m_heldBits << topword) | (uiBits >> next_num_held_bits);

    switch (num_total_bits >> 3) {
    case 4:
        m_fifo->push_back ((unsigned char)(write_bits >> 24));
    case 3:
        m_fifo->push_back ((unsigned char)(write_bits >> 16));
    case 2:
        m_fifo->push_back ((unsigned char)(write_bits >> 8));
    case 1:
        m_fifo->push_back ((unsigned char)write_bits);
    }

    m_heldBits = next_held_bits;
    m_numHeldBits = next_num_held_bits;
}

/**
    Copy data into another BitstreamOutput object
    */
Void BitstreamOutput::CopyBufferOut (BitstreamOutput* outBuffer) {
    if (m_numHeldBits != 0) {
        fprintf (stderr, "BitstreamOutput::CopyBufferOut(): must be byte-aligned.\n");
        exit (EXIT_FAILURE);
    }
    for (UInt byte = 0; byte < m_fifo->size (); byte++) {
        outBuffer->Write (m_fifo->at (byte), 8);
        
    }
}