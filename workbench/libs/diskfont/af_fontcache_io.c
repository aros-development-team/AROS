/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Functions for fontcache i/o
    Lang: English.
*/

#define AROS_ALMOST_COMPATIBLE

#include <string.h>

#include <proto/utility.h>
#include <proto/dos.h>

#include "diskfont_intern.h"

#define DEBUG 0
#include <aros/debug.h>

/* Structure on disk 

FIN->TAF.taf_Type;
FIN->TAF.taf_Attr.tta_YSize;
FIN->TAF.taf_Attr.tta_Style;
FIN->TAF.taf_Attr.tta_Flags;
FIN->Flags;
if not FNF_REUSENAME: FontName. 
if not FNF_REUSETAGS: NumTags, FontTags.

*/

/* A local define */

/****************************************************************************************/

#define NUMENTRIES_OFFSET sizeof(CACHE_IDSTR) + sizeof(struct  DateStamp)

/****************************************************************************************/

/************/
/* ReadTags */
/************/

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
        if (!ReadLong( &DFB(DiskfontBase)->dsh, &(tagptr->ti_Tag), fh))
            goto readfail;
            
                        
        if (!ReadLong( &DFB(DiskfontBase)->dsh, &(tagptr->ti_Data), fh ))
            goto readfail;
                        
        tagptr ++;
    }

    ReturnPtr ("ReadTags", struct TagItem*, taglist);

readfail:   
    FreeVec(taglist);
rt_failure:
    ReturnPtr("ReadTags", struct TagItem*, FALSE);
    
}

/****************************************************************************************/

/**************/
/* WriteTags  */
/**************/

/****************************************************************************************/

BOOL WriteTags(BPTR fh, struct TagItem *taglist, struct DiskfontBase_intern *DiskfontBase)
{

    struct TagItem *tag;

    D(bug("WriteTags(fh=%p, taglists=%p)\n", fh, taglist));

    for (; (tag = NextTagItem((const struct TagItem **)&taglist)); )
    {
        if (!WriteLong( &DFB(DiskfontBase)->dsh, tag->ti_Tag, fh ))
            goto wt_failure;
            
        if (!WriteLong( &DFB(DiskfontBase)->dsh, tag->ti_Data, fh))
            goto wt_failure;
    }

    ReturnBool ("WriteTags", TRUE);
    
wt_failure:
    ReturnBool ("WriteTags", FALSE);
}

/****************************************************************************************/

/************/
/* WriteFIN */
/************/

/****************************************************************************************/

STATIC BOOL WriteFIN(BPTR fh, struct FontInfoNode *finode, struct DiskfontBase_intern *DiskfontBase)
{
    /* Writes all the fields into the FontInfoNode. */
    
    struct TAvailFonts *taf;

    D(bug("WriteFIN(fh=%p, finode=%p)\n", fh, finode));

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
    
    ReturnBool ("WriteFIN", TRUE);
    
wf_failure:
    ReturnBool ("WriteFIN", FALSE);
    
}

/****************************************************************************************/

/***********/
/* ReadFIN */
/***********/

/****************************************************************************************/

STATIC BOOL ReadFIN(BPTR fh, struct FontInfoNode *finode, struct DiskfontBase_intern *DiskfontBase)
{
    /* Reads all the fields into the FontInfoNode. */
    
    struct TAvailFonts *taf;
    
  	D(bug("ReadFIN(fh=%p, finode=%p)\n", fh, finode));

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
    
    ReturnBool ("ReadFIN", TRUE);
    
rf_failure:
    ReturnBool ("ReadFIN", FALSE);
}

/****************************************************************************************/

/**************/
/* ReadDate   */
/**************/

/****************************************************************************************/

BOOL ReadDate(BPTR fh, struct DateStamp *ds, struct DiskfontBase_intern *DiskfontBase)
{
    D(bug("ReadDate(fh=%p, datestamp=%p)\n", fh, ds));

    if (!ReadLong(&DFB(DiskfontBase)->dsh, ((ULONG*)&ds->ds_Days), fh))
        goto rd_failure;

    if (!ReadLong(&DFB(DiskfontBase)->dsh, ((ULONG*)&ds->ds_Minute), fh))
        goto rd_failure;

    if (!ReadLong(&DFB(DiskfontBase)->dsh, ((ULONG*)&ds->ds_Tick), fh))
        goto rd_failure;

    ReturnBool ("ReadDate", TRUE);
    
rd_failure:
    ReturnBool ("ReadDate", FALSE);
}

