/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/****************************************************************************************/

#include <diskfont/diskfont.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include <aros/debug.h>
#include <aros/macros.h>
#include "diskfont_intern.h"

/****************************************************************************************/

/* Test */
#define  GetByte(x)  ((x)[0])
#define  GetWord(x)  ((x)[1] | ((x)[0] << 8))
#define  GetPtr(x)   (((x)[0] << 24) | ((x)[1] << 16) | ((x)[2] << 8) | ((x)[3]))

struct AmiTagItem
{
    ULONG ti_Tag;
    ULONG ti_Data;
};

/****************************************************************************************/

VOID  CopyContents(struct List *list, APTR mem);
VOID  FreeBuffers(struct List *list);
struct AmiTagItem *AmiNextTagItem(struct AmiTagItem **ti);

/****************************************************************************************/

struct contentsBuffer
{
    struct Node         node;
    struct FontContents fc;
};

/****************************************************************************************/

#define TFC(node) ((struct TFontContents *)(&node->fc))

/*****************************************************************************

    NAME */
#include <proto/diskfont.h>

	AROS_LH2(struct FontContentsHeader *, NewFontContents,

/*  SYNOPSIS */
	AROS_LHA(BPTR  , fontsLock, A0),
	AROS_LHA(STRPTR, fontName, A1),

/*  LOCATION */
	struct Library *, DiskfontBase, 7, Diskfont)

