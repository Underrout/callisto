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

#ifndef STRINGUTILS_H
#define STRINGUTILS_H
#pragma once

#include <Windows.h>

/**
 * Finds the first occurrence of the string Sub in string Str.
 *
 * @param Str The string to search for the substring.
 * @param Sub The string to search for in Str.
 * @param StartIdx The index of Str to begin the search from. A negative starting index will count backwards from the
 *			length of Str. Default is 0.
 * @param Dir The direction to perform the search in. Set to 1 for a left-to-right search, set to -1 for a
 *		right-to-left search.
 * @return Returns the starting index of the substring in Str or -1 if not found.
 */
int StrFind(LPCTSTR Str, LPCTSTR Sub, int StartIdx = 0, int Dir = 1);

/**
 * Searches a string for the first occurrence of a provided search string and replaces it with another.
 *
 * @param SrcStr The source string to search for the substring and perform replacement on.
 * @param Search The substring to search for.
 * @param Replace The string to replace the substring with.
 * @param DestStr The destination to write the resulting string to.
 * @param StartIdx The index of Str to begin the search from. A negative starting index will count backwards from the
 *			length of SrcStr. Default is 0.
 * @param Dir The direction to perform the search in. Set to 1 for a left-to-right search, set to -1 for a
 *		right-to-left search.
 * @return Returns true if the operation was successful, otherwise false.
 */
bool StrReplace(LPCTSTR SrcStr, LPCTSTR Search, LPCTSTR Replace, LPTSTR DestStr, int StartIdx = 0, int Dir = 1);

#endif //STRINGUTILS_H