/****************************************************************************************/

/**************/
/* WriteDate  */
/**************/

/****************************************************************************************/

BOOL WriteDate(BPTR fh, struct DateStamp *ds, struct DiskfontBase_intern *DiskfontBase)
{
    D(bug("WriteDate(fh=%p, datestamp=%p)\n", fh, ds));

    if (!WriteLong(&DFB(DiskfontBase)->dsh, ds->ds_Days, fh))
        goto wd_failure;

    if (!WriteLong(&DFB(DiskfontBase)->dsh, ds->ds_Minute, fh))
        goto wd_failure;

    if (!WriteLong(&DFB(DiskfontBase)->dsh, ds->ds_Tick, fh))
        goto wd_failure;

    ReturnBool ("WriteDate", TRUE);
    
wd_failure:
    ReturnBool ("WriteDate", FALSE);
}

/****************************************************************************************/

/**************/
/* ReadCache  */
/**************/

/****************************************************************************************/

BOOL ReadCache(ULONG userflags, struct MinList *filist, struct DiskfontBase_intern *DiskfontBase)
{
    BPTR fh;
    
    UWORD numentries,
          numtags;
          
    struct TagItem *taglist;
    
    struct FontInfoNode *finode = 0;
    
    D(bug("ReadCache(userflags=%d, filist=%p)\n", userflags, filist));

    
    /* Open the cache file */
    if (!(fh = Open(CACHE_FILE, MODE_OLDFILE)))
        goto rc_failure;
        
    /* Seek past the ID and datestamp */
    
    Seek(fh, NUMENTRIES_OFFSET, OFFSET_BEGINNING);

    /* Read the number of elements in the cache */
    if (!ReadWord(&DFB(DiskfontBase)->dsh, &numentries, fh))
        goto rc_failure;
        
    D(bug("\tRC: Numentries: %d\n", numentries));
    
    /* Read all the cache elements */
      
    for (; numentries --; )
    {
	finode = AllocFIN(filist, DFB(DiskfontBase) );
	if (!finode)
	    goto rc_failure;
                
        /* Read info into the finode */
        if (!ReadFIN(fh, finode, DFB(DiskfontBase) ))
            goto rc_failure;
        
        if (!(finode->Flags & FDF_REUSENAME))        
        {
            if (!ReadString(&DFB(DiskfontBase)->dsh, &(finode->TAF.taf_Attr.tta_Name), fh))
                goto rc_failure;
            finode->NameLength = strlen(finode->TAF.taf_Attr.tta_Name) + 1;
        }          

	D(bug("\tRCache: Name=%s, YSize=%d\n",
		finode->TAF.taf_Attr.tta_Name,
		finode->TAF.taf_Attr.tta_YSize)); 
            
        /* Should we read tags ? */

        if ( !(finode->Flags & FDF_REUSETAGS) )
        {                   
 
            /* Read the number of tags */
            if (!ReadWord(&DFB(DiskfontBase)->dsh, &numtags, fh))
        	goto rc_failure;

            if (numtags)
            {
                /* Read the tags themselves */

                if (!(taglist = ReadTags(fh, numtags, DFB(DiskfontBase))))
                    goto rc_failure;

		if (userflags & AFF_TAGGED)
		{
                    if (!(finode->TAF.taf_Attr.tta_Tags = CloneTagItems(taglist)))
                    {
                	FreeVec(taglist);
                	goto rc_failure;
                    }

                    finode->NumTags = NumTags(finode->TAF.taf_Attr.tta_Tags, DFB(DiskfontBase));
                }

                /* Since AllocFontTagsNode clones the taglist we must free the original one */

                FreeVec(taglist);
	
            } /* if (numtags) */
	    	    
	} /* if ( !(finode->Flags & FDF_REUSETAGS) ) */
        
    } /* for (; numentries--; ); */
    
    Close(fh);
    
    ReturnBool ("ReadCache", TRUE);
    
rc_failure:
    if (fh)
        Close(fh);
        
    ReturnBool ("ReadCache", FALSE);
}

