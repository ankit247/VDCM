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
#ifndef __SYNTAX_ELEMENT_H__
#define __SYNTAX_ELEMENT_H__

#include "TypeDef.h"
#include <string>
#include "BitFifo.h"

class SyntaxElement {
private:
    UInt m_substream;   // which substream does this SE belong to
    UInt m_blockIdx;    // block idx within slice
    UInt m_seMaxSize;   // max allowable SE size
    UInt m_numBits;     // SE total length
    UInt m_component;   // ignore for now, since one SE might have data from multiple components
    BitFifo* m_data;    // storage for current SE
    std::string m_mode; // mode name

public:
    SyntaxElement (UInt ssmIdx, UInt blockIdx, UInt maxSize, std::string const& mode);
    ~SyntaxElement ();

    //! methods !//
    Void AddSyntaxWord (UInt data, UInt nBits); // add bits to syntax element

    //! getters !//
    BitFifo* GetFifo () { return m_data; }
    UInt GetNumBits () { return m_numBits; }

    //! setters !//
    Void SetModeString (std::string mode) { m_mode = mode; }
};

#endif // __SYNTAX_ELEMENT_H__