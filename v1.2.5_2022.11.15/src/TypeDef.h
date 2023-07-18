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

/* Copyright (c) 2015-2017 MediaTek Inc., RDO weighting is modified and/or implemented by MediaTek Inc. based on TypeDef.h
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
\file       TypeDef.h
\brief      Type definitions, globals, enums, etc.
*/

#ifndef __TYPEDEF__
#define __TYPEDEF__

#include "Config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <algorithm>

//! return codes !//
#define RETURN_PARSE_COMMAND_LINE_ERROR     10
#define RETURN_INVALID_PARAMS               11

// ====================================================================================================================
// Basic type redefinition
// ====================================================================================================================

typedef     void                Void;
typedef     bool                Bool;
typedef     char                Char;
typedef     unsigned char       UChar;
typedef     short               Short;
typedef     unsigned short      UShort;
typedef     int                 Int;
typedef     unsigned int        UInt;
typedef     double              Double;
typedef     signed char			int8;
typedef     unsigned char		uint8;
typedef     signed short        int16;
typedef     unsigned short      uint16;
typedef     signed int          int32;
typedef     unsigned int        uint32;
typedef     UChar               Pxl;        ///< 8-bit pixel type
//typedef     Short               Pel;        ///< 16-bit pixel type

// ====================================================================================================================
// 64-bit integer type
// ====================================================================================================================

#ifdef _MSC_VER
typedef       __int64             Int64;

#if _MSC_VER <= 1200 // MS VC6
typedef       __int64             UInt64;   // MS VC6 does not support unsigned __int64 to double conversion
#else
typedef       unsigned __int64    UInt64;
#endif

#else

typedef       long long           Int64;
typedef       unsigned long long  UInt64;

#endif

// ====================================================================================================================
// Helper Routines
// ====================================================================================================================

#define MAX_UINT                    0xFFFFFFFFU ///< max. value of unsigned 32-bit integer
#define MAX_INT                     2147483647  ///< max. value of signed 32-bit integer
#define MAX_INT64                   0x7FFFFFFFFFFFFFFF  ///< max. value of signed 64-bit integer
#define MAX_DOUBLE                  1.7e+308    ///< max. value of double-type value
#define LARGE_INT                   999999      ///< large integer
#define Max(x, y)                   ((x)>(y)?(x):(y))   
#define Min(x, y)                   ((x)<(y)?(x):(y))                                                 ///< min of (x, y)
#define M1                          MAX_UINT
#define Median(a, b, c)             ((a)>(b)?(a)>(c)?(b)>(c)?(b):(c):(a):(b)>(c)?(a)>(c)?(a):(c):(b)) ///< 3-point median

/** clip a, such that minVal <= a <= maxVal */
template <typename T> inline T Clip3 (T minVal, T maxVal, T a) { return std::min<T> (std::max<T> (minVal, a), maxVal); }  ///< general min/max clip

#define DATA_ALIGN                  1                                                                 ///< use 32-bit aligned malloc/free
#if     DATA_ALIGN && _WIN32 && ( _MSC_VER > 1300 )
#define xMalloc( type, len )        _aligned_malloc( sizeof(type)*(len), 32 )
#define xFree( ptr )                _aligned_free  ( ptr )
#else
#define xMalloc( type, len )        malloc   ( sizeof(type)*(len) )
#define xFree( ptr )                free     ( ptr )
#endif

#define FATAL_ERROR_0(MESSAGE, EXITCODE)                      \
{                                                             \
    printf(MESSAGE);                                            \
    exit(EXITCODE);                                             \
}

// ====================================================================================================================
// DPX
// ====================================================================================================================

enum DpxRgbOrderType { kDpxRgb, kDpxBgr };
enum DpxEndianType { kDpxLittleEndian, kDpxBigEndian };

struct DpxCommandLineOptions {
    Bool forceReadLittleEndian;
    Bool forceReadBgrOrder;
    Bool forceWriteLittleEndian;
    Bool forceWriteBgrOrder;
	Bool forceWriteDataOffset;
	Bool forceWritePadLines;
	Bool forceReadPadLines;
};

static const UInt g_dpxMagicWordBigEndian = 0x53445058;
static const UInt g_dpxMagicWordLittleEndian = 0x58504453;
static const UInt g_dpxSampleOrder_uyvy[4] = { 1, 0, 2, 0 };
static const UInt g_dpxSampleOrder_uyv[3] = { 1, 0, 2 };
static const UInt g_dpxSampleOrder_bgr[3] = { 2, 1, 0 };
static const UInt g_dpxSampleOrder_rgb[3] = { 0, 1, 2 };

struct DpxFileInformationHeader {
    UInt magicNumber;
    UInt imageDataByteOffset;
    Char versionNumber[8];
    Char creator[100];
    UInt totalFileSize;
};

struct DpxImageInformationHeader {
    uint16 imageOrientation;
    uint16 numImageElements;
    UInt pixelsPerLine;
    UInt linesPerImageElement;
};

struct DpxImageElement {
    UInt dataSign;
    uint8 descriptor;
    uint8 transferCharacteristic;
    uint8 colorimetricSpecification;
    uint8 bitDepth;
    uint16 packing;
    uint16 encoding;
    UInt offsetToData;
    UInt refLoCodeValue;
    UInt refHiCodeValue;
    UInt refLoCodeRep;
    UInt refHiCodeRep;
};

// ====================================================================================================================
// constants
// ====================================================================================================================

