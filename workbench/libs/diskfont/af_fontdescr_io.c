/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Functions for readin .font files
    Lang: English.
*/

/****************************************************************************************/

#include <diskfont/diskfonttag.h>
#include <aros/macros.h>

#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/arossupport.h>
#include <string.h>
#include "diskfont_intern.h"

#include <aros/debug.h>

/****************************************************************************************/

/******************/
/* ReadFontDescr  */
/******************/

/****************************************************************************************/

struct FontDescrHeader *ReadFontDescr(CONST_STRPTR filename, struct DiskfontBase_intern *DiskfontBase)
{
    struct FontDescrHeader  *fdh = 0;
    struct TTextAttr 	    *tattr;
    BPTR    	    	     fh;
    UWORD   	    	     aword, numentries, numtags;
    UWORD   	    	    *availsizes = NULL;
    UWORD   	    	     numoutlineentries = 0;
    UBYTE   	    	     strbuf[MAXFONTPATH + 1];

    D(bug("ReadFontDescr(filename=%s\n", filename));
    
    /* Allocate the FontDescrHeader */
    if (!(fh = Open(filename, MODE_OLDFILE)))
    	goto failure;

    if (!(fdh = AllocMem(sizeof (struct FontDescrHeader), MEMF_ANY | MEMF_CLEAR)) )
        goto failure;

    /* First read the file id (FCH_ID or TFCH_ID) */
    if (!ReadWord( &DFB(DiskfontBase)->dsh, &aword, (void *)fh ))
        goto failure;
    
    /* Check that this is a .font file */
    
    if ( (aword != FCH_ID) && (aword != TFCH_ID) && (aword != OFCH_ID) )
    {
	D(bug("ReadFontDescr: unsupported file id\n"));
        goto failure;
    }
        
    fdh->ContentsID = aword;
    
    fdh->Tagged = ((aword == TFCH_ID) || (aword == OFCH_ID));
    
     /* Read number of (T)FontContents structs in the file */
    if (!ReadWord( &DFB(DiskfontBase)->dsh, &numentries , (void *)fh ))
    {
	D(bug("ReadFontDescr: error reading numentries\n"));
        goto failure;
    }
    
    fdh->NumEntries = numentries;

    if (fdh->ContentsID == OFCH_ID)
    {
    	fdh->OTagList = OTAG_GetFile(filename, DiskfontBase);
	if (fdh->OTagList)
	{
	    availsizes = (UWORD *)GetTagData(OT_AvailSizes, (IPTR) NULL, fdh->OTagList->tags);
	    if (availsizes)
	    {
	    	/* OT_AvailSizes points to an UWORD array, where the first UWORD
                   is the number of entries, and the following <numentry> UWORD's
		   are the sizes */
		   
	    	numoutlineentries = AROS_BE2WORD(*availsizes);
		availsizes++;
		
	    }
	}
	else
	    D(bug("ReadFontDescr: No OTagList for outlined font\n"));
    }
    
    if (!(numentries + numoutlineentries)) goto failure;
    
    /* Allocate mem for array of TTextAttrs */
    fdh->TAttrArray = AllocVec((fdh->NumEntries + numoutlineentries) * sizeof(struct TTextAttr), MEMF_ANY|MEMF_CLEAR);
    if (!fdh->TAttrArray) goto failure;
    
    tattr = fdh->TAttrArray;
        
    while (numentries--)
    {
        
        /* Read the fontname */
        
        STRPTR	strptr;
        UWORD   len = 0; /* Length of fontname in file including null-termination */

        strptr = strbuf;
        
        do
        {
       	    if (!ReadByte( &DFB(DiskfontBase)->dsh, strptr, (void *)fh))
        	goto failure;
        		
            len ++;
        }
        while (*strptr ++);

    	/* We set tattr->tta_Name to the real font name, for
	   example FONTS:helvetica/15 */
	   
	if (!(tattr->tta_Name = AllocVec(len + 1, MEMF_ANY)))
	    goto failure;
		
	strcpy(tattr->tta_Name, strbuf);

	D(bug("ReadFontDescr: tta_Name \"%s\"\n", tattr->tta_Name));
       	    
        /* Seek to the end of the fontnamebuffer ( - 2 if tagged) */
        Flush(fh);
        Seek
        (
            fh,
            MAXFONTPATH - ( len + (fdh->Tagged ? 2 : 0) ),
            OFFSET_CURRENT
        );
            
       
        /* Do special stuff for files with tags in them */
        if (fdh->Tagged)
        {

            /* Next thing up is the tagcount */
            if (!ReadWord(&DFB(DiskfontBase)->dsh, &numtags, (void *)fh))
                goto failure;
                
            
            if (numtags)
            {               
	        /* Seek back and read the tags. Note, that the TAG_DONE
		   tagitem "goes over" the numtags, ie. it's ti_Data will
		   contain the numtags in the lower WORD:
		   
		             TAGLIST start
		            /
		   00000000 80000008 00370000 80000001
		   003F003F 80000002 00640064 00000000
		   00000004 00378022          \TAG_DONE
		       |||| ||||||||
		   TagCount ||||||Flags
		            |||||| 
			    ||||Style
			    ||||
			    YSize
	    	*/
		
                Flush(fh);
                Seek
                (
                    fh, 
                    -(numtags * 8), /*  sizeof (struct TagItem) = 8 */
                    OFFSET_CURRENT
                );

                if (!(tattr->tta_Tags = ReadTags(fh, numtags, DFB(DiskfontBase) )))
                    goto failure;
                
            } /* if (numtags) */
            else
		tattr->tta_Tags = NULL;
        
        } /* if (fdh->Tagged) */
	else
	    tattr->tta_Tags = NULL;

        D(bug("ReadFontDescr: font \"%s\" tags %p\n", tattr->tta_Name, tattr->tta_Tags));
	
        /* Read the rest of the info */
        if (!ReadWord( &DFB(DiskfontBase)->dsh, &(tattr->tta_YSize), (void *)fh ))
            goto failure;

        if (!ReadByte(&DFB(DiskfontBase)->dsh, &(tattr->tta_Style), (void *)fh ))
            goto failure;
            
        if (!ReadByte( &DFB(DiskfontBase)->dsh, &(tattr->tta_Flags), (void *)fh ))
            goto failure;

        tattr ++;
        
    } /* while (numentries--) */
    
