/*
    Copyright (C) 2013, The AROS Development Team.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/ata.h>
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
struct ATAWindow_DATA
{
    /* Nothing to add */
};

static inline void SetCheckState(Object *img, OOP_Object *dev, ULONG attr)
{
    LONG state = OOP_GET(dev, attr) ? IDS_SELECTED : IDS_NORMAL;

    SET(img, MUIA_Image_State, state);
}

static Object *ATAWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    Object *ioalt_img, *pio32_img, *dma_img;

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, __(MSG_ATA_PROPERTIES),
        MUIA_Window_ID, MAKE_ID('A', 'T', 'A', 'P'),
        WindowContents, (IPTR)(VGroup,
            Child, (IPTR)(DevicePageObject,
                MUIA_PropertyWin_Object, (IPTR)dev,
            End),
            Child, (IPTR)(ColGroup(3),
                MUIA_FrameTitle, __(MSG_ATA),
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                Child, (IPTR)Label(_(MSG_USE_IOALT)),
                Child, (IPTR)(ioalt_img = ImageObject,
                    MUIA_Image_Spec, MUII_CheckMark,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                End),
                Child, (IPTR)HSpace(0),
                Child, (IPTR)Label(_(MSG_USE_32BIT)),
                Child, (IPTR)(pio32_img = ImageObject,
                    MUIA_Image_Spec, MUII_CheckMark,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                End),
                Child, (IPTR)HSpace(0),
                Child, (IPTR)Label(_(MSG_USE_DMA)),
                Child, (IPTR)(dma_img = ImageObject,
                    MUIA_Image_Spec, MUII_CheckMark,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                End),
                Child, (IPTR)HSpace(0),
            End),
        End),
        TAG_DONE
    );

    if (self)
    {
        SetCheckState(ioalt_img, dev, aHidd_ATABus_UseIOAlt);
        SetCheckState(pio32_img, dev, aHidd_ATABus_Use32Bit);
        SetCheckState(dma_img, dev, aHidd_ATABus_UseDMA);
    }

    return self;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    ATAWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
