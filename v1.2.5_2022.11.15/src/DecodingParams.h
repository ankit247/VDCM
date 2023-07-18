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

#ifndef __DECODINGPARAMS__
#define __DECODINGPARAMS__

#include "TypeDef.h"

class DecodingParams
{
protected:
    char*           m_cmpFile;              // compressed bitstream file
    char*           m_recFile;              // output reconstruction file
    UInt            m_srcWidth;             // source width in pixel
    UInt            m_srcHeight;            // source height in pixel
    UInt            m_bitDepth;             // source bit-depth (assume BigEndian byte order)
    ChromaFormat    m_chromaFormat;         // Chroma format 
    Bool            m_isInterleaved;        // Is output file format interleaved or planar
    ColorSpace      m_origSrcCsc;           // Colorspace format
    UInt            m_sliceHeight;          // Height of a Slice. Slices are assumed to consist of multiple lines
    UInt            m_sliceWidth;           // Width of a slice.
    Int             m_slicesPerLine;        // # of slices per line.
    Int             m_sliceBytes;           // The number of bytes in a slice. This is constant across slices.
    Int             m_blkHeight;            // Height of a block (which is the basic coding unit)
    Int             m_blkWidth;             // Width of a block (which is the basic coding unit)
    Int             m_bpp;                  //
    FileFormat      m_fileFormatDec;        // file format for decoded image (could be PPM/DPX)
    DpxCommandLineOptions* m_dpxOptions;

    //! decoder flags !//
    Bool            m_ppsDumpFields;
    Bool            m_ppsDumpBytes;
    Bool            m_debugTracer;
    Bool            m_dumpModes;
    Bool            m_ppsDumpOnly;

public:
    DecodingParams ()
        : m_cmpFile (NULL)
        , m_recFile (NULL)
        , m_bitDepth (0)
        , m_srcWidth (0)
        , m_srcHeight (0)
        , m_sliceBytes (0)
        , m_chromaFormat (k444)
        , m_blkHeight (0)
        , m_blkWidth (0)
        , m_sliceHeight (0)
        , m_origSrcCsc (kRgb)
        , m_isInterleaved (true)
        , m_fileFormatDec (kFileFormatUndef)
        , m_ppsDumpBytes (false)
        , m_ppsDumpFields (false)
        , m_debugTracer (false)
        , m_dumpModes (false)
        , m_ppsDumpOnly (false)
    {
        m_dpxOptions = new DpxCommandLineOptions;
        m_dpxOptions->forceReadBgrOrder = false;
        m_dpxOptions->forceReadLittleEndian = false;
        m_dpxOptions->forceWriteBgrOrder = false;
        m_dpxOptions->forceWriteLittleEndian = false;
		m_dpxOptions->forceWriteDataOffset = false;
    };

    ~DecodingParams () {
        delete m_dpxOptions;
    }

    //! methods !//
    Bool Init (Int argc, Char* argv[]);
    Void ShowDecoderUsage ();
};

#endif // __ENCODINGPARAMS__