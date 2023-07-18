Copyright (c) 2015-2018 Qualcomm Technologies, Inc. All Rights Reserved

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The above license is used as a license under copyright only. Please reference VESA Policy #200D for patent licensing terms.

# ------------------------------------------------------------------------------------------------------------------------------------------

1) Description

This package includes a test model for the VDC-M codec.  This codec can be used to compress and decompress source images
at a fixed compression-rate per image, as is required for compression of display streams.  The test model supports a number of
input formats (RAW, YUV, PPM, DPX), chroma-sampling formats (4:4:4, 4:2:2, 4:2:0) and color spaces (RGB, YCbCr).  The compression rate
can be specified with 1/16 bit-per-pixel (BPP) resolution with a minimum supported rate of 4.75 BPP.  The default rate is 6BPP.

# ------------------------------------------------------------------------------------------------------------------------------------------

2) Usage

Please follow the examples below for usage of the VDC-M test model.

# ------------------------------------------------------------------------------------------------------------------------------------------

2.1) IO

The following file formats are supported by the test model.  Different file formats may be used for input/output images, with
a few restrictions.  For example, If the input is DPX (YUV4:2:2) or YUV, then PPM/RAW may not be used for the output image.

* PPM (RGB 4:4:4)

* DPX (RGB 4:4:4, YUV 4:4:4/4:2:2/4:2:0)
  * supporting 8, 10, 12 bits/component
  * must use packing 1 if bitDepth is 10 or 12 (integer number of samples in each dword)
  * big endian order is assumed for data (pass flag -dpxForce
  * data order for RGB is [R, G, B, R, G, B, ...] counter to the spec but assumed by XnView/Vooya/etc.
  * tested with:
    * XnView 2.40
    * Vooya 1.8.2
    * GraphicsMagick 1.3.26 Q16
    * ImageMagick 7.0.7-9 Q16

* RAW (RGB 4:4:4)
  * must specify: width, height, bitDepth
  * data order is [R, G, B, R, G, B, ...] (interleaved)
  * if bitDepth > 8, each sample is padded to a 16-bit word (pad MSBs with zeros)

* YUV (YUV 4:4:4/4:2:2/4:2:0) - planar
  * must specify: width, height, bitDepth, chromaFormat
    * chromaFormat == 0 --> 4:4:4
    * chromaFormat == 1 --> 4:2:2
    * chromaFormat == 2 --> 4:2:0
  * data order is [Y, ..., Y, U, ..., U, V, ..., V] (planar)
  * if bitDepth > 8, each sample is padded to a 16-bit word (pad MSBs with zeros)

# ------------------------------------------------------------------------------------------------------------------------------------------

2.2) Running the test model (encoder)

VDCM_Encoder.exe ...

Required parameters:

                  -inFile | source image (raw / yuv / ppm / dpx)
                 -recFile | reconstructed image (raw / yuv / ppm / dpx)
                     -bpp | bits per pixel of compressed bitstream (see section 3)
               -bitstream | compressed bitstream
              -configFile | VDC-M config file
                   -width | source width (must be specified for RAW/YUV, otherwise inferred)
                  -height | source height (must be specified for RAW/YUV, otherwise inferred)
                -bitDepth | source bit depth (must be specified for RAW/YUV, otherwise inferred)
            -chromaFormat | chroma sampling format (must be specified for YUV, otherwise inferred)

Optional parameters (override values in config file):

             -sliceHeight | slice height in pixels (e.g. 32, 64, 108, etc.)
           -slicesPerLine | # of slices per line.  Source width should be evenly divisible
            -ssmMaxSeSize | SSM - max syntax element size (bits)

Additional flags:

               -debugMaps | generate debug maps
             -debugTracer | generate encoder debugTrace (warning: will slow runtime considerably)
            -dumpPpsBytes | dump PPS information (by byte)
           -dumpPpsFields | dump PPS information (by field)
               -dumpModes | dump mode distribution
                 -minilog | generate minilog for small debugging info
                -calcPsnr | calculate PSNR between source and reconstructed images

