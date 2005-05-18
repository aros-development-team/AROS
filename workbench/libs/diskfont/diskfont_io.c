/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Functions for reading disk font files.
*/

/****************************************************************************************/

#include <stdio.h>
#include <exec/execbase.h>
#include <exec/initializers.h>
#include <dos/doshunks.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include <string.h>

#include <proto/dos.h>
#include <proto/graphics.h>

#include "diskfont_intern.h"

/****************************************************************************************/

#define DEBUG 1
#include <aros/debug.h>

/****************************************************************************************/

#define SKIPLONG(ptr) 	ptr += sizeof (LONG);

#define SKIPWORD(ptr)	ptr += sizeof (WORD)

#define SKIPBYTE(ptr)	ptr ++;

#define SKIPPTR(ptr) 	ptr += sizeof(APTR)

#define CONVLONG(ptr, destlong)  \
destlong = ptr[0] << 24 |ptr[1] << 16 | ptr[2] << 8 | ptr[3]; \
SKIPLONG(ptr);

#define CONVWORD(ptr, destword)		\
destword = ptr[0] <<  8 | ptr[1];	\
SKIPWORD(ptr);
														

#define CONVBYTE(ptr, destbyte)	\
destbyte = ptr[0]; 							\
SKIPBYTE(ptr);

/*
We don't need endian conversion of pointers since this is done inside LoadSeg_AOS()
#define CONVPTR(ptr, destptr) \
((APTR)destptr) = (APTR)(ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3]); \
SKIPPTR(ptr);
*/

#if 0
#define COPYPTR(ptr, destptr) \
((APTR)destptr) = (APTR)(ptr[0] | ptr[1] << 8 | ptr [2] << 16 | ptr[3] << 24); \
SKIPPTR(ptr);
#else

#define COPYPTR(ptr,destptr) \
(destptr) = *((APTR *)(ptr)); \
SKIPPTR(ptr);
#endif
/****************************************************************************************/

/****************/
/* ConvDiskFont */
/****************/

/****************************************************************************************/

struct DiskFontHeader *ConvDiskFont(BPTR seglist, CONST_STRPTR fontname,
    	    	    	      struct DiskfontBase_intern *DiskfontBase)
{
    UWORD count, numchars;
    register int i;

    UBYTE *ptr;

    UWORD *destptr;
    ULONG *destlptr;
    ULONG chardatasize;
    struct DiskFontHeader tmp_dfh, *dfh = 0;
    struct TextFont *tf = 0;

    APTR ctf_chardata_ptrs[8] = {0};
    struct ColorFontColors *cfc_ptr = 0;
    UWORD *colortable_ptr = NULL;

    APTR    chardata_ptr 	= NULL,
	    charloc_ptr  	= NULL,
	    charspace_ptr 	= NULL,
	    charkern_ptr 	= NULL,
	    taglist_ptr 	= NULL,
	    prevsegment     	= NULL;
    BPTR    fontsegment = NULL;
    BOOL    fontextended = FALSE;
    
    CONST_STRPTR filepart;

    EnterFunc(bug("ConvDiskFont(seglist=%p, fontname=%s)\n", seglist, fontname));		

    /* Clear temporary diskfontheader struct */
    memset(&tmp_dfh, 0, sizeof (struct DiskFontHeader));

    /* Get start of diskfontheader. (Go past the dummy exe header) */ 
    ptr = UB(BADDR(seglist)) + sizeof (ULONG) * 2;

    /* Skip the whole DiskFontHeader */
    SKIPPTR(ptr);	    	    	    /* dfh_TF.ln_Succ	*/
    SKIPPTR(ptr);	    	    	    /* dfh_TF.ln_Pred	*/
    CONVBYTE(ptr, tmp_dfh.dfh_DF.ln_Type);  /* dfh_TF.ln_Type	*/	
    CONVBYTE(ptr, tmp_dfh.dfh_DF.ln_Pri);   /* dfh_TF.ln_Pri	*/
    SKIPPTR(ptr);	    	    	    /* dfh_TF.ln_Name	*/
    CONVWORD(ptr, tmp_dfh.dfh_FileID);	    /* dfh_FileID   	*/

    if (tmp_dfh.dfh_FileID != DFH_ID)
	goto failure;

    CONVWORD(ptr, tmp_dfh.dfh_Revision);    /* dfh_Revision */
    
    COPYPTR(ptr, taglist_ptr);	    	    /* dfh_Segment */
    ptr += MAXFONTNAME;

    /* dfh_TF starts */
    
    /* Skip nodes successor and predecessor field */
    SKIPPTR(ptr);
    SKIPPTR(ptr);

    /* skip type and pri */
    SKIPBYTE(ptr);
    SKIPBYTE(ptr);
    tmp_dfh.dfh_TF.tf_Message.mn_Node.ln_Type = NT_FONT;

