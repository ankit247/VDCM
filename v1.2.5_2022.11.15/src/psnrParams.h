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

#ifndef __PSNRPARAMS__
#define __PSNRPARAMS__

#include "TypeDef.h"

class PsnrParams
{
protected:
  // file I/O
  char*     m_origFile;               // orig image file name
  char*     m_recFile;                // reconstructed image file name
  char*     m_diffFile;               // difference image file name

  // source specification
  Int       m_srcWidth;               // source width in pixel
  Int       m_srcHeight;              // source height in pixel
  Int       m_bitDepth;               // source bit-depth (assume BigEndian byte order)
  Int       m_chromaFormat;           // Chroma format 

  Bool      m_isInterleaved;          // Is source file interleaved or planar
  Int       m_scaleFactor;            // scale factor for the difference image

public:

  PsnrParams ()
      : m_bitDepth(8)
      , m_chromaFormat(2) /* Default: 2 - 4:4:4 ( 0 - 4:2:0, 1 - 422, 2 - 444 ) */
      , m_isInterleaved(true)
      , m_scaleFactor(1)
      , m_srcWidth(0)
      , m_srcHeight(0)
      , m_origFile(NULL)
      , m_recFile(NULL)
      , m_diffFile(NULL)
  {};
  ~PsnrParams() {};

  Bool parseCfg( Int argc, Char* argv[] );

  void setOrigFile(Char* origFile) { m_origFile = origFile; };
  void setRecFile(Char* recFile) { m_recFile = recFile; };
  void setDiffFile(Char* diffFile) { m_diffFile = diffFile; };
  void setWidth(Int width) { m_srcWidth = width; };
  void setHeight(Int height) { m_srcHeight = height; };
  void setBitDepth(Int bitDepth) { m_bitDepth = bitDepth; };
  void setChromaFormat(Int chromaFormat) { m_chromaFormat = chromaFormat; };
  void setIsInterleaved(Bool isInterleaved ) { m_isInterleaved = isInterleaved; };
  void setScaleFactor(Int scaleFactor) { m_scaleFactor = scaleFactor; }; 

  Char*  getOrigFile() { return m_origFile; };
  Char*  getDiffFile() { return m_diffFile; };
  Char*  getRecFile() { return m_recFile; };
  Int    getWidth() { return m_srcWidth; };
  Int    getHeight() { return m_srcHeight; };
  Int    getBitDepth() { return m_bitDepth; };
  Int    getChromaFormat() { return m_chromaFormat; };
  Bool   getIsInterleaved() {return m_isInterleaved; };
  Int    getScaleFactor() { return m_scaleFactor; };
}; // END CLASS DEFINITION PsnrParams

#endif // __PsnrPARAMS__