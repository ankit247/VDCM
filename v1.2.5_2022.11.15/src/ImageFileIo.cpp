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

#include "TypeDef.h"
#include "ImageFileIo.h"
#include "VideoFrame.h"
#include <fstream>
#include <iostream>
#include <cstring>

//! general !//
Void ImageFileIo::InitDpxParams () {
    m_dpxFileInfoHeader = new DpxFileInformationHeader;
    m_dpxImageInfoHeader = new DpxImageInformationHeader;
    m_dpxImageElement = new DpxImageElement;
    memset (m_dpxFileInfoHeader, 0, sizeof (m_dpxFileInfoHeader));
    memset (m_dpxImageInfoHeader, 0, sizeof (m_dpxImageInfoHeader));
    memset (m_dpxImageElement, 0, sizeof (m_dpxImageElement));

    //! fill in DPX header from available info !//
    Char* version = "V2.0";
    Char* creator = "VDC-M";
    UInt dataOffset = 8192;
    UInt descriptor = 0;
    UInt packing = 0;
    if (m_colorspace == kRgb && m_chroma == k444) {
        descriptor = 50;
    }
    else if (m_colorspace == kYCbCr && m_chroma == k422) {
        descriptor = 100;
    }
    else if (m_colorspace == kYCbCr && m_chroma == k444) {
        descriptor = 102;
    }
    else if (m_colorspace == kYCbCr && m_chroma == k420) {
        descriptor = 104;
    }
    switch (m_bitDepth) {
        case 8:
        case 16:
            packing = 0;
            break;
        case 10:
        case 12:
            packing = 1;
            break;
    }
    m_dpxImageElement->descriptor = descriptor;
    m_dpxImageElement->packing = packing;
    m_dpxImageElement->bitDepth = m_bitDepth;
    m_dpxImageInfoHeader->pixelsPerLine = m_width;
    m_dpxImageInfoHeader->linesPerImageElement = m_height;

	m_samplesPerLine = GetNumSamplesPerLine ();
	UInt totalFileSize = (m_dpxCommandLineOptions->forceWritePadLines)
		? (4 * m_height * GetNumDoubleWordsPerLine ()) + dataOffset
		: (4 * GetNumDoubleWords ()) + dataOffset;
    m_dpxFileInfoHeader->imageDataByteOffset = dataOffset;
    m_dpxFileInfoHeader->totalFileSize = totalFileSize;
    memcpy (m_dpxFileInfoHeader->versionNumber, version, 8 * sizeof (Char));
    memcpy (m_dpxFileInfoHeader->creator, creator, strlen (creator));
    m_dpxImageInfoHeader->imageOrientation = 0;
    m_dpxImageInfoHeader->numImageElements = 1;
    
    m_dpxImageElement->dataSign = 0;
    m_dpxImageElement->transferCharacteristic = 5;
    m_dpxImageElement->colorimetricSpecification = 5;
    m_dpxImageElement->encoding = 0;
    m_dpxImageElement->offsetToData = dataOffset;
}

Void ImageFileIo::CopyParams (ImageFileIo* io) {
    m_width = io->GetWidth ();
    m_height = io->GetHeight ();
    m_bitDepth = io->GetBitDepth ();
    m_fileBytes = io->GetFileBytes ();
    m_chroma = io->GetChromaFormat ();
    m_colorspace = io->GetColorspace ();
    if (m_fileFormat == kFileFormatDpx) {
        if (io->GetFileFormat () == kFileFormatDpx) {
            m_dpxFileInfoHeader = new DpxFileInformationHeader;
            m_dpxImageInfoHeader = new DpxImageInformationHeader;
            m_dpxImageElement = new DpxImageElement;
            memcpy (m_dpxFileInfoHeader, io->GetDpxFileInformationHeader (), sizeof (DpxFileInformationHeader));
            memcpy (m_dpxImageInfoHeader, io->GetDpxImageInformationHeader (), sizeof (DpxImageInformationHeader));
            memcpy (m_dpxImageElement, io->GetDpxImageElement (), sizeof (DpxImageElement));
			if (m_dpxCommandLineOptions->forceWritePadLines) {
				m_dpxFileInfoHeader->totalFileSize = (4 * m_height * GetNumDoubleWordsPerLine ()) + m_dpxFileInfoHeader->imageDataByteOffset;
			}
			else {
				m_dpxFileInfoHeader->totalFileSize = (4 * GetNumDoubleWords ()) + m_dpxFileInfoHeader->imageDataByteOffset;
			}
        }
        else {
            InitDpxParams ();
        }
    }
}

