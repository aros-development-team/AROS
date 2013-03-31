/*
    Copyright (C) 2013, The AROS Development Team.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/ata.h>
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

#include <aros/debug.h>

#include <zune/customclasses.h>

/*** Instance Data **********************************************************/
struct ATAUnitWindow_DATA
{
    /* Nothing to add */
};

static const char *const xferModeNames[] =
{
    "PIO0" , "PIO1" , "PIO2 ", "PIO3" , "PIO4" ,
    "MDMA0", "MDMA1", "MDMA2",
    "UDMA0", "UDMA1", "UDMA2", "UDMA3", "UDMA4", "UDMA5", "UDMA6",
    (const char *)AB_XFER_48BIT,
    "LBA48", "Multisector", "ATAPI", "LBA", "PIO32",
    NULL
};

static void DecodeBits(char *str, ULONG flags, const char *const *names)
{
    unsigned char i, done;

    for (i = 0, done = 0; *names; names++)
    {
        if ((IPTR)*names < 32)
        {
            i = (IPTR)*names;
            continue;
        }
        if (flags & (1 << i))
        {
            strcpy(str, *names);
            str += strlen(*names);
            if ((done % 5) == 4)
                *str++ = '\n';
            else
                *str++ = ' ';
            done++;
        }
        i++;
    }

    *str = 0;
}

static Object *ATAUnitWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    IPTR model, revision, serial;
    IPTR val;
    char xfermodes_str[256];
    char usemodes_str[256];
    char multisector_str[4];
    char unit_str[6];
    LONG removable;

    OOP_GetAttr(dev, aHidd_ATAUnit_Model   , &model);
    OOP_GetAttr(dev, aHidd_ATAUnit_Revision, &revision);
    OOP_GetAttr(dev, aHidd_ATAUnit_Serial  , &serial);

    removable = OOP_GET(dev, aHidd_ATAUnit_Removable) ? IDS_SELECTED : IDS_NORMAL;

    OOP_GetAttr(dev, aHidd_ATAUnit_Number, &val);
    snprintf(unit_str, sizeof(unit_str), "%ld", val);
    
    OOP_GetAttr(dev, aHidd_ATAUnit_XferModes, &val);        
    DecodeBits(xfermodes_str, val, xferModeNames),

    OOP_GetAttr(dev, aHidd_ATAUnit_ConfiguredModes, &val);        
    DecodeBits(usemodes_str, val, xferModeNames),
    
    OOP_GetAttr(dev, aHidd_ATAUnit_MultiSector, &val);
    snprintf(multisector_str, sizeof(multisector_str), "%ld", val);
    
    return (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, __(MSG_ATA_UNIT_PROPERTIES),
        MUIA_Window_ID, MAKE_ID('A', 'U', 'N', 'P'),
        WindowContents, (IPTR)(ColGroup(2),
            Child, (IPTR)Label(_(MSG_UNIT_NUMBER)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, (IPTR)unit_str,
            End),
            Child, (IPTR)Label(_(MSG_MODEL)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, model,
            End),
            Child, (IPTR)Label(_(MSG_REVISION)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, revision,
            End),
            Child, (IPTR)Label(_(MSG_SERIAL)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, serial,
            End),
            Child, (IPTR)Label(_(MSG_TRANSFER_MODES)),
            Child, (IPTR)(NFloattextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Floattext_Text, (IPTR)xfermodes_str,
            End),
            Child, (IPTR)Label(_(MSG_CONFIG_MODES)),
            Child, (IPTR)(NFloattextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Floattext_Text, (IPTR)usemodes_str,
            End),
            Child, (IPTR)Label(_(MSG_MULTISECTOR)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, (IPTR)multisector_str,
            End),
            Child, (IPTR)Label(_(MSG_REMOVABLE)),
            Child, (IPTR)(HGroup,
                Child, (IPTR)(ImageObject,
                    MUIA_Image_Spec, MUII_CheckMark,
                    MUIA_Image_State, removable,
                    TextFrame,
                    MUIA_CycleChain, 1,
                    MUIA_Background, MUII_TextBack,
                End),
                Child, (IPTR)HSpace(0),
            End),
        End),
        TAG_DONE
    );
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    ATAUnitWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
