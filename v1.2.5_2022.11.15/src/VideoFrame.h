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
\file       VideoFrame.h
\brief      Storage for an RGB frame
*/

#ifndef __VIDEOFRAME__
#define __VIDEOFRAME__

#include <stdio.h>
#include "TypeDef.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

// video frame buffer class
class VideoFrame
{
private:
    Int m_bitDepth[g_numComps];             // bitdepth of video components.
    ColorSpace m_origSrcCsc;                // whether the internal video format is RGB or YUV
    Int* m_frameBuf[g_numComps];            // Frame Buffer
	UInt m_width;
	UInt m_height;
	UInt m_padX;
	UInt m_padY;
    ChromaFormat m_chromaFormat;            // 0: 420, 1: 422, 2: 444


public:
    VideoFrame ();
    ~VideoFrame ();

    //! methods !//
	Void Create (const UInt frameWidth, const UInt frameHeight, const ChromaFormat chromaFormat, const UInt padX, const UInt padY, const ColorSpace csc, const Int bitDepth);
    Void Destroy ();

    //! getters !//
	UInt getWidth (const Int compID) { return m_width >> g_compScaleX[compID][m_chromaFormat]; }
	UInt getHeight (const Int compID) { return m_height >> g_compScaleY[compID][m_chromaFormat]; }
	UInt getOrigWidth (const Int compID) { return ((m_width - m_padX) >> g_compScaleX[compID][m_chromaFormat]); }
	UInt getPadX (const Int compID) { return (m_padX >> g_compScaleX[compID][m_chromaFormat]); }
    Int getBitDepth (const Int compID){ return m_bitDepth[compID]; }
    ChromaFormat getChromaFormat () { return m_chromaFormat; }
    ColorSpace getColorSpace () { return m_origSrcCsc; }
    Int* getBuf (const Int compID) { return m_frameBuf[compID]; }
};

#endif // __VIDEOFRAME__
