/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Functions for readin .font files
    Lang: English.
*/


#include "diskfont_intern.h"




/******************/
/* ReadFontDescr  */
/******************/


struct FontDescrHeader *ReadFontDescr(BPTR fh, struct DiskfontBase_intern *DiskfontBase)
{
    UWORD aword,
          numentries;
          
    struct FontDescrHeader *fdh = 0;
    
    struct FontDescr  *fdptr;
    
    
    /* Allocate the FontDescrHeader */
    
    if (!(fdh = AllocMem(sizeof (struct FontDescrHeader), MEMF_ANY|MEMF_CLEAR)) )
        goto failure;

    /* First read the file id (FCH_ID or TFCH_ID) */
    if (!ReadWord( &DFB(DiskfontBase)->dsh, &aword, fh ))
        goto failure;
    
    /* Check that this is a .font file */
    
    if (aword != FCH_ID && aword != TFCH_ID)
        goto failure;
        
    fdh->Tagged = (aword == TFCH_ID);
    
     /* Read number of (T)FontContents structs in the file */
    if (!ReadWord( &DFB(DiskfontBase)->dsh, &numentries ,fh ))
        goto failure;
    
    fdh->NumEntries = numentries;
        
    /* Allocate mem for array of FontDescrs */
    if 
    (!(
        fdh->FontDescrArray = AllocVec
        (
            UB(&fdptr[fdh->NumEntries]) - UB(&fdptr[0]),
            MEMF_ANY|MEMF_CLEAR
        )
    ))
        goto failure;
    
    fdptr = fdh->FontDescrArray;
        
    for (; numentries --; )
    {
        
        /* Read the fontname */
        
        if (!ReadString( &DFB(DiskfontBase)->dsh, &(fdptr->FontName), fh ))
            goto failure;
            
        /* Seek to the end of the fontnamebuffer ( - 2 if tagged) */
        Flush(fh);
        Seek
        (
            fh,
            MAXFONTPATH - ( (strlen(fdptr->FontName) + 1) + (fdh->Tagged ? 2 : 0) ),
            OFFSET_CURRENT
        );
            
        
        /* Do special stuff for files with tags in them */
        if (fdh->Tagged)
        {

            /* Next thing up is the tagcount */
            if (!ReadWord(&DFB(DiskfontBase)->dsh, &(fdptr->NumTags), fh))
                goto failure;
                
                
            
            if (fdptr->NumTags)
            {
                


                /* Seek back and read the tags */
                Flush(fh);
                Seek
                (
                    fh, 
                    - ( (fdptr->NumTags << 3) + sizeof (UWORD) ), /*  sizeof (struct TagItem) = 8 */
                    OFFSET_CURRENT
                );

                if (!(fdptr->Tags = ReadTags(fh, fdptr->NumTags, DFB(DiskfontBase) )))
                    goto failure;
                
                /* Seek past the tagcount */
                if (!ReadWord( &DFB(DiskfontBase)->dsh, &aword, fh ))
                    goto failure;
                

            } /* if (fdh->NumTags) */
            
        
        } /* if (fdh->Tagged) */
        
        /* Read the rest of the info */
        if (!ReadWord( &DFB(DiskfontBase)->dsh, &(fdptr->YSize), fh ))
            goto failure;

        if (!ReadByte(&DFB(DiskfontBase)->dsh, &(fdptr->Style), fh ))
            goto failure;
            
        if (!ReadByte( &DFB(DiskfontBase)->dsh, &(fdptr->Flags), fh ))
            goto failure;

        fdptr ++;
        
    } /* for (; numentries --; ) */
    
    return (fdh);

failure:
    /* Free all allocated memory */
    
    if (fdh)
        FreeFontDescr(fdh, DFB(DiskfontBase) );
        
    return(FALSE);    
}

/******************/
/* FreeFontDescr  */
/******************/


VOID FreeFontDescr(struct FontDescrHeader *fdh, struct DiskfontBase_intern *DiskfontBase)
{
    struct FontDescr *fdptr;

    UWORD numentries;
    

    if (fdh)
    {       
        fdptr       = fdh->FontDescrArray;
        numentries  = fdh->NumEntries;
    
        if (fdptr)
        {
            /* Go through the whole array freeing strings and tags */
            for (; numentries --; )
            {
                if (fdptr->FontName)
                    FreeVec(fdptr->FontName);
                
                if (fdptr->Tags)
                    FreeVec(fdptr->Tags);
                
                fdptr ++;
            }
            
            FreeVec(fdh->FontDescrArray);
        }
    
        FreeMem(fdh, sizeof (struct FontDescrHeader));
    }
    
    return;
}