/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Functions for reading disk font files.
    Lang: English.
*/

/****************************************************************************************/

#include <stdio.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <dos/doshunks.h>
#include <dos/dosasl.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <string.h>
#include "diskfont_intern.h"

/****************************************************************************************/

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

/****************************************************************************************/

VOID EndianConv(ULONG *valptr)
{
    ULONG tmp;

    tmp = UB(valptr)[0] << 24 | UB(valptr)[1] << 16 | UB(valptr)[2] << 8 | UB(valptr)[3];

    *valptr = tmp;

    return;	
}

/****************************************************************************************/

struct hunk
{
    ULONG size;
    UBYTE *memory;
};

/****************************************************************************************/

static int read_block(BPTR file, APTR buffer, ULONG size)
{
    LONG subsize;
    UBYTE *buf=(UBYTE *)buffer;

    while(size)
    {
	subsize=Read(file,buf,size);
	if(subsize==0)
	{
	    ((struct Process *)FindTask(NULL))->pr_Result2=ERROR_BAD_HUNK;
	    return 1;
	}
	if(subsize<0)
	    return 1;
	buf+=subsize;
	size-=subsize;
    }
    return 0;
}

/****************************************************************************************/

BPTR LoadSeg_AOS(BPTR file)
{
    struct hunk 	*hunktab = NULL;
    ULONG 		hunktype, count = 0, first, last, offset, curhunk = 0, numhunks;
    LONG 		t;
    UBYTE 		name_buf[255];
    register int 	i;
    BPTR 		last_p = 0;
  #if DEBUG
    static STRPTR 	segtypes[] = { "CODE", "DATA", "BSS", };
  #endif


  #define ERROR(a)    { *error=a; goto end; }
    LONG 		*error=&((struct Process *)FindTask(NULL))->pr_Result2;

    if (Seek(file, 0, OFFSET_BEGINNING) < 0)
	goto end;
	
    while(!read_block(file, &hunktype, sizeof(hunktype)))
    {
  	EndianConv(&hunktype);

	switch(hunktype)
	{
	    ULONG tmp, req;

	case HUNK_SYMBOL:
	    while(!read_block(file, &count, sizeof(count)) && count)
	    {
		EndianConv(&count);
		if (Seek(file, (count+1)*4, OFFSET_CURRENT) < 0)
		    goto end;
	    }
	    break;
	    
	case HUNK_UNIT:
	    if (read_block(file, &count, sizeof(count)))
		goto end;
	      
	    EndianConv(&count);
	    count /= 4;
	    
	    if (read_block(file, name_buf, count))
		goto end;
	      
	    D(bug("HUNK_UNIT: \"%.*s\"\n", count, name_buf));
	    break;
	  
	case HUNK_HEADER:
	    D(bug("HUNK_HEADER:\n"));
	    while (1)
	    {
		if (read_block(file, &count, sizeof(count)))
		    goto end;
		    
		EndianConv(&count);
		
		if (count == 0L)
		    break;
		    
		count *= 4;
		
		if (read_block(file, name_buf, count))
		    goto end;
		    
		D(bug("\tlibname: \"%.*s\"\n", count, name_buf));
	    }
	    
	    if (read_block(file, &numhunks, sizeof(numhunks)))
		goto end;
		
	    EndianConv(&numhunks);
	    
	    D(bug("\tHunk count: %ld\n", numhunks));
	    hunktab = (struct hunk *)AllocVec(sizeof(struct hunk) * numhunks,
					      MEMF_CLEAR);
	    if (hunktab == NULL)
		ERROR(ERROR_NO_FREE_STORE);
		
	    if (read_block(file, &first, sizeof(first)))
		goto end;
		
	    EndianConv(&first);
	    D(bug("\tFirst hunk: %ld\n", first));
	    curhunk = first;
	    
	    if (read_block(file, &last, sizeof(last)))
		goto end;
	      
	    EndianConv(&last);
	    D(bug("\tLast hunk: %ld\n", last));
	    
	    for (i = first; i <= last; i++)
	    {
		if (read_block(file, &count, sizeof(count)))
		    goto end;
		    
		EndianConv(&count);
		tmp = count & 0xC0000000;
		count &= 0x3FFFFFFF;
		D(bug("\tHunk %d size: 0x%06lx bytes in ", i, count*4));
		
		switch(tmp)
		{
		case 0x80000000:
		    D(bug("FAST"));
		    req = MEMF_FAST;
		    break;
		    
		case 0x40000000:
		    D(bug("CHIP"));
		    req = MEMF_CHIP;
		    break;
		    
		default:
		    D(bug("ANY"));
		    req = MEMF_ANY;
		    break;
		    
		}
		
		D(bug(" memory\n"));
		hunktab[i].size = count * 4;
		hunktab[i].memory = (UBYTE *)AllocVec(hunktab[i].size + sizeof(BPTR),
	                                	      (req | MEMF_CLEAR));
		if (hunktab[i].memory == NULL)
		    ERROR(ERROR_NO_FREE_STORE);
		  
		hunktab[i].memory += sizeof(BPTR);
	    }
	    break;
	  
	case HUNK_CODE:
	case HUNK_DATA:
	case HUNK_BSS:
	    if (read_block(file, &count, sizeof(count)))
		goto end;
		
	    EndianConv(&count);
	    tmp = count & 0xC0000000;
	    count &= 0x3fffffff;
	    D(bug("HUNK_%s(%d): Length: 0x%06lx bytes in ",
		  segtypes[hunktype-HUNK_CODE], curhunk, count*4));

	    switch(tmp)
	    {
	    case 0x8000000:
		D(bug("FAST"));
		req = MEMF_FAST;
		break;
	      
	    case 0x40000000:
		D(bug("CHIP"));
		req = MEMF_CHIP;
		break;
	      
	    default:
		D(bug("ANY"));
		req = MEMF_ANY;
		break;
		
	    }
	    
	    D(bug(" memory\n"));
	    if (hunktype != HUNK_BSS && count)
		if (read_block(file, hunktab[curhunk].memory, count*4))
		    goto end;
	    break;
	  
	case HUNK_RELOC32:
	    D(bug("HUNK_RELOC32:\n"));
	    while (1)
	    {
		ULONG *addr;

		if (read_block(file, &count, sizeof(count)))
		    goto end;
		    
		EndianConv(&count);
		
		if (count == 0L)
		    break;
		    
		i = count;
		
		if (read_block(file, &count, sizeof(count)))
		    goto end;
		    
		EndianConv(&count);
		
		D(bug("\tHunk #%ld:\n", count));
		while (i > 0)
		{
		    if (read_block(file, &offset, sizeof(offset)))
			goto end;
			
		    EndianConv(&offset);
		    D(bug("\t\t0x%06lx\n", offset));
		    addr = (ULONG *)(hunktab[curhunk].memory + offset);
		    EndianConv(addr);
		    *addr += (ULONG)(hunktab[count].memory);
		    D(bug("\taddr: %p\tRelocaddr: %p\thunkaddr: %p\n", addr, *addr, hunktab[count].memory));
		    --i;
		}
		D(bug("%d\n", 0x0646 -0x0076));
	    }
	    break;
	  
	case HUNK_END:
	    D(bug("HUNK_END\n"));
	    ++curhunk;
	    break;
	  
	case HUNK_RELOC16:
	case HUNK_RELOC8:
	case HUNK_NAME:
	case HUNK_EXT:
	case HUNK_DEBUG:
	case HUNK_OVERLAY:
	case HUNK_BREAK:
	default:
	    ERROR(ERROR_BAD_HUNK);
	  
	} /* switch(hunktype) */
      
    } /* while(!read_block(file, &hunktype, sizeof(hunktype)))*/
    
    /* Clear caches */
    for (t=last; t >= (LONG)first; t--)
    {
	if (hunktab[t].size)
	{
	    CacheClearE(hunktab[t].memory, hunktab[t].size, CACRF_ClearI|CACRF_ClearD);
	    ((BPTR *)hunktab[t].memory)[-1] = last_p;
	    last_p = MKBADDR((BPTR *)hunktab[t].memory-1);
	}
    }
    FreeVec(hunktab);
    hunktab = NULL;
end:
    if (hunktab != NULL)
    {
	for (t = first; t <= last; t++)
	    if (hunktab[t].memory != NULL)
		FreeVec(hunktab[t].memory - sizeof(BPTR));
	FreeVec(hunktab);
    }
    return last_p;
}

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

