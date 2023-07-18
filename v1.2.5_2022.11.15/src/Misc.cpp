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
\file       Misc.cpp
\brief      Miscellaneous routines
*/

#include <fstream>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "TypeDef.h"
#include "Misc.h"
#include "EncTop.h"

Bool enEncTracer;
Bool enDecTracer;
FILE* encTracer;
FILE* decTracer;

/**
    Check for parameter in file

    @param cParamFileName filename to parse
    @param cParamName parameter to check for
    @return parameter found? (bool)
    */
Bool CheckParam (std::string cParamFileName, char *cParamName) {
    char cStr[256];
    Bool foundFlag = false;
    FILE *fpi = fopen (cParamFileName.c_str (), "r");
    assert (fpi != NULL);
    while (!foundFlag) {
        if (fscanf (fpi, "%s", cStr) == EOF) {
            break;
        }
        if (strstr (cStr, cParamName)) {
            foundFlag = true;
        }
    }
    return foundFlag;
}

/**
    Read parameter from file (integer)

    @param cParamFileName filename to parse
    @param cParamName parameter to check for
    @return parameter value (integer)
    */
Int ReadParam (std::string cParamFileName, char *cParamName) {
    char cStr[256];
    Bool foundFlag = false;
    Int value = 0;
    FILE *fpi = fopen (cParamFileName.c_str (), "r");
    assert (fpi != NULL);
    while (!foundFlag) {
        if (fscanf (fpi, "%s", cStr) == EOF) {
            break;
        }
        if (strstr (cStr, cParamName)) {
            foundFlag = true;
        }
    }
    if (foundFlag) {
        fscanf (fpi, "%d", &value);
    }
    else {
        value = 0;
    }
    fclose (fpi);
    return value;
}

/**
    Read parameter from file (string)

    @param cParamFileName filename to parse
    @param cParamName parameter to check for
    @return parameter value (string)
    */
std::string ReadParamString (std::string cParamFileName, char *cParamName) {
    char cStr[256];
    std::string configFileLocation;
    Bool foundFlag = false;
    FILE *fpi = fopen (cParamFileName.c_str (), "r");
    assert (fpi != NULL);
    while (!foundFlag) {
        if (fscanf (fpi, "%s", cStr) == EOF) {
            break;
        }
        if (strstr (cStr, cParamName)) {
            foundFlag = true;
        }
    }
    if (foundFlag) {
        char tempString[256];
        fscanf (fpi, "%s", tempString);
        configFileLocation = std::string (tempString);
    }
    else {
        configFileLocation = std::string ("file not found");
    }
    fclose (fpi);
    return configFileLocation;
}

/**
    Read array of values from file (integer)

    @param cParamFileName filename to parse
    @param cParamName parameter to check for
    @param dest destination buffer
    @param length number of parameter values to parse
    */
Void ReadParamVecSigned (std::string cParamFileName, char *cParamName, Int *dest, UInt length) {
	char cStr[256];
	Bool foundFlag = false;
	FILE *fpi = fopen (cParamFileName.c_str (), "r");
	assert (fpi != NULL);
	while (!foundFlag) {
		if (fscanf (fpi, "%s", cStr) == EOF) {
			break;
		}
		if (strstr (cStr, cParamName)) {
			foundFlag = true;
		}
	}

	if (!foundFlag) {
		return;
	}

	for (UInt i = 0; i < length; i++) {
		Int tempValue;
		char tempChar;
		fscanf (fpi, "%d", &tempValue);
		dest[i] = tempValue;
		fscanf (fpi, "%c", &tempChar); // comma
	}
	fclose (fpi);
}

Void ReadParamVec (std::string cParamFileName, char *cParamName, UInt *dest, UInt length) {
    char cStr[256];
    Bool foundFlag = false;
    FILE *fpi = fopen (cParamFileName.c_str (), "r");
    assert (fpi != NULL);
    while (!foundFlag) {
        if (fscanf (fpi, "%s", cStr) == EOF) {
            break;
        }
        if (strstr (cStr, cParamName)) {
            foundFlag = true;
        }
    }

    if (!foundFlag) {
        return;
    }

    for (UInt i = 0; i < length; i++) {
        UInt tempValue;
        char tempChar;
        fscanf (fpi, "%d", &tempValue);
        dest[i] = tempValue;
        fscanf (fpi, "%c", &tempChar); // comma
    }
    fclose (fpi);
}