//! versioning
const UInt g_currentYear = 2022;
const UInt g_codecVersionMajor = 1;
const UInt g_codecVersionMinor = 2;
const Char g_codecVersionRelease = 0x0; //(Char)'a';
const UInt g_codecVersionPatch = 3;

//! general !//
const UInt g_numComps = 3;
const UInt g_numChromaFormats = 3;
const UInt g_minSliceWidth = 64;
const UInt g_minSliceHeight = 16;
const UInt g_minSlicePixels = 4096;

//! intra prediction !//
const UInt g_intra_numTotalModes = 8;
const UInt g_intra_numTotalModesBits = 3;
const UInt g_intra_numModesSecondPass = 4;

//! rate control !//
const Int g_rc_bppFractionalBits = 4;           //
const Int g_rc_qpStepSizeOne = 16;              //
const Int g_rc_BFRangeBits = 16;                //
const Int g_rc_BFScaleApprox = 4;               //
const Int g_rc_targetRateBits = 4;              //
const Int g_rc_targetRateLutEntriesBits = 6;    //
const Int g_rc_targetRateLutScaleBits = 6;      //
const Int g_rc_maxQpLutBits = 3;
const Int g_rc_bufferOffsetPrecision = 16;      //
const Int g_rc_globalMaxQp = 56;                //

//! substream multiplexing !//
const UInt g_ssm_numSubstreams = 4;
const UInt g_ssm_defaultSsmMuxWordSize = 128;
const UInt g_ssm_defaultSsmMaxSeSize = 128;
const UInt g_ssm_defaultSsmEncBalanceFifoSize = 16384;

// ====================================================================================================================
// enumerations
// ====================================================================================================================
enum ColorSpace { kRgb, kYCoCg, kYCbCr };
const std::string g_colorSpaceNames[3] = { "RGB", "YCoCg", "YCbCr" };

enum ChromaFormat { k444, k422, k420 };
const std::string g_chromaFormatNames[3] = { "444", "422", "420" };

enum EcType { kEcTransform, kEcBp };

const UInt g_numFormats = 4;
enum FileFormat { kFileFormatUndef, kFileFormatRaw, kFileFormatPpm, kFileFormatDpx, kFileFormatYuv };
const std::string g_formatWhiteList[g_numFormats] = { ".raw", ".ppm", ".dpx", ".yuv" };

const UInt g_numModes = 5;
const std::string g_codingModeNames[g_numModes] = { "XFORM", "BP", "MPP", "MPPF", "BP-SKIP" };

// available coding modes
enum ModeType {
    kModeTypeTransform,
    kModeTypeBP,
    kModeTypeMPP,
    kModeTypeMPPF,
    kModeTypeBPSkip,
    kModeTypeUndefined
};

//! intra predictors !//
enum IntraPredictorType {
    kIntraDc,
    kIntraVertical,
    kIntraDiagonalRight,
    kIntraDiagonalLeft,
    kIntraVerticalRight,
    kIntraVerticalLeft,
    kIntraHorizontalRight,
    kIntraHorizontalLeft,
    kIntraFbls,
    kIntraUndefined,
};

const std::string g_intraModeNames[9] = { "DC", "Vertical", "Diagonal-Right", "Diagonal-Left",
"Vertical-Right", "Vertical-Left", "Horizontal-Right", "Horizontal-Left", "FBLS" };

// ====================================================================================================================
// defaults
// ====================================================================================================================
const Int g_default_blkWidth = 8;
const Int g_default_blkHeight = 2;
const Int g_default_bitDepth = 8;
const ChromaFormat g_default_chromaFormat = k444;
const Int g_default_sliceHeight = 108;
const Int g_default_slicesPerLine = 1;
const Double g_default_bpp = 6.0;
const ColorSpace g_default_csc = kRgb;

const Bool g_default_isInterleaved = true;
const Int g_minBpc = 8;
const Int g_maxResidualBits = 16;
const UInt g_rcInitQp_444 = 36;
const UInt g_rcInitQp_422 = 30;
const UInt g_rcInitQp_420 = 32;
const UInt g_ec_groupSizeLimit = 50;
// ====================================================================================================================

// ====================================================================================================================
// chroma subsampling information
// ====================================================================================================================
const Int g_compScaleX[g_numComps][g_numChromaFormats] = { { 0, 0, 0 }, { 0, 1, 1 }, { 0, 1, 1 } };
const Int g_compScaleY[g_numComps][g_numChromaFormats] = { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0, 1 } };

// ====================================================================================================================
// Weights to calculate distortion
// ====================================================================================================================
const Int g_ycocgWeights[g_numComps] = { 443, 181, 222 };
const Int g_uniformWeights[g_numComps] = { 0, 0, 0 }; //right shift amount 

// ====================================================================================================================
// rate control
// ====================================================================================================================
#define BP_QP_OFFSET_NFLS                           2
#define BP_QP_OFFSET_FLS                            4

const Int g_qpIndexThresholdPositive[g_numChromaFormats][5] = {
    { 10, 29, 50, 60, 70 }, // 4:4:4
    { 9, 26, 45, 54, 63 }, // 4:2:2
    { 8, 23, 40, 48, 55 } // 4:2:0
};

const Int g_qpIndexThresholdNegative[g_numChromaFormats][4] = {
    { 10, 20, 35, 65 }, // 4:4:4
    { 9, 18, 31, 58 }, // 4:2:2
    { 8, 16, 28, 51 } // 4:2:0
};

