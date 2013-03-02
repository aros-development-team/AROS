/*
    Copyright (C) 2013, The AROS Development Team.
    $Id: computer_page_cl.c 46751 2013-03-01 21:01:29Z sonic $
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <resources/hpet.h>
#include <libraries/mui.h>
#include <mui/NFloattext_mcc.h>
#include <resources/processor.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/alib.h>
#include <proto/aros.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/hpet.h>
#include <proto/kernel.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/processor.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "classes.h"
#include "cpuspecific.h"
#include "locale.h"

#define DEBUG 1
#include <aros/debug.h>

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
        MUIA_Window_Title, (IPTR)"Device properties",
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