    /* Skip name pointer, replyport and msg length */
    SKIPPTR(ptr);
    SKIPPTR(ptr);
    SKIPWORD(ptr);

    CONVWORD(ptr, tmp_dfh.dfh_TF.tf_YSize);
    CONVBYTE(ptr, tmp_dfh.dfh_TF.tf_Style);
    CONVBYTE(ptr, tmp_dfh.dfh_TF.tf_Flags);
    CONVWORD(ptr, tmp_dfh.dfh_TF.tf_XSize);
    CONVWORD(ptr, tmp_dfh.dfh_TF.tf_Baseline);
    CONVWORD(ptr, tmp_dfh.dfh_TF.tf_BoldSmear);
    SKIPWORD(ptr); /* tf_Accessors */
    CONVBYTE(ptr, tmp_dfh.dfh_TF.tf_LoChar);
    CONVBYTE(ptr, tmp_dfh.dfh_TF.tf_HiChar);
    COPYPTR(ptr, chardata_ptr); /* tf_CharData */
    CONVWORD(ptr, tmp_dfh.dfh_TF.tf_Modulo);
    COPYPTR(ptr, charloc_ptr); /* tf_CharLoc		*/
    COPYPTR(ptr, charspace_ptr); /* tf_CharSpace	*/
    COPYPTR(ptr, charkern_ptr); /* tf_CharKern		*/
 
    D(bug("Textfont struct converted\n"));
    D(bug("YSize:     %d\n",	tmp_dfh.dfh_TF.tf_YSize));
    D(bug("Style:     %d\n",	tmp_dfh.dfh_TF.tf_Style));
    D(bug("Flags:     %d\n",	tmp_dfh.dfh_TF.tf_Flags));
    D(bug("XSize:     %d\n",	tmp_dfh.dfh_TF.tf_XSize));
    D(bug("Baseline:  %d\n",	tmp_dfh.dfh_TF.tf_Baseline));
    D(bug("Boldsmear: %d\n",	tmp_dfh.dfh_TF.tf_BoldSmear));
    D(bug("LoChar:    %d\n",	tmp_dfh.dfh_TF.tf_LoChar));
    D(bug("HiChar:    %d\n",	tmp_dfh.dfh_TF.tf_HiChar));
    D(bug("chardara:  %p\n", 	chardata_ptr));
    D(bug("Modulo:    %d\n", 	tmp_dfh.dfh_TF.tf_Modulo));
    D(bug("charloc:   %p\n", 	charloc_ptr));
    D(bug("charspace: %p\n", 	charspace_ptr));
    D(bug("charkern:  %p\n", 	charkern_ptr));

    /* Allocate memory for font */

    i = sizeof(struct DiskFontHeader);
    if (tmp_dfh.dfh_TF.tf_Style & FSF_COLORFONT)
    {
    	/* +8 should this calc. not work 100 % because of alignments */  	
    	i += (sizeof(struct ColorTextFont) - sizeof(struct TextFont) + 8);
    }
    
    dfh = prevsegment = AllocSegment(prevsegment, i, MEMF_ANY | MEMF_CLEAR, DiskfontBase);
    if (!dfh) goto failure;

    fontsegment = (BPTR)MAKE_REAL_SEGMENT(dfh);
    tmp_dfh.dfh_Segment = fontsegment;
   
    tf = &dfh->dfh_TF;
    
    D(bug("charkern in temp:  %p\n", 	tmp_dfh.dfh_TF.tf_CharKern));

    /*	Copy already converted stuff into allocated mem */
    CopyMem(&tmp_dfh, dfh, sizeof (struct DiskFontHeader));

    D(bug("tmp_tf copied, charkern=%p\n", tf->tf_CharKern));

    /* Calculate size of one character data bitmap */	
    chardatasize = tf->tf_YSize * tf->tf_Modulo;

    numchars = (tf->tf_HiChar - tf->tf_LoChar) + 2; /* + 2 because of default character (255) */

