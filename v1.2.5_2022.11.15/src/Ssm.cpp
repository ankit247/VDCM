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

#include "Ssm.h"
#include <assert.h>

/**
    Constructor

    @param isEncoder flag to indicate encoder or decoder
    @param numSubstreams number of substreams
    @param mwSize mux word size
    @param seMaxSize max syntax element size
    @param encBalanceFifoLen encoder balance FIFO size
*/
SubStreamMux::SubStreamMux (Bool isEncoder, UInt numSubstreams, UInt seMaxSize)
    : m_numSubstreams (numSubstreams)
    , m_isEncoder (isEncoder)
    , m_encBalanceFifos (numSubstreams, NULL)
    , m_decFunnelShifters (numSubstreams, NULL)
    , m_decFunnelShifterFullness (numSubstreams, 0)
    , m_blockIdx (0)
    , m_ssmDelay (numSubstreams, 0)
    , m_ssmSkew (numSubstreams, 0)
    , m_isEncodingStarted (numSubstreams, false)
    , m_isEncodingFinished (numSubstreams, false)
    , m_isDecodingFinished (numSubstreams, false)
    , m_isEncodingDone (false)
    , m_inputBitstream (NULL)
    , m_muxWordSize (seMaxSize)
    , m_seMaxSize (seMaxSize)
{
    if (m_isEncoder) {
        m_encBalanceFifoLen = m_seMaxSize * m_seMaxSize;
    }
    SetConfigTemp ();
    // setup SE storage, balance FIFOs, funnel shifters
    for (UInt ssmIdx = 0; ssmIdx < m_numSubstreams; ssmIdx++) {
        if (m_isEncoder) {
            // encoder has one balance FIFO and once syntax FIFO per substream
            m_encBalanceFifos.at (ssmIdx) = new BitFifo ((m_encBalanceFifoLen + 7) >> 3);
            m_syntaxElements.push_back (std::deque<SyntaxElement*> ());
        }
        else {
            // decoder has one funnel shifter per substream
            m_decFunnelShifters.at (ssmIdx) = new BitFifo ((m_decFunnelShifterLen + 7) >> 3);
        }
    }
}

/**
    Destructor
*/
SubStreamMux::~SubStreamMux () {
    if (m_isEncoder) {
        for (UInt ssmIdx = 0; ssmIdx < m_numSubstreams; ssmIdx++) {
            m_encBalanceFifos.at (ssmIdx)->Clear ();
            delete m_encBalanceFifos.at (ssmIdx);
        }
        if (m_syntaxElements.size () > 0) {
            m_syntaxElements.clear ();
        }
    }
    else {
        for (UInt ssmIdx = 0; ssmIdx < m_numSubstreams; ssmIdx++) {
            m_decFunnelShifters.at (ssmIdx)->Clear ();
            delete m_decFunnelShifters.at (ssmIdx);
        }
    }
}

/**
    Move bits from one FIFO to another

    @param src source FIFO
    @param dst destination FIFO
    @param numBits number of bits to move
*/
Void SubStreamMux::MoveFifoBits (BitFifo* src, BitFifo* dst, UInt numBits) {
    UInt remBits = numBits;
    while (remBits > 8) {
        dst->PutBits (src->GetBits (8), 8);
        remBits -= 8;
    }
    if (remBits > 0) {
        dst->PutBits (src->GetBits (remBits), remBits);
    }
}

/**
    Setup the substream multiplexer
*/
Void SubStreamMux::SetConfigTemp () {
    m_seMinSize = 2;
    m_decFunnelShifterLen = (m_muxWordSize + m_seMaxSize - 1);
    UInt ssmDelayBase = m_seMaxSize;
    if (ssmDelayBase < 2) {
        fprintf (stderr, "SubStreamMux::SetConfigTemp: ssm delay is too short: %d.\n", ssmDelayBase);
        exit (EXIT_FAILURE);
    }
    m_ssmSkew[0] = 0;
    m_ssmSkew[1] = 1;
    m_ssmSkew[2] = 1;
    m_ssmSkew[3] = 1;
    for (UInt ss = 0; ss < m_numSubstreams; ss++) {
        m_ssmDelay[ss] = (ssmDelayBase - 1) + m_ssmSkew[ss];
    }
}

/**
    Generate storage for new syntax elements and place into SE buffer
*/
Void SubStreamMux::GenerateNewSyntaxElements () {
    for (UInt ssmIdx = 0; ssmIdx < m_numSubstreams; ssmIdx++) {
        SyntaxElement* se = new SyntaxElement (ssmIdx, m_blockIdx, m_seMaxSize, std::string ("undefined"));
        m_syntaxElements.at (ssmIdx).push_back (se);
    }
}

/**
    Read bits from selected funnel shifter

    @param ssmId substream index
    @param nBits number of bits to read
*/
Int SubStreamMux::ReadFromFunnelShifter (UInt ssmId, UInt nBits) {
    Int sw = m_decFunnelShifters.at (ssmId)->GetBits (nBits);
    return sw;
}

/**
    Set a name string for the current syntax element

    @param mode name string
*/
Void SubStreamMux::SetCurSyntaxElementMode (std::string mode) {
    for (UInt ssmIdx = 0; ssmIdx < m_numSubstreams; ssmIdx++) {
        GetCurSyntaxElement (ssmIdx)->SetModeString (mode);
    }
}

