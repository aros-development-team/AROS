/*
    Copyright (C) 2003-2026, The AROS Development Team. All rights reserved.
*/

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

/****** Mccprefs.mui/MUIC_Mccprefs ************
*
*   NAME
*       MUIC_Mccprefs
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Provide a Group-derived class with no additional preferences behavior in
*       this AROS implementation. Construction delegates to Group, scans the
*       tags without handling any class-specific attributes, and returns the
*       object or zero on superclass failure.
*       The public header declares only the class name. There are no local
*       preference load/save methods, editor controls or custom attributes. All
*       messages other than construction delegate to Group; child ownership,
*       setup and disposal follow that superclass. The instance contains only a
*       dummy field. Do not infer a complete custom-class preferences editor
*       from this class name.
*
*/

extern struct Library *MUIMasterBase;

struct MUI_MccprefsData
{
    LONG dummy;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Delegate Group construction and return object or zero on failure. Scan
 * readable tag list without handling any local attribute. Own no additional
 * resource or preferences state; Group governs children and disposal.
 */
IPTR Mccprefs__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tag, *tags;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);

    if (!obj)
        return FALSE;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        }
    }

    return (IPTR) obj;
}


/*
 * Dispatch local overrides and return handler results; delegate other
 * messages to the declared superclass. Requires correct method ABI and live
 * object except construction. No implicit additional ownership,
 * synchronization or conversion is supplied by this dispatcher.
 */
BOOPSI_DISPATCHER(IPTR, Mccprefs_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Mccprefs__OM_NEW(cl, obj, (struct opSet *)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Mccprefs_desc =
{
    MUIC_Mccprefs,
    MUIC_Group,
    sizeof(struct MUI_MccprefsData),
    (void *) Mccprefs_Dispatcher
};
