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

#include "../include/StringUtils.h"

#include <strsafe.h>

int StrFind(LPCTSTR Str, LPCTSTR Sub, int StartIdx, int Dir)
{
	int result = -1;

	// Grab the length of the string to search
	size_t StrLength;
	if (FAILED(StringCchLength(Str, MAX_PATH, &StrLength)) || StrLength == 0)
	{
		return result;
	}

	// Grab the length of the substring
	size_t SubLength;
	if (FAILED(StringCchLength(Sub, MAX_PATH, &SubLength)) || SubLength == 0)
	{
		return result;
	}

	// When the substring is longer it is not possible to find it
	if (SubLength > StrLength)
	{
		return result;
	}

	// Make sure the StartIdx is not beyond StrLength
	if (StartIdx >= (int)StrLength)
	{
		return result;
	}

	// If StartIdx is negative count backwards from the length
	if (StartIdx < 0)
	{
		StartIdx += (int)StrLength;
	}

	int StrIdx = min(StartIdx, (int)StrLength-1);
	int SubIdx = Dir < 0 ? (int)SubLength - 1 : 0;
	for (; StrIdx >= 0 && StrIdx < (int)StrLength; StrIdx = StrIdx + Dir)
	{
		if (Str[StrIdx] == Sub[SubIdx])
		{
			if (result < 0)
			{
				result = StrIdx;
			}

			SubIdx = SubIdx + Dir;
		}
		else if (result >= 0 && (StrIdx - result) * Dir >= (int)SubLength)
		{
			break;
		}
		else
		{
			result = -1;
			SubIdx = Dir < 0 ? (int)SubLength - 1 : 0;
		}
	}

	// When searching in reverse correct the starting index
	if (result >= 0 && Dir < 0)
	{
		result = result - (int)SubLength + 1;
	}

	return result;
}

bool StrReplace(LPCTSTR SrcStr, LPCTSTR Search, LPCTSTR Replace, LPTSTR DestStr, int StartIdx, int Dir)
{
	// Determine the length of SrcStr
	size_t SrcStrLength;
	if (FAILED(StringCchLength(SrcStr, MAX_PATH, &SrcStrLength)) || SrcStrLength == 0)
	{
		return false;
	}

	// Determine the length of Search
	size_t SearchLength;
	if (FAILED(StringCchLength(Search, MAX_PATH, &SearchLength)) || SearchLength == 0)
	{
		return false;
	}

	// Find the starting index of Search in SrcStr
	int FoundIdx = StrFind(SrcStr, Search, StartIdx, Dir);

	// Copy the beginning of SrcStr and swap out the Search for Replace
	if (FoundIdx >= 0)
	{
		StringCchCopy(DestStr, FoundIdx+1, SrcStr);
		StringCchCat(DestStr, MAX_PATH, Replace);
		StringCchCat(DestStr, MAX_PATH, &SrcStr[FoundIdx+SearchLength]);
	}
	// When no substring is found copy the source to the destination in full
	else
	{
		StringCchCopy(DestStr, MAX_PATH, SrcStr);
	}

	return true;
}