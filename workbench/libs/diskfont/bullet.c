/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Functions for readin .font files
    Lang: English.
*/

/****************************************************************************************/

#include <dos/dos.h>
#include <diskfont/diskfonttag.h>
#include <aros/macros.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "diskfont_intern.h"

/****************************************************************************************/

STRPTR OTAG_MakeFileName(STRPTR filename, struct DiskfontBase_intern *DiskfontBase)
{
    STRPTR  retval;
    LONG    l;
    
    l = strlen(filename) + 1;
    if (l < 7) return NULL;
    
    retval = AllocVec(l, MEMF_ANY);
    if (retval)
    {
    	memcpy(retval, filename, l - 5);
	strcpy(retval + l - 5, "otag");   	
    }
    
    return retval;
}

/****************************************************************************************/

VOID OTAG_FreeFileName(STRPTR filename, struct DiskfontBase_intern *DiskfontBase)
{
    if (filename) FreeVec(filename);
}

/****************************************************************************************/

#ifdef _AROS
#warning This needs to be reworked for systems where sizeof(TagItem) is > 8!
#endif

/****************************************************************************************/

struct OTagList *OTAG_GetFile(STRPTR filename, struct DiskfontBase_intern *DiskfontBase)
{
    struct FileInfoBlock    *fib;
    struct OTagList 	    *otaglist;
    struct TagItem  	    *ti;
    STRPTR  	    	     otagfilename;
    BPTR    	    	     otagfile;
    LONG    	    	     l;
    BOOL    	    	     ok;
    
    otagfilename = OTAG_MakeFileName(filename, DiskfontBase);
    if (!otagfilename) return NULL;
    
    otagfile = Open(otagfilename, MODE_OLDFILE);
    if (!otagfile)
    {
    	OTAG_FreeFileName(otagfilename, DiskfontBase);
    	return NULL;
    }
    
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        OTAG_FreeFileName(otagfilename, DiskfontBase);
    	Close(otagfile);
	return NULL;
    }
    
    ok = ExamineFH(otagfile, fib);
    l = fib->fib_Size;
    FreeDosObject(DOS_FIB, fib);
    
    if (!ok)
    {
    	OTAG_FreeFileName(otagfilename, DiskfontBase);
    	Close(otagfile);
	return NULL;
    }

    otaglist = AllocVec(sizeof(struct OTagList) + l, MEMF_PUBLIC | MEMF_CLEAR);
    if (!otaglist)
    {
    	OTAG_FreeFileName(otagfilename, DiskfontBase);
    	Close(otagfile);
	return NULL;
    }
    
    ok = (Read(otagfile, otaglist->tags, l) == l);
    Close(otagfile);
    
    if (AROS_LONG2BE(otaglist->tags[0].ti_Tag)  != OT_FileIdent) ok = FALSE;
    if (AROS_LONG2BE(otaglist->tags[0].ti_Data) != l) ok = FALSE;
    
    if (!ok)
    {
    	OTAG_FreeFileName(otagfilename, DiskfontBase);
    	FreeVec(otaglist);
	return NULL;
    }
    
    ti = otaglist->tags;

    do
    {
    	ti->ti_Tag = AROS_LONG2BE(ti->ti_Tag);
	ti->ti_Data = AROS_LONG2BE(ti->ti_Data);
	
	if (ti->ti_Tag & OT_Indirect)
	{
	    ti->ti_Data = (IPTR)otaglist->tags + ti->ti_Data;
	}
		
    } while ((ti++)->ti_Tag != TAG_DONE);
    
    otaglist->filename = otagfilename;
    
    return otaglist;
    
}

/****************************************************************************************/

VOID OTAG_KillFile(struct OTagList *otaglist, struct DiskfontBase_intern *DiskfontBase)
{
    if (otaglist)
    {
    	if (otaglist->filename) OTAG_FreeFileName(otaglist->filename, DiskfontBase);
    	FreeVec(otaglist);
    }
}

/****************************************************************************************/

UBYTE OTAG_GetFontStyle(struct OTagList *otaglist, struct DiskfontBase_intern *DiskfontBase)
{
    UBYTE style = 0;

    /* A font gets FSF_BOLD if OT_StemWeight >= 0x90 */
    
    if (GetTagData(OT_StemWeight, 0, otaglist->tags) >= 0x90)
    {
    	style |= FSF_BOLD;
    }

    /* A font gets FSF_ITALIC if OT_SlantStyle != OTS_Upright */
    
    if (GetTagData(OT_SlantStyle, OTS_Upright, otaglist->tags) != OTS_Upright)
    {
    	style |= FSF_ITALIC;
    }

    /* A font gets FSF_EXTENDED if OT_HorizStyle >= 0xA0 */
    
    if (GetTagData(OT_HorizStyle, 0, otaglist->tags) >= 0xA0)
    {
    	style |= FSF_EXTENDED;
    }
    
    return style;
}

/****************************************************************************************/

UBYTE OTAG_GetFontFlags(struct OTagList *otaglist, struct DiskfontBase_intern *DiskfontBase)
{
    UBYTE flags;
    
    flags = FONTTYPE_OUTLINEFONT;
    if (GetTagData(OT_IsFixed, FALSE, otaglist->tags) == FALSE)
    {
    	flags |= FPF_PROPORTIONAL;
    }
    
    return flags;
}

/****************************************************************************************/
