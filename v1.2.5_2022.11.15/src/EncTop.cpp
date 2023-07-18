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

/* Copyright (c) 2015-2017 MediaTek Inc., PSNR calculation/BP-skip mode/RDO weighting are modified and/or implemented by MediaTek Inc. based on EncTop.cpp
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
  \file       EncTop.cpp
  \brief      Encoder Top
  */

#include <vector>
#include <string.h>
#include "Config.h"
#include "TypeDef.h"
#include "EncTop.h"
#include "Block.h"
#include "RateControl.h"
#include "FlatnessDetection.h"
#include "TransMode.h"
#include "BpMode.h"
#include "MppMode.h"
#include "MppfMode.h"
#include "Pps.h"
#include "Ssm.h"
#include "DebugMap.h"
#include "ImageFileIo.h"
#include <fstream>
#include "Misc.h"

extern Bool enEncTracer;
extern FILE* encTracer;
Int encModeCounter[g_numModes] = { 0 };
Int encGlobalBlockCounter = 0;
Bool enMinilog = false;

FILE* minilogFile = NULL;

/**
    Encode one slice

    @param pps picture parameter set object
    @param startY starting position y of slice in picture
    @param startX starting position x of slice in picture
    @param sliceBits slice output bitstream
    @param orgVideoFrame VideoFrame object for current picture
    @param recVideoFrame VideoFrame object for current slice
*/
Void EncTop::EncodeSlice (Pps* pps, Int startY, Int startX, BitstreamOutput* sliceBits, VideoFrame* orgVideoFrame, VideoFrame* recVideoFrame) {
    Bool isEncoder = true;
    ModeType prevMode = kModeTypeUndefined;
    ModeType bestMode = kModeTypeUndefined;
    UInt minBlockBits = 0;
    Int sliceBlockCounter = 0;

    if (enEncTracer) {
        fprintf (encTracer, "SLICE_START: %4d, %4d\n", startY, startX);
    }

    if (enMinilog) {
        fprintf (minilogFile, "sliceStart; y=%04d, x=%04d\n", startY, startX);
    }

    //! initialize storage for mode R/D information !//
    std::vector<Int> modeRates (g_numModes, LARGE_INT);
    std::vector<Int> modeSAD (g_numModes, LARGE_INT);
    std::vector<Int> modeRdCost (g_numModes, LARGE_INT);
    std::vector<Mode*> modeContainer (g_numModes);
    std::vector<Bool> modeSelectionIsValid (g_numModes, true);

    //! set up output bitstream !//
    BitstreamOutput* sliceOutBits = new BitstreamOutput;

    //! set up SSM !//
    SubStreamMux* ssm = new SubStreamMux (true, g_ssm_numSubstreams, m_ssmMaxSeSize);
    ssm->SetNewBitsOut (sliceOutBits);

    //! set up flatness detection !//
    FlatnessDetection *flatness = new FlatnessDetection (m_blkWidth, m_blkHeight, m_bitDepth, m_origSrcCsc, m_chromaFormat);
    FlatnessType curFlatnessType = kFlatnessTypeUndefined;

    //! set up frame data storage !//
    VideoFrame *curSliceSrcFrame = new VideoFrame ();
    VideoFrame *curSliceRecFrame = new VideoFrame ();
	VideoFrame *curSliceRecExtra = new VideoFrame ();

    Int finalSliceWidth = pps->GetSliceWidth ();
    Int finalSliceHeight = pps->GetSliceHeight ();
    Int numBlksX = finalSliceWidth >> 3;
    Int numBlksY = finalSliceHeight >> 1;
    Int padX = m_slicePadX;
    Int padY = (startY + finalSliceHeight > m_srcHeight) ? m_bottomSlicePadY : 0;
    Int origSliceWidth = finalSliceWidth - padX;
    Int origSliceHeight = finalSliceHeight - padY;

    //! set up current slice VideoFrame !//
    curSliceSrcFrame->Create (finalSliceWidth, finalSliceHeight, m_chromaFormat, padX, padY, m_origSrcCsc, m_bitDepth);
    curSliceRecFrame->Create (finalSliceWidth, finalSliceHeight, m_chromaFormat, padX, padY, m_origSrcCsc, m_bitDepth);
	if (m_origSrcCsc == kRgb) {
		curSliceRecExtra->Create (finalSliceWidth, 2, m_chromaFormat, padX, padY, kYCoCg, m_bitDepth);
	}
    InitSliceData (orgVideoFrame, curSliceSrcFrame, startY, startX, finalSliceWidth, finalSliceHeight, padX, padY);
    Int sliceStartRow = startY;

    // set up PPS
    Pps *curSlicePps = pps;

    // set up rate control
    RateControl* rc = new RateControl (m_chromaFormat, ssm, curSlicePps);
    rc->InitSlice (startX, startY);

    Block* block = new Block (m_origSrcCsc, m_blkWidth, m_blkHeight, curSliceSrcFrame, curSliceRecFrame);
    Block* blockNext = new Block (m_origSrcCsc, m_blkWidth, m_blkHeight, curSliceSrcFrame, curSliceRecFrame);

    // set up entropy encoder
    EntropyCoder *ec = new EntropyCoder (m_blkWidth, m_blkHeight, m_chromaFormat);
    ec->SetRcStuffingBits (curSlicePps->GetRcStuffingBits ());

    ColorSpace targetCsc = (m_origSrcCsc == kRgb ? kYCoCg : kYCbCr);

    // transform mode
    TransMode* transMode = new TransMode (m_bpp, m_bitDepth, true, m_chromaFormat, m_origSrcCsc, targetCsc);
    transMode->Init (startY, startX, block, curSliceRecFrame, rc, ec, ssm, curSlicePps);
	transMode->SetRecBufferYCoCg (curSliceRecExtra);
    modeContainer[kModeTypeTransform] = transMode;

    // BP, BP-SKIP
    BpMode* bpMode = new BpMode (m_bpp, m_bitDepth, true, m_chromaFormat, m_origSrcCsc, targetCsc, false);
    BpMode* bpModeSkip = new BpMode (m_bpp, m_bitDepth, true, m_chromaFormat, m_origSrcCsc, targetCsc, true);
    bpMode->Init (startY, startX, block, curSliceRecFrame, rc, ec, ssm, curSlicePps);
    bpModeSkip->Init (startY, startX, block, curSliceRecFrame, rc, ec, ssm, curSlicePps);
    bpModeSkip->SetBpModeRef (bpMode); // bpModeSkip holds reference to bpMode to copy BPVs
	bpMode->SetRecBufferYCoCg (curSliceRecExtra);
	bpModeSkip->SetRecBufferYCoCg (curSliceRecExtra);
    modeContainer[kModeTypeBP] = bpMode;
    modeContainer[kModeTypeBPSkip] = bpModeSkip;

    // MPP
    MppMode* mppMode = new MppMode (m_bpp, m_bitDepth, true, m_chromaFormat, m_origSrcCsc, g_mtkMppColorSpaceRdoQpThreshold);
    mppMode->Init (startY, startX, block, curSliceRecFrame, rc, ec, ssm, curSlicePps);
	mppMode->SetRecBufferYCoCg (curSliceRecExtra);
    modeContainer[kModeTypeMPP] = mppMode;

    // MPPF
    MppfMode* mppfMode = new MppfMode (m_bpp, m_bitDepth, true, m_chromaFormat, m_origSrcCsc);
    mppfMode->Init (startY, startX, block, curSliceRecFrame, rc, ec, ssm, curSlicePps);
	mppfMode->SetRecBufferYCoCg (curSliceRecExtra);
    modeContainer[kModeTypeMPPF] = mppfMode;
    minBlockBits = mppfMode->GetMinBlockBits ();

    Int bitCnt = 0;
    Int cumSliceNumPixels = 0;
    Int prevBlockBits = 0; // number of bits spent for the previous block
    Int prevBlockBitsWithoutPadding = 0;

    UInt maxBitsPcm = g_maxModeHeaderLen + g_maxFlatHeaderLen + (m_bitDepth == 8 ? 3 : 4);
    for (UInt k = 0; k < g_numComps; k++) {
        maxBitsPcm += m_bitDepth * (16 >> (g_compScaleX[k][m_chromaFormat] + g_compScaleY[k][m_chromaFormat]));
    }

    if (enMinilog) {
        fprintf (minilogFile, "RC; %10s, %10s, %10s, %10s, %10s, %10s, %10s\n", "blockIdx", "prev", "pad", "target", "buf", "rc", "qp");
    }

    for (Int r = 0; r < numBlksY; r++) {
        Bool isFls = (r == 0 ? true : false);
        flatness->PartialReset ();
        Int blk_y = (m_blkHeight * r);
        Int blkSrc_y = startY + blk_y;
		for (Int c = 0; c < numBlksX; c++) {
			Bool lastBlock = false;
			Int mppfTypeUsed = -1;
			Int blk_x = (m_blkWidth * c);
			Int blkSrc_x = startX + blk_x;

			if (enEncTracer) {
				fprintf (encTracer, "BLOCK_START: %4d, %4d\n", blk_y, blk_x);
			}
			block->Init (blk_x, blk_y, m_bitDepth, m_origSrcCsc);

			// update rate control
			rc->UpdateBufferFullness (prevBlockBits);
			rc->UpdateRcFullness ();

			// update flatness information
			ColorSpace flatnessCsc = (m_origSrcCsc == kRgb ? kYCoCg : kYCbCr);

			flatness->SetFlatnessDetectionColorSpace (flatnessCsc);
			flatness->UpdatePositionData (r, c, numBlksX, numBlksY);
			flatness->InitCurBlock (block);
			flatness->InitNextBlock (blockNext);
			flatness->CalcComplexity ();
			flatness->Classify ();
			curFlatnessType = flatness->GetFlatnessType ();

			// reset qpUpdateMode
			rc->SetQpUpdateMode (0);

			// decide the QP based on the bits spent on encoding the previous block and target Bits
			Int targetBits;
			rc->CalcTargetRate (targetBits, lastBlock, (r == 0));
			rc->SetIsFls (r == 0);
			if (prevBlockBits > 0) {
				rc->UpdateQp (prevBlockBitsWithoutPadding, targetBits, (r == 0), curFlatnessType);
			}

			Int numRcPaddingBits = prevBlockBits - prevBlockBitsWithoutPadding;
			Bool enableUnderflowPrevention = rc->CheckUnderflowPrevention ();


			//! minilog - rate control info !//
			if (enMinilog) {
				fprintf (minilogFile, "RC; %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d\n",
                    sliceBlockCounter, prevBlockBits, numRcPaddingBits, targetBits, rc->GetBufferFullness (), rc->GetRcFullness (), rc->GetRcOffset (), rc->GetQp ());
			}

			//! debugTracer - rate control info !//
			if (enEncTracer) {
				if (curFlatnessType != kFlatnessTypeUndefined) {
					fprintf (encTracer, "FLATNESS: %s\n", g_flatnessTypeNames[curFlatnessType].c_str ());
				}
				fprintf (encTracer, "RC: prevBlockRate = %d (%d padding bits)\n", prevBlockBits, numRcPaddingBits);
				fprintf (encTracer, "RC: targetRate = %d\n", targetBits);
				fprintf (encTracer, "RC: bufferFullness = %d\n", rc->GetBufferFullness ());
				fprintf (encTracer, "RC: rcFullness = %d\n", rc->GetRcFullness ());
				fprintf (encTracer, "RC: qp = %d\n", rc->GetQp ());
				if (enableUnderflowPrevention) {
					fprintf (encTracer, "RC: underflow prevention mode enabled\n");
				}
			}

			//! render flatness, QP to debugMap !//
			if (m_debugInfoConfig.debugMapsEnable) {
				m_debugMapFlat->DrawFlatness (blkSrc_y, blkSrc_x, curFlatnessType);
				m_debugMapQp->DrawQp (blkSrc_y, blkSrc_x, rc->GetQp (), pps);
			}

			UInt64 sliceBitsRemaining = lastBlock ? rc->GetSliceBitsRemainingLastBlock () : rc->GetSliceBitsRemaining ();
			UInt maxAllowedBits = (UInt)Min (maxBitsPcm, sliceBitsRemaining);

			// Initialize the rate and SAD for all the modes 
			for (Int k = 0; k < g_numModes; k++) {
				modeRates[k] = LARGE_INT;
				modeSAD[k] = LARGE_INT;
				modeRdCost[k] = LARGE_INT;
			}

			//calculate lambda based on buffer fullness
			Int maxBuffer = ComputeLambdaBufferFullness ((1 << g_rc_BFRangeBits) - 1, curSlicePps);
			Int dBuffer = (lastBlock ? maxBuffer : ComputeLambdaBufferFullness (rc->GetRcFullness (), curSlicePps));

			for (UInt curMode = 0; curMode < modeContainer.size (); curMode++) {
				if (!modeContainer[curMode]) {
					// no mode loaded in slot, continue to next
					continue;
				}
				modeContainer[curMode]->SetUnderflowMode (enableUnderflowPrevention ? true : false);
				if (curMode == kModeTypeBPSkip) {
					modeContainer[curMode]->FetchDataFromReference (); // copy BPVs from reference to bpMode (using virtual in Mode class)
				}
				modeContainer[curMode]->Update (blk_x, blk_y, isFls, prevMode, curFlatnessType, maxAllowedBits, dBuffer);
				modeContainer[curMode]->Test ();
			}

			UInt bpTestRate = modeContainer[kModeTypeBP]->GetRate ();
			UInt bpEncodeRate = 0;

			// Copy R/D information from Mode() class to local storage
			for (UInt curMode = 0; curMode < modeContainer.size (); curMode++) {
				if (!modeContainer[curMode]) {
					modeRates[curMode] = LARGE_INT;
					modeSAD[curMode] = LARGE_INT;
					modeRdCost[curMode] = LARGE_INT;
					continue;
				}
				modeRates[curMode] = modeContainer[curMode]->GetRate ();
				modeSAD[curMode] = modeContainer[curMode]->GetDistortion ();
				modeRdCost[curMode] = modeContainer[curMode]->GetRdCost ();
			}

            // encoder mode selection
            for (UInt c = 0; c < modeSelectionIsValid.size (); c++) {
                modeSelectionIsValid[c] = true;
            }
            bestMode = SelectCurBlockMode (modeRates, modeSAD, modeRdCost, rc, lastBlock, minBlockBits, pps, modeSelectionIsValid);

			// disallow BP-SKIP replacement if underflow prevention is active
			Bool allowReplacement = !enableUnderflowPrevention;
			if (bestMode == kModeTypeBP && allowReplacement && bpMode->GetBPSkipFlag ()) {
                if (modeSelectionIsValid[kModeTypeBPSkip]) {
                    bestMode = kModeTypeBPSkip;
                }
			}

			if (bestMode == kModeTypeMPP && allowReplacement && m_chromaFormat != k420) {
				Int offset = 8;
				Int MPPstepSize = (rc->GetQp () - g_rc_qpStepSizeOne) + offset;
				Int stepSizeThr = (m_bitDepth << 3) - g_mtkMppSubstitutionThreshold;
				bestMode = ((modeRdCost[kModeTypeBPSkip] < modeRdCost[kModeTypeMPP]) && (MPPstepSize >= stepSizeThr)) ? kModeTypeBPSkip : kModeTypeMPP;
			}

			if (m_debugInfoConfig.debugMapsEnable) {
				m_debugMapMode->DrawMode (blkSrc_y, blkSrc_x, bestMode, bpMode->GetUse2x2 ());
				m_debugMapBuffer->DrawBuffer (blkSrc_y, blkSrc_x, (UInt)rc->GetRcFullness ());
				m_debugMapTargetBitrate->DrawTargetBitrate (blkSrc_y, blkSrc_x, targetBits);
			}

			//! encode best mode !//
			ssm->GenerateNewSyntaxElements ();
			modeContainer[bestMode]->Encode ();
			encModeCounter[bestMode]++;
			encGlobalBlockCounter++;
			prevBlockBitsWithoutPadding = modeContainer[bestMode]->GetRateWithoutPadding ();

			if (m_origSrcCsc == kRgb) {
				CopyYCoCgDataToRgbBuffer (curSliceRecExtra, curSliceRecFrame, blk_y, blk_x);
			}

			if (enEncTracer) {
				PrintEncodeInfo ((ModeType)bestMode, blkSrc_y, blkSrc_x, modeContainer[bestMode]->GetQP (), modeContainer[bestMode]->GetRate (), prevBlockBitsWithoutPadding);
				fprintf (encTracer, "\n");
			}

			//! track post-encode bits !//
			UInt curBlockBits = 0;
			for (UInt ss = 0; ss < ssm->GetNumSubStreams (); ss++) {
				curBlockBits += ssm->GetCurSyntaxElement (ss)->GetNumBits ();
			}

			// update slice counters for blocks/pixels
			cumSliceNumPixels += 16;
			sliceBlockCounter++;

			// track best mode for mode header prediction
			prevMode = bestMode;

			// encode current block and update simulated decoder model
			ssm->EncodeBlock ();

			prevBlockBits = curBlockBits;
			if (r == 0 && c == 0) {
				rc->SetIsFirstBlockInSlice (false);
			}
        }
    }

    // update RC for sanity checks
    rc->UpdateBufferFullness (prevBlockBits);

    // slice encoding is finished for SSM0
    ssm->SetIsEncodingFinished (0, true);

    // process final block encoding for SSM1..SSM3
    ssm->EncodeBlock ();
    for (UInt ssmIdx = 1; ssmIdx < ssm->GetNumSubStreams (); ssmIdx++) {
        ssm->SetIsEncodingFinished (ssmIdx, true);
    }

    // still need to transmit mux words from balance FIFOs
    while (ssm->GetNumStoredSyntaxElement () > 0) {
        ssm->EncodeBlock ();
    }

    if (sliceOutBits->GetNumBitsUntilByteAligned () > 0) {
        sliceOutBits->WriteByteAlignment ();
    }

    if (enEncTracer) {
        fprintf (encTracer, "SLICE_END: %4d, %4d\n\n\n", startY, startX);
    }

    //! sanity checks !//
    Bool checkA = rc->GetBufferFullness () <= rc->GetRcBufferInitSize ();
    rc->UpdateBufferLastChunksInSlice ();
    Bool checkB = rc->GetBufferFullness () + rc->GetSliceBitsRemaining () + rc->GetNumExtraMuxBits () == rc->GetRcBufferInitSize ();
    if (!checkA || !checkB) {
        fprintf (stderr, "EncTop::EncodeSlice(): rate buffer constraints exceeded for current slice.\n");
        exit (EXIT_FAILURE);
    }

    while (sliceOutBits->GetByteStreamLength () < (UInt)((rc->GetSliceBitsTotal () + rc->GetNumExtraMuxBits ()) / 8)) {
        sliceOutBits->Write (0, 1);
    }
    sliceOutBits->CopyBufferOut (sliceBits);

    assert (sliceOutBits->GetByteStreamLength () == ((rc->GetSliceBitsTotal () + rc->GetNumExtraMuxBits ()) / 8));
    if ((sliceOutBits->GetByteStreamLength () * 8) > (UInt)(rc->GetSliceBitsTotal () + rc->GetNumExtraMuxBits ())) {
        fprintf (stderr, "bit budget exceeded for current slice (y=%04d, x=%04d).\n", startY, startX);
    }
    block->Destroy ();
    blockNext->Destroy ();

    //! copy reconstructed slice data to reconstructed frame !//
    for (Int k = 0; k < g_numComps; k++) {
        Int sx = startX >> g_compScaleX[k][m_chromaFormat];
        Int sy = startY >> g_compScaleY[k][m_chromaFormat];
        Int *pSrc = curSliceRecFrame->getBuf (k);
        Int *pDst = recVideoFrame->getBuf (k);
        Int sliceHeight = origSliceHeight >> g_compScaleY[k][m_chromaFormat];
        Int sliceWidth = origSliceWidth >> g_compScaleX[k][m_chromaFormat];
        for (Int y = 0; y < sliceHeight; y++) {
            for (Int x = 0; x < sliceWidth; x++) {
				UInt64 srcLoc = (UInt64)(y) * (UInt64)(finalSliceWidth >> g_compScaleX[k][m_chromaFormat]) + (UInt64)(x);
				UInt64 dstLoc = (UInt64)(sy + y) * (UInt64)(m_srcWidth >> g_compScaleX[k][m_chromaFormat]) + (UInt64)(sx + x);
                pDst[dstLoc] = pSrc[srcLoc];
            }
        }
    }

    if (enMinilog) {
        fprintf (minilogFile, "sliceEnd; --------------------------------------------------\n");
    }

    delete curSliceSrcFrame;    curSliceSrcFrame = NULL;
    delete curSliceRecFrame;    curSliceRecFrame = NULL;
	delete curSliceRecExtra;	curSliceRecExtra = NULL;
    delete ec;                  ec = NULL;
    delete rc;                  rc = NULL;
    delete block;               block = NULL;
    delete blockNext;           blockNext = NULL;
    delete flatness;            flatness = NULL;
    delete ssm;                 ssm = NULL;
    delete sliceOutBits;        sliceOutBits = NULL;

    // clear memory for coding modes
    for (UInt curMode = 0; curMode < modeContainer.size (); curMode++) {
        if (!modeContainer[curMode]) {
            continue;
        }
        delete modeContainer[curMode];
        modeContainer[curMode] = NULL;
    }
}

