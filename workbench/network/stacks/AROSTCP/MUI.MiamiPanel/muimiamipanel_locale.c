/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/locale.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/MUI.miamipanel.catalog"
#define CATALOG_VERSION  0

#include <libraries/gadtools.h>
#include <mui/TheBar_mcc.h>

#include "muimiamipanel_intern.h"

/*** Functions **************************************************************/
/* Main *********************************************************************/
CONST_STRPTR _(ULONG id, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    if (LocaleBase != NULL && MiamiPanelBaseIntern->mpb_cat != NULL)
    {
	return GetCatalogStr(MiamiPanelBaseIntern->mpb_cat, id, CatCompArray[id].cca_Str);
    } 
    else 
    {
	return CatCompArray[id].cca_Str;
    }
}

void
localizeArray(UBYTE **strings,ULONG *ids, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
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
localizeMenus(struct NewMenu *menu,ULONG *ids, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
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
localizeButtonsBar(struct MUIS_TheBar_Button *buttons,ULONG *ids, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    while (buttons->img!=MUIV_TheBar_End)
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
    if(LocaleBase != NULL && MiamiPanelBaseIntern->mpb_cat != NULL)
	{
		CloseCatalog(MiamiPanelBaseIntern->mpb_cat);
		MiamiPanelBaseIntern->mpb_cat = NULL;
	}
}

