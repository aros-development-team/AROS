/*
    Copyright  2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/palette.h>
#include <prefs/prefhdr.h>
/* #define DEBUG 1 */
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <aros/debug.h>

#include "locale.h"
#include "paleditor.h"
#include "prefs.h"

/*** Instance Data **********************************************************/

struct PalEditor_DATA
{

};

STATIC VOID PalPrefs2Gadgets(struct PalEditor_DATA *data);
STATIC VOID Gadgets2PalPrefs(struct PalEditor_DATA *data);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct PalEditor_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *PalEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, _(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/palette.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Palette",
        Child, HGroup,
        End,
        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

#if 0
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

#endif
    }

    return self;
}

STATIC VOID Gadgets2PalPrefs (struct PalEditor_DATA *data)
{
    //serialprefs.sp_BitsPerChar = XGET(data->databits, MUIA_Cycle_Active);
}

STATIC VOID PalPrefs2Gadgets(struct PalEditor_DATA *data)
{
    //NNSET(data->databits, MUIA_Cycle_Active, serialprefs.sp_BitsPerChar);
}

IPTR PalEditor__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_ImportFH(message->fh);
    if (success) PalPrefs2Gadgets(data);

    return success;
}

IPTR PalEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    Gadgets2PalPrefs(data);
    success = Prefs_ExportFH(message->fh);

    return success;
}

IPTR PalEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_Default();
    if (success) PalPrefs2Gadgets(data);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    PalEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);
