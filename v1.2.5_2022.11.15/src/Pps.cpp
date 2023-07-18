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
\file       Pps.cpp
\brief      Picture parameter set class definitions
*/

#include "Pps.h"
#include "Misc.h"
#include <cstring>

/**
    PPS constructor for encoder

    @param sourceWidth picture width
    @param sourceHeight picture height
    @param sliceHeight slice height
    @param slicesPerLine slices per line
    @param bpp compressed bitrate
    @param bitDepth source bitdepth
    @param csc source colorspace
    @param cf chroma-sampling format
    */
Pps::Pps (Int sourceWidth, Int sourceHeight, Int sliceWidth, Int sliceHeight, Int bpp, Int bitDepth, ColorSpace csc, ChromaFormat cf)
    : m_srcWidth (sourceWidth)
    , m_srcHeight (sourceHeight)
    , m_sliceHeight (sliceHeight)
    , m_ppsIdentifier (0)
    , m_sliceWidth (sliceWidth)
    , m_versionRelease (g_codecVersionRelease)
    , m_sliceNumPx (sliceWidth * sliceHeight)
    , m_blockWidth (8)
    , m_blockHeight (2)
    , m_bpp (bpp)
    , m_bitDepth (bitDepth)
    , m_srcCsc (csc)
    , m_chroma (cf)
{
    m_isEncoder = true;
}

/**
    PPS constructor for decoder
    */
Pps::Pps () {
    m_isEncoder = false;
}

/**
    Destructor
    */
Pps::~Pps () {
    delete[] m_flatnessQpLut;
    delete[] m_targetRateDeltaLut;
    delete[] m_lambdaBitrateLut;
    delete[] m_lambdaBufferFullnessLut;
    delete[] m_maxQpLut;
    delete[] m_mppfBitsPerCompA;
    delete[] m_mppfBitsPerCompB;
}

/**
    Initialize PPS params
    */
Void Pps::InitPps () {
    Int sliceNumPx = m_sliceWidth * m_sliceHeight;
    Int blockNumPx = 16;
    Int avgBlockBits = m_bpp;
    UInt sliceNumBlocklines = m_sliceHeight >> 1;

    //! warning for ssm_max_se_size < 128 added in test model v1.2.3 !//
    if (m_ssmMaxSeSize < 128)
        printf("WARNING: PPS paramter ssm_max_se_size should be 128 bits or greater\nSee Section D of the written spec for more details.\n");

    //! maximum block size !//
    m_blockMaxBits = g_maxModeHeaderLen + g_maxFlatHeaderLen;
    for (Int k = 0; k < g_numComps; k++) {
        m_blockMaxBits += m_bitDepth * (blockNumPx >> (g_compScaleX[k][m_chroma] + g_compScaleY[k][m_chroma]));
    }

    //! HRD size, initial transmission delay !//
    if (m_sliceWidth <= 720) {
        m_rcBufferInitSize = 4096;
    }
    else if (m_sliceWidth <= 2048) {
        m_rcBufferInitSize = 8192;
    }
    else {
        m_rcBufferInitSize = 10752;
    }

    Int initTxDelay = (Int)((Double)m_rcBufferInitSize / avgBlockBits);
    m_rcBufferInitSize = (initTxDelay * blockNumPx * m_bpp) >> g_rc_bppFractionalBits;
    m_rcInitTxDelay = (Int)((Double)m_rcBufferInitSize / avgBlockBits);
    m_rcBufferMinSize = 0;
    m_rcBufferMaxSize = (2 * m_rcBufferInitSize) + (2 * m_sliceWidth * m_rcTargetRateExtraFbls);

    //! scaling factor for computing target rate !//
	m_rcTargetRateScale = SlowLog2 (sliceNumPx) + 1;
    m_rcTargetRateThreshold = 1 << (m_rcTargetRateScale - 1);

    //! scaling factor for buffer fullness
    m_rcBufferFullnessScale = (1 << (g_rc_BFScaleApprox + g_rc_BFRangeBits)) / m_rcBufferMaxSize;

    //! scaling factor for computing bitrate lambda !//
    m_rcLambdaBitrateScale = ((1 << g_rc_bitrateLambdaBits) + (m_blockMaxBits >> 1)) / m_blockMaxBits;

    //! slope/threshold for decreasing rate buffer fullness through slice !//
    if (m_rcFullnessOffsetThreshold < 3 || m_rcFullnessOffsetThreshold >= (Int)sliceNumBlocklines) {
        fprintf (stderr, "Pps::InitPps(): rc_fullness_offset_threshold must be in range [3, (sliceHeight / 2) - 1].\n");
		fprintf (stderr, "Pps::InitPps(): setting rc_fullness_offset_threshold to %d.\n", sliceNumBlocklines - 1);
        m_rcFullnessOffsetThreshold = sliceNumBlocklines - 1;
    }

    UInt rampBlocks = m_rcFullnessOffsetThreshold * (m_sliceWidth >> 3);
    UInt64 rampBits = ((UInt64)(m_rcBufferMaxSize - m_rcBufferInitSize)) << g_rc_bufferOffsetPrecision;
    m_rcFullnessOffsetSlope = (UInt)(rampBits / rampBlocks);
   
    //! chunk size 
	Int chunkBits = (m_sliceWidth * m_bpp + 15) >> g_rc_bppFractionalBits;
	m_chunkSize = (chunkBits + 7) >> 3; // (bytes)

    //! use fixed size stuffing words to prevent rate buffer underflow !//
	UInt perChunkPadBits = (16 * m_chunkSize) - (((m_sliceWidth << 1) * m_bpp) >> g_rc_bppFractionalBits);
    Int totalPaddingBits = avgBlockBits + perChunkPadBits - 8;
    m_rcStuffingBits = (Int)((totalPaddingBits + 8) / 9);

	m_sliceNumBits = 8 * GetChunkSize () * GetSliceHeight ();

	m_numExtraMuxBits = 4 * (2 * GetSsmMaxSeSize () - 2);
	//! adjust m_numExtraMuxBits such that:
	//  (m_sliceBitsTotal - m_numExtraMuxBits is a multiple of muxWordSize !//
	while ((GetSliceNumBits () - m_numExtraMuxBits) % GetSsmMaxSeSize ()) {
		m_numExtraMuxBits--;
	}

	m_chunkAdjBits = (16 * GetChunkSize ()) - (((GetSliceWidth () << 1) * m_bpp) >> g_rc_bppFractionalBits);
}

