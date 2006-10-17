/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>

#include <string.h>

#include "locale.h"
#include "wpeditor.h"


/*** Instance Data **********************************************************/
struct WPEditor_DATA
{
    Object *wped_WorkbenchPI,
           *wped_DrawersPI; 
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct WPEditor_DATA *data = INST_DATA(CLASS, self)


/*** Methods ****************************************************************/
Object *WPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct WPEditor_DATA *data;
    Object               *workbenchPI, *drawersPI;
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_PrefsEditor_Name,        __(MSG_NAME),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/Wanderer.prefs",
        
        Child, (IPTR) ColGroup(2),
            GroupFrameT(_(MSG_BACKGROUNDS)),
            MUIA_Group_SameSize, TRUE,
            
            Child, (IPTR) (workbenchPI = PopimageObject,
                MUIA_Window_Title,     __(MSG_SELECT_WORKBENCH_BACKGROUND),
                MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
                MUIA_CycleChain,       1,
            End),
            Child, (IPTR) (drawersPI = PopimageObject,
                MUIA_Window_Title,     __(MSG_SELECT_DRAWER_BACKGROUND),
                MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
                MUIA_CycleChain,       1,
            End),
            Child, (IPTR) CLabel(_(MSG_BACKGROUND_WORKBENCH)),
            Child, (IPTR) CLabel(_(MSG_BACKGROUND_DRAWERS)),
        End,
        
        TAG_DONE
    );
    
    if (self != NULL)
    {
        data = INST_DATA(CLASS, self);
        data->wped_WorkbenchPI = workbenchPI;
        data->wped_DrawersPI   = drawersPI;
        
        /*-- Setup notifications -------------------------------------------*/
        DoMethod
        (
            workbenchPI, MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            drawersPI, MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
    }
    
    return self;
}


IPTR WPEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{

    SETUP_INST_DATA;
    
    struct ContextNode     *context;
    struct IFFHandle       *handle;
    struct WandererPrefs    wpd;    
    BOOL                    success = TRUE;
    LONG                    error;


                 
    if (!(handle = AllocIFF()))
        return FALSE;
    
    handle->iff_Stream = (IPTR) message->fh;
    InitIFFasDOS(handle);

    if ((error = OpenIFF(handle, IFFF_READ)) == 0)
    {
	
        BYTE i;
        
        // FIXME: We want some sanity checking here!
        for (i = 0; i < 1; i++)
        {
            if ((error = StopChunk(handle, ID_PREF, ID_WANDR)) == 0)
            {
                if ((error = ParseIFF(handle, IFFPARSE_SCAN)) == 0)
                {
                    context = CurrentChunk(handle);
                    
                    error = ReadChunkBytes
                    (
                        handle, &wpd, sizeof(struct WandererPrefs)
                    );
                    
                    if (error < 0)
                    {
                        Printf("Error: ReadChunkBytes() returned %ld!\n", error);
                    }                    
                }
                else
                {
                    Printf("ParseIFF() failed, returncode %ld!\n", error);
                    success = FALSE;
                    break;
                }
            }
            else
            {
                Printf("StopChunk() failed, returncode %ld!\n", error);
                success = FALSE;
            }
        }

        CloseIFF(handle);
    }
    else
    {
        //ShowError(_(MSG_CANT_OPEN_STREAM));
    }

    FreeIFF(handle);
    
    
    if (success)
    {
        //SMPByteSwap(&wpd);

	    NNSET(data->wped_WorkbenchPI, MUIA_Imagedisplay_Spec, (STRPTR)wpd.wpd_WorkbenchBackground);
        NNSET(data->wped_DrawersPI, MUIA_Imagedisplay_Spec, (STRPTR)wpd.wpd_DrawerBackground);
            
    }
    
    return success;
}


IPTR WPEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    
    struct PrefHeader header = { 0 }; 
    struct IFFHandle *handle;
    BOOL              success = TRUE;
    LONG              error   = 0;
        
    if ((handle = AllocIFF()))
    {
        handle->iff_Stream = (IPTR) message->fh;
        
        InitIFFasDOS(handle);
        
        if (!(error = OpenIFF(handle, IFFF_WRITE))) /* NULL = successful! */
        {
            struct WandererPrefs wpd;    
     	    memset(&wpd, 0, sizeof(wpd));
                
    	    BYTE i;
            
            PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */
            
            header.ph_Version = PHV_CURRENT;
            header.ph_Type    = 0;
            
            PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */
            
            WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));
            
            PopChunk(handle);     
            
            for (i = 0; i < 1; i++)
            {
                error = PushChunk(handle, ID_PREF, ID_WANDR, sizeof(struct WandererPrefs));
                
                if (error != 0) // TODO: We need some error checking here!
                {
                    Printf("error: PushChunk() = %ld ", error);
                }
                
                /* */
                STRPTR ws = (STRPTR) XGET(data->wped_WorkbenchPI, MUIA_Imagedisplay_Spec);
                STRPTR ds = (STRPTR) XGET(data->wped_DrawersPI, MUIA_Imagedisplay_Spec);    
                strcpy(wpd.wpd_WorkbenchBackground, ws);   
                strcpy(wpd.wpd_DrawerBackground, ds);                  
                
                /* TODO: fix problems with endianess?? */
		        //SMPByteSwap(&wpd); 
			
                error = WriteChunkBytes(handle, &wpd, sizeof(struct WandererPrefs));
                error = PopChunk(handle);
                                
                if (error != 0) // TODO: We need some error checking here!
                {
                    Printf("error: PopChunk() = %ld ", error);
                }
                
            }        
            
            /* Terminate the FORM */
            PopChunk(handle);   
        }
        else
        {
            //ShowError(_(MSG_CANT_OPEN_STREAM));
            Printf("error: cant open stream!");
            success = FALSE;
        }
        
        CloseIFF(handle);
	    FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        //ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        success = FALSE;
    }

    return success;
}
           
            
/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_3
(
    WPEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                    struct opSet *,
    MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *
);
