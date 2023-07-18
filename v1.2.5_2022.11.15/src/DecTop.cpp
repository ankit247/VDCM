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

 /* Copyright (c) 2015-2017 MediaTek Inc., BP-Skip mode/RDO weighting are modified and/or implemented by MediaTek Inc. based on DecTop.cpp
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
#include "DecTop.h"
#include "IntraPrediction.h"
#include "RateControl.h"
#include "Misc.h"
#include "MppfMode.h"
#include "MppMode.h"
#include "BpMode.h"
#include "Mode.h"
#include "TransMode.h"
#include "Pps.h"
#include "Ssm.h"
#include "Bitstream.h"
#include "ImageFileIo.h"
#include <iostream>
#include <fstream>

//! for debugTracer !//
extern Bool enDecTracer;
extern FILE* decTracer;

//! for mode distrubtion tracking !//
Int decModeCounter[g_numModes] = { 0 };
Int decGlobalBlockCounter = 0;

/**
	Decode one slice

	@param pps
	@param startY
	@param startX
	@param sliceBits
	@param recVideoFrame
	*/
Void DecTop::DecodeSlice (Pps* pps, UInt startY, UInt startX, BitstreamInput* sliceBits, VideoFrame* recVideoFrame) {
	Bool isEncoder = false;
	ModeType mode = kModeTypeUndefined;
	ModeType prevMode = kModeTypeUndefined;
	FlatnessType curFlatnessType;
	std::vector<Mode*> codingModes (g_numModes);
	Int bitCnt = 0;
	Int prevBlockBits = 0;
	Int prevQp = g_rcInitQp_444;
	Int finalSliceWidth = pps->GetSliceWidth ();
	Int padX = m_slicePadX;
	Int origSliceWidth = finalSliceWidth - padX;
	Int finalSliceHeight = pps->GetSliceHeight ();
	UInt padY = (startY + finalSliceHeight > m_srcHeight ? startY + finalSliceHeight - m_srcHeight : 0);
	Int origSliceHeight = finalSliceHeight - padY;
	Int numBlksX = m_sliceWidth >> 3;
	Int numBlksY = m_sliceHeight >> 1;

	if (m_debugTracer) {
		fprintf (decTracer, "SLICE_START: %4d, %4d\n", startY, startX);
	}

	Pps *curSlicePps = pps;

	//! set up SSM !//
	SubStreamMux* ssm = new SubStreamMux (false, g_ssm_numSubstreams, pps->GetSsmMaxSeSize ());
	std::vector <UInt> ssmDecFunnelShifterFullnessTracker (g_ssm_numSubstreams, 0);
	ssm->SetNewBitsIn (sliceBits);

	//! set up video frame !//
	VideoFrame *curSliceRecFrame = new VideoFrame ();
	VideoFrame *curSliceRecExtra = new VideoFrame ();
	if (m_origSrcCsc == kRgb) {
		curSliceRecExtra->Create (finalSliceWidth, 2, m_chromaFormat, padX, padY, kYCoCg, m_bitDepth);
	}
	RateControl* rc = new RateControl (m_chromaFormat, ssm, curSlicePps);
	curSliceRecFrame->Create (finalSliceWidth, finalSliceHeight, m_chromaFormat, padX, padY, m_origSrcCsc, m_bitDepth);
	Int numBlkRowsPerSlice = finalSliceHeight / m_blkHeight;
	Int prevBits = 0; // for tracking #bits/block at decoder side
	Int postBits = 0; // for tracking #bits/block at decoder side
	rc->InitSlice (startX, startY);

	ModeType modeNext;
	FlatnessType nextFlatnessType;
	UInt mppfIndexNextBlock;

	ColorSpace targetCsc = (m_origSrcCsc == kRgb ? kYCoCg : kYCbCr);

	// setup - entropy coder
	EntropyCoder* ec = new EntropyCoder (m_blkWidth, m_blkHeight, m_chromaFormat);
	ec->SetRcStuffingBits (curSlicePps->GetRcStuffingBits ());

	// setup - transform mode
	TransMode *transMode = new TransMode (m_bpp, m_bitDepth, false, m_chromaFormat, m_origSrcCsc, targetCsc);
	transMode->InitDecoder (startY, startX, curSliceRecFrame, rc, ec, m_blkWidth, m_blkHeight, ssm, curSlicePps);
	transMode->SetRecBufferYCoCg (curSliceRecExtra);
	codingModes[kModeTypeTransform] = transMode;

	// setup - block prediction mode
	BpMode* bpMode = new BpMode (m_bpp, m_bitDepth, false, m_chromaFormat, m_origSrcCsc, targetCsc, false);
	BpMode* bpModeSkip = new BpMode (m_bpp, m_bitDepth, false, m_chromaFormat, m_origSrcCsc, targetCsc, true);
	bpMode->InitDecoder (startY, startX, curSliceRecFrame, rc, ec, m_blkWidth, m_blkHeight, ssm, curSlicePps);
	bpModeSkip->InitDecoder (startY, startX, curSliceRecFrame, rc, ec, m_blkWidth, m_blkHeight, ssm, curSlicePps);
	bpMode->SetRecBufferYCoCg (curSliceRecExtra);
	bpModeSkip->SetRecBufferYCoCg (curSliceRecExtra);
	codingModes[kModeTypeBP] = bpMode;
	codingModes[kModeTypeBPSkip] = bpModeSkip;

	// setup - midpoint prediction mode
	MppMode* mppMode = new MppMode (m_bpp, m_bitDepth, false, m_chromaFormat, m_origSrcCsc, g_mtkMppColorSpaceRdoQpThreshold);
	mppMode->InitDecoder (startY, startX, curSliceRecFrame, rc, ec, m_blkWidth, m_blkHeight, ssm, curSlicePps);
	mppMode->SetRecBufferYCoCg (curSliceRecExtra);
	mppMode->InitNextBlockCache ();
	codingModes[kModeTypeMPP] = mppMode;

	// setup - midpoint prediction fallback mode
	MppfMode* mppfMode = new MppfMode (m_bpp, m_bitDepth, false, m_chromaFormat, m_origSrcCsc);
	mppfMode->InitDecoder (startY, startX, curSliceRecFrame, rc, ec, m_blkWidth, m_blkHeight, ssm, curSlicePps);
	mppfMode->SetRecBufferYCoCg (curSliceRecExtra);
	mppfMode->InitNextBlockCache ();
	codingModes[kModeTypeMPPF] = mppfMode;

	// loop over rows in slice
	Int prevBlockBitsWithoutPadding = 0;

	// in this case, we are going to parse SS0 one SE ahead of other substreams
	ssm->DecodeBlock ();

	// initial state for substream 0 decoder funnel shifter after requesting a muxword
	UInt ssmFunnelShifter_0 = ssm->GetDecFunnelShifterFullness (0);

	// parsing
	modeNext = DecodeModeHeader (ssm, prevMode);        // mode header
	nextFlatnessType = DecodeFlatnessType (ssm);    // flatness header

	// parse and store additional data from substream 0
	switch (modeNext) {
		case kModeTypeTransform:
			transMode->DecodeBestIntraPredictor ();
			break;
		case kModeTypeBP:
			bpMode->DecodeBpvNextBlock ();
			break;
		case kModeTypeBPSkip:
			bpModeSkip->DecodeBpvNextBlock ();
			break;
		case kModeTypeMPP:
			mppMode->ParseCsc ();
			mppMode->ParseStepSize ();
			mppMode->DecodeMppSuffixBits (true);
			break;
		case kModeTypeMPPF:
			mppfIndexNextBlock = (m_origSrcCsc == kYCbCr ? 0 : DecodeMppfType (ssm));
			mppfMode->DecodeMppfSuffixBits (true);
			break;
		default:
			fprintf (stderr, "QC_SSM_SUBSTREAM_ADVANCE: mode %d not expected at decoder.\n", modeNext);
			exit (EXIT_FAILURE);
	}

	// parsed bits for substream 0
	UInt nextBlockBitsSsm0 = ssmFunnelShifter_0 - ssm->GetDecFunnelShifterFullness (0);

	// increment block idx to 1.  Substreams 1-3 will request muxword during next block time
	//ssm->PrintCurrentState ();
	ssm->IncrementBlockIdx ();

	for (Int r = 0; r < numBlksY; r++) {
		Bool isFls = (r == 0 ? true : false);
		Int blk_y = (m_blkHeight * r);  // position relative to current slice
		Int blkSrc_y = startY + blk_y;  // position relative to current frame

		// loop over blocks in row
		for (Int c = 0; c < numBlksX; c++) {
			if (c == (numBlksX - 1)) {
				bpMode->SetIsNextBlockFls (false);
				bpModeSkip->SetIsNextBlockFls (false);
			}
			Int blk_x = (m_blkWidth * c);   // position relative to current slice
			Int blkSrc_x = startX + blk_x;  // position relative to current frame

			if (m_debugTracer) {
				fprintf (decTracer, "BLOCK_START: %4d, %4d\n", blk_y, blk_x);
			}

			rc->UpdateBufferFullness (prevBlockBits);
			rc->UpdateRcFullness ();

			Bool isLastBlock = (c == numBlksX - 1) && (r == numBlksY - 1);
			if (isLastBlock) {
				ssm->SetIsDecodingFinished (0, true);
			}
			// request muxwords if needed by decoder funnel shifters
			ssm->DecodeBlock ();

			// decoder funnel shifter state prior to decoding current block
			for (UInt ssmIdx = 0; ssmIdx < ssm->GetNumSubStreams (); ssmIdx++) {
				ssmDecFunnelShifterFullnessTracker.at (ssmIdx) = ssm->GetDecFunnelShifterFullness (ssmIdx);
			}

			bpMode->BpvSwap ();
			bpModeSkip->BpvSwap ();
			mppMode->CscBitSwap ();
			mppMode->NextBlockStepSizeSwap ();
			mppfMode->MppfIndexSwap ();
			mppMode->CopyCachedSuffixBits ();
			mppfMode->CopyCachedSuffixBits ();
			transMode->NextBlockIntraSwap ();

			mode = modeNext;
			curFlatnessType = nextFlatnessType;

			if ((r == 0) && (c == numBlksX - 1)) {
				transMode->SetNextBlockIsFls (false);
			}

			if (!isLastBlock) {
				modeNext = DecodeModeHeader (ssm, mode);
				nextFlatnessType = DecodeFlatnessType (ssm);
			}

			if (!isLastBlock) {
				switch (modeNext) {
					case kModeTypeTransform:
						transMode->DecodeBestIntraPredictor ();
						break;
					case kModeTypeBP:
						bpMode->DecodeBpvNextBlock ();
						break;
					case kModeTypeMPP:
						mppMode->ParseCsc ();
						mppMode->ParseStepSize ();
						mppMode->DecodeMppSuffixBits (true);
						break;
					case kModeTypeMPPF:
						mppfMode->SetMppfIndexNextBlock (m_origSrcCsc == kYCbCr ? 0 : DecodeMppfType (ssm));
						mppfMode->DecodeMppfSuffixBits (true);
						break;
					case kModeTypeBPSkip:
						bpModeSkip->DecodeBpvNextBlock ();
						break;
				}
			}

			decGlobalBlockCounter++;
			decModeCounter[mode]++;

			// reset qpUpdateMode
			rc->SetQpUpdateMode (0);

			// get the bitBudget
			Int targetBits;
			Bool lastBlock = false;
			rc->CalcTargetRate (targetBits, lastBlock, (r == 0));
			rc->SetIsFls (r == 0);

			//! update QP !//
			if (prevBlockBits > 0) {
				rc->UpdateQp (prevBlockBitsWithoutPadding, targetBits, r == 0, curFlatnessType);
			}

			//! debugTracer - rate control info !//
			if (enDecTracer) {
				if (curFlatnessType != kFlatnessTypeUndefined) {
					fprintf (decTracer, "FLATNESS: %s\n", g_flatnessTypeNames[curFlatnessType].c_str ());
				}
				Int numRcPaddingBits = prevBlockBits - prevBlockBitsWithoutPadding;
				fprintf (decTracer, "RC: prevBlockRate = %d (%d padding bits)\n", prevBlockBits, numRcPaddingBits);
				fprintf (decTracer, "RC: targetRate = %d\n", targetBits);
				fprintf (decTracer, "RC: bufferFullness = %d\n", rc->GetBufferFullness ());
				fprintf (decTracer, "RC: rcFullness = %d\n", rc->GetRcFullness ());
				fprintf (decTracer, "RC: qp = %d\n", rc->GetQp ());
				Bool enableUnderflowPrevention = rc->CheckUnderflowPrevention ();
				if (enableUnderflowPrevention) {
					fprintf (decTracer, "RC: underflow prevention mode enabled\n");
				}
			}

			//! check underflow prevention !//
			Bool enableUnderflowPrevention = rc->CheckUnderflowPrevention ();

			//! decode current block mode !//
			codingModes[mode]->SetUnderflowMode (enableUnderflowPrevention ? true : false);
			codingModes[mode]->Update (blk_x, blk_y, isFls, prevMode);
			codingModes[mode]->Decode ();

			if (m_origSrcCsc == kRgb) {
				CopyYCoCgDataToRgbBuffer (curSliceRecExtra, curSliceRecFrame, blk_y, blk_x);
			}

			prevMode = mode;

			// handle the one block time shift for substream 0
			UInt curBlockBitsSsm0 = nextBlockBitsSsm0;
			postBits += curBlockBitsSsm0;
			nextBlockBitsSsm0 = ssmDecFunnelShifterFullnessTracker.at (0) - ssm->GetDecFunnelShifterFullness (0);
			for (UInt ssmIdx = 1; ssmIdx < ssm->GetNumSubStreams (); ssmIdx++) {
				postBits += (ssmDecFunnelShifterFullnessTracker.at (ssmIdx) - ssm->GetDecFunnelShifterFullness (ssmIdx));
			}

			// check for rate buffer underflow.  If a rate buffer underflow would occur, then the encoder
			// will have padded zero bits among the substreams (in order).
			UInt curBlockBits = postBits - prevBits;
			prevBits = postBits; // for next block
			prevBlockBitsWithoutPadding = curBlockBits;

			if (enableUnderflowPrevention && (prevMode == kModeTypeTransform || prevMode == kModeTypeBP)) {
				prevBlockBitsWithoutPadding -= 9 * curSlicePps->GetRcStuffingBits ();
			}

			// update the balance FIFO state
			std::vector <UInt> bitsSsm (ssm->GetNumSubStreams (), 0);

			for (UInt ssmIdx = 1; ssmIdx < ssm->GetNumSubStreams (); ssmIdx++) {
				bitsSsm.at (ssmIdx) = (ssmDecFunnelShifterFullnessTracker.at (ssmIdx) - ssm->GetDecFunnelShifterFullness (ssmIdx));
			}

			prevBlockBits = curBlockBits;
			if (r == 0 && c == 0) {
				rc->SetIsFirstBlockInSlice (false);
			}

			//! debugTracer - mode info !//
			if (enDecTracer) {
				fprintf (decTracer, "BEST_MODE: %s, qp = %d\n", g_codingModeNames[mode].c_str (), codingModes[mode]->GetQP ());
				UInt padding = prevBlockBits - prevBlockBitsWithoutPadding;
				if (padding > 0) {
					fprintf (decTracer, "BEST_MODE: bits = %d + (%d padding) = %d\n", prevBlockBitsWithoutPadding, padding, prevBlockBits);
				}
				else {
					fprintf (decTracer, "BEST_MODE: bits = %d\n", prevBlockBits);
				}
				fprintf (decTracer, "\n");
			}

			//! increment decoder block index !//
			//ssm->PrintCurrentState ();
			ssm->IncrementBlockIdx ();
		}
	}

	// update RC for sanity checks
	rc->UpdateBufferFullness (prevBlockBits);

	if (m_debugTracer) {
		fprintf (decTracer, "SLICE_END: %4d, %4d\n\n\n", startY, startX);
	}

	// copy reconstructed slice to reconstructed frame
    for (Int k = 0; k < g_numComps; k++) {
        Int sx = startX >> g_compScaleX[k][m_chromaFormat];
        Int sy = startY >> g_compScaleY[k][m_chromaFormat];
        Int sliceHeight = origSliceHeight >> g_compScaleY[k][m_chromaFormat];
        Int sliceWidth = origSliceWidth >> g_compScaleX[k][m_chromaFormat];
        Int *pSrc = curSliceRecFrame->getBuf (k);
        Int *pDst = recVideoFrame->getBuf (k);
        for (Int y = 0; y < sliceHeight; y++) {
            for (Int x = 0; x < sliceWidth; x++) {
                UInt64 srcLoc = (UInt64)(y) * (UInt64)(finalSliceWidth >> g_compScaleX[k][m_chromaFormat]) + (UInt64)(x);
                UInt64 dstLoc = (UInt64)(sy + y) * (UInt64)(m_srcWidth >> g_compScaleX[k][m_chromaFormat]) + (UInt64)(sx + x);
                pDst[dstLoc] = pSrc[srcLoc];
            }
        }
    }

	mppMode->ClearMppSuffixMapping ();
	mppMode->ClearNextBlockCache ();

	// clean up memory
	delete curSliceRecFrame;    curSliceRecFrame = NULL;
	delete curSliceRecExtra;	curSliceRecExtra = NULL;
	delete rc;                  rc = NULL;
	delete ec;                  ec = NULL;
	delete ssm;                 ssm = NULL;

	for (UInt curMode = 0; curMode < codingModes.size (); curMode++) {
		if (!codingModes[curMode]) {
			continue;
		}
		delete codingModes[curMode];
		codingModes[curMode] = NULL;
	}
}

