/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Functions for fontcache i/o
    Lang: English.
*/


#include "diskfont_intern.h"

/* Structure on disk 

FIN->TAF.taf_Type;
FIN->TAF.taf_Attr.tta_YSize;
FIN->TAF.taf_Attr.tta_Style;
FIN->TAF.taf_Attr.tta_Flags;
FIN->Flags;
evt. FontName.
evt. FontTags.

*/

/* A local define */

#define NUMENTRIES_OFFSET sizeof(CACHE_IDSTR) + sizeof(struct  DateStamp)


/************/
/* ReadTags */
/************/

struct TagItem *ReadTags(BPTR fh, ULONG numtags, struct DiskfontBase_intern *DiskfontBase)
{
    struct TagItem  *taglist,
                    *tagptr;

    /* Allocate memory for the tags */
    if
    (!(
        taglist = AllocVec
        (
            UB(&taglist[ numtags ]) - UB(&taglist[0]),
            MEMF_ANY
        )
    ))
        goto rt_failure;

    tagptr = taglist;
    
    /* Read the taglist into the buffer */
                
    for (; numtags --; )
    {
        if (!ReadLong( &DFB(DiskfontBase)->dsh, &(tagptr->ti_Tag), fh))
            goto readfail;
            
                        
        if (!ReadLong( &DFB(DiskfontBase)->dsh, &(tagptr->ti_Data), fh ))
            goto readfail;
                        
        tagptr ++;
    }

    return (taglist);

readfail:   
    FreeVec(taglist);
rt_failure:
    return (FALSE);
    
}

/**************/
/* WriteTags  */
/**************/

BOOL WriteTags(BPTR fh, struct TagItem *taglist, struct DiskfontBase_intern *DiskfontBase)
{
    
    struct TagItem *tag;
    for (; (tag = NextTagItem(&taglist)); )
    {
        if (!WriteLong( &DFB(DiskfontBase)->dsh, tag->ti_Tag, fh ))
            goto wt_failure;
            
        if (!WriteLong( &DFB(DiskfontBase)->dsh, tag->ti_Data, fh))
            goto wt_failure;
    }

    return (TRUE);
    
wt_failure:
    return (FALSE);
}


/************/
/* WriteFIN */
/************/


STATIC BOOL WriteFIN(BPTR fh, struct FontInfoNode *finode, struct DiskfontBase_intern *DiskfontBase)
{
    /* Writes all the fields into the FontInfoNode. */
    
    struct TAvailFonts *taf;
    
    taf = &(finode->TAF);
    
    if (!WriteWord( &DFB(DiskfontBase)->dsh, taf->taf_Type, fh))
        goto wf_failure;

    if (!WriteWord(&DFB(DiskfontBase)->dsh, taf->taf_Attr.tta_YSize, fh ))
        goto wf_failure;

    if (!WriteByte(&DFB(DiskfontBase)->dsh, taf->taf_Attr.tta_Style, fh))
        goto wf_failure;
        
    if (!WriteByte(&DFB(DiskfontBase)->dsh, taf->taf_Attr.tta_Flags, fh ))
        goto wf_failure;

    if (!WriteByte(&DFB(DiskfontBase)->dsh, finode->Flags, fh ))
        goto wf_failure;
    
    return (TRUE);
    
wf_failure:
    return (FALSE);
    
}


/***********/
/* ReadFIN */
/***********/


STATIC BOOL ReadFIN(BPTR fh, struct FontInfoNode *finode, struct DiskfontBase_intern *DiskfontBase)
{
    /* Reads all the fields into the FontInfoNode. */
    
    struct TAvailFonts *taf;
    
    taf = &(finode->TAF);
    
    if (!ReadWord(&DFB(DiskfontBase)->dsh, &(taf->taf_Type), fh ))
        goto rf_failure;

    if (!ReadWord(&DFB(DiskfontBase)->dsh, &(taf->taf_Attr.tta_YSize), fh ))
        goto rf_failure;

    if (!ReadByte(&DFB(DiskfontBase)->dsh, &(taf->taf_Attr.tta_Style), fh ))
        goto rf_failure;
        
    if (!ReadByte(&DFB(DiskfontBase)->dsh, &(taf->taf_Attr.tta_Flags), fh ))
        goto rf_failure;

    if (!ReadByte(&DFB(DiskfontBase)->dsh, &(finode->Flags), fh))
        goto rf_failure;
    
    return (TRUE);
    
rf_failure:
    return (FALSE);
}

/**************/
/* ReadDate   */
/**************/

