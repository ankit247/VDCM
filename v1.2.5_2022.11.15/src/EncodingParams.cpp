/* Copyright (c) 2015-2022 Qualcomm Technologies, Inc. All Rights Reserved
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
 \file       EncodingParams.cpp
 \brief      Encoder Parameters
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <math.h>
#include "Misc.h"
#include "EncodingParams.h"
#include "ImageFileIo.h"

using namespace std;

Void EncodingParams::ShowEncoderUsage () {
    fprintf (stdout, "VDCM_Encoder.exe -inFile <input file name>\n");
    fprintf (stdout, "                 -bitstream <compressed bitstream file>\n");
    fprintf (stdout, "                 -recFile <reconstructed image>\n");
    fprintf (stdout, "                 -bpp <bits per pixel>\n");
    fprintf (stdout, "                 -configFile <config file\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "over-rides for config file:\n");
    fprintf (stdout, "                 -sliceHeight <slice height (px)>\n");
    fprintf (stdout, "                 -slicesPerLine <slices per line>\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "for RAW/YUV:\n");
    fprintf (stdout, "                 -width <width>           : required for RAW/YUV input formats.\n");
    fprintf (stdout, "                 -height <height>         : required for RAW/YUV input formats.\n");
    fprintf (stdout, "                 -bitDepth <bpc>          : required for RAW/YUV input formats.\n");
    fprintf (stdout, "                 -chromaFormat <cf>       : required for YUV input format. \n");
    fprintf (stdout, "                                          : cf = 0: 4:4:4.\n");
    fprintf (stdout, "                                          : cf = 1: 4:2:2.\n");
    fprintf (stdout, "                                          : cf = 2: 4:2:0.\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "optional flags:\n");
    fprintf (stdout, "                 -debugMaps               : write various graphical info to file.\n");
    fprintf (stdout, "                 -debugTracer             : write debug trace to file (slow! large file!)\n");
    fprintf (stdout, "                 -dumpPpsBytes            : dump PPS byte table.\n");
    fprintf (stdout, "                 -dumpPpsFields           : dump PPS by field.\n");
    fprintf (stdout, "                 -dumpModes               : print out the coding mode distrubition.\n");
    fprintf (stdout, "                 -minilog                 : print small log to file.\n");
    fprintf (stdout, "                 -calcPsnr                : compute MSE/PSNR between source and reconstructed.\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "DPX flags:\n");
    fprintf (stdout, "                 -forceDpxReadLE          : read DPX file data using little endian byte order.\n");
    fprintf (stdout, "                 -forceDpxReadBGR         : read DPX file data using BGR color order.\n");
    fprintf (stdout, "                 -forceDpxWriteLE         : write DPX file data using little endian byte order.\n");
    fprintf (stdout, "                 -forceDpxWriteBGR        : write DPX file data using BGR color order.\n");
	fprintf (stdout, "                 -forceDpxWriteDataOffset : write DPX data offset header for RGB source content.\n");
	fprintf (stdout, "                 -forceDpxWritePadLines   : pad each line to nearest doubleword when writing DPX file.\n");
	fprintf (stdout, "                 -forceDpxReadPadLines    : read DPX file where each line was padded to nearest doubleword.\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "general info:\n");
    fprintf (stdout, "* supported formats: PPM, DPX, RAW (interleaved), YUV (planar).\n");
    fprintf (stdout, "* for RAW format, specify: width, height, bitDepth.\n");
    fprintf (stdout, "* for YUV format, specify: width, height, bitDepth, chromaFormat.\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "parameter info:\n");
    fprintf (stdout, "param                 default     min     max     note\n");
    fprintf (stdout, "sliceHeight           108         16      *       \n");
    fprintf (stdout, "slicesPerLine         1           1       *       \n");
    fprintf (stdout, "bpp                   6           4       12      resolution is 1/16bpp (e.g. 4, 4.0625, 4.125, ...)\n");
    fprintf (stdout, "\n");
    fprintf (stdout, "for more, see readme.\n");
}

Void EncodingParams::ConfigFileDataInit () {
    m_configFileData = new ConfigFileData ();
	m_configFileData->mppMinStepSize = 0;
    m_configFileData->lutTargetRateDelta = new UInt[1 << g_rc_targetRateBits];
	m_configFileData->lutFlatnessQp = new Int[1 << g_flatnessQpLutBits];
    m_configFileData->lutLambdaBitrate = new UInt[1 << g_rc_bitrateLambdaLutBitsInt];
    m_configFileData->lutLambdaBF = new UInt[1 << g_rc_bfLambdaLutBits];
    m_configFileData->lutMaxQp = new UInt[1 << g_rc_maxQpLutBits];
    m_configFileData->mppfBitsPerCompA = new UInt[g_numComps];
    m_configFileData->mppfBitsPerCompB = new UInt[g_numComps];
    ::memset (m_configFileData->mppfBitsPerCompA, 0, g_numComps * sizeof (UInt));
    ::memset (m_configFileData->mppfBitsPerCompB, 0, g_numComps * sizeof (UInt));
}

Bool EncodingParams::ParseConfigFile () {
    Bool status = true;

	//! check if config file exists !//
	FILE *ftemp = fopen (m_configFile.c_str (), "r");
	if (ftemp == NULL) {
		fprintf (stderr, "EncodingParams::ParseConfigFile(): unable to find config file (%s).\n", m_configFile.c_str ());
		return false;
	}
	fclose (ftemp);
    
	ConfigFileDataInit ();

    //! slice height !//
    Int sliceHeight = ReadParam (m_configFile, "sliceHeight");
    if (sliceHeight > 0) {
        m_configFileData->sliceHeight = sliceHeight;
    }

    //! slices per line !//
    Int slicesPerLine = ReadParam (m_configFile, "slicesPerLine");
    if (slicesPerLine >= 1) {
        m_configFileData->slicesPerLine = slicesPerLine;
    }

    //! ssm max se size !//
    Int ssmMaxSeSize = ReadParam (m_configFile, "ssmMaxSeSize");
    if (ssmMaxSeSize >= 1) {
        m_configFileData->ssmMaxSeSize = ssmMaxSeSize;
    }

	Int mppMinStepSize = ReadParam (m_configFile, "mppMinStepSize");
	if (mppMinStepSize >= 0) {
		m_configFileData->mppMinStepSize = mppMinStepSize;
	}

    if (CheckParam (m_configFile, "versionMajor") && CheckParam (m_configFile, "versionMinor")) {
        m_configFileData->versionMajor = ReadParam (m_configFile, "versionMajor");
        m_configFileData->versionMinor = ReadParam (m_configFile, "versionMinor");
    }
    else {
        fprintf (stderr, "EncodingParams::ParseConfigFile(): For codec version v1.2.3 and later, version_major and version_minor fields must be specfified in config file.\n");
        return false;
    }

	ReadParamVecSigned (m_configFile, "flatnessQpLut", m_configFileData->lutFlatnessQp, 1 << g_flatnessQpLutBits);
    ReadParamVec (m_configFile, "targetRateDeltaLut", m_configFileData->lutTargetRateDelta, 1 << g_rc_targetRateBits);
    ReadParamVec (m_configFile, "lambdaBitrateLut", m_configFileData->lutLambdaBitrate, 1 << g_rc_bitrateLambdaLutBitsInt);
    ReadParamVec (m_configFile, "lambdaBFLut", m_configFileData->lutLambdaBF, 1 << g_rc_bfLambdaLutBits);
    ReadParamVec (m_configFile, "maxQpLut", m_configFileData->lutMaxQp, 1 << g_rc_maxQpLutBits);
    ReadParamVec (m_configFile, "mppfBitsPerCompA", m_configFileData->mppfBitsPerCompA, g_numComps);
    ReadParamVec (m_configFile, "mppfBitsPerCompB", m_configFileData->mppfBitsPerCompB, g_numComps);
    m_configFileData->flatnessQpVeryFlatFbls = ReadParam (m_configFile, "flatnessQpVeryFlatFbls");
    m_configFileData->flatnessQpVeryFlatNfbls = ReadParam (m_configFile, "flatnessQpVeryFlatNfbls");
    m_configFileData->flatnessQpSomewhatFlatFbls = ReadParam (m_configFile, "flatnessQpSomewhatFlatFbls");
    m_configFileData->flatnessQpSomewhatFlatNfbls = ReadParam (m_configFile, "flatnessQpSomewhatFlatNfbls");
    m_configFileData->rcFullnessOffsetThreshold = ReadParam (m_configFile, "rcFullnessOffsetThreshold");
    m_configFileData->rcTargetRateExtraFbls = ReadParam (m_configFile, "rcTargetRateExtraFbls");
    return status;
}

Bool EncodingParams::InitAndParseCommandLine (Int argc, Char* argv[]) {
    Bool status = true;
    Bool isFoundBitstream = false;
    Bool isFoundConfigFile = false;
    Bool isFoundSliceHeight = false;
    Bool isFoundSlicesPerLine = false;
    Bool isFoundInputFile = false;
    Bool isFoundSourceWidth = false;
    Bool isFoundSourceHeight = false;
    Bool isFoundBitDepth = false;
    Bool isFoundBpp = false;
    Bool isFoundSsmMaxSeSize = false;
    Bool isFoundChromaFormat = false;
    Double bppTemp = 0.0; // will be converted to integer with 4 fractional bits.

    if (argc < 4) {
        cerr << "\nNumber of command line arguments is less than required!\n";
        ShowEncoderUsage ();
        return false;
    }

    for (Int n = 1; n < argc; n++) {
        string arg = argv[n];
        if ((arg == "-h") || (arg == "--help")) {
            ShowEncoderUsage ();
            return false;
        }
        else if (arg == "-forceDpxReadBGR") {
            m_dpxOptions->forceReadBgrOrder = true;
        }
        else if (arg == "-forceDpxReadLE") {
            m_dpxOptions->forceReadLittleEndian = true;
        }
        else if (arg == "-forceDpxWriteBGR") {
            m_dpxOptions->forceWriteBgrOrder = true;
        }
        else if (arg == "-forceDpxWriteLE") {
            m_dpxOptions->forceWriteLittleEndian = true;
        }
		else if (arg == "-forceDpxWriteDataOffset") {
			m_dpxOptions->forceWriteDataOffset = true;
		}
		else if (arg == "-forceDpxReadPadLines") {
			m_dpxOptions->forceReadPadLines = true;
		}
		else if (arg == "-forceDpxWritePadLines") {
			m_dpxOptions->forceWritePadLines = true;
		}
        else if (arg == "-dumpPpsFields") {
            m_ppsDumpFields = true;
        }
        else if (arg == "-dumpPpsBytes") {
            m_ppsDumpBytes = true;
        }
        else if (arg == "-debugTracer") {
            m_debugTracer = true;
        }
        else if (arg == "-debugMaps") {
            m_debugInfoConfig.debugMapsEnable = true;
        }
        else if (arg == "-calcPsnr") {
            m_calcPsnr = true;
        }
        else if (arg == "-dumpModes") {
            m_dumpModes = true;
        }
        else if (arg == "-minilog") {
            m_minilog = true;
        }
        else if (arg == "-inFile") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                m_inFile = argv[++n];
                isFoundInputFile = true;
            }
            else { // An argument is required
                cerr << "-inFile option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-ssmMaxSeSize") {
            if ((n + 1) < argc) {
                m_ssmMaxSeSize = (UInt)atoi (argv[++n]);
                isFoundSsmMaxSeSize = true;
            }
            else {
                cerr << "-ssmMaxSeSize option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-bitstream") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                m_cmpFile = argv[++n];
                isFoundBitstream = true;
            }
            else { // An argument is required
                cerr << "-bitstream option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-recFile") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                m_recFile = argv[++n];
                m_isRecFileSpecified = true;
            }
            else { // An argument is required
                cerr << "-recFile option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-width") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                m_srcWidth = atoi (argv[++n]);
                isFoundSourceWidth = true;
            }
            else { // An argument is required
                cerr << "-width option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-height") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                m_srcHeight = atoi (argv[++n]);
                isFoundSourceHeight = true;
            }
            else { // An argument is required
                cerr << "-height option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-configFile") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                m_configFile = string (argv[++n]);
                isFoundConfigFile = true;
            }
            else { // An argument is required
                cerr << "-configFile option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-bitDepth") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                m_bitDepth = atoi (argv[++n]);
                isFoundBitDepth = true;
            }
            else { // An argument is required
                cerr << "-bitDepth option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-chromaFormat") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                m_chromaFormat = (ChromaFormat)atoi (argv[++n]);
                isFoundChromaFormat = true;
            }
            else { // An argument is required
                cerr << "-chromaFormat option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-bpp") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                bppTemp = atof (argv[++n]);
                isFoundBpp = true;
            }
            else { // An argument is required
                cerr << "-bpp option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-sliceHeight") {
            if ((n + 1) < argc) { // Make sure we aren't at the end of argv!
                m_sliceHeight = atoi (argv[++n]);
                isFoundSliceHeight = true;
            }
            else { // An argument is required
                cerr << "-sliceHeight option requires one argument." << endl;
                return false;
            }
        }
        else if (arg == "-slicesPerLine") {
            if ((n + 1) < argc) {
                m_slicesPerLine = atoi (argv[++n]);
                isFoundSlicesPerLine = true;
            }
            else {
                cerr << "-slicesPerLine option requires on argument." << endl;
                return false;
            }
        }
        else {
            cerr << "param not recognized: " << arg << endl;
            ShowEncoderUsage ();
            return false;
        }
    }
    m_fileFormatIn = ParseFileFormat (m_inFile);
    if (m_isRecFileSpecified) {
        m_fileFormatRec = ParseFileFormat (m_recFile);
    }
	
	//! check if source file exists !//
	FILE *ftemp = fopen (m_inFile, "r");
	if (ftemp == NULL) {
		fprintf (stderr, "EncodingParams::InitAndParseCommandLine(): unable to find source image (%s).\n", m_inFile);
		return false;
	}
	fclose (ftemp);

    //! read width/height/bitDepth from file header (PPM/DPX) !//
    m_imageFileIn = new ImageFileIo (m_inFile, m_fileFormatIn);
    switch (m_fileFormatIn) {
        case kFileFormatPpm:
        case kFileFormatDpx:
            m_imageFileIn->ParseHeader ();
            m_srcWidth = m_imageFileIn->GetWidth ();
            m_srcHeight = m_imageFileIn->GetHeight ();
            m_bitDepth = m_imageFileIn->GetBitDepth ();
            m_chromaFormat = m_imageFileIn->GetChromaFormat ();
            m_origSrcCsc = m_imageFileIn->GetColorspace ();
            isFoundSourceWidth = true;
            isFoundSourceHeight = true;
            isFoundBitDepth = true;
            break;
        case kFileFormatYuv:
            m_origSrcCsc = kYCbCr;
            m_imageFileIn->SetBaseParams (m_srcWidth, m_srcHeight, m_bitDepth, kYCbCr, m_chromaFormat);
            break;
        case kFileFormatRaw:
            m_chromaFormat = k444;
            m_origSrcCsc = kRgb;
            m_imageFileIn->SetBaseParams (m_srcWidth, m_srcHeight, m_bitDepth, kRgb, k444);
            break;
    }

    //! check for invalid use cases !//
    switch (m_fileFormatIn) {
        case kFileFormatRaw:
            break;
        case kFileFormatYuv:
            if (!isFoundChromaFormat) {
                fprintf (stderr, "must specify -chromaFormat for YUV input.\n");
                return false;
            }
            break;
        case kFileFormatDpx:
            break;
        case kFileFormatPpm:
            break;
    }

    //! error out if width/height/bpc unspecified !//
    if (!(isFoundSourceHeight || isFoundSourceWidth || isFoundBitDepth)) {
        ShowEncoderUsage ();
        return false;
    }

    //! parse the config file !//
    if (isFoundConfigFile) {
        if (!ParseConfigFile ()) {
            return false;
        }
    }
    else {
        fprintf (stderr, "must specify -configFile.\n");
        return false;
    }

    if (!isFoundBpp) {
        bppTemp = g_default_bpp;
        isFoundBpp = true;
    }

    //! slice height !//
    if (!isFoundSliceHeight) {
        m_sliceHeight = m_configFileData->sliceHeight;
        isFoundSliceHeight = true;
    }

    //! slices per line !//
    if (!isFoundSlicesPerLine) {
        m_slicesPerLine = m_configFileData->slicesPerLine;
        isFoundSlicesPerLine = true;
    }

    //! ssm max se size !//
    if (!isFoundSsmMaxSeSize) {
        m_ssmMaxSeSize = m_configFileData->ssmMaxSeSize;
        isFoundSsmMaxSeSize = true;
    }

    // m_bpp must be a multiple of 1/16
    Double precision = (Double)(1 << g_rc_bppFractionalBits);
    if (floor (precision * bppTemp) != ceil (precision * bppTemp)) {
        fprintf (stderr, "bpp must be a multiple of (1/16).\n");
        return false;
    }
    m_bpp = (Int)(bppTemp * precision);
    m_sliceWidth = (Int)(m_srcWidth / m_slicesPerLine);

    if (!isFoundBitstream) {
        ShowEncoderUsage ();
        return false;
    }

    return true;
}