/*  FUNCTION

    Create an array of FontContents entries describing the fonts related
    with 'fontName' -- this is those in the directory with the same name
    as 'fontName' without the ".font" suffix.

    INPUTS

    fontsLock  --  A lock on the FONTS: directory or another directory
                   containing the font file and associated directory
		   exists.
    fontName   --  The font name (with the ".font" suffix).

    RESULT

    Pointer to a struct FontContentsHeader describing the font or NULL
    if something went wrong.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    DisposeFontContents()

    INTERNALS

    The whole issue of fonts being Amiga executable files is a big mess.
    We'd better do something about it (define a new format?).
        Some code here should use a function similar to the one in
    ReadDiskFont() -- however, we do not want a struct TextFont *.

    HISTORY

    5.8.1999  SDuvan  partial implementation

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR   oldDir;
    BPTR   lock, otLock = NULL;
    STRPTR suffix;
    char   name[MAXFONTNAME];
    struct List contentsList;

    struct FileInfoBlock      *fib;
    struct FontContentsHeader *ret = NULL;

    (void) DiskfontBase;

    NEWLIST(&contentsList);
    oldDir = CurrentDir(fontsLock);

    strcpy((char *)&name, fontName);
    suffix = strrchr(name, '.');

    if(suffix == NULL || strcmp(suffix, ".font") != 0)
	return NULL;

    strcpy(suffix, ".otag");

    /* otLock will be an indicator of whether there exists a .otag file */
    otLock = Lock(name, SHARED_LOCK);
    UnLock(otLock);

    /* Get the correct directory name */
    name[strlen(name) - sizeof(".otag") + 1] = 0;

    lock = Lock(name, SHARED_LOCK); /* Lock font directory */

    fib = AllocDosObject(DOS_FIB, NULL);

    if(fib == NULL)
    {
	UnLock(lock);
	CurrentDir(oldDir);
	return NULL;
    }		
    
    CurrentDir(lock);

    if(Examine(lock, fib) == DOSTRUE)
    {
	struct FontContentsHeader fch = { FCH_ID , 0 };

	/* Loop through the files in this font's directory */ 
	while(ExNext(lock, fib))
	{
	    BPTR  fontSeg;
	    UBYTE                 *fileDfh;  /* struct DiskFontHeader */ 
	    struct contentsBuffer *cNode;
	    
	    /* Skip directories */
	    if(fib->fib_DirEntryType >= 0)
		continue;

	    fontSeg = LoadSeg(fib->fib_FileName);

	    if(fontSeg == NULL)
		continue;
	    
	    /* Skip NextSegment and ReturnCode */
	    fileDfh = ((UBYTE *)BADDR(fontSeg)) + 8;

	    if(GetWord(fileDfh + 14) != DFH_ID) /* dfh_FileID */
	    {
		UnLoadSeg(fontSeg);
		continue;
	    }
	    
	    cNode = AllocVec(sizeof(struct contentsBuffer),
			     MEMF_PUBLIC | MEMF_CLEAR);
	    
	    if(cNode == NULL)
	    {
		FreeBuffers((struct List *)&contentsList);
		UnLoadSeg(fontSeg);
		UnLock(lock);
		FreeDosObject(DOS_FIB, fib);
		CurrentDir(oldDir);
		return NULL;
	    }
	    
	    AddTail((struct List *)&contentsList, (struct Node *)cNode);
	    
	    strcpy(cNode->fc.fc_FileName, name);
	    strcat(cNode->fc.fc_FileName, "/");
	    strcat(cNode->fc.fc_FileName, fib->fib_FileName);

	    /* TODO: Factor out the code in diskfont_io.c that extracts
	             tags and make it a function... */

	    /* Embedded tags? */
	    if(GetByte(fileDfh+14+2+2+4+32+20+2) & FSF_TAGGED) /* dfh_TF.tf_Style */
	    {
		struct AmiTagItem *ti = (struct AmiTagItem *)(*(ULONG *)(fileDfh+14+2+2)); /* dfh_TagList */
		struct AmiTagItem *tPtr;
	 	struct AmiTagItem *item;
		WORD   nTags = 0;
		WORD   i;	        /* Loop variable */
		
		/* We cannot use the standard function here as the
		   TagItems saved on disk may not correspond very
		   well to those on this particular architecture. */
		while(AmiNextTagItem(&ti) != NULL)
		    nTags++;
		nTags++;	/* Include TAG_DONE */
		
		TFC(cNode)->tfc_TagCount = nTags;
		fch.fch_FileID = TFCH_ID;
		
		tPtr = (struct AmiTagItem *)((IPTR)&(TFC(cNode)->tfc_TagCount) + 2 - nTags*sizeof(struct AmiTagItem));
		
		ti = (struct AmiTagItem *)(*(ULONG *)(fileDfh+14+2+2)); /* dfh_TagList */
		
		i = 0;
		while((item = AmiNextTagItem(&ti)) != NULL)
		{
		    tPtr[i].ti_Tag  = AROS_LONG2BE(item->ti_Tag);
		    tPtr[i].ti_Data = AROS_LONG2BE(item->ti_Data);
		    i++;
		}
		/* Add TAG_DONE tag, but no data (to avoid writing over the
		   TagCount) */
		tPtr[i].ti_Tag = TAG_DONE;


	    } /* if(this was a tagged font) */
	    
	    cNode->fc.fc_YSize = GetWord(fileDfh+14+2+2+4+32+20);  /* dfh_TF.tf_YSize */
	    cNode->fc.fc_Style = GetByte(fileDfh+14+2+2+4+32+20+2); /* dfh_TF.tf_Style */
	    cNode->fc.fc_Flags = GetByte(fileDfh+14+2+2+4+32+20+2+1); /* dfh_TF.tf_Flags */

    	    cNode->fc.fc_Flags &= ~FPF_REMOVED;
	    cNode->fc.fc_Flags &= ~FPF_ROMFONT;
	    cNode->fc.fc_Flags |=  FPF_DISKFONT;
	    
	    fch.fch_NumEntries++;
	    
	    UnLoadSeg(fontSeg);
	    
	} /* while(there are files left in the directory) */
	
	if(IoErr() == ERROR_NO_MORE_ENTRIES)
	{
	    ret = (struct FontContentsHeader *)AllocVec(sizeof(struct FontContentsHeader) + fch.fch_NumEntries*sizeof(struct TFontContents), MEMF_PUBLIC | MEMF_CLEAR);
	    
	    if(ret != NULL)
	    {
		ret->fch_NumEntries = fch.fch_NumEntries;
		ret->fch_FileID = otLock == NULL ? fch.fch_FileID : OFCH_ID;
		
		CopyContents((struct List *)&contentsList, 
			     ((UBYTE *)ret + sizeof(struct FontContentsHeader)));
	    }
	    
	}
	
	FreeBuffers(&contentsList);
	
    } /* if(we could examine the font's directory) */
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    CurrentDir(oldDir);
    
    return ret;
    
    AROS_LIBFUNC_EXIT
    
} /* NewFontContents */

/****************************************************************************************/

VOID FreeBuffers(struct List *list)
{
    struct Node *node, *temp;

    ForeachNodeSafe(list, node, temp)
    {
	Remove(node);
	FreeVec(node);
    }
}

/****************************************************************************************/

VOID CopyContents(struct List *list, APTR mem)
{
    struct contentsBuffer *temp;

    ForeachNode(list, (struct Node *)temp)
    {
	CopyMem(&temp->fc, mem, sizeof(struct FontContents));
	mem += sizeof(struct FontContents);
    }
}

/****************************************************************************************/

struct AmiTagItem *AmiNextTagItem(struct AmiTagItem **ti)
{
    if((*ti)->ti_Tag == TAG_DONE)
	return NULL;

    return (*ti)++;
}

/****************************************************************************************/
