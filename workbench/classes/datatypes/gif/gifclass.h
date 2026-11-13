/*
    Copyright ©	1995-2001, The AROS Development	Team. All rights reserved.
    $Id$
*/

typedef	struct {
    union {
	struct IFFHandle *iff;
	BPTR bptr;
    } filehandle;

    UBYTE		*filebuf;
    UBYTE		*filebufpos;
    long		filebufbytes;
    long		filebufsize;
    UBYTE		*linebuf;
    UBYTE		*linebufpos;
    long		linebufbytes;
    long		linebufsize;
    
    APTR		codecvars;
} GifHandleType;

BOOL LoadGIF_FillBuf(GifHandleType *gifhandle, long minbytes);
BOOL SaveGIF_EmptyBuf(GifHandleType *gifhandle,	long minbytes);