Void ImageFileIo::ParseHeader () {
    switch (m_fileFormat) {
        case kFileFormatDpx:
            ParseDpxHeader ();
            break;
        case kFileFormatPpm:
            ParsePpmHeader ();
            break;
    }
}
Void ImageFileIo::ReadData (VideoFrame* fr) {
    switch (m_fileFormat) {
        case kFileFormatPpm:
            m_headerEndian = kDpxBigEndian;
            ReadPpmData (fr);
            break;
        case kFileFormatDpx:
			m_samplesPerLine = GetNumSamplesPerLine ();
            ReadDpxData (fr);
            break;
        case kFileFormatRaw:
            m_headerEndian = kDpxBigEndian;
            ReadRawData (fr);
            break;
        case kFileFormatYuv:
			m_headerEndian = kDpxLittleEndian;
            ReadYuvData (fr);
            break;
    }
}
Void ImageFileIo::WriteData (VideoFrame* fr) {
    std::fstream fileStream (m_fileName, std::fstream::binary | std::fstream::out);
    switch (m_fileFormat) {
        case kFileFormatPpm:
            if (m_chroma == k422 || m_chroma == k420) {
                fprintf (stderr, "incorrect chroma format for PPM output: %s\n", g_chromaFormatNames[m_chroma].c_str ());
                exit (EXIT_FAILURE);
            }
            WritePpmHeader (&fileStream);
            WritePpmData (&fileStream, fr);
            break;
        case kFileFormatRaw:
            if (m_chroma == k422 || m_chroma == k420) {
                fprintf (stderr, "incorrect chroma format for RAW output: %s\n", g_chromaFormatNames[m_chroma].c_str ());
                exit (EXIT_FAILURE);
            }
            WriteRawData (&fileStream, fr);
            break;
        case kFileFormatDpx:
            WriteDpxHeader (&fileStream);
            WriteDpxData (&fileStream, fr);
            break;
        case kFileFormatYuv:
            WriteYuvData (&fileStream, fr);
            break;
    }
    fileStream.close ();
}

//! dpx !//
Void ImageFileIo::ReadDpxData (VideoFrame* fr) {
    std::fstream dpxFileStream (m_fileName, std::fstream::binary | std::fstream::in);
    dpxFileStream.seekg (m_dpxFileInfoHeader->imageDataByteOffset);
    m_dpxRgbOrderRead = (m_dpxCommandLineOptions->forceReadBgrOrder ? kDpxBgr : kDpxRgb);
    m_dataEndian = (m_dpxCommandLineOptions->forceReadLittleEndian ? kDpxLittleEndian : kDpxBigEndian);
	if (m_dpxCommandLineOptions->forceReadPadLines) {
		for (UInt h = 0; h < m_height; h++) {
			m_curLineSampleCounter = 0;
			for (UInt dw = 0; dw < GetNumDoubleWordsPerLine (); dw++) {
				Char buffer[4];
				dpxFileStream.read (buffer, 4);
				DoubleWordToImageBuffer (buffer, fr);
			}
		}
	}
	else {
		for (UInt dw = 0; dw < GetNumDoubleWords (); dw++) {
			Char buffer[4];
			dpxFileStream.read (buffer, 4);
			DoubleWordToImageBuffer (buffer, fr);
		}
	}
    dpxFileStream.close ();
}

Void ImageFileIo::WriteDpxData (std::fstream* stream, VideoFrame* fr) {
    ResetDpxCounters ();
    m_dpxRgbOrderWrite = (m_dpxCommandLineOptions->forceWriteBgrOrder ? kDpxBgr : kDpxRgb);
    m_dataEndian = (m_dpxCommandLineOptions->forceWriteLittleEndian ? kDpxLittleEndian : kDpxBigEndian);
	if (m_dpxCommandLineOptions->forceWritePadLines) {
		m_samplesPerLine = GetNumSamplesPerLine ();
		for (UInt h = 0; h < m_height; h++) {
			m_curLineSampleCounter = 0;
			for (UInt dw = 0; dw < GetNumDoubleWordsPerLine (); dw++) {
				Char buffer[4];
				ImageBufferToDoubleWord (buffer, fr);
				stream->write (buffer, 4);
			}
		}
	}
	else {
		for (UInt dw = 0; dw < GetNumDoubleWords (); dw++) {
			Char buffer[4];
			ImageBufferToDoubleWord (buffer, fr);
			stream->write (buffer, 4);
		}
	}
}

Void ImageFileIo::ReadDpxMagicWord (std::fstream* stream) {
    Char temp = stream->get ();
    m_headerEndian = (temp == 0x53 ? kDpxBigEndian : kDpxLittleEndian);
    m_dataEndian = m_headerEndian; // may be overloaded later
    stream->seekg (0);
    m_dpxFileInfoHeader->magicNumber = ReadDoubleWord (stream);
}

