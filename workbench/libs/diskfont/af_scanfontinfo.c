/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Functions for calling the different hooks and getting font descriptions

    Lang: English.
*/

/****************************************************************************************/

#include <proto/utility.h>
#include <string.h>
#include "diskfont_intern.h"

#ifndef TURN_OFF_DEBUG
#define DEBUG 0
#endif

#include <aros/debug.h>

/****************************************************************************************/

/* The array of hooks is in diskfont_init.c */
extern struct AFHookDescr hdescr[];

/****************************************************************************************/

/* Some states in the ScanFontInfo routine */
#define SFI_NEWDESCRNODE  0
#define SFI_READDESCR     1

/****************************************************************************************/


BOOL ScanFontInfo
(   
    ULONG           		userflags,
    struct MinList  		*filist,
    struct DiskfontBase_intern 	*DiskfontBase
)
{

    ULONG retval;
    UWORD idx;

    struct AFHookDescr      *afhd;
    struct FontInfoNode     *finode = NULL;
    struct FontHookCommand  fhc = {0};

    struct TTextAttr *srcattr, *destattr = NULL;
    UWORD sfi_state;

    BOOL  success	= TRUE;

    D(bug("ScanFontInfo(userflags=%d, filist=%p)\n", userflags,	filist));


    /* The hook's deatination TextAttr becomes our source TextAttr */
    srcattr = &(fhc.fhc_DestTAttr);

    /* Execute all fitting hooks.*/

    for (idx = 0; idx < NUMFONTHOOKS; idx ++)
    {
	BOOL  done = FALSE;

	afhd = &DFB(DiskfontBase)->hdescr[idx];

	/* Call this hook only if it has the flags input by the user */

    #if 1
	if ((afhd->ahd_Flags & AFF_MEMORY) && !(userflags & AFF_MEMORY)) continue;
	if ((afhd->ahd_Flags & AFF_DISK) && !(userflags & AFF_DISK)) continue;
    #else    
	if (afhd->ahd_Flags != ( afhd->ahd_Flags & userflags ) )
	    continue;
    #endif

	/* Ask the hook to initialize itself */
	fhc.fhc_Command = FHC_AF_INIT;

       /* If the initialization of this hook fails, we continue with the next hook */
	if ( !CallHookPkt( &afhd->ahd_Hook, &fhc, DFB(DiskfontBase) ))
	    continue;

	/* Read all the fonts available through the hook. Update command type */
	fhc.fhc_Command = FHC_AF_READFONTINFO;

	sfi_state = SFI_NEWDESCRNODE;
	do
	{

	    switch (sfi_state)
	    {
       		case SFI_NEWDESCRNODE:
        	    /* Allocate a new FontInfoNode */
        	    if ( !(finode = AllocFIN(filist, DFB(DiskfontBase) )))
        	    {
		        success = FALSE;
			break;
		    }

        	    destattr = &(finode->TAF.taf_Attr);

        	    /* Note that there are no break here, because
        	       READDESCR should be executed if NEWDESCRNODE is
        	    */

        	case SFI_READDESCR:
        	    /* Reset the DestTExtattr to all 0s */		
        	    memset(&(fhc.fhc_DestTAttr), 0, sizeof (struct TTextAttr));

        	    /* Read the font description */
        	    retval = CallHookPkt( &afhd->ahd_Hook, &fhc, DFB(DiskfontBase) );

        	    /* If FALSE, then there is a fatal error */
        	    if (!retval)
        	    {
			success = FALSE;
			break;
		    }


        	    /* Is there a non-fatal error ? If so reuse the fontinfonode */
        	    if (retval & FH_SINGLEERROR)
        	    {
		        sfi_state = SFI_READDESCR;
			break;
		    }

        	    /* Otherwise start allocate a new fontinfonode */
        	    sfi_state = SFI_NEWDESCRNODE;

        	    /* Are we finished scanning ? */
        	    if (retval & FH_SCANFINISHED)
        	    {
        		FreeFIN(finode, DFB(DiskfontBase) );
        		done = TRUE;
        		break;
        	    }

        	    /* If we don't want scaled fonts and the font isn't designed,
        	       then skip it
        	    */
        	    if ( !(userflags & AFF_SCALED) && !(srcattr->tta_Flags & FPF_DESIGNED))
        	    {
		        sfi_state = SFI_READDESCR;
			break;
		    }

        	    /* Everything went OK, now process the info we read */

        	    destattr->tta_YSize = srcattr->tta_YSize;
        	    destattr->tta_Style = srcattr->tta_Style;
        	    destattr->tta_Flags = srcattr->tta_Flags;


        	    /* Set AvailFonts type */
        	    finode->TAF.taf_Type =
        	        (destattr->tta_Flags & FPF_DESIGNED ? 0 : AFF_SCALED) | afhd->ahd_Flags;


        	    /* Should we reuse last fontname ? */

        	    if (retval & FH_REUSENAME)
        	        finode->Flags |= FDF_REUSENAME;
        	    else
        	    {
        		/* Copy fontname */

        		finode->NameLength = strlen(srcattr->tta_Name) + 1;

        		destattr->tta_Name = AllocVec( finode->NameLength, MEMF_ANY );
        		if (!destattr->tta_Name)
        		{
			    success = FALSE;
			    break;
			}

        		strcpy(destattr->tta_Name, srcattr->tta_Name);
        	    }

        	    /* Likewise with tags */
        	    if (userflags & AFF_TAGGED)
        	    {
        		if (retval & FH_REUSETAGS)
        		    finode->Flags |= FDF_REUSETAGS;
        		else
        		{
        		    if (srcattr->tta_Tags)
        		    {
                		destattr->tta_Tags = CloneTagItems(srcattr->tta_Tags);
                		if (!destattr->tta_Tags)
                		{
				    success = FALSE;
				    break;
				}

                		/* Count number of tags for later */
                		finode->NumTags = NumTags(destattr->tta_Tags, DFB(DiskfontBase) );
        		    }
        		}
        	    }
		  
        	break;

	    } /* switch (sfi_state) */

	    /* Do while successfull and not yet finished scanning */
	}
	while (success && !done);

	/* Tell the hook to clean up */
	fhc.fhc_Command = FHC_AF_CLEANUP;

	CallHookPkt( &afhd->ahd_Hook, &fhc, DFB(DiskfontBase) );


    }  /* for (idx = 0; idx < NUMFONTHOOKS; idx ++) */

    ReturnBool ("ScanFontInfo", success);
}

/****************************************************************************************/

