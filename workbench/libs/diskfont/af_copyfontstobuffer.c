/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Funcs for copying the list in AvailFonts to the buffer.
    Lang: English.
*/


#include "diskfont_intern.h"



BOOL CopyDescrToBuffer
(
	UBYTE *buffer, 
	ULONG buflen, 
	ULONG flags, 
	struct AF_Lists *lists, 
	struct DiskfontBase_intern *DiskfontBase
)
{
    APTR bufend,
         nextptr;
         
    UBYTE *bufptr;
    
    APTR node;
    
    struct TAvailFonts *taf;
    
    UWORD len,
          curstate,
          numentries = 0;

    struct TagItem default_taglist = {TAG_DONE, 0L};
    
    bufptr = buffer;
    
    /* Get a pointer to the end of the buffer */
    bufend = bufptr + buflen;
      
    /* Go past the AvailFontsHeader */
    bufptr = UB(&AFH(bufptr)[1]);
    
    
    /* Write the array of (T)AvailFonts */

    curstate = BFSTATE_FONTINFO;
    
    ForeachNode( &(lists->FontInfoList), node)
    {
        /* Get pointer to next structure */
        nextptr = flags & AFF_TAGGED ? (APTR)&TAVF(bufptr)[1] : (APTR)&AVF(bufptr)[1];
        
        /* Check if there is place for one more element */
        if (nextptr > bufend)
            goto bufferfull;
        
        /* Copy stuff into the buffer */
        taf = &(FIN(node)->TAF);
    
        /* 
          AvailFonts and TAVailFonts are stored similarly in memory except for
          the last taf_TagList field in TAvailFonts
        */
        
        TAVF(bufptr)->taf_Type = taf->taf_Type;
      
        /* We do not write fontname pointer because
        it is not yet copied to the buffer. */
                
        TAVF(bufptr)->taf_Attr.tta_YSize = taf->taf_Attr.tta_YSize;
        TAVF(bufptr)->taf_Attr.tta_Style = taf->taf_Attr.tta_Style;
        TAVF(bufptr)->taf_Attr.tta_Flags = taf->taf_Attr.tta_Flags;
      
        /* We do not write evt. taglist pointer, because
        it is not yet copied to the buffer. */
        
        /* Count number of entries */
        numentries ++;
        
        
        bufptr = nextptr;
    }
    
    

    /* ------------ */
    
    
    /* Write the tags into the buffer */
    if (flags & AFF_TAGGED)
    {

        curstate = BFSTATE_FONTTAGS;
        
        /* !!! Here we should do alignment !!! */   
        ForeachNode( &(lists->FontTagsList), node)
        {
            
            /* Count the number of tags to copy. */
            nextptr = &TI(bufptr)[ NumTags(FTN(node)->TagList, DFB(DiskfontBase)) ];
    
            if (nextptr > bufend)
                goto bufferfull;
            
            /* Copy the tags */
            CopyTagItems( TI(bufptr), FTN(node)->TagList, DFB(DiskfontBase) );
            
            /* Remember where we put them */
            FTN(node)->TagListInBuf = TI(bufptr);
            
            bufptr = nextptr;
        }

        /* ------------ */
        
        /* Move the default tags into the buffer */
        
        curstate = BFSTATE_DEFAULTTAGS;
        nextptr = &TI(bufptr)[1];
        
        if (nextptr > bufend)
            goto bufferfull;
            
        CopyTagItems( TI(bufptr), &default_taglist, DFB(DiskfontBase) );
        
        /* Save the position in the buffer */
        lists->DefTagsInBuf = TI(bufptr);
        
        bufptr = nextptr;
    } 

    /* ------------ */

    /* Write the fontnames into the buffer */

    curstate = BFSTATE_FONTNAME;
    
    /* !!! Here we should do alignment stuff !!! */
    ForeachNode( &(lists->FontNameList), node )
    {

        
        len = strlen (FNN(node)->FontName) +1;
        UB(nextptr) += len;
        
        if (nextptr > bufend)
            goto bufferfull;
        
        /* Write the string into the buffer */
        strcpy( bufptr, FNN(node)->FontName);
        
        /* 
          Remember where we put it, so we can update the (T)AvailFonts structs
          later on.
        */
        
        FNN(node)->FontNameInBuf = bufptr;
        
        bufptr = nextptr;
    }
        
    /* Everything went OK. Insert number of entries in the buffer */
    
    AFH(buffer)->afh_NumEntries = numentries;
    
    return (TRUE);

/* Not enough place in the buffer */
bufferfull:
    lists->BufferFullNode   = node;
    lists->BufferFullState  = curstate;
    lists->BufferFullPtr    = bufptr;
    
    return (FALSE);
}


