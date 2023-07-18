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

#ifndef __RATE_CONTROL__
#define __RATE_CONTROL__

#include <math.h>
#include "TypeDef.h"
#include "Block.h"
#include "Pps.h"
#include "Ssm.h"

class EncTop;

class RateControl
{
protected:
    Int m_blockMaxNumPixels;                // Maximum number of pixel in a block (normally 16 for 8x2 block)
    UInt64 m_sliceBitsTotal;                // Max. number of bits that can be used for the current slice.
    UInt m_sliceBitsCurrent;                // Number of bits already used
    Int m_numBlocksInSlice;                 // Total number of blocks in slice
    Int m_numBlocksInLine;                  // Total number of blocks in a Line
    Int m_numBlocksCoded;                   // Number of block coded
    Int m_numPixelsCoded;                   // number of pixels coded in the current slice
    Bool m_isBufferTxStart;                 // indicates whether buffer transmission is started or not
    Int m_srcWidth;                         // width of the source
    Int m_srcHeight;                        // Height of the source
    Int m_slicePixelsRemaining;             // The number of slice pixels that remain to be coded.
    Int m_slicePixelsTotal;                 // The number of slice pixels that remain to be coded.
    Int m_sliceHeight;                      // Slice height
    Int m_sliceWidth;                       // Slice width
    Int m_sliceStartX;                      // starting position of current slice (X)
    Int m_sliceStartY;                      // starting position of current slice (Y)
    Bool m_isFirstBlockInSlice;             //
    Int m_sliceIdx;                         // current slice number
    Bool m_isFirstSliceInFrame;             //
    ChromaFormat m_chromaFormat;            // chroma subsampling
    Int m_byteAlignBits;                    // used for debugging (stores the number of bits added to make the final bitstream byte aligned)
    Bool m_isFls;                           //
    Int m_bitDepth;                         // Input bitdepth
    Int m_curBlockNumPixels;                // number of pixels in the current block
    Int m_blockBits;                        // bits spent on the current block
    Pps *m_pps;                             //

    //! bitrate !//
    Int m_bpp;                              // compressed bitrate
    Int m_avgBlockBits;                     // Average number of bits per block
    Int m_fracAccumAvgBlockBits;            // accumulated fractional bits while calculating avgBlockBits

    //! RC update loop !//
    Int m_rcBufferInitSize;                 // initial point of the buffer fullness (used to calculate TxDelay)
    Int m_rcBufferMaxSize;                  // maximum fullness of the rate buffer
    Int m_bufferFullness;                   // fullness of the rate buffer
    Int m_rcFullness;                       // fullness of the RC fullness model
    Int m_rcOffset;                         // rcFullness offset to limit bits in rate buffer at the end of a slice
    Int m_rcOffsetInit;                     // rcFullness offset to account for bits in rate buffer from previous slice
    Int m_initTxDelay;                      // initial transmission starts delay (in blocks time)
    Int m_targetRateScale;                  // scale factor for computing targetRate
    Int m_targetRateThreshold;              // threshold (in block times) for updating targetRateScale
    Int m_bufferFracBitsAccum;              // fractional bits accumulator while calculating m_bufferOffset
    Int m_qpUpdateMode;                     // mapping to a set of QP update rules
    Int m_qp;                               // QP
    
    //! flatness params !//
    Int m_flatnessQp;                       // QP associated when flatnessType0 || flatnessType3 is true
    Int m_QpIncrementTable[5][6];           // Qp increment table
    Int m_QpDecrementTable[5][5];           // Qp decrement table

    //! chunks for slice multiplexing !//
    Int m_maxChunkSize;                     // Max allowed bytes in current chunk
    Int m_curChunkSize;                     // number of bits in the current chunk
    Int m_numChunkBlks;                     // number of blks in the current chunk
    Int m_chunkCounts;                      // total number of chunks in slice
    Int m_chunkAdjustmentBits;              // adjustment bits to meet m_maxChunkSize = m_curChunkSize

    //! ssm object info used to initialize some RC params (not used per block-time) !//
    SubStreamMux* m_ssm;                    // ssm object
    Int m_numExtraMuxBits;                  // bits remain at the end of slice due to SSM

public:
    //! constructor !//
    RateControl (ChromaFormat chroma, SubStreamMux* ssm, Pps* pps);

    //! destructor !//
    ~RateControl ();

    //! methods !//
    Void InitSlice (Int startX, Int startY);
    Void UpdateQp (Int prevBlockBits, Int targetBits, Bool isFls, FlatnessType flatnessType);
    
    Void CalcTargetRate (Int &targetRate, Bool &lastBlock, Bool isFls);
    Int ModeBufferCheck (Int modeBits);
    Void UpdateBufferFullness (Int bits);
    Void UpdateRcFullness ();
    Void UpdateRcOffsets ();
    Int CalcDeltaQp (Int diffBits);
    Void UpdateBufferLastChunksInSlice ();
    Bool CheckUnderflowPrevention ();

    //! setters !//
    Void SetIsFirstSliceInFrame (Bool is) { m_isFirstSliceInFrame = is; }
    Void SetIsFls (Bool x) { m_isFls = x; }
    Void SetIsFirstBlockInSlice (Bool is) { m_isFirstBlockInSlice = is; }
    Void SetQpUpdateMode (Int mode) {m_qpUpdateMode = mode;}

    //! getters !//
    Int GetBufferFullness () {return m_bufferFullness;}
    Int GetRcFullness () {return m_rcFullness;}
    Double GetSlicePixelsRemaining () { return (Double)m_slicePixelsRemaining; }
    UInt64 GetSliceBitsRemaining () { return (m_sliceBitsTotal - m_sliceBitsCurrent - m_numExtraMuxBits); }
    UInt64 GetSliceBitsRemainingLastBlock () { return (m_sliceBitsTotal - m_sliceBitsCurrent); }
	UInt64 GetSliceBitsTotal () { return (m_sliceBitsTotal - m_numExtraMuxBits); }
    Int GetRcOffset () { return m_rcOffset; }
    Int GetRcOffsetInit () {return m_rcOffsetInit;}
    Int GetRcBufferInitSize () { return m_rcBufferInitSize; }
    Int GetQp () { return m_qp; }
    Int GetAvgBlockBits () { return m_avgBlockBits; }
    Int GetBlockBits () { return m_blockBits; }
    Int GetNumExtraMuxBits () { return m_numExtraMuxBits; }
    Int GetMaxQp ();
    Int GetByteAlignBits () { return m_byteAlignBits; }
    Int GetChunkAdjBits () { return m_chunkAdjustmentBits; }
    UInt GetSliceBitsCurrent () { return m_sliceBitsCurrent; }
};

#endif // __RATE_CONTROL__