BOOL ReadDate(BPTR fh, struct DateStamp *ds, struct DiskfontBase_intern *DiskfontBase)
{
    if (!ReadLong(&DFB(DiskfontBase)->dsh, ((ULONG*)&ds->ds_Days), fh))
        goto rd_failure;

    if (!ReadLong(&DFB(DiskfontBase)->dsh, ((ULONG*)&ds->ds_Minute), fh))
        goto rd_failure;

    if (!ReadLong(&DFB(DiskfontBase)->dsh, ((ULONG*)&ds->ds_Tick), fh))
        goto rd_failure;

    return (TRUE);
    
rd_failure:
    return (FALSE);
}

/**************/
/* WriteDate  */
/**************/

BOOL WriteDate(BPTR fh, struct DateStamp *ds, struct DiskfontBase_intern *DiskfontBase)
{
    if (!WriteLong(&DFB(DiskfontBase)->dsh, ds->ds_Days, fh))
        goto wd_failure;

    if (!WriteLong(&DFB(DiskfontBase)->dsh, ds->ds_Minute, fh))
        goto wd_failure;

    if (!WriteLong(&DFB(DiskfontBase)->dsh, ds->ds_Tick, fh))
        goto wd_failure;

    return (TRUE);
    
wd_failure:
    return (FALSE);
}

/**************/
/* ReadCache  */
/**************/

BOOL ReadCache(ULONG userflags, struct AF_Lists *lists, struct DiskfontBase_intern *DiskfontBase)
{
    BPTR fh;
    
    UWORD numentries,
          numtags;
          
    struct TagItem *taglist;
          
    STRPTR str;
    
    struct FontInfoNode *finode = 0;
    
    /* Open the cache file */
    if (!(fh = Open(CACHE_FILE, MODE_OLDFILE)))
        goto rc_failure;
        
    /* Seek past the ID and datestamp */
    
    Seek(fh, NUMENTRIES_OFFSET, OFFSET_BEGINNING);

    /* Read the number of elements in the cache */
    if (!ReadWord(&DFB(DiskfontBase)->dsh, &numentries, fh))
        goto rc_failure;
        
    /* Read all the cache elements */
      
    for (; numentries --; )
    {

        if (!(finode = AllocMem(sizeof (struct FontInfoNode), MEMF_ANY|MEMF_CLEAR )))
            goto rc_failure;

        AddTail
        ( 
            ((struct List*)&(lists->FontInfoList)),
            ((struct Node*)finode)
        );
                
        /* Read info into the finode */
            
        if (!ReadFIN(fh, finode, DFB(DiskfontBase) ))
            goto rc_failure;
                
            
        /* If the fontname is reused, update last pointer */
        
        if ( finode->Flags & FDF_REUSENAME )
            finode->FontName = FIN(((struct Node*)finode)->ln_Pred)->FontName;
        
        else
        {
            if (!ReadString(&DFB(DiskfontBase)->dsh, &str, fh))
            {
                goto rc_failure;
            }
                    
            /* Clones the string */
            if (!(finode->FontName = AllocFontNameNode(str, DFB(DiskfontBase) )))
            {
                FreeVec(str);
                goto rc_failure;
            }

            FreeVec(str);

            AddTail
            (
                (struct List *)&(lists->FontNameList),
                (struct Node *)finode->FontName
            );
            

        }
            
        /* Should we read tags ? */
        
        if (userflags & AFF_TAGGED)
        {
            /* If this node nether reuses, tags of the last entry nor
                uses the default empty taglist, then a taglist is present.
            */
            
            if ((finode->Flags & (FDF_REUSETAGS|FDF_USEDEFTAGS)) == 0)
            {
                
                /* Read the number of tags */
                if (!ReadWord(&DFB(DiskfontBase)->dsh, &numtags, fh))
                    goto rc_failure;
                    
                /* Read the tags themselves */
                if (!(taglist = ReadTags(fh, numtags, DFB(DiskfontBase))))
                    goto rc_failure;
                    
                /* Allocate fonttagsnode (taglist is being cloned) */
                
                if (!(finode->FontTags = AllocFontTagsNode(taglist, DFB(DiskfontBase))))
                {
                    FreeVec(taglist);
                    goto rc_failure;
                }
                
                /* Since AllocFontTagsNode clones the taglist we must free the original one */

                FreeVec(taglist);

                AddTail
                (
                    (struct List *)&(lists->FontTagsList),
                    (struct Node *)finode->FontTags
                );

            }
            else if (finode->Flags & FDF_REUSETAGS)
                finode->FontTags = FIN(((struct Node*)finode)->ln_Pred)->FontTags;
          
            
        } /* if (flags & AFF_TAGGED) */
        
    } /* for (; numentries--; ); */
    
    Close(fh);
    
    return (TRUE);
    
rc_failure:
    if (fh)
        Close(fh);
        
    return (FALSE);
}

