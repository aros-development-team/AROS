/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Main fileof diskfont function AvailFonts()
    Lang: english
*/


#include "diskfont_intern.h"
#include <string.h>

#include <aros/debug.h>

/****************************************************************************/

struct BufferInfo
{
    struct AvailFontsHeader *afh;
    LONG space;
    union {
	struct TAvailFonts *taf;
	struct AvailFonts *af;
    } u;
    UBYTE *endptr;
};


STATIC struct BufferInfo *BufferInfoCreate(STRPTR buffer, LONG bufBytes, BOOL tagged, struct DiskfontBase_intern *DiskfontBase);
STATIC VOID BufferInfoAdd(struct BufferInfo *bi, UWORD type, struct TTextAttr *tattr, BOOL tagged, struct DiskfontBase_intern *DiskfontBase);
STATIC VOID BufferInfoFree(struct BufferInfo *bi, struct DiskfontBase_intern *DiskfontBase);

/*****************************************************************************

    NAME */
#include <clib/diskfont_protos.h>

	AROS_LH3(LONG, AvailFonts,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, buffer, A0),
	AROS_LHA(LONG  , bufBytes, D0),
	AROS_LHA(LONG  , flags, D1),

/*  LOCATION */
	struct Library *, DiskfontBase, 6, Diskfont)

/*  FUNCTION
        Fill the supplied buffer with info about the available fonts.
        The buffer will after function execution first contains a 
        struct AvailFontsHeader, and then an array of struct AvailFonts 
        element (or TAvailFonts elements if AFF_TAGGED is specified in the
        flags parameter). If the buffer is not big enough for the
        descriptions than the additional length needed will be returned.

    INPUTS
        buffer    - pointer to a buffer in which the font descriptions
                    should be placed.
                    
        bufBytes  - size of the supplied buffer.
        
        flags     - flags telling what kind of fonts to load,
                    for example AFF_TAGGED for tagged fonts also,
                    AFF_MEMORY for fonts in memory, AFF_DISK for fonts
                    on disk.

    RESULT
        shortage  - 0 if buffer was big enough or a number telling
                    how much additional place is needed.

    NOTES
        If the routine failes, then the afh_Numentries field
        in the AvailFontsHeader will be 0.

    EXAMPLE

    BUGS

    SEE ALSO
        OpenDiskfont(), <diskfont/diskfont.h>

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    diskfont_lib.fd and clib/diskfont_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG retval = 0;
    APTR iterator;
    struct TTextAttr *attr;
    BOOL tagged = (flags & AFF_TAGGED) ? TRUE : FALSE;
    struct BufferInfo *bi;
    
    D(bug("AvailFonts(buffer=%p, bufbytes=%d,flags=%d)\n", buffer, bufBytes, flags));

    bi = BufferInfoCreate(buffer, bufBytes, tagged, DFB(DiskfontBase));
    
    if (flags & AFF_MEMORY)
    {
	iterator = MF_IteratorInit(DFB(DiskfontBase));
	while((attr = MF_IteratorGetNext(iterator, DFB(DiskfontBase)))!=NULL)
	{
	    if ((!IS_SCALED_FONT(attr) || (flags & AFF_SCALED))
		&& !(IS_OUTLINE_FONT(attr) && (flags & AFF_BITMAP)))
	    {
#warning CHECKME
		     	    
		/* taf_Type only ever seems to contain one of AFF_MEMORY/AFF_DISK/AFF_SCALED,
		 but not a combination of these. */
		UWORD type = IS_SCALED_FONT(attr) ? AFF_SCALED : AFF_MEMORY;

		BufferInfoAdd(bi, type, attr, tagged, DFB(DiskfontBase));
	    }
	}
	MF_IteratorFree(iterator, DFB(DiskfontBase));
    }
	
    if (flags & AFF_DISK)
    {
	iterator = DF_IteratorInit(NULL, DFB(DiskfontBase));
	while((attr = DF_IteratorGetNext(iterator, DFB(DiskfontBase)))!=NULL)
	{
	    if ((!IS_SCALED_FONT(attr) || (flags & AFF_SCALED))
		&& !(IS_OUTLINE_FONT(attr) && (flags & AFF_BITMAP)))
	    {
#warning CHECKME
		/* For disk fonts the type is always AFF_DISK ??? */
		BufferInfoAdd(bi, AFF_DISK, attr, tagged, DFB(DiskfontBase));
	    }
	}
	DF_IteratorFree(iterator, DFB(DiskfontBase));
    }

    retval = bi->space>=0 ? 0 : -bi->space;
    
    BufferInfoFree(bi, DFB(DiskfontBase));
    
    ReturnInt ("AvailFonts", ULONG, retval);

    AROS_LIBFUNC_EXIT
    
} /* AvailFonts */