/**
    Encode one picture

    @return false if error, otherwise true
*/
Bool EncTop::Encode () {
    enEncTracer = m_debugTracer;
    enMinilog = m_minilog;
    std::fstream bitstreamFile (m_cmpFile, std::fstream::binary | std::fstream::out);
    if (!bitstreamFile) {
        fprintf (stderr, "failed to open bitstream file for writing: %s.\n", m_cmpFile);
        return false;
    }

    if (enMinilog) {
        std::string line (m_inFile);
        Int suffixPos = (Int)line.find_last_of (".");
        Int curFolder = 1 + std::max ((Int)line.find_last_of ("//"), (Int)line.find_last_of ("\\"));
        std::string rawFileName = line.substr (curFolder, suffixPos - curFolder);
        std::string minilogFileLocation = rawFileName + "_minilog.txt";
        minilogFile = fopen (minilogFileLocation.c_str (), "wt");
    }

    std::string line (m_inFile);
    Int suffixPos = (Int)line.find_last_of (".");
    Int curFolder = 1 + std::max ((Int)line.find_last_of ("//"), (Int)line.find_last_of ("\\"));
    std::string rawFileName = line.substr (curFolder, suffixPos - curFolder);

    if (m_debugInfoConfig.debugMapsEnable) {
        //! allocate memory for debug maps !//
        m_debugMapMode = new DebugMap (m_srcWidth, m_srcHeight, m_blkWidth, m_blkHeight, m_bitDepth, rawFileName + "_modeMap.ppm");
        m_debugMapFlat = new DebugMap (m_srcWidth, m_srcHeight, m_blkWidth, m_blkHeight, m_bitDepth, rawFileName + "_flatMap.ppm");
        m_debugMapQp = new DebugMap (m_srcWidth, m_srcHeight, m_blkWidth, m_blkHeight, m_bitDepth, rawFileName + "_qpMap.ppm");
        m_debugMapBuffer = new DebugMap (m_srcWidth, m_srcHeight, m_blkWidth, m_blkHeight, m_bitDepth, rawFileName + "_bufMap.ppm");
        m_debugMapTargetBitrate = new DebugMap (m_srcWidth, m_srcHeight, m_blkWidth, m_blkHeight, m_bitDepth, rawFileName + "_targetBitrateMap.ppm");
    }

    encTracer = (enEncTracer ? fopen ("debugTracerEncoder.txt", "w") : NULL);

    if (m_calcPsnr) {
        m_psnrFile = fopen ("encPsnr.dat", "a");
    }

    // allocate memory for source and reconstructed frames
    VideoFrame *orgVideoFrame = NULL;
    VideoFrame *recVideoFrame = NULL;
    Int padX = 0;
    Int padY = 0;
    orgVideoFrame = new VideoFrame;
    orgVideoFrame->Create (m_srcWidth, m_srcHeight, m_chromaFormat, padX, padY, m_origSrcCsc, m_bitDepth);
    recVideoFrame = new VideoFrame;
    recVideoFrame->Create (m_srcWidth, m_srcHeight, m_chromaFormat, padX, padY, m_origSrcCsc, m_bitDepth);

    // handle case of full slice height
    if (m_sliceHeight == 0) {
        m_sliceHeight = m_srcHeight;
    }

    //! read input file !//
    m_imageFileIn->SetCommandLineOptions (m_dpxOptions);
    m_imageFileIn->ReadData (orgVideoFrame);

    if (m_debugInfoConfig.debugMapsEnable) {
        // copy the input image to flatness map (flatness info will be drawn over)
        m_debugMapFlat->CopySourceData (orgVideoFrame);
        if (orgVideoFrame->getColorSpace () == kYCbCr) {
            m_debugMapFlat->Yuv2Rgb ();
        }
    }

    UInt numSlicesY = ((orgVideoFrame->getHeight (0) - 1) / m_sliceHeight) + 1;
    UInt numSlicesX = ((orgVideoFrame->getOrigWidth (0) - 1) / m_sliceWidth) + 1;
	if (!CalculateSlicePadding ()) {
        return false;
    }

    Int origSliceWidth = m_sliceWidth - m_slicePadX;
    Pps *pps = new Pps (m_srcWidth, m_srcHeight, m_sliceWidth, m_sliceHeight, m_bpp, m_bitDepth, m_origSrcCsc, m_chromaFormat);
    pps->LoadFromConfigFileData (m_configFileData, true);
    pps->CheckVersionInfo ();
	pps->SetSsmMaxSeSize (m_ssmMaxSeSize);
    pps->InitPps ();
	if (pps->GetVersionMinor () >= 2) {
		pps->SetSlicesPerLine (m_slicesPerLine);
		pps->SetSlicePadX (m_slicePadX);
	}

	if (!VersionChecks (pps)) {
		return false;
	}

    pps->GeneratePpsBitstream ();
    pps->WritePpsBitstream (&bitstreamFile);
    if (m_ppsDumpBytes) {
        pps->DumpPpsBytes ();
    }
    if (m_ppsDumpFields) {
        pps->DumpPpsFields ();
    }

    //! set up vector of buffers for slice multiplexing !//
    std::vector<BitstreamOutput*> sliceBytesContainer;
    for (UInt sliceX = 0; sliceX < numSlicesX; sliceX++) {
        sliceBytesContainer.push_back (new BitstreamOutput ());
    }

    for (UInt sliceY = 0; sliceY < numSlicesY; sliceY++) {
        //! encode slices !//
        for (UInt sliceX = 0; sliceX < numSlicesX; sliceX++) {
            Int startY = sliceY * m_sliceHeight;
            Int startX = sliceX * origSliceWidth;
            sliceBytesContainer.at (sliceX)->Clear ();
            EncodeSlice (pps, startY, startX, sliceBytesContainer.at (sliceX), orgVideoFrame, recVideoFrame);
        }

        //! perform slice multiplexing !//
        UInt chunkSize = pps->GetChunkSize ();
        for (UInt line = 0; line < m_sliceHeight; line++) {
            for (UInt slice = 0; slice < numSlicesX; slice++) {
                Char* sliceBytes = sliceBytesContainer.at (slice)->GetByteStream ();
                bitstreamFile.write (&sliceBytes[line * chunkSize], chunkSize);
            }
        }
    }

    //! clear the slice multiplexer storage !//
    for (UInt sliceX = 0; sliceX < numSlicesX; sliceX++) {
        delete sliceBytesContainer.at (sliceX);
    }
    sliceBytesContainer.clear ();

    //! calculate PSNR !//
    if (m_calcPsnr) {
        Int* Rec0 = recVideoFrame->getBuf (0);
        Int* Rec1 = recVideoFrame->getBuf (1);
        Int* Rec2 = recVideoFrame->getBuf (2);
        Int* Cur0 = orgVideoFrame->getBuf (0);
        Int* Cur1 = orgVideoFrame->getBuf (1);
        Int* Cur2 = orgVideoFrame->getBuf (2);
        UInt numSamples = recVideoFrame->getWidth (0) * recVideoFrame->getHeight (0);
        UInt numSamplesChroma = numSamples >> (g_compScaleX[1][m_chromaFormat] + g_compScaleY[1][m_chromaFormat]);
        Int maxVal = (1 << m_bitDepth) - 1;
        Double err;
        Double MSE = 0.0, MSE_r = 0.0, MSE_g = 0.0, MSE_b = 0.0, MSE_y = 0.0, MSE_u = 0.0, MSE_v = 0.0;
        Double PSNR = 0.0, PSNR_r = 0.0, PSNR_b = 0.0, PSNR_g = 0.0, PSNR_y = 0.0, PSNR_u = 0.0, PSNR_v = 0.0;
        switch (orgVideoFrame->getColorSpace ()) {
            case kRgb:
                for (UInt s = 0; s < numSamples; s++) {
                    err = (Double)abs (Rec0[s] - Cur0[s]);
                    MSE_r += (err * err);
                    err = (Double)abs (Rec1[s] - Cur1[s]);
                    MSE_g += (err * err);
                    err = (Double)abs (Rec2[s] - Cur2[s]);
                    MSE_b += (err * err);
                }
                MSE_r /= (Double)numSamples;
                MSE_g /= (Double)numSamples;
                MSE_b /= (Double)numSamples;
                MSE = (MSE_r + MSE_g + MSE_b) / 3.0;
                PSNR_r = (MSE_r == 0.0) ? 100.0 : 10.0 * log10 ((Double)(maxVal * maxVal) / MSE_r);
                PSNR_g = (MSE_g == 0.0) ? 100.0 : 10.0 * log10 ((Double)(maxVal * maxVal) / MSE_g);
                PSNR_b = (MSE_b == 0.0) ? 100.0 : 10 * log10 ((Double)(maxVal * maxVal) / MSE_b);
                PSNR = (MSE == 0.0) ? 100.0 : 10.0 * log10 ((Double)(maxVal * maxVal) / MSE);

                fprintf (stdout, "\n");
                fprintf (stdout, "MSE/PSNR\n");
                fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
                fprintf (stdout, "        %10s\t%10s\t%10s\t%10s\n", "R", "G", "B", "avg");
                fprintf (stdout, "MSE   : %10.4f\t%10.4f\t%10.4f\t%10.4f\n", MSE_r, MSE_g, MSE_b, MSE);
                fprintf (stdout, "PSNR  : %10.4f\t%10.4f\t%10.4f\t%10.4f\n", PSNR_r, PSNR_g, PSNR_b, PSNR);
                fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
                fprintf (m_psnrFile, "        %10s\t%10s\t%10s\t%10s\n", "R", "G", "B", "avg");
                fprintf (m_psnrFile, "MSE   : %10.4f\t%10.4f\t%10.4f\t%10.4f\n", MSE_r, MSE_g, MSE_b, MSE);
                fprintf (m_psnrFile, "PSNR  : %10.4f\t%10.4f\t%10.4f\t%10.4f\n", PSNR_r, PSNR_g, PSNR_b, PSNR);
                break;
            case kYCbCr:
                //! luma !//
                for (UInt s = 0; s < numSamples; s++) {
                    err = (Double)abs (Rec0[s] - Cur0[s]);
                    MSE_y += (err * err);
                }
                //! chroma !//
                for (UInt s = 0; s < numSamplesChroma; s++) {
                    err = (Double)abs (Rec1[s] - Cur1[s]);
                    MSE_u += (err * err);
                    err = (Double)abs (Rec2[s] - Cur2[s]);
                    MSE_v += (err * err);
                }

                MSE_y /= (Double)numSamples;
                MSE_u /= (Double)numSamplesChroma;
                MSE_v /= (Double)numSamplesChroma;
                MSE = (MSE_y + MSE_u + MSE_v) / 3.0;
                PSNR_y = (MSE_y == 0.0) ? 100.0 : 10.0 * log10 ((Double)(maxVal * maxVal) / MSE_y);
                PSNR_u = (MSE_u == 0.0) ? 100.0 : 10.0 * log10 ((Double)(maxVal * maxVal) / MSE_u);
                PSNR_v = (MSE_v == 0.0) ? 100.0 : 10 * log10 ((Double)(maxVal * maxVal) / MSE_v);
                PSNR = (MSE == 0.0) ? 100.0 : 10.0 * log10 ((Double)(maxVal * maxVal) / MSE);

                fprintf (stdout, "\n");
                fprintf (stdout, "MSE/PSNR\n");
                fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
                fprintf (stdout, "        %10s\t%10s\t%10s\t%10s\n", "Y", "U", "V", "avg");
                fprintf (stdout, "MSE   : %10.4f\t%10.4f\t%10.4f\t%10.4f\n", MSE_y, MSE_u, MSE_v, MSE);
                fprintf (stdout, "PSNR  : %10.4f\t%10.4f\t%10.4f\t%10.4f\n", PSNR_y, PSNR_u, PSNR_v, PSNR);
                fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
                fprintf (m_psnrFile, "        %10s\t%10s\t%10s\t%10s\n", "Y", "U", "V", "avg");
                fprintf (m_psnrFile, "MSE   : %10.4f\t%10.4f\t%10.4f\t%10.4f\n", MSE_y, MSE_u, MSE_v, MSE);
                fprintf (m_psnrFile, "PSNR  : %10.4f\t%10.4f\t%10.4f\t%10.4f\n", PSNR_y, PSNR_u, PSNR_v, PSNR);
                break;
        }
    }

    //! mode distrubtions !//
    if (m_dumpModes) {
        Double xform = 100.0 * (Double)encModeCounter[0] / encGlobalBlockCounter;
        Double bp = 100.0 * (Double)encModeCounter[1] / encGlobalBlockCounter;
        Double mpp = 100.0 * (Double)encModeCounter[2] / encGlobalBlockCounter;
        Double mppf = 100.0 * (Double)encModeCounter[3] / encGlobalBlockCounter;
        Double bpskip = 100.0 * (Double)encModeCounter[4] / encGlobalBlockCounter;
        fprintf (stdout, "\n");
        fprintf (stdout, "Coding mode distrubtion (%d total blocks)\n", encGlobalBlockCounter);
        fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
        fprintf (stdout, "XFORM   : %10.4f%%\n", xform);
        fprintf (stdout, "BP      : %10.4f%%\n", bp);
        fprintf (stdout, "MPP     : %10.4f%%\n", mpp);
        fprintf (stdout, "MPPF    : %10.4f%%\n", mppf);
        fprintf (stdout, "BP-SKIP : %10.4f%%\n", bpskip);
        fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
        if (m_calcPsnr && orgVideoFrame->getColorSpace () == kRgb) {
            fprintf (m_psnrFile, "        %10s\t%10s\t%10s\t%10s\t%10s\n",
                "XFORM", "BP", "MPP", "MPPF", "BPSKIP");
            fprintf (m_psnrFile, "MODES : %10.4f\t%10.4f\t%10.4f\t%10.4f\t%10.4f\n",
                xform, bp, mpp, mppf, bpskip);
        }
    }

    //! write debugMaps to disk !//
    if (m_debugInfoConfig.debugMapsEnable) {
        m_debugMapQp->SavePpm ();              delete m_debugMapQp;               m_debugMapQp = NULL;
        m_debugMapMode->SavePpm ();            delete m_debugMapMode;             m_debugMapMode = NULL;
        m_debugMapFlat->SavePpm ();            delete m_debugMapFlat;             m_debugMapFlat = NULL;
        m_debugMapBuffer->SavePpm ();          delete m_debugMapBuffer;           m_debugMapBuffer = NULL;
        m_debugMapTargetBitrate->SavePpm ();   delete m_debugMapTargetBitrate;    m_debugMapTargetBitrate = NULL;
    }

    if (enMinilog) {
        fclose (minilogFile);
    }

    //! write reconstructed image to disk !//
    if (m_isRecFileSpecified) {
        m_imageFileRec = new ImageFileIo (m_recFile, m_fileFormatRec);
		m_imageFileRec->SetCommandLineOptions (m_dpxOptions);
        m_imageFileRec->CopyParams (m_imageFileIn);
        m_imageFileRec->WriteData (recVideoFrame);
        delete m_imageFileRec;
    }
    delete pps;
    delete m_imageFileIn;

    //! clean up !//
    if (enEncTracer) {
        fclose (encTracer);
    }
    if (m_calcPsnr) {
        fclose (m_psnrFile);
    }

    bitstreamFile.close ();
    orgVideoFrame->Destroy ();      delete orgVideoFrame;       orgVideoFrame = NULL;
    recVideoFrame->Destroy ();      delete recVideoFrame;       recVideoFrame = NULL;
    return true;
}

