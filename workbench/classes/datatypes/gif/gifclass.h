/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

typedef struct {
    struct IFFHandle		*filehandle;

	UBYTE					*filebuf;
	UBYTE					*filebufpos;
	long					filebufbytes;
	long					filebufsize;

	UBYTE					*linebuf;
	UBYTE					*linebufpos;
	long					linebufbytes;
	long					linebufsize;
} GifHandleType;

extern BOOL LoadGIF_FillBuf(GifHandleType *gifhandle, long minbytes);