const Int g_iQpIncrementSliceHeight16And32[5][6] = {
    { 0, 1, 2, 3, 4, 5 },    // normal operation 
    { 1, 3, 5, 6, 6, 6 },    // rcFullness >= 76
    { 2, 4, 5, 6, 7, 7 },    // rcFullness >= 88
    { -1, 0, 1, 1, 2, 2 },    // rcFullness <= 24
    { -2, -1, -1, 0, 1, 1 },    // rcFullness <= 12
};

const Int g_iQpIncrement[5][6] = {
    { 0, 1, 2, 3, 4, 5 },    // normal operation 
    { 1, 2, 3, 5, 5, 6 },    // rcFullness >= 76
    { 2, 3, 4, 6, 7, 7 },    // rcFullness >= 88
    { -1, 0, 1, 1, 2, 2 },    // rcFullness <= 24
    { -2, -1, -1, 0, 1, 1 },    // rcFullness <= 12
};

const Int g_iQpDecrementSliceHeight16And32[5][5] = {
    { 0, 1, 2, 3, 4 },        // normal operation 
    { -1, 0, 0, 1, 1 },        // rcFullness >= 76
    { -2, -2, 0, 1, 1 },        // rcFullness >= 88
    { 1, 1, 2, 4, 4 },        // rcFullness <= 24
    { 2, 2, 4, 5, 5 },        // rcFullness <= 12
};

const Int g_iQpDecrement[5][5] = {
    { 0, 1, 2, 3, 4 },        // normal operation 
    { -1, 0, 0, 1, 1 },        // rcFullness >= 76
    { -2, -2, 0, 1, 1 },        // rcFullness >= 88
    { 1, 1, 2, 4, 4 },        // rcFullness <= 24
    { 2, 2, 4, 5, 5 },        // rcFullness <= 12
};

// target rate approx. for (1/p), used in calculating the average target rate (# bits remaining in slice) / (# blocks remaining in slice)
const Int g_targetRateInverseLut[32] = {
    126, 122, 119, 115, 112, 109, 106, 103, 101, 98, 96, 94, 92, 90, 88, 86,
    84, 82, 81, 79, 78, 76, 75, 73, 72, 71, 70, 68, 67, 66, 65, 64
};
// ====================================================================================================================

// ====================================================================================================================
// mode header
// ====================================================================================================================
const Int g_maxModeHeaderLen = 4;
//                                          XFORM,    BP,   MPP,  MPPF, BPSKP
const Int g_modeHeaderLen[g_numModes] = { 2, 2, 2, 3, 3 };
const Int g_modeHeaderCode[g_numModes] = { 0, 1, 2, 6, 7 };
const UInt g_modeMapColor[g_numModes][3] = {
    { 255, 255, 255 },      // transform (white)
    { 255, 0, 255 },        // BP (magenta)
    { 0, 255, 0 },          // MPP (gren)
    { 0, 255, 255 },        // MPPF (cyan)
    { 255, 0, 0 },          // BP-SKIP (red)
};
// ====================================================================================================================

// ====================================================================================================================
// block prediction (BP)
// ====================================================================================================================
const Int g_defaultBpSearchRange = 64;
const Int g_absentPxValueRgb[3] = { 0, 0, 0 };
const Int g_absentPxValueYuv[3] = { 128, 128, 128 };
const Int g_absentPxValueYcg[3] = { 128, 0, 0 };
const static Int g_bpQuantBits = 6;
const static Int g_bpQuantDeadZone = 22;
const static Int g_bpQuantScales[8] = { 255, 234, 216, 200, 182, 167, 152, 139 };
const static Int g_bpInvQuantScales[8] = { 128, 140, 152, 164, 180, 196, 216, 236 };
const static UInt g_quantStepDouble = 8;
// ====================================================================================================================

// ====================================================================================================================
// flatness
// ====================================================================================================================
const UInt g_maxFlatHeaderLen = 3;
const UInt g_flatnessQpLutBits = 3;
const UInt g_flatnessQpLutSize = (1 << g_flatnessQpLutBits);

enum FlatnessType {
    kFlatnessTypeVeryFlat,
    kFlatnessTypeSomewhatFlat,
    kFlatnessTypeComplexToFlat,
    kFlatnessTypeFlatToComplex,
    kFlatnessTypeUndefined
};

const std::string g_flatnessTypeNames[4] = { "VeryFlat", "SomewhatFlat", "ComplexToFlat", "FlatToComplex" };

const UInt g_flatMapColor[4][3] = {
    { 255, 0, 0 },      // very flat (red)
    { 0, 255, 0 },      // somewhat flat (green)
    { 255, 255, 255 },      // complex-to-flat (white)
    { 0, 0, 255 }       // flat-to-complex (blue)
};

struct ComplexityStruct {
    Int prev[3];
    Int cur[3];
    Int next[3];
};
// ====================================================================================================================

struct DebugInfoConfig {
    Bool debugMapsEnable;
};

struct ConfigFileData {
    UInt sliceHeight;
    UInt slicesPerLine;
    UInt ssmMaxSeSize;
	UInt mppMinStepSize;
    UInt* lutTargetRateDelta;
    UInt* lutLambdaBitrate;
    UInt* lutLambdaBF;
    UInt* lutMaxQp;
    UInt* mppfBitsPerCompA;
    UInt* mppfBitsPerCompB;
	Int* lutFlatnessQp;
	Int flatnessQpVeryFlatFbls;
	Int flatnessQpVeryFlatNfbls;
	Int flatnessQpSomewhatFlatFbls;
	Int flatnessQpSomewhatFlatNfbls;
    UInt rcFullnessOffsetThreshold;
    UInt rcTargetRateExtraFbls;
    UInt versionMajor;
    UInt versionMinor;
};

