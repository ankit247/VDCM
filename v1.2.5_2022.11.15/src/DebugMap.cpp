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

#include "TypeDef.h"
#include "DebugMap.h"
#include "Config.h"
#include "Misc.h"

#include <fstream>
#include <assert.h>

/**
    DebugMap constructor
    
    @param width picture width
    @param height picture height
    @param blkWidth block width
    @param blkHeight block height
    @param bpc picture bit depth
    @param filename filename
    */
DebugMap::DebugMap (Int width, Int height, Int blkWidth, Int blkHeight, Int bpc, std::string const& filename)
    : m_srcWidth (width)
    , m_srcHeight (height)
    , m_stride (width)
    , m_blkWidth (blkWidth)
    , m_blkHeight (blkHeight)
    , m_bitDepth (bpc)
    , m_fileName (filename) {
    for (Int k = 0; k < g_numComps; k++) {
		UInt64 bufferLength = (UInt64)m_srcWidth * (UInt64)m_srcHeight;
		m_data[k] = new Int[bufferLength];
		::memset (m_data[k], 0, bufferLength * sizeof (Int));
    }
}

/**
    Destructor
    */
DebugMap::~DebugMap () {
    for (Int k = 0; k < g_numComps; k++) {
        delete[] m_data[k];
    }
}

/**
    Render mode information to DebugMap

    @param startY Y position of slice within frame
    @param startX X position of slice within frame
    @param mode selected mode for current block
    @param isSplit for BP mode, indicator of sub-block types
    */
Void DebugMap::DrawMode (Int startY, Int startX, ModeType mode, Bool* isSplit) {
    for (Int j = 0; j < m_blkHeight; j++) {
        for (Int i = 0; i < m_blkWidth; i++) {
            if (startY + j < m_srcHeight && startX + i < m_srcWidth) {
                Int loc = ((startY + j) * m_stride) + (startX + i);
                Bool bpRegionPartitionType = (isSplit[i >> 1] && mode == kModeTypeBP);
                Int bpSplitPartitionColor[3] = { 255, 255, 0 };
                m_data[0][loc] = bpRegionPartitionType ? bpSplitPartitionColor[0] : (Int)g_modeMapColor[mode][0];
                m_data[1][loc] = bpRegionPartitionType ? bpSplitPartitionColor[1] : (Int)g_modeMapColor[mode][1];
                m_data[2][loc] = bpRegionPartitionType ? bpSplitPartitionColor[2] : (Int)g_modeMapColor[mode][2];
            }
        }
    }
}

/**
    Render QP information to DebugMap

    @param startY Y position of slice within frame
    @param startX X position of slice within frame
    @param qp QP parameter
    */
Void DebugMap::DrawQp (Int startY, Int startX, UInt qp, Pps* pps) {
    Int minQp = GetMinQp (pps);
    Double ratio = Clip3<Double> (0.0, 1.0, (Double)(qp - minQp) / (Double)(72 - minQp));
    Int qpVal = (Int)(255.0 * ratio);
    for (Int j = 0; j < m_blkHeight; j++) {
        for (Int i = 0; i < m_blkWidth; i++) {
            if (startY + j < m_srcHeight && startX + i < m_srcWidth) {
                Int loc = ((startY + j) * m_stride) + (startX + i);
                m_data[0][loc] = qpVal;
                m_data[1][loc] = qpVal;
                m_data[2][loc] = qpVal;
            }
        }
    }
}

/**
    Render rate buffer information to DebugMap

    @param startY Y position of slice within frame
    @param startX X position of slice within frame
    @param buffer rate buffer fullness
    */
Void DebugMap::DrawBuffer (Int startY, Int startX, UInt buffer) {
    Double ratio = Clip3<Double> (0.0, 1.0, (Double)buffer / (1 << g_rc_BFRangeBits));
    Int bufferColorVal = (Int)(255.0 * ratio);
    for (Int j = 0; j < m_blkHeight; j++) {
        for (Int i = 0; i < m_blkWidth; i++) {
            if (startY + j < m_srcHeight && startX + i < m_srcWidth) {
                Int loc = ((startY + j) * m_stride) + (startX + i);
                m_data[0][loc] = bufferColorVal;
                m_data[1][loc] = bufferColorVal;
                m_data[2][loc] = bufferColorVal;
            }
        }
    }
}

/**
    Render target bitrate information to DebugMap

    @param startY Y position of slice within frame
    @param startX X position of slice within frame
    @param target target bitrate
    */
