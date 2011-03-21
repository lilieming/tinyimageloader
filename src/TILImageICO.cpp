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

#include "TILImageICO.h"
#include "TILInternal.h"

#include <windows.h>
#include <wingdi.h>

#if (TIL_FORMAT & TIL_FORMAT_ICO)

#ifdef TIL_TARGET_DEBUG
	#define ICO_DEBUG(msg, ...)        TIL_PRINT_DEBUG("ICO: "msg, __VA_ARGS__)
#else
	#define ICO_DEBUG(msg, ...)
#endif

//#define GETBIT(data, place) ((data >> (place)) & 1)
#define GETBIT(data, place) ((data >> (8 - place)) & 1)

namespace til
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	typedef uint8* (*ColorFuncRow)(uint8*, uint8*, uint8*, BufferICO*);

	uint8* ColorFunc_4b_A8B8G8R8_Row(uint8* a_Dst, uint8* a_Src, uint8* a_AndMask, BufferICO* a_Buffer)
	{
		color_32b* dst = (color_32b*)a_Dst;

		uint8* src = a_Src;
		uint8* and = a_AndMask;

		if (a_Buffer->palette == 16)
		{
			for (uint32 i = 0; i < a_Buffer->width; i += 8)
			{
				for (int32 j = 1; j < 8; j += 2)
				{
					if (GETBIT(*and, j    ) == 0) 
					{ 
						*dst = a_Buffer->colors[(*src & 0xF0) >> 4];
					}
					else
					{
						*dst = 0xFFFF00FF;
					}
					dst++;

					if (GETBIT(*and, j - 1) == 0) 
					{ 
						*dst = a_Buffer->colors[(*src & 0x0F)     ]; 
					}
					else
					{
						*dst = 0xFFFF00FF;
					}
					dst++;

					/*
					*dst++ = a_Buffer->colors[(*src & 0xF0) >> 4];
					*dst++ = a_Buffer->colors[(*src & 0x0F)     ];
					*/

					src++;
				}
				and++;
			}

			return (uint8*)dst;
		}
		else if (a_Buffer->palette == 256)
		{
			for (uint32 i = 0; i < a_Buffer->width; i++)
			{
				*dst++ = a_Buffer->colors[*src];
				src++;
			}

			return (uint8*)dst;
		}
		else
		{
			for (uint32 j = 0; j < a_Buffer->width; j++)
			{
				*dst++ = AlphaBlend_32b_A8B8G8R8(src[2], src[1], src[0], src[3]);
				src += 4;
			}

			return (uint8*)dst;
		}
	}

	uint8* ColorFunc_1b_A8B8G8R8_Row(uint8* a_Dst, uint8* a_Src, uint8* a_AndMask, BufferICO* a_Buffer)
	{
		color_32b* dst = (color_32b*)a_Dst;

		uint8* src = a_Src;

		for (uint32 j = 0; j < a_Buffer->width; j++)
		{
			//*dst++ = AlphaBlend_32b_A8B8G8R8(src[2], src[1], src[0], src[3]);
			src++;
		}

		return (uint8*)dst;
	}

	ColorFuncRow g_ColorFuncRow = NULL;