Void ImageFileIo::ParseDpxHeader () {
    std::fstream dpxFileStream (m_fileName, std::fstream::binary | std::fstream::in);

    m_dpxFileInfoHeader = new DpxFileInformationHeader;
    memset (m_dpxFileInfoHeader, 0, sizeof (DpxFileInformationHeader));
    m_dpxImageInfoHeader = new DpxImageInformationHeader;
    memset (m_dpxImageInfoHeader, 0, sizeof (DpxImageInformationHeader));
    m_dpxImageElement = new DpxImageElement;
    memset (m_dpxImageElement, 0, sizeof (DpxImageElement));

    //! get the file size of the input image !//
    dpxFileStream.seekg (0, std::ios::end);
    m_fileBytes = (UInt)dpxFileStream.tellg ();
    dpxFileStream.seekg (0, std::ios::beg);

    //! file information header (5.1) !//
    ReadDpxMagicWord (&dpxFileStream);
    m_dpxFileInfoHeader->imageDataByteOffset = ReadDoubleWord (&dpxFileStream);
    Char version[8];
    dpxFileStream.read (version, 8);
    memcpy (m_dpxFileInfoHeader->versionNumber, version, 8 * sizeof (Char));
    m_dpxFileInfoHeader->totalFileSize = ReadDoubleWord (&dpxFileStream);

    //! sanity check the total file size !//
    if (m_dpxFileInfoHeader->totalFileSize != m_fileBytes) {
        fprintf (stderr, "ImageFileIo::ParseDpxHeader(): warning! file size in DPX header appears incorrect.\n");
        fprintf (stderr, "ImageFileIo::ParseDpxHeader():     actual size: %d bytes.\n", m_fileBytes);
        fprintf (stderr, "ImageFileIo::ParseDpxHeader(): DPX header size: %d bytes.\n", m_dpxFileInfoHeader->totalFileSize);
        m_dpxFileInfoHeader->totalFileSize = m_fileBytes;
    }

    //! "creator" field !//
    dpxFileStream.seekg (160);
    Char creator[100];
    for (UInt i = 0; i < 100; i++) {
        dpxFileStream >> creator[i];
    }

    //! image information header (5.2) !//
    dpxFileStream.seekg (768);
    m_dpxImageInfoHeader->imageOrientation = ReadWord (&dpxFileStream);
    m_dpxImageInfoHeader->numImageElements = ReadWord (&dpxFileStream);
    m_dpxImageInfoHeader->pixelsPerLine = ReadDoubleWord (&dpxFileStream);
    m_dpxImageInfoHeader->linesPerImageElement = ReadDoubleWord (&dpxFileStream);

    //! image element 1 !//
    m_dpxImageElement->dataSign = ReadDoubleWord (&dpxFileStream);
    m_dpxImageElement->refLoCodeValue = ReadDoubleWord (&dpxFileStream);
    m_dpxImageElement->refLoCodeRep = ReadDoubleWord (&dpxFileStream);
    m_dpxImageElement->refHiCodeValue = ReadDoubleWord (&dpxFileStream);
    m_dpxImageElement->refHiCodeRep = ReadDoubleWord (&dpxFileStream);
    m_dpxImageElement->descriptor = ReadByte (&dpxFileStream);
    m_dpxImageElement->transferCharacteristic = ReadByte (&dpxFileStream);
    m_dpxImageElement->colorimetricSpecification = ReadByte (&dpxFileStream);
    m_dpxImageElement->bitDepth = ReadByte (&dpxFileStream);
    m_dpxImageElement->packing = ReadWord (&dpxFileStream);
    m_dpxImageElement->encoding = ReadWord (&dpxFileStream);
    m_dpxImageElement->offsetToData = ReadDoubleWord (&dpxFileStream);

    //! copy info to data structure !//
    m_width = m_dpxImageInfoHeader->pixelsPerLine;
    m_height = m_dpxImageInfoHeader->linesPerImageElement;
    m_bitDepth = m_dpxImageElement->bitDepth;
    switch (m_dpxImageElement->descriptor) {
        case 50:
            m_colorspace = kRgb;
            m_chroma = k444;
            break;
        case 100:
            m_colorspace = kYCbCr;
            m_chroma = k422;
            break;
        case 102:
            m_colorspace = kYCbCr;
            m_chroma = k444;
            break;
        case 104:
            m_colorspace = kYCbCr;
            m_chroma = k420;
            break;
        default:
            fprintf (stderr, "ImageFileIo::ParseDpxHeader(): unsupported DPX descriptor (%d).\n", m_dpxImageElement->descriptor);
            exit (EXIT_FAILURE);
    }

    m_dataEndian = kDpxBigEndian;
}

Void ImageFileIo::WriteDpxHeader (std::fstream* stream) {
    Char* version = "V2.0";
    Char* creator = "VDC-M";
    UInt dataOffset = 8192;
    WriteDoubleWord (g_dpxMagicWordBigEndian, stream);
    WriteDoubleWord (dataOffset, stream);
    WriteCharArray (version, 8, stream);

    //! file size !//
	UInt totalFileSize = (m_dpxCommandLineOptions->forceWritePadLines)
		? (4 * m_height * GetNumDoubleWordsPerLine ()) + dataOffset
		: (4 * GetNumDoubleWords ()) + dataOffset;
    WriteDoubleWord (totalFileSize, stream);

    memcpy (m_dpxFileInfoHeader->creator, creator, strlen (creator));
    //! zero pad to 160 !//
    UInt offsetBytes = 160 - (UInt)stream->tellg ();
    while (offsetBytes > 0) {
        WriteByte (0, stream);
        offsetBytes--;
    }
    WriteCharArray (m_dpxFileInfoHeader->creator, (UInt)strlen (m_dpxFileInfoHeader->creator), stream);

    //! zero pad to 660 !//
    offsetBytes = 660 - (UInt)stream->tellg ();
    while (offsetBytes > 0) {
        WriteByte (0, stream);
        offsetBytes--;
    }

    //! encryption key (unset) !//
    WriteDoubleWord (0xFFFFFFFF, stream);

    //! zero pad to 768 !//
    offsetBytes = 768 - (UInt)stream->tellg ();
    while (offsetBytes > 0) {
        WriteByte (0, stream);
        offsetBytes--;
    }
    WriteWord (m_dpxImageInfoHeader->imageOrientation, stream);
    WriteWord (m_dpxImageInfoHeader->numImageElements, stream);
    WriteDoubleWord (m_dpxImageInfoHeader->pixelsPerLine, stream);
    WriteDoubleWord (m_dpxImageInfoHeader->linesPerImageElement, stream);
    WriteDoubleWord (m_dpxImageElement->dataSign, stream);
    WriteDoubleWord (m_dpxImageElement->refLoCodeValue, stream);
    WriteDoubleWord (m_dpxImageElement->refLoCodeRep, stream);
    WriteDoubleWord (m_dpxImageElement->refHiCodeValue, stream);
    WriteDoubleWord (m_dpxImageElement->refHiCodeRep, stream);

    //! zero pad to 800 !//
    offsetBytes = 800 - (UInt)stream->tellg ();
    while (offsetBytes > 0) {
        WriteByte (0, stream);
        offsetBytes--;
    }
    WriteByte (m_dpxImageElement->descriptor, stream);
    WriteByte (m_dpxImageElement->transferCharacteristic, stream);
    WriteByte (m_dpxImageElement->colorimetricSpecification, stream);
    WriteByte (m_dpxImageElement->bitDepth, stream);
    WriteWord (m_dpxImageElement->packing, stream);
    WriteWord (m_dpxImageElement->encoding, stream);
    
    if (m_dpxImageElement->descriptor == 50) {
		if (m_dpxCommandLineOptions->forceWriteDataOffset) {
			WriteDoubleWord (m_dpxImageElement->offsetToData, stream);
		}
		else {
			WriteDoubleWord (0, stream);

		}
    }
    else {
        WriteDoubleWord (m_dpxImageElement->offsetToData, stream);
    }

    //! zero pad to data offset !//
    offsetBytes = dataOffset - (UInt)stream->tellg ();
    while (offsetBytes > 0) {
        WriteByte (0, stream);
        offsetBytes--;
    }
}

