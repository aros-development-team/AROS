/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Functions for readin .font files
    Lang: English.
*/

/****************************************************************************************/

#include <dos/dos.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <aros/debug.h>

#include "diskfont_intern.h"

/****************************************************************************************/

APTR AllocSegment(APTR prevsegment, ULONG segmentsize, ULONG memflags,
    	    	  struct DiskfontBase_intern *DiskfontBase)
{
    UBYTE *mem;
    
    if ((mem = AllocMem(segmentsize + sizeof(ULONG) + sizeof(BPTR), memflags)))
    {
    	*((ULONG *)mem)++ = segmentsize + sizeof(ULONG) + sizeof(BPTR);
	
	if (prevsegment)
	{
	    ((BPTR *)prevsegment)[-1] = MKBADDR(mem);
	}

	*((BPTR *)mem)++ = NULL;
		
    }
    
    return (APTR)mem;
}

/****************************************************************************************/

struct TagItem *ReadTags(BPTR fh, ULONG numtags, struct DiskfontBase_intern *DiskfontBase)
{
    struct TagItem  *taglist,
                    *tagptr;

    D(bug("ReadTags(fh=%p, numtags=%d)\n", fh, numtags));
	
    /* Allocate memory for the tags */
    
    
    taglist = AllocVec(
    	numtags * sizeof(struct TagItem),
        MEMF_ANY);
       
    if (!taglist)
        goto rt_failure;

    tagptr = taglist;
    
    /* Read the taglist into the buffer */
                
    for (; numtags --; )
    {
    	ULONG val;
	
        if (!ReadLong( &DFB(DiskfontBase)->dsh, &val, (void *)fh))
            goto readfail;
	tagptr->ti_Tag = val;
                                   
        if (!ReadLong( &DFB(DiskfontBase)->dsh, &val, (void *)fh ))
            goto readfail;
    	tagptr->ti_Data = val;          
	           
        tagptr ++;
    }

    ReturnPtr ("ReadTags", struct TagItem*, taglist);

readfail:   
    FreeVec(taglist);
rt_failure:
    ReturnPtr("ReadTags", struct TagItem*, FALSE);
    
}

/****************************************************************************************/

BOOL WriteTags(BPTR fh, struct TagItem *taglist, struct DiskfontBase_intern *DiskfontBase)
{

    struct TagItem *tag;

    D(bug("WriteTags(fh=%p, taglists=%p)\n", fh, taglist));

#ifdef AROSAMIGA
    for (; (tag = NextTagItem((struct TagItem **)&taglist)); )
#else
    for (; (tag = NextTagItem((const struct TagItem **)&taglist)); )
#endif
    {
        if (!WriteLong( &DFB(DiskfontBase)->dsh, tag->ti_Tag, (void *)fh ))
            goto wt_failure;
            
        if (!WriteLong( &DFB(DiskfontBase)->dsh, tag->ti_Data, (void *)fh))
            goto wt_failure;
    }
    WriteLong(&DFB(DiskfontBase)->dsh, TAG_DONE, (void *)fh);
    WriteLong(&DFB(DiskfontBase)->dsh, 0, (void *)fh);
    
    ReturnBool ("WriteTags", TRUE);
    
wt_failure:
    ReturnBool ("WriteTags", FALSE);
}

/****************************************************************************************/