/**
    Select best coding mode for current block

    @param rates vector of coding rates for each coding mode
    @param distortions vector of distortion (SAD) for each coding mode
    @param rdCosts vector of rdCost for each coding mode
    @param rc rate control object
    @param isLastBlock flag to indicate last block in slice
    @param minBlockBits worst-case coding rate for MPPF mode, used to guarantee at least one mode available at all times
    @return selected coding mode
*/

ModeType EncTop::SelectCurBlockMode (std::vector<Int>& rates, std::vector<Int>& distortions, std::vector<Int>& rdCosts, RateControl* rc, Bool isLastBlock, Int minBlockBits, Pps *pps, std::vector<Bool> &modeSelectionIsValid)
{
    UInt numValidModes = 0;
    ModeType bestMode = kModeTypeUndefined;

    // check valid for each mode
    std::vector<Bool> isValid (g_numModes, true);
    for (UInt mode = 0; mode < g_numModes; mode++) {
        if (rdCosts.at (mode) == LARGE_INT) {
            isValid[mode] = false;
            modeSelectionIsValid[mode] = false;
            continue;
        }
        Int curModeRate = rates.at (mode);
        UInt64 numBitsLeft = (isLastBlock) ? rc->GetSliceBitsRemainingLastBlock () : rc->GetSliceBitsRemaining () - curModeRate;
        Int numBlocksLeft = (Int)(rc->GetSlicePixelsRemaining () / (m_blkWidth * m_blkHeight));

        //! check for rate buffer overflow (A) and underflow (B)
        Bool discardModeA = rc->ModeBufferCheck (curModeRate) == 1;
        Bool discardModeB = rc->ModeBufferCheck (curModeRate) == 2;

        //! check for proper bit allocation over remainder of slice !//
		Bool discardModeC;
		if (pps->GetVersionMinor () >= 2) {
            discardModeC = numBitsLeft < ((numBlocksLeft - 1) * minBlockBits);
		}
		else {
			discardModeC = numBitsLeft < (numBlocksLeft * minBlockBits);
		}

        //! set mode as invalid if any checks failed !//
        if (discardModeA || discardModeB || discardModeC) {
            isValid[mode] = false;
            modeSelectionIsValid[mode] = false;
        }
    }

    // compare the non-fallback modes (XFORM, BP, MPP)
    if (isValid[kModeTypeTransform]) {
        numValidModes++;
        if (isValid[kModeTypeBP] && isValid[kModeTypeMPP]) { // valid mode(s): XFORM, BP, MPP
            numValidModes += 2;
            if (rdCosts[kModeTypeTransform] <= rdCosts[kModeTypeBP] && rdCosts[kModeTypeTransform] <= rdCosts[kModeTypeMPP]) {
                bestMode = kModeTypeTransform;
            }
            else {
                bestMode = (rdCosts[kModeTypeBP] <= rdCosts[kModeTypeMPP]) ? kModeTypeBP : kModeTypeMPP;
            }
        }
        else if (isValid[kModeTypeBP]) { // valid mode(s): XFORM, BP
            numValidModes++;
            bestMode = (rdCosts[kModeTypeTransform] <= rdCosts[kModeTypeBP]) ? kModeTypeTransform : kModeTypeBP;
        }
        else if (isValid[kModeTypeMPP]) { // valid mode(s): XFORM, MPP
            numValidModes++;
            bestMode = (rdCosts[kModeTypeTransform] <= rdCosts[kModeTypeMPP]) ? kModeTypeTransform : kModeTypeMPP;
        }
        else { // valid mode(s): XFORM
            bestMode = kModeTypeTransform;
        }
    }
    else {
        if (isValid[kModeTypeBP] && isValid[kModeTypeMPP]) { // valid mode(s): BP, MPP
            numValidModes += 2;
            bestMode = (rdCosts[kModeTypeBP] <= rdCosts[kModeTypeMPP]) ? kModeTypeBP : kModeTypeMPP;
        }
        else if (isValid[kModeTypeBP]) { // valid mode(s): BP
            numValidModes++;
            bestMode = kModeTypeBP;
        }
        else if (isValid[kModeTypeMPP]) { // valid mode(s): MPP
            numValidModes++;
            bestMode = kModeTypeMPP;
        }
    }

    // compare fallback modes
    if (numValidModes == 0) {
        if (isValid[kModeTypeBPSkip] && isValid[kModeTypeMPPF]) { // valid mode(s): MPPF, BPSKIP
            bestMode = (rdCosts.at (kModeTypeMPPF) < rdCosts.at (kModeTypeBPSkip)) ? kModeTypeMPPF : kModeTypeBPSkip;
        }
        else if (isValid[kModeTypeBPSkip]) { // valid mode(s): BPSKIP
            bestMode = kModeTypeBPSkip;
        }
        else if (isValid[kModeTypeMPPF]) { // valid mode(s): MPPF
            bestMode = kModeTypeMPPF;
        }
        else { // valid mode(s): none
            fprintf (stderr, "EncTop::SelectCurBlockMode(): no valid modes found.\n");
            exit (EXIT_FAILURE);
        }
    }
    return bestMode;
}

