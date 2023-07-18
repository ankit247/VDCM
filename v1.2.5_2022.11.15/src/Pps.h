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
\file       Pps.h
\brief      Class definition for picture parameter set (PPS)
*/

#ifndef __PPS_H__
#define __PPS_H__

#include "TypeDef.h"
#include "Bitstream.h"

class Pps {
protected:
    Bool m_isEncoder;

    //! picture properties !//
    Int m_blockWidth;
    Int m_blockHeight;
    Int m_srcWidth;
    Int m_srcHeight;
    Int m_sliceHeight;
    Int m_sliceWidth;
	UInt m_slicesPerLine;
	UInt m_slicePadX;
	UInt m_mppMinStepSize;
	UInt m_sliceNumPx;
	UInt64 m_sliceNumBits;
    Int m_bitDepth;
    Int m_bpp;
    ColorSpace m_srcCsc;
    ChromaFormat m_chroma;

    //! rate control !//
    Int m_rcTargetRateScale;
    Int m_rcTargetRateThreshold;
    UInt m_rcTargetRateExtraFbls;
    Int m_rcBufferFullnessScale;
    Int m_rcLambdaBitrateScale;
    Int m_blockMaxBits;
    Int m_rcInitTxDelay;
    Int m_rcBufferInitSize;
    Int m_rcBufferMinSize;
    UInt m_rcBufferMaxSize;
    Int m_rcStuffingBits;
    UInt m_rcFullnessOffsetSlope;
    Int m_rcFullnessOffsetThreshold;

    //! slice multiplexing !//
    UInt m_chunkSize;
	UInt m_chunkAdjBits;

    //! substream multiplexing !//
    Int m_ssmMaxSeSize;
	UInt m_numExtraMuxBits;

    //! flatness QP LUT, very flat (FBLS/NFBLS), somewhat flat (FBLS/NFBLS) !//
    UInt m_flatnessQpLutBits;
    UInt m_flatnessQpLutSize;
	Int* m_flatnessQpLut;
	Int m_flatnessQpVeryFlatFbls;
	Int m_flatnessQpVeryFlatNfbls;
	Int m_flatnessQpSomewhatFlatFbls;
	Int m_flatnessQpSomewhatFlatNfbls;

    //! target rate delta LUT !//
    UInt m_targetRateLutBits;
    UInt m_targetRateLutSize;
    UInt* m_targetRateDeltaLut;

    //! lambda LUTs !//
    UInt m_lambdaBitrateLutBits;
    UInt m_lambdaBitrateLutSize;
    UInt m_lambdaBufferFullnessLutBits;
    UInt m_lambdaBufferFullnessLutSize;
    UInt* m_lambdaBitrateLut;
    UInt* m_lambdaBufferFullnessLut;

    //! max QP LUT !//
    UInt m_maxQpLutSize;
    UInt* m_maxQpLut;

    //! MPPF bits/component !//
    UInt* m_mppfBitsPerCompA;
    UInt* m_mppfBitsPerCompB;

    //! PPS bitstream !//
    std::vector<uint8_t> m_ppsBitBuffer;

    //! versioning !//
    UInt m_versionMajor;
    UInt m_versionMinor;
    Char m_versionRelease;

    UInt m_ppsIdentifier;

public:
    Pps ();
    Pps (Int sourceWidth, Int sourceHeight, Int sliceHeight, Int slicesPerLine, Int bpp, Int bitDepth, ColorSpace csc, ChromaFormat cf);
    ~Pps ();

    // methods
    Void InitPps ();
    Void InitMemoryPpsLuts ();
    Void LoadFromConfigFileData (ConfigFileData* configFileData, Bool isEncoder);
    Void GeneratePpsBitstream ();
    Void ParsePpsBitstream (std::fstream* in);
    Void WritePpsBitstream (std::fstream* out);
    Void DumpPpsBytes ();
    Void DumpPpsFields ();
    Void InitDecoder ();
    Void CheckVersionInfo ();