#define COPYPTR(ptr, destptr) \
((APTR)destptr) = (APTR)(ptr[0] | ptr[1] << 8 | ptr [2] << 16 | ptr[3] << 24); \
SKIPPTR(ptr);

/****************************************************************************************/

/****************/
/* ConvDiskFont */
/****************/

/****************************************************************************************/

struct TextFont *ConvDiskFont(
	BPTR seglist, 
	STRPTR fontname, 
	struct DiskfontBase_intern *DiskfontBase)
{
    UWORD count, numchars, id;
    register int i;

    UBYTE *ptr;

    UWORD *destptr;
    ULONG chardatasize;
    struct TextFont tmp_tf, *tf = 0;

    APTR ctf_chardata_ptrs[8] = {0};
    struct ColorFontColors *cfc_ptr = 0;
    UWORD *colortable_ptr = NULL;

    APTR    chardata_ptr 	= NULL,
	    charloc_ptr  	= NULL,
	    charspace_ptr 	= NULL,
	    charkern_ptr 	= NULL,
	    taglist_ptr 	= NULL;


    EnterFunc(bug("ConvDiskFont(seglist=%p, fontname=%s)\n", seglist, fontname));		

    /* Get start of diskfontheader. (Go past the dummy exe header) */ 
    ptr = UB(BADDR(seglist)) + sizeof (ULONG) * 2;

