/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/alib.h>

#include <string.h>

#include "wandererprefs.h"
#include "support.h"

/*** Instance Data **********************************************************/
struct WandererPrefs_DATA
{
    STRPTR wpd_WorkbenchBackground,
           wpd_DrawerBackground;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct WandererPrefs_DATA *data = INST_DATA(CLASS, self)

/*** Utility Functions ******************************************************/
BOOL SetString(STRPTR *dst, STRPTR src)
{
    if (src != NULL)
    {
        if
        (
               *dst == NULL
            || strcmp(src, *dst) != 0
        )
        {
            STRPTR tmp = StrDup(src);
            
            if (tmp != NULL)
            {
                FreeVec(*dst);
                *dst = tmp;
                
                return TRUE;
            }
        }
    }
       
    return FALSE;
}

/*** Methods ****************************************************************/
Object *WandererPrefs__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);
    
    if (self != NULL)
    {
        DoMethod(self, MUIM_WandererPrefs_Reload);
    }
    
    return self;
}

IPTR WandererPrefs__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    
    FreeVec(data->wpd_WorkbenchBackground);
    FreeVec(data->wpd_DrawerBackground);
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR WandererPrefs__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, *tag;
    
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_WandererPrefs_WorkbenchBackground:
                if
                (
                    !SetString
                    (
                        &data->wpd_WorkbenchBackground, 
                        (STRPTR) tag->ti_Data
                    )
                )
                {
                    tag->ti_Tag = TAG_IGNORE;
                }                
                break;
                
            case MUIA_WandererPrefs_DrawerBackground:
                if
                (
                    !SetString
                    (
                        &data->wpd_DrawerBackground, 
                        (STRPTR) tag->ti_Data
                    )
                )
                {
                    tag->ti_Tag = TAG_IGNORE;
                }                
                break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR WandererPrefs__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
        case MUIA_WandererPrefs_WorkbenchBackground:
            *store = (IPTR) data->wpd_WorkbenchBackground;
            break;
            
        case MUIA_WandererPrefs_DrawerBackground:
            *store = (IPTR) data->wpd_DrawerBackground;
            
        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}

IPTR WandererPrefs__MUIM_WandererPrefs_Reload
(
    Class *CLASS, Object *self, Msg message
)
{
    BPTR fh;
    
    if ((fh = Open("ENV:SYS/Wanderer.prefs", MODE_OLDFILE)) != NULL)
    {
        STRPTR buffer = NULL;
        LONG   size;
        
        Seek(fh, 0, OFFSET_END);
        size = Seek(fh, 0, OFFSET_BEGINNING) + 2;
        
        if ((buffer = AllocVec(size, MEMF_ANY)) != NULL)
        {
            if (!ReadLine(fh, buffer, size)) goto end;
            SET(self, MUIA_WandererPrefs_WorkbenchBackground, (IPTR) buffer);
            
            if (!ReadLine(fh, buffer, size)) goto end;
            SET(self, MUIA_WandererPrefs_DrawerBackground, (IPTR) buffer);
            
end:        FreeVec(buffer);
        }
        
        Close(fh);
    }

    
    return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_5
(
    WandererPrefs, NULL, MUIC_Notify, NULL,
    OM_NEW,                    struct opSet *,
    OM_DISPOSE,                Msg,
    OM_SET,                    struct opSet *,
    OM_GET,                    struct opGet *,
    MUIM_WandererPrefs_Reload, Msg
);
