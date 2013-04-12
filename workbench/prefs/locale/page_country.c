/*
   Copyright  2003-2013, The AROS Development Team. All rights reserved.
   $Id$
*/

#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/alib.h>

#include <stdio.h>

#include <aros/debug.h>

#include "prefs.h"
#include "page_country.h"
#include "registertab.h"

static struct Hook display_hook;

/*** Instance Data **********************************************************/

struct Country_DATA
{
    Object             *me;
    Object             *child;
    Object             *prefs;
    ULONG               active;
};

struct MUI_CustomClass     *Country_CLASS;

/*** Helpers *****************************************************************/

/*************************************************
 * The display function for the KeyTypes listview
 *************************************************/
STATIC VOID country_display_func(struct Hook *h, char **array, struct ListviewEntry *entry)
{
    *array++ = entry->displayflag;
    *array   = entry->node.ln_Name;
}

/*** Methods ****************************************************************
 *
 */

Object *Country__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Country_DATA *data;
    struct TagItem *tstate, *tag;
    struct CountryEntry *entry;

    D(bug("[country class] Country Class New\n"));

    display_hook.h_Entry = HookEntry;
    display_hook.h_SubEntry = (HOOKFUNC)country_display_func;

    /*
     * country flags are at the moment 17 pixels high
     * MUIA_List_MinLineHeight, 19 leaves at least two
     * pixel space between the images
     * If images ever get bigger, this should be
     * no problem.
     */

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,

        InputListFrame,
        MUIA_List_MinLineHeight, 20,
        MUIA_List_Format, "P=\033c,",
        MUIA_List_DisplayHook, &display_hook,
        TAG_DONE
    );

    if (obj == NULL)
    {
        D(bug("ERROR: [country class] DoSuperNewTags failed!\n"));
        return NULL;
    }

    data = INST_DATA(cl, obj);

    tstate = ((struct opSet *)msg)->ops_AttrList;
    while ((tag = (struct TagItem *) NextTagItem((APTR) &tstate)))
    {
        switch (tag->ti_Tag)
        {
            case MUIA_UserData:
                data->prefs = (Object *) tag->ti_Data;
                break;
        }
    }

    data->child = obj;

    ForeachNode(&country_list, entry)
    {
        DoMethod
        (
            data->child,
            MUIM_List_InsertSingle,
            (IPTR)entry,
            MUIV_List_Insert_Bottom
        );
    }

    /* we did remember that */
    NNSET(data->child, MUIA_List_Active, data->active);
    /* changed hook */
    DoMethod(obj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR) data->prefs, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

    data->me = obj;
    return obj;
}

/*** Get ******************************************************************/
static IPTR Country__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Country_DATA *data = INST_DATA(cl, obj);
    struct CountryEntry    *entry;
    IPTR rc;
    IPTR nr = 0;
    ULONG i;


    switch (msg->opg_AttrID)
    {
        case MUIA_Country_Countryname:
            GET(data->child, MUIA_List_Active, &nr);
            rc = -1;
            i  = 0;
            ForeachNode(&country_list, entry)
            {
                if (i == nr)
                {
                    rc = (IPTR)entry->lve.realname;
                }
                i++;
            }

            if (rc == -1)
            {
                *msg->opg_Storage = 0;
                return FALSE;
            }
            break;

        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    *msg->opg_Storage = rc;
    return TRUE;
}

/*** Set ******************************************************************/
static IPTR Country__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Country_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tstate, *tag;
    struct CountryEntry    *entry;
    ULONG update;
    ULONG nr;
    ULONG i;

    tstate = msg->ops_AttrList;
    update = FALSE;

    while ((tag = (struct TagItem *) NextTagItem((APTR) &tstate)))
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Country_Countryname:

                nr = -1;
                i  = 0;
                ForeachNode(&country_list, entry)
                {
                    if (!stricmp(entry->lve.realname, (STRPTR)tag->ti_Data))
                    {
                        nr=i;
                    }
                    i++;
                }

                if (nr < 0)
                {
                    D(bug("ERROR: [country class] could not find >%s< !?\n",tag->ti_Data));
                }
                else
                {
                    NNSET(data->child, MUIA_List_Active, nr);
                    update = TRUE;
                }
                break;

            default:
                return DoSuperMethodA(cl, obj, (Msg)msg);
        }
    }

    if(update)
    {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_3
(
    Country, NULL, MUIC_List, NULL,
    OM_NEW,         struct opSet *,
    OM_SET,         struct opSet *,
    OM_GET,         struct opGet *
);
