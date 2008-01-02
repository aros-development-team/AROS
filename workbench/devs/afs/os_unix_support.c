/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * -date------ -name------------------- -description-----------------------------
 * 02-jan-2008 [Tomasz Wiszkowski]      added disk check option for broken disks
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "os.h"
#include "error.h"
#include "volumes.h"

void showPtrArgsText(struct AFSBase *afsbase, char *string, va_list args) {
	vprintf(string, args);
	printf("\n");
}

void showText(struct AFSBase *afsbase, char *string, ...) {
va_list ap;

	va_start(ap, string);
	showPtrArgsText(afsbase, string, ap);
	va_end(ap);
}

LONG showError(struct AFSBase *afsbase, ULONG error, ...) {
char *texts[]={0,
            "No ioport",
            "Couldn't open device %s",
            "Couldn't add disk as dosentry",
            "Disk is not validated!\n",
            "Wrong data block %ld",
            "Wrong checksum on block %ld",
            "Missing some more bitmap blocks",
            "Wrong blocktype on block %ld",
            "Read/Write Error (%ld)",
		      "*** This may be a non-AFS disk. ***\n"
			      "Any attempt to fix it in this case may render the original\n"
			      "file system invalid, and its contents unrecoverable.\n\n"
			      "Please select what to do.",
            "Block %lu used twice",
            "Block %lu is located outside volume scope\nand will be removed.",
            "Repairing disk structure will lead to data loss.\n"
               "It's best to make a backup before proceeding.\n\n"
               "Please select what to do.",
            0,
            "Unknown error"
};

   if (error==ERR_ALREADY_PRINTED)
      return 0;
   if (error>=ERR_UNKNOWN)
   {
      showText(afsbase, texts[ERR_UNKNOWN], error);
   }
   else
	{
		va_list ap;
		va_start(ap, error);
      showPtrArgsText(afsbase, texts[error], ap);
		va_end(ap);
	}
   return 0;
}

LONG readDisk
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		ULONG start, ULONG count, APTR mem
	)
{
struct IOHandle *ioh;

	ioh = &volume->ioh;
	if (fseek(ioh->fh, start*512, SEEK_SET) == 0)
	{
		if (fread(mem, 512, count, ioh->fh) == count)
		{
			return 0;
		}
	}
	return 1;
}

LONG writeDisk
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		ULONG start, ULONG count, APTR mem
	)
{
struct IOHandle *ioh;

	ioh = &volume->ioh;
	if (fseek(ioh->fh, start*512, SEEK_SET) == 0)
	{
		if (fwrite(mem, 512, count, ioh->fh) == count)
		{
			return 0;
		}
	}
	return 1;
}

UBYTE diskPresent(struct AFSBase *afsbase, struct IOHandle *ioh) {
	return ioh->fh ? 1 : 0;
}

void check64BitSupport(struct AFSBase *afsbase, struct Volume *volume) {
	printf("%s: We just support 64Bit (or not ...)\n", __FUNCTION__);
}

struct IOHandle *openBlockDevice(struct AFSBase *afsbase, struct IOHandle *ioh) {
	ioh->fh = fopen(ioh->blockdevice, "r+");
	if (ioh->fh != NULL)
	{
		ioh->ioflags |= IOHF_DISK_IN;
		return ioh;
	}
	else
		showError(afsbase, ERR_DEVICE, ioh->blockdevice);
	return NULL;
}

void closeBlockDevice(struct AFSBase *afsbase, struct IOHandle *ioh) {
	fclose(ioh->fh);
	ioh->fh = NULL;
	ioh->ioflags &= ~IOHF_DISK_IN;
}

BOOL flush(struct AFSBase *afsbase, struct Volume *volume) {
	flushCache(afsbase, volume);
	clearCache(afsbase, volume->blockcache);
	return DOSFALSE;
}

LONG osMediumInit(struct AFSBase *afsbase, struct Volume *volume, struct BlockCache *blockbuffer) {
	printf("%s: Don't know what to do here\n", __FUNCTION__);
	return 0;
}

void osMediumFree(struct AFSBase *afsbase, struct Volume *volume, LONG all) {
	printf("%s: Don't know what to do here\n", __FUNCTION__);
}

/********************************* OS Functions *****************************/
/* exec */
void *AllocMem(ULONG size, ULONG flags) {
void *mem;

	if (flags & MEMF_CLEAR)
		mem = calloc(1, size);
	else
		mem = malloc(size);
	return mem;
}

void *AllocVec(ULONG size, ULONG flags) {
	return AllocMem(size, flags);
}

void FreeMem(APTR mem, ULONG size) {
	free(mem);
}

void FreeVec(APTR mem) {
	free(mem);
}

void CopyMem(APTR src, APTR dst, ULONG size) {
	memcpy(dst, src, size);
}

/* dos */
struct DateStamp *DateStamp(struct DateStamp *ds) {
time_t current;
time_t diff;
struct tm as={0, 0, 0, 1, 0, 78, -1, -1, -1};

	time(&current);
	diff = mktime(&as);
	current -= diff; /* time since 00:00:00, Jan 1, 1978 */
	ds->ds_Days = current/60/60/24;
	current -= (ds->ds_Days*60*60*24);
	ds->ds_Minute = current/60;
	current -= (ds->ds_Minute*60);
	ds->ds_Tick = current*50;
	return ds;
}

STRPTR PathPart(STRPTR path) {
STRPTR ptr;

    /* '/' at the begining of the string really is part of the path */
    while (*path == '/')
    {
        ++path;
    }
                                                                                
    ptr = path;
                                                                                
    while (*ptr)
    {
        if (*ptr == '/')
        {
            path = ptr;
        }
        else if (*ptr == ':')
        {
            path = ptr + 1;
        }
                                                                                
        ptr++;
    }
                                                                                
    return path;
}
