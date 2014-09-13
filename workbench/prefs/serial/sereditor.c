/*
    Copyright © 2003-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

// #define DEBUG 1
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <stdlib.h>

#include <aros/debug.h>

#include "locale.h"
#include "sereditor.h"
#include "prefs.h"

/*** String Data ************************************************************/

CONST_STRPTR BaudrateLabels[] =
{
    (CONST_STRPTR)    "50",
    (CONST_STRPTR)    "75",
    (CONST_STRPTR)   "110",
    (CONST_STRPTR)   "134",
    (CONST_STRPTR)   "150",
    (CONST_STRPTR)   "200",
    (CONST_STRPTR)   "300",
    (CONST_STRPTR)   "600",
    (CONST_STRPTR)  "1200",
    (CONST_STRPTR)  "2400",
    (CONST_STRPTR)  "4800",
    (CONST_STRPTR)  "9600",
    (CONST_STRPTR) "19200",
    (CONST_STRPTR) "38400",
    (CONST_STRPTR) "57600",
    (CONST_STRPTR)"115200",
    (CONST_STRPTR)   NULL
};

CONST_STRPTR StopBitsLabels[] =
{
    (CONST_STRPTR) "1",
    (CONST_STRPTR) "1.5",
    (CONST_STRPTR) "2",
    NULL
};

CONST_STRPTR DataBitsLabels[] =
{
    (CONST_STRPTR) "8",
    (CONST_STRPTR) "7",
    (CONST_STRPTR) "6",
    (CONST_STRPTR) "5",
    NULL
};

CONST_STRPTR BufferSizeLabels[] =
{
    (CONST_STRPTR)  "512",
    (CONST_STRPTR) "1024",
    (CONST_STRPTR) "2048",
    (CONST_STRPTR) "4096",
    NULL
};

/*** Instance Data **********************************************************/

struct SerEditor_DATA
{
    int i;

    CONST_STRPTR ParityLabels[6];
    Object *child;
    Object *baudrate;
    Object *stopbits;
    Object *databits;
    Object *parity;
    Object *inputbuffersize;
    Object *outputbuffersize;
};

STATIC VOID SerPrefs2Gadgets(struct SerEditor_DATA *data);
STATIC VOID Gadgets2SerPrefs(struct SerEditor_DATA *data);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct SerEditor_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *SerEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
#if SHOWICON
    Object *icon;
#endif

    D(bug("[seredit class] SerEdit Class New\n"));

    /*
     * we create self first and then create the child,
     * so we have self->data available already
     */

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, _(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/serial.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Serial",

        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->ParityLabels[0] = _(MSG_PARITY_NONE);
        data->ParityLabels[1] = _(MSG_PARITY_EVEN);
        data->ParityLabels[2] = _(MSG_PARITY_ODD);
        data->ParityLabels[3] = _(MSG_PARITY_MARK);
        data->ParityLabels[4] = _(MSG_PARITY_SPACE);
        data->ParityLabels[5] = NULL;

    #if SHOWPIC
        icon = DtpicObject, MUIA_Dtpic_Name, "PROGDIR:Serial.info", End;

        if (!icon) icon = HVSpace;
    #endif

        data->child =
    #if SHOWPIC
        HGroup,
            Child, icon,
            Child,
    #endif

        VGroup,
            Child, (IPTR)ColGroup(2),
                Child, (IPTR)Label1(_(MSG_GAD_BAUDRATE)),
                Child, (IPTR)(data->baudrate = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)BaudrateLabels,
                End),
                Child, (IPTR)Label1(_(MSG_GAD_DATABITS)),
                Child, (IPTR)(data->databits = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)DataBitsLabels,
                End),
                Child, (IPTR)Label1(_(MSG_GAD_PARITY)),
                Child, (IPTR)(data->parity = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)data->ParityLabels,
                End),
                Child, (IPTR)Label1(_(MSG_GAD_STOPBITS)),
                Child, (IPTR)(data->stopbits = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)StopBitsLabels,
                End),
                Child, (IPTR)Label1(_(MSG_GAD_INPUTBUFFERSIZE)),
                Child, (IPTR)(data->inputbuffersize = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)BufferSizeLabels,
                End),
                Child, (IPTR)Label1(_(MSG_GAD_OUTPUTBUFFERSIZE)),
                Child, (IPTR)(data->outputbuffersize = (Object *)CycleObject,
                    MUIA_Cycle_Entries, (IPTR)BufferSizeLabels,
                End),
            End, /* ColGroup */