/**
    Compute the rcFullness lambda parameter

    @param bf buffer fullness state
    @param pps PPS container
    @return lambda buffer fullness lagrangian parameter
    */
Int EncTop::ComputeLambdaBufferFullness (Int bf, Pps *pps) {
    Int scale = (g_rc_BFRangeBits - g_rc_bfLambdaLutBitsMod);
    Int maxIndexMod = (1 << g_rc_bfLambdaLutBits) - 1;
    Int maxIndex = (1 << g_rc_bfLambdaLutBitsMod) - 1;

    // interpolate lambda LUT
    Int index = Clip3<Int> (0, maxIndex, (bf + (1 << (scale - 1))) >> scale);
    Int interpBits = g_rc_bfLambdaLutBitsMod - g_rc_bfLambdaLutBits;
    Int indexMod = index & 0x3;
    Int idx0 = Clip3<Int> (0, maxIndexMod, (index >> interpBits));
    Int idx1 = Clip3<Int> (0, maxIndexMod, (index >> interpBits) + 1);
    Int x_0 = pps->GetLutLambdaBufferFullness (idx0);
    Int x_1 = pps->GetLutLambdaBufferFullness (idx1);
    Int w_0 = (1 << interpBits) - indexMod;
    Int w_1 = indexMod;
    Int bufferMul = (w_0 * x_0 + w_1 * x_1 + 2) >> interpBits;
    return bufferMul;
}