/********************/
/* CountBytesNeeded */
/********************/

ULONG CountBytesNeeded
(
	UBYTE *bufend, 
	ULONG flags, 
	struct AF_Lists *lists, 
	struct DiskfontBase_intern *DiskfontBase
)
{
    struct MinNode *node;
    /* What state were we inn when the buffer was filled ? */
    
    UBYTE *bufptr;    

    /* Get a pointer to where the buffer got filled */
    bufptr = lists->BufferFullPtr;
    
    switch (lists->BufferFullState)
    {
        case BFSTATE_FONTINFO:
            ForeachNode( &(lists->FontInfoList), node)
                bufptr = flags & AFF_TAGGED ? (UBYTE *)&TAVF(bufptr)[1] : (UBYTE *)&AVF(bufptr)[1];

            /* There is no break here, so that we continue with the strings */

  
        case BFSTATE_FONTTAGS:
          /* !!! Here we SHOULD do alignment !!! */
          
            ForeachNode( &(lists->FontTagsList), node)
                bufptr = (UBYTE*)&TI(bufptr)[ NumTags( FTN(node)->TagList, DFB(DiskfontBase)) ];
              
        case BFSTATE_DEFAULTTAGS:
            bufptr = (UBYTE*)&TI(bufptr)[1];

    
        case BFSTATE_FONTNAME:
            /* !!! Here we SHOULD do alignment !!! */
            
            ForeachNode( &(lists->FontNameList), node)
                bufptr += strlen(FNN(node)->FontName) + 1;
                
    }

    return ( (ULONG)(bufptr - bufend) );
} 

/******************/
/* UpdatePointers */
/******************/

VOID UpdatePointers
(
	UBYTE *bufptr, 
	ULONG flags, 
	struct AF_Lists *lists, 
	struct DiskfontBase_intern *DiskfontBase
)
{
    UWORD numentries;
    
    struct MinNode *node;
    
    /* Update the pointers in the (T)Availfonts elements */
    numentries = AFH(bufptr)->afh_NumEntries;
    
    /* Skip the AvailFontsHeader */
    bufptr = (UBYTE *)&AFH(bufptr)[1];
    
    /* Walk down the FontInfoList too */
    node = lists->FontInfoList.mlh_Head;
    
    for (; numentries --; )
    {
        /* Update the fontname */
        TAVF(bufptr)->taf_Attr.tta_Name = FIN(node)->FontName->FontNameInBuf;
        
        if (flags & AFF_TAGGED)
        {
            /* Either use the default emty taglist, or a supplied one */
            if (!FIN(node)->FontTags)
                TAVF(bufptr)->taf_Attr.tta_Tags = lists->DefTagsInBuf;
            else
                TAVF(bufptr)->taf_Attr.tta_Tags = FIN(node)->FontTags->TagListInBuf;
        
            bufptr = (UBYTE *)&TAVF(bufptr)[1];
        }
        else
            bufptr = (UBYTE *)&AVF(bufptr)[1];
            
        node = node->mln_Succ;

    }

    return; 
}
