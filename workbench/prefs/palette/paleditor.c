/*
    Copyright  2010-2018, The AROS Development Team. All rights reserved.
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

#include <proto/alib.h>
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

STATIC CONST_STRPTR pennames[9];

/*
    without this initial palette we can't later set MUIA_Palette_Entries.
    This is probably a bug in Zune.
*/
STATIC const struct MUI_Palette_Entry initialpens[9] =
{
    {0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0},
    {2, 0, 0, 0, 0},
    {3, 0, 0, 0, 0},
    {4, 0, 0, 0, 0},
    {5, 0, 0, 0, 0},
    {6, 0, 0, 0, 0},
    {7, 0, 0, 0, 0},
    {MUIV_Palette_Entry_End, 0, 0, 0, 0}
};

/*** Instance Data **********************************************************/
struct PalEditor_DATA
{
    Object                     *palpe_palette;
};

STATIC VOID PalPrefs2Gadgets(struct PalEditor_DATA *data);
STATIC VOID Gadgets2PalPrefs(struct PalEditor_DATA *data);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct PalEditor_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *PalEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *palpe_palette;

    pennames[0] = _(MSG_PEN0);
    pennames[1] = _(MSG_PEN1);
    pennames[2] = _(MSG_PEN2);
    pennames[3] = _(MSG_PEN3);
    pennames[4] = _(MSG_PEN4);
    pennames[5] = _(MSG_PEN5);
    pennames[6] = _(MSG_PEN6);
    pennames[7] = _(MSG_PEN7);
    pennames[8] = NULL;

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, __(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/palette.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Palette",
        Child, HGroup,
            Child, (IPTR)(palpe_palette = (Object *)PaletteObject,
                MUIA_Palette_Entries, (IPTR)initialpens,
                MUIA_Palette_Names, (IPTR)pennames,
            End),
        End,
        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->palpe_palette = palpe_palette;

        DoMethod
        (
            data->palpe_palette, MUIM_Notify, MUIA_Palette_Entries, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
    }

    return self;
}

STATIC VOID Gadgets2PalPrefs (struct PalEditor_DATA *data)
{
    LONG i;
    struct MUI_Palette_Entry *currentpens =
        (struct MUI_Palette_Entry *)XGET(data->palpe_palette, MUIA_Palette_Entries);

    for (i = 0; i < 8; i++)
    {
        paletteprefs.pap_Colors[i].ColorIndex = currentpens[i].mpe_ID;
        paletteprefs.pap_Colors[i].Red = currentpens[i].mpe_Red >> 16;
        paletteprefs.pap_Colors[i].Green = currentpens[i].mpe_Green >> 16;
        paletteprefs.pap_Colors[i].Blue = currentpens[i].mpe_Blue >> 16;
    }
}

STATIC VOID PalPrefs2Gadgets(struct PalEditor_DATA *data)
{
    LONG i;
    struct MUI_Palette_Entry currentpens[9];

    for (i = 0; i < 8; i++)
    {
        currentpens[i].mpe_ID = paletteprefs.pap_Colors[i].ColorIndex;
        currentpens[i].mpe_Red = paletteprefs.pap_Colors[i].Red << 16;
        currentpens[i].mpe_Green = paletteprefs.pap_Colors[i].Green << 16;
        currentpens[i].mpe_Blue = paletteprefs.pap_Colors[i].Blue << 16;
    }
    currentpens[8].mpe_ID = MUIV_Palette_Entry_End;
    NNSET(data->palpe_palette, MUIA_Palette_Entries, currentpens);
}

IPTR PalEditor__MUIM_PrefsEditor_ImportFH
(
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
