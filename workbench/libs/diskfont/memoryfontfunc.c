/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hook for getting font descriptions from memory font list in mem
    Lang: English.
*/

#include <proto/graphics.h>
#include <graphics/text.h>
#include <graphics/gfxbase.h>
#include "diskfont_intern.h"

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

/* Userdata needed by the MemoryFontHook */
struct MFHData
{
    /* Pointer to the current font in the memory font list */
    struct TextFont *CurrentFont;
};

/****************************************************************************************/

/* Hook for reading fonts from memory */

/****************************************************************************************/

AROS_UFH3(IPTR, MemoryFontFunc,
    AROS_UFHA(struct Hook *,				h, 				A0),
    AROS_UFHA(struct FontHookCommand *,		fhc,			A2),
    AROS_UFHA(struct DiskfontBase_intern *,	DiskfontBase,	A1)
)
{
    AROS_USERFUNC_INIT

    /* Note: FALSE is default */
    ULONG retval = FALSE;
    
    struct MFHData  *mfhd;
    struct TextFont             *curfont;

    struct TTextAttr *tattr = &(fhc->fhc_DestTAttr);
    D(bug("MemoryFontFunc(hook=%p, fhc=%p)\n", h, fhc));
        
    /* What command do we have */
    
    switch (fhc->fhc_Command)
    {
    	case FHC_ODF_INIT:
        case FHC_AF_INIT:
        
            /* Allocate user data */
            if ((mfhd = AllocMem( sizeof (struct MFHData), MEMF_ANY )) != 0 )
            {
            	/* GFX library allready open */
 			
 		/* To prevent race conditions */
		Forbid();
					
                /* Get the first font */
                mfhd->CurrentFont = (struct TextFont*)DFB(DiskfontBase)->gfxbase->TextFonts.lh_Head;
                    
                /* Insert the userdata into the hookcommand struct */
                fhc->fhc_UserData = mfhd;

                retval = FH_SUCCESS;
            }         
            break;
            
        /* ---------------------- */            
        
        case FHC_ODF_GETMATCHINFO:
        case FHC_AF_READFONTINFO:
			 
            /* Get userdata */
            mfhd = fhc->fhc_UserData;
            
            /* Get current font */
            curfont = mfhd->CurrentFont;
           
            /* Get a pointer to the next font. Are we at the end of the list ? */
            if (!(mfhd->CurrentFont = (struct TextFont*)curfont->tf_Message.mn_Node.ln_Succ))
            {
	        retval |= FH_SCANFINISHED;
		break;
	    }
            
            /* Insert font info into the supplied tattr */
            tattr->tta_Tags = NULL; /* Defaults to NULL */
            
	    tattr->tta_Name =  curfont->tf_Message.mn_Node.ln_Name;
            tattr->tta_YSize = curfont->tf_YSize;
            tattr->tta_Style = curfont->tf_Style;
            tattr->tta_Flags = curfont->tf_Flags;
        
            /* Does this font have an exstension structure ? */ 
            if (ExtendFont(curfont, 0L))
            	tattr->tta_Tags = TFE(curfont->tf_Extension)->tfe_Tags;
            

	    retval |= FH_SUCCESS;
            break;

        /* ---------------------- */                        
        
        case FHC_ODF_CLEANUP:
        case FHC_AF_CLEANUP:
            /* Danger for race conditions over */
            Permit();
        
            /* Free the userdata */
            FreeMem(fhc->fhc_UserData, sizeof (struct MFHData));
            
            /* FHC_CLEANUP is never allowed to fail */
            retval = FH_SUCCESS;
            
            break;
        
        case FHC_ODF_OPENFONT:
            fhc->fhc_TextFont = OpenFont((struct TextAttr*)fhc->fhc_ReqAttr);
    	    retval = (fhc->fhc_TextFont != NULL);
    	    break;
	    
    }
    
    ReturnInt ("MemoryFontFunc", ULONG, retval);

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/