#if SHOWPIC
        End,
#endif
        End;

        DoMethod(self, OM_ADDMEMBER, (IPTR) data->child);

        DoMethod
        (
            data->baudrate, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            data->stopbits, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            data->databits, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
            (data->parity, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            data->inputbuffersize, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
        DoMethod
        (
            data->outputbuffersize, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );

        SerPrefs2Gadgets(data);
    }

    return self;
}

/*
 * update struct serialprefs with actual data selected in gadgets
 */
STATIC void Gadgets2SerPrefs (struct SerEditor_DATA *data)
{
    D(bug("Gadgets2SerPrefs\n"));

    serialprefs.sp_BaudRate = atol
    (
        (char *)BaudrateLabels[XGET(data->baudrate, MUIA_Cycle_Active)]
    );

    serialprefs.sp_BitsPerChar = XGET(data->databits, MUIA_Cycle_Active);
    serialprefs.sp_Parity =      XGET(data->parity,MUIA_Cycle_Active);
    serialprefs.sp_StopBits =    XGET(data->stopbits,MUIA_Cycle_Active);

    serialprefs.sp_InputBuffer = atol
    (
        (char *)BufferSizeLabels[XGET(data->inputbuffersize, MUIA_Cycle_Active)]
    );
    serialprefs.sp_OutputBuffer = atol
    (
        (char *)BufferSizeLabels[XGET(data->outputbuffersize, MUIA_Cycle_Active)]
    );

    D(bug("Gadgets2SerPrefs left\n"));
}

/*
 * helper for SerPrefs2Gadgets
 */
STATIC VOID RefreshGadget(Object *obj, ULONG value, CONST_STRPTR *labels)
{
    ULONG i = 0;

    while (NULL != labels[i])
    {
        if (atol((char *) labels[i]) == value)
        {
            NNSET(obj, MUIA_Cycle_Active, i);
            return;
        }
        i++;
    }
    NNSET(obj, MUIA_Cycle_Active, 0);
}

/*
 * update gadgets with values of struct serialprefs
 */
STATIC VOID SerPrefs2Gadgets(struct SerEditor_DATA *data)
{
    RefreshGadget(data->baudrate, serialprefs.sp_BaudRate, BaudrateLabels);

    NNSET(data->databits, MUIA_Cycle_Active, serialprefs.sp_BitsPerChar);
    NNSET(data->parity, MUIA_Cycle_Active, serialprefs.sp_Parity);
    NNSET(data->stopbits, MUIA_Cycle_Active, serialprefs.sp_StopBits);

    RefreshGadget(data->inputbuffersize, serialprefs.sp_InputBuffer, BufferSizeLabels);
    RefreshGadget(data->outputbuffersize, serialprefs.sp_OutputBuffer, BufferSizeLabels);
}

IPTR SerEditor__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[seredit class] SerEdit Class Import\n"));

    success = Prefs_ImportFH(message->fh);
    if (success) SerPrefs2Gadgets(data);

    return success;
}

IPTR SerEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[seredit class] SerEdit Class Export\n"));

    Gadgets2SerPrefs(data);
    success = Prefs_ExportFH(message->fh);

    return success;
}

IPTR SerEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[seredit class] SerEdit Class SetDefaults\n"));

    success = Prefs_Default();
    if (success) SerPrefs2Gadgets(data);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    SerEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);