    /* Skip the whole DiskFontHeader */
    SKIPPTR(ptr);	/* dfh_DF.ln_Succ	*/
    SKIPPTR(ptr);	/* dfh_DF.ln_Pred	*/
    SKIPBYTE(ptr);	/* dfh_DF.ln_Type	*/	
    SKIPBYTE(ptr);	/* dfh_DF.ln_Pri	*/
    SKIPPTR(ptr);	/* dfh_DF.ln_Name	*/
    CONVWORD(ptr, id);

    if (id != DFH_ID)
	goto failure;

    SKIPWORD(ptr);
    COPYPTR(ptr, taglist_ptr);
    ptr += MAXFONTNAME;

    /* Clear temporary textfont struct */
    memset(&tmp_tf, 0, sizeof (struct TextFont));

#if 0
    {
	UWORD i;
	for (i = 0; i < sizeof (struct TextFont); i ++)
	    printf("%d\t%d\n", ((UBYTE *)&tmp_tf)[i], sizeof (struct TextFont));
    }
#endif	

    /* Skip nodes successor and predecessor field */
    SKIPPTR(ptr);
    SKIPPTR(ptr);

    /* skip type and pri */
    SKIPBYTE(ptr);
    SKIPBYTE(ptr);
    tmp_tf.tf_Message.mn_Node.ln_Type = NT_FONT;

    /* Skip name pointer, replyport and msg length */
    SKIPPTR(ptr);
    SKIPPTR(ptr);
    SKIPWORD(ptr);

    CONVWORD(ptr, tmp_tf.tf_YSize);
    CONVBYTE(ptr, tmp_tf.tf_Style);
    CONVBYTE(ptr, tmp_tf.tf_Flags);
    CONVWORD(ptr, tmp_tf.tf_XSize);
    CONVWORD(ptr, tmp_tf.tf_Baseline);
    CONVWORD(ptr, tmp_tf.tf_BoldSmear);
    SKIPWORD(ptr); /* tf_Accessors */
    CONVBYTE(ptr, tmp_tf.tf_LoChar);
    CONVBYTE(ptr, tmp_tf.tf_HiChar);
    COPYPTR(ptr, chardata_ptr); /* tf_CharData */
    CONVWORD(ptr, tmp_tf.tf_Modulo);
    COPYPTR(ptr, charloc_ptr); /* tf_CharLoc		*/
    COPYPTR(ptr, charspace_ptr); /* tf_CharSpace	*/
    COPYPTR(ptr, charkern_ptr); /* tf_CharKern		*/

D(bug("Textfont struct converted\n"));
    D(bug("YSize:     %d\n",	tmp_tf.tf_YSize));
    D(bug("Style:     %d\n",	tmp_tf.tf_Style));
    D(bug("Flags:     %d\n",	tmp_tf.tf_Flags));
    D(bug("XSize:     %d\n",	tmp_tf.tf_XSize));
    D(bug("Baseline:  %d\n",	tmp_tf.tf_Baseline));
    D(bug("Boldsmear: %d\n",	tmp_tf.tf_BoldSmear));
    D(bug("LoChar:    %d\n",	tmp_tf.tf_LoChar));
    D(bug("HiChar:    %d\n",	tmp_tf.tf_HiChar));
    D(bug("chardara:  %p\n", 	chardata_ptr));
    D(bug("Modulo:    %d\n", 	tmp_tf.tf_Modulo));
    D(bug("charloc:   %p\n", 	charloc_ptr));
    D(bug("charspace: %p\n", 	charspace_ptr));
    D(bug("charkern:  %p\n", 	charkern_ptr));

