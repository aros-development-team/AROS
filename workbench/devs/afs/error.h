#ifndef ERROR_H
#define ERROR_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"

enum {
	ERR_NONE,
	ERR_IOPORT,
	ERR_DEVICE,
	ERR_DOSENTRY,
	ERR_DISKNOTVALID,
	ERR_WRONG_DATA_BLOCK,
	ERR_CHECKSUM,						// block errors
	ERR_MISSING_BITMAP_BLOCKS,
	ERR_BLOCKTYPE,
	ERR_READWRITE,
	ERR_ALREADY_PRINTED,
	ERR_UNKNOWN
};

void showText(struct AFSBase *, char *, ...);
void showError(struct AFSBase *, ULONG, ...);
LONG showRetriableError(struct AFSBase *, TEXT *, ...);

#endif
