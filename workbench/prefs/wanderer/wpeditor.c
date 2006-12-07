/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1

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

// #include <string.h>

#include "locale.h"
#include "wpeditor.h"


/*** Instance Data **********************************************************/
struct WPEditor_DATA
{
    Object *wped_WorkbenchPI, *wped_DrawersPI; 
    Object *wped_c_NavigationMethod, 
           *wped_cm_ToolbarEnabled, 
           *wped_toolbarpreview;
    Object *wped_icon_listmode;
    Object *wped_icon_textmode;
    Object *wped_icon_textmaxlen;
        
};

//static struct Hook navichangehook;
static STRPTR navigationtypelabels[3];
static STRPTR iconlistmodes[3];
static STRPTR icontextmodes[3];
static STRPTR registerpages[4];

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct WPEditor_DATA *data = INST_DATA(CLASS, self)


/*** Methods ****************************************************************/
Object *WPEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    struct WPEditor_DATA *data = NULL;
    
    Object *workbenchPI = NULL, *drawersPI = NULL, *c_navitype = NULL, 
           *bt_dirup = NULL, *bt_search = NULL, *cm_toolbarenabled = NULL, 
           *toolbarpreview = NULL, *wped_icon_listmode = NULL, *wped_icon_textmode = NULL, 
           *wped_icon_textmaxlen = NULL;

    //Object *cm_searchenabled;

    registerpages[WPD_GENERAL] = (STRPTR)_(MSG_GENERAL);
    registerpages[WPD_APPEARANCE] = (STRPTR)_(MSG_APPEARANCE);
    registerpages[WPD_TOOLBAR] = (STRPTR)_(MSG_TOOLBAR);

    iconlistmodes[WPD_ICONLISTMODE_GRID] = (STRPTR)_(MSG_ICONLISTMODE_GRID);
    iconlistmodes[WPD_ICONLISTMODE_PLAIN] = (STRPTR)_(MSG_ICONLISTMODE_PLAIN);

    icontextmodes[WPD_ICONTEXTMODE_OUTLINE] = (STRPTR)_(MSG_ICONTEXTMODE_OUTLINE);
    icontextmodes[WPD_ICONTEXTMODE_PLAIN] = (STRPTR)_(MSG_ICONTEXTMODE_PLAIN);

    navigationtypelabels[WPD_NAVIGATION_CLASSIC] = (STRPTR)_(MSG_CLASSIC);
    navigationtypelabels[WPD_NAVIGATION_ENHANCED] = (STRPTR)_(MSG_ENHANCED);

    c_navitype = MUI_MakeObject(MUIO_Cycle, NULL, navigationtypelabels);
    wped_icon_listmode = MUI_MakeObject(MUIO_Cycle, NULL, iconlistmodes);
    wped_icon_textmode = MUI_MakeObject(MUIO_Cycle, NULL, icontextmodes);
    cm_toolbarenabled = MUI_MakeObject(MUIO_Checkmark,NULL);
    wped_icon_textmaxlen = MUI_MakeObject(MUIO_String,NULL,4);

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
            
            MUIA_PrefsEditor_Name, __(MSG_NAME),
            MUIA_PrefsEditor_Path, (IPTR) "SYS/Wanderer.prefs",
            
            Child, (IPTR) (RegisterObject,
                MUIA_Register_Titles, (IPTR) registerpages,
                Child, (IPTR) GroupObject,                     // general 
                    MUIA_FrameTitle, __(MSG_NAVIGATION),
                    MUIA_Group_SameSize, TRUE,
                    MUIA_Frame, MUIV_Frame_Group,
                    Child, (IPTR) HGroup,
                        Child, (IPTR) Label1(_(MSG_METHOD)),
                        Child, (IPTR) c_navitype,
                    End,
                End,
                Child, (IPTR) (GroupObject,                     // appearance 
                    MUIA_Group_SameSize, TRUE,
                    MUIA_Group_Horiz, TRUE,
                    
                    Child, (IPTR) (GroupObject,
                        MUIA_FrameTitle, __(MSG_BACKGROUNDS),
                        MUIA_Frame, MUIV_Frame_Group,
                        
                        Child, (IPTR) HGroup,
                            Child, (IPTR) Label1(_(MSG_BACKGROUND_WORKBENCH)),
                            Child, (IPTR) (workbenchPI = PopimageObject,
                                MUIA_Window_Title, __(MSG_SELECT_WORKBENCH_BACKGROUND),
                                MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
                                MUIA_CycleChain,       1,
                            End),
                        End,
                        Child, (IPTR) HGroup,
                            Child, (IPTR) Label1(_(MSG_BACKGROUND_DRAWERS)),
                            Child, (IPTR) (drawersPI = PopimageObject,
                                MUIA_Window_Title, __(MSG_SELECT_DRAWER_BACKGROUND),
                                MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
                                MUIA_CycleChain,       1,
                            End),
                        End,
                    End),
                    Child, (IPTR) (GroupObject,
                        MUIA_FrameTitle, __(MSG_ICONSPREFS),
                        MUIA_Frame, MUIV_Frame_Group,
                        Child, (IPTR) HGroup,
                            Child, (IPTR) Label1(_(MSG_ICONLISTMODE)),
                            Child, (IPTR) wped_icon_listmode,
                        End,
                        Child, (IPTR) HGroup,
                            Child, (IPTR) Label1(_(MSG_ICONTEXTMODE)),
                            Child, (IPTR) wped_icon_textmode,
                        End,
                        Child, (IPTR) HGroup,
                            Child, (IPTR) Label1(_(MSG_ICONTEXTLENGTH)),
                            Child, (IPTR) wped_icon_textmaxlen,
                        End,
                    End),
                End),
                Child, (IPTR) (GroupObject,                     // toolbar 
                    Child, (IPTR) HGroup,
                        MUIA_FrameTitle,  __(MSG_OBJECTS),
                        MUIA_Group_SameSize, TRUE,
                        MUIA_Frame, MUIV_Frame_Group,
                        Child, (IPTR) Label1(_(MSG_TOOLBAR_ENABLED)),
                        Child, (IPTR) cm_toolbarenabled,
                        
                        //Child, Label1("search"),
                        //Child, cm_searchenabled = MUI_MakeObject(MUIO_Checkmark,NULL),                        
                    End,
                    Child, (IPTR) (toolbarpreview = GroupObject,
                        MUIA_FrameTitle, __(MSG_PREVIEW),
                        MUIA_Group_SameSize, TRUE,
                        MUIA_Frame, MUIV_Frame_Group,
        
                        Child, (IPTR) HGroup,
                            Child, (IPTR) (bt_dirup = ImageButton("", "THEME:Images/Gadgets/Prefs/Revert")),
                            Child, (IPTR) (bt_search = ImageButton("", "THEME:Images/Gadgets/Prefs/Test")),
                        End,
                    End),
                End),          
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
        data->wped_toolbarpreview = toolbarpreview;
        data->wped_icon_listmode = wped_icon_listmode;
        data->wped_icon_textmode = wped_icon_textmode;
        data->wped_icon_textmaxlen = wped_icon_textmaxlen;
        
        //-- Setup notifications -------------------------------------------
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
        
        DoMethod
        (
            wped_icon_listmode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        ); 
        DoMethod
        (
            wped_icon_textmode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );        
                
        /* toolbar enabled checkmark */
        DoMethod
        (
            cm_toolbarenabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            cm_toolbarenabled, MUIM_Notify, MUIA_Selected, FALSE,  
            (IPTR) toolbarpreview, 3, MUIM_Set, MUIA_Disabled, TRUE
        );    
        DoMethod
        (
            cm_toolbarenabled, MUIM_Notify, MUIA_Selected, TRUE,  
            (IPTR) toolbarpreview, 3, MUIM_Set, MUIA_Disabled, FALSE
        );     
        
        /* navigation cycle linked to toolbar checkbox, enhanced nevigation sets toolbar */
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
        // Icon textmode maxlength
        set ( wped_icon_textmaxlen, MUIA_String_Integer, ICON_TEXT_MAXLEN_DEFAULT );
        set ( wped_icon_textmaxlen, MUIA_String_MaxLen, 3 );
        set ( wped_icon_textmaxlen, MUIA_String_Format, MUIV_String_Format_Right );
        set ( wped_icon_textmaxlen, MUIA_String_Accept, ( IPTR )"0123456789" ); 
        DoMethod ( 
            wped_icon_textmaxlen, MUIM_Notify, MUIA_String_Integer, MUIV_EveryTime,  
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

        
        // FIXME: We want some sanity checking here!
        BYTE i = 0; for (; i < 1; i++)
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
                        D(bug("[WPEDITOR] ReadChunkBytes() returned %ld!\n", error));
                    }                    
                }
                else
                {
                    D(bug("[WPEDITOR] ParseIFF() failed, returncode %ld!\n", error));
                    success = FALSE;
                    break;
                }
            }
            else
            {
                D(bug("[WPEDITOR] StopChunk() failed, returncode %ld!\n", error));
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
        // Why is this commented out?
        //SMPByteSwap(&wpd);

        NNSET(data->wped_WorkbenchPI, MUIA_Imagedisplay_Spec, (STRPTR)wpd.wpd_WorkbenchBackground);
        NNSET(data->wped_DrawersPI, MUIA_Imagedisplay_Spec, (STRPTR)wpd.wpd_DrawerBackground);
        NNSET(data->wped_cm_ToolbarEnabled, MUIA_Selected, wpd.wpd_ToolbarEnabled);

        /* check if toolbar set */
        if (wpd.wpd_ToolbarEnabled == FALSE)
            set(data->wped_toolbarpreview, MUIA_Disabled, TRUE);
        
        /* Icon listmode */
        set ( data->wped_icon_listmode, MUIA_Cycle_Active, (IPTR)wpd.wpd_IconListMode );
        
        /* Icon textmode */
        set ( data->wped_icon_textmode, MUIA_Cycle_Active, (IPTR)wpd.wpd_IconTextMode );
        
        /* set max text length */
        set(data->wped_icon_textmaxlen, MUIA_String_Integer, (IPTR)wpd.wpd_IconTextMaxLen);
        
        /* set navigation type */   
        set(data->wped_c_NavigationMethod, MUIA_Cycle_Active, (IPTR)wpd.wpd_NavigationMethod);    
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
                    D(bug("[WPEDITOR] PushChunk() = %ld failed\n", error));
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
                
                /* save the icon listing method */
                get(data->wped_icon_listmode, MUIA_Cycle_Active, &wpd.wpd_IconListMode);
                
                /* save the icon text mode */
                get(data->wped_icon_textmode, MUIA_Cycle_Active, &wpd.wpd_IconTextMode);
                
                /* save the max length of icons */
                get(data->wped_icon_textmaxlen, MUIA_String_Integer, &wpd.wpd_IconTextMaxLen);
                
                /* TODO: fix problems with endianess?? */
                //SMPByteSwap(&wpd); 
            
                error = WriteChunkBytes(handle, &wpd, sizeof(struct WandererPrefs));
                error = PopChunk(handle);
                                
                if (error != 0) // TODO: We need some error checking here!
                {
                    D(bug("[WPEDITOR] PopChunk() = %ld\n", error));
                }
                
            }        
            
            /* Terminate the FORM */
            PopChunk(handle);   
        }
        else
        {
            //ShowError(_(MSG_CANT_OPEN_STREAM));
            D(bug("[WPEDITOR] Can't open stream!\n"));
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
