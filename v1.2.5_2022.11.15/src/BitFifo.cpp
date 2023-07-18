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

#include "BitFifo.h"

/**
    Clear fifo
    */
Void BitFifo::Clear () {
    m_curBitWriteIdx = 0;
    m_curBitReadIdx = 0;
    m_fullness = 0;
}

/**
    Insert bits into fifo

    @param data data to insert into fifo
    @param numBits number of bits
    */
Void BitFifo::PutBits (UInt data, UInt numBits) {
    //! check for fifo overflow !//
    if (numBits + m_fullness > m_maxFullness) {
        fprintf (stderr, "BitFifo::PutBits(): overflow condition (%d + %d > %d).\n", m_fullness, numBits, m_maxFullness);
        exit (EXIT_FAILURE);
    }

    //! place bits into fifo !//
    if (m_data.empty ()) {
        m_data.push (0);
    }

    UInt remBits = numBits;
    while (remBits > 0) {
        m_data.back () |= (((UChar)((data >> (remBits - 1)) & 0x1)) << (7 - m_curBitWriteIdx));
        m_curBitWriteIdx += 1;
        m_fullness += 1;
        if (m_curBitWriteIdx == 8) {
            m_curBitWriteIdx = 0;
            m_data.push (0);
        }
        remBits -= 1;
    }
}

/**
    Read bits into fifo

    @param numBits number of bits
    @return data
    */
UInt BitFifo::GetBits (UInt numBits) {
    //! check for fifo underflow !//
    if (numBits > m_fullness) {
        fprintf (stderr, "BitFifo::GetBits(): underflow condition (%d - %d < 0).\n", m_fullness, numBits);
        exit (EXIT_FAILURE);
    }

    //! read bits from fifo !//
    UInt data = 0;
    UInt remBits = numBits;
    while (remBits > 0) {
        data |= (((m_data.front () >> (7 - m_curBitReadIdx)) & 0x1) << (remBits - 1));
        m_curBitReadIdx += 1;
        m_fullness -= 1;
        if (m_curBitReadIdx == 8) {
            m_curBitReadIdx = 0;
            m_data.pop ();
        }
        remBits -= 1;
    }
    return data;
}