DPX flags:

         -forceDpxReadBGR | force BGR order for DPX data read (descriptor 50 only)
          -forceDpxReadLE | force little endian for DPX data read
        -forceDpxWriteBGR | force BGR order for DPX data write (descriptor 50 only)
         -forceDpxWriteLE | force little endian for DPX data write
 -forceDpxWriteDataOffset | force writing data offset to header for RGB DPX files
   -forceDpxWritePadLines | force padding of each line to the nearest doubleword
    -forceDpxReadPadLines | read and ignore padding of each line to nearest doubleword

# ------------------------------------------------------------------------------------------------------------------------------------------

2.3) Running the test model (decoder)

VDCM_Decoder.exe ...

Required parameters:

               -bitstream | compressed bitstream
                 -recFile | reconstructed image (raw/yuv/ppm/dpx)

Additional flags:

             -debugTracer | generate encoder debugTrace (warning: will slow runtime considerably)
            -dumpPpsBytes | dump PPS information (by byte)
           -dumpPpsFields | dump PPS information (by field)
               -dumpModes | dump mode distribution

DPX flags:

        -forceDpxWriteBGR | force BGR order for DPX data write (descriptor 50 only)
         -forceDpxWriteLE | force little endian for DPX data write
 -forceDpxWriteDataOffset | force writing data offset to header for RGB DPX files
   -forceDpxWritePadLines | force padding of each line to the nearest doubleword

# ------------------------------------------------------------------------------------------------------------------------------------------

2.4) Example usage

* Example 1: PPM, RGB 4:4:4 8bpc, 6bpp, v1.1

VDCM_Encoder.exe    -inFile source.ppm
                    -bitstream vdcm.bits
                    -recFile source_rec.ppm
                    -bpp 6.0
                    -configFile configFiles_v1.1/config_444_8bpc_6bpp.txt

VDCM_Decoder.exe    -bitstream vdcm.bits
                    -recFile source_dec.ppm

* Example 2: PPM, RGB 4:4:4 10bpc, 6bpp, override: 2 slices/line, v1.2

VDCM_Encoder.exe    -inFile source.ppm
                    -bitstream vdcm.bits
                    -recFile source_rec.ppm
                    -bpp 6.0
                    -configFile configFiles_v1.2/config_444_10bpc_6bpp.txt
                    -slicesPerLine 2

VDCM_Decoder.exe    -bitstream vdcm.bits
                    -recFile source_dec.ppm

* Example 3: DPX, YUV 4:2:2 8bpc, 5bpp, override: slice height 96, v1.1

VDCM_Encoder.exe    -inFile source.dpx
                    -bitstream vdcm.bits
                    -recFile source_rec.dpx
                    -bpp 5.0
                    -configFile configFiles_v1.1/config_422_8bpc_5bpp.txt
                    -sliceHeight 96

VDCM_Decoder.exe    -bitstream vdcm.bits
                    -recFile source_dec.dpx

* Example 4: YUV, YUV 4:2:2 10bpc, 6bpp, 1920x1080, v1.2

VDCM_Encoder.exe    -inFile source.yuv
                    -bitstream vdcm.bits
                    -recFile source_rec.yuv
                    -bpp 6.0
                    -configFile configFiles_v1.2/config_422_10bpc_6bpp.txt
                    -width 1920
                    -height 1080
                    -bitDepth 10
                    -chromaFormat 1
                
VDCM_Decoder.exe    -bitstream vdcm.bits
                    -recFile source_dec.yuv

# ------------------------------------------------------------------------------------------------------------------------------------------

3) Compressed bitstream

  * minimum supported: 4bpp
  * default: 6bpp
  * resolution: (1/16) bpp

Ex supported bpp: 4.0, 4.0625, 4.125, ..., 6.0, 6.0625, ..., 12.0

The compressed bitstream is coded as a binary file.  The picture parameter set (PPS) will be transmitted
as the first 128 bytes of the compressed bitstream.  The decoder will use the PPS to configure itself.

# ------------------------------------------------------------------------------------------------------------------------------------------

4) Config file

A config file is required by the encoder.  The config file is specified via command
line using the flag '-configFile []'.  The config file used should match the parameters of the source
picture.  For example, compression of a 4:4:4, 8bpc RGB source image with a compressed bitrate of 6bpp
is specified as: '-configFile config_444_8bpc_6bpp.txt'.

All command line parameters take precedence over the config file.  For example, if sliceHeight is
specified as 108 in the config file, but the parameter '-sliceHeight 96' is passed to the encoder,
then a slice height of 96 is used.