    // setters
    Void SetSsmMaxSeSize (Int x) { m_ssmMaxSeSize = x; }
	Void SetSlicesPerLine (UInt x) { m_slicesPerLine = x; }
	Void SetSlicePadX (UInt x) { m_slicePadX = x; }

    // getters
    Int GetBpp () { return m_bpp; }
    Int GetSrcWidth () { return m_srcWidth; }
    Int GetSrcHeight () { return m_srcHeight; }
    Int GetBitDepth () { return m_bitDepth; }
    ColorSpace GetSrcCsc () { return m_srcCsc; }
    ChromaFormat GetChromaFormat () { return m_chroma; }
    Int GetChunkSize () { return m_chunkSize; }
    Int GetRcTargetRateScale () { return m_rcTargetRateScale; }
    Int GetRcTargetRateThreshold () { return m_rcTargetRateThreshold; }
    Int GetRcFullnessOffsetSlope () { return m_rcFullnessOffsetSlope; }
    Int GetRcFullnessOffsetThreshold () { return m_rcFullnessOffsetThreshold; }
    Int GetRcBufferFullnessScale () { return m_rcBufferFullnessScale; }
    Int GetRcLambdaBitrateScale () { return m_rcLambdaBitrateScale; }
    Int GetRcInitTxDelay () { return m_rcInitTxDelay; }
    Int GetRcBufferInitSize () { return m_rcBufferInitSize; }
    Int GetRcBufferMaxSize () { return m_rcBufferMaxSize; }
    Int GetRcStuffingBits () { return m_rcStuffingBits; }
    UInt GetLutLambdaBufferFullness (UInt idx) { return m_lambdaBufferFullnessLut[idx]; }
    UInt GetLutLambdaBitrate (UInt idx) { return m_lambdaBitrateLut[idx]; }
    UInt GetLutTargetRateDelta (UInt idx) { return m_targetRateDeltaLut[idx]; }
    
    UInt GetLutMaxQp (UInt idx) { return m_maxQpLut[idx]; }
    UInt GetMppfBitsPerCompA (UInt k) { return m_mppfBitsPerCompA[k]; }
    UInt GetMppfBitsPerCompB (UInt k) { return m_mppfBitsPerCompB[k]; }
    UInt GetSliceWidth () { return m_sliceWidth; }
    UInt GetSliceHeight () { return m_sliceHeight; }

	Int GetLutFlatnessQp (UInt idx) { return m_flatnessQpLut[idx]; }
	Int GetFlatnessQpVeryFlat (Bool isFls) { return (isFls ? m_flatnessQpVeryFlatFbls : m_flatnessQpVeryFlatNfbls); }
	Int GetFlatnessQpSomewhatFlat (Bool isFls) { return (isFls ? m_flatnessQpSomewhatFlatFbls : m_flatnessQpSomewhatFlatNfbls); }
    UInt GetRcTargetRateExtraFbls () { return m_rcTargetRateExtraFbls; }
    UInt GetSsmMaxSeSize () { return m_ssmMaxSeSize; }
    UInt MapCodeBpc (UInt code);
    ChromaFormat MapCodeChroma (UInt code);
    ColorSpace MapCodeCsc (UInt code);
    UInt GetCodeBpc ();
    UInt GetCodeCsc ();
    UInt GetCodeChroma ();
    UInt GetSliceNumPx () { return m_sliceNumPx; }
    UInt GetVersionMajor () { return m_versionMajor; }
    UInt GetVersionMinor () { return m_versionMinor; }
    Char GetVersionRelease () { return m_versionRelease; }
	UInt GetNumExtraMuxBits () { return m_numExtraMuxBits; }
	UInt GetChunkAdjBits () { return m_chunkAdjBits; }
	UInt64 GetSliceNumBits () { return m_sliceNumBits; }
	UInt GetSlicesPerLine () { return m_slicesPerLine; }
	UInt GetSlicePadX () { return m_slicePadX; }
	UInt GetMppMinStepSize () { return m_mppMinStepSize; }
};

#endif //__PPS_H__