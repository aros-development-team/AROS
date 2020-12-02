/*
    Copyright (C) 2020, The AROS Development Team.
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

#include "pci_classes.h"
#include "cpuspecific.h"
#include "locale.h"

#include <zune/customclasses.h>

/*** Instance Data **********************************************************/
struct PCIDevWindow_DATA
{
    /* Nothing to add */
};

static Object *PCIDevWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    STRPTR pagetitles[3] =
    {
        "General",
        "Capabilities",
        NULL
    };

    IPTR name;
    OOP_Object *display_obj = 
        (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);

//    OOP_GetAttr(display_obj, aHidd_Name, &name);

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, (IPTR)name,
        MUIA_Window_ID, MAKE_ID('P', 'C', 'I', 'D'),
        WindowContents, (IPTR)(RegisterObject,
            MUIA_Register_Titles, (IPTR) pagetitles,
            Child, VGroup,
                Child, HVSpace,
            End,
            Child, VGroup,
                Child, HVSpace,
            End,
        End),
        TAG_DONE
    );

    return self;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    PCIDevWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