Void ImageFileIo::ResetDpxCounters () {
    m_dpxSampleCounter = 0;
    memset (m_dpxCompCounters, 0, g_numComps * sizeof (UInt));
}

Void ImageFileIo::GetDpxSampleComponent (UInt& comp, UInt& idx, Bool isRead, Bool enPadding, Bool& isValid) {
	if (enPadding && m_curLineSampleCounter >= m_samplesPerLine) {
		isValid = false;
		return;
	}
	else {
		isValid = true;
	}
    switch (m_dpxImageElement->descriptor) {
        case 50:        //! either BGR or RGB !//
            if (isRead) {
                comp = m_dpxRgbOrderRead == kDpxBgr ? g_dpxSampleOrder_bgr[m_dpxSampleCounter % 3] : g_dpxSampleOrder_rgb[m_dpxSampleCounter % 3];
            }
            else {
                comp = m_dpxRgbOrderWrite == kDpxBgr ? g_dpxSampleOrder_bgr[m_dpxSampleCounter % 3] : g_dpxSampleOrder_rgb[m_dpxSampleCounter % 3];
            }
            break;
        case 100:       //! Cb Y Cr Y (4:2:2) !//
            comp = g_dpxSampleOrder_uyvy[m_dpxSampleCounter % 4];
            break;
        case 102:       //! Cb Y Cr (4:4:4) !//
            comp = g_dpxSampleOrder_uyv[m_dpxSampleCounter % 3];
            break;
        case 104:       //! Y plane, Cb/Cr half-plane (4:2:2) !//
            comp = (m_dpxSampleCounter < (m_width * m_height)) ? 0 : (m_dpxSampleCounter % 2 == 0 ? 1 : 2);
            break;
    }
    idx = m_dpxCompCounters[comp];
    m_dpxSampleCounter++;
    m_dpxCompCounters[comp]++;
	if (enPadding) {
		m_curLineSampleCounter++;
	}
}

//! ppm !//
Void ImageFileIo::WritePpmHeader (std::fstream* stream) {
    Char a[64], b[64], c[64], d[64];
    sprintf (a, "P6\n");
    sprintf (b, "# VDC-M\n");
    sprintf (c, "%d %d\n", m_width, m_height);
    sprintf (d, "%d\n", m_bitDepth == 8 ? 255 : m_bitDepth == 10 ? 1023 : 4095);
    stream->write (a, strlen (a));
    stream->write (b, strlen (b));
    stream->write (c, strlen (c));
    stream->write (d, strlen (d));
}

