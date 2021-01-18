/* nt4_dev.c - routines for direct drive access in Windows NT 4.0
 *
 * Copyright 1999 by Dan Sutherland <dan@chromerhino.demon.co.uk>
 *
 * These routines only currently work with drives <2GB and 512 bytes per sector
 *
 *  This file is part of ADFLib.
 *
 *  ADFLib is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  ADFLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include "nt4_dev.h"

HANDLE NT4OpenDrive(char *lpstrDrive)
{
	char strDriveFile[40];
	HANDLE hDrv;
	DWORD dwRet;

	switch (lpstrDrive[0]) {
		case 'H':
			sprintf(strDriveFile, "\\\\.\\PhysicalDrive%c", lpstrDrive[1]);
			break;
		/* add support for other device types here */
		default:
			return NULL;
	}

	hDrv = CreateFile(strDriveFile, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		0, NULL);

	if (hDrv == INVALID_HANDLE_VALUE)
		return NULL;

	if (! DeviceIoControl(hDrv, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,
		&dwRet, NULL))
		return NULL;

	return hDrv;
}

BOOL NT4CloseDrive(HANDLE hDrv)
{
	DWORD dwRet;

	if( hDrv==NULL ) return TRUE;										/* BV */

	if (! DeviceIoControl(hDrv, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0,
		&dwRet,	NULL))
		return FALSE;

	if (! CloseHandle(hDrv))
		return FALSE;

	return TRUE;
}

BOOL NT4ReadSector(HANDLE hDrv, long iSect, int iSize, void *lpvoidBuf)
{
	void *lpvoidTempBuf;
	DWORD dwActual;

	lpvoidTempBuf = VirtualAlloc(NULL, 512, MEM_COMMIT, PAGE_READWRITE);

	if (SetFilePointer(hDrv, iSect * 512, NULL, FILE_BEGIN) == 0xFFFFFFFF) {
		VirtualFree(lpvoidTempBuf, 0, MEM_RELEASE);
		return FALSE;
	}

	if (! ReadFile(hDrv, lpvoidTempBuf, 512, &dwActual, NULL)) {
		VirtualFree(lpvoidTempBuf, 0, MEM_RELEASE);
		return FALSE;
	}

	memcpy(lpvoidBuf, lpvoidTempBuf, iSize);
	VirtualFree(lpvoidTempBuf, 0, MEM_RELEASE);

	return TRUE;
}

BOOL NT4WriteSector(HANDLE hDrv, long iSect, int iSize, void *lpvoidBuf)
{
	void *lpvoidTempBuf;
	DWORD dwActual;

	if (iSize != 512)
		return FALSE;

	lpvoidTempBuf = VirtualAlloc(NULL, 512, MEM_COMMIT, PAGE_READWRITE);

	if (SetFilePointer(hDrv, iSect * 512, NULL, FILE_BEGIN) == 0xFFFFFFFF) {
		VirtualFree(lpvoidTempBuf, 0, MEM_RELEASE);
		return FALSE;
	}

	memcpy(lpvoidTempBuf, lpvoidBuf, iSize);

	if (! WriteFile(hDrv, lpvoidTempBuf, 512, &dwActual, NULL)) {
		VirtualFree(lpvoidTempBuf, 0, MEM_RELEASE);
		return FALSE;
	}
	
	VirtualFree(lpvoidTempBuf, 0, MEM_RELEASE);

	return TRUE;
}

ULONG NT4GetDriveSize(HANDLE hDrv)
{
	DWORD dwActual;
	DISK_GEOMETRY dgGeom;
	long size;

	DeviceIoControl(hDrv, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
		&dgGeom, sizeof(DISK_GEOMETRY), &dwActual, NULL);

	size =  dgGeom.Cylinders.LowPart * dgGeom.TracksPerCylinder *
		dgGeom.SectorsPerTrack * dgGeom.BytesPerSector;
/* BV */
/*	printf("Total sectors: %i\n", dgGeom.Cylinders.LowPart * dgGeom.TracksPerCylinder * dgGeom.SectorsPerTrack);
**	printf("Byte size: %i\n", size);
*/
	return size;
}