/**
    Allocate memory for PPS LUTs
    */
Void Pps::InitMemoryPpsLuts () {
    m_flatnessQpLutBits = g_flatnessQpLutBits;
    m_targetRateLutBits = g_rc_targetRateBits;
    m_lambdaBitrateLutBits = g_rc_bitrateLambdaLutBitsInt;
    m_lambdaBufferFullnessLutBits = g_rc_bfLambdaLutBits;
    m_maxQpLutSize = 1 << g_rc_maxQpLutBits;
    m_flatnessQpLutSize = 1 << g_flatnessQpLutBits;
    m_targetRateLutSize = 1 << g_rc_targetRateBits;
    m_lambdaBitrateLutSize = 1 << g_rc_bitrateLambdaLutBitsInt;
    m_lambdaBufferFullnessLutSize = 1 << g_rc_bfLambdaLutBits;
	m_flatnessQpLut = new Int[m_flatnessQpLutSize];
    m_targetRateDeltaLut = new UInt[m_targetRateLutSize];
    m_lambdaBitrateLut = new UInt[m_lambdaBitrateLutSize];
    m_lambdaBufferFullnessLut = new UInt[m_lambdaBufferFullnessLutSize];
    m_maxQpLut = new UInt[m_maxQpLutSize];
    m_mppfBitsPerCompA = new UInt[g_numComps];
    m_mppfBitsPerCompB = new UInt[g_numComps];
}

/**
    Load data from RC file

    @param rcFileData struct for RC file data
    @param isEncoder flag to indicate encoder or decoder
    */