/****************************************************************************************/

/****************/
/* WriteCache   */
/****************/

/****************************************************************************************/

BOOL WriteCache(struct MinList *filist, struct DiskfontBase_intern *DiskfontBase)
{
    ULONG numentries  = 0;
    BPTR  fh          = 0;
    WORD  numtags;
    
    /* FontInfoNode->Flags */
    UBYTE flags;
    
    struct FontInfoNode *node;
    
    struct DateStamp now;
    
    D(bug("WriteCache(filist=%p)\n", filist));

    /* Open the font file for writing */
    
    if (!(fh = Open(CACHE_FILE,MODE_NEWFILE)))
        goto wc_failure;

    /* Write the cache ID */
    if (!WriteString(&DFB(DiskfontBase)->dsh,CACHE_IDSTR, fh))
        goto wc_failure;
            
    /* Get the current time */
    DateStamp(&now);
        
    if (!WriteDate(fh, &now, DFB(DiskfontBase) ))
        goto wc_failure;           
    
    /* Leave "empty" space for numentries.numentries is inserted later, 
      when number of entries have bee counted. */
    if (!WriteWord( &DFB(DiskfontBase)->dsh, numentries, fh))
    	goto wc_failure;
    
    ForeachNode(filist, node)
    {
    	D(bug("\tWCache: Examining node %s\n", node->TAF.taf_Attr.tta_Name));
        /* Only write fonts residing on disk to the cache */
        if ( node->TAF.taf_Type & AFF_DISK)
        {
            D(bug("\tWCache: Diskfont found\n"));
        	
            flags = FIN(node)->Flags;
            numentries ++;
            
            /* Write general fontinfo */
            if (!WriteFIN(fh, node, DFB(DiskfontBase)))
                goto wc_failure;
            
            /* Write fontname if not reused */
            if ( !(flags & FDF_REUSENAME) )
            {
                if (!WriteString(&DFB(DiskfontBase)->dsh, node->TAF.taf_Attr.tta_Name, fh))
                    goto wc_failure;
            }
            
            /* Write tags if present */ 
        
	    if ( !(flags & FDF_REUSETAGS) )
	    {
        	if ( node->TAF.taf_Attr.tta_Tags )
                    numtags = NumTags(node->TAF.taf_Attr.tta_Tags, DFB(DiskfontBase));
		else
	            numtags = 0;

    		if (!WriteWord( &DFB(DiskfontBase)->dsh, numtags, fh))
		    goto wc_failure;

		if (numtags)
		{    
                    if (!WriteTags(fh, node->TAF.taf_Attr.tta_Tags, DFB(DiskfontBase) ))
                	goto wc_failure;
        	}
            }
	    
        } /* Diskfont ? */
            
    } /* ForeachNode */
        
    /* Seek back and write number of entries */
    Flush(fh);
    Seek(fh, NUMENTRIES_OFFSET, OFFSET_BEGINNING);

    if (!WriteWord(&DFB(DiskfontBase)->dsh, numentries, fh))
        goto wc_failure;
        
    Close(fh);
    
    ReturnBool ("WriteCache", TRUE);
    
    
wc_failure:

    if (fh)
        Close(fh);
        
    ReturnBool ("WriteCache", FALSE);

}

/****************************************************************************************/

/******************/
/* OKToReadCache  */
/******************/

/* Determine whether or not the cache is outdated, and whether or not
  the cache file even exists
*/

/****************************************************************************************/

/* The array of hooks is in diskfont_init.c */
extern struct AFHookDescr hdescr[];

/****************************************************************************************/

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
    
    D(bug("OKToReadCache(void)\n"));

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
                    
                    fhc.fhc_Command   = FHC_AF_GETDATE;
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
		    
                } /* if (ReadDate(fh, &cachedate, DFB(DiskfontBase) )) */
		
            } /* if (strcmp(idstr, CACHE_IDSTR) == 0) */
            FreeVec(idstr);
	    
        } /* if (ReadString(&DFB(DiskfontBase)->dsh, &idstr, fh)) */
        Close(fh);
	
    } /* if ((fh = Open(CACHE_FILE,MODE_OLDFILE)) != 0) */
    
    ReturnBool ("OKToReadCache", retval && cacheok);
}

/****************************************************************************************/
