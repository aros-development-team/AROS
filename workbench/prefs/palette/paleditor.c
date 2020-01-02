/*
    Copyright  2010-2020, The AROS Development Team. All rights reserved.
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
#include "palette.h"
#include "prefs.h"

STATIC CONST_STRPTR pennames[MAXPENS + 1];

/*
    without this initial palette we can't later set MUIA_Palette_Entries.
    This is probably a bug in Zune.
*/
STATIC const struct MUI_Palette_Entry initialpens[MAXPENS + 1] =
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
    Object                      *palpe_palette;
    struct MUI_Palette_Entry    *pens;
};

STATIC VOID PalPrefs2Gadgets(struct PalEditor_DATA *data);
STATIC VOID Gadgets2PalPrefs(struct PalEditor_DATA *data);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct PalEditor_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *PalEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *pale = NULL, *palpe_palette;
    struct MUI_Palette_Entry *pens;

    pennames[0] = _(MSG_PEN0);
    pennames[1] = _(MSG_PEN1);
    pennames[2] = _(MSG_PEN2);
    pennames[3] = _(MSG_PEN3);
    pennames[4] = _(MSG_PEN4);
    pennames[5] = _(MSG_PEN5);
    pennames[6] = _(MSG_PEN6);
    pennames[7] = _(MSG_PEN7);
    pennames[8] = NULL;

    pens = AllocMem(sizeof(struct MUI_Palette_Entry) * (MAXPENS + 1), MEMF_CLEAR);
    D(bug("[PaletteEditor] %s: pens @ 0x%p\n", __func__, pens);)
    if (!pens)
        return NULL;

    CopyMem(initialpens, pens, sizeof(struct MUI_Palette_Entry) * (MAXPENS + 1));

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, __(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/palette.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Palette",
        Child, HGroup,
            Child, (IPTR)(palpe_palette = (Object *)PEPaletteObject,
                MUIA_Palette_Entries, (IPTR)pens,
                MUIA_Palette_Names, (IPTR)pennames,
            End),
        End,
        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->palpe_palette = palpe_palette;
        data->pens = pens;

        D(
            bug("[PaletteEditor] %s: editor obj @ 0x%p\n", __func__, self);
            bug("[PaletteEditor] %s: palette obj @ 0x%p\n", __func__, data->palpe_palette);
        )        
        DoMethod
        (
            data->palpe_palette, MUIM_Notify, MUIA_Palette_Entries, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
        );
    }

    return self;
}

IPTR PalEditor__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    D(bug("[PaletteEditor] %s: editor obj @ 0x%p\n", __func__, self);)
    if (data->pens)
        FreeMem(data->pens, sizeof(struct MUI_Palette_Entry) * (MAXPENS + 1));
    return DoSuperMethodA(CLASS, self, message);
}

STATIC VOID Gadgets2PalPrefs (struct PalEditor_DATA *data)
{
    LONG i;
    struct MUI_Palette_Entry *pallpens =
        (struct MUI_Palette_Entry *)XGET(data->palpe_palette, MUIA_Palette_Entries);

    for (i = 0; i < MAXPENS; i++)
    {
        paletteprefs.pap_Colors[i].ColorIndex = pallpens[i].mpe_ID;
        paletteprefs.pap_Colors[i].Red = pallpens[i].mpe_Red >> 16;
        paletteprefs.pap_Colors[i].Green = pallpens[i].mpe_Green >> 16;
        paletteprefs.pap_Colors[i].Blue = pallpens[i].mpe_Blue >> 16;
        D(bug("[PaletteEditor] %s: #%02d r:%04x g:%04x b:%04x\n", __func__, i, paletteprefs.pap_Colors[i].Red, paletteprefs.pap_Colors[i].Green, paletteprefs.pap_Colors[i].Blue);)
    }
}

STATIC VOID PalPrefs2Gadgets(struct PalEditor_DATA *data)
{
    LONG i;
    struct MUI_Palette_Entry *prefpens = data->pens;

    if (prefpens)
    {
        D(bug("[PaletteEditor] %s: pens @ 0x%p\n", __func__, prefpens);)
        for (i = 0; i < MAXPENS; i++)
        {
            prefpens[i].mpe_ID = paletteprefs.pap_Colors[i].ColorIndex;
            prefpens[i].mpe_Red = paletteprefs.pap_Colors[i].Red << 16 | paletteprefs.pap_Colors[i].Red;
            prefpens[i].mpe_Green = paletteprefs.pap_Colors[i].Green << 16 | paletteprefs.pap_Colors[i].Green;
            prefpens[i].mpe_Blue = paletteprefs.pap_Colors[i].Blue << 16 | paletteprefs.pap_Colors[i].Blue;
            D(bug("[PaletteEditor] %s: #%02d r:%08x g:%08x b:%08x\n", __func__, i, prefpens[i].mpe_Red, prefpens[i].mpe_Green, prefpens[i].mpe_Blue);)
        }
        prefpens[8].mpe_ID = MUIV_Palette_Entry_End;
        NNSET(data->palpe_palette, MUIA_Palette_Entries, prefpens);
    }
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
ZUNE_CUSTOMCLASS_5
(
    PalEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    OM_DISPOSE,                   Msg,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);
