/* Copyright (c) 2015-2019 Qualcomm Technologies, Inc. All Rights Reserved
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
\file       VideoFrame.cpp
\brief      Storage for an RGB frame
*/

#include <cstdlib>
#include <assert.h>
#include <memory.h>

#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include "TypeDef.h"
#include "VideoFrame.h"

/**
    VideoFrame constructor
    */
VideoFrame::VideoFrame ()
    : m_width (0)
    , m_height (0)
    , m_chromaFormat (k444)
    , m_padX (0)
    , m_padY (0) {
    for (Int i = 0; i < g_numComps; i++) {
        m_frameBuf[i] = NULL;   // Buffer (including margin)
        m_bitDepth[i] = 8;
    }
};

/**
    Destructor
    */
VideoFrame::~VideoFrame () {
    Destroy ();
}

/**
    Set up VideoFrame object and allocate memory

    @param frameWidth picture width
    @param frameHeight picture height
    @param chromaFormat chroma sampling format
    @param padX frame padding in x dimension
    @param padY frame padding in y dimension
    @param csc color space
    @param bitDepth bit depth
    */
Void VideoFrame::Create (const UInt frameWidth, const UInt frameHeight, const ChromaFormat chromaFormat, const UInt padX, const UInt padY, const ColorSpace csc, const Int bitDepth)
{
    m_width = frameWidth;
    m_height = frameHeight;
    m_chromaFormat = chromaFormat;
    m_padX = padX;
    m_padY = padY;
    m_origSrcCsc = csc;
    m_bitDepth[0] = bitDepth;
    m_bitDepth[1] = bitDepth;
    m_bitDepth[2] = bitDepth;
    // assign the picture arrays and set up the ptr to the top left of the original picture
    for (Int c = 0; c < g_numComps; c++) {
		UInt64 bufferLength = (UInt64)getWidth (c) * (UInt64)getHeight (c);
		m_frameBuf[c] = (Int *)xMalloc (Int, bufferLength);
    }
    return;
}

/**
    Clear memory
    */
Void VideoFrame::Destroy () {
    for (Int c = 0; c < g_numComps; c++) {
        if (m_frameBuf[c]) {
            xFree (m_frameBuf[c]);
            m_frameBuf[c] = NULL;
        }
    }
}