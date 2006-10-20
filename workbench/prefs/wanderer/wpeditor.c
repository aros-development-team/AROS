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
    Object *wped_c_NavigationMethod, *wped_cm_ToolbarEnabled, *wped_toolbarpreviev;
           
};

static struct Hook  	       navichangehook;

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct WPEditor_DATA *data = INST_DATA(CLASS, self)


/*** Methods ****************************************************************/
Object *WPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct WPEditor_DATA *data;
    Object               *workbenchPI, *drawersPI, *c_navitype, *bt_dirup, *bt_search, 
                         *cm_toolbarenabled, *cm_searchenabled, *toolbarpreviev;  
 
 
   
    //char *registerpages[] = {_(MSG_GENERAL),_(MSG_APPEARANCE),_(MSG_TOOLBAR),NULL};
    char *registerpages[] = {"General","Appearance","Toolbar",NULL};
    static STRPTR navigationtypelabels[3];
    
    //navigationtypelabels[WPD_NAVIGATION_CLASSIC] = MSG(MSG_CLASSIC);
    //navigationtypelabels[WPD_NAVIGATION_ENHANCED] = MSG(MSG_ENHANCED);  
    navigationtypelabels[WPD_NAVIGATION_CLASSIC] = "Classic";
    navigationtypelabels[WPD_NAVIGATION_ENHANCED] = "Enhanced";
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_PrefsEditor_Name,        __(MSG_NAME),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/Wanderer.prefs",
        
        Child, (IPTR) (RegisterGroup(registerpages),
            Child, (IPTR) ColGroup(2),                     /* general */
                GroupFrameT(_(MSG_NAVIGATION)),
                MUIA_Group_SameSize, TRUE,
                
                Child, Label1(_(MSG_METHOD)),
                Child, c_navitype = MUI_MakeObject(MUIO_Cycle, NULL, navigationtypelabels),                 
            End,        
            Child, (IPTR) ColGroup(2),                     /* appearance */
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
            Child, (IPTR) ColGroup(2),                     /* toolbar */
            MUIA_Group_Horiz, FALSE,

                 
                 Child, (IPTR) ColGroup(2),
                        GroupFrameT(_(MSG_OBJECTS)),
                        MUIA_Group_SameSize, TRUE,
                
                
                        Child, Label1(MSG_TOOLBAR_ENABLED),
                        Child, cm_toolbarenabled = MUI_MakeObject(MUIO_Checkmark,NULL),

                        //Child, Label1("search"),
                        //Child, cm_searchenabled = MUI_MakeObject(MUIO_Checkmark,NULL),                        
                 End,
                 
                 Child, (IPTR) (toolbarpreviev = ColGroup(2),
                        GroupFrameT(_(MSG_PREVIEW)),
                        //GroupFrameT("Preview"),
                        MUIA_Group_SameSize, TRUE,
        
                        Child, (IPTR) (GroupObject,
                            MUIA_Group_Horiz, TRUE,
                            Child, (IPTR) (bt_dirup = ImageButton("", "THEME:Images/Gadgets/Prefs/Revert")),
                            Child, (IPTR) (bt_search = ImageButton("", "THEME:Images/Gadgets/Prefs/Test")),
                        End ),
                End),

            End,             
        End),
        
        TAG_DONE
    );
    
    if (self != NULL)
    {
        data = INST_DATA(CLASS, self);
        data->wped_WorkbenchPI = workbenchPI;
        data->wped_DrawersPI   = drawersPI;
        data->wped_c_NavigationMethod = c_navitype;
        data->wped_cm_ToolbarEnabled = cm_toolbarenabled;
        data->wped_toolbarpreviev = toolbarpreviev;
        
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
        DoMethod
        (
             c_navitype, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,  
             (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        ); 
        DoMethod
        (
             cm_toolbarenabled, MUIM_Notify, MUIA_Pressed, MUIV_EveryTime,  
             (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        ); 
        
                        
        /* navigation type cycle button */
        DoMethod
        (
             c_navitype, MUIM_Notify, MUIA_Cycle_Active, WPD_NAVIGATION_ENHANCED,  
             (IPTR) cm_toolbarenabled, 3, MUIM_Set, MUIA_Selected, TRUE
        ); 
        DoMethod
        (
             c_navitype, MUIM_Notify, MUIA_Cycle_Active, WPD_NAVIGATION_CLASSIC,  
             (IPTR) cm_toolbarenabled, 3, MUIM_Set, MUIA_Selected, FALSE
        ); 
        
        /* toolbar enabled checkmark*/
        DoMethod
        (
             cm_toolbarenabled, MUIM_Notify, MUIA_Selected, FALSE,  
             (IPTR) toolbarpreviev, 3, MUIM_Set, MUIA_Disabled, TRUE
        );    
        DoMethod
        (
             cm_toolbarenabled, MUIM_Notify, MUIA_Selected, TRUE,  
             (IPTR) toolbarpreviev, 3, MUIM_Set, MUIA_Disabled, FALSE
        );     
        
        /* toolbar enabled checkmark*/
 /*       DoMethod
        (
             cm_searchenabled, MUIM_Notify, MUIA_Selected, FALSE,  
             (IPTR) bt_search, 3, MUIM_Set, MUIA_ShowMe, FALSE
        );    
        DoMethod
        (
             cm_searchenabled, MUIM_Notify, MUIA_Selected, TRUE,  
             (IPTR) bt_search, 3, MUIM_Set, MUIA_ShowMe, TRUE
        );  */                  
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
        
        /* check if toolbar set */
        if (wpd.wpd_ToolbarEnabled == TRUE)
        {
            set(data->wped_cm_ToolbarEnabled, MUIA_Selected, TRUE);
        }
        else
        {
            set(data->wped_toolbarpreviev, MUIA_Disabled, TRUE);
        }
           
        /* set navigation type */   
        set(data->wped_c_NavigationMethod, MUIA_Cycle_Active, wpd.wpd_NavigationMethod);    
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
                
                /* save background paths */
                STRPTR ws = (STRPTR) XGET(data->wped_WorkbenchPI, MUIA_Imagedisplay_Spec);
                STRPTR ds = (STRPTR) XGET(data->wped_DrawersPI, MUIA_Imagedisplay_Spec);    
                strcpy(wpd.wpd_WorkbenchBackground, ws);   
                strcpy(wpd.wpd_DrawerBackground, ds);                  
                
                /* save toolbar state*/
                get(data->wped_cm_ToolbarEnabled, MUIA_Selected, &wpd.wpd_ToolbarEnabled);
                
                /* save navigation bahaviour */
                get(data->wped_c_NavigationMethod, MUIA_Cycle_Active, &wpd.wpd_NavigationMethod);
                
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
