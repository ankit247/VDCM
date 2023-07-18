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

/**
\file       MppfMode.h
\brief      Midpoint Prediction Fallback Mode (header)
*/

#ifndef __MPPF_MODE__
#define __MPPF_MODE__

#include "Mode.h"
#include "TypeDef.h"

class MppfMode : public Mode
{
protected:
    Int m_bias;
    Int m_stepSize;
    ColorSpace m_mppfAdaptiveCsc[2];
    UInt m_bitsPerCompA[g_numComps];
    UInt m_bitsPerCompB[g_numComps];
    UInt m_mppfAdaptiveNumTrials;
    UInt m_minBlockBits;
    UInt m_mppfIndex;
    UInt m_mppfIndexNext;
    Int* m_midpoint[g_numComps];
    Int* m_nextBlockQuantRes[g_numComps];
    Int* m_mppSuffixSsmMapping[g_numComps];
    Int* m_qResBufA[g_numComps];
    Int* m_qResBufB[g_numComps];
    Int* m_recBufA[g_numComps];
    Int* m_recBufB[g_numComps];
	Int* m_recBufC[g_numComps];

public:
    MppfMode (Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat, ColorSpace csc);
    ~MppfMode ();

    // virtual overloads
    Void InitModeSpecific ();
    Void UpdateQp ();
    Void Test ();
    Void Encode ();
    Void Decode ();

    // methods
	Void ComputeMidpointUnified ();
    Void CopyTempRecBuffer (UInt mppfIndex);
    Void CopyTempQuantBuffer (UInt mppfIndex);
    Void TempRecBufferYCoCgToRgb ();
    Int EncodeMppfType (Bool isFinal);
    Void ClearMppSuffixMapping ();
    Void DecodeMppfSuffixBits (Bool isSsmIdx0);
    Void InitNextBlockCache ();
    Void CopyCachedSuffixBits ();
    Void MppfIndexSwap () { m_mppfIndex = m_mppfIndexNext; }
	Void TempRecBufferRgbToYCoCg ();

    // setters
    Void SetMppfIndexNextBlock (UInt index) { m_mppfIndexNext = index; }
    Void SetMppSuffixMapping ();
    Void SetMppfBitsPerComp (UInt idx, UInt ss0, UInt ss1, UInt ss2);
    Void SetMidpoint (Int comp, Int part, Int mp) { m_midpoint[comp][part] = mp; }

    // getters
    Int GetMidpoint (Int comp, Int part) { return m_midpoint[comp][part]; }
    UInt GetMinBlockBits () { return m_minBlockBits; }
    Int GetStepSize (UInt index, UInt k);
};

#endif // __MPPF_MODE__