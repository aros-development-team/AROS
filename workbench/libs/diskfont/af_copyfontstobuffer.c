/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Funcs for copying the list in AvailFonts to the buffer.
    Lang: English.
*/

/*****************************************************************************/

#include <string.h>

#include "diskfont_intern.h"

#ifndef TURN_OFF_DEBUG
#define DEBUG 0
#endif

#include <aros/debug.h>

/******************************************************************************/

BOOL CopyDescrToBuffer
(
	UBYTE *buffer, 
	ULONG buflen, 
	ULONG flags, 
	struct MinList *filist,
	struct CopyState *cs, 
	struct DiskfontBase_intern *DiskfontBase
)
{
    APTR bufend,
         nextptr = NULL;
         
    UBYTE *bufptr;
    
    struct FontInfoNode *node;
    
    struct TAvailFonts *taf;
    struct TTextAttr *tattr;
    
    UWORD curstate,
          numentries = 0;
    
    STRPTR lastnameinbuf = NULL;


    D(bug(
	  "CopyDescrToBuffer(buffer=%p, buflen=%d, userflags=%d, filist=%p, "
	  "copystate=%p)\n",
	  buffer, buflen, flags, filist, cs
	  ));
	
    bufptr = buffer;
    
    /* Get a pointer to the end of the buffer */
    bufend = bufptr + buflen;
      
    /* Go past the AvailFontsHeader */
    bufptr = UB(&AFH(bufptr)[1]);
    
    
    /* Write the array of (T)AvailFonts */

    curstate = BFSTATE_FONTINFO;
    
    ForeachNode( filist, node )
    {
        /* Get pointer to next structure */
        nextptr = (flags & AFF_TAGGED) ? (APTR)&TAVF(bufptr)[1] : (APTR)&AVF(bufptr)[1];
        
        /* Check if there is place for one more element */
        if (nextptr > bufend)
            goto bufferfull;
        
        /* Copy stuff into the buffer */
        taf = &(node->TAF);
    
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
    	struct TagItem  *lasttagsinbuf = NULL;
    	
        curstate = BFSTATE_FONTTAGS;
        
        /* !!! Here we should do alignment !!! */   
        ForeachNode( filist, node)
        {
            tattr = &(node->TAF.taf_Attr);

	    /* Reuse tags ? */
	    if ( node->Flags & FDF_REUSETAGS )
	    {
		node->TagsInBuf = lasttagsinbuf;
		node->NumTags = ((struct FontInfoNode *)node->NodeHeader.mln_Pred)->NumTags;
	    }
	    else
	    {

		/* Does this node have tags ? */
		if ( tattr->tta_Tags )
		{
    	   	    nextptr = &TI(bufptr)[ node->NumTags ];

		    if (nextptr > bufend)
        		goto bufferfull;
            
            	    /* Copy the tags */
            	    CopyTagItems( TI(bufptr), tattr->tta_Tags, DFB(DiskfontBase) );

        	    node->TagsInBuf = lasttagsinbuf = TI(bufptr);
        	}
            }
            bufptr = nextptr;
        }
    }
	
    /* ------------ */

    /* Write the fontnames into the buffer */

    curstate = BFSTATE_FONTNAME;
    
    /* !!! Here we should do alignment stuff !!! */
    ForeachNode( filist, node )
    {
        tattr = &(node->TAF.taf_Attr);
        
        if (node->Flags & FDF_REUSENAME)
        {
            D(bug("\tReused name\n"));
            node->NameInBuf = lastnameinbuf;
        }
        else
        {
            UB(nextptr) += node->NameLength;
            D(bug("\tCFTB: %s, %d\n", node->TAF.taf_Attr.tta_Name, node->NameLength));

            if (nextptr > bufend)
                goto bufferfull;

            /* Write the string into the buffer */
            strcpy( bufptr, tattr->tta_Name);

            /* 
            Remember where we put it, so we can update the (T)AvailFonts structs
            later on.
       	    */

            node->NameInBuf = bufptr;
            lastnameinbuf   = bufptr;
            D(bug("\tCFTB: Name in buffer %s\n", bufptr));

        }
        bufptr = nextptr;
    }
        
    /* Everything went OK. Insert number of entries in the buffer */
    
    AFH(buffer)->afh_NumEntries = numentries;
    
    ReturnBool ("CopyDescrToBuffer", TRUE);

/* Not enough place in the buffer */
bufferfull:
    cs->BufferFullNode   = node;
    cs->BufferFullState  = curstate;
    cs->BufferFullPtr    = bufptr;
    
    ReturnBool ("CopyDescrToBuffer", FALSE);
}

