/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "title_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

IPTR Title__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        MUIA_Group_Horiz, TRUE,
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
    if (!obj) return FALSE;

    D(bug("muimaster.library/title.c: Title Object created at 0x%lx\n",obj));

    return (IPTR)obj;
}

#if ZUNE_BUILTIN_TITLE
BOOPSI_DISPATCHER(IPTR, Title_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW: return Title__OM_NEW(cl, obj, (struct opSet *)msg);
        default: return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Title_desc =
{
    MUIC_Title,
    MUIC_Group,
    sizeof(struct Title_DATA),
    (void*)Title_Dispatcher
};
#endif /* ZUNE_BUILTIN_TITLE */
