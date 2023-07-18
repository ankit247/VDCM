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

#ifndef __SSM_H__
#define __SSM_H__

#include "TypeDef.h"
#include "Bitstream.h"
#include "BitFifo.h"
#include "SyntaxElement.h"
#include <vector>
#include <queue>

class SubStreamMux
{
private:
    Bool m_isEncoder;                                           // (enc/dec) flag to denote whether encoder or decoder is running
    UInt m_numSubstreams;                                       // (enc/dec) number of substreams
    UInt m_muxWordSize;                                         // (enc/dec) mux word size
    UInt m_encBalanceFifoLen;                                   // (enc/dec) size of balance FIFO
    UInt m_decFunnelShifterLen;                                 // (enc/dec) size of funnel shifter
    UInt m_seMaxSize;                                           // (enc/dec) max syntax element size
    UInt m_seMinSize;                                           // (enc/dec) min syntax element size
    UInt m_blockIdx;                                            // (enc/dec) block index within slice (decBlockIdx = encBlockIdx + blockDelay)
    Bool m_isEncodingDone;                                      // (enc) flag for encoding complete (all SEs in balance FIFOs)
    BitstreamInput* m_inputBitstream;
    BitstreamOutput* m_outputBitstream;
    std::vector<BitFifo*> m_encBalanceFifos;
    std::vector<BitFifo*> m_decFunnelShifters;
    std::vector<UInt> m_decFunnelShifterFullness;               // (enc) tracks dec funnel shifter state
    std::vector<std::deque<SyntaxElement*> > m_syntaxElements;   // (enc) FIFO to store SEs for each block
    std::vector<UInt> m_ssmSkew;
    std::vector<UInt> m_ssmDelay;
    std::vector<Bool> m_isEncodingStarted;
    std::vector<Bool> m_isEncodingFinished;
    std::vector<Bool> m_isDecodingFinished;

public:
    //SubStreamMux (Bool isEncoder, UInt numSubstreams, UInt mwSize, UInt seMaxSize, UInt encBalanceFifoLen);
    SubStreamMux (Bool isEncoder, UInt numSubstreams, UInt seMaxSize);
    ~SubStreamMux ();

    // methods
    Void GenerateNewSyntaxElements ();                              // (enc) add a new blank SE to the end of the queue
    Void IncrementBlockIdx () { m_blockIdx++; }                     // (enc/dec) update the block index
    Void DecodeBlock ();                                            // (dec) request muxwords if needed to decode current block
    Void EncodeBlock ();                                            // (enc) encode current block and update simulated decoder model
    Int ReadFromFunnelShifter (UInt ssmId, UInt nBits);             // (dec) parse nBits from substream ssmIdx
    UInt GetDecFunnelShifterFullness (UInt ssmIdx);                 // (dec) get fullness of selected funnel shifter
    Bool IsDecReqMuxWord (UInt ssmIdx);                             // check if decoder will require mux words
    Void GenerateMuxWord (UInt ssmIdx);                             // generate mux word from substream balance FIFO
    Void DecodeSubStreamBits (UInt ssmIdx);                         // (enc) update simulated decoder model
    Void SetConfigTemp ();                                          // (enc/dec) load initial config (later replace with PPS)
    Void AddCurSyntaxElementsToFifo (UInt ssmIdx);
    Void AddPrevSyntaxElementsToFifo (UInt ssmIdx);
    Void MoveFifoBits (BitFifo* src, BitFifo* dst, UInt numBits);

    Void SetIsEncodingFinished (UInt ssmIdx, Bool x) { m_isEncodingFinished.at (ssmIdx) = x; }
    Void SetIsDecodingFinished (UInt ssmIdx, Bool x) { m_isDecodingFinished.at (ssmIdx) = x; }
    Void RemoveOldestSyntaxElement (UInt ssmIdx);

    // getters
    UInt GetNumSubStreams () { return m_numSubstreams; }
    UInt GetMuxWordSize () { return m_muxWordSize; }
    UInt GetMaxSeSize () { return m_seMaxSize; }
    SyntaxElement* GetCurSyntaxElement (UInt ss) { return m_syntaxElements.at (ss).back (); }
    SyntaxElement* GetOldestSyntaxElement (UInt ss) { return m_syntaxElements.at (ss).front (); }
    SyntaxElement* GetPrevSyntaxElement (UInt ss) {
        UInt idx = (UInt)m_syntaxElements.at(ss).size () - 2;
        return m_syntaxElements.at (ss).at (idx);
    }
    UInt GetNumStoredSyntaxElement () {
        return (UInt)std::max (m_syntaxElements.at (0).size (), m_syntaxElements.at (1).size ());
    }

    // setters
    Void SetNewBitsIn (BitstreamInput* in) { m_inputBitstream = in; }
    Void SetNewBitsOut (BitstreamOutput* out) { m_outputBitstream = out; }
    Void SetCurSyntaxElementMode (std::string mode);
};

#endif