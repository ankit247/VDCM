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

#ifndef __IMAGE_FILE_IO__
#define __IMAGE_FILE_IO__

#include "TypeDef.h"
#include "VideoFrame.h"

class ImageFileIo {
protected:
    UInt m_width;
    UInt m_height;
    UInt m_bitDepth;
    UInt m_fileBytes;
    UInt m_ppmDataOffset;
    ChromaFormat m_chroma;
    ColorSpace m_colorspace;
    FileFormat m_fileFormat;
    char* m_fileName;
    DpxCommandLineOptions* m_dpxCommandLineOptions;
    DpxFileInformationHeader* m_dpxFileInfoHeader;
    DpxImageInformationHeader* m_dpxImageInfoHeader;
    DpxImageElement* m_dpxImageElement;
    DpxEndianType m_headerEndian;
    DpxEndianType m_dataEndian;
    UInt m_dpxSampleCounter;
    UInt* m_dpxCompCounters;
    DpxRgbOrderType m_dpxRgbOrderRead;
    DpxRgbOrderType m_dpxRgbOrderWrite;
	UInt m_curLineSampleCounter;
	UInt m_samplesPerLine;

public:
    ImageFileIo (char* fileName, FileFormat format)
        : m_fileName (fileName)
        , m_fileFormat (format)
        , m_width (0)
        , m_dpxCommandLineOptions (NULL)
        , m_dpxFileInfoHeader (NULL)
        , m_dpxImageInfoHeader (NULL)
        , m_dpxImageElement (NULL)
        , m_height (0)
        , m_fileBytes (0)
        , m_bitDepth (8)
        , m_chroma (k444)
        , m_colorspace (kRgb)
        , m_headerEndian (kDpxBigEndian)
        , m_dataEndian (kDpxBigEndian)
        , m_dpxRgbOrderRead (kDpxRgb)
        , m_dpxRgbOrderWrite (kDpxRgb)
        , m_ppmDataOffset (0)
		, m_curLineSampleCounter(0)
		, m_samplesPerLine (0)
    {
        m_dpxCompCounters = new UInt[g_numComps];
        ResetDpxCounters ();
        m_dpxCommandLineOptions = new DpxCommandLineOptions;
    };
    ~ImageFileIo () {
        if (m_dpxCommandLineOptions) { delete m_dpxCommandLineOptions; }
        if (m_dpxFileInfoHeader) { delete m_dpxFileInfoHeader; }
        if (m_dpxImageInfoHeader) { delete m_dpxImageInfoHeader; }
        if (m_dpxImageElement) { delete m_dpxImageElement; }
        if (m_dpxCompCounters) { delete[] m_dpxCompCounters; }
    };

    //! methods !//
    Void ParseHeader ();
    Void ParseDpxHeader ();
    Void ParsePpmHeader ();
    Void WriteDpxHeader (std::fstream* stream);
    Void WritePpmHeader (std::fstream* stream);
    Void CopyParams (ImageFileIo* io);
    Void DoubleWordToImageBuffer (Char* dw, VideoFrame* fr);
    Void ImageBufferToDoubleWord (Char* dw, VideoFrame* fr);
    Void ReadDpxMagicWord (std::fstream* stream);
    Void ReadData (VideoFrame* fr);
    Void ReadDpxData (VideoFrame* fr);
    Void ReadPpmData (VideoFrame* fr);
    Void ReadRawData (VideoFrame* fr);
    Void ReadYuvData (VideoFrame* fr);
    Void WriteData (VideoFrame* fr);
    Void WriteDpxData (std::fstream* stream, VideoFrame* fr);
    Void WritePpmData (std::fstream* stream, VideoFrame* fr);
    Void WriteRawData (std::fstream* stream, VideoFrame* fr);
    Void WriteYuvData (std::fstream* stream, VideoFrame* fr);
    Void ResetDpxCounters ();
    Void InitDpxParams ();
    Void SetCommandLineOptions (DpxCommandLineOptions* ref);

    //! helpers !//
    UInt ReadWord (std::fstream* stream);
    UInt ReadDoubleWord (std::fstream* stream);
    UChar ReadByte (std::fstream* stream);
    Void WriteCharArray (Char* array, UInt len, std::fstream* stream);
    Void WriteDoubleWord (UInt value, std::fstream* stream);
    Void WriteWord (UInt value, std::fstream* stream);
    Void WriteByte (UChar byte, std::fstream* stream);
    Void SkipWhitespace (std::fstream* stream);
    Bool IsNextCharWhitespace (std::fstream* stream);
    Bool IsNextCharNewline (std::fstream* stream);

	Void GetDpxSampleComponent (UInt& comp, UInt& idx, Bool isRead, Bool enPadding, Bool& isValid);
    
	//! setters !//
    Void SetBaseParams (UInt width, UInt height, UInt bitDepth, ColorSpace cs, ChromaFormat cf);

    //! getters !//
    UInt GetWidth () { return m_width; }
    UInt GetHeight () { return m_height; }
    UInt GetBitDepth () { return m_bitDepth; }
    UInt GetFileBytes () { return m_fileBytes; }
    UInt GetNumSamples ();
	UInt GetNumDoubleWordsPerLine ();
	UInt GetNumSamplesPerLine ();
    UInt GetNumDoubleWords ();
    FileFormat GetFileFormat () { return m_fileFormat; }
    ColorSpace GetColorspace () { return m_colorspace; }
    ChromaFormat GetChromaFormat () { return m_chroma; }
    DpxFileInformationHeader* GetDpxFileInformationHeader () { return m_dpxFileInfoHeader; }
    DpxImageInformationHeader* GetDpxImageInformationHeader () { return m_dpxImageInfoHeader; }
    DpxImageElement* GetDpxImageElement () { return m_dpxImageElement; }
};

#endif // __IMAGE_FILE_IO__