Void DebugMap::DrawTargetBitrate (Int startY, Int startX, Int target) {
    int targetRateColor = std::min (target, 255);
    for (Int j = 0; j < m_blkHeight; j++) {
        for (Int i = 0; i < m_blkWidth; i++) {
            if (startY + j < m_srcHeight && startX + i < m_srcWidth) {
                Int loc = ((startY + j) * m_stride) + (startX + i);
                m_data[0][loc] = targetRateColor;
                m_data[1][loc] = targetRateColor;
                m_data[2][loc] = targetRateColor;
            }
        }
    }
}

/**
    Render flatness information to DebugMap

    @param startY Y position of slice within frame
    @param startX X position of slice within frame
    @param flatnessType flatness type
    */
Void DebugMap::DrawFlatness (Int startY, Int startX, FlatnessType flatnessType) {
    if (flatnessType == kFlatnessTypeUndefined) {
        return;
    }

    UInt flatnessIndex = (UInt)flatnessType;
    for (Int j = 0; j < m_blkHeight; j++) {
        for (Int i = 0; i < m_blkWidth; i++) {
            if (startY + j < m_srcHeight && startX + i < m_srcWidth) {
                Int loc = ((startY + j) * m_stride) + (startX + i);
                m_data[0][loc] = (Int)g_flatMapColor[flatnessIndex][0];
                m_data[1][loc] = (Int)g_flatMapColor[flatnessIndex][1];
                m_data[2][loc] = (Int)g_flatMapColor[flatnessIndex][2];
            }
        }
    }
}

/**
    Save DebugMap as PPM
    */
Void DebugMap::SavePpm () {
    Int maxVal = 255; // Debug Maps are 8bpc
    const char* outFileName = m_fileName.c_str ();
    std::fstream outFile (outFileName, std::fstream::out | std::fstream::binary);
    outFile << "P6\n";
    outFile << m_srcWidth << " " << m_srcHeight << "\n";
    outFile << "255\n";
    for (Int i = 0; i < (m_srcHeight * m_srcWidth); i++) {
        outFile.put ((char)m_data[0][i]);
        outFile.put ((char)m_data[1][i]);
        outFile.put ((char)m_data[2][i]);
    }
    outFile.close ();
}

/**
    Convert DebugMap frame data from YCbCr to RGB
    */
Void DebugMap::Yuv2Rgb () {
    Int bits = 8;
    Int width = m_srcWidth;
    Int height = m_srcHeight;
    Int black = 16 << (bits - 8);
    Int half = 128 << (bits - 8);
    Int max = (256 << (bits - 8)) - 1;
    for (Int i = 0; i < height; i++) {
        for (Int j = 0; j < width; j++) {
            Double y = (Double)m_data[0][i * width + j];
            Double u = (Double)m_data[1][i * width + j];
            Double v = (Double)m_data[2][i * width + j];
            Double r = 1.164 * (y - black) + 1.793 * (v - half);
            Double g = 1.164 * (y - black) - 0.534 * (v - half) - 0.213 * (u - half);
            Double b = 1.164 * (y - black) + 2.115 * (u - half);
            r += 0.5;
            g += 0.5;
            b += 0.5;
            m_data[0][i * width + j] = Clip3<Int> (0, max, (Int)r);
            m_data[1][i * width + j] = Clip3<Int> (0, max, (Int)g);
            m_data[2][i * width + j] = Clip3<Int> (0, max, (Int)b);
        }
    }
}

/**
    Copy source data from video frame object to DebugMap

    @param fr video frame object
    */
Void DebugMap::CopySourceData (VideoFrame* fr) {
    for (Int k = 0; k < g_numComps; k++) {
        Int srcLoc, dstLoc;
        Int width = fr->getWidth (0);
        Int height = fr->getHeight (0);
        Int bitDepth = fr->getBitDepth (0);
        Int* src = fr->getBuf (k);
        Int* dst = m_data[k];
        Int srcLocValid = (height >> g_compScaleY[k][fr->getChromaFormat ()]) * (width >> g_compScaleX[k][fr->getChromaFormat ()]);
        for (Int i = 0; i < height; i++) {
            for (Int j = 0; j < width; j++) {
                dstLoc = (i * width + j);
                if (k > 0) {
                    switch (fr->getChromaFormat ()) {
                    case k444:
                        srcLoc = dstLoc;
                        break;
                    case k422:
                        srcLoc = (i * (width >> 1)) + (j >> 1);
                        break;
                    case k420:
                        srcLoc = ((i >> 1) * (width >> 1)) + (j >> 1);
                        break;
                    default:
                        assert (false);
                    }
                }
                else {
                    srcLoc = dstLoc;
                }

                if (srcLoc < srcLocValid) {
                    dst[dstLoc] = Clip3<Int> (0, 255, src[srcLoc] >> (bitDepth - 8));
                }
            }
        }
    }
}