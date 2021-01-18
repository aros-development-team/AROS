/* Win32/adf_nativ.h
 *
 * Win32 specific drive access routines for ADFLib
 * Copyright 1999 by Dan Sutherland <dan@chromerhino.demon.co.uk>
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
	char path[4096];		// Add a path variable for the Opus info dialogue.
};							// This modifies the standard ADFLib.

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