// ====================================================================================================================
// quantization
// ====================================================================================================================
const static Int g_qStepChroma[57] = { 16, 17, 18, 20, 21, 22, 23, 24, 26, 27, 28, 29, 30, 31, 33, 34, 35, 37, 38, 39, 40, 41, 43, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 56, 57, 58, 59, 60, 62, 63, 64, 65, 66, 67, 68, 70, 71, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72 };
const static Int g_qStepCo[57] = { 24, 25, 26, 27, 29, 30, 31, 33, 34, 35, 37, 38, 39, 40, 42, 43, 44, 46, 47, 48, 50, 51, 52, 53, 55, 56, 57, 59, 60, 61, 63, 64, 65, 66, 68, 69, 70, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72 };
const static Int g_qStepCg[57] = { 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68, 69, 70, 71, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72 };
// ====================================================================================================================

// ====================================================================================================================
// transform
// ====================================================================================================================
const static Int g_maxTrDynamicRange[2] = { 16, 23 };
const static Int g_dequantShift = 9;
const static Int g_transformMatrixShiftHad = 6;
const static Int g_transformMatrixShiftHaar = 0;

//! DCT pre-shift !//
static const Int g_dctPreShift = 2;
static const Int g_dctPreShiftRound = 2;

//! 4-point DCT coefficients !//
static const Int g_dctA4 = 17;
static const Int g_dctB4 = 41;
static const Int g_dctShift4 = 6;

//! 8-point DCT coefficients !//
static const Int g_dctA8 = 17;
static const Int g_dctB8 = 41;
static const Int g_dctG8 = 111;
static const Int g_dctD8 = 22;
static const Int g_dctE8 = 94;
static const Int g_dctZ8 = 63;
static const Int g_dctShift8 = 7;

//! transform quant parameters !//
static const Int g_dctQuantBits = 8;
static const Int g_dctQuantDeadZone = 102; // 8-bit rep. dead zone of 0.4 (102/256)
static const Int g_dctDeQuantBits = 8;
static const Int g_dctDeQuantRound = 128;

//! mapping to normalization + quant coefficients for each sample !//
static const UInt g_dctQuantMapping_8x2[8] = { 0, 1, 2, 3, 0, 3, 2, 1 }; // map for Q/DQ coefficients
static const UInt g_dctQuantMapping_4x2[4] = { 0, 1, 0, 1 };
static const UInt g_dctQuantMapping_4x1[4] = { 0, 1, 0, 1 };

//! forward and inverse quant coefficients !//
static const UInt g_dctForwardQuant_8x2[8][4] = {
    { 256, 291, 516, 403 },
    { 241, 262, 473, 374 },
    { 216, 238, 437, 349 },
    { 195, 228, 406, 318 },
    { 178, 202, 370, 291 },
    { 164, 187, 341, 269 },
    { 152, 175, 310, 244 },
    { 141, 159, 284, 223 } };

static const UInt g_dctInverseQuant_8x2[8][4] = {
    { 16, 18, 33, 26 },
    { 17, 20, 36, 28 },
    { 19, 22, 39, 30 },
    { 21, 23, 42, 33 },
    { 23, 26, 46, 36 },
    { 25, 28, 50, 39 },
    { 27, 30, 55, 43 },
    { 29, 33, 60, 47 } };

static const UInt g_dctForwardQuant_4x2[8][2] = {
    { 356, 741 },
    { 328, 681 },
    { 303, 619 },
    { 282, 568 },
    { 256, 524 },
    { 234, 480 },
    { 216, 437 },
    { 200, 401 } };

static const UInt g_dctInverseQuant_4x2[8][2] = {
    { 23, 46 },
    { 25, 50 },
    { 27, 55 },
    { 29, 60 },
    { 32, 65 },
    { 35, 71 },
    { 38, 78 },
    { 41, 85 } };

static const UInt g_dctForwardQuant_4x1[8][2] = {
    { 512, 1048 },
    { 468, 960 },
    { 431, 873 },
    { 400, 802 },
    { 364, 741 },
    { 334, 675 },
    { 303, 619 },
    { 278, 568 } };

static const UInt g_dctInverseQuant_4x1[8][2] = {
    { 32, 65 },
    { 35, 71 },
    { 38, 78 },
    { 41, 85 },
    { 45, 92 },
    { 49, 101 },
    { 54, 110 },
    { 59, 120 } };
// ==================================================================================================================

// ================================================================================================
// midpoint prediction
// ================================================================================================

const UInt g_mppErrorDiffusionThreshold = 3;
const Int g_mtkMppColorSpaceRdoQpThreshold = 36;
const Int g_mtkMppSubstitutionThreshold = 24;

//                                       LUMA STEP SIZE = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }
const UInt mppStepSizeMapLen = 12;
const static UInt g_mppStepSizeMapCo[mppStepSizeMapLen] = { 1, 3, 4, 5, 6, 7, 7, 7, 8, 9, 10, 11 };
const static UInt g_mppStepSizeMapCg[mppStepSizeMapLen] = { 1, 2, 3, 4, 5, 6, 7, 7, 8, 9, 10, 11 };

const Int g_mppSsmMapping_444[3][16] = {
    { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }
};

const Int g_mppSsmMapping_422[3][16] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 3, 3, 3, 3, 3, 3, 3, 3, -1, -1, -1, -1, -1, -1, -1, -1 }
};