/**
	Decode one picture
	*/
Void DecTop::Decode () {
	enDecTracer = m_debugTracer;
	std::fstream bitstreamFile (m_cmpFile, std::fstream::binary | std::fstream::in);
	if (!bitstreamFile) {
		std::cerr << "\nfailed to open bitstream file " << m_cmpFile << " for reading\n" << std::endl;
		exit (EXIT_FAILURE);
	}
	bitstreamFile.seekg (0, std::ios::end);
	m_sliceBytes = (Int)bitstreamFile.tellg ();
	bitstreamFile.seekg (0, std::ios::beg);

	Pps* pps = new Pps ();
	pps->ParsePpsBitstream (&bitstreamFile);
	pps->InitDecoder ();
	if (m_ppsDumpBytes) {
		pps->DumpPpsBytes ();
	}
	if (m_ppsDumpFields) {
		pps->DumpPpsFields ();
	}
	if (m_ppsDumpOnly) {
		delete pps;
		bitstreamFile.close ();
		exit (EXIT_SUCCESS);
	}

	if (pps->GetVersionMajor () == 1 && pps->GetVersionMinor () == 0) {
		fprintf (stderr, "DecTop::Decode(): VDC-M v1.0 is deprecated.\n");
		exit (EXIT_FAILURE);
	}

	m_blkWidth = 8;
	m_blkHeight = 2;
	m_srcWidth = pps->GetSrcWidth ();
	m_srcHeight = pps->GetSrcHeight ();
	m_sliceWidth = pps->GetSliceWidth ();
	m_sliceHeight = pps->GetSliceHeight ();
	m_bitDepth = pps->GetBitDepth ();
	m_origSrcCsc = pps->GetSrcCsc ();
	m_chromaFormat = pps->GetChromaFormat ();
	m_bpp = pps->GetBpp ();

	VideoFrame* recVideoFrame = new VideoFrame;
	recVideoFrame->Create (m_srcWidth, m_srcHeight, m_chromaFormat, 0, 0, m_origSrcCsc, m_bitDepth);

	// main decoder loop
	if (m_sliceHeight == 0) {
		m_sliceHeight = m_srcHeight;
	}

	if (m_sliceWidth == 0) {
		m_sliceWidth = m_srcWidth;
	}

    if (pps->GetVersionMinor () >= 2) {
        m_numSlicesX = pps->GetSlicesPerLine ();
        m_slicePadX = pps->GetSlicePadX ();
    }
    else {
        UInt spl = 0;
        while (m_sliceWidth * spl < m_srcWidth) {
            spl++;
        }
        m_slicePadX = ((m_sliceWidth * spl) - m_srcWidth) / spl;
        m_numSlicesX = spl;
    }
	m_numSlicesY = ((m_srcHeight - 1) / m_sliceHeight) + 1;
	m_bottomSlicePadY = (m_numSlicesY * m_sliceHeight) - m_srcHeight;

	if (enDecTracer) {
		decTracer = fopen ("debugTracerDecoder.txt", "w");
	}

	//! set up vector of buffers for slice multiplexing !//
	std::vector<BitstreamInput*> sliceBytesContainer;
	for (UInt sliceX = 0; sliceX < m_numSlicesX; sliceX++) {
		sliceBytesContainer.push_back (new BitstreamInput ());
	}
	UInt chunkSize = pps->GetChunkSize ();

	Int origSliceWidth = m_sliceWidth - m_slicePadX;
	UInt numBlockLines = m_sliceHeight >> 1;

	for (UInt sliceY = 0; sliceY < m_numSlicesY; sliceY++) {
		//! perform slice demultiplexing !//
		for (UInt line = 0; line < m_sliceHeight; line++) {
			for (UInt sliceX = 0; sliceX < m_numSlicesX; sliceX++) {
				sliceBytesContainer.at (sliceX)->BytesFromStream (&bitstreamFile, chunkSize);
			}
		}
		//! decode slices !//
		for (UInt sliceX = 0; sliceX < m_numSlicesX; sliceX++) {
			UInt startY = (sliceY * m_sliceHeight);
			UInt startX = (sliceX * origSliceWidth);
			DecodeSlice (pps, startY, startX, sliceBytesContainer.at (sliceX), recVideoFrame);
			sliceBytesContainer.at (sliceX)->Clear ();
		}
	}

	//! clear sliceBytesContainer !//
	for (UInt sliceX = 0; sliceX < m_numSlicesX; sliceX++) {
		delete sliceBytesContainer.at (sliceX);
	}

	//! Write reconstructed file !//
	ImageFileIo* imageFileDec = new ImageFileIo (m_recFile, m_fileFormatDec);
	imageFileDec->SetBaseParams (m_srcWidth, m_srcHeight, m_bitDepth, m_origSrcCsc, m_chromaFormat);
	if (m_fileFormatDec == kFileFormatDpx) {
		imageFileDec->InitDpxParams ();
	}
	imageFileDec->SetCommandLineOptions (m_dpxOptions);
	imageFileDec->WriteData (recVideoFrame);

	if (enDecTracer) {
		fclose (decTracer);
	}

	if (m_dumpModes) {
		//! print mode distribution to stdout !//
		Double xform = 100.0 * (Double)decModeCounter[0] / decGlobalBlockCounter;
		Double bp = 100.0 * (Double)decModeCounter[1] / decGlobalBlockCounter;
		Double mpp = 100.0 * (Double)decModeCounter[2] / decGlobalBlockCounter;
		Double mppf = 100.0 * (Double)decModeCounter[3] / decGlobalBlockCounter;
		Double bpskip = 100.0 * (Double)decModeCounter[4] / decGlobalBlockCounter;
		fprintf (stdout, "\n");
		fprintf (stdout, "Coding mode distrubtion (%d total blocks)\n", decGlobalBlockCounter);
		fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
		fprintf (stdout, "XFORM   : %10.4f%%\n", xform);
		fprintf (stdout, "BP      : %10.4f%%\n", bp);
		fprintf (stdout, "MPP     : %10.4f%%\n", mpp);
		fprintf (stdout, "MPPF    : %10.4f%%\n", mppf);
		fprintf (stdout, "BP-SKIP : %10.4f%%\n", bpskip);
		fprintf (stdout, "----------------------------------------------------------------------------------------------------\n");
	}

	//! clean up !//
	recVideoFrame->Destroy ();
	delete recVideoFrame;   recVideoFrame = NULL;
	delete pps; pps = NULL;
    delete imageFileDec;
	bitstreamFile.close ();
}

/**
	Calc slice padding from PPS parameters
	*/
Bool DecTop::DecoderCalcSlicePadding () {
	Bool status = true;

	//! determine number of slices in y direction, padding for bottom slice !//
	m_numSlicesY = ((m_srcHeight - 1) / m_sliceHeight) + 1;	
	m_bottomSlicePadY = (m_numSlicesY * m_sliceHeight) - m_srcHeight;
		
	//! determine number of slices per line and x padding !//
	UInt spl = 0;
	while ((m_sliceWidth << spl) < m_srcWidth) {
		spl++;
	}
	m_slicePadX = ((m_sliceWidth << spl) - m_srcWidth) >> spl;
	m_numSlicesX = (1 << spl);

	return status;
}
