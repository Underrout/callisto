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

#include "../include/Junction.h"

#include <strsafe.h>
#include <winioctl.h>

#include "../include/CharUtils.h"
#include "../include/ntfstypes.h"

namespace libntfslinks
{

bool IsJunction(LPCTSTR Path)
{
	bool result = false;
	HANDLE fileHandle;

	// We need an existing file handle in order to call DeviceIoControl
	fileHandle = CreateFile(Path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		// Allocate enough space to fit the maximum sized reparse data buffer
		char Tmp[MAXIMUM_REPARSE_DATA_BUFFER_SIZE] = {0};
		// Redefine the above char array into a REPARSE_DATA_BUFFER we can work with
		REPARSE_DATA_BUFFER& reparseData = *(REPARSE_DATA_BUFFER*)Tmp;

		// Call DeviceIoControl to get the reparse point data
		DWORD bytesRet;
		if (DeviceIoControl(fileHandle, FSCTL_GET_REPARSE_POINT, NULL, 0, &reparseData, sizeof(Tmp), &bytesRet, NULL))
		{
			// The reparse tag indicates if this is a junction or not
			result = reparseData.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT;
		}

		// Close the handle
		CloseHandle(fileHandle);
	}

	return result;
}

DWORD GetJunctionTarget(LPCTSTR Path, LPTSTR TargetPath, size_t TargetSize)
{
	DWORD result = (DWORD)E_FAIL;
	HANDLE fileHandle;

	// We need an existing file handle in order to call DeviceIoControl
	fileHandle = CreateFile(Path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		// Allocate enough space to fit the maximum sized reparse data buffer
		char Tmp[MAXIMUM_REPARSE_DATA_BUFFER_SIZE] = {0};
		// Redefine the above char array into a REPARSE_DATA_BUFFER we can work with
		REPARSE_DATA_BUFFER& reparseData = *(REPARSE_DATA_BUFFER*)Tmp;

		// Call DeviceIoControl to get the reparse point data
		DWORD bytesRet;
		if (DeviceIoControl(fileHandle, FSCTL_GET_REPARSE_POINT, NULL, 0, &reparseData, sizeof(Tmp), &bytesRet, NULL))
		{
			// Verify quickly that the reparse tag is what we expect
			if (reparseData.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
			{
				// Copy the target out of the struct into a temporary string
				WCHAR Tmp[MAX_PATH] = {0};
				LPSTR;
				if (SUCCEEDED(StringCchCopyNW(Tmp, MAX_PATH, &reparseData.MountPointReparseBuffer.PathBuffer[reparseData.MountPointReparseBuffer.PrintNameOffset/sizeof(WCHAR)], reparseData.MountPointReparseBuffer.PrintNameLength+1)))
				{
					// Copy the temp string to the provided Target buffer
					result = WCHARtoTCHAR(Tmp, min(static_cast<int>(reparseData.MountPointReparseBuffer.PrintNameLength), MAX_PATH), TargetPath, TargetSize) == S_OK ? S_OK : GetLastError();
				}
				else
				{
					result = GetLastError();
				}
			}
		}
		else
		{
			result = GetLastError();
		}

		// Close the handle
		CloseHandle(fileHandle);
	}
	else
	{
		result = GetLastError();
	}

	return result;
}

DWORD CreateJunction(LPCTSTR Link, LPCTSTR Target)
{
	DWORD result = (DWORD)E_FAIL;
	HANDLE fileHandle;

	// Attempt to expand the full path of Target.
	TCHAR FullTarget[MAX_PATH];
	if (GetFullPathName(Target, MAX_PATH, FullTarget, NULL) == 0)
	{
		return GetLastError();
	}

	// Create the junction as a normal directory first
	if (!CreateDirectory(Link, NULL))
	{
		return GetLastError();
	}

	// Open a new handle for the junction
	fileHandle = CreateFile(Link, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		// Allocate enough space to fit the maximum sized reparse data buffer
		char Tmp[MAXIMUM_REPARSE_DATA_BUFFER_SIZE] = {0};
		// Redefine the above char array into a REPARSE_DATA_BUFFER we can work with
		REPARSE_DATA_BUFFER& reparseData = *(REPARSE_DATA_BUFFER*)Tmp;

		// Set the type of reparse point we are creating
		reparseData.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
		reparseData.Reserved = 0;

		// Copy the junction's target
		TCHAR SubstituteName[MAX_PATH] = {0};
		StringCchCopy(SubstituteName, MAX_PATH, TEXT("\\??\\"));
		StringCbCat(SubstituteName, MAX_PATH, FullTarget);
		size_t SubstituteNameLength;
		StringCchLength(SubstituteName, MAX_PATH, &SubstituteNameLength);
		TCHARtoWCHAR(SubstituteName, SubstituteNameLength, reparseData.MountPointReparseBuffer.PathBuffer, sizeof(Tmp) - sizeof(REPARSE_DATA_BUFFER));
		reparseData.MountPointReparseBuffer.SubstituteNameLength = (USHORT)(SubstituteNameLength * sizeof(WCHAR));

		// Copy the junction's link name
		size_t LinkNameLength;
		StringCchLength(FullTarget, MAX_PATH, &LinkNameLength);
		TCHARtoWCHAR(FullTarget, LinkNameLength, &reparseData.MountPointReparseBuffer.PathBuffer[SubstituteNameLength + 1], sizeof(Tmp) - sizeof(REPARSE_DATA_BUFFER) - reparseData.MountPointReparseBuffer.SubstituteNameOffset);
		reparseData.MountPointReparseBuffer.PrintNameOffset = (USHORT)(reparseData.MountPointReparseBuffer.SubstituteNameLength + sizeof(WCHAR));
		reparseData.MountPointReparseBuffer.PrintNameLength = (USHORT)(LinkNameLength * sizeof(WCHAR));

		// Set the total size of the data buffer
		reparseData.ReparseDataLength = (USHORT)(sizeof(reparseData.MountPointReparseBuffer) +
			reparseData.MountPointReparseBuffer.PrintNameOffset + reparseData.MountPointReparseBuffer.PrintNameLength);

		// Now tell DeviceIoControl to set the reparse point data on the file handle
		size_t HeaderSize = FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer);
		if (DeviceIoControl(fileHandle, FSCTL_SET_REPARSE_POINT, &reparseData, (DWORD)HeaderSize+reparseData.ReparseDataLength, NULL, 0, NULL, 0))
		{
			result = S_OK;
		}
		else
		{
			result = GetLastError();
		}

		// Close the handle
		CloseHandle(fileHandle);
	}
	else
	{
		result = GetLastError();
	}

	return result;
}

DWORD DeleteJunction(LPCTSTR Path)
{
	DWORD result = (DWORD)E_FAIL;
	HANDLE fileHandle;

	// We need an existing file handle in order to call DeviceIoControl
	fileHandle = CreateFile(Path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		REPARSE_GUID_DATA_BUFFER rgdb = {0};
		rgdb.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;

		// Tell DeviceIoControl to delete the reparse point data
		BOOL bReparseDeleted = DeviceIoControl(fileHandle, FSCTL_DELETE_REPARSE_POINT, &rgdb, REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, NULL, 0, NULL, 0);
		result = bReparseDeleted ? S_OK : GetLastError();

		// Close the handle
		CloseHandle(fileHandle);
	}
	else
	{
		result = GetLastError();
	}
	
	return result;
}

} // namespace libntfslinks
