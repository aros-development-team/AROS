/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Some support functions
*/

/****************************************************************************************/

#include <dos/dos.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <aros/debug.h>

#include "diskfont_intern.h"

/****************************************************************************************/

APTR AllocSegment
(
    BPTR *prevsegment, ULONG segmentsize, ULONG memflags,
    struct DiskfontBase_intern *DiskfontBase
)
{
    ULONG *mem;
    
    if ((mem = AllocMem(segmentsize + sizeof(ULONG) + sizeof(BPTR), memflags)))
    {
        BPTR *membptr;

    	*mem++ = segmentsize + sizeof(ULONG) + sizeof(BPTR);
        if (prevsegment) prevsegment[-1] = MKBADDR(mem);
        membptr = (BPTR *) mem;
        *membptr++ = NULL;
    	mem = (ULONG *)membptr;
    }
    
    return mem;
}

/****************************************************************************************/

/************/
/* ReadTags */
/************/

/****************************************************************************************/

struct TagItem *ReadTags(BPTR fh, ULONG numtags, struct DiskfontBase_intern *DiskfontBase)
{
    struct TagItem  *taglist,
                    *tagptr;
    
    D(bug("ReadTags(fh=%p, numtags=%u)\n", fh, numtags));
	
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
	
        if (!ReadLong( &DiskfontBase->dsh, &val, (void *)fh))
            goto readfail;
	tagptr->ti_Tag = val;
                                   
        if (!ReadLong( &DiskfontBase->dsh, &val, (void *)fh ))
            goto readfail;
    	tagptr->ti_Data = val;          
	           
        tagptr ++;
    }

    ReturnPtr ("ReadTags", struct TagItem*, taglist);

readfail:   
    FreeVec(taglist);
rt_failure:
    
    ReturnPtr("ReadTags", struct TagItem*, NULL);
    
}

/****************************************************************************************/

/***************/
/* ReadTagsNum */
/***************/

/****************************************************************************************/

struct TagItem *ReadTagsNum(BPTR fh, ULONG *numtagsptr, struct DiskfontBase_intern *DiskfontBase)
{
    struct TagItem  *taglist;
    ULONG            numtags;
    
    D(bug("ReadTagsNum(fh=%p, numtagptr=0x%lx)\n", fh, numtagsptr));
	
    /* Allocate memory for the tags */
    
    if (!ReadLong(&DiskfontBase->dsh, &numtags, (void *)fh))
	goto rt_failure;
	
    taglist = ReadTags(fh, numtags, DiskfontBase);
    if (taglist == NULL)
	goto rt_failure;
    
    if (numtagsptr != NULL)
	*numtagsptr = numtags;
    
    ReturnPtr ("ReadTagsNum", struct TagItem*, taglist);

rt_failure:
    if (numtagsptr != NULL)
	*numtagsptr = 0;
    
    ReturnPtr("ReadTagsNum", struct TagItem*, FALSE);
    
}

/****************************************************************************************/

/****************/
/* WriteTagsNum */
/****************/

/****************************************************************************************/

BOOL WriteTagsNum(BPTR fh, const struct TagItem *taglist, struct DiskfontBase_intern *DiskfontBase)
{
    struct TagItem *tag;
    ULONG num;
    
    D(bug("WriteTags(fh=%p, taglists=%p)\n", fh, taglist));

    num = NumTags(taglist, DiskfontBase);

    if (!WriteLong(&DiskfontBase->dsh, num, (void *)fh))
	goto wt_failure;
    
    for (; (tag = NextTagItem(&taglist)); )
    {
        if (!WriteLong( &DiskfontBase->dsh, tag->ti_Tag, (void *)fh ))
            goto wt_failure;
            
        if (!WriteLong( &DiskfontBase->dsh, tag->ti_Data, (void *)fh))
            goto wt_failure;
    }
    WriteLong(&DiskfontBase->dsh, TAG_DONE, (void *)fh);
    WriteLong(&DiskfontBase->dsh, 0, (void *)fh);
    
    ReturnBool ("WriteTags", TRUE);
    
wt_failure:
    ReturnBool ("WriteTags", FALSE);
}

/****************************************************************************************/

/**************/
/* NumTags    */
/**************/

/****************************************************************************************/

ULONG NumTags(const struct TagItem *taglist, struct DiskfontBase_intern *DiskfontBase)
/* Counts the number of tags in at taglist including TAG_DONE */

{
    ULONG numtags = 0;

    D(bug("NumTags(taglist=%p)\n", taglist));

    for (; NextTagItem(&taglist); )
        numtags ++;

    numtags ++; /* Count TAG_DONE */
  
    ReturnInt ("NumTags", ULONG, numtags);
}

/****************************************************************************************/

/****************/
/* CopyTagItems */
/****************/

/****************************************************************************************/

ULONG CopyTagItems
(
    struct TagItem *desttaglist, 
    const struct TagItem *sourcetaglist, 
    struct DiskfontBase_intern *DiskfontBase
)
/* Copies tags from a taglist to another memory location, returning
  the number of tags copied */

{
    ULONG numtags=0;
  
    struct TagItem *tag;

    D(bug("CopyTagItems(desttaglist=%p, sourcetaglist=%p)\n", desttaglist, sourcetaglist));

    for (; (tag = NextTagItem(&sourcetaglist)); )
    {
        desttaglist->ti_Tag   = tag->ti_Tag;
        desttaglist->ti_Data  = tag->ti_Data;
    
        desttaglist++;
        numtags++;
    }
    
    /* Insert TAG_DONE */
    desttaglist->ti_Tag   = TAG_DONE;
    desttaglist->ti_Data  = 0L;

    /* Count TAG_DONE */
    numtags++;
  
    ReturnInt ("CopyTagItems", ULONG, numtags);
}

/****************************************************************************************/