/**
    Get muxwords for current block-time    
*/
Void SubStreamMux::DecodeBlock () {
    assert (!m_isEncoder);
    for (UInt ssmIdx = 0; ssmIdx < m_numSubstreams; ssmIdx++) {
        if (m_blockIdx >= m_ssmSkew.at (ssmIdx) && !m_isDecodingFinished.at (ssmIdx)) {
            if (m_decFunnelShifters.at (ssmIdx)->GetFullness () < m_seMaxSize) {
                for (UInt j = 0; j < (m_muxWordSize >> 3); j++) {
                    unsigned char d = m_inputBitstream->Read (8);
                    m_decFunnelShifters.at (ssmIdx)->PutBits (d, 8);
                }
            }
        }
    }
}

/**
    Check if demultiplexer needs a muxword for this substream

    @param ssmIdx substream index
*/
Bool SubStreamMux::IsDecReqMuxWord (UInt ssmIdx) {
    assert (m_isEncoder);
    Bool decReqMuxWord = false;
    if (m_decFunnelShifterFullness.at (ssmIdx) < m_seMaxSize) {
        decReqMuxWord = true;
    }
    return decReqMuxWord;
}

/**
    Generate muxword and insert into bitstream

    @param ssmIdx substream index
*/
Void SubStreamMux::GenerateMuxWord (UInt ssmIdx) {
    assert (m_isEncoder);
    UInt remBits = m_muxWordSize;
    UInt totalBytes = (remBits + 7) >> 3;
    for (UInt j = 0; j < totalBytes; j++) {
        // for last block in slice
        Int sz = (Int)m_encBalanceFifos.at (ssmIdx)->GetFullness ();
        Int word;
        if (sz >= 8) {
            word = m_encBalanceFifos.at (ssmIdx)->GetBits (8);
        }
        else if (sz > 0) {
            word = m_encBalanceFifos.at (ssmIdx)->GetBits (sz) << (8 - sz);
        }
        else {
            word = 0;
        }
        m_outputBitstream->Write (word, 8);
        m_decFunnelShifterFullness.at (ssmIdx) += 8;
    }
}

/**
    Update demultiplexer model

    @param ssmIdx substream index
*/
Void SubStreamMux::DecodeSubStreamBits (UInt ssmIdx) {
    assert (m_isEncoder);
    m_decFunnelShifterFullness.at (ssmIdx) -= GetOldestSyntaxElement (ssmIdx)->GetNumBits ();
    RemoveOldestSyntaxElement (ssmIdx);
}

/**
    Encode current block and update demultiplexer model
*/
Void SubStreamMux::EncodeBlock () {
    assert (m_isEncoder);
    // for each substream, copy SE to encoder balance fifo
    for (UInt ssmIdx = 0; ssmIdx < m_numSubstreams; ssmIdx++) {
        if (!m_isEncodingFinished.at (ssmIdx)) {
            if (m_blockIdx >= m_ssmSkew.at (ssmIdx)) {
                if (ssmIdx == 0) {
                    AddCurSyntaxElementsToFifo (ssmIdx);
                }
                else {
                    if (m_isEncodingFinished.at (0)) {
                        AddCurSyntaxElementsToFifo (ssmIdx);
                    }
                    else {
                        AddPrevSyntaxElementsToFifo (ssmIdx);
                    }
                }
            }
        }
    }

    // generate muxwords if necessary and update decoder model
    for (UInt ssmIdx = 0; ssmIdx < m_numSubstreams; ssmIdx++) {
        if (m_blockIdx >= m_ssmDelay.at (ssmIdx) && m_syntaxElements.at (ssmIdx).size () > 0) {
            if (IsDecReqMuxWord (ssmIdx)) {
				GenerateMuxWord (ssmIdx);
            }
            DecodeSubStreamBits (ssmIdx); // update demultiplexer model
        }
    }
    m_blockIdx++;
}

/**
    Get fullness of selected funnel shifter

    @param ssmIdx substream index
    @return fullness of specified funnel shifter
*/
UInt SubStreamMux::GetDecFunnelShifterFullness (UInt ssmIdx) {
    assert (!m_isEncoder);
    return m_decFunnelShifters.at (ssmIdx)->GetFullness ();
}

/**
    Remove oldest syntax element from syntax FIFO

    @param ssmIdx substream index
*/
Void SubStreamMux::RemoveOldestSyntaxElement (UInt ssmIdx) {
    delete m_syntaxElements.at (ssmIdx).front ();
    m_syntaxElements.at (ssmIdx).pop_front ();
}

/**
    Move bits for previous syntax element into SE buffer

    @param ssmIdx substream index
*/
Void SubStreamMux::AddPrevSyntaxElementsToFifo (UInt ssmIdx) {
    BitFifo* src = GetPrevSyntaxElement (ssmIdx)->GetFifo ();
    BitFifo* dst = m_encBalanceFifos.at (ssmIdx);
    MoveFifoBits (src, dst, GetPrevSyntaxElement (ssmIdx)->GetNumBits ());

    //! sanity check !//
    if (m_encBalanceFifos.at (ssmIdx)->GetFullness () > m_encBalanceFifoLen) {
        fprintf (stderr, "AddPrevSyntaxElementsToFifo() funnel shifter overflow.\n");
        exit (EXIT_FAILURE);
    }
}

/**
    Move bits for current syntax element into SE buffer

    @param ssmIdx substream index
*/
Void SubStreamMux::AddCurSyntaxElementsToFifo (UInt ssmIdx) {
    BitFifo* src = GetCurSyntaxElement (ssmIdx)->GetFifo ();
    BitFifo* dst = m_encBalanceFifos.at (ssmIdx);
    MoveFifoBits (src, dst, GetCurSyntaxElement (ssmIdx)->GetNumBits ());
    
    //! sanity check !//
    if (m_encBalanceFifos.at (ssmIdx)->GetFullness () > m_encBalanceFifoLen) {
        fprintf (stderr, "AddCurSyntaxElementToFifos(): funnel shifter overflow.\n");
        exit (EXIT_FAILURE);
    }
}