/**
    Initialize data for slice, padding to integer number of 8x2 blocks

    @param orgVideoFrame VideoFrame object for current picture
    @param curSliceFrame VideoFrame object for current slice only
    @param startY starting position y of slice in picture
    @param startX starting position x of slice in picture
    @param finalSliceWidth slice width after x-padding
    @param finalSliceHeight slice width after y-padding
    @param padX padding pixels in x direction
    @param padY padding pixels in y direction
*/
Void EncTop::InitSliceData (VideoFrame* orgVideoFrame, VideoFrame* curSliceFrame, Int startY, Int startX, Int finalSliceWidth, Int finalSliceHeight, Int padX, Int padY) {
    for (Int k = 0; k < g_numComps; k++) {
        Int srcWidth = (finalSliceWidth - padX) >> g_compScaleX[k][m_chromaFormat];
        Int srcHeight = (finalSliceHeight - padY) >> g_compScaleY[k][m_chromaFormat];
        Int srcStride = orgVideoFrame->getWidth (k);
        Int dstStride = (finalSliceWidth) >> g_compScaleX[k][m_chromaFormat];
        Int sy = startY >> g_compScaleY[k][m_chromaFormat];
        Int sx = startX >> g_compScaleX[k][m_chromaFormat];

        // compute per-component padding
        Int compPadX = 0;
        Int compPadY = 0;
        if (padX > 0) {
            compPadX = dstStride - srcWidth;
        }
        if (padY > 0) {
            compPadY = padY >> g_compScaleY[k][m_chromaFormat];
        }

        // valid source data
        Int* pSrc = orgVideoFrame->getBuf (k);
        Int* pDst = curSliceFrame->getBuf (k);
        for (Int y = 0; y < srcHeight; y++) {
            for (Int x = 0; x < srcWidth; x++) {
                UInt64 srcLoc = (UInt64)(sy + y) * (UInt64)(srcStride) + (UInt64)(sx + x);
                UInt64 dstLoc = (UInt64)(y) * (UInt64)(dstStride) + (UInt64)(x);
                pDst[dstLoc] = pSrc[srcLoc];
            }
        }

        // padding in x direction
        if (compPadX > 0) {
            for (Int y = 0; y < srcHeight; y++) {
                for (Int x = srcWidth; x < (srcWidth + compPadX); x++) {
                    UInt64 srcLoc = (UInt64)(sy + y) * (UInt64)(srcStride) + (UInt64)(sx + srcWidth - 1);
                    UInt64 dstLoc = (UInt64)(y) * (UInt64)(dstStride) + (UInt64)(x);
                    pDst[dstLoc] = pSrc[srcLoc];
                }
            }
        }

        // padding in y direction
        if (compPadY > 0) {
            for (Int y = srcHeight; y < (srcHeight + compPadY); y++) {
                for (Int x = 0; x < (srcWidth + compPadX); x++) {
                    UInt64 srcLoc = (UInt64)(srcHeight - 1) * (UInt64)(dstStride) + (UInt64)(x);
                    UInt64 dstLoc = (UInt64)(y) * (UInt64)(dstStride) + (UInt64)(x);
                    pDst[dstLoc] = pDst[srcLoc];
                }
            }
        }
    }
}

