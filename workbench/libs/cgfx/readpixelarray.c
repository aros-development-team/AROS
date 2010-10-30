/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH10(ULONG, ReadPixelArray,

/*  SYNOPSIS */
	AROS_LHA(APTR             , dst		, A0),
	AROS_LHA(UWORD            , destx	, D0),
	AROS_LHA(UWORD            , desty	, D1),
	AROS_LHA(UWORD            , dstmod	, D2),
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , srcx	, D3),
	AROS_LHA(UWORD            , srcy	, D4),
	AROS_LHA(UWORD            , width	, D5),
	AROS_LHA(UWORD            , height	, D6),
	AROS_LHA(UBYTE            , dstformat	, D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 20, Cybergraphics)

/*  FUNCTION

    INPUTS

        dstformat - A RECTFMT_xxx value describing requested format in which
                    data in dst will be available. Currently supported original
                    values are:
                    
                    RECTFMT_RGB
                    RECTFMT_RGBA
                    RECTFMT_ARGB
                    RECTFMT_RAW
                    
                    Currently supported AROS extensions are:

                    RECTFMT_RGB15
                    RECTFMT_BGR15
                    RECTFMT_RGB15PC
                    RECTFMT_BGR15PC
                    RECTFMT_RGB16
                    RECTFMT_BGR16
                    RECTFMT_RGB16PC
                    RECTFMT_BGR16PC
                    RECTFMT_RGB24
                    RECTFMT_BGR24
                    RECTFMT_0RGB32
                    RECTFMT_BGR032
                    RECTFMT_RGB032
                    RECTFMT_0BGR32
                    RECTFMT_ARGB32
                    RECTFMT_BGRA32
                    RECTFMT_RGBA32
                    RECTFMT_ABGR32
                    
                    Following values are not supported and will cause function
                    to return 0:
                    
                    RECTFMT_LUT8
                    RECTFMT_GREY8

    RESULT

        Number of pixels read.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    cybergraphics_lib.fd and clib/cybergraphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (width && height)
    {
	return driver_ReadPixelArray(dst
    	    , destx, desty
	    , dstmod
	    , rp
	    , srcx, srcy
	    , width, height
	    , dstformat
	    , GetCGFXBase(CyberGfxBase)
        );
    }
    else return 0;
    
    AROS_LIBFUNC_EXIT
} /* ReadPixelArray */