/****************/
/* WriteCache   */
/****************/

BOOL WriteCache(struct AF_Lists *lists, struct DiskfontBase_intern *DiskfontBase)
{
    ULONG numentries  = 0;
    BPTR  fh          = 0;
    BOOL  retval;
    
    /* FontInfoNode->Flags */
    UBYTE flags;
    
    struct MinNode *node;
    
    struct DateStamp now;
    
    
    /* Open the font file for writing */
    
    if (!(fh = Open(CACHE_FILE,MODE_NEWFILE)))
        retval = FALSE;
    else
    {
        /* Write the cache ID */
        if (!WriteString(&DFB(DiskfontBase)->dsh,CACHE_IDSTR, fh))
            goto wc_failure;
            
        /* Get the current time */
        DateStamp(&now);
        
        if (!WriteDate(fh, &now, DFB(DiskfontBase) ))
            goto wc_failure;           
        ForeachNode(&(lists->FontInfoList), node)
        {
            /* Only write fonts residing on disk to the cache */
            if ( FIN(node)->TAF.taf_Type & AFF_DISK)
            {
                flags = FIN(node)->Flags;
                numentries ++;
            
            
                /* Write general fontinfo */
                if (!WriteFIN(fh, FIN(node), DFB(DiskfontBase)))
                    goto wc_failure;
            
                /* Write fontname if not reused */
                if ( !(flags & FDF_REUSENAME) )
                {
                    if (!WriteString(&DFB(DiskfontBase)->dsh, FIN(node)->FontName->FontName, fh))
                        goto wc_failure;
                }
            
                /* If neither defaulttags nor reuse of tags, then a tagarray is present
                and we should write it
                */
            
                if ( (flags & (FDF_REUSETAGS|FDF_USEDEFTAGS)) == 0)
                {
                    if (!WriteTags(fh, FIN(node)->FontTags->TagList, DFB(DiskfontBase) ))
                        goto wc_failure;
                }
                
            } /* Diskfont ? */
            
        } /* ForeachNode */
        
        /* Seek back and write number of entries */
        Flush(fh);
        Seek(fh, NUMENTRIES_OFFSET, OFFSET_BEGINNING);

        if (!WriteWord(&DFB(DiskfontBase)->dsh, numentries, fh))
            goto wc_failure;
        
        Close(fh);
    } /* if (Open()) */
    
    return (TRUE);
    
    
wc_failure:

    if (fh)
        Close(fh);
        
    return (FALSE);

}

/******************/
/* OKToReadCache  */
/******************/

/* Determine whether or not the cache is outdated, and whether or not
  the cache file even exists
*/


/* The array of hooks is in diskfont_init.c */
extern struct AFHookDescr hdescr[];

BOOL OKToReadCache(struct DiskfontBase_intern *DiskfontBase)
{
    BPTR  fh;
    BOOL  retval  = FALSE,
          cacheok = TRUE;
    
    STRPTR idstr;
    
    struct AFHookDescr      *afhd;
    struct FontHookCommand  fhc;

   	UWORD idx;
   
    /* Last time a font source was updated */
    struct DateStamp fontsourcedate;
    
    struct DateStamp cachedate;
    
    if ((fh = Open(CACHE_FILE,MODE_OLDFILE)) != 0)
    {
        if (ReadString(&DFB(DiskfontBase)->dsh, &idstr, fh))
        {
            if (strcmp(idstr, CACHE_IDSTR) == 0)
            {
                if (ReadDate(fh, &cachedate, DFB(DiskfontBase) ))
                {
                    /* All initalisation went well */
                    retval = TRUE;
                    
                    fhc.fhc_Command   = FHC_GETDATE;
                    fhc.fhc_UserData  = (APTR)&fontsourcedate;
         	
                    for (idx = 0; idx < NUMFONTHOOKS; idx ++)
                    {
                    	afhd = &DFB(DiskfontBase)->hdescr[idx];
                    	
                        /* Only check for hooks reading from disk */
                        if (afhd->ahd_Flags & AFF_DISK)
                        {
                            CallHookPkt(&afhd->ahd_Hook, &fhc, DFB(DiskfontBase) );
                      
                            /* Is cache older than fontsource ? */
                            if (CompareDates(&cachedate, &fontsourcedate) > 0)
                                cacheok = FALSE;
                        }
                    }
                }
            }
            FreeVec(idstr);
        }
        Close(fh);
    }
    
    return (retval && cacheok);
}