Void ImageFileIo::ParsePpmHeader () {
    std::fstream ppmFileStream (m_fileName, std::fstream::binary | std::fstream::in);

    //! get the file size of the input image !//
    ppmFileStream.seekg (0, std::ios::end);
    m_fileBytes = (UInt)ppmFileStream.tellg ();
    ppmFileStream.seekg (0, std::ios::beg);

    UInt maxVal = 0;
    Char magicWord[2];
    ppmFileStream.read (magicWord, 2);
    SkipWhitespace (&ppmFileStream);

    //! ignore comment lines (starts with #) !//
    if (ppmFileStream.peek () == 0x23) {
        while (!IsNextCharNewline (&ppmFileStream)) {
            ppmFileStream.get ();
        }
    }
    SkipWhitespace (&ppmFileStream);
    while (!IsNextCharWhitespace (&ppmFileStream)) {
        m_width = (10 * m_width) + (UInt)(ppmFileStream.get () - 0x30);
    }
    SkipWhitespace (&ppmFileStream);
    while (!IsNextCharWhitespace (&ppmFileStream)) {
        m_height = (10 * m_height) + (UInt)(ppmFileStream.get () - 0x30);
    }
    SkipWhitespace (&ppmFileStream);
    while (!IsNextCharWhitespace (&ppmFileStream)) {
        maxVal = (10 * maxVal) + (UInt)(ppmFileStream.get () - 0x30);
    }

    //! just one whitespace char here !//
    ppmFileStream.get ();
    m_ppmDataOffset = (UInt)ppmFileStream.tellg ();
    switch (maxVal) {
        case 255:
            m_bitDepth = 8;
            break;
        case 1023:
            m_bitDepth = 10;
            break;
        case 4095:
            m_bitDepth = 12;
            break;
        default:
            fprintf (stderr, "ImageFileIo::ParsePpmHeader(): unsupported PPM bit depth (max val = %d).\n", maxVal);
            exit (EXIT_FAILURE);
    }
    m_chroma = k444;
    m_colorspace = kRgb;
    ppmFileStream.close ();
}
Void ImageFileIo::ReadPpmData (VideoFrame* fr) {
    std::fstream ppmFileStream (m_fileName, std::fstream::binary | std::fstream::in);
    ppmFileStream.seekg (m_ppmDataOffset);
    for (UInt i = 0; i < m_width*m_height; i++) {
        for (UInt k = 0; k < 3; k++) {
            UInt sample = (UInt)(m_bitDepth == 8 ? ReadByte (&ppmFileStream) : ReadWord (&ppmFileStream));
            fr->getBuf (k)[i] = sample;
        }
    }
    ppmFileStream.close ();
}
Void ImageFileIo::WritePpmData (std::fstream* stream, VideoFrame* fr) {
    for (UInt i = 0; i < (m_width * m_height); i++) {
        for (UInt k = 0; k < 3; k++) {
            UInt sample = fr->getBuf (k)[i];
            switch (m_bitDepth) {
                case 8:
                    stream->put ((Char)sample);
                    break;
                case 10:
                case 12:
                    stream->put ((Char)((sample >> 8) & 0xFF));
                    stream->put ((Char)(sample & 0xFF));
                    break;
            }
        }
    }
}

Void ImageFileIo::SetBaseParams (UInt width, UInt height, UInt bitDepth, ColorSpace cs, ChromaFormat cf) {
    m_width = width;
    m_height = height;
    m_bitDepth = bitDepth;
    m_colorspace = cs;
    m_chroma = cf;
}

//! raw !//
Void ImageFileIo::ReadRawData (VideoFrame* fr) {
    std::fstream rawFileStream (m_fileName, std::fstream::binary | std::fstream::in);
    rawFileStream.seekg (0);
    for (UInt i = 0; i < m_width*m_height; i++) {
        for (UInt k = 0; k < 3; k++) {
            UInt sample = (UInt)(m_bitDepth == 8 ? ReadByte (&rawFileStream) : ReadWord (&rawFileStream));
            fr->getBuf (k)[i] = sample;
        }
    }
    rawFileStream.close ();
}
Void ImageFileIo::WriteRawData (std::fstream* stream, VideoFrame* fr) {
    for (UInt i = 0; i < (m_width * m_height); i++) {
        for (UInt k = 0; k < 3; k++) {
            UInt sample = fr->getBuf (k)[i];
            switch (m_bitDepth) {
                case 8:
                    stream->put ((Char)sample);
                    break;
                case 10:
                case 12:
                    stream->put ((Char)((sample >> 8) & 0xFF));
                    stream->put ((Char)(sample & 0xFF));
                    break;
            }
        }
    }
}

//! yuv !//
Void ImageFileIo::ReadYuvData (VideoFrame* fr) {
    std::fstream yuvFileStream (m_fileName, std::fstream::binary | std::fstream::in);
    yuvFileStream.seekg (0);
    UInt w, h;
    switch (m_chroma) {
        case k444:
            for (UInt k = 0; k < g_numComps; k++) {
                w = m_width;
                h = m_height;
                for (UInt i = 0; i < w * h; i++) {
                    UInt sample = (UInt)(m_bitDepth == 8 ? ReadByte (&yuvFileStream) : ReadWord (&yuvFileStream));
                    fr->getBuf (k)[i] = sample;
                }
            }
            break;
        case k422:
            for (UInt k = 0; k < g_numComps; k++) {
                w = (k == 0 ? m_width : m_width >> 1);
                h = m_height;
                for (UInt i = 0; i < w * h; i++) {
                    UInt sample = (UInt)(m_bitDepth == 8 ? ReadByte (&yuvFileStream) : ReadWord (&yuvFileStream));
					fr->getBuf (k)[i] = sample;
                }
            }
            break;
        case k420:
            for (UInt k = 0; k < g_numComps; k++) {
                w = (k == 0 ? m_width : m_width >> 1);
                h = (k == 0 ? m_height : m_height >> 1);
                for (UInt i = 0; i < w * h; i++) {
                    UInt sample = (UInt)(m_bitDepth == 8 ? ReadByte (&yuvFileStream) : ReadWord (&yuvFileStream));
                    fr->getBuf (k)[i] = sample;
                }
            }
            break;
    }
    yuvFileStream.close ();
}
Void ImageFileIo::WriteYuvData (std::fstream* stream, VideoFrame* fr) {
    for (UInt k = 0; k < 3; k++) {
        UInt w = (m_width >> g_compScaleX[k][m_chroma]);
        UInt h = (m_height >> g_compScaleY[k][m_chroma]);
        for (UInt i = 0; i < w * h; i++) {
            UInt sample = fr->getBuf (k)[i];
            switch (m_bitDepth) {
                case 8:
                    stream->put ((Char)sample);
                    break;
                case 10:
                case 12:
					// currently set to write as little endian
					stream->put ((Char)(sample & 0xFF));
                    stream->put ((Char)((sample >> 8) & 0xFF));
                    break;
            }
        }
    }
}