    if (tf->tf_Style & FSF_COLORFONT)
    {
	APTR temp_ptr;

	#undef CTF
	#define CTF(tf) ((struct ColorTextFont *)tf)

    	D(bug("Colorfont found\n"));

	/* Convert extended colortextfont info */
	CONVWORD(ptr, CTF(tf)->ctf_Flags);
	CONVBYTE(ptr, CTF(tf)->ctf_Depth);
	CONVBYTE(ptr, CTF(tf)->ctf_FgColor);
	CONVBYTE(ptr, CTF(tf)->ctf_Low);
	CONVBYTE(ptr, CTF(tf)->ctf_High);
	CONVBYTE(ptr, CTF(tf)->ctf_PlanePick);
	CONVBYTE(ptr, CTF(tf)->ctf_PlaneOnOff);
	COPYPTR(ptr,  cfc_ptr);

	for (i = 0; i < 8; i ++ )
	{
	    COPYPTR(ptr,  ctf_chardata_ptrs[i]);
	}

	/* ------------------------------- */
	/* Handle ColorFontColors structure */
	#undef CFC
	#define CFC(p) ((struct ColorFontColors*)p) 
	
	CTF(tf)->ctf_ColorFontColors = prevsegment = AllocSegment(prevsegment,
	    	    	    	    	    	    	    	  sizeof(struct ColorFontColors),
								  MEMF_ANY | MEMF_CLEAR,
								  DiskfontBase);
	
	if (!CTF(tf)->ctf_ColorFontColors) goto failure;

	temp_ptr = CTF(tf)->ctf_ColorFontColors;

	ptr = (UBYTE *)cfc_ptr;
	SKIPWORD(ptr);
	CONVWORD(ptr, CFC(temp_ptr)->cfc_Count);
	COPYPTR (ptr, colortable_ptr);

	/* ------------------------------- */
	/* Handle colortable */
	count = CFC(temp_ptr)->cfc_Count;
	ptr = (UBYTE *)colortable_ptr;

	CFC(temp_ptr)->cfc_ColorTable = prevsegment = AllocSegment(prevsegment,
	    	    	    	    	    	    	    	   count * sizeof (UWORD),
								   MEMF_ANY,
								   DiskfontBase);
	
	if (!CFC(temp_ptr)->cfc_ColorTable) goto failure;

	for (i = 0; i < count; i ++)
	{
	    CONVWORD(ptr, CFC(temp_ptr)->cfc_ColorTable[i]);
	}

	/* ------------------------------- */
	/* Handle character bitmap data for colorfonts */
	for (i = 0; i < 8; i ++)
	{
	    if (!ctf_chardata_ptrs[i]) continue;

	    CTF(tf)->ctf_CharData[i] = prevsegment = AllocSegment(prevsegment,
	    	    	    	    	    	    	    	  chardatasize,
								  MEMF_ANY,
								  DiskfontBase);
		
	    if (!CTF(tf)->ctf_CharData[i]) goto failure;

	    CopyMem(ctf_chardata_ptrs[i], CTF(tf)->ctf_CharData[i], chardatasize); 

	}

    }
    
    if (chardata_ptr) /* CHECKME: check necessary? Do also colorfonts always have this? */
    {
    	D(bug("B&W font\t Chardatasize: %d\n", chardatasize));

	/* Character data for B/W fonts */
	tf->tf_CharData = prevsegment = AllocSegment(prevsegment,
	    	    	    	    	    	     chardatasize,
						     MEMF_ANY,
						     DiskfontBase);
	if (!tf->tf_CharData) goto failure;
    	
	
	D(bug("chardataptr=%p\n",chardata_ptr));
	CopyMem(chardata_ptr, tf->tf_CharData, chardatasize);
    	D(bug("Chardata copied\n"));
    }		

    /* ----------------------- */
    /* Add fontname */
    filepart = FilePart(fontname);
    i = strlen(filepart) + 1;
    if (i >= sizeof(dfh->dfh_Name)) i = sizeof(dfh->dfh_Name) - 1;
    
    CopyMem((STRPTR) filepart, dfh->dfh_Name, i);
    
    tf->tf_Message.mn_Node.ln_Name = dfh->dfh_Name;
    dfh->dfh_DF.ln_Name = dfh->dfh_Name;
    
    /* ----------------------- */
    /* Allocate memory for charloc */

    D(bug("Doing charloc\n"));
    
    tf->tf_CharLoc = prevsegment = AllocSegment(prevsegment,
    	    	    	    	    	    	numchars * sizeof (ULONG),
						MEMF_ANY,
						DiskfontBase);
	
    if (!tf->tf_CharLoc) goto failure;

    /* Convert charloc data */
    ptr = charloc_ptr;
    destlptr = (ULONG *) tf->tf_CharLoc;
    for (i = 0; i < numchars; i ++ )
    {
        CONVLONG(ptr,  *destlptr ++);
        D(bug("charloc[%d]: %x\n", i, destlptr[-1]));
    }
    D(bug("Charloc OK\n"));	

    if (charspace_ptr)
    {

    	D(bug("Proportional font\n"));
	
	tf->tf_CharSpace = prevsegment = AllocSegment(prevsegment,
	    	    	    	    	    	      numchars * sizeof (UWORD) ,
						      MEMF_ANY,
						      DiskfontBase);
	
	if (!tf->tf_CharSpace) goto failure;

	/* Convert charspace data */
	ptr = charspace_ptr;
	destptr = tf->tf_CharSpace;
	for (i = numchars; i --;)
	{
	    CONVWORD(ptr,  *destptr ++ );
	}

    	D(bug("Charspace OK\n"));
    }

