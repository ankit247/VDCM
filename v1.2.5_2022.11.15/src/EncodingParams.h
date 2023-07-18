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
\file       EncodingParams.h
\brief      Encoder Parameters (header)
*/

#ifndef __ENCODINGPARAMS__
#define __ENCODINGPARAMS__

#include "TypeDef.h"
#include "ImageFileIo.h"

class EncodingParams
{
protected:
    char*                   m_inFile;               // source file name
    char*                   m_cmpFile;              // compressed bitstream file
    char*                   m_recFile;              // output reconstruction file
    FILE*                   m_psnrFile;             //
    std::string             m_configFile;           // config file (must be provided)
    Int                     m_startFrame;           // Begin compression from this frame
    Int                     m_srcWidth;             // source width in pixel
    Int                     m_srcHeight;            // source height in pixel
    Int                     m_bitDepth;             // source bit-depth (assume BigEndian byte order)
    ChromaFormat            m_chromaFormat;         // Chroma format 
    Int                     m_bpp;                  //
    Bool                    m_isInterleaved;        // Is source file interleaved or planar
    ColorSpace              m_origSrcCsc;           // color space of input 
    UInt                    m_sliceHeight;          // Height of a Slice. Slices are assumed to consist of multiple lines
    UInt                    m_sliceWidth;           // Width of a slice.
    UInt                    m_slicesPerLine;        // # of slices per line.
    Int                     m_blkHeight;            // Height of a block (which is the basic coding unit)
    Int                     m_blkWidth;             // Width of a block (which is the basic coding unit)
    FileFormat              m_fileFormatIn;         //
    FileFormat              m_fileFormatRec;        //
    DebugInfoConfig         m_debugInfoConfig;      //
    Bool                    m_isRecFileSpecified;
    ConfigFileData*         m_configFileData;
    Int                     m_ssmMuxWordSize;
    Int                     m_ssmMaxSeSize;
    Int                     m_ssmBalanceFifoSize;
	ImageFileIo*		    m_imageFileIn;
	ImageFileIo*		    m_imageFileRec;
    DpxCommandLineOptions*  m_dpxOptions;

    //! encoder flags !//
    Bool                m_ppsDumpFields;
    Bool                m_ppsDumpBytes;
    Bool                m_debugTracer;
    Bool                m_calcPsnr;
    Bool                m_dumpModes;
    Bool                m_minilog;

public:
    EncodingParams ()
        : m_startFrame (0)
        , m_srcWidth (0)
        , m_srcHeight (0)
        , m_bitDepth (g_default_bitDepth)
        , m_isInterleaved (g_default_isInterleaved)
        , m_blkHeight (g_default_blkHeight)
        , m_blkWidth (g_default_blkWidth)
        , m_sliceHeight (g_default_sliceHeight)
        , m_origSrcCsc (g_default_csc)
        , m_chromaFormat (g_default_chromaFormat)
        , m_inFile (NULL)
        , m_cmpFile (NULL)
        , m_recFile (NULL)
        , m_psnrFile (NULL)
        , m_configFileData (NULL)
        , m_fileFormatIn (kFileFormatUndef)
        , m_fileFormatRec (kFileFormatUndef)
        , m_isRecFileSpecified (false)
        , m_ppsDumpBytes (false)
        , m_ppsDumpFields (false)
        , m_debugTracer (false)
        , m_calcPsnr (false)
        , m_dumpModes (false)
        , m_minilog (false)
		, m_imageFileIn (NULL)
		, m_imageFileRec (NULL)
    {
        // reset debug info
        m_debugInfoConfig.debugMapsEnable = false;
        m_dpxOptions = new DpxCommandLineOptions;
        m_dpxOptions->forceReadBgrOrder = false;
        m_dpxOptions->forceReadLittleEndian = false;
        m_dpxOptions->forceWriteBgrOrder = false;
        m_dpxOptions->forceWriteLittleEndian = false;
		m_dpxOptions->forceWriteDataOffset = false;
		m_dpxOptions->forceReadPadLines = false;
		m_dpxOptions->forceWritePadLines = false;
    }

    ~EncodingParams () {
        delete m_dpxOptions;
        if (m_configFileData) {
            if (m_configFileData->lutLambdaBF) { delete[] m_configFileData->lutLambdaBF; }
            if (m_configFileData->lutTargetRateDelta) { delete[] m_configFileData->lutTargetRateDelta; }
            if (m_configFileData->lutFlatnessQp) { delete[] m_configFileData->lutFlatnessQp; }
            if (m_configFileData->lutLambdaBitrate) { delete[] m_configFileData->lutLambdaBitrate; }
            if (m_configFileData->lutMaxQp) { delete[] m_configFileData->lutMaxQp; }
            if (m_configFileData->mppfBitsPerCompA) { delete[] m_configFileData->mppfBitsPerCompA; }
            if (m_configFileData->mppfBitsPerCompB) { delete[] m_configFileData->mppfBitsPerCompB; }
            delete m_configFileData;
        }
	}

    Bool InitAndParseCommandLine (Int argc, Char* argv[]);
    Bool ParseConfigFile ();
    Void ConfigFileDataInit ();
    Void ShowEncoderUsage ();
};

#endif // __ENCODINGPARAMS__