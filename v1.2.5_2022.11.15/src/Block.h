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
\file       Block.h
\brief      Block Class (header)
*/

#ifndef __BLOCK__
#define __BLOCK__

#include <math.h>
#include "TypeDef.h"
#include "VideoFrame.h"

class RateControl;

class Block
{
protected:
    Int m_blkWidth;                     // block width in pixels
    Int m_blkStride;                    // Maximum block width in pixels. This will be same as m_blkWidth unless the block is on the right boundary.
    Int m_blkHeight;                    // block height in pixels
    Bool m_isPartial;                   // whether the block is a partial block   
    Int* m_blkBufOrg[g_numComps];        // Block buffers for the original video frame
    VideoFrame* m_pOrgVideoFrame;       //
    ColorSpace m_origSrcCsc;            //
    ColorSpace m_curCsc;                //

public:
    Block (ColorSpace csc, Int width, Int height, VideoFrame* pOrg, VideoFrame* pRec);
    ~Block () {};

    // methods
    Void Init (Int startCol, Int startRow, Int bitDepth, ColorSpace csc);
    Void Destroy ();

    // getters
    Int GetWidth (const Int compID) { return m_blkWidth >> g_compScaleX[compID][m_pOrgVideoFrame->getChromaFormat ()]; }
    Int GetBlkStride (const Int compID) { return m_blkStride >> g_compScaleX[compID][m_pOrgVideoFrame->getChromaFormat ()]; }
    Int GetHeight (const Int compID) { return m_blkHeight >> g_compScaleY[compID][m_pOrgVideoFrame->getChromaFormat ()]; }
    Int* GetBufOrg (const Int compID) { return m_blkBufOrg[compID]; }
};

#endif // __BLOCK__
