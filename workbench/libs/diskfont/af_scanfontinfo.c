/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Functions for calling the different hooks and getting font descriptions
    Lang: English.
*/


#include "diskfont_intern.h"


/* The array of hooks is in diskfont_init.c */
extern struct AFHookDescr hdescr[];




BOOL ScanFontInfo
(   
    ULONG           userflags,
    struct MinList  *fontinfolist,
    struct MinList  *fontnamelist,
    struct MinList  *fonttagslist,
    struct DiskfontBase_intern *DiskfontBase
)
{
    BOOL  success = TRUE;
    
    WORD  sfi_state;
    
    WORD  retval;

	UWORD idx;
	
    struct AFHookDescr      *afhd;
    struct FontInfoNode     *finode;
    
    struct FontHookCommand  fhc;

    fhc.fhc_Flags = userflags;

    /* Execute all fitting hooks.*/
    
    
    for (idx = 0; idx < NUMFONTHOOKS; idx ++)
    {
    	afhd = &DFB(DiskfontBase)->hdescr[idx];
    	
        /* Call this hook only if it has the flags input by the user */
        if (afhd->ahd_Flags == ( afhd->ahd_Flags & userflags ) )
        {
            /* Ask the hook to initialize itself */
            fhc.fhc_Command = FHC_INIT;
  
            /* If the initialization of this hook fails, we continue with the next */             
            if
            (
                CallHookPkt
                (
                    &afhd->ahd_Hook,
                    &fhc,
                    DFB(DiskfontBase)
                )
            )
            {
                
                /* Read all the fonts available through the hook. Update command type */
                fhc.fhc_Command = FHC_READFONTINFO;
                
                /* Start state */
                sfi_state = SFI_NEWDESCRNODE;
                
                do
                {
                    switch (sfi_state)
                    {
                        case SFI_NEWDESCRNODE:
                
                            /* Allocate a new FontInfoNode */
                            if 
                            (
                                !(finode = AllocMem
                                (
                                    sizeof( struct FontInfoNode ),
                                    MEMF_ANY|MEMF_CLEAR
                                ))
                            )
                            {
                                success = FALSE;
                                break;
                            }
                            else
                            {
                                /* Add the node */
                                AddTail
                                (
                                    (struct List*)fontinfolist,
                                    (struct Node*)finode
                                );                                
            
                                /* Insert the node in the command structure */
                                fhc.fhc_FINode = finode;
                            }             

                        /* Beware that there are no break here, because
                          READDESCR should be executed if NEWDESCRNODE is
                         */
                            
                        case SFI_READDESCR:
              
                    
                            /* Read the font description */
                            retval = CallHookPkt
                            (
                                &afhd->ahd_Hook,
                                &fhc,
                                DFB(DiskfontBase)
                            );
                            
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
                            else
                                /* Elsewise start allocate a new fontinfonode */
                                sfi_state = SFI_NEWDESCRNODE;
                            
                            /* Are we finished scanning ? */
                            if (retval & FH_SCANFINISHED)
                            {
                                /* Remove and deallocate the last node */
                                Remove( (struct Node*)finode );
                                
                                FreeMem
                                (
                                    finode,
                                    sizeof (struct FontInfoNode)
                                );
                                break;
                            }
                            
                            /* Everything went OK */
                        
                            /* Should we reuse an old fontname ? If not, then add
                            this one to the list. 
                            */
                            
                            if ( !(retval & FH_REUSENAME) )
                                AddTail
                                (
                                    (struct List*)fontnamelist,
                                    (struct Node*)finode->FontName
                                );
                            else
                            {
                                /* Reuse the previous node */
                                finode->FontName = FIN( ((struct MinNode*)finode)->mln_Pred)->FontName;
                                
                                /* This flag is used by the cache */
                                finode->Flags |= FDF_REUSENAME;
                            } 
                            
                            /* Likewise with tags */
                            if (userflags & AFF_TAGGED)
                            {
                                
                                if ( !(retval & FH_REUSETAGS) )
                                {
                                    /* Any tags allocated for this node ? */
                                    if ( finode->FontTags )
                                    {
                                        AddTail
                                        (
                                            (struct List*)fonttagslist,
                                            (struct Node*)finode->FontTags
                                        );
                                    }
                                    else
                                    {
                                        /* This flag is used by the cache */
                                        finode->Flags |= FDF_USEDEFTAGS;
                                    }
                                }
                                else
                                {
                                    /* Reuse the previous node */
                                    finode->FontTags = FIN( ((struct MinNode*)finode)->mln_Pred)->FontTags;
                                    
                                    /* This flag is used by the cache */
                                    finode->Flags |= FDF_REUSETAGS;
                                }

                            }
                            break;
                    }
                
                }
                /* Do while successfull and not yet finished scanning */
                while (success && !(retval & FH_SCANFINISHED) );

                /* Ask the hook to clean up */
                fhc.fhc_Command = FHC_CLEANUP;
      
                CallHookPkt
                (
                    &afhd->ahd_Hook,
                    &fhc,
                    DFB(DiskfontBase)
                );
            }
        }
	}  /* for (idx = 0; idx < NUMFONTHOOKS; idx ++) */
    
    return (success);
}


