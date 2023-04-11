///////////////////////////////////////////////////////////////////////////////
//
// This file is part of libntfslinks.
//
// Copyright (c) 2014, Jean-Philippe Steinmetz
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "../include/CharUtils.h"

#include <strsafe.h>

namespace libntfslinks
{

/**
 * Utilty function for converting a WCHAR string to a TCHAR string.
 */
bool WCHARtoTCHAR(WCHAR* src, size_t srcSize, TCHAR* dest, size_t destSize)
{
#ifdef UNICODE
	return StringCchCopy(dest, std::min(srcSize+1, destSize), (TCHAR*)src) == S_OK;
#else
	return WideCharToMultiByte(CP_UTF8, 0, src, (int)srcSize, dest, (int)destSize, NULL, NULL) != 0;
#endif
}

/**
 * Utility function for converting a TCHAR string to a WCHAR string.
 */
bool TCHARtoWCHAR(TCHAR* src, size_t srcSize, WCHAR* dest, size_t destSize)
{
#ifdef UNICODE
	return StringCchCopy(dest, std::min(srcSize+1, destSize), (TCHAR*)src) == S_OK;
#else
	return MultiByteToWideChar(CP_UTF8, 0, src, (int)srcSize, dest, (int)destSize) != 0;
#endif
}

} // namespace libntfslinks