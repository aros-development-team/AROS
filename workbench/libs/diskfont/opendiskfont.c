/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Diskfont function OpenDiskFont()
    Lang: english
*/

#ifndef TURN_OFF_DEBUG
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#endif
#  include <aros/debug.h>

#include <proto/utility.h>
#include <proto/graphics.h>
#include "diskfont_intern.h"


/*****************************************************************************

    NAME */
#include <clib/diskfont_protos.h>

	AROS_LH1(struct TextFont *, OpenDiskFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextAttr *, textAttr, A0),

/*  LOCATION */
	struct Library *, DiskfontBase, 5, Diskfont)

/*  FUNCTION
		Tries to open the font specified by textAttr. If the font has allready
		been loaded into memory, it will be opened with OpenFont(). Otherwise
		OpenDiskFont() will try to load it from disk.

    INPUTS
    	textAttr - Description of the font to load. If the textAttr->ta_Style
    			   FSF_TAGGED bit is set, it will be treated as a struct TTextAttr.
    	

    RESULT
    	Pointer to a struct TextFont on success, 0 on failure.

    NOTES

    EXAMPLE

    BUGS
    	Loadseg_AOS() which is internally used to load a font file
    	from disk, does not handle address relocation for 64 bit CPUs
    	yet. Will add a hack to support this.

    SEE ALSO
    	AvailFonts()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    diskfont_lib.fd and clib/diskfont_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,DiskfontBase)
 	
    WORD 			match_weight = 0, new_match_weight;
    struct FontHookCommand 	fhc;
    struct Hook 		*hook, *bestmatch_hook = NULL;
    struct TTextAttr 		best_so_far = {0}, *destattr;
    struct TextFont 		*tf = 0;
    UWORD 			idx;

    BOOL 			finished = FALSE;
    BOOL 			matchfound = FALSE;

    D(bug("OpenDiskFont(textAttr=%p)\n", textAttr));

    /* PerfectMatch info should be passed to hook */

    fhc.fhc_ReqAttr = (struct TTextAttr*)textAttr;
    destattr = &(fhc.fhc_DestTAttr);

    /* Ask the hooks for matching textattrs */

    for (idx = 0; (idx < NUMFONTHOOKS) && !finished; idx ++)
    {
	ULONG retval;

	hook = &(DFB(DiskfontBase)->hdescr[idx].ahd_Hook);

	/* Initalize the hook */
	fhc.fhc_Command = FHC_ODF_INIT;

	if ( !CallHookPkt(hook, &fhc, DFB(DiskfontBase) ))
	    continue; /* Go on with next hook */

	/* Ask the hook for info on a font */
	fhc.fhc_Command = FHC_ODF_GETMATCHINFO;

	for (;;)
	{				

	    /* Reset tags field in case the hook doesn't do it */
	    destattr->tta_Tags = 0;

	    retval = CallHookPkt(hook, &fhc, DFB(DiskfontBase));

	    /* Error or finished scanning ? */
	    if (!retval || (retval & FH_SCANFINISHED))
		break;

	    /* WeightTAMatch does not compare the fontnames, so we do it here */
	    if ( strcmp(textAttr->ta_Name, destattr->tta_Name) != 0 )
		continue;

	    matchfound = TRUE;

	    /* How well does this font match ? */
	    new_match_weight = WeighTAMatch(textAttr, (struct TextAttr *)destattr, destattr->tta_Tags);

	    D(bug("New matchweight: %d", new_match_weight));					

	    /* Better match found ? */

	    if (new_match_weight > match_weight)
	    {
	    	match_weight = new_match_weight;
		
		if (best_so_far.tta_Tags)
		    FreeTagItems(best_so_far.tta_Tags);

		best_so_far = *destattr;
		if (best_so_far.tta_Tags)
		{
		    best_so_far.tta_Tags = CloneTagItems(best_so_far.tta_Tags);
		}
		bestmatch_hook 	= hook;

		/* Perfect match found ? */
		if (new_match_weight == MAXFONTMATCHWEIGHT)
		{
		    finished = TRUE;
		    D(bug("\tPerfect match\n"));
		    break;
		}

	    }

	} /* for (;;) */ 

	/* Tell the hook to cleanup */
	fhc.fhc_Command = FHC_ODF_CLEANUP;
	CallHookPkt(hook, &fhc, DFB(DiskfontBase) );


    } /* for ( iterate hooktable ) */

    if (matchfound)
    {

	/* Open the font */
	fhc.fhc_Command = FHC_ODF_OPENFONT;
	fhc.fhc_ReqAttr = &best_so_far;

	/* We cannot used the name returned by the hook earlier since it has
	been freed during FHC_ODF_CLEANUP */
	best_so_far.tta_Name = textAttr->ta_Name;

	CallHookPkt(bestmatch_hook, &fhc, DFB(DiskfontBase) );
	tf = fhc.fhc_TextFont;

	ExtendFont(tf, NULL);
    }

    if (best_so_far.tta_Tags) FreeTagItems(best_so_far.tta_Tags);

    ReturnPtr("OpenDiskFont", struct TextFont *, tf);
	
    AROS_LIBFUNC_EXIT
    
} /* OpenDiskFont */
