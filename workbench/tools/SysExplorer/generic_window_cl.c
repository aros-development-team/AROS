/*
    Copyright (C) 2013, The AROS Development Team.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <libraries/mui.h>
#include <mui/NFloattext_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "classes.h"
#include "cpuspecific.h"
#include "locale.h"

#include <zune/customclasses.h>

/*** Instance Data **********************************************************/
struct GenericWindow_DATA
{
    /* Nothing to add */
};

static Object *GenericWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, __(MSG_DEVICE_PROPERTIES),
        MUIA_Window_ID, MAKE_ID('D', 'E', 'V', 'P'),
        WindowContents, (IPTR)(DevicePageObject,
            TAG_MORE, (IPTR)msg->ops_AttrList,
        End),
        TAG_DONE
    );

    return self;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    GenericWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
