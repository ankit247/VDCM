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

/* Copyright (c) 2015-2017 MediaTek Inc., BP-Skip mode is modified and/or implemented by MediaTek Inc. based on BpMode.h
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
\file       BpMode.h
\brief      Block Prediction Mode (header)
*/

#ifndef __BP_MODE__
#define __BP_MODE__

#include "TypeDef.h"
#include "Mode.h"
#include "EntropyCoder.h"
#include "BpSearchRange.h"

class BpMode : public Mode
{
protected:
    Int m_partitionSize;
    Int m_srSize;
    Int m_srBufferLength;
    Int m_bpvNumBits;
    Int* m_resBlkBuf2x1[g_numComps];
    Int* m_recBlkBuf2x1[g_numComps];
    Int* m_resBlkBuf2x2[g_numComps];
    Int* m_recBlkBuf2x2[g_numComps];
    Int* m_qResBlkBuf2x1[g_numComps];
    Int* m_qResBlkBuf2x2[g_numComps];
    Int* m_iqResBlkBuf2x1[g_numComps];
    Int* m_iqResBlkBuf2x2[g_numComps];
    Int* m_bpv2x1;
    Int* m_bpv2x2;
    Int* m_distortion2x1;
    Int* m_distortion2x2;
    Bool* m_use2x2;
    Int* m_temp_bpv2x2;
    Int* m_temp_bpv2x1;
    Bool* m_temp_use2x2;
    Bool m_bpskip;
    Bool m_bpskip_flag; // In BP mode, if m_bpskip_flag == 1, then it means best mode can be changed to BP skip mode.
    BpMode* m_bpModeRef;
    std::vector<BpSearchRange*> m_bpSrStorage;
    Int* m_mapSrId;
    Int* m_mapSrPos;
    Int m_partBitsPerLine;  // used to map each partition to its position within a block
    UInt m_numSubBlocks;

public:
    BpMode (Int bpp, Int bitDepth, Bool isEncoder, ChromaFormat chromaFormat, ColorSpace csc, ColorSpace targetCsc, Bool isBpSkip);
    ~BpMode ();

    // virtual overloads
    Void InitModeSpecific ();
    Void Test ();
    Void Encode ();
    Void Decode ();
    Void FetchDataFromReference ();

    // methods
    Void FetchBpvs (BpMode* bpModeRef);
    Int EncodeBpv (Bool isFinal);
    Void QuantizeResidual ();
    Void EncoderReconstruct ();
	Void PartitionDecisionParallel (UInt headerBits);
	Void CopyPartitionGridData (Bool* select2x2, Int** targetArray, Int** srcArray0, Int** srcArray1);
	Void CopyBufferToBuffer (Int** src, Int** dst);
	Int DequantSample (Int quant, Int scale, Int offset, Int shift);
    Void InitSearchRange ();
    Void InitSearchRangeA ();
    Void InitSearchRangeB ();
    Void InitSearchRangeC ();
	Void InitSearchRangeC_YCoCg ();
    Void BpvSearch ();
    Void ComputeResidual ();
    Void DecoderReconstruct ();
    Void UpdateQp ();
    Void DeleteBufferMemory ();
    Int ReconstructSample (Int predictor, Int dequant, Bool isSkip, Int min, Int max);
    Void GenerateBpSrMappings (Int posA, Int posB, Int posC);
    Void SetSrPositions ();
    Int GetPartitionStartOffset (Int partIdx, Int comp, BpPartitionType type);
    Int GetPartitionSearchRange (Int partIdx, Int comp, BpPartitionType type, BpPartitionHalf half);
    Void DecodeBpvNextBlock ();
    Void DecodeBpvCurBlock ();
    Int GetOverlapRegionOffset (Int k);
    Void BpvMapping (Int lumaBpv, Int comp, Int &srId, Int &srPos);
    Void BpvSwap ();

    // setters
    Void SetBpv2x1 (Int idx, Int bpv) { m_bpv2x1[idx] = bpv; }
    Void SetDistortion2x1 (Int idx, Int d) { m_distortion2x1[idx] = d; }
    Void SetDistortion2x2 (Int idx, Int d) { m_distortion2x2[idx] = d; }
    Void SetBpv2x2 (Int idx, Int bpv) { m_bpv2x2[idx] = bpv; }
    Void SetUse2x2 (Int idx, Bool use) { m_use2x2[idx] = use; }
    Void SetBpModeRef (BpMode* bpModeRef) { m_bpModeRef = bpModeRef; }

    // getters
    Bool GetBPSkipFlag () { return m_bpskip_flag; }
    Int GetBpv2x1 (Int idx) { return m_bpv2x1[idx]; }
    Int GetBpv2x2 (Int idx) { return m_bpv2x2[idx]; }
    Int GetSrSize (Int comp) { return m_srSize >> (g_compScaleX[comp][m_chromaFormat]); }
    Int GetPartitionSize (Int comp) { return m_partitionSize >> g_compScaleX[comp][m_chromaFormat]; }
    Int GetDistortion2x1 (Int idx) { return m_distortion2x1[idx]; }
    Int GetDistortion2x2 (Int idx) { return m_distortion2x2[idx]; }
    Bool* GetUse2x2 () { return m_use2x2; }
    Int* GetRecBlkBuf2x1 (Int comp) { return m_recBlkBuf2x1[comp]; }
    Int* GetResBlkBuf2x1 (Int comp) { return m_resBlkBuf2x1[comp]; }
    Int* GetQResBlkBuf2x1 (Int comp) { return m_qResBlkBuf2x1[comp]; }
    Int* GetIQResBlkBuf2x1 (Int comp) { return m_iqResBlkBuf2x1[comp]; }
    Int* GetRecBlkBuf2x2 (Int comp) { return m_recBlkBuf2x2[comp]; }
    Int* GetResBlkBuf2x2 (Int comp) { return m_resBlkBuf2x2[comp]; }
    Int* GetQResBlkBuf2x2 (Int comp) { return m_qResBlkBuf2x2[comp]; }
    Int* GetIQResBlkBuf2x2 (Int comp) { return m_iqResBlkBuf2x2[comp]; }
    
    //! debugging !//
    Void EncoderDebugBpv ();
    Void DecoderDebugBpv ();
};

#endif // __BP_MODE__