#endif

	ImageICO::ImageICO()
	{
		m_First = m_Current = NULL;
	}

	ImageICO::~ImageICO()
	{
		ReleaseMemory(m_First);
		delete m_First;
	}

	void ImageICO::AddBuffer(uint32 a_Width, uint32 a_Height)
	{
		if (m_First)
		{	
			m_Current->next = new BufferICO;
			m_Current->next->next    = NULL;
			m_Current->next->width   = a_Width;
			m_Current->next->height  = a_Height;
			m_Current->next->pitch   = a_Width;
			m_Current->next->buffer  = new byte[a_Width * a_Height * m_BPP];
			m_Current->next->colors  = NULL;
			m_Current = m_Current->next;
		}
		else
		{ 
			m_First = new BufferICO;
			m_Current = m_First;
			m_Current->next    = NULL;
			m_Current->width   = a_Width;
			m_Current->height  = a_Height;
			m_Current->pitch   = a_Width;
			m_Current->buffer  = new byte[a_Width * a_Height * m_BPP];
			m_Current->colors  = NULL;
		}
	}

	void ImageICO::ExpandPalette(BufferICO* a_Buffer)
	{
		a_Buffer->colors = new color_32b[a_Buffer->palette];

		color_32b* dst = a_Buffer->colors;

		for (int i = 0; i < a_Buffer->palette; i++)
		{
			byte read[4];
#ifdef OLDMETHOD
			fread(read, 1, 4, m_Handle);
#else
			m_Stream->ReadByte(read, 4);
#endif
			*dst++ = Construct_32b_A8R8G8B8(read[0], read[1], read[2], (a_Buffer->palette == 16) ? 255 : read[3]);
		}
	}

	bool ImageICO::Parse(uint32 a_ColorDepth)
	{

#ifdef OLDMETHOD

		m_Handle = fopen(m_Stream->GetFilePath(), "rb");

#endif
		// empty anyway
#ifdef OLDMETHOD
		fseek(m_Handle, 2, SEEK_CUR);
#else
		m_Stream->Seek(2, TIL_FILE_SEEK_CURR);
#endif

#ifdef OLDMETHOD
		word header;          fread(&header, 2, 1, m_Handle);
#else
		word header;          m_Stream->ReadWord(&header);
#endif
		if (header != 1 && header != 2)
		{
			TIL_ERROR_EXPLAIN("Not a valid ICO file!");
			return false;
		}

#ifdef OLDMETHOD
		word imagecount;      fread(&imagecount, 2, 1, m_Handle);
#else
		word imagecount;      m_Stream->ReadWord(&imagecount);
#endif
		m_Images = imagecount;

		byte width, height, palette, reserved;
		word planes, bpp;
		dword datasize, offset;

		for (uint16 i = 0; i < imagecount; i++)
		{

#ifdef OLDMETHOD
			fread(&width, 1, 1, m_Handle);
			fread(&height, 1, 1, m_Handle);

			fread(&palette, 1, 1, m_Handle);
			fread(&reserved, 1, 1, m_Handle);

			fread(&planes, 2, 1, m_Handle);
			fread(&bpp, 2, 1, m_Handle);

			fread(&datasize, 4, 1, m_Handle);
			fread(&offset, 4, 1, m_Handle);
#else
			m_Stream->ReadByte(&width);
			m_Stream->ReadByte(&height);

			m_Stream->ReadByte(&palette);
			m_Stream->ReadByte(&reserved);

			m_Stream->ReadWord(&planes);
			m_Stream->ReadWord(&bpp);

			m_Stream->ReadDWord(&datasize);
			m_Stream->ReadDWord(&offset);
#endif

			AddBuffer(width, height);
			m_Current->offset    = offset;
			m_Current->datasize  = datasize;
			m_Current->palette   = palette;

			if (bpp == 0) 
			{ 
				m_Current->bytespp = datasize / (width * height); 
				m_Current->bitspp = 0;
			}
			else
			{
				if (bpp == 4)
				{
					m_Current->bytespp = 1;
				}
				else
				{
					m_Current->bytespp = bpp >> 3;
				}
				m_Current->bitspp = bpp;
			}
		}

		BufferICO* cur = m_First;
		for (uint32 i = 0; i < m_Images; cur = cur->next, i++)
		{
			/*
				typedef struct tagBITMAPINFOHEADER{
				DWORD      biSize;
				LONG       biWidth;
				LONG       biHeight;
				WORD       biPlanes;
				WORD       biBitCount;
				DWORD      biCompression;
				DWORD      biSizeImage;
				LONG       biXPelsPerMeter;
				LONG       biYPelsPerMeter;
				DWORD      biClrUsed;
				DWORD      biClrImportant;
				} BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;
			*/

			unsigned long size2 = sizeof(BITMAPINFOHEADER);

#ifdef OLDMETHOD
			fseek(m_Handle, cur->offset, SEEK_SET);
#else
			//m_Stream->Seek(cur->offset, TIL_FILE_SEEK_CURR);
			m_Stream->Seek(cur->offset, TIL_FILE_SEEK_START);
#endif

			//BITMAPINFOHEADER bleh;
			//fread(&bleh, sizeof(bleh), 1, m_Handle);

#ifdef OLDMETHOD
			dword size;     fread(&size, 4, 1, m_Handle);
			int32 width;     fread(&width, 4, 1, m_Handle);
			int32 height;    fread(&height, 4, 1, m_Handle);
			word planes;   fread(&planes, 2, 1, m_Handle);
			word bitcount; fread(&bitcount, 2, 1, m_Handle);
#else
			dword size;      m_Stream->ReadDWord(&size);
			int32 width;     m_Stream->Read(&width, sizeof(int32));
			int32 height;    m_Stream->Read(&height, sizeof(int32));
			word planes;     m_Stream->ReadWord(&planes);
			word bitcount;   m_Stream->ReadWord(&bitcount);
#endif

#ifdef OLDMETHOD
			fseek(m_Handle, 24, SEEK_CUR);
#else
			m_Stream->Seek(24, TIL_FILE_SEEK_CURR);
#endif

			ICO_DEBUG("Image: %i", i);
			ICO_DEBUG("Dimensions: (%i, %i)", cur->width, cur->height);
			ICO_DEBUG("Bytes per pixel: %i", cur->bytespp);
			ICO_DEBUG("Bitcount: %i", bitcount);

			uint16 colors_used = 0;
			if (bitcount == 8) 
			{ 
				colors_used = 256; 
			} 
			else if (bitcount == 4) 
			{ 
				colors_used = 16; 
			}

			cur->palette = colors_used;

			uint32 current = 0;

			g_ColorFuncRow =  ColorFunc_4b_A8B8G8R8_Row;

			uint32 pitch = cur->pitch * m_BPP;
			cur->pitchy = cur->pitch * m_BPP;

			byte* src;

			byte* target = cur->buffer + ((cur->height - 1) * pitch);

			if (cur->palette > 0)
			{
				ExpandPalette(cur);

				/*for(i=0;i<(icon.bWidth/8)*icon.bHeight;i++)
					*(icon.icAND+i)=getc(fp);*/

				/*for (int i = 0; i < (cur->width / 8) * cur->height; i++)
				{

				}*/

				int div = 1;
				if (cur->palette == 16) { div = 2; }

				cur->pitch = cur->width / div;
				cur->pitchy = cur->height / div;
				//cur->pitchy = cur->height;

				src = new byte[cur->width * cur->pitchy * cur->bytespp];
#ifdef OLDMETHOD
				fread(src, cur->bytespp, cur->width * cur->pitchy, m_Handle); 
#else
				m_Stream->Read(src, cur->bytespp, cur->width * cur->pitchy);
#endif

				cur->andmask = new byte[(cur->width / 8) * cur->height];
#ifdef OLDMETHOD
				fread(cur->andmask, 1, (cur->width / 8) * cur->height, m_Handle);
#else
				m_Stream->Read(cur->andmask, sizeof(byte), (cur->width / 8) * cur->height);
#endif

				byte* read = src;
				byte* and = cur->andmask;

				int offset = (cur->width / 8);

				if (cur->palette == 16)
				{
					for (uint32 y = 0; y < cur->pitchy; y++)
					{
						g_ColorFuncRow(target, read, and, cur);
						target -= pitch;
						read += cur->pitch;
						and += cur->width / 8;

						g_ColorFuncRow(target, read, and, cur);
						target -= pitch;
						read += cur->pitch;
						and += cur->width / 8;
					}
				}
				else
				{
					for (uint32 y = 0; y < cur->pitchy; y++)
					{
						g_ColorFuncRow(target, read, and, cur);
						target -= pitch;
						read += cur->pitch;
						and += cur->width / 8;
					}
				}

				delete src;
				delete cur->andmask;

				/*for(i=0,y=(icon.bHeight)-1;y>-1;y--)
					for(x=0;x<icon.bWidth;x+=8,i++){
						for(z=0;z<8;z++){
							if(!(getbits(icon.icAND[i],z,1)))
								drwpoint(AND,0x00,x+z+icon.x,y+icon.y);
						}
					}*/


			}
			else
			{
				//fseek(m_Handle, cur->offset + 36, SEEK_SET);

				src = new byte[cur->width * cur->bytespp];

				for (uint32 y = 0; y < cur->height; y++)
				{
#ifdef OLDMETHOD
					fread(src, cur->width, cur->bytespp, m_Handle); 
#else
					m_Stream->Read(src, cur->bytespp, cur->width);
#endif

					g_ColorFuncRow(target, src, NULL, cur);
					target -= pitch;
				}

				delete src;
			}

			/*for (uint32 y = 0; y < cur->height / 2; y++)
			{
				fread(src, cur->width, cur->bytespp, m_Handle); 

				g_ColorFuncRow(target, src, cur);
				target -= pitch;

				if (cur->palette > 0)
				{
					g_ColorFuncRow(target, src + (cur->width / 2), cur);
					target -= pitch;
				}
			}*/

			//uint32 offset = datasize - current;

			//fseek(m_Handle, datasize, SEEK_SET);
			//fseek(m_Handle, offset, SEEK_CUR);
		}

		return true;
	}

	uint32 ImageICO::GetFrameCount()
	{
		return m_Images;
	}

	byte* ImageICO::GetPixels(uint32 a_Frame /*= 0*/)
	{
		if (a_Frame >= 0 && a_Frame < m_Images)
		{
			BufferICO* current = m_First;
			for (uint32 i = 0; i < a_Frame; i++) { current = current->next; }
			return current->buffer;
		}

		return NULL;
	}

	uint32 ImageICO::GetWidth(uint32 a_Frame)
	{
		if (a_Frame >= 0 && a_Frame < m_Images)
		{
			BufferICO* current = m_First;
			for (uint32 i = 0; i < a_Frame; i++) { current = current->next; }
			return current->width;
		}

		return 0;
	}

	uint32 ImageICO::GetHeight(uint32 a_Frame)
	{
		if (a_Frame >= 0 && a_Frame < m_Images)
		{
			BufferICO* current = m_First;
			for (uint32 i = 0; i < a_Frame; i++) { current = current->next; }
			return current->height;
		}

		return 0;
	}

	void ImageICO::ReleaseMemory(BufferICO* a_Buffer)
	{
		if (a_Buffer && a_Buffer->next) 
		{
			ReleaseMemory(a_Buffer->next); 
		}
		delete a_Buffer->next;
		delete a_Buffer->buffer;
		if (a_Buffer->colors) { delete a_Buffer->colors; }
	}

}; // namespace til

#endif