/**
    Print info for current block selected mode (for debugTracer)

    @param mode selected coding mode
    @param y y-position
    @param x x-position
    @param qp quantization parameter
    @param nBits number of bits used to code previous block
    @param prevBitsNoPadding number of bits used to code previous block (not including underflow prevention padding)
*/
Void EncTop::PrintEncodeInfo (ModeType mode, UInt y, UInt x, UInt qp, UInt nBits, UInt prevBitsNoPadding) {
    if (enEncTracer) {
        fprintf (encTracer, "BEST_MODE: %s, qp = %d\n", g_codingModeNames[mode].c_str (), qp);
        UInt padding = nBits - prevBitsNoPadding;
        if (padding > 0) {
            fprintf (encTracer, "BEST_MODE: bits = %d + (%d padding) = %d\n", prevBitsNoPadding, padding, nBits);
        }
        else {
            fprintf (encTracer, "BEST_MODE: bits = %d\n", nBits);
        }
    }
}

/**
    Slice is padded to be an integer number of 8x2 blocks

    @return false if any error in slice configuration, otherwise true
    */
Bool EncTop::CalculateSlicePadding () {
    Bool status = true;
    //! sanity checks !//
    if (m_chromaFormat == k420 && m_srcHeight % 2 == 1) {
        fprintf (stderr, "EncTop::CalculateSlicePadding(): 4:2:0 content must have even source height.\n");
        status = false;
    }
    if (m_slicesPerLine * m_sliceWidth != m_srcWidth) {
        fprintf (stderr, "EncTop::CalculateSlicePadding(): source width must be divisible by slicesPerLine.\n");
        status = false;
    }

    //! note: (v1.2.4) chroma width check and warning message have been updated !//
    if ((m_chromaFormat == k420 || m_chromaFormat == k422) && m_sliceWidth % 2 == 1) {
        fprintf (stderr, "EncTop::CalculateSlicePadding(): 4:2:0/4:2:2 content must have even slice width.\n");
        status = false;
    }

    //! slice padding in x dimension !//
    UInt origSliceWidth = m_srcWidth / m_slicesPerLine;
    m_sliceWidth = ((origSliceWidth + 7) >> 3) << 3;
    m_slicePadX = m_sliceWidth - origSliceWidth;

    //! slice padding in y dimension (bottom row of slices) !//
    Int numSlicesY = ((m_srcHeight - 1) / m_sliceHeight) + 1;
    Int finalSourceHeight = numSlicesY * m_sliceHeight;
    m_bottomSlicePadY = finalSourceHeight - m_srcHeight;

	//! sanity checks !//
	if (m_sliceWidth < g_minSliceWidth) {
		fprintf (stderr, "EncTop::CalculateSlicePadding(): desired slice width (%d) is less than supported minimum of %d.\n",
			m_sliceWidth, g_minSliceWidth);
		status = false;
	}
	if (m_sliceHeight < g_minSliceHeight) {
		fprintf (stderr, "EncTop::CalculateSlicePadding(): desired slice height (%d) is less than supported minimum of %d.\n",
			m_sliceHeight, g_minSliceHeight);
		status = false;
	}
	if (m_sliceHeight * m_sliceWidth < g_minSlicePixels) {
		fprintf (stderr, "EncTop::CalculateSlicePadding(): desired slice num px (%d) is less than supported minimum of %d.\n",
			m_sliceHeight * m_sliceWidth, g_minSlicePixels);
		status = false;
	}

    return status;
}