/******************************************************************************/

/********************/
/* CountBytesNeeded */
/********************/

/******************************************************************************/

ULONG CountBytesNeeded
(
    UBYTE 			*bufend, 
    ULONG 			flags, 
    struct CopyState		*cs,
    struct DiskfontBase_intern	*DiskfontBase
)
{
    /* Macro for going through the rest of nodes in a list */

    #define ForRestOfNodes(node) \
	for (; ((struct Node*)(node))->ln_Succ; \
	    node = (void *)((struct Node*)(node))->ln_Succ )
	
    struct FontInfoNode *node;
    
    UBYTE *bufptr;    

	
    D(bug(
	  "CountBytesNeeded(bufend=%p, userflags=%d, copystate=%p)\n",
	  bufend, flags, cs
	  ));

    /* Get a pointer to where the buffer got filled */
    bufptr = cs->BufferFullPtr;
    
    node = cs->BufferFullNode; 
    
    /* What state were we in when the buffer was filled ? */

    switch (cs->BufferFullState)
    {
        case BFSTATE_FONTINFO:
            ForRestOfNodes( node)
                bufptr = (flags & AFF_TAGGED) ? (UBYTE *)&TAVF(bufptr)[1] : (UBYTE *)&AVF(bufptr)[1];

            /* There is no break here, so that we continue with the strings */
  
        case BFSTATE_FONTTAGS:
          /* !!! Here we SHOULD do alignment !!! */
          
          	/* I finode->NumTags == 0, then bufptr will remain the same */
            ForRestOfNodes( node )
                bufptr = (UBYTE*)&TI(bufptr)[ node->NumTags ];
    		
        case BFSTATE_FONTNAME:
            /* !!! Here we SHOULD do alignment !!! */
            
            ForRestOfNodes( node )
                bufptr += node->NameLength;
                
    }

    ReturnInt ("CountBytesNeeded", ULONG, (bufptr - bufend) );
} 

/******************************************************************************/

/******************/
/* UpdatePointers */
/******************/

/******************************************************************************/

VOID UpdatePointers
(
    UBYTE 			*bufptr, 
    ULONG 			flags, 
    struct MinList 		*filist, 
    struct DiskfontBase_intern 	*DiskfontBase
)
{
    UWORD numentries;
    
    struct FontInfoNode *node;
    
    D(bug(
	  "UpdatePointers(buffer=%p, userflags=%d, filist=%p)\n",
	  bufptr, flags, filist
	 ));

    /* Update the pointers in the (T)Availfonts elements */
    numentries = AFH(bufptr)->afh_NumEntries;
    
    /* Skip the AvailFontsHeader */
    bufptr = (UBYTE *)&AFH(bufptr)[1];
    
    /* Walk down the FontInfoList too */
    node = (struct FontInfoNode*)filist->mlh_Head;
    
    for (; numentries --; )
    {
        /* Update the fontname */
        TAVF(bufptr)->taf_Attr.tta_Name = node->NameInBuf;
        
	D(bug("\tUP: Name:      %s\n", node->TAF.taf_Attr.tta_Name));
        D(bug("\tUP: NameInBuf: %s\n", node->NameInBuf));
        
		/* Update tagptr */
        if (flags & AFF_TAGGED)
        {
            TAVF(bufptr)->taf_Attr.tta_Tags = node->TagsInBuf;
        
            bufptr = (UBYTE *)&TAVF(bufptr)[1];
        }
        else
            bufptr = (UBYTE *)&AVF(bufptr)[1];
            
        node = (struct FontInfoNode*)node->NodeHeader.mln_Succ;

    }

    ReturnVoid("UpdatePointers"); 
}

/******************************************************************************/
