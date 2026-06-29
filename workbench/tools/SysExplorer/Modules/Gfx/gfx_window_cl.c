/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: SysExplorer property window for a graphics driver object.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
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

#include "gfx_classes.h"
#include "cpuspecific.h"
#include "locale.h"

#include <zune/customclasses.h>

extern OOP_AttrBase HiddAttrBase;
extern OOP_AttrBase HiddGfxAttrBase;

/*** Instance Data **********************************************************/
struct GfxWindow_DATA
{
    Object *memTotal;
    Object *memFree;
    Object *GARTTotal;
    Object *GARTFree;
    struct TagItem gfxMemTags[12];
};

static Object *GfxWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    static STRPTR pagetitles[3] =
    {
        "General",
        "Capabilities",
        NULL
    };
    Object *memTotal, *memFree, *GARTTotal, *GARTFree;
    IPTR name = (IPTR)NULL, driver = (IPTR)NULL;
    OOP_Object *gfxhidd_obj =
        (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);

    OOP_GetAttr(gfxhidd_obj, aHidd_HardwareName, &name);
    OOP_GetAttr(gfxhidd_obj, aHidd_Name, &driver);

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, name,
        MUIA_Window_ID, MAKE_ID('G', 'F', 'X', 'D'),
        WindowContents, (IPTR)(RegisterObject,
            MUIA_Register_Titles, (IPTR)pagetitles,
            Child, (IPTR)(VGroup,
                Child, (IPTR)(VGroup,
                    GroupFrameT("Details"),
                    Child, (IPTR)(ColGroup(2),
                        Child, (IPTR)Label("Device:"),
                        Child, (IPTR)LLabel((STRPTR)name),
                        Child, (IPTR)Label("Manufacturer:"),
                        Child, (IPTR)LLabel(""),
                        Child, (IPTR)Label("Driver:"),
                        Child, (IPTR)LLabel((STRPTR)driver),
                    End),
                End),
                Child, (IPTR)(HGroup,
                    GroupFrameT("Memory Information"),
                    Child, (IPTR)(VGroup,
                        Child, (IPTR)Label(""),
                        Child, (IPTR)Label("Video RAM"),
                        Child, (IPTR)Label("GART"),
                    End),
                    Child, (IPTR)(ColGroup(2),
                        Child, (IPTR)Label("Total"),
                        Child, (IPTR)Label("Free"),
                        Child, (IPTR)(memTotal = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"N/A",
                        End),
                        Child, (IPTR)(memFree = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"",
                        End),
                        Child, (IPTR)(GARTTotal = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"N/A",
                        End),
                        Child, (IPTR)(GARTFree = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"",
                        End),
                    End),
                End),
            End),
            Child, (IPTR)(VGroup,
                Child, (IPTR)HVSpace,
            End),
        End),
        TAG_DONE
    );

    if (self)
    {
        struct GfxWindow_DATA *data = INST_DATA(cl, self);
        char valBuf[24];
        data->memTotal = memTotal;
        data->memFree = memFree;
        data->GARTTotal = GARTTotal;
        data->GARTFree = GARTFree;

        data->gfxMemTags[0].ti_Tag = tHidd_Gfx_MemTotal;
        data->gfxMemTags[1].ti_Tag = tHidd_Gfx_MemFree;
        data->gfxMemTags[2].ti_Tag = tHidd_Gfx_MemAddressableTotal;
        data->gfxMemTags[3].ti_Tag = tHidd_Gfx_MemAddressableFree;
        data->gfxMemTags[4].ti_Tag = tHidd_Gfx_MemClock;
        data->gfxMemTags[5].ti_Tag = TAG_DONE;

        OOP_GetAttr(gfxhidd_obj, aHidd_Gfx_MemoryAttribs, (IPTR *)data->gfxMemTags);

        if (data->gfxMemTags[0].ti_Data)
        {
            sprintf(valBuf, "%ld", (long)data->gfxMemTags[0].ti_Data);
            SET(data->memTotal, MUIA_Text_Contents, valBuf);
            sprintf(valBuf, "%ld", (long)data->gfxMemTags[1].ti_Data);
            SET(data->memFree, MUIA_Text_Contents, valBuf);
        }
        if (data->gfxMemTags[2].ti_Data)
        {
            sprintf(valBuf, "%ld", (long)data->gfxMemTags[2].ti_Data);
            SET(data->GARTTotal, MUIA_Text_Contents, valBuf);
            sprintf(valBuf, "%ld", (long)data->gfxMemTags[3].ti_Data);
            SET(data->GARTFree, MUIA_Text_Contents, valBuf);
        }
    }
    return self;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    GfxWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