Bool EncTop::VersionChecks (Pps* pps) {
	ChromaFormat cf = pps->GetChromaFormat ();
	UInt version_major = pps->GetVersionMajor ();
	UInt version_minor = pps->GetVersionMinor ();
	UInt slice_width = pps->GetSliceWidth ();
	UInt bpc = pps->GetBitDepth ();
	UInt bpp = pps->GetBpp ();
	UInt ssm_max_se_size = pps->GetSsmMaxSeSize ();
	UInt pps_min_step_size = pps->GetMppMinStepSize ();
	UInt chunk_adj_bits = (pps->GetChunkAdjBits () + 1) >> 1;

	//! check if BPP > max !//
	UInt max_bpp = (Int)(bpc * (cf == k444 ? 3.0 : cf == k422 ? 2.0 : 1.5));
	if ((bpp >> 4) > max_bpp) {
		fprintf (stderr, "EncTop::VersionChecks(): max_bpp (%d) exceeded.\n", max_bpp);
		return false;
	}

	switch (version_major) {
		case 1:
			switch (version_minor) {
				case 0: {
					fprintf (stderr, "EncTop::VersionChecks(): VDC-M v1.0 is deprecated.\n");
					return false;
				}
				case 1: {
					//! check if BPP > 12 !//
					if ((bpp >> 4) > 12) {
						fprintf (stderr, "EncTop::VersionChecks(): VDC-M v1.1 is deprecated for BPP > 12.\n");
						return false;
					}

                    //! check if slicesPerLine is a power of 2 !//
                    if (floor (log2 ((Double)m_slicesPerLine) != ceil (log2 ((Double)m_slicesPerLine)))) {
                        fprintf (stderr, "EncTop::VersionChecks(): VDC-M v1.1 is deprecated for use case: slicesPerLine != 2 ^ n.\n");
                        return false;
                    }

					//! check if chunk_adj_bits > 0 !//
					if ((bpp * slice_width) % 128 > 0) {
						fprintf (stderr, "EncTop::VersionChecks(): VDC-M v1.1 is deprecated for use case: mod(bpp * slice_width, 128) > 0.\n");
						return false;
					}

					//! checks for mpp_min_step_size, ssm_max_se_size !//
					UInt samples_per_substream = cf == k444 ? 12 : cf == k422 ? 8 : 6;
					UInt header_min = bpc == 8 ? 5 : 6;
					UInt header_max = bpc == 8 ? 10 : 11;
					UInt min_step_size = 0;
					if (cf == k444) {
						switch (bpc) {
							case 10:
								min_step_size = (ssm_max_se_size == 128 ? 1 : 0);
								break;
							case 12:
								min_step_size = (ssm_max_se_size == 128 ? 3 : 1);
								break;
						}
					}

					//! check if mpp_min_step_size is small enough to prevent underflow !//
					UInt min_step_size_req = bpc - (UInt)ceil ((Double)(bpp + chunk_adj_bits - header_min) / (4 * samples_per_substream));
					if (min_step_size > min_step_size_req) {
						fprintf (stderr, "EncTop::VersionChecks(): unsupported config! (%s/%dbpc/%dbpp, mpp_min_step_size = %d)...\n --> required: mpp_min_step_size <= %d.\n",
							g_chromaFormatNames[cf].c_str (), bpc, (bpp >> 4), min_step_size, min_step_size_req);
						return false;
					}

					//! check if ssm_max_se_size is large enough to contain worst case MPP syntax element !//
					UInt ssm_max_se_size_req = 8 * (UInt)ceil (((double)(header_max + (bpc - min_step_size) * samples_per_substream)) / 8);
					if (ssm_max_se_size < ssm_max_se_size_req) {
						fprintf (stderr, "EncTop::VersionChecks(): unsupported config! (%s/%dbpc/%dbpp, ssm_max_se_size = %d, min_step_size = %d)...\n --> required: ssm_max_se_size >= %d.\n",
							g_chromaFormatNames[cf].c_str (), bpc, (bpp >> 4), ssm_max_se_size, min_step_size, ssm_max_se_size_req);
						return false;
					}

					break;
				}
				case 2: {
					UInt samples_per_substream = (cf == k444 ? 12 : cf == k422 ? 8 : 6);
					UInt header_min = (bpc == 8 ? 5 : 6);
					UInt header_max = (bpc == 8 ? 10 : 11);

					//! check if mpp_min_step_size is small enough to prevent underflow !//
					UInt min_step_size_req = bpc - (UInt)ceil ((Double)(bpp + chunk_adj_bits - header_min) / (4 * samples_per_substream));
					if (pps_min_step_size > min_step_size_req) {
						fprintf (stderr, "EncTop::VersionChecks(): unsupported config! (%s/%dbpc/%dbpp, mpp_min_step_size = %d)...\n --> required: mpp_min_step_size <= %d.\n",
							g_chromaFormatNames[cf].c_str (), bpc, (bpp >> 4), pps_min_step_size, min_step_size_req);
						return false;
					}

					//! check if ssm_max_se_size is large enough to contain worst case MPP syntax element !//
					UInt ssm_max_se_size_req = 8 * (UInt)ceil (((double)(header_max + (bpc - pps_min_step_size) * samples_per_substream)) / 8);
					if (ssm_max_se_size < ssm_max_se_size_req) {
						fprintf (stderr, "EncTop::VersionChecks(): unsupported config! (%s/%dbpc/%dbpp, mpp_min_step_size = %d, ssm_max_se_size = %d)...\n --> required: ssm_max_se_size >= %d.\n",
							g_chromaFormatNames[cf].c_str (), bpc, (bpp >> 4), pps_min_step_size, ssm_max_se_size, ssm_max_se_size_req);
						return false;
					}
					break;
				}
			}
			break;
		default:
			fprintf (stderr, "EncTop::VersionChecks(): unsupported version: %d.%d.\n", version_major, version_minor);
			return false;
	}
	return true;
}