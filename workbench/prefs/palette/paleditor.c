/*
    Copyright  2010-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/palette.h>
#include <prefs/prefhdr.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
    {DETAILPEN,                 0, 0, 0, MSG_PEN0},
    {BLOCKPEN,                  0, 0, 0, MSG_PEN1},
    {TEXTPEN,                   0, 0, 0, MSG_PEN2},
    {SHINEPEN,                  0, 0, 0, MSG_PEN3},
    {SHADOWPEN,                 0, 0, 0, MSG_PEN4},
    {FILLPEN,                   0, 0, 0, MSG_PEN5},
    {FILLTEXTPEN,               0, 0, 0, MSG_PEN6},
    {BACKGROUNDPEN,             0, 0, 0, MSG_PEN7},
    {HIGHLIGHTTEXTPEN,          0, 0, 0, MSG_PEN8},
#if (MAXPENS > 8)
    {BARDETAILPEN,              0, 0, 0, MSG_PEN9},
    {BARBLOCKPEN,               0, 0, 0, MSG_PEN10},
    {BARTRIMPEN,                0, 0, 0, MSG_PEN11},
#endif
    {MUIV_Palette_Entry_End,    0, 0, 0, 0}
};

/*** Instance Data **********************************************************/
struct PalEditor_DATA
{
    Object                      *palpe_palette;
    struct MUI_Palette_Entry    *pens;
    UWORD                       *origcols;
    UWORD                       count;
    BOOL                        restore;
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
    int i;

    /* Initialise the palette information .. */
    pens = AllocMem(sizeof(struct MUI_Palette_Entry) * (MAXPENS + 1), MEMF_ANY);
    D(bug("[PaletteEditor] %s: pens @ 0x%p\n", __func__, pens);)
    if (!pens)
        return NULL;
    CopyMem(initialpens, pens, sizeof(struct MUI_Palette_Entry) * (MAXPENS + 1));
    for (i = 0; i < MAXPENS; i++)
    {
        pennames[i] = _(pens[i].mpe_Group);
    }
    pennames[MAXPENS] = NULL;

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

    D(bug("[PaletteEditor] %s(0x%p)\n", __func__, message->fh));

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

    D(bug("[PaletteEditor] %s(0x%p)\n", __func__, message->fh));

    Gadgets2PalPrefs(data);
    success = Prefs_ExportFH(message->fh);
    if ((success) && (data->origcols) && (data->count > 0))
    {
        UWORD i;

        D(bug("[PaletteEditor] %s: updating %d entries ...\n", __func__, data->count);)
        for (i = 0; i < data->count; i++)
        {
            data->origcols[i] = GetRGB4(_screen(self)->ViewPort.ColorMap, i);
        }
    }
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


IPTR PalEditor__MUIM_Setup
(
    Class *CLASS, Object *self, struct MUIP_Setup *message
)
{
    SETUP_INST_DATA;
    UWORD i;

    if (!(DoSuperMethodA(CLASS, self, (Msg) message)))
        return 0;

    /* Backup the original colors used by the screen */
    data->count = _screen(self)->ViewPort.ColorMap->Count;

    D(bug("[PaletteEditor] %s: screen @ 0x%p\n", __func__, _screen(self));)
    if (data->count > 0)
    {
        D(bug("[PaletteEditor] %s: backing up %d entries ...\n", __func__, data->count);)

        data->origcols = AllocVec(sizeof(UWORD) * data->count, MEMF_CLEAR);
        data->restore = TRUE;
        for (i = 0; i < data->count; i++)
        {
            data->origcols[i] = GetRGB4(_screen(self)->ViewPort.ColorMap, i);
        }
    }
    return 1;
}


IPTR PalEditor__MUIM_Cleanup
(
    Class *CLASS, Object *self, struct MUIP_Cleanup *message
)
{
    SETUP_INST_DATA;

    if ((data->origcols) && (data->count > 0))
    {
        if (data->restore)
        {
            D(bug("[PaletteEditor] %s: restoring %d entries ...\n", __func__, data->count);)
            LoadRGB4(&_screen(self)->ViewPort, data->origcols, data->count);   
        }
        data->restore = FALSE;
        data->count = 0;
        FreeVec(data->origcols);
        data->origcols = NULL;
    }
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_7
(
    PalEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    OM_DISPOSE,                   Msg,
    MUIM_Setup,                 struct MUIP_Setup *,
    MUIM_Cleanup,               struct MUIP_Cleanup *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);
