/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Hook for getting font descriptions from memory font list in mem
    Lang: English.
*/


#include "diskfont_intern.h"

/* Hook for reading fonts from memory */

IPTR MemoryFontFunc
(
    struct Hook             *h,
    struct FontHookCommand  *fhc,
    struct DiskfontBase_intern *DiskfontBase
)
{
    /* Note: FALSE is default */
    ULONG retval = FALSE;
    
    struct MemoryFontHook_Data  *mfhd;
    struct TextFont             *curfont;
    struct FontInfoNode         *fontinfonode;
    struct TAvailFonts          *taf;
    
        
    /* What command do we have */
    
    switch (fhc->fhc_Command)
    {
        case FHC_INIT:
        
            /* Allocate user data */
            if ((mfhd = AllocMem( sizeof (struct MemoryFontHook_Data), MEMF_ANY )) != 0 )
            {
            	/* GFX library allready open */
 			
 				/* To prevent race conditions */
				Forbid();
					
                /* Get the first font */
                mfhd->mfhd_CurrentFont = (struct TextFont*)DFB(DiskfontBase)->gfxbase->TextFonts.lh_Head;
                    
                /* Insert the userdata into the hookcommand struct */
                fhc->fhc_UserData = mfhd;

                retval = FH_SUCCESS;
            }
          
            break;
            
        /* ---------------------- */            
        
        case FHC_READFONTINFO:

            /* Get userdata */
            mfhd = fhc->fhc_UserData;
            
            /* Get current font */
            curfont = mfhd->mfhd_CurrentFont;
            
            /* Get the fontinfonode */
            fontinfonode = fhc->fhc_FINode;
            
            /* Get a pointer to the next font. Are we at the end of the list ? */
            if (!(mfhd->mfhd_CurrentFont = (struct TextFont*)curfont->tf_Message.mn_Node.ln_Succ))
            {
                /* We are finished scanning */
                retval |= FH_SCANFINISHED|FH_SUCCESS;
                break;
            }
          
            /* If the font is not designed and scaled fonts
              are not wanted (AFF_SCALED not specified), then exit */
            if 
            ( 
                !(
                    (fhc->fhc_Flags     & AFF_SCALED    ) 
                ||  
                    (curfont->tf_Flags  & FPF_DESIGNED  ) 
                )
            )
            {
                retval = FH_SINGLEERROR;  break;
            }
  
        
            /* Allocte a node to hold this fontname, and put a pointer
            to it into the FontInfoNode
            */
            
            if (!( 
                fontinfonode->FontName = AllocFontNameNode
                (
                    curfont->tf_Message.mn_Node.ln_Name ,
                    DFB(DiskfontBase)
                )
            ))
            {
                retval = FALSE; break;
            }
            
            taf = &(fontinfonode->TAF);
          
            /* Put stuff into structure */
                
            /* Mask of unrelevant flags (AFF_DISK etc. ) */   
            taf->taf_Type = fhc->fhc_Flags & (AFF_MEMORY | AFF_SCALED );
              
            /* Insert font info into the taf in the fontinfonode */
            taf->taf_Attr.tta_Tags = NULL; /* Defaults to NULL */
              
            taf->taf_Attr.tta_YSize = curfont->tf_YSize;
            taf->taf_Attr.tta_Style = curfont->tf_Style;
            taf->taf_Attr.tta_Flags = curfont->tf_Flags;
            
            retval |= FH_SUCCESS;
                
            /* Take special care with tagged fonts */
            if (fhc->fhc_Flags & AFF_TAGGED)
            {

                /* Does this font have an exstension structure ? */ 
                if (ExtendFont(curfont, 0L))
                {
                    /* Allocate node to hold the taglist extension */
                    if 
                    (!(
                        fontinfonode->FontTags = AllocFontTagsNode
                        (
                            TFE(curfont->tf_Extension)->tfe_Tags,
                        	DFB(DiskfontBase)
                        )
                    ))
                    {
                        retval = FALSE; break;
                    }
                }
            }

            break;

        /* ---------------------- */                        
        
        case FHC_CLEANUP:
            /* Danger for race conditions over */
            Permit();
        
            /* Free the userdata */
            FreeMem
            (
                fhc->fhc_UserData,
                sizeof (struct MemoryFontHook_Data)
            );
            
            /* This is never allowed to fail */
            retval = FH_SUCCESS;
            
            break;
        
    }
    
    return ((IPTR)retval);
}
    
   