//! helpers !//
Void ImageFileIo::WriteCharArray (Char* array, UInt len, std::fstream* stream) {
    for (UInt byte = 0; byte < len; byte++) {
        stream->put (array[byte]);
    }
}
Void ImageFileIo::WriteDoubleWord (UInt value, std::fstream* stream) {
    for (UInt byte = 0; byte < 4; byte++) {
        stream->put ((Char)(value >> (8 * (3 - byte))));
    }
}
Void ImageFileIo::WriteWord (UInt value, std::fstream* stream) {
    for (UInt byte = 0; byte < 2; byte++) {
        stream->put ((Char)(value >> (8 * (1 - byte))));
    }
}
Void ImageFileIo::WriteByte (UChar byte, std::fstream* stream) {
    stream->put (byte);
}
UChar ImageFileIo::ReadByte (std::fstream* stream) {
    Char byte;
    stream->read (&byte, 1);
    return (UChar)byte;
}
UInt ImageFileIo::ReadWord (std::fstream* stream) {
    UInt value = 0;
    for (UInt byte = 0; byte < 2; byte++) {
        Char tempByte = stream->get ();
        switch (m_headerEndian) {
            case kDpxLittleEndian:
                value |= ((UChar)tempByte) << (8 * byte);
                break;
            case kDpxBigEndian:
                value |= ((UChar)tempByte) << (8 * (1 - byte));
                break;
        }
    }
    return value;
}
UInt ImageFileIo::ReadDoubleWord (std::fstream* stream) {
    UInt value = 0;
    for (UInt byte = 0; byte < 4; byte++) {
        Char tempByte = stream->get ();
        switch (m_headerEndian) {
            case kDpxLittleEndian:
                value |= ((UChar)tempByte) << (8 * byte);
                break;
            case kDpxBigEndian:
                value |= ((UChar)tempByte) << (8 * (3 - byte));
                break;
        }
    }
    return value;
}

Void ImageFileIo::DoubleWordToImageBuffer (Char* dw, VideoFrame* fr) {
    //! construct 32-bit value from four bytes !//
    UChar byteA = (UChar)dw[m_dataEndian == kDpxBigEndian ? 0 : 3];
    UChar byteB = (UChar)dw[m_dataEndian == kDpxBigEndian ? 1 : 2];
    UChar byteC = (UChar)dw[m_dataEndian == kDpxBigEndian ? 2 : 1];
    UChar byteD = (UChar)dw[m_dataEndian == kDpxBigEndian ? 3 : 0];
    UInt dword = (byteA << 24) | (byteB << 16) | (byteC << 8) | (byteD);
    UInt datum0, datum1, datum2, datum3;
    UInt curSampleComp = 0;
    UInt curSampleIdx = 0;
	Bool curSampleValid = true;
	Bool enPadding = m_dpxCommandLineOptions->forceReadPadLines;

	UInt compNumPx[3] = {0};
	for (UInt k = 0; k < 3; k++) {
		compNumPx[k] = (m_width * m_height) >> (g_compScaleX[k][m_chroma] + g_compScaleY[k][m_chroma]);
	}

    switch (m_dpxImageElement->packing) {
        case 0:
            switch (m_bitDepth) {
                case 8:
                    datum3 = (dword & 0xFF000000) >> 24;
                    datum2 = (dword & 0x00FF0000) >> 16;
                    datum1 = (dword & 0x0000FF00) >> 8;
                    datum0 = (dword & 0x000000FF);
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum3;
					}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum2;
					}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum1;
					}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum0;
					}
                    break;
                case 10:
                case 12:
                    fprintf (stderr, "ImageFileIo::DoubleWordToImageBuffer(): invalid bit depth: %d (use other routine).\n", m_bitDepth);
                    exit (EXIT_FAILURE);
                    break;
                case 16:
                    datum1 = (dword & 0xFFFF0000) >> 16;
                    datum0 = (dword & 0x0000FFFF);
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum1;
					}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum0;
					}
                    break;
            }
            break;
        case 1:
            switch (m_bitDepth) {
                case 10:
                    datum2 = (dword & 0xFFC00000) >> 22;
                    datum1 = (dword & 0x003FF000) >> 12;
                    datum0 = (dword & 0x00000FFC) >> 2;
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum2;
					}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum1;
			}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum0;
					}
                    break;
                case 12:
					datum1 = (dword & 0xFFF00000) >> 20;
					datum0 = (dword & 0x0000FFF0) >> 4;
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum1;
					}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum0;
					}
                    break;
                default:
                    fprintf (stderr, "ImageFilIo::DoubleWordToImageBuffer(): unsupported combination: bpc=%d, packing=%d.\n",
                        m_bitDepth, m_dpxImageElement->packing);
                    exit (EXIT_FAILURE);
            }
            break;        
        case 2:
            switch (m_bitDepth) {
                case 10:
                    datum2 = (dword & 0x3FF00000) >> 20;
                    datum1 = (dword & 0x000FFC00) >> 10;
                    datum0 = (dword & 0x000003FF);
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum2;
					}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum1;
					}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum0;
					}
                    break;
                case 12:
                    datum1 = (dword & 0x0FFF0000) >> 16;
                    datum0 = (dword & 0x00000FFF);
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum1;
					}
					GetDpxSampleComponent (curSampleComp, curSampleIdx, true, enPadding, curSampleValid);
					if (curSampleValid) {
						fr->getBuf (curSampleComp)[curSampleIdx] = datum0;
					}
                    break;
                default:
                    fprintf (stderr, "ImageFilIo::DoubleWordToImageBuffer(): unsupported combination: bpc=%d, packing=%d.\n",
                        m_bitDepth, m_dpxImageElement->packing);
                    exit (EXIT_FAILURE);
            }
    }
}

