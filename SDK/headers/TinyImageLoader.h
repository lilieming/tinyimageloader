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

#ifndef _TINYIMAGELOADER_H_
#define _TINYIMAGELOADER_H_

/*! \file TinyImageLoader.h
    \brief The main include for using TinyImageLoader in your project.
    
    Details.
*/

#include "TILSettings.h"
#include "TILImage.h"

//! TinyImageLoader namespace.

namespace til
{

	//! Initializes TinyImageLoader. 
	/*! 
		\param a_Settings Settings for TinyImageLoader.

		Starts TinyImageLoader and determines vital settings.

		Valid settings are a combination of:

		Line endings
		- #TIL_FILE_CRLF
		- #TIL_FILE_CR
		- #TIL_FILE_LF
	*/
	extern void TIL_Init(uint32 a_Settings = TIL_SETTINGS);

	extern void TIL_ShutDown();

	//! Interprets data in memory as an image loaded from disk.
	extern Image* TIL_LoadFromMemory(byte* a_Data, uint32 a_Options = TIL_DEPTH_A8R8G8B8);

	//! Main interface for loading images.
	/*!
		\param a_FileName A string containing either a path to an image.
		\param a_Options A combination of loading options.

		\return An instance of #Image or NULL on failure.

		When #TIL_FILE_ABSOLUTEPATH is specified, a_FileName is treated as a complete path.
		If #TIL_FILE_ADDWORKINGDIR is specified, the working directory is appended.

		A color depth is required, or the function returns NULL.

		Valid options include a combination of a file option:
		- #TIL_FILE_ABSOLUTEPATH
		- #TIL_FILE_ADDWORKINGDIR

		And a color depth option:
		- #TIL_DEPTH_A8R8G8B8
		- #TIL_DEPTH_A8B8G8R8
		- #TIL_DEPTH_R8G8B8A8
		- #TIL_DEPTH_B8G8R8A8
		- #TIL_DEPTH_R8G8B8
		- #TIL_DEPTH_R5G6B5
	*/
	extern Image* TIL_Load(const char* a_FileName, uint32 a_Options = (TIL_FILE_ABSOLUTEPATH | TIL_DEPTH_A8R8G8B8));

	//! Set the internal working directory
	/*!
		\param a_Path The path to set the working directory to.
		\param a_Length The length of the path string.

		\return The length of the string saved.

		This function sets the internal working directory to the value specified.
		When you load an image using #TIL_FILE_ADDWORKINGDIR, this is the string
		that is prepended to the path.

		On Windows systems, this value is set to the value returned by 
		GetModuleFileNameA, which is the path to folder the executable is located.

		On other systems, this value is left blank.
	*/
	extern size_t TIL_SetWorkingDirectory(const char* a_Path, size_t a_Length);

	//! Get the version as a string.
	/*!
		\param a_Target The string to write to.
		\param a_MaxLength The size of the destination buffer.

		Get the library version as a string.

		\code
		char version[32];
		TIL_GetVersion(version, 32); // version now contains "1.5.0"
		\endcode
	*/
	extern void TIL_GetVersion(char* a_Target, size_t a_MaxLength);

	extern void TIL_SetErrorFunc(MessageFunc a_Func);

	//! Get the errors as a string

	/*!
		\return Error string.

		If no errors were reported, the string is empty.
		An error is in indication something went wrong while loading an image. 

		The messages are in the following format:

		\code
		TinyImageLoader - Error: Something terrible happened!
		\endcode
	*/
	extern char* TIL_GetError();

	//! Get the length of the error string
	/*!
		\return Length.

		If no errors were reported, this function returns 0.
	*/
	extern size_t TIL_GetErrorLength();

	extern void TIL_SetDebugFunc(MessageFunc a_Func);

	//! Get the debug messages as a string
	/*!
		\return Debug string.

		If no debug messages were reported, the string is empty. 
		Debug messages are for the developers of this library, not for an end-user.
		Only when compiled under debug mode does the program generate these messages.

		The messages are in the following format:

		\code
		TinyImageLoader - Debug: I figured you'd like this.
		\endcode
	*/
	extern char* TIL_GetDebug();

	//! Get the length of the debug string
	/*!
		\return Length.

		If no debug messages were posted, this function returns 0.
	*/
	extern size_t TIL_GetDebugLength();

}; // namespace til

#endif