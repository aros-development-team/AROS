#ifndef ERROR_H
#define ERROR_H

/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "afshandler.h"

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

void showText(struct afsbase *, char *, ...);
void showError(struct afsbase *, ULONG, ...);

#endif