Void ImageFileIo::ImageBufferToDoubleWord (Char* dw, VideoFrame* fr) {
    UInt curSampleComp = 0;
    UInt curSampleIdx = 0;
    UInt curSampleValue = 0;
    UInt dword = 0;
	Bool curSampleValid = true;
	Bool enPadding = m_dpxCommandLineOptions->forceWritePadLines;

    switch (m_dpxImageElement->packing) {
        case 0:
            switch (m_bitDepth) {
                case 8:
                    for (UInt sample = 0; sample < 4; sample++) {
						GetDpxSampleComponent (curSampleComp, curSampleIdx, false, enPadding, curSampleValid);
						curSampleValue = curSampleValid ? fr->getBuf (curSampleComp)[curSampleIdx] : 0;
						dword |= ((curSampleValue & 0xFF) << (8 * (3 - sample)));
                    }
                    break;
                case 10:
                case 12:
                    fprintf (stderr, "ImageFileIo::ImageBufferToDoubleWord(): invalid bit depth: %d (use other routine).\n", m_bitDepth);
                    exit (EXIT_FAILURE);
                    break;
                case 16:
                    for (UInt sample = 0; sample < 2; sample++) {
						GetDpxSampleComponent (curSampleComp, curSampleIdx, false, enPadding, curSampleValid);
						curSampleValue = curSampleValid ? fr->getBuf (curSampleComp)[curSampleIdx] : 0;
                        dword |= ((curSampleValue & 0xFFFF) << (16 * (1 - sample)));
                    }
                    break;
            }
            break;
        case 1:
            switch (m_bitDepth) {
                case 10:
                    for (UInt sample = 0; sample < 3; sample++) {
						GetDpxSampleComponent (curSampleComp, curSampleIdx, false, enPadding, curSampleValid);
						curSampleValue = curSampleValid ? fr->getBuf (curSampleComp)[curSampleIdx] : 0;
                        dword |= ((curSampleValue & 0x3FF) << (2 + 10 * (2 - sample)));
                    }
                    break;
                case 12:
                    for (UInt sample = 0; sample < 2; sample++) {
						GetDpxSampleComponent (curSampleComp, curSampleIdx, false, enPadding, curSampleValid);
						curSampleValue = curSampleValid ? fr->getBuf (curSampleComp)[curSampleIdx] : 0;
                        dword |= ((curSampleValue & 0xFFF) << (sample == 0 ? 20 : 4));
                    }
                    break;
            }
            break;
        case 2:
            switch (m_bitDepth) {
                case 10:
                    for (UInt sample = 0; sample < 3; sample++) {
						GetDpxSampleComponent (curSampleComp, curSampleIdx, false, enPadding, curSampleValid);
						curSampleValue = curSampleValid ? fr->getBuf (curSampleComp)[curSampleIdx] : 0;
                        dword |= ((curSampleValue & 0x3FF) << (10 * (2 - sample)));
                    }
                    break;
                case 12:
                    for (UInt sample = 0; sample < 2; sample++) {
						GetDpxSampleComponent (curSampleComp, curSampleIdx, false, enPadding, curSampleValid);
						curSampleValue = curSampleValid ? fr->getBuf (curSampleComp)[curSampleIdx] : 0;
                        dword |= ((curSampleValue & 0xFFF) << (sample == 0 ? 16 : 0));
                    }
                    break;
            }
            break;
    }
    dw[0] = (Char)((dword & 0xFF000000) >> 24);
    dw[1] = (Char)((dword & 0x00FF0000) >> 16);
    dw[2] = (Char)((dword & 0x0000FF00) >> 8);
    dw[3] = (Char)((dword & 0x000000FF));
}

UInt ImageFileIo::GetNumDoubleWordsPerLine () {
	UInt numSamples = GetNumSamplesPerLine ();
	UInt numDoubleWords = 0;
	switch (m_dpxImageElement->packing) {
		case 0:
			switch (m_bitDepth) {
				case 8:     //! 1 dw for each 4 samples !//
					numDoubleWords = 1 * ((numSamples + 3) >> 2);
					break;
				case 10:    //! 5 dws for each 16 samples !//
					numDoubleWords = 5 * ((numSamples + 15) >> 4);
					break;
				case 12:    //! 3 dws for each 8 samples !//
					numDoubleWords = 3 * ((numSamples + 7) >> 3);
					break;
				case 16:    //! 1 dw for each 2 samples !//
					numDoubleWords = 1 * ((numSamples + 1) >> 1);
					break;
				default:
					fprintf (stderr, "ImageFileIo::GetNumDoubleWords(): unsupported combination of packing (%d) and bit depth (%d).\n",
						m_dpxImageElement->packing, m_bitDepth);
					exit (EXIT_FAILURE);
			}
			break;
		case 1:
		case 2:
			switch (m_bitDepth) {
				case 10:    //! 1 dw for each 3 samples !//
					numDoubleWords = 1 * ((UInt)((numSamples + 2) / 3));
					break;
				case 12:    //! 1 dw for each 2 samples !//
					numDoubleWords = 1 * ((numSamples + 1) >> 1);
					break;
				default:
					fprintf (stderr, "ImageFileIo::GetNumDoubleWords(): unsupported combination of packing (%d) and bit depth (%d).\n",
						m_dpxImageElement->packing, m_bitDepth);
					exit (EXIT_FAILURE);
			}
			break;
		default:
			fprintf (stderr, "ImageFileIo::GetNumDoubleWords(): unsupported packing (%d)\n", m_dpxImageElement->packing);
			exit (EXIT_FAILURE);
	}
	return numDoubleWords;
}

