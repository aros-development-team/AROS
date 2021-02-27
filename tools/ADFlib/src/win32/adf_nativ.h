/* Win32/adf_nativ.h
 *
 * Win32 specific drive access routines for ADFLib
 * Copyright 1999 by Dan Sutherland <dan@chromerhino.demon.co.uk>
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
 *
 */

#ifndef ADF_NATIV_H
#define ADF_NATIV_H

#include "../adf_str.h"

#define NATIVE_FILE  8001

#ifndef BOOL
#define BOOL int
#endif

#ifndef RETCODE
#define RETCODE long
#endif

struct nativeDevice{
	FILE *fd; /* needed by adf_dump.c */
	void *hDrv;
};

struct nativeFunctions{
	RETCODE (*adfInitDevice)(struct Device*, char*, BOOL);
	RETCODE (*adfNativeReadSector)(struct Device*, long, int, unsigned char*);
	RETCODE (*adfNativeWriteSector)(struct Device*, long, int, unsigned char*);
	BOOL (*adfIsDevNative)(char*);
	RETCODE (*adfReleaseDevice)();
};

void adfInitNativeFct();

RETCODE Win32ReadSector(struct Device *dev, long n, int size, unsigned char* buf);
RETCODE Win32WriteSector(struct Device *dev, long n, int size, unsigned char* buf);
RETCODE Win32InitDevice(struct Device *dev, char* name, BOOL ro);
RETCODE Win32ReleaseDevice(struct Device *dev);
BOOL Win32IsDevNative(char*);

#endif /* ndef ADF_NATIV_H */