    Close(fh); fh = 0;
      
    if (numoutlineentries)
    {
    	UBYTE flags = OTAG_GetFontFlags(fdh->OTagList, DiskfontBase);
	UBYTE style = OTAG_GetFontStyle(fdh->OTagList, DiskfontBase);
	
	while(numoutlineentries--)
	{
	    UWORD size;
	    UWORD i;
	    BOOL  exists = FALSE;
	    
	    size = AROS_BE2WORD(*availsizes);
	    availsizes++;
	    
	    /* Check if this size already exists as bitmap
	       font. If it does, then ignore it */
	       
	    for(i = 0; i < fdh->NumEntries; i++)
	    {
	    	if (fdh->TAttrArray[i].tta_YSize == size)
		{
		    exists = TRUE;
		    break;
		}
	    }
	    
	    if (exists) continue;
	    
	    /* NumEntries contains the total number of entries, ie.
	       bitmap fonts and outline fonts included. NumOutlineEntries
	       tells how many among these (at the end of the array) are
	       outline fonts */
	       
	    fdh->NumEntries++;
	    fdh->NumOutlineEntries++;
	    
	    tattr->tta_Name  = fdh->OTagList->filename;
	    tattr->tta_YSize = size;
	    tattr->tta_Flags = flags;
	    tattr->tta_Style = style;
	    tattr->tta_Tags  = NULL;

	    D(bug("ReadFontDescr: font \"%s\" tags %p\n", tattr->tta_Name, tattr->tta_Tags));
	    
	    tattr++;
 	} /* while(numoutlineentries--) */
	
    } /* if (numoutlineentries) */
    
    ReturnPtr ("ReadFontDescr", struct FontDescrHeader *, fdh);

failure:
    /* Free all allocated memory */
    
    if (fh) Close(fh);
    
    if (fdh)
        FreeFontDescr(fdh, DFB(DiskfontBase) );
        
    ReturnPtr ("ReadFontDescr", struct FontDescrHeader*, FALSE);

}

/****************************************************************************************/

/******************/
/* FreeFontDescr  */
/******************/

/****************************************************************************************/

VOID FreeFontDescr(struct FontDescrHeader *fdh, struct DiskfontBase_intern *DiskfontBase)
{
    struct TTextAttr *tattr;

    UWORD numentries;
    
    D(bug("FreeFontDescr(fdh=%p\n", fdh));

    if (fdh)
    {       
        tattr       = fdh->TAttrArray;
        numentries  = fdh->NumEntries - fdh->NumOutlineEntries;
    
        if (tattr)
        {
            /* Go through the whole array of bitmap font entries
	       freeing strings and tags. Outline font entries
	       always have the tta_Name pointing to the otag file
	       and must not get the tta_Name freed. They also have
	       tta_Tags always being NULL. Therefore the loop below
	       goes only through the bitmap font entries */
	       
            while(numentries--)
            {
                if (tattr->tta_Name)
                    FreeVec(tattr->tta_Name);

		if (tattr->tta_Tags)
		    FreeVec(tattr->tta_Tags);
                
                tattr ++;
            }
            
            FreeVec(fdh->TAttrArray);
        } 
    
    	if (fdh->OTagList) OTAG_KillFile(fdh->OTagList, DiskfontBase);
	
        FreeMem(fdh, sizeof (struct FontDescrHeader));
    }
    
    ReturnVoid("FreeFontDescr");
}

/****************************************************************************************/