UInt ImageFileIo::GetNumDoubleWords () {
    UInt numSamples = GetNumSamples ();
    UInt numDoubleWords = 0;
    switch (m_dpxImageElement->packing) {
        case 0:
            switch (m_bitDepth) {
                case 8:     //! 1 dw for each 4 samples !//
                    numDoubleWords = 1 * ((numSamples + 3) >> 2);
                    break;
                case 10:    //! 5 dws for each 16 samples !//
                    numDoubleWords = 5 * ((numSamples + 15) >> 4);
                    break;
                case 12:    //! 3 dws for each 8 samples !//
                    numDoubleWords = 3 * ((numSamples + 7) >> 3);
                    break;
                case 16:    //! 1 dw for each 2 samples !//
                    numDoubleWords = 1 * ((numSamples + 1) >> 1);
                    break;
                default:
                    fprintf (stderr, "ImageFileIo::GetNumDoubleWords(): unsupported combination of packing (%d) and bit depth (%d).\n",
                        m_dpxImageElement->packing, m_bitDepth);
                    exit (EXIT_FAILURE);
            }
            break;
        case 1:
        case 2:
            switch (m_bitDepth) {
                case 10:    //! 1 dw for each 3 samples !//
                    numDoubleWords = 1 * ((UInt)((numSamples + 2) / 3));
                    break;
                case 12:    //! 1 dw for each 2 samples !//
                    numDoubleWords = 1 * ((numSamples + 1) >> 1);
                    break;
                default:
                    fprintf (stderr, "ImageFileIo::GetNumDoubleWords(): unsupported combination of packing (%d) and bit depth (%d).\n",
                        m_dpxImageElement->packing, m_bitDepth);
                    exit (EXIT_FAILURE);
            }
            break;
        default:
            fprintf (stderr, "ImageFileIo::GetNumDoubleWords(): unsupported packing (%d)\n", m_dpxImageElement->packing);
            exit (EXIT_FAILURE);
    }
    return numDoubleWords;
}

UInt ImageFileIo::GetNumSamplesPerLine () {
	UInt numSamples = 0;
	switch (m_dpxImageElement->descriptor) {
		case 50:    //! 4:4:4 RGB !//
		case 102:   //! 4:4:4 CbYCr !//
			numSamples = 3 * m_width;
			break;
		case 100:   //! 4:2:2 CbYCrY !//
			numSamples = 2 * m_width;
			break;
		case 104:   //! 4:2:0 Y plane, Cb/Cr half-plane !//
			numSamples = (3 * m_width) >> 1;
			break;
	}
	return numSamples;
}

UInt ImageFileIo::GetNumSamples () {
    UInt numSamples = 0;
    switch (m_dpxImageElement->descriptor) {
        case 50:    //! 4:4:4 RGB !//
        case 102:   //! 4:4:4 CbYCr !//
            numSamples = 3 * m_width * m_height;
            break;
        case 100:   //! 4:2:2 CbYCrY !//
            numSamples = 2 * m_width * m_height;
            break;
        case 104:   //! 4:2:0 Y plane, Cb/Cr half-plane !//
            numSamples = (3 * m_width * m_height) >> 1;
            break;
    }
    return numSamples;
}

Bool ImageFileIo::IsNextCharNewline (std::fstream* stream) {
    return (stream->peek () == 0x0A ? true : false);
}
Bool ImageFileIo::IsNextCharWhitespace (std::fstream* stream) {
    Char a = stream->peek ();
    Bool isWhitespace = (a == 0x20 || a == 0x09 || a == 0x0A || a == 0x0B || a == 0x0C || a == 0x0D);
    return (isWhitespace ? true : false);
}
Void ImageFileIo::SkipWhitespace (std::fstream* stream) {
    while (IsNextCharWhitespace (stream)) {
        stream->get ();
    }
}

Void ImageFileIo::SetCommandLineOptions (DpxCommandLineOptions* ref) {
    m_dpxCommandLineOptions->forceReadBgrOrder = ref->forceReadBgrOrder;
    m_dpxCommandLineOptions->forceReadLittleEndian = ref->forceReadLittleEndian;
    m_dpxCommandLineOptions->forceWriteBgrOrder = ref->forceWriteBgrOrder;
    m_dpxCommandLineOptions->forceWriteLittleEndian = ref->forceWriteLittleEndian;
	m_dpxCommandLineOptions->forceWriteDataOffset = ref->forceWriteDataOffset;
	m_dpxCommandLineOptions->forceReadPadLines = ref->forceReadPadLines;
	m_dpxCommandLineOptions->forceWritePadLines = ref->forceWritePadLines;
}