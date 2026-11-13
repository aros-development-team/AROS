/*
** IconListClass - a subclass of List.mui that owns the row icon images. On
** MUIM_Setup it builds one MUI list image per entry in the (original) icon
** set; the various display hooks in ActionClass render "\33O[image] text".
** This mirrors how Trident's IconListClass provides icons for its lists.
*/

#include <exec/types.h>
#include <intuition/classusr.h>
#include <graphics/view.h>
#include <libraries/mui.h>

#define MUIMASTER_YES_INLINE_STDARG
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <clib/alib_protos.h>

#include "bluetoothprefs.h"
#include "IconListClass.h"
#include "icons.h"
#include "debug.h"

#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

/* /// "IconListDispatcher()" */
AROS_UFH3(IPTR, IconListDispatcher,
          AROS_UFHA(struct IClass *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
    struct IconListData *data = NULL;

    if(msg->MethodID != OM_NEW) data = INST_DATA(cl, obj);

    switch(msg->MethodID)
    {
        case MUIM_Setup:
            if(!DoSuperMethodA(cl, obj, msg)) return FALSE;
            {
                ULONG i;
                for(i = 0; i < ICON_COUNT; i++)
                {
                    data->bodies[i] = BodychunkObject,
                        MUIA_FixWidth,              ICON_WIDTH,
                        MUIA_FixHeight,             ICON_HEIGHT,
                        MUIA_Bitmap_Width,          ICON_WIDTH,
                        MUIA_Bitmap_Height,         ICON_HEIGHT,
                        MUIA_Bodychunk_Depth,       ICON_DEPTH,
                        MUIA_Bodychunk_Body,        (IPTR)icon_bodies[i],
                        MUIA_Bodychunk_Compression, 0,
                        MUIA_Bodychunk_Masking,     2,
                        MUIA_Bitmap_Transparent,    ICON_TRANSPARENT,
                        MUIA_Bitmap_SourceColors,   (IPTR)icon_colors,
                        MUIA_Bitmap_UseFriend,      TRUE,
                        MUIA_Bitmap_Precision,      PRECISION_ICON,
                        End;
                    data->images[i] = (Object *)DoMethod(obj, MUIM_List_CreateImage, data->bodies[i], 0);
                }
            }
            return TRUE;

        case MUIM_Cleanup:
            {
                ULONG i;
                for(i = 0; i < ICON_COUNT; i++)
                {
                    if(data->images[i]) { DoMethod(obj, MUIM_List_DeleteImage, data->images[i]); data->images[i] = NULL; }
                    if(data->bodies[i]) { MUI_DisposeObject(data->bodies[i]); data->bodies[i] = NULL; }
                }
            }
            break;
    }
    return DoSuperMethodA(cl, obj, msg);

    AROS_USERFUNC_EXIT
}
/* \\\ */
