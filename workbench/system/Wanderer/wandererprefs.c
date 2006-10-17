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
#include <proto/iffparse.h>

#include <string.h>

#include "wandererprefs.h"
#include "support.h"

#include <prefs/prefhdr.h>
#include <prefs/wanderer.h>

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
        if (  (*dst == NULL)  ||  (strcmp(src, *dst) != 0)  )
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
                if ( !SetString (&data->wpd_WorkbenchBackground, (STRPTR) tag->ti_Data) )
                {
                    tag->ti_Tag = TAG_IGNORE;
                }                
                break;
                
            case MUIA_WandererPrefs_DrawerBackground:
                if ( !SetString (&data->wpd_DrawerBackground, (STRPTR) tag->ti_Data)  )
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
    struct ContextNode     *context;    
    struct IFFHandle       *handle;
    struct WandererPrefs    wpd;  
    BOOL                    success = TRUE;
    LONG                    error;
    
                    
    if (!(handle = AllocIFF()))
        return FALSE;
    
    handle->iff_Stream = (IPTR)Open("ENV:SYS/Wanderer.prefs", MODE_OLDFILE); 

    if (!handle->iff_Stream) return FALSE;
    
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
                    
                    error = ReadChunkBytes( handle, &wpd, sizeof(struct WandererPrefs) );
                    
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

    Close((IPTR)handle->iff_Stream);
    FreeIFF(handle);
    
    
    if (success)
    {
        /* TODO: fix problems with endianess?? */
        //SMPByteSwap(&wpd);
        
        SET(self, MUIA_WandererPrefs_WorkbenchBackground, (STRPTR)wpd.wpd_WorkbenchBackground);
        SET(self, MUIA_WandererPrefs_DrawerBackground, (STRPTR)wpd.wpd_DrawerBackground);    
        
        return TRUE;       
    }

    
    return FALSE;
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
