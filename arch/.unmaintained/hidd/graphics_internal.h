/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: GfxHIDD specific Internal Information Management
    Lang: english
*/

/* A Pointer to this struct is stored in each RastPort->longreserved[0] */

struct gfx_driverdata {
    UWORD	* dd_AreaPtrn;		/* Amiga current AreaPtrn	*/
    BYTE	  dd_AreaPtSz;		/* Amiga AreaPtSz		*/
    UWORD	  dd_LinePtrn;		/* Amiga current LinePtrn	*/
    Object	* dd_GC;
    struct RastPort * dd_RastPort;	/* This RastPort		*/
};