    /* Allocate memory for font */
    tf = AllocVec( tmp_tf.tf_Style & FSF_COLORFONT ? 
			    sizeof (struct ColorTextFont) : sizeof (struct TextFont)
		  ,  MEMF_ANY|MEMF_CLEAR);
    if (!tf)
	goto failure;

D(bug("charkern in temp:  %p\n", 	tmp_tf.tf_CharKern));

    /*	Copy allready converted stuff into allocated mem */
    CopyMem(&tmp_tf, tf, sizeof (struct TextFont));

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
	    COPYPTR(ptr,  ctf_chardata_ptrs[count]);
	}

	/* ------------------------------- */
	/* Handle ColorFontColors structure */
	#undef CFC
	#define CFC(p) ((struct ColorFontColors*)p) 
	if (!(CTF(tf)->ctf_ColorFontColors = AllocVec(sizeof (struct ColorFontColors), MEMF_ANY|MEMF_CLEAR)))
	    goto failure;

	temp_ptr = CTF(tf)->ctf_ColorFontColors;

	ptr = (UBYTE *)cfc_ptr;
	SKIPWORD(ptr);
	CONVWORD(ptr, CFC(temp_ptr)->cfc_Count);
	COPYPTR (ptr, colortable_ptr);

	/* ------------------------------- */
	/* Handle colortable */
	count = CFC(temp_ptr)->cfc_Count;
	ptr = (UBYTE *)colortable_ptr;

	if (!(CFC(temp_ptr)->cfc_ColorTable = AllocVec(count * sizeof (UWORD), MEMF_ANY)))
	    goto failure;

	for (i = 0; i < count; i ++)
	{
	    CONVWORD(ptr, CFC(temp_ptr)->cfc_ColorTable[i]);
	}

	/* ------------------------------- */
	/* Handle character bitmap data for colorfonts */
	for (i = 0; i < 8; i ++)
	{
	    if (!ctf_chardata_ptrs[i])
		continue;

	    if (!(CTF(tf)->ctf_CharData[i] = AllocVec(chardatasize, MEMF_ANY)))
		goto failure;

	    CopyMem(ctf_chardata_ptrs[i], CTF(tf)->ctf_CharData[i], chardatasize); 
	}
    }
    else
    {
D(bug("B&W font\t Chardatasize: %d\n", chardatasize));
	/* Character data for B/W fonts */
	if (!(tf->tf_CharData = AllocVec(chardatasize, MEMF_ANY)))
	    goto failure;
D(bug("chardataptr=%p\n",chardata_ptr));
	CopyMem(chardata_ptr, tf->tf_CharData, chardatasize);
D(bug("Chardata copied\n"));
    }		

    /* ----------------------- */
    /* Add fontname */
    if (!(tf->tf_Message.mn_Node.ln_Name = AllocVec( strlen(fontname) + 1, MEMF_ANY)))
	goto failure;
    strcpy(tf->tf_Message.mn_Node.ln_Name, fontname);
D(bug("Fontname copied, %s\n", fontname));

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
	taglist = (struct TagItem *)AllocVec(numtags * sizeof(struct TagItem), MEMF_ANY);
	if (!taglist)
	    goto failure;

	/* Copy tags into allocated mem */
	ptr = taglist_ptr;
	for (i = 0; i < numtags; i ++ )
	{
	    CONVLONG(ptr, taglist[i].ti_Tag);
	    CONVLONG(ptr, taglist[i].ti_Data);
	}

	if (!ExtendFont(tf, taglist))
	{
	    FreeVec(taglist);
	    goto failure;
	}
    }
    else
    {
D(bug("No tags, extending it\n"));

	if (!ExtendFont(tf, NULL))
	    goto failure;
    }
    /* ----------------------- */
    /* Allocate memory for charloc */