const Int g_mppSsmMapping_420[3][16] = {
    { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2 },
    { 2, 2, 3, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 3, 3, 3, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
};

const Int g_rc_mppMaxQp[g_numChromaFormats][8] = {
    //    64,  80,  96, 112, 128, 144, 160, 192
    { 64, 64, 64, 56, 56, 48, 48, 48 }, // 4:4:4
    { 64, 56, 56, 48, 48, 40, 40, 32 }, // 4:2:2
    { 56, 48, 48, 40, 32, 32, 24, 16 }  // 4:2:0
};
// ================================================================================================

// ================================================================================================
// bitrate lambda, buffer fullness lambda
// ================================================================================================
const Int g_rc_bitrateLambdaLutBits = 6;
const Int g_rc_bitrateLambdaLutBitsInt = 4;
const Int g_rc_bitrateLambdaBits = 12;
const Int g_rc_BFLambdaPrecisionLut = 1;
const Int g_rc_bfLambdaLutBits = 4;
const Int g_rc_bfLambdaLutBitsMod = 6;
// ================================================================================================

// ====================================================================================================================
// entropy coding
// ====================================================================================================================

//! ECG order for transform mode !//
const Int g_ecTransformEcgStart_444[4] = { 4, 1, 9, 0 };
const Int g_ecTransformEcgStart_422[4] = { 0, 1, -1, -1 };
const Int g_ecTransformEcgStart_420[4] = { 0, -1, -1, -1 };

const Int g_transformEcgMappingLastSigPos_444[16][4] = {
    { 0, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 2, 0, 1 }, { 0, 3, 0, 1 },
    { 1, 3, 0, 1 }, { 2, 3, 0, 1 }, { 3, 3, 0, 1 }, { 4, 3, 0, 1 },
    { 5, 3, 0, 1 }, { 5, 3, 1, 1 }, { 5, 3, 2, 1 }, { 5, 3, 3, 1 },
    { 5, 3, 4, 1 }, { 5, 3, 5, 1 }, { 5, 3, 6, 1 }, { 5, 3, 7, 1 }
};

const Int g_transformEcgMappingLastSigPos_422[8][4] = {
    { 1, 0, 0, 0 }, { 1, 1, 0, 0 }, { 1, 2, 0, 0 }, { 1, 3, 0, 0 },
    { 1, 4, 0, 0 }, { 1, 5, 0, 0 }, { 1, 6, 0, 0 }, { 1, 7, 0, 0 }
};

const Int g_transformEcgMappingLastSigPos_420[4][4] = {
    { 1, 0, 0, 0 }, { 2, 0, 0, 0 }, { 3, 0, 0, 0 }, { 4, 0, 0, 0 }
};


//! transform !//
const Int g_ecGroupMapping_Transform_8x2[4] = { 1, 3, 5, 7 };
const Int g_ecGroupMapping_Transform_4x2[2] = { 1, 7 };
const Int g_ecGroupMapping_Transform_4x1[1] = { 4 };
const Int g_ecIndexMapping_Transform_8x2[16] = { 0, 1, 2, 4, 5, 9, 10, 11, 3, 6, 7, 8, 12, 13, 14, 15 };
const Int g_ecIndexMapping_Transform_4x2[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
const Int g_ecIndexMapping_Transform_4x1[4] = { 0, 1, 2, 3 };

//! BP !//
const Int g_ecGroupMapping_BP_8x2[4] = { 4, 4, 4, 4 };
const Int g_ecGroupMapping_BP_4x2[2] = { 4, 4 };
const Int g_ecGroupMapping_BP_4x1[1] = { 4 };
const Int g_ecIndexMapping_BP_8x2[16] = { 0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15 };
const Int g_ecIndexMapping_BP_4x2[8] = { 0, 1, 4, 5, 2, 3, 6, 7 };
const Int g_ecIndexMapping_BP_4x1[4] = { 0, 1, 2, 3 };
// ==================================================================================================================

// ==================================================================================================================
// vector entropy coding (VEC)
// ==================================================================================================================
//! general parameters for QCOM VEC EC (both 2sC and S/M representations)
const UInt g_qcomVectorEcThreshold = 2;
const Int g_qcomVectorEcBestGr_Y[g_qcomVectorEcThreshold] = { 1, 5 };
const Int g_qcomVectorEcBestGr_C[g_qcomVectorEcThreshold] = { 1, 5 };
const Int g_mtkQcomVectorEcThreshold = 2;
const Int g_mtkQcomVectorEcBestGr_Y[g_mtkQcomVectorEcThreshold] = { 2, 5 };
const Int g_mtkQcomVectorEcBestGr_C[g_mtkQcomVectorEcThreshold] = { 2, 5 };

const Int g_vec_sm_bitsReq_1_luma[16] = { 15, 1, 0, 6, 3, 5, 8, 10, 2, 7, 4, 11, 9, 13, 12, 14 };
const Int g_vec_sm_bitsReq_1_chroma[16] = { 15, 1, 0, 4, 2, 7, 8, 11, 3, 9, 6, 10, 5, 12, 13, 14 };
const Int g_vec_sm_bitsReq_2_luma[256] = { 255, 247, 1, 19, 251, 243, 4, 40, 0, 5, 34, 84, 18, 39, 82, 120, 253, 245, 6, 42, 249, 241, 16, 62, 11, 20, 56, 102, 46, 65, 109, 154, 3, 9, 35, 85, 13, 24, 55, 100, 44, 61, 103, 156, 97, 125, 171, 193, 29, 45, 88, 135, 52, 76, 113, 153, 99, 126, 167, 203, 151, 180, 204, 223, 254, 246, 10, 47, 250, 242, 21, 64, 7, 17, 54, 110, 41, 60, 106, 150, 252, 244, 23, 63, 248, 240, 32, 89, 22, 33, 67, 136, 66, 86, 139, 174, 15, 27, 58, 115, 31, 38, 75, 131, 72, 87, 111, 165, 128, 145, 187, 201, 50, 74, 112, 157, 80, 92, 144, 158, 133, 143, 178, 213, 179, 192, 211, 231, 2, 12, 48, 96, 8, 25, 68, 134, 36, 57, 101, 163, 90, 108, 159, 197, 14, 30, 71, 132, 26, 37, 83, 141, 59, 70, 117, 172, 114, 140, 170, 216, 43, 73, 105, 160, 69, 81, 116, 188, 107, 119, 123, 198, 176, 186, 195, 225, 94, 122, 161, 202, 138, 146, 182, 210, 175, 185, 194, 221, 219, 220, 233, 236, 28, 53, 98, 149, 49, 77, 121, 169, 91, 104, 164, 199, 130, 152, 196, 224, 51, 79, 124, 181, 78, 93, 148, 191, 118, 127, 189, 217, 155, 162, 208, 226, 95, 137, 177, 218, 129, 142, 190, 215, 166, 183, 200, 229, 206, 209, 227, 235, 147, 173, 207, 234, 168, 184, 205, 228, 212, 214, 222, 239, 232, 230, 237, 238 };
const Int g_vec_sm_bitsReq_2_chroma[256] = { 255, 247, 0, 30, 251, 243, 4, 43, 1, 5, 12, 64, 29, 45, 66, 48, 253, 245, 6, 46, 249, 241, 16, 67, 13, 20, 44, 108, 63, 79, 109, 124, 3, 10, 34, 85, 17, 27, 59, 112, 61, 68, 114, 166, 140, 146, 180, 205, 40, 54, 95, 107, 72, 89, 126, 164, 137, 144, 178, 213, 189, 197, 220, 236, 254, 246, 14, 62, 250, 242, 21, 77, 7, 18, 42, 113, 47, 71, 110, 134, 252, 244, 23, 80, 248, 240, 26, 87, 24, 28, 38, 106, 83, 93, 102, 94, 8, 22, 55, 119, 32, 35, 50, 122, 74, 82, 99, 157, 145, 149, 171, 188, 49, 86, 121, 168, 97, 100, 133, 154, 155, 158, 175, 198, 210, 203, 215, 228, 2, 15, 56, 138, 11, 31, 69, 151, 37, 60, 111, 184, 96, 117, 161, 206, 9, 33, 78, 148, 25, 36, 76, 153, 65, 51, 98, 170, 132, 128, 142, 191, 19, 53, 120, 196, 57, 41, 101, 183, 118, 103, 81, 169, 199, 174, 181, 147, 84, 123, 185, 208, 135, 127, 162, 204, 194, 186, 173, 200, 231, 219, 227, 223, 39, 70, 136, 182, 52, 88, 152, 207, 92, 129, 176, 224, 115, 172, 211, 235, 58, 90, 143, 195, 91, 105, 156, 216, 131, 139, 177, 221, 165, 163, 217, 238, 75, 125, 192, 234, 130, 116, 179, 226, 187, 160, 190, 229, 230, 212, 201, 214, 73, 150, 225, 233, 141, 104, 193, 237, 209, 202, 159, 218, 239, 232, 222, 167 };

const Int g_vec_sm_bitsReq_1_luma_inv[16] = { 2, 1, 8, 4, 10, 5, 3, 9, 6, 12, 7, 11, 14, 13, 15, 0 };
const Int g_vec_sm_bitsReq_1_chroma_inv[16] = { 2, 1, 4, 8, 3, 12, 10, 5, 6, 9, 11, 7, 13, 14, 15, 0 };
const Int g_vec_sm_bitsReq_2_luma_inv[256] = { 8, 2, 128, 32, 6, 9, 18, 72, 132, 33, 66, 24, 129, 36, 144, 96, 22, 73, 12, 3, 25, 70, 88, 82, 37, 133, 148, 97, 192, 48, 145, 100, 86, 89, 10, 34, 136, 149, 101, 13, 7, 76, 19, 160, 40, 49, 28, 67, 130, 196, 112, 208, 52, 193, 74, 38, 26, 137, 98, 152, 77, 41, 23, 83, 71, 29, 92, 90, 134, 164, 153, 146, 104, 161, 113, 102, 53, 197, 212, 209, 116, 165, 14, 150, 11, 35, 93, 105, 50, 87, 140, 200, 117, 213, 176, 224, 131, 44, 194, 56, 39, 138, 27, 42, 201, 162, 78, 168, 141, 30, 75, 106, 114, 54, 156, 99, 166, 154, 216, 169, 15, 198, 177, 170, 210, 45, 57, 217, 108, 228, 204, 103, 147, 120, 135, 51, 91, 225, 180, 94, 157, 151, 229, 121, 118, 109, 181, 240, 214, 195, 79, 60, 205, 55, 31, 220, 43, 115, 119, 142, 163, 178, 221, 139, 202, 107, 232, 58, 244, 199, 158, 46, 155, 241, 95, 184, 172, 226, 122, 124, 61, 211, 182, 233, 245, 185, 173, 110, 167, 218, 230, 215, 125, 47, 186, 174, 206, 143, 171, 203, 234, 111, 179, 59, 62, 246, 236, 242, 222, 237, 183, 126, 248, 123, 249, 231, 159, 219, 227, 188, 189, 187, 250, 63, 207, 175, 223, 238, 247, 235, 253, 127, 252, 190, 243, 239, 191, 254, 255, 251, 85, 21, 69, 5, 81, 17, 65, 1, 84, 20, 68, 4, 80, 16, 64, 0 };
const Int g_vec_sm_bitsReq_2_chroma_inv[256] = { 2, 8, 128, 32, 6, 9, 18, 72, 96, 144, 33, 132, 10, 24, 66, 129, 22, 36, 73, 160, 25, 70, 97, 82, 88, 148, 86, 37, 89, 12, 3, 133, 100, 145, 34, 101, 149, 136, 90, 192, 48, 165, 74, 7, 26, 13, 19, 76, 15, 112, 102, 153, 196, 161, 49, 98, 130, 164, 208, 38, 137, 40, 67, 28, 11, 152, 14, 23, 41, 134, 193, 77, 52, 240, 104, 224, 150, 71, 146, 29, 83, 170, 105, 92, 176, 35, 113, 87, 197, 53, 209, 212, 200, 93, 95, 50, 140, 116, 154, 106, 117, 166, 94, 169, 245, 213, 91, 51, 27, 30, 78, 138, 39, 75, 42, 204, 229, 141, 168, 99, 162, 114, 103, 177, 31, 225, 54, 181, 157, 201, 228, 216, 156, 118, 79, 180, 194, 56, 131, 217, 44, 244, 158, 210, 57, 108, 45, 175, 147, 109, 241, 135, 198, 151, 119, 120, 214, 107, 121, 250, 233, 142, 182, 221, 55, 220, 43, 255, 115, 171, 155, 110, 205, 186, 173, 122, 202, 218, 58, 230, 46, 174, 195, 167, 139, 178, 185, 232, 111, 60, 234, 159, 226, 246, 184, 211, 163, 61, 123, 172, 187, 238, 249, 125, 183, 47, 143, 199, 179, 248, 124, 206, 237, 59, 239, 126, 215, 222, 251, 189, 62, 219, 254, 191, 203, 242, 231, 190, 127, 235, 236, 188, 253, 243, 227, 207, 63, 247, 223, 252, 85, 21, 69, 5, 81, 17, 65, 1, 84, 20, 68, 4, 80, 16, 64, 0 };

const Int g_vec_2c_bitsReq_1_luma[16] = { 15, 1, 0, 4, 3, 5, 9, 10, 2, 8, 6, 11, 7, 13, 12, 14 };
const Int g_vec_2c_bitsReq_1_chroma[16] = { 15, 1, 0, 4, 2, 6, 8, 11, 3, 9, 7, 12, 5, 13, 14, 10 };
const Int g_vec_2c_bitsReq_2_luma[256] = { 255, 0, 21, 247, 1, 6, 71, 18, 20, 70, 79, 42, 251, 19, 31, 243, 3, 5, 67, 15, 9, 27, 127, 36, 69, 121, 160, 94, 13, 39, 97, 32, 24, 66, 80, 55, 65, 110, 179, 135, 140, 196, 209, 174, 74, 129, 162, 98, 253, 14, 33, 245, 12, 34, 143, 57, 75, 132, 154, 109, 249, 54, 85, 241, 2, 8, 52, 11, 4, 26, 116, 30, 60, 93, 164, 102, 17, 58, 91, 38, 7, 29, 120, 41, 28, 72, 171, 96, 111, 184, 206, 157, 37, 107, 161, 83, 73, 100, 180, 128, 136, 181, 218, 176, 183, 223, 237, 235, 145, 159, 228, 201, 23, 50, 95, 46, 49, 99, 165, 87, 131, 158, 231, 188, 56, 89, 190, 112, 25, 64, 146, 78, 63, 106, 186, 134, 81, 177, 210, 169, 51, 125, 197, 139, 76, 141, 198, 137, 117, 185, 222, 173, 195, 213, 236, 227, 142, 178, 232, 207, 86, 155, 216, 163, 170, 208, 238, 221, 215, 239, 217, 229, 168, 230, 226, 205, 61, 104, 191, 124, 113, 166, 233, 193, 153, 224, 214, 194, 92, 189, 211, 152, 254, 10, 77, 246, 16, 43, 144, 62, 45, 133, 149, 84, 250, 48, 105, 242, 22, 44, 130, 68, 47, 115, 175, 88, 126, 172, 225, 200, 59, 90, 192, 119, 53, 114, 167, 103, 101, 156, 219, 187, 182, 234, 220, 212, 122, 204, 202, 151, 252, 35, 118, 244, 40, 82, 199, 123, 108, 203, 150, 148, 248, 138, 147, 240 };
const Int g_vec_2c_bitsReq_2_chroma[256] = { 255, 1, 22, 247, 0, 4, 64, 16, 23, 75, 31, 33, 251, 14, 32, 243, 3, 6, 79, 20, 9, 27, 169, 68, 62, 150, 143, 100, 12, 43, 117, 41, 25, 85, 74, 55, 63, 135, 179, 114, 160, 221, 203, 197, 82, 172, 142, 103, 253, 19, 38, 245, 11, 35, 125, 53, 71, 165, 108, 95, 249, 73, 81, 241, 2, 8, 61, 10, 7, 28, 126, 49, 86, 168, 144, 127, 15, 59, 96, 34, 5, 29, 137, 44, 30, 26, 208, 128, 140, 196, 133, 154, 46, 112, 155, 57, 78, 157, 182, 132, 146, 204, 205, 190, 220, 226, 238, 235, 166, 214, 232, 207, 17, 60, 111, 50, 52, 116, 178, 90, 164, 216, 222, 211, 67, 153, 199, 122, 24, 65, 162, 84, 83, 131, 217, 163, 80, 181, 195, 156, 54, 118, 193, 106, 77, 141, 223, 175, 167, 201, 227, 215, 185, 200, 237, 225, 145, 183, 233, 213, 47, 151, 202, 138, 158, 134, 239, 228, 212, 236, 115, 174, 130, 229, 177, 92, 37, 104, 189, 110, 129, 159, 234, 206, 152, 231, 180, 147, 91, 194, 187, 102, 254, 13, 72, 246, 21, 45, 171, 70, 42, 136, 109, 76, 250, 51, 97, 242, 18, 48, 173, 66, 69, 121, 218, 149, 124, 176, 219, 186, 56, 89, 210, 113, 36, 120, 148, 88, 101, 161, 224, 188, 191, 230, 170, 184, 105, 192, 139, 99, 252, 39, 107, 244, 40, 58, 209, 119, 98, 198, 87, 94, 248, 123, 93, 240 };

const Int g_vec_2c_bitsReq_1_luma_inv[16] = { 2, 1, 8, 4, 3, 5, 10, 12, 9, 6, 7, 11, 14, 13, 15, 0 };
const Int g_vec_2c_bitsReq_1_chroma_inv[16] = { 2, 1, 4, 8, 3, 12, 5, 10, 6, 9, 15, 7, 11, 13, 14, 0 };
const Int g_vec_2c_bitsReq_2_luma_inv[256] = { 1, 4, 64, 16, 68, 17, 5, 80, 65, 20, 193, 67, 52, 28, 49, 19, 196, 76, 7, 13, 8, 2, 208, 112, 32, 128, 69, 21, 84, 81, 71, 14, 31, 50, 53, 241, 23, 92, 79, 29, 244, 83, 11, 197, 209, 200, 115, 212, 205, 116, 113, 140, 66, 224, 61, 35, 124, 55, 77, 220, 72, 176, 199, 132, 129, 36, 33, 18, 211, 24, 9, 6, 85, 96, 44, 56, 144, 194, 131, 10, 34, 136, 245, 95, 203, 62, 160, 119, 215, 125, 221, 78, 188, 73, 27, 114, 87, 30, 47, 117, 97, 228, 75, 227, 177, 206, 133, 93, 248, 59, 37, 88, 127, 180, 225, 213, 70, 148, 242, 223, 82, 25, 236, 247, 179, 141, 216, 22, 99, 45, 210, 120, 57, 201, 135, 39, 100, 147, 253, 143, 40, 145, 156, 54, 198, 108, 130, 254, 251, 202, 250, 239, 191, 184, 58, 161, 229, 91, 121, 109, 26, 94, 46, 163, 74, 118, 181, 226, 172, 139, 164, 86, 217, 151, 43, 214, 103, 137, 157, 38, 98, 101, 232, 104, 89, 149, 134, 231, 123, 189, 126, 178, 222, 183, 187, 152, 41, 142, 146, 246, 219, 111, 238, 249, 237, 175, 90, 159, 165, 42, 138, 190, 235, 153, 186, 168, 162, 170, 102, 230, 234, 167, 150, 105, 185, 218, 174, 155, 110, 171, 173, 122, 158, 182, 233, 107, 154, 106, 166, 169, 255, 63, 207, 15, 243, 51, 195, 3, 252, 60, 204, 12, 240, 48, 192, 0 };
const Int g_vec_2c_bitsReq_2_chroma_inv[256] = { 4, 1, 64, 16, 5, 80, 17, 68, 65, 20, 67, 52, 28, 193, 13, 76, 7, 112, 208, 49, 19, 196, 2, 8, 128, 32, 85, 21, 69, 81, 84, 10, 14, 11, 79, 53, 224, 176, 50, 241, 244, 31, 200, 29, 83, 197, 92, 160, 209, 71, 115, 205, 116, 55, 140, 35, 220, 95, 245, 77, 113, 66, 24, 36, 6, 129, 211, 124, 23, 212, 199, 56, 194, 61, 34, 9, 203, 144, 96, 18, 136, 62, 44, 132, 131, 33, 72, 250, 227, 221, 119, 188, 175, 254, 251, 59, 78, 206, 248, 239, 27, 228, 191, 47, 177, 236, 143, 242, 58, 202, 179, 114, 93, 223, 39, 170, 117, 30, 141, 247, 225, 213, 127, 253, 216, 54, 70, 75, 87, 180, 172, 133, 99, 90, 165, 37, 201, 82, 163, 238, 88, 145, 46, 26, 74, 156, 100, 187, 226, 215, 25, 161, 184, 125, 91, 94, 139, 97, 164, 181, 40, 229, 130, 135, 120, 57, 108, 148, 73, 22, 234, 198, 45, 210, 171, 147, 217, 174, 118, 38, 186, 137, 98, 157, 235, 152, 219, 190, 231, 178, 103, 232, 237, 142, 189, 138, 89, 43, 249, 126, 153, 149, 162, 42, 101, 102, 183, 111, 86, 246, 222, 123, 168, 159, 109, 151, 121, 134, 214, 218, 104, 41, 122, 146, 230, 155, 105, 150, 167, 173, 233, 185, 110, 158, 182, 107, 169, 154, 106, 166, 255, 63, 207, 15, 243, 51, 195, 3, 252, 60, 204, 12, 240, 48, 192, 0 };
// ==================================================================================================================

#endif // __TYPEDEF