/**
    Read parameter from file (boolean)

    @param cParamFileName filename to parse
    @param cParamName parameter to check for
    @return parameter value (boolean)
    */
Bool ReadParamBool (std::string cParamFileName, char *cParamName) {
    char cStr[256];
    Bool foundFlag = false;
    Int value = 0;
    FILE *fpi = fopen (cParamFileName.c_str (), "r");
    assert (fpi != NULL);
    while (!foundFlag) {
        if (fscanf (fpi, "%s", cStr) == EOF) {
            break;
        }
        if (strstr (cStr, cParamName)) {
            foundFlag = true;
        }
    }
    if (foundFlag) {
        fscanf (fpi, "%d", &value);
    }
    else {
        value = -1;
    }
    fclose (fpi);
    return (value == 1 ? true : false);
}

/**
    Parse file format

    @param filename filename to parse
    @return file format
    */
FileFormat ParseFileFormat (std::string filename) {
    FileFormat fmt = kFileFormatUndef;
    Int filenameLen = (Int)filename.length ();
    std::transform (filename.begin (), filename.end (), filename.begin (), ::tolower);
    for (int i = 0; i < g_numFormats; i++) {
        std::string curCheckFormat = g_formatWhiteList[i];
        Int findResult = (Int)filename.find (curCheckFormat, filenameLen - curCheckFormat.length ());
        if (findResult > 0) {
            fmt = (FileFormat)(i + 1);
            break;
        }
    }
    return fmt;
}

/**
    Decode MPPF type from bitstream

    @param ssm SSM object
    @return MPPF type
    */
UInt DecodeMppfType (SubStreamMux *ssm) {
    UInt mppfTypeBits = 1;
    UInt ssmIdx = 0;
    UInt type = (UInt)ssm->ReadFromFunnelShifter (ssmIdx, mppfTypeBits);
    return type;
}

/**
    Parse mode header from bitstream

    @param ssm SSM object
    @param prevMode mode type of previous block
    @return mode type of current block
    */
ModeType DecodeModeHeader (SubStreamMux *ssm, ModeType prevMode) {
    ModeType mode;
    UInt ssmIdx = 0;
    Int modeSameBits = 1;
    UInt sameFlag = (UInt)ssm->ReadFromFunnelShifter (ssmIdx, modeSameBits);
    if (sameFlag == 1) {
        mode = prevMode;
    }
    else {
        UInt temp = ssm->ReadFromFunnelShifter (ssmIdx, 2);
        switch (temp) {
        case 0:
            mode = kModeTypeTransform;
            break;
        case 1:
            mode = kModeTypeBP;
            break;
        case 2:
            mode = kModeTypeMPP;
            break;
        case 3:
            temp = ssm->ReadFromFunnelShifter (ssmIdx, 1);
            switch (temp) {
            case 0:
                mode = kModeTypeMPPF;
                break;
            case 1:
                mode = kModeTypeBPSkip;
                break;
            }
            break;
        }
    }
    return mode;
}

/**
    Parse flatness type from bitstream

    @param ssm SSM object
    @return flatness type
    */
FlatnessType DecodeFlatnessType (SubStreamMux *ssm) {
    FlatnessType flatnessType = kFlatnessTypeUndefined;
    UInt ssmIdx = 0;

    // parse the flatness flag
    UInt flatnessFlag = (UInt)ssm->ReadFromFunnelShifter (ssmIdx, 1);

    // parse the flatness type
    if (flatnessFlag == 1) {
        flatnessType = (FlatnessType)ssm->ReadFromFunnelShifter (ssmIdx, 2);
    }
    return flatnessType;
}

/**
    Compute log2 of input (used by PPS only)

    @param x input
    @return log2(x)
    */
Int SlowLog2 (Int x) {
    Int result = 0;
    while (x > 0) {
        x >>= 1;
        ++result;
    }
    return result;
}

/**
    Write array to debugTracer (1 component)

    @param fid debugTracer file pointer
    @param mode mode
    @param arrayName type of data for the array 
    @param len length of array
    @param array array data
    */
