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

#ifndef __DEBUG_MAP__
#define __DEBUG_MAP__

#include "TypeDef.h"
#include "Block.h"
#include "Pps.h"
#include <string.h>

class DebugMap
{
protected:
    Int m_srcWidth;
    Int m_srcHeight;
    Int m_stride;
    Int m_blkWidth;
    Int m_blkHeight;
    std::string m_fileName;
    Int m_bitDepth;
    Int* m_data[g_numComps];

public:
    DebugMap (Int width, Int height, Int blkWidth, Int blkHeight, Int bpc, std::string const& filename);
    ~DebugMap ();

    // methods
    Void Yuv2Rgb ();
    Void SavePpm ();
    Void DrawQp (Int startY, Int startX, UInt qp, Pps* pps);
    Void DrawMode (Int startY, Int startX, ModeType mode, Bool* isSplit);
    Void DrawTargetBitrate (Int startY, Int startX, Int target);
    Void CopySourceData (VideoFrame* fr);
    Void DrawFlatness (Int startY, Int startX, FlatnessType flatnessType);
    Void DrawBuffer (Int startY, Int startX, UInt buffer);
};

#endif // __DEBUG_MAP__