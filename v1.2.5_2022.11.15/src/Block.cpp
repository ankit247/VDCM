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
\file       Block.cpp
\brief      Block Class
*/

#include <string.h>
#include <assert.h>
#include <math.h>
#include "TypeDef.h"
#include "Block.h"

/**
    Block Constructor

    @param csc color space
    @param width block width
    @param height block height
    @param pOrg source VideoFrame buffer
    @param pRec destination VideoFrame buffer
    */
Block::Block (ColorSpace csc, Int width, Int height, VideoFrame* pOrg, VideoFrame* pRec)
    : m_origSrcCsc (csc)
    , m_curCsc (csc)
    , m_blkWidth (width)
    , m_blkStride (width)
    , m_blkHeight (height)
{
    m_pOrgVideoFrame = pOrg;
    m_isPartial = false;
    for (Int c = 0; c < g_numComps; c++) {
        Int stride = GetBlkStride (c);
        m_blkBufOrg[c] = (Int *)xMalloc (Int, stride * GetHeight (c));
    }
}

/**
    Clear memory
    */
Void Block::Destroy () {
    for (Int c = 0; c < g_numComps; c++) {
        if (m_blkBufOrg[c]) { xFree (m_blkBufOrg[c]); m_blkBufOrg[c] = NULL; }
    }
}

/**
    Initialize block

    @param startCol position within slice (column)
    @param startRow position within slice (row)
    @param bitDepth bit depth
    @param csc colorspace
    */
Void Block::Init (Int startCol, Int startRow, Int bitDepth, ColorSpace csc) {
    ChromaFormat chromaFormat = m_pOrgVideoFrame->getChromaFormat ();
    Int sliceWidth = m_pOrgVideoFrame->getOrigWidth (0) + m_pOrgVideoFrame->getPadX (0);
    m_origSrcCsc = csc;
    m_curCsc = csc;

    if ((startCol + m_blkStride) > sliceWidth) {
        m_isPartial = true;
        m_blkWidth = sliceWidth - startCol;
    }
    else {
        m_isPartial = false;
        m_blkWidth = m_blkStride;
    }

    for (Int c = 0; c < g_numComps; c++) {
        Int col = startCol >> g_compScaleX[c][chromaFormat];
        Int row = startRow >> g_compScaleY[c][chromaFormat];
        Int blkStride = GetBlkStride (c);
        Int sliceStride = (m_pOrgVideoFrame->getOrigWidth (0) + m_pOrgVideoFrame->getPadX (0)) >> g_compScaleX[c][chromaFormat];
        Int* pBufFr = m_pOrgVideoFrame->getBuf (c) + (row * sliceStride) + col;
        Int* pBufBlk = GetBufOrg (c);

        for (Int i = 0; i < GetHeight (c); i++) {
            ::memcpy ((void *)pBufBlk, (void *)pBufFr, sizeof (Int)* GetWidth (c));
            pBufFr += sliceStride;
            pBufBlk += blkStride;
        }
    }
}