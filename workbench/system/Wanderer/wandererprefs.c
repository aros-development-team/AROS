/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id$
*/
#define DEBUG 0
#include <aros/debug.h>

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
           
    ULONG  wpd_NavigationMethod;
    ULONG  wpd_ToolbarEnabled;

    ULONG  wpd_IconListMode;
    ULONG  wpd_IconTextMode;
    ULONG  wpd_IconTextMaxLen;
    ULONG  wpd_DoubleBuffered;
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
    const struct TagItem *tstate = message->ops_AttrList;
    struct TagItem *tag;
    
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

            case MUIA_WandererPrefs_NavigationMethod:
                data->wpd_NavigationMethod = (LONG) tag->ti_Data;
                
            case MUIA_WandererPrefs_Toolbar_Enabled:
                data->wpd_ToolbarEnabled = (LONG) tag->ti_Data;

            case MUIA_WandererPrefs_DoubleBuffered:
                data->wpd_DoubleBuffered = (LONG) tag->ti_Data;

            case MUIA_WandererPrefs_Icon_ListMode:
                data->wpd_IconListMode = (LONG) tag->ti_Data;

            case MUIA_WandererPrefs_Icon_TextMode:
                data->wpd_IconTextMode = (ULONG) tag->ti_Data;
                break;
                
            case MUIA_WandererPrefs_Icon_TextMaxLen:
                data->wpd_IconTextMaxLen = (ULONG) tag->ti_Data;
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
            break;

        case MUIA_WandererPrefs_NavigationMethod:
            *store = (IPTR) data->wpd_NavigationMethod;
            break;

        case MUIA_WandererPrefs_Toolbar_Enabled:
            *store = (IPTR) data->wpd_ToolbarEnabled;
            break;

        case MUIA_WandererPrefs_DoubleBuffered:
            *store = (IPTR) data->wpd_DoubleBuffered;
            break;

        case MUIA_WandererPrefs_Icon_ListMode:
            *store = (IPTR) data->wpd_IconListMode;
            break;

        case MUIA_WandererPrefs_Icon_TextMode:
            *store = (IPTR) data->wpd_IconTextMode;
            break;
            
        case MUIA_WandererPrefs_Icon_TextMaxLen:
            *store = (IPTR) data->wpd_IconTextMaxLen;
            break;

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
    
    handle->iff_Stream = (IPTR) Open("ENV:SYS/Wanderer.prefs", MODE_OLDFILE); 

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

    Close((APTR)handle->iff_Stream);
    FreeIFF(handle);
    
    
    if (success)
    {
        /* TODO: fix problems with endianess?? */
        //SMPByteSwap(&wpd);
        
        SetAttrs(self, MUIA_WandererPrefs_WorkbenchBackground, (STRPTR)wpd.wpd_WorkbenchBackground,
                       MUIA_WandererPrefs_DrawerBackground, (STRPTR)wpd.wpd_DrawerBackground,
                       MUIA_WandererPrefs_NavigationMethod, wpd.wpd_NavigationMethod,
                       MUIA_WandererPrefs_Toolbar_Enabled, wpd.wpd_ToolbarEnabled,
                       MUIA_WandererPrefs_DoubleBuffered, wpd.wpd_DoubleBuffered,
                       MUIA_WandererPrefs_Icon_ListMode, wpd.wpd_IconListMode,
                       MUIA_WandererPrefs_Icon_TextMode, wpd.wpd_IconTextMode, 
                       MUIA_WandererPrefs_Icon_TextMaxLen, wpd.wpd_IconTextMaxLen,
                       TAG_DONE);

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
