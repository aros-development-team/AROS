/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Functions for fontcache i/o
    Lang: English.
*/

/****************************************************************************************/

#ifndef NO_FONT_CACHE

/****************************************************************************************/

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

/****************************************************************************************/

#define NUMENTRIES_OFFSET sizeof(CACHE_IDSTR) + sizeof(struct  DateStamp)

/****************************************************************************************/

STATIC BOOL WriteFIN(BPTR fh, struct FontInfoNode *finode, struct DiskfontBase_intern *DiskfontBase)
{
    /* Writes all the fields into the FontInfoNode. */
    
    struct TAvailFonts *taf;

    D(bug("WriteFIN(fh=%p, finode=%p)\n", fh, finode));

    taf = &(finode->TAF);
    
    if (!WriteWord( &DFB(DiskfontBase)->dsh, taf->taf_Type, (void *)fh))
        goto wf_failure;

    if (!WriteWord(&DFB(DiskfontBase)->dsh, taf->taf_Attr.tta_YSize, (void *)fh ))
        goto wf_failure;

    if (!WriteByte(&DFB(DiskfontBase)->dsh, taf->taf_Attr.tta_Style, (void *)fh))
        goto wf_failure;
        
    if (!WriteByte(&DFB(DiskfontBase)->dsh, taf->taf_Attr.tta_Flags & ~FPF_REMOVED, (void *)fh ))
        goto wf_failure;

    if (!WriteByte(&DFB(DiskfontBase)->dsh, finode->Flags, (void *)fh ))
        goto wf_failure;
    
    ReturnBool ("WriteFIN", TRUE);
    
wf_failure:
    ReturnBool ("WriteFIN", FALSE);
    
}

/****************************************************************************************/

STATIC BOOL ReadFIN(BPTR fh, struct FontInfoNode *finode, struct DiskfontBase_intern *DiskfontBase)
{
    /* Reads all the fields into the FontInfoNode. */
    
    struct TAvailFonts *taf;
    
  	D(bug("ReadFIN(fh=%p, finode=%p)\n", fh, finode));

    taf = &(finode->TAF);
    
    if (!ReadWord(&DFB(DiskfontBase)->dsh, &(taf->taf_Type), (void *)fh ))
        goto rf_failure;

    if (!ReadWord(&DFB(DiskfontBase)->dsh, &(taf->taf_Attr.tta_YSize), (void *)fh ))
        goto rf_failure;

    if (!ReadByte(&DFB(DiskfontBase)->dsh, &(taf->taf_Attr.tta_Style), (void *)fh ))
        goto rf_failure;
        
    if (!ReadByte(&DFB(DiskfontBase)->dsh, &(taf->taf_Attr.tta_Flags), (void *)fh ))
        goto rf_failure;

    if (!ReadByte(&DFB(DiskfontBase)->dsh, &(finode->Flags), (void *)fh))
        goto rf_failure;
    
    ReturnBool ("ReadFIN", TRUE);
    
rf_failure:
    ReturnBool ("ReadFIN", FALSE);
}

/****************************************************************************************/

BOOL ReadDate(BPTR fh, struct DateStamp *ds, struct DiskfontBase_intern *DiskfontBase)
{
    D(bug("ReadDate(fh=%p, datestamp=%p)\n", fh, ds));

    if (!ReadLong(&DFB(DiskfontBase)->dsh, ((ULONG*)&ds->ds_Days), (void *)fh))
        goto rd_failure;

    if (!ReadLong(&DFB(DiskfontBase)->dsh, ((ULONG*)&ds->ds_Minute), (void *)fh))
        goto rd_failure;

    if (!ReadLong(&DFB(DiskfontBase)->dsh, ((ULONG*)&ds->ds_Tick), (void *)fh))
        goto rd_failure;

    ReturnBool ("ReadDate", TRUE);
    
rd_failure:
    ReturnBool ("ReadDate", FALSE);
}

/****************************************************************************************/

BOOL WriteDate(BPTR fh, struct DateStamp *ds, struct DiskfontBase_intern *DiskfontBase)
{
    D(bug("WriteDate(fh=%p, datestamp=%p)\n", fh, ds));

    if (!WriteLong(&DFB(DiskfontBase)->dsh, ds->ds_Days, (void *)fh))
        goto wd_failure;

    if (!WriteLong(&DFB(DiskfontBase)->dsh, ds->ds_Minute, (void *)fh))
        goto wd_failure;

    if (!WriteLong(&DFB(DiskfontBase)->dsh, ds->ds_Tick, (void *)fh))
        goto wd_failure;

    ReturnBool ("WriteDate", TRUE);
    
wd_failure:
    ReturnBool ("WriteDate", FALSE);
}

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
    if (!ReadWord(&DFB(DiskfontBase)->dsh, &numentries, (void *)fh))
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
            if (!ReadString(&DFB(DiskfontBase)->dsh, &(finode->TAF.taf_Attr.tta_Name), (void *)fh))
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
            if (!ReadWord(&DFB(DiskfontBase)->dsh, &numtags, (void *)fh))
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
    if (!WriteString(&DFB(DiskfontBase)->dsh,CACHE_IDSTR, (void *)fh))
        goto wc_failure;
            
    /* Get the current time */
    DateStamp(&now);
        
    if (!WriteDate(fh, &now, DFB(DiskfontBase) ))
        goto wc_failure;           
    
    /* Leave "empty" space for numentries.numentries is inserted later, 
      when number of entries have bee counted. */
    if (!WriteWord( &DFB(DiskfontBase)->dsh, numentries, (void *)fh))
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
                if (!WriteString(&DFB(DiskfontBase)->dsh, node->TAF.taf_Attr.tta_Name, (void *)fh))
                    goto wc_failure;
            }
            
            /* Write tags if present */ 
        
	    if ( !(flags & FDF_REUSETAGS) )
	    {
        	if ( node->TAF.taf_Attr.tta_Tags )
                    numtags = NumTags(node->TAF.taf_Attr.tta_Tags, DFB(DiskfontBase));
		else
	            numtags = 0;

    		if (!WriteWord( &DFB(DiskfontBase)->dsh, numtags, (void *)fh))
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

    if (!WriteWord(&DFB(DiskfontBase)->dsh, numentries, (void *)fh))
        goto wc_failure;
        
    Close(fh);
    
    ReturnBool ("WriteCache", TRUE);
    
    
wc_failure:

    if (fh)
        Close(fh);
        
    ReturnBool ("WriteCache", FALSE);

}

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
        if (ReadString(&DFB(DiskfontBase)->dsh, &idstr, (void *)fh))
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
	    
        } /* if (ReadString(&DFB(DiskfontBase)->dsh, &idstr, (void *)fh)) */
        Close(fh);
	
    } /* if ((fh = Open(CACHE_FILE,MODE_OLDFILE)) != 0) */
    
    ReturnBool ("OKToReadCache", retval && cacheok);
}

/****************************************************************************************/

#endif

/****************************************************************************************/