Void Pps::LoadFromConfigFileData (ConfigFileData* configFileData, Bool isEncoder) {
    //! allocate memory to store config file data !//
    InitMemoryPpsLuts ();

    if (isEncoder) {
        //! allocate memory for lambda LUTs !//
        std::memcpy (m_lambdaBitrateLut, configFileData->lutLambdaBitrate, m_lambdaBitrateLutSize * sizeof (UInt));
        std::memcpy (m_lambdaBufferFullnessLut, configFileData->lutLambdaBF, m_lambdaBufferFullnessLutSize * sizeof (UInt));
    }
    std::memcpy (m_targetRateDeltaLut, configFileData->lutTargetRateDelta, m_targetRateLutSize * sizeof (UInt));
	std::memcpy (m_flatnessQpLut, configFileData->lutFlatnessQp, m_flatnessQpLutSize * sizeof (Int));
    std::memcpy (m_maxQpLut, configFileData->lutMaxQp, m_maxQpLutSize * sizeof (UInt));
    std::memcpy (m_mppfBitsPerCompA, configFileData->mppfBitsPerCompA, g_numComps * sizeof (UInt));
    std::memcpy (m_mppfBitsPerCompB, configFileData->mppfBitsPerCompB, g_numComps * sizeof (UInt));
    m_flatnessQpVeryFlatFbls = configFileData->flatnessQpVeryFlatFbls;
    m_flatnessQpVeryFlatNfbls = configFileData->flatnessQpVeryFlatNfbls;
    m_flatnessQpSomewhatFlatFbls = configFileData->flatnessQpSomewhatFlatFbls;
    m_flatnessQpSomewhatFlatNfbls = configFileData->flatnessQpSomewhatFlatNfbls;
    m_rcFullnessOffsetThreshold = configFileData->rcFullnessOffsetThreshold;
    m_rcTargetRateExtraFbls = configFileData->rcTargetRateExtraFbls;
    m_versionMajor = configFileData->versionMajor;
    m_versionMinor = configFileData->versionMinor;

	if (m_versionMinor >= 2) {
		m_mppMinStepSize = configFileData->mppMinStepSize;
	}

	//! make sure min block rate is less than average block rate !//
	UInt minBlockRate = g_maxModeHeaderLen + g_maxFlatHeaderLen + 1;
	for (UInt k = 0; k < g_numComps; k++) {
		UInt numCompSamples = 16 >> (g_compScaleX[k][m_chroma] + g_compScaleY[k][m_chroma]);
		minBlockRate += (numCompSamples * m_mppfBitsPerCompA[k]);
	}
	if (minBlockRate >= (UInt)m_bpp) {
		fprintf (stderr, "Pps::LoadFromConfigFileData(): minBlockRate (%d) must be strictly less than average block rate (%d).\n",
			minBlockRate, m_bpp);
		exit (EXIT_FAILURE);
	}

	//! make sure YCoCg MPPF bits/sample is less than or equal to RGB MPPF bits/sample !//
	if (m_srcCsc == kRgb) {
		UInt mppfSumA = m_mppfBitsPerCompA[0] + m_mppfBitsPerCompA[1] + m_mppfBitsPerCompA[2];
		UInt mppfSumB = m_mppfBitsPerCompB[0] + m_mppfBitsPerCompB[1] + m_mppfBitsPerCompB[2];
		if (mppfSumB > mppfSumA) {
			fprintf (stderr, "Pps::LoadFromConfigFileData(): sum(mppfBitsPerCompB) = %d bits must be <= sum(mppfBitsPerCompA) = %d bits.\n",
				mppfSumB, mppfSumA);
			exit (EXIT_FAILURE);
		}
	}
}

Void Pps::DumpPpsBytes () {
    fprintf (stdout, "\n");
    fprintf (stdout, "PPS by byte\n");
    fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
    for (UInt line = 0; line < 8; line++) {
        for (UInt byte = 0; byte < 16; byte++) {
            UInt idx = 16 * line + byte;
            if (idx >= m_ppsBitBuffer.size()) {
                break;
            }
            uint8_t curByte = m_ppsBitBuffer.at (idx);
            fprintf (stdout, "%02X ", curByte);
        }
        fprintf (stdout, "\n");
    }
    fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
}