Void debugTracerWriteArray (FILE* fid, const char* mode, const char* arrayName, Int len, Int* array) {
    if (fid == NULL) {
        return;
    }
    fprintf (fid, "%s: %s = [", mode, arrayName);
    for (Int i = 0; i < len; i++) {
        fprintf (fid, "%d", array[i]);
        if (i < (len - 1)) {
            fprintf (fid, ", ");
        }
    }
    fprintf (fid, "]\n");
}

/**
    Write arrays to debugTracer (3 components)

    @param fid debugTracer file pointer
    @param mode mode
    @param arrayName type of data for the array 
    @param len length of array
    @param array array data
    @param cf chroma sampling format
    */
Void debugTracerWriteArray3 (FILE* fid, const char* mode, const char* arrayName, Int len, Int** array, ChromaFormat cf) {
    if (fid == NULL) {
        return;
    }
    for (Int k = 0; k < g_numComps; k++) {
        fprintf (fid, "%s: %s[%d] = [", mode, arrayName, k);
        Int compLen = len >> (g_compScaleX[k][cf] + g_compScaleY[k][cf]);
        for (Int i = 0; i < compLen; i++) {
            fprintf (fid, "%d", array[k][i]);
            if (i < (compLen - 1)) {
                fprintf (fid, ", ");
            }
        }
        fprintf (fid, "]\n");
    }
}

/**
    Write VESA release flag to the command line
    */
Void VesaReleaseFlag () {
    if (g_codecVersionRelease == 0x0) {
        fprintf (stdout, "VESA Display Compression - M (VDC-M) reference model v%d.%d.%d.\n",
            g_codecVersionMajor, g_codecVersionMinor, g_codecVersionPatch);
    }
    else {
        fprintf (stdout, "VESA Display Compression - M (VDC-M) reference model v%d.%d%c.%d.\n",
            g_codecVersionMajor, g_codecVersionMinor, g_codecVersionRelease, g_codecVersionPatch);
    }
    fprintf (stdout, "Copyright 2015-%d Qualcomm Incorporated.  All rights reserved.\n",
        g_currentYear);
    fprintf (stdout, "VESA Confidential.\n");
};

Int GetMinQp (Pps* pps) {
    switch (pps->GetBitDepth ()) {
        case 8:
            return 16;
        case 10:
            return 0;
        case 12:
            if (pps->GetVersionMajor () == 1 && pps->GetVersionMinor () == 0 && pps->GetVersionRelease () == NULL) {
                return 0;
            }
            else {
                return -16;
            }
        default:
            fprintf (stderr, "GetMinQp(): invalid bit depth: %d.\n", pps->GetBitDepth ());
            exit (EXIT_FAILURE);
    }
}

Void CopyYCoCgDataToRgbBuffer (VideoFrame* ycocg, VideoFrame* rgb, Int yStart, Int xStart) {
	Int *src_y = ycocg->getBuf (0);
	Int *src_co = ycocg->getBuf (1);
	Int *src_cg = ycocg->getBuf (2);
	Int *dst_r = rgb->getBuf (0);
	Int *dst_g = rgb->getBuf (1);
	Int *dst_b = rgb->getBuf (2);
	Int sliceWidth = rgb->getWidth (0);
	Int bitDepth = rgb->getBitDepth (0);
	Int rgbMax = (1 << bitDepth) - 1;
	for (Int y = 0; y < 2; y++) {
		for (Int x = 0; x < 8; x++) {
			Int src_idx = (y * sliceWidth) + (x + xStart);
			Int dst_idx = ((y + yStart) * sliceWidth) + (x + xStart);
			Int temp = src_y[src_idx] - (src_cg[src_idx] >> 1);
			Int G = src_cg[src_idx] + temp;
			Int B = temp - (src_co[src_idx] >> 1);
			Int R = src_co[src_idx] + B;
			dst_r[dst_idx] = Clip3<Int> (0, rgbMax, R);
			dst_g[dst_idx] = Clip3<Int> (0, rgbMax, G);
			dst_b[dst_idx] = Clip3<Int> (0, rgbMax, B);
		}
	}
}
