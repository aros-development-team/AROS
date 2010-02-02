/*
    Copyright  2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/pointer.h>
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
#include "pteditor.h"
#include "prefs.h"

/*** Instance Data **********************************************************/
struct PTEditor_DATA
{
    Object *child;
};

STATIC VOID PTPrefs2Gadgets(struct PTEditor_DATA *data);
STATIC VOID Gadgets2PTPrefs(struct PTEditor_DATA *data);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct PTEditor_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *PTEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    D(bug("[seredit class] SerEdit Class New\n"));

    /*
     * we create self first and then create the child,
     * so we have self->data available already
     */

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, _(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/pointer.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Pointer",

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
#endif

        PTPrefs2Gadgets(data);
    }

    return self;
}


STATIC void Gadgets2PTPrefs(struct PTEditor_DATA *data)
{
#if 0
    serialprefs.sp_BitsPerChar = XGET(data->databits, MUIA_Cycle_Active);
#endif
}

STATIC VOID PTPrefs2Gadgets(struct PTEditor_DATA *data)
{
#if 0
    NNSET(data->databits, MUIA_Cycle_Active, serialprefs.sp_BitsPerChar);
#endif
}

IPTR PTEditor__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    success = Prefs_ImportFH(message->fh);
    if (success) PTPrefs2Gadgets(data);

    return success;
}

IPTR PTEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[seredit class] SerEdit Class Export\n"));

    Gadgets2PTPrefs(data);
    success = Prefs_ExportFH(message->fh);

    return success;
}

IPTR PTEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[seredit class] SerEdit Class SetDefaults\n"));

    success = Prefs_Default();
    if (success) PTPrefs2Gadgets(data);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    PTEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);