Void Pps::GeneratePpsBitstream () {
    uint8_t tempByte;

    //! versioning !//
    m_ppsBitBuffer.push_back (m_versionMajor & 0x7F);
    m_ppsBitBuffer.push_back (m_versionMinor & 0x7F);
    m_ppsBitBuffer.push_back (m_versionRelease);
    m_ppsBitBuffer.push_back (0); //! pps identifier !//

    //! source dimensions !//
    m_ppsBitBuffer.push_back ((m_srcWidth >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_srcWidth & 0xFF);
    m_ppsBitBuffer.push_back ((m_srcHeight >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_srcHeight & 0xFF);

    //! slice dimensions !//
    m_ppsBitBuffer.push_back ((m_sliceWidth >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_sliceWidth & 0xFF);
    m_ppsBitBuffer.push_back ((m_sliceHeight >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_sliceHeight & 0xFF);

    //! slice num px !//
    m_ppsBitBuffer.push_back ((m_sliceNumPx >> 24) & 0xFF);
    m_ppsBitBuffer.push_back ((m_sliceNumPx >> 16) & 0xFF);
    m_ppsBitBuffer.push_back ((m_sliceNumPx >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_sliceNumPx & 0xFF);

    //! bpp, bpc, csc, chroma format !//
    uint8_t bpc = GetCodeBpc ();
    uint8_t csc = GetCodeCsc ();
    uint8_t cf = GetCodeChroma ();
    tempByte = ((m_bpp >> 8) & 0x3) ;
    m_ppsBitBuffer.push_back (tempByte & 0xFF);

    tempByte = (m_bpp & 0xFF);
    m_ppsBitBuffer.push_back (tempByte & 0xFF);

    tempByte = 0; //! reserved byte !//
    m_ppsBitBuffer.push_back (tempByte & 0xFF);

    tempByte = ((bpc & 0xF) << 4) | ((csc & 0x3) << 2) | (cf & 0x3);
    m_ppsBitBuffer.push_back (tempByte & 0xFF);

    //! chunk size !//
    m_ppsBitBuffer.push_back (0); //! reserved !//
    m_ppsBitBuffer.push_back (0); //! reserved !//
    m_ppsBitBuffer.push_back ((m_chunkSize >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_chunkSize & 0xFF);

    //! rc buffer init size !//
    m_ppsBitBuffer.push_back (0); //! reserved !//
    m_ppsBitBuffer.push_back (0); //! reserved !//
    m_ppsBitBuffer.push_back ((m_rcBufferInitSize >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_rcBufferInitSize & 0xFF);

    //! rc stuffing bits !//
    m_ppsBitBuffer.push_back (m_rcStuffingBits & 0xFF);

    //! rc init tx delay !//
    m_ppsBitBuffer.push_back (m_rcInitTxDelay & 0xFF);

    //! rc buffer max size !//
    m_ppsBitBuffer.push_back ((m_rcBufferMaxSize >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_rcBufferMaxSize & 0xFF);

    //! rc target rate threshold !//
    m_ppsBitBuffer.push_back ((m_rcTargetRateThreshold >> 24) & 0xFF);
    m_ppsBitBuffer.push_back ((m_rcTargetRateThreshold >> 16) & 0xFF);
    m_ppsBitBuffer.push_back ((m_rcTargetRateThreshold >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_rcTargetRateThreshold & 0xFF);

    //! rc target rate scale !//
    m_ppsBitBuffer.push_back (m_rcTargetRateScale & 0xFF);

    //! rc fullness scale !//
    m_ppsBitBuffer.push_back (m_rcBufferFullnessScale & 0xFF);

    //! rc fullness offset threshold !//
    m_ppsBitBuffer.push_back ((m_rcFullnessOffsetThreshold >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_rcFullnessOffsetThreshold & 0xFF);

    //! rc fullness offset slope !//
    m_ppsBitBuffer.push_back ((m_rcFullnessOffsetSlope >> 16) & 0xFF);
    m_ppsBitBuffer.push_back ((m_rcFullnessOffsetSlope >> 8) & 0xFF);
    m_ppsBitBuffer.push_back (m_rcFullnessOffsetSlope & 0xFF);

    //! rc target rate extra FBLS !//
    m_ppsBitBuffer.push_back (m_rcTargetRateExtraFbls & 0xF);
    
	m_ppsBitBuffer.push_back ((int8)m_flatnessQpVeryFlatFbls & 0xFF);
	m_ppsBitBuffer.push_back ((int8)m_flatnessQpVeryFlatNfbls & 0xFF);
	m_ppsBitBuffer.push_back ((int8)m_flatnessQpSomewhatFlatFbls & 0xFF);
	m_ppsBitBuffer.push_back ((int8)m_flatnessQpSomewhatFlatNfbls & 0xFF);

    //! flatness QP LUT !//
    for (UInt b = 0; b < 8; b++) {
        m_ppsBitBuffer.push_back (m_flatnessQpLut[b] & 0xFF);
    }

    //! max QP LUT !//
    for (UInt b = 0; b < 8; b++) {
        m_ppsBitBuffer.push_back (m_maxQpLut[b] & 0xFF);
    }

    //! target rate delta LUT !//
    for (UInt b = 0; b < 16; b++) {
        m_ppsBitBuffer.push_back (m_targetRateDeltaLut[b] & 0xFF);
    }

    //! mppf bits per comp !//
    m_ppsBitBuffer.push_back (0); //! reserved !//
    tempByte = (m_mppfBitsPerCompA[0] << 4) | (m_mppfBitsPerCompA[1]);
    m_ppsBitBuffer.push_back (tempByte & 0xFF);
    tempByte = (m_mppfBitsPerCompA[2] << 4) | (m_mppfBitsPerCompB[0]);
    m_ppsBitBuffer.push_back (tempByte & 0xFF);
    tempByte = (m_mppfBitsPerCompB[1] << 4) | (m_mppfBitsPerCompB[2]);
    m_ppsBitBuffer.push_back (tempByte & 0xFF);

    //! ssm max se size !//
    m_ppsBitBuffer.push_back (0); //! reserved !//
    m_ppsBitBuffer.push_back (0); //! reserved !//
    m_ppsBitBuffer.push_back (0); //! reserved !//
    m_ppsBitBuffer.push_back (m_ssmMaxSeSize & 0xFF);

	m_ppsBitBuffer.push_back (0); //! reserved !//
	m_ppsBitBuffer.push_back (0); //! reserved !//
	m_ppsBitBuffer.push_back (0); //! reserved !//
	m_ppsBitBuffer.push_back ((m_sliceNumBits >> 32) & 0xFF);
	m_ppsBitBuffer.push_back ((m_sliceNumBits >> 24) & 0xFF);
	m_ppsBitBuffer.push_back ((m_sliceNumBits >> 16) & 0xFF);
	m_ppsBitBuffer.push_back ((m_sliceNumBits >> 8) & 0xFF);
	m_ppsBitBuffer.push_back (m_sliceNumBits & 0xFF);

	m_ppsBitBuffer.push_back (0); //! reserved !//
	m_ppsBitBuffer.push_back (m_chunkAdjBits);
	m_ppsBitBuffer.push_back ((m_numExtraMuxBits >> 8) & 0xFF);
	m_ppsBitBuffer.push_back (m_numExtraMuxBits & 0xFF);
	
	//! additional slice information for v1.2 !//
	if (m_versionMinor >= 2) {
		m_ppsBitBuffer.push_back ((m_slicesPerLine >> 8) & 0x3);
		m_ppsBitBuffer.push_back (m_slicesPerLine & 0xFF);
		m_ppsBitBuffer.push_back (m_slicePadX & 0x7);
		m_ppsBitBuffer.push_back (m_mppMinStepSize & 0xF);		
	}

	//! fill remainder of PPS with zeros !//
    while (m_ppsBitBuffer.size () < 128) {
        m_ppsBitBuffer.push_back (0);
    }
}

Void Pps::WritePpsBitstream (std::fstream* out) {
    for (UInt idx = 0; idx < 128; idx++) {
        out->put (m_ppsBitBuffer.at (idx));
    }
}

Void Pps::ParsePpsBitstream (std::fstream* in) {
    Char temp;
    for (UInt idx = 0; idx < 128; idx++) {
        in->read (&temp, 1);
        m_ppsBitBuffer.push_back (temp);
    }
}

Void Pps::DumpPpsFields () {
    fprintf (stdout, "\n");
    fprintf (stdout, "PPS by field\n");
    fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");

    //! picture parameters !//
    fprintf (stdout, "%-40s: %d\n", "version_major", m_versionMajor);
    fprintf (stdout, "%-40s: %d\n", "version_minor", m_versionMinor);
    fprintf (stdout, "%-40s: %c\n", "version_release", m_versionRelease);
    fprintf (stdout, "%-40s: %d\n", "pps_identifier", 0);
    fprintf (stdout, "%-40s: %d\n", "frame_width", m_srcWidth);
    fprintf (stdout, "%-40s: %d\n", "frame_height", m_srcHeight);
    fprintf (stdout, "%-40s: %d\n", "slice_width", m_sliceWidth);
    fprintf (stdout, "%-40s: %d\n", "slice_height", m_sliceHeight);
    fprintf (stdout, "%-40s: %d\n", "slice_num_px", m_sliceNumPx);
	if (m_versionMinor >= 2) {
		fprintf (stdout, "%-40s: %d\n", "slices_per_line", m_slicesPerLine);
		fprintf (stdout, "%-40s: %d\n", "slice_pad_x", m_slicePadX);
	}
    fprintf (stdout, "%-40s: %d\n", "bits_per_component", GetCodeBpc ());
    fprintf (stdout, "%-40s: %d\n", "bits_per_pixel", m_bpp);
    fprintf (stdout, "%-40s: %d\n", "source_csc", GetCodeCsc ());
    fprintf (stdout, "%-40s: %d\n", "chroma_format", GetCodeChroma ());
    fprintf (stdout, "%-40s: %d\n", "chunk_size", m_chunkSize);
    fprintf (stdout, "\n");

    //! rate control !//
    fprintf (stdout, "%-40s: %d\n", "rc_buffer_init_size", m_rcBufferInitSize);
    fprintf (stdout, "%-40s: %d\n", "rc_init_tx_delay", m_rcInitTxDelay);
    fprintf (stdout, "%-40s: %d\n", "rc_buffer_max_size", m_rcBufferMaxSize);
    fprintf (stdout, "%-40s: %d\n", "rc_stuffing_bits", m_rcStuffingBits);
    fprintf (stdout, "%-40s: %d\n", "rc_target_rate_scale", m_rcTargetRateScale);
    fprintf (stdout, "%-40s: %d\n", "rc_target_rate_threshold", m_rcTargetRateThreshold);
    fprintf (stdout, "%-40s: %d\n", "rc_target_rate_extra_fbls", m_rcTargetRateExtraFbls);
    fprintf (stdout, "%-40s: %d\n", "rc_fullness_scale", m_rcBufferFullnessScale);
    fprintf (stdout, "%-40s: %d\n", "rc_fullness_offset_threshold", m_rcFullnessOffsetThreshold);
    fprintf (stdout, "%-40s: %d\n", "rc_fullness_offset_slope", m_rcFullnessOffsetSlope);
    fprintf (stdout, "\n");

    //! flatness !//
    fprintf (stdout, "%-40s: %d\n", "flatness_qp_very_flat_fbls", m_flatnessQpVeryFlatFbls);
    fprintf (stdout, "%-40s: %d\n", "flatness_qp_very_flat_nfbls", m_flatnessQpVeryFlatNfbls);
    fprintf (stdout, "%-40s: %d\n", "flatness_qp_somewhat_flat_fbls", m_flatnessQpSomewhatFlatFbls);
    fprintf (stdout, "%-40s: %d\n", "flatness_qp_somewhat_flat_nfbls", m_flatnessQpSomewhatFlatNfbls);
    fprintf (stdout, "\n");

    //! ssm !//
    fprintf (stdout, "%-40s: %d\n", "ssm_max_se_size", m_ssmMaxSeSize);
    fprintf (stdout, "\n");

	//! MPP min step size !//
	if (m_versionMinor >= 2) {
		fprintf (stdout, "%-40s: %d\n", "mpp_min_step_size", m_mppMinStepSize);
		fprintf (stdout, "\n");
	}

    //! max QP LUT !//
    fprintf (stdout, "%-40s: [", "max_qp_lut");
    for (UInt i = 0; i < 8; i++) {
        fprintf (stdout, "%3d", m_maxQpLut[i]);
        if (i < 7) {
            fprintf (stdout, ", ");
        }
    }
    fprintf (stdout, "]\n");

    //! flatness QP LUT !//
    fprintf (stdout, "%-40s: [", "flatness_qp_lut");
    for (UInt i = 0; i < 8; i++) {
        fprintf (stdout, "%3d", m_flatnessQpLut[i]);
        if (i < 7) {
            fprintf (stdout, ", ");
        }
    }
    fprintf (stdout, "]\n");

    //! target rate delta LUT !//
    fprintf (stdout, "%-40s: [", "target_rate_delta_lut");
    for (UInt i = 0; i < 8; i++) {
        fprintf (stdout, "%3d, ", m_targetRateDeltaLut[i]);
    }
    fprintf (stdout, "...\n");
    fprintf (stdout, "%-40s   ", "");
    for (UInt i = 8; i < 16; i++) {
        fprintf (stdout, "%3d", m_targetRateDeltaLut[i]);
        if (i < 15) {
            fprintf (stdout, ", ");
        }
    }
    fprintf (stdout, "]\n");

    //! MPPF bits/component !//
    fprintf (stdout, "%-40s: %d\n", "mppf_bits_per_comp R/Y", m_mppfBitsPerCompA[0]);
    fprintf (stdout, "%-40s: %d\n", "mppf_bits_per_comp G/Cb", m_mppfBitsPerCompA[1]);
    fprintf (stdout, "%-40s: %d\n", "mppf_bits_per_comp B/Cr", m_mppfBitsPerCompA[2]);
    fprintf (stdout, "%-40s: %d\n", "mppf_bits_per_comp Y", m_mppfBitsPerCompB[0]);
    fprintf (stdout, "%-40s: %d\n", "mppf_bits_per_comp Co", m_mppfBitsPerCompB[1]);
    fprintf (stdout, "%-40s: %d\n", "mppf_bits_per_comp Cg", m_mppfBitsPerCompB[2]);
	fprintf (stdout, "\n");
	fprintf (stdout, "%-40s: %I64d\n", "slice_num_bits", m_sliceNumBits);
	fprintf (stdout, "%-40s: %d\n", "chunk_adj_bits", m_chunkAdjBits);
	fprintf (stdout, "%-40s: %d\n", "num_extra_mux_bits", m_numExtraMuxBits);
    fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
}

UInt Pps::MapCodeBpc (UInt code) {
    UInt bpc;
    switch (code) {
    case 0:
        bpc = 8;
        break;
    case 1:
        bpc = 10;
        break;
    case 2:
        bpc = 12;
        break;
    }
    return bpc;
}

ChromaFormat Pps::MapCodeChroma (UInt code) {
    ChromaFormat cf;
    switch (code) {
    case 0:
        cf = k444;
        break;
    case 1:
        cf = k422;
        break;
    case 2:
        cf = k420;
        break;
    }
    return cf;
}

ColorSpace Pps::MapCodeCsc (UInt code) {
    ColorSpace csc;
    switch (code) {
    case 0:
        csc = kRgb;
        break;
    case 1:
        csc = kYCbCr;   
        break;
    }
    return csc;
}

UInt Pps::GetCodeBpc () {
    UInt bpcCode;
    switch (m_bitDepth) {
    case 8:
        bpcCode = 0;
        break;
    case 10:
        bpcCode = 1;
        break;
    case 12:
        bpcCode = 2;
        break;
    }
    return bpcCode;
}

UInt Pps::GetCodeCsc () {
    UInt csc;
    switch (m_srcCsc) {
    case kRgb:
        csc = 0;
        break;
    case kYCbCr:
        csc = 1;
        break;
    }
    return csc;
}

UInt Pps::GetCodeChroma () {
    UInt cf;
    switch (m_chroma) {
    case k444:
        cf = 0;
        break;
    case k422:
        cf = 1;
        break;
    case k420:
        cf = 2;
        break;
    }
    return cf;
}

Void Pps::InitDecoder () {
    InitMemoryPpsLuts ();
    m_rcBufferMinSize = 0;
    m_blockWidth = 8;
    m_blockHeight = 2;
    UInt curByte = 0;

    //! versioning !//
    m_versionMajor = (m_ppsBitBuffer[curByte] & 0x7F);
    curByte += 1;
    m_versionMinor = (m_ppsBitBuffer[curByte] & 0x7F);
    curByte += 1;
    m_versionRelease = m_ppsBitBuffer[curByte];
    curByte += 1;

    curByte += 1; //! pps identifier !//

    CheckVersionInfo ();

    //! source dimensions !//
    m_srcWidth = (m_ppsBitBuffer[curByte] << 8) 
        | m_ppsBitBuffer[curByte + 1];
    curByte += 2;
    m_srcHeight = (m_ppsBitBuffer[curByte] << 8)
        | m_ppsBitBuffer[curByte + 1];
    curByte += 2;

    //! slice dimensions !//
    m_sliceWidth = (m_ppsBitBuffer[curByte] << 8) 
        | m_ppsBitBuffer[curByte + 1];
    curByte += 2;
    m_sliceHeight = (m_ppsBitBuffer[curByte]) << 8 
        | m_ppsBitBuffer[curByte + 1];
    curByte += 2;

    m_sliceNumPx = (m_ppsBitBuffer[curByte] << 24) 
        | (m_ppsBitBuffer[curByte + 1] << 16) 
        | (m_ppsBitBuffer[curByte + 2] << 8) 
        | (m_ppsBitBuffer[curByte + 3]);
    curByte += 4;

    //! bpp, bit depth, colorspace, chroma format !//
    m_bpp = ((m_ppsBitBuffer[curByte] & 0x3) << 8) 
        | (m_ppsBitBuffer[curByte + 1] & 0xFF);
    curByte += 2;

    // reserved
    curByte++;

    m_bitDepth = MapCodeBpc ((m_ppsBitBuffer[curByte] >> 4) & 0xF);
    m_srcCsc = MapCodeCsc ((m_ppsBitBuffer[curByte] >> 2) & 0x3);
    m_chroma = MapCodeChroma (m_ppsBitBuffer[curByte] & 0x3);
    curByte++;

    //! chunk size !//
    curByte += 2; //! reserved !//
    m_chunkSize = (m_ppsBitBuffer[curByte] << 8) 
        | m_ppsBitBuffer[curByte + 1];
    curByte += 2;
    
    //! rc buffer init size !//
    curByte += 2; //! reserved !//
    m_rcBufferInitSize = (m_ppsBitBuffer[curByte] << 8) 
        | m_ppsBitBuffer[curByte + 1];
    curByte += 2;

    //! rc stuffing bits !//
    m_rcStuffingBits = m_ppsBitBuffer[curByte];
    curByte += 1;

    //! rc init tx delay !//
    m_rcInitTxDelay = m_ppsBitBuffer[curByte];
    curByte += 1;

    //! rc buffer max size !//
    m_rcBufferMaxSize = (m_ppsBitBuffer[curByte] << 8)
        | m_ppsBitBuffer[curByte + 1];
    curByte += 2;

    //! rc target rate threshold !//
    m_rcTargetRateThreshold = (m_ppsBitBuffer[curByte] << 24)
        | (m_ppsBitBuffer[curByte + 1] << 16)
        | (m_ppsBitBuffer[curByte + 2] << 8)
        | (m_ppsBitBuffer[curByte + 3]);
    curByte += 4;

    //! rc target rate scale !//
    m_rcTargetRateScale = m_ppsBitBuffer[curByte];
    curByte += 1;

    //! rc fullness scale !//
    m_rcBufferFullnessScale = m_ppsBitBuffer[curByte];
    curByte += 1;

    //! rc fullness offset threshold !//
    m_rcFullnessOffsetThreshold = (m_ppsBitBuffer[curByte] << 8)
        | (m_ppsBitBuffer[curByte + 1]);
    curByte += 2;

    //! rc fullness offset slope !//
    m_rcFullnessOffsetSlope = (m_ppsBitBuffer[curByte] << 16)
        | (m_ppsBitBuffer[curByte + 1] << 8)
        | (m_ppsBitBuffer[curByte + 2]);
    curByte += 3;

    //! rc target rate extra FBLS !//
    m_rcTargetRateExtraFbls = (m_ppsBitBuffer[curByte] & 0xF);
    curByte += 1;

    //! flatness qp very flat FBLS !//
	m_flatnessQpVeryFlatFbls = (int8)m_ppsBitBuffer[curByte];
    curByte += 1;
    
    //! flatness qp very flat NFBLS !//
	m_flatnessQpVeryFlatNfbls = (int8)m_ppsBitBuffer[curByte];
    curByte += 1;
    
    //! flatness qp somewhat flat FBLS !//
	m_flatnessQpSomewhatFlatFbls = (int8)m_ppsBitBuffer[curByte];
    curByte += 1;
    
    //! flatness qp somewhat flat NFBLS !//
	m_flatnessQpSomewhatFlatNfbls = (int8)m_ppsBitBuffer[curByte];
    curByte += 1;
    
    //! flatness QP LUT !//
    for (UInt b = 0; b < 8; b++) {
		m_flatnessQpLut[b] = (int8)m_ppsBitBuffer[curByte];
        curByte += 1;
    }
    
    //! max QP LUT !//
    for (UInt b = 0; b < 8; b++) {
        m_maxQpLut[b] = m_ppsBitBuffer[curByte];
        curByte += 1;
    }

    //! target rate delta LUT !//
    for (UInt b = 0; b < 16; b++) {
        m_targetRateDeltaLut[b] = m_ppsBitBuffer[curByte];
        curByte += 1;
    }

    //! mppf bits per comp !//
    curByte += 1; //! reserved !//
    m_mppfBitsPerCompA[0] = m_ppsBitBuffer[curByte] >> 4;
    m_mppfBitsPerCompA[1] = m_ppsBitBuffer[curByte] & 0xF;
    curByte += 1;
    m_mppfBitsPerCompA[2] = m_ppsBitBuffer[curByte] >> 4;
    m_mppfBitsPerCompB[0] = m_ppsBitBuffer[curByte] & 0xF;
    curByte += 1;
    m_mppfBitsPerCompB[1] = m_ppsBitBuffer[curByte] >> 4;
    m_mppfBitsPerCompB[2] = m_ppsBitBuffer[curByte] & 0xF;
    curByte += 1;

    //! ssm max se size !//
    curByte += 3; //! reserved !//
    m_ssmMaxSeSize = m_ppsBitBuffer[curByte];
    curByte += 1;

	//! slice num bits !//
	curByte += 3; //! reserved !//
	UInt64 hi = (UInt64)(m_ppsBitBuffer[curByte]);
	curByte += 1;
	UInt64 lo = (m_ppsBitBuffer[curByte] << 24)
		| (m_ppsBitBuffer[curByte + 1] << 16)
		| (m_ppsBitBuffer[curByte + 2] << 8)
		| (m_ppsBitBuffer[curByte + 3]);
	curByte += 4;
	m_sliceNumBits = (hi << 32) | lo;

	//! num extra mux bits !//
	curByte += 1; //! reserved!//
	m_chunkAdjBits = m_ppsBitBuffer[curByte];
	curByte += 1;

	//! chunk adjustment bits !//
	m_numExtraMuxBits = (m_ppsBitBuffer[curByte] << 8) | m_ppsBitBuffer[curByte + 1];
	curByte += 2;

	if (m_versionMinor >= 2) {
		//! slices per line !//
		m_slicesPerLine = ((m_ppsBitBuffer[curByte] & 0x3) << 8) | m_ppsBitBuffer[curByte + 1];
		curByte += 2;

		//! horizontal slice padding !//
		m_slicePadX = (m_ppsBitBuffer[curByte] & 0x7);
		curByte += 1;

		//! mpp min step size !//
		m_mppMinStepSize = (m_ppsBitBuffer[curByte] & 0xF);
		curByte += 1;
	}
}

Void Pps::CheckVersionInfo () {
    if (m_versionMajor < 0 || m_versionMajor > 127) {
        fprintf (stderr, "Pps::CheckVersionInfo(): version major (%d) is out of valid numeric range [0-99].\n", m_versionMajor);
        exit (EXIT_FAILURE);
    }
    if (m_versionMinor < 0 || m_versionMinor > 127) {
        fprintf (stderr, "Pps::CheckVersionInfo(): version minor (%d) is out of valid numeric range [0-99].\n", m_versionMinor);
        exit (EXIT_FAILURE);
    }
    Bool isReleaseCharValid = (m_versionRelease == 0x0 || (m_versionRelease >= 0x61 && m_versionRelease <= 0x7a));
    if (!isReleaseCharValid) {
        fprintf (stderr, "Pps::CheckVersionInfo(): version release (%c (0x%x)) is out of valid ascii range {0x0, [0x61-0x7a]}.\n", m_versionRelease, m_versionRelease);
        exit (EXIT_FAILURE);
    }
}