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
\file       EncTop.h
\brief      Encoder Top (header)
*/

#ifndef __ENC_TOP__
#define __ENC_TOP__

#include "ImageFileIo.h"
#include <vector>
#include "VideoFrame.h"
#include "EncodingParams.h"
#include "RateControl.h"
#include "DebugMap.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder class
class EncTop : public EncodingParams
{
private:
    DebugMap* m_debugMapMode;
    DebugMap* m_debugMapFlat;
    DebugMap* m_debugMapQp;
    DebugMap* m_debugMapBuffer;
    DebugMap* m_debugMapTargetBitrate;
    UInt m_slicePadX;
    UInt m_bottomSlicePadY;

public:
    //constructor
    EncTop ()
        : m_debugMapMode (NULL)
        , m_debugMapFlat (NULL)
        , m_debugMapQp (NULL)
        , m_debugMapBuffer (NULL)
        , m_debugMapTargetBitrate (NULL)
        , m_slicePadX (0)
        , m_bottomSlicePadY (0)
    {
    };

    ~EncTop () {};

    Bool Encode ();
    Void EncodeSlice (Pps* pps, Int startY, Int startX, BitstreamOutput* sliceBits, VideoFrame* orgVideoFrame, VideoFrame* recVideoFrame);

    Void InitSliceData (VideoFrame* orgVideoFrame, VideoFrame* curSliceFrame, Int startY, Int startX, Int finalSliceWidth, Int finalSliceHeight, Int padX, Int padY);

    Bool CalculateSlicePadding ();
    Int ComputeLambdaBufferFullness (Int bf, Pps *pps);
    ModeType SelectCurBlockMode (std::vector<Int>& rates, std::vector<Int>& distortions, std::vector<Int>& rdCosts, RateControl* rc, Bool isLastBlock, Int minBlockBits, Pps *pps, std::vector<Bool> &modeSelectionIsValid);
    Void PrintEncodeInfo (ModeType mode, UInt y, UInt x, UInt qp, UInt nBits, UInt prevBitsNoPadding);
	Bool VersionChecks (Pps* pps);
};

#endif // __ENCTOP__