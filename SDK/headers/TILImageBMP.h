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

#ifndef _TILIMAGEBMP_H_
#define _TILIMAGEBMP_H_

#include "TILImage.h"

#if (TIL_FORMAT & TIL_FORMAT_BMP)

namespace til
{

	//! Halloooow.

	class ImageBMP : public Image
	{

		enum ColorType
		{
			COLOR_BLACKANDWHITE,
			COLOR_MAPPED,
			COLOR_TRUECOLOR
		};

		enum Header
		{
			HDR_OS2V1 = 12,
			HDR_OS2V2 = 64,
			HDR_WINDOWSV3 = 40,
			HDR_WINDOWSV4 = 108,
			HDR_WINDOWSV5 = 124
		};

		enum Compression
		{
			COMP_RGB = 0,
			COMP_RLE8 = 1,
			COMP_RLE4 = 2,
			COMP_BITFIELDS = 3,
			COMP_JPEG = 4,
			COMP_PNG = 5
		};

	public:

		ImageBMP();
		~ImageBMP();

		bool Parse(uint32 a_ColorDepth);

		uint32 GetFrameCount();
		byte* GetPixels(uint32 a_Frame = 0);

		uint32 GetWidth(uint32 a_Frame = 0);
		uint32 GetHeight(uint32 a_Frame = 0);

	private:

		//! Bla bla
		dword GetDWord();

		uint32 m_Depth;

		byte m_Data[4];
		byte* m_Pixels;
		byte* m_Target;

		uint32 m_Width, m_Height, m_Pitch;

	}; // class ImageBMP

}; // namespace til

#endif

#endif