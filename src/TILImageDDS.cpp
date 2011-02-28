/*
	TinyImageLoader - load images, just like that

	Copyright (C) 2010 - 2011 by Quinten Lansu
	
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	
	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include "..\SDK\headers\TILImageDDS.h"

#if (TIL_FORMAT & TIL_FORMAT_DDS)

#ifdef TIL_PRINT_DEBUG
	#define DDS_DEBUG(msg, ...)        TIL_PRINT_DEBUG("DDS: "msg, __VA_ARGS__)
#endif

#include <math.h>

namespace til
{



	#define DDSD_CAPS          0x00000001
	#define DDSD_HEIGHT        0x00000002
	#define DDSD_WIDTH         0x00000004
	#define DDSD_PITCH         0x00000008
	#define DDSD_PIXELFORMAT   0x00001000
	#define DDSD_MIPMAPCOUNT   0x00020000
	#define DDSD_LINEARSIZE    0x00080000
	#define DDSD_DEPTH         0x00800000

	#define DDSCAPS_COMPLEX    0x00000008
	#define DDSCAPS_TEXTURE    0x00001000
	#define DDSCAPS_MIPMAP     0x00400000
	#define DDSCAPS2_CUBEMAP   0x00000200
	#define DDSCAPS2_VOLUME    0x00200000

	struct DDPixelFormat
	{
		int size;
		int flags;
		int fourCC;
		int bpp;
		int redMask;
		int greenMask;
		int blueMask;
		int alphaMask;
	};

	struct DDSCaps
	{
		int caps;
		int caps2;
		int caps3;
		int caps4;
	};

	struct DDColorKey
	{
		int lowVal;
		int highVal;
	};

	struct DDSurfaceDesc
	{
		int size;
		int flags;
		int height;
		int width;
		int pitch;
		int depth;
		int mipMapLevels;
		int alphaBitDepth;
		int reserved;
		int surface;

		DDColorKey ckDestOverlay;
		DDColorKey ckDestBlt;
		DDColorKey ckSrcOverlay;
		DDColorKey ckSrcBlt;

		DDPixelFormat format;
		DDSCaps caps;

		int textureStage;
	};

	struct DataDXT1
	{
		byte c0_lo, c0_hi, c1_lo, c1_hi;
		byte bits_0, bits_1, bits_2, bits_3;
	};

	struct DataDXT5
	{
		byte alpha0, alpha1;
		byte a_bits_0, a_bits_1, a_bits_2, a_bits_3, a_bits_4, a_bits_5;

		byte c0_lo, c0_hi, c1_lo, c1_hi;
		byte bits_0, bits_1, bits_2, bits_3;
	};

#define GETCODE(x, y, data) (data & ((2 * (4 * y + x)) + 1))

	typedef void (*ColorFuncDDS_DXT1)(byte* a_DstColors, color_16b& a_Color0, color_16b& a_Color1);

	void ColorFuncDDS_DXT1_A8B8G8R8(byte* a_DstColors, color_16b& a_Color0, color_16b& a_Color1)
	{
		color_32b* colors = (color_32b*)a_DstColors;

		colors[0] = Convert_From_16b_B5G6R5_To_32b_A8B8G8R8(a_Color0);
		colors[1] = Convert_From_16b_B5G6R5_To_32b_A8B8G8R8(a_Color1);
		colors[2] = Convert_From_16b_B5G6R5_To_32b_A8B8G8R8(Blend_16b_B5G6R5(a_Color0, a_Color1, 0xAA));
		colors[3] = Convert_From_16b_B5G6R5_To_32b_A8B8G8R8(Blend_16b_B5G6R5(a_Color0, a_Color1, 0x55));

		colors[4] = colors[0];
		colors[5] = colors[1];
		colors[6] = Convert_From_16b_B5G6R5_To_32b_A8B8G8R8(Blend_16b_B5G6R5(a_Color0, a_Color1, 0x80));
		colors[7] = 0;
	}

	void ColorFuncDDS_DXT5_A8B8G8R8(byte* a_Dst, byte* a_Src, byte& a_Alpha)
	{
		color_32b* colors = (color_32b*)a_Dst;
		color_32b* src = (color_32b*)a_Src;

		*colors = Construct_32b_A8B8G8R8(*(color_32b*)a_Src, a_Alpha);
		//*colors = Construct_32b_A8R8G8B8(*(color_32b*)a_Src, 0);
		//*colors = (*src & 0xFFFFFF);

		int i = 0;
	}

	ImageDDS::ImageDDS() : Image()
	{
		m_Data = NULL;
		m_Pixels = NULL;
		m_Colors = NULL;
		m_Alpha = NULL;
	}

	ImageDDS::~ImageDDS()
	{
		if (m_Data) { delete m_Data; }
		if (m_Pixels) { delete m_Pixels; }
		if (m_Colors) { delete m_Colors; }
		if (m_Alpha) { delete m_Alpha; }
	}

	bool ImageDDS::Parse(uint32 a_ColorDepth)
	{
		dword header;
		//fread(&header, sizeof(dword), 1, m_Handle);
		m_Stream->ReadDWord(&header);

		if (header != DDS_FOURCC_DDS)
		{
			TIL_ERROR_EXPLAIN("%s is not a DDS file or header is invalid.", m_FileName);
			return false;
		}

		DDSurfaceDesc ddsd;
		//in.read(reinterpret_cast<char*>(&ddsd), sizeof ddsd);
		//fread(&ddsd, 1, sizeof(DDSurfaceDesc), m_Handle);
		m_Stream->Read(&ddsd, sizeof(DDSurfaceDesc));

		m_Format = 0;
		m_InternalBPP = 0;
		m_InternalDepth = 0;

		if (ddsd.format.fourCC != 0)
		{
			m_Format = ddsd.format.fourCC;

			m_BlockSize = 0;
			switch (m_Format)
			{

			case DDS_FOURCC_DXT1:
				{
					DDS_DEBUG("Format: DXT1");
					m_BlockSize = 8;
					break;
				}

			case DDS_FOURCC_ATI1:
				{
					DDS_DEBUG("Format: ATI1");
					m_BlockSize = 8;
					break;
				}
				
			case DDS_FOURCC_DXT2:
				{
					DDS_DEBUG("Format: DXT2");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_DXT3:
				{
					DDS_DEBUG("Format: DXT3");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_DXT4:
				{
					DDS_DEBUG("Format: DXT4");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_DXT5:
				{
					DDS_DEBUG("Format: DXT5");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_RXGB:
				{
					DDS_DEBUG("Format: RXGB");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_ATI2:
				{
					DDS_DEBUG("Format: ATI2");
					m_BlockSize = 16;
					break;
				}
			default:
				{
					TIL_ERROR_EXPLAIN("Unknown FourCC in DDS file: %d", ddsd.format.fourCC);
					return false;
				}
				
			}
		}
		else
		{
			if (ddsd.format.bpp == 32)
			{
				if (ddsd.format.redMask == 0x00ff0000)
				{
					m_InternalDepth = TIL_DEPTH_A8R8G8B8;
				}
				else
				{
					m_InternalDepth = TIL_DEPTH_A8B8G8R8;
				}
			}
			else if (ddsd.format.bpp == 24)
			{
				if (ddsd.format.redMask == 0x00ff0000)
				{
					m_InternalDepth = TIL_DEPTH_R8G8B8;
				}
				else
				{
					m_InternalDepth = TIL_DEPTH_B8G8R8;
				}
			}
			else
			{
				TIL_ERROR_EXPLAIN("Unknown bit-depth: %d", ddsd.format.bpp);
				return false;
			}
		}

		m_InternalBPP = ddsd.format.bpp;

		m_Width = ddsd.width;
		m_Height = ddsd.height;
		m_Depth = ddsd.depth;
		m_MipMaps = (ddsd.mipMapLevels >= 1 ? ddsd.mipMapLevels : 1) - 1;

		DDS_DEBUG("Dimensions: (%d, %d)", m_Width, m_Height);
		DDS_DEBUG("Depth: %d", m_Depth);
		DDS_DEBUG("Mipmaps: %d", m_MipMaps);

		m_CubeMap = false;
		if ((ddsd.caps.caps2 & DDSCAPS2_CUBEMAP) == DDSCAPS2_CUBEMAP)
		{
			DDS_DEBUG("Cubemap");
			m_CubeMap = true;
		}

		ReadData();
		GetOffsets();

		DDS_DEBUG("Blocks: %d", m_Blocks);

		m_Pixels = new byte[m_Width * m_Height * m_BPP];

		m_Colors = new byte[m_BPP * 8];

		if (m_Format == DDS_FOURCC_DXT1)
		{
			DecompressDXT1();
		}
		else if (m_Format == DDS_FOURCC_DXT5)
		{
			m_Alpha = new byte[16 * sizeof(dword)];
			DecompressDXT5();
		}
		else
		{
			TIL_ERROR_EXPLAIN("Unknown compression algorithm: %d", m_Format);
		}

		return true;
	}

	uint32 ImageDDS::GetFrameCount()
	{
		return 1;
	}

	byte* ImageDDS::GetPixels(uint32 a_Frame /*= 0*/)
	{
		return m_Pixels;
	}

	uint32 ImageDDS::GetWidth(uint32 a_Frame /*= 0*/)
	{
		return m_Width;
	}

	uint32 ImageDDS::GetHeight(uint32 a_Frame /*= 0*/)
	{
		return m_Height;
	}

	void ImageDDS::GetOffsets()
	{
		uint32 size = 128; // sizeof(DDSHeader);
		uint32 faces = 1;

		if (m_CubeMap)
		{
			faces = 6;
		}

		/*if (header.hasDX10Header())
		{
			size += 20; // sizeof(DDSHeader10);
		}*/

		for (int i = 0; i < faces; i++)
		{
			uint32 count = m_MipMaps;
			uint32 size = 0;

			for (uint32 m = 0; m < count; m++)
			{
				uint32 w = m_Width;
				uint32 h = m_Height;
				uint32 d = m_Depth;

				for (uint32 m = 0; m < m_MipMaps; m++)
				{
					w = (w / 2);
					h = (h / 2);
					d = (d / 2);
				}

				if (m_Format)
				{
					// @@ How are 3D textures aligned?
					w = (w + 3) / 4;
					h = (h + 3) / 4;
					size += m_BlockSize * w * h;
				}
				else
				{
					// Align pixels to bytes.
					uint32 byteCount = (m_InternalBPP + 7) / 8;

					// Align pitch to 4 bytes.
					uint32 pitch = 4 * ((w * byteCount + 3) / 4);

					size += pitch * h * d;
				}
			}

			int i = 0;
		}

		/*for (uint m = 0; m < mipmap; m++)
		{
			size += mipmapSize(m);
		}*/

		//return size;
	}

	void ImageDDS::ReadData()
	{
		int powres = 1 << m_MipMaps;
		int nW = m_Width / powres;
		int nH = m_Height / powres;
		int nD = m_Depth / powres;

		m_Blocks = 0;

		// 4x4 blocks, 8 bytes per block
		if (m_Format == DDS_FOURCC_DXT1)
		{
			//m_Size = (((nW + 3) / 4) * ((nH + 3) / 4) * 8);
			m_Blocks = (m_Width / 4) * (m_Height / 4);
		}
		// 4x4 blocks, 16 bytes per block
		else if (m_Format == DDS_FOURCC_DXT5 || m_Format == DDS_FOURCC_DXT3)
		{
			//m_Size = (((nW + 3) / 4) * ((nH + 3) / 4) * 16);
			m_Blocks = (m_Width / 4) * (m_Height / 4);
		}
		else if (m_Depth > 0)
		{
			// Width   Height   Depth   Width*Height*Depth     Width*Height*Depth
			// ----- * ------ * ----- = ------------         = ------------
			// 2^i     2^i		2^i     2^i*2^i*2^i		       2^(i+i+i)

			m_Size = (((m_Width * m_Height * m_Depth) / (powres * powres * powres)) * m_InternalBPP);
		}
		// No 3d image loaded, so mipmapping is different.
		else 
		{
			m_Size = (((m_Width * m_Height) / (powres * powres)) * m_InternalBPP);
		}
		m_Size = m_Blocks * m_BlockSize;

		if (m_CubeMap)
		{
			m_Size *= 6;
		}

		m_Data = new byte[m_Size];
		//fread(m_Data, m_Size, 1, m_Handle);
		m_Stream->ReadByte(m_Data, m_Size);
	}

	void ImageDDS::DecompressDXT1()
	{
		color_32b* write = (color_32b*)m_Pixels;
		color_32b* dst_x = write;

		uint32 pitch = m_Width * 4;
		uint32 pitch_blocks = m_Width / 4;
		uint32 curr = 0;

		DataDXT1* src = (DataDXT1*)m_Data;

		int glob_x = 0;
		int glob_y = 0;
		int pos_x = 0;
		int pos_y = 0;

		for (int i = 0; i < m_Blocks; i++)
		{
			color_16b color0 = (src->c0_lo + (src->c0_hi * 256));
			color_16b color1 = (src->c1_lo + (src->c1_hi * 256));
			dword bits = src->bits_0 + (256 * (src->bits_1 + 256 * (src->bits_2 + 256 * src->bits_3)));

			ColorFuncDDS_DXT1_A8B8G8R8(m_Colors, color0, color1);
			color_32b* colors = (color_32b*)m_Colors;

			int offset = (color0 > color1) ? 0 : 4;
		
			pos_y = glob_y;

			for (int y = 0; y < 4; y++)
			{
				pos_x = glob_x;

				for (int x = 0; x < 4; x++)
				{
					int curr = (2 * (4 * y + x));
					int enabled = ((bits & (0x3 << curr)) >> curr) + offset;

					color_32b* dst = &write[pos_x + (pos_y * m_Width)];

					*dst = colors[enabled];

					pos_x++;
				}

				pos_y++;
			}

			glob_x += 4;
			if (++curr == pitch_blocks)
			{
				curr = 0;

				glob_y += 4;
				glob_x = 0;
			}

			src++;
		}
	}

	void ImageDDS::DecompressDXT5()
	{
		color_32b* write = (color_32b*)m_Pixels;
		color_32b* dst_x = write;

		uint32 pitch = m_Width * 4;
		uint32 pitch_blocks = m_Width / 4;
		uint32 curr = 0;

		DataDXT5* src = (DataDXT5*)m_Data;

		int glob_x = 0;
		int glob_y = 0;
		int pos_x = 0;
		int pos_y = 0;

		for (int i = 0; i < m_Blocks; i++)
		{
			uint64 a_bits_total = 
				((uint64)src->a_bits_0      ) | ((uint64)src->a_bits_1 << 8 ) | 
				((uint64)src->a_bits_2 << 16) | ((uint64)src->a_bits_3 << 24) |
				((uint64)src->a_bits_4 << 32) | ((uint64)src->a_bits_5 << 40);

			color_16b color0 = (src->c0_lo + (src->c0_hi << 8));
			color_16b color1 = (src->c1_lo + (src->c1_hi << 8));
			dword bits = (src->bits_0) | (src->bits_1 << 8) | (src->bits_2 << 16) | (src->bits_3 << 24);

			byte* alpha = (byte*)m_Alpha;

			alpha[0] = src->alpha0;
			alpha[1] = src->alpha1;
			alpha[2] = (6 * src->alpha0 + 1 * src->alpha1) / 7;
			alpha[3] = (5 * src->alpha0 + 2 * src->alpha1) / 7;
			alpha[4] = (4 * src->alpha0 + 3 * src->alpha1) / 7;
			alpha[5] = (3 * src->alpha0 + 4 * src->alpha1) / 7;
			alpha[6] = (2 * src->alpha0 + 5 * src->alpha1) / 7;
			alpha[7] = (1 * src->alpha0 + 6 * src->alpha1) / 7;

			alpha[8] = src->alpha0;
			alpha[9] = src->alpha1;
			alpha[10] = (4 * src->alpha0 + 1 * src->alpha1) / 5;
			alpha[11] = (3 * src->alpha0 + 2 * src->alpha1) / 5;
			alpha[12] = (2 * src->alpha0 + 3 * src->alpha1) / 5;
			alpha[13] = (1 * src->alpha0 + 4 * src->alpha1) / 5;
			alpha[14] = 0;
			alpha[15] = 255;

			ColorFuncDDS_DXT1_A8B8G8R8(m_Colors, color0, color1);
			color_32b* colors = (color_32b*)m_Colors;

			pos_y = glob_y;

			int offset = 0;
			if (src->alpha0 <= src->alpha1)
			{
				offset = 8;
			}

			for (int y = 0; y < 4; y++)
			{
				pos_x = glob_x;

				for (int x = 0; x < 4; x++)
				{
					uint64 bits_alpha = 0;

					int curr_a = (3 * (4 * y + x));
					bits_alpha = (a_bits_total & ((uint64)0x7 << curr_a)) >> curr_a;

					int curr = (2 * (4 * y + x));
					int enabled = ((bits & (0x3 << curr)) >> curr);

					color_32b* dst = &write[pos_x + (pos_y * m_Width)];

					ColorFuncDDS_DXT5_A8B8G8R8((byte*)dst, (byte*)&colors[enabled], alpha[bits_alpha + offset]);

					pos_x++;
				}

				pos_y++;
			}

			glob_x += 4;
			if (++curr == pitch_blocks)
			{
				curr = 0;

				glob_y += 4;
				glob_x = 0;
			}

			src++;
		}
	}

}; // namespace til

#endif