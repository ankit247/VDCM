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

/* Copyright (c) 2015-2017 MediaTek Inc., MPP mode is modified and/or implemented by MediaTek Inc. based on MppMode.h
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
\file       MppMode.h
\brief      Midpoint Prediction Mode (header)
*/

#ifndef __MPP_MODE__
#define __MPP_MODE__

#include "Mode.h"
#include "TypeDef.h"

class MppMode : public Mode
{
protected:
    Int* m_midpoint[g_numComps];
    Int m_bias;
    Int m_stepSize;
    Int m_colorSpaceQpThres;
    Int* m_mppSuffixSsmMapping[g_numComps];
    ColorSpace m_nextBlockCsc;
    UInt m_nextBlockStepSize;
    UInt m_curBlockStepSize;
    Int* m_nextBlockQuantRes[g_numComps];
    Int* m_resBlkBufA[g_numComps];
    Int* m_resBlkBufB[g_numComps];
    Int* m_quantBlkBufA[g_numComps];
    Int* m_quantBlkBufB[g_numComps];
    Int* m_recBlkBufA[g_numComps];
    Int* m_recBlkBufB[g_numComps];
	Int* m_recBlkBufC[g_numComps];

public:
    MppMode (Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat, ColorSpace csc, Int colorSpaceQpThres);
    ~MppMode ();

    // virtual overloads
    Void InitModeSpecific ();
    Void UpdateQp ();
    Void Test ();
    Void Encode ();
    Void Decode ();

    // methods
    Void InitBuffers ();
    Int ComputeCurTrialDistortion (Int trial);
	Void RecTrialRgbToYCoCg ();
	Void RecTrialYCoCgToRgb ();
    Void RecTrialYCoCgToRgb (Int trial);
	Void ComputeMidpointUnified ();
    UInt EncodeStepSize (Bool isFinal);
    Void ParseStepSize ();
    Void NextBlockStepSizeSwap ();
    Int DecMapStepSize (Int k, Bool isSsm0);
    Int EncMapStepSize (Int k);
    Void ParseCsc ();
    Void CscBitSwap ();
    Void DecodeMppSuffixBits (Bool isSsmIdx0);
    Void InitNextBlockCache ();
    Void ClearNextBlockCache ();
    Void SetMppSuffixMapping ();
    Void ClearMppSuffixMapping ();
    Void CopyCachedSuffixBits ();

    // setters
    Void SetMidpoint (Int comp, Int part, Int mp) { m_midpoint[comp][part] = mp; }

    // getters
    Int GetMidpoint (Int comp, Int part) { return m_midpoint[comp][part]; }
    Int GetStepSize (Int k);
    Int GetBppIndex ();
};

#endif // __MPP_MODE__