/*****************************************************************************/

STATIC struct BufferInfo *BufferInfoCreate(STRPTR buffer, LONG bufBytes, BOOL tagged, struct DiskfontBase_intern *DiskfontBase)
{
    struct BufferInfo *retval;
    
    retval = (struct BufferInfo *)AllocMem(sizeof(struct BufferInfo), MEMF_ANY | MEMF_CLEAR);
    if (retval != NULL)
    {
	retval->afh = (struct AvailFontsHeader *)buffer;
	retval->afh->afh_NumEntries = 0;
	retval->space = bufBytes-sizeof(struct AvailFontsHeader);
	if (tagged)
	    retval->u.taf = (struct TAvailFonts *)(retval->afh+1);
	else
	    retval->u.af = (struct AvailFonts *)(retval->afh+1);
	retval->endptr = (UBYTE *)buffer + bufBytes;
    }
    
    return retval;
}


STATIC VOID BufferInfoAdd(struct BufferInfo *bi, UWORD type, struct TTextAttr *tattr, BOOL tagged, struct DiskfontBase_intern *DiskfontBase)
{
    if (tagged && tattr->tta_Tags!=NULL)
	bi->space -= sizeof(struct TAvailFonts) + strlen(tattr->tta_Name)+1 + NumTags(tattr->tta_Tags, DiskfontBase)*sizeof(struct TagItem);
    else
	bi->space -= sizeof(struct AvailFonts) + strlen(tattr->tta_Name)+1;
    
    if (bi->space >= 0)
    {
	bi->endptr -= strlen(tattr->tta_Name)+1;
	strcpy(bi->endptr, tattr->tta_Name);
	
	if (tagged)
	{
	    LONG size;
	    
	    bi->u.taf->taf_Type = type;
	    bi->u.taf->taf_Attr.tta_Name = bi->endptr;
	    bi->u.taf->taf_Attr.tta_YSize = tattr->tta_YSize;
	    bi->u.taf->taf_Attr.tta_Style = tattr->tta_Style;
	    bi->u.taf->taf_Attr.tta_Flags = tattr->tta_Flags;
	    
	    if (tattr->tta_Tags!=NULL)
	    {
	        size = NumTags(tattr->tta_Tags, DiskfontBase)*sizeof(struct TagItem);
	        bi->endptr -= size;
	        memcpy(bi->endptr, tattr->tta_Tags, size);
	        bi->u.taf->taf_Attr.tta_Tags = (struct TagItem *)bi->endptr;
	    }
	    else
	        bi->u.taf->taf_Attr.tta_Tags = NULL;
	   
	    bi->u.taf++;
	}
	else
	{
	    bi->u.af->af_Type = type;
	    bi->u.af->af_Attr.ta_Name = bi->endptr;
	    bi->u.af->af_Attr.ta_YSize = tattr->tta_YSize;
	    bi->u.af->af_Attr.ta_Style = tattr->tta_Style;
	    bi->u.af->af_Attr.ta_Flags = tattr->tta_Flags;
	    
	    bi->u.af++;
	}
	
	bi->afh->afh_NumEntries++;
    }
}


STATIC VOID BufferInfoFree(struct BufferInfo *bi, struct DiskfontBase_intern *DiskfontBase)
{
    FreeMem(bi, sizeof(struct BufferInfo));
}

/*****************************************************************************/
