# Copyright (c) 2015-2017 Qualcomm Technologies, Inc. All Rights Reserved
# 
# Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
# 
# Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# 
# Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without
# specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# The above license is used as a license under copyright only. Please reference VESA Policy #200D for patent licensing terms.
# 
# ------------------------------------------------------------------------------------------------------------------------------------------

# ------------------------------------------
# Makefile
# Version:  v0.90
# Date:     2017.11.10
# ------------------------------------------

CXX = g++
CXX_FLAGS = -g -O2 -w
ENC_TARGET = VDCM_Encoder
DEC_TARGET = VDCM_Decoder
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SRC_ENC =   Block.cpp VideoFrame.cpp RateControl.cpp FlatnessDetection.cpp EntropyCoder.cpp Bitstream.cpp \
            IntraPrediction.cpp EncodingParams.cpp Misc.cpp EncTop.cpp Mode.cpp TransMode.cpp BpMode.cpp MppMode.cpp Pps.cpp \
            MppfMode.cpp DebugMap.cpp SyntaxElement.cpp Ssm.cpp BpSearchRange.cpp ImageFileIo.cpp BitFifo.cpp VdcmEncoder.cpp

SRC_DEC =   Block.cpp VideoFrame.cpp RateControl.cpp EntropyCoder.cpp Bitstream.cpp Pps.cpp IntraPrediction.cpp \
            DecodingParams.cpp Misc.cpp DecTop.cpp Mode.cpp TransMode.cpp BpMode.cpp MppMode.cpp MppfMode.cpp \
            SyntaxElement.cpp Ssm.cpp BpSearchRange.cpp ImageFileIo.cpp BitFifo.cpp VdcmDecoder.cpp

OBJ_ENC = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC_ENC))
OBJ_DEC = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC_DEC))

all: $(BIN_DIR)/$(ENC_TARGET) $(BIN_DIR)/$(DEC_TARGET)

encoder: $(BIN_DIR)/$(ENC_TARGET)

decoder: $(BIN_DIR)/$(DEC_TARGET)

$(BIN_DIR)/$(ENC_TARGET): $(OBJ_ENC)
	@mkdir -p $(@D)
	@echo Linking encoder...
	$(CXX) $(CXX_FLAGS) -o $(BIN_DIR)/$(ENC_TARGET) $(OBJ_ENC)
	@echo Done.
	@echo

$(BIN_DIR)/$(DEC_TARGET): $(OBJ_DEC)
	@mkdir -p $(@D)
	@echo Linking decoder...
	$(CXX) $(CXX_FLAGS) -o $(BIN_DIR)/$(DEC_TARGET) $(OBJ_DEC)
	@echo Done.
	@echo

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

clean:
	@rm -rf $(OBJ_DIR)
	@rm -rf $(BIN_DIR)