D(bug("Doing charloc\n"));
    if (!(tf->tf_CharLoc = AllocVec(numchars * sizeof (ULONG) , MEMF_ANY)))
	goto failure;

    /* Convert charloc data */
    ptr = charloc_ptr;
    destptr = tf->tf_CharLoc;
    for (i = 0; i < numchars; i ++ )
    {
        CONVLONG(ptr,  *((ULONG *)destptr) ++);
	D(bug("charloc[%d]: %x\n", i, ((ULONG *)destptr)[-1]));
    }
D(bug("Charloc OK\n"));	

    /* ----------------------- */
    /* Only proportional fonts have a CharSpace array */

    if (charspace_ptr/* tf->tf_Flags & FPF_PROPORTIONAL*/)
    {

D(bug("Proportional font\n"));
	if (!(tf->tf_CharSpace = AllocVec(numchars * sizeof (UWORD) , MEMF_ANY)))
	    goto failure;

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
	if (!(tf->tf_CharKern = AllocVec(numchars * sizeof (UWORD) , MEMF_ANY)))
	    goto failure;

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

    ReturnPtr("ConvTextFont", struct TextFont *, tf);

failure:

    D(bug("failure\n"));

    if (tf)
    {
	if (tf->tf_Message.mn_Node.ln_Name)
	    FreeVec(tf->tf_Message.mn_Node.ln_Name);

	if (ExtendFont(tf, 0L))
	    StripFont(tf);

	if  (tf->tf_Style & FSF_COLORFONT)
	{
	    struct ColorFontColors *cfc;

	    for (i = 0; i < 8; i ++)
		if (CTF(tf)->ctf_CharData[i])
		    FreeVec(CTF(tf)->ctf_CharData[i]);

	    cfc = CTF(tf)->ctf_ColorFontColors;
	    if (cfc)
	    {
		if (cfc->cfc_ColorTable)
		    FreeVec(cfc->cfc_ColorTable);

		FreeVec(cfc);
	    }
	}
	else
	{
	    if (tf->tf_CharData)
		FreeVec(tf->tf_CharData);
	}

	if (tf->tf_CharLoc)
	    FreeVec(tf->tf_CharLoc);

	if (tf->tf_CharSpace)
	    FreeVec(tf->tf_CharSpace);

	if (tf->tf_CharKern)
	    FreeVec(tf->tf_CharKern);

	FreeVec(tf);

    }

    ReturnPtr("ConvTextFont", struct TextFont *, 0);		
}

/****************************************************************************************/

/****************/
/* ReadDiskFont	*/
/****************/

/****************************************************************************************/

struct TextFont *ReadDiskFont(
	struct TTextAttr *reqattr, 
	struct DiskfontBase_intern *DiskfontBase)
{	
    STRPTR filename;
    UWORD  len;
    UBYTE  ysizebuf[4];
    struct TextFont *tf = NULL;
	
    EnterFunc(bug("ReadDiskFont(reqattr=%p, name=%s, ysize=%d)\n",
		 reqattr, reqattr->tta_Name, reqattr->tta_YSize));
	
    /* Construct the font's path + filename */
    len = strcspn(reqattr->tta_Name, ".");
    
    snprintf( ysizebuf
    	    , sizeof (ysizebuf)
	    , "%d"
	    , reqattr->tta_YSize );
			
    /* Allocate mem for constructed filename */
    filename = AllocVec(   sizeof (FONTSDIR) + len  + sizeof("/") 
    				+ strlen(ysizebuf) + 1
			 , MEMF_ANY);
		
    if (filename)
    { 
	BPTR fh, seglist;
	
	strcpy (filename, FONTSDIR);
	strncat(filename, reqattr->tta_Name, len);
	strcat (filename, "/");
	strcat (filename, ysizebuf);
		
	
	if ((fh = Open(filename, MODE_OLDFILE)) != 0)
	{
	    if ((seglist = LoadSeg_AOS(fh)) != 0)
	    {
		tf = ConvDiskFont(seglist, reqattr->tta_Name, DiskfontBase);
			
		UnLoadSeg(seglist);
	    }
	    Close(fh);
	}
	FreeVec(filename);
    }
    ReturnPtr("ReadDiskFont", struct TextFont *, tf);	
}

/****************************************************************************************/
