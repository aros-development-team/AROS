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

/*** Utility Functions ******************************************************/
BOOL ReadLine(BPTR fh, STRPTR buffer, ULONG size)
{
    if (FGets(fh, buffer, size) != NULL)
    {
        ULONG last = strlen(buffer) - 1;
        if (buffer[last] == '\n') buffer[last] = '\0';
        
        return TRUE;
    }
    
    return FALSE;
}

BOOL WriteLine(BPTR fh, CONST_STRPTR buffer)
{
    if (FPuts(fh, buffer) != 0) goto error;
    if (FPuts(fh, "\n")   != 0) goto error;

    return TRUE;
    
error:
    return FALSE;
}

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
            
            Child, (IPTR) workbenchPI = PopimageObject,
                MUIA_Window_Title,     __(MSG_SELECT_WORKBENCH_BACKGROUND),
                MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
                MUIA_CycleChain,       1,
            End,
            Child, (IPTR) drawersPI = PopimageObject,
                MUIA_Window_Title,     __(MSG_SELECT_DRAWER_BACKGROUND),
                MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_Background,
                MUIA_CycleChain,       1,
            End,
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
    
    BOOL   success = FALSE;
    STRPTR buffer  = NULL;
    LONG   size;
    
    if (message->fh != NULL)
    {
        Seek(message->fh, 0, OFFSET_END);
        size = Seek(message->fh, 0, OFFSET_BEGINNING) + 2;
        
        if ((buffer = AllocVec(size, MEMF_ANY)) != NULL)
        {
            if (!ReadLine(message->fh, buffer, size)) goto end;
            NNSET(data->wped_WorkbenchPI, MUIA_Imagedisplay_Spec, buffer);
            
            if (!ReadLine(message->fh, buffer, size)) goto end;
            NNSET(data->wped_DrawersPI, MUIA_Imagedisplay_Spec, buffer);
            
            success = TRUE;
end:        FreeVec(buffer);
        }
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
    
    BOOL success = FALSE;
    
    if (message->fh != NULL)
    {
        STRPTR ws = (STRPTR) XGET(data->wped_WorkbenchPI, MUIA_Imagedisplay_Spec);
        STRPTR ds = (STRPTR) XGET(data->wped_DrawersPI, MUIA_Imagedisplay_Spec);
        
        if (!WriteLine(message->fh, ws)) goto end;
        if (!WriteLine(message->fh, ds)) goto end;
                
        success = TRUE;
    }

end:
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
