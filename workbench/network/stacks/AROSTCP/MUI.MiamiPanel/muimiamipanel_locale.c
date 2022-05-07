/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/MUI.miamipanel.catalog"
#include "catalogs/catalog_version.h"

#include <libraries/gadtools.h>
#include <mui/TheBar_mcc.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"

/*** Functions **************************************************************/
/* Main *********************************************************************/
CONST_STRPTR _(ULONG id, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    CONST_STRPTR retstr = NULL;
    int i = 0;

    while (CatCompArray[i].cca_ID != 0)
    {
        if (CatCompArray[i].cca_ID == id)
        {
            retstr = CatCompArray[i].cca_Str;
            break;
        }
        i++;
    }
    if ((LocaleBase != NULL) && (MiamiPanelBaseIntern->mpb_cat != NULL))
    {
        retstr = GetCatalogStr(MiamiPanelBaseIntern->mpb_cat, id, retstr);
        return retstr;
    }
    return retstr;
}

void
localizeArray(CONST_STRPTR *strings, ULONG *ids, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    for (;;)
    {
        if (*ids) *strings++ = _(*ids++, MiamiPanelBaseIntern);
        else
        {
            *strings = NULL;
            break;
        }
    }
}

/***********************************************************************/

void
localizeMenus(struct NewMenu *menu, ULONG *ids, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    while (menu->nm_Type!=NM_END)
    {
        register ULONG id = *ids++;

        if (id && menu->nm_Label!=NM_BARLABEL)
            menu->nm_Label = _(id, MiamiPanelBaseIntern);

        menu++;
    }
}

/***********************************************************************/

void
localizeButtonsBar(struct MUIS_TheBar_Button *buttons, ULONG *ids, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    while (buttons->img != MUIV_TheBar_End)
    {
        register ULONG t = *ids++;
        register ULONG h = *ids++;

        if (t) buttons->text = _(t, MiamiPanelBaseIntern);
        if (h) buttons->help = _(h, MiamiPanelBaseIntern);

        buttons++;
    }
}

/* Setup ********************************************************************/
VOID Locale_Initialize(struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    if (LocaleBase != NULL)
    {
        MiamiPanelBaseIntern->mpb_cat = OpenCatalog
        (
            NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE
        );
    }
    else
    {
        MiamiPanelBaseIntern->mpb_cat = NULL;
    }
}

VOID Locale_Deinitialize(struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    if ((LocaleBase != NULL) && (MiamiPanelBaseIntern->mpb_cat != NULL))
    {
        CloseCatalog(MiamiPanelBaseIntern->mpb_cat);
        MiamiPanelBaseIntern->mpb_cat = NULL;
    }
}