    /* ----------------------- */
    /* Allocate memory for charkern */

    D(bug("Doing Charkern, ptr =%p\n", charkern_ptr));	
    if (charkern_ptr)
    {
	tf->tf_CharKern = prevsegment = AllocSegment(prevsegment,
	    	    	    	    	    	     numchars * sizeof (UWORD),
						     MEMF_ANY,
						     DiskfontBase);
	if (!tf->tf_CharKern) goto failure;

	/* Convert charkern data */
	ptr = charkern_ptr;
	destptr = tf->tf_CharKern;
	for (i = numchars; i --;)
	{
	    CONVWORD(ptr,  *destptr ++);
	    D(bug("Setting to %d\n", destptr[-1]));
	}
    	D(bug("Charkern OK\n"));	
    }

    D(bug("Charkern, ptr =%p\n", tf->tf_CharKern));	


    /* ----------------------- */
    /* Handle taglist */
    
    if (tf->tf_Style & FSF_TAGGED)
    {
	UWORD numtags = 0;
	ULONG tag;
	struct TagItem *taglist;
	
    	D(bug("Tagged font\n"));

	/* Convert the tags */
	ptr = taglist_ptr;

	/* We assume that tags are placed in one single array, and not
	spread around the whole file with TAG_NEXT
	*/

	/* Count number of tags w/TAG_DONE */
	do
	{
	    CONVLONG(ptr, tag)
	    SKIPLONG(ptr);
	    numtags ++;
	}
	while (tag != TAG_DONE);

	/* Allocate memory for taglist */
	prevsegment = taglist = (struct TagItem *)AllocSegment(prevsegment,
	    	    	    	    	    	    	       numtags * sizeof(struct TagItem),
							       MEMF_ANY,
							       DiskfontBase);
	if (!taglist)
	    goto failure;

	/* Copy tags into allocated mem */
	ptr = taglist_ptr;
	for (i = 0; i < numtags; i ++ )
	{
	    CONVLONG(ptr, taglist[i].ti_Tag);
	    CONVLONG(ptr, taglist[i].ti_Data);
	}

	if (ExtendFont(tf, taglist))
	{
	    fontextended = TRUE;
	}
	else
	{
	    goto failure;
	}
    }
    else
    {
    	D(bug("No tags, extending it\n"));

	if (ExtendFont(tf, NULL))
	{
	    fontextended = TRUE;
	}
	else
	{
	    goto failure;
	}
    }
    
    /* ----------------------- */

    ReturnPtr("ConvTextFont", struct DiskFontHeader *, dfh);

failure:

    D(bug("failure\n"));

    if (fontsegment)
    {
    	if (fontextended) StripFont(tf);

    	UnLoadSeg(fontsegment);
    }

    ReturnPtr("ConvTextFont", struct DiskFontHeader *, 0);		
}

void DisposeConvDiskFont(struct DiskFontHeader *dfh,
			 struct DiskfontBase_intern *DiskfontBase)
{
    if (dfh!=NULL)
    {
	StripFont(&dfh->dfh_TF);
	UnLoadSeg(MKBADDR(((BPTR *)dfh)-1));
    }
}

/****************************************************************************************/

/****************/
/* ReadDiskFont	*/
/****************/

/****************************************************************************************/

struct TextFont *ReadDiskFont(
	struct TTextAttr *reqattr,
	CONST_STRPTR realfontname,
	struct DiskfontBase_intern *DiskfontBase)
{	
    struct DiskFontHeader *dfh = NULL;
    STRPTR  	     filename;
    BPTR    	     seglist;
    
    EnterFunc(bug("ReadDiskFont(reqattr=%p, name=%s, ysize=%d)\n",
		 reqattr, reqattr->tta_Name, reqattr->tta_YSize));
	
    filename = reqattr->tta_Name;
    
    if ((seglist = LoadSeg(filename)) != 0)
    {
	dfh = ConvDiskFont(seglist, realfontname, DiskfontBase);
	UnLoadSeg(seglist);

	if (dfh != NULL)
	{
	    D(bug("ReadDiskFont: dfh=0x%lx\n"));
	    ReturnPtr("ReadDiskFont", struct TextFont *, &dfh->dfh_TF);
	}
	else
	{
	    D(bug("ReadDiskFont: error converting seglist\n"));
	    ReturnPtr("ReadDiskFont", struct TextFont *, NULL);
	}
    }
    else
    {
	D(bug("Could not load segment list\n"));
	ReturnPtr("ReadDiskFont", struct TextFont *, NULL);
    }
}

/****************************************************************************************/
