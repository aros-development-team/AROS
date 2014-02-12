/*
   Copyright © 2003-2014, The AROS Development Team. All rights reserved.
   $Id$
 */

#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/codesets.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include "locale.h"
#include "prefs.h"
#include "misc.h"
#include "registertab.h"
#include "page_language.h"
#include "languagelist.h"

/*** Instance Data **********************************************************/

struct Language_DATA
{
    int nr_languages;
    int nr_preferred;

    Object *child;
    Object *available;
    Object *preferred;
    Object *charset;
    Object *cslist;
    Object *cslistview;
    Object *popup;
    Object *clear;
    Object *prefs;

    char  **strings_available;
    char  **strings_preferred;
    STRPTR *strings_charsets;

    char  *langpreferred[10]; /* result array for MUIA_Language_Preferred */
};

struct MUI_CustomClass     *Language_CLASS;

static struct Hook hook_available;
static struct Hook hook_preferred;
static struct Hook hook_clear;

/*** Helpers *****************************************************************/

STATIC VOID update_language_lists(struct Language_DATA *data)
{
    struct LanguageEntry *entry;
    int a = 0;
    int p = 0;

    ForeachNode(&language_list, entry)
    {
        if(entry->preferred)
            data->strings_preferred[p++] = entry->lve.node.ln_Name;
        else
            data->strings_available[a++] = entry->lve.node.ln_Name;
    }
    data->strings_available[a] = NULL;
    data->strings_preferred[p] = NULL;
}

STATIC VOID init_language_lists(struct Language_DATA *data)
{

    struct LanguageEntry *entry;
    int i;

    data->nr_languages = 0;
    ForeachNode(&language_list, entry)
    {
        D(bug("[LocalePrefs-LanguageClass]   language '%s' ('%s')\n", entry->lve.node.ln_Name, entry->lve.realname));
        entry->preferred = FALSE;

        /* max 10 preferred langs, see prefs/locale.h */
        for (i = 0; (i < 10) && (entry->preferred == FALSE) && (localeprefs.lp_PreferredLanguages[i][0]); i++)
        {
            if (Stricmp(localeprefs.lp_PreferredLanguages[i], entry->lve.realname) == 0)
            {
                D(bug("[LocalePrefs-LanguageClass]            %s is preferred\n", entry->lve.realname));
                entry->preferred = TRUE;
            }
        }
        data->nr_languages++;
    }

    D(bug("[LocalePrefs-LanguageClass]: nr of languages: %d\n",data->nr_languages));

    data->strings_available = AllocVec(sizeof(char *) * (data->nr_languages+1), MEMF_CLEAR);
    data->strings_preferred = AllocVec(sizeof(char *) * (data->nr_languages+1), MEMF_CLEAR);

    update_language_lists(data);
}

/*** Hooks ******************************************************************
 *
 * hook_func_available
 *   handles double clicks on the available languages
 *
 * hook_func_preferred
 *   handles double clicks on the preferred languages
 *
 * hook_func_clear
 *   handles clearing of preferred languages
 *
 ****************************************************************************/

STATIC VOID func_move_to_selected(char* selstr, struct Language_DATA *data)
{
    struct LanguageEntry *entry = NULL;
    unsigned int i = 0;
    char *test;

    D(bug("[LocalePrefs-LanguageClass] func_move_to_selected('%s')\n", selstr));

    if(selstr) {
        ForeachNode(&language_list, entry)
        {
            if (Stricmp(selstr, entry->lve.realname) == 0)
            {
                DoMethod(data->preferred,
                        MUIM_List_InsertSingle, entry->lve.node.ln_Name,
                        MUIV_List_Insert_Bottom);

                entry->preferred = TRUE;
                break;
            }
        }
    }

    /* we could use
     * DoMethod(data->available,MUIM_List_Remove, MUIV_List_Remove_Active);
     * of course, but I also want to be able to use that for
     * the Set method.
     */

    if (entry)
    {
        GET(data->available, MUIA_List_Entries, &i);
        while(i)
        {
            i--;
            DoMethod(data->available, MUIM_List_GetEntry, i, &test);
            if (Stricmp(entry->lve.node.ln_Name, test) == 0)
            {
                DoMethod(data->available, MUIM_List_Remove, i);
                i = 0;
            }
        }
    }
}

AROS_UFH2
(
    void, hook_func_available,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR,             obj,    A2)
)
{
    AROS_USERFUNC_INIT

    struct Language_DATA *data = hook->h_Data;
    struct LanguageEntry *entry;

    char  *selstr;

    D(bug("[LocalePrefs-LanguageClass] hook_func_available\n"));

    DoMethod(obj,MUIM_List_GetEntry,
            MUIV_List_GetEntry_Active, &selstr);

    ForeachNode(&language_list, entry)
    {
        if (Stricmp(selstr, entry->lve.node.ln_Name) == 0)
        {
            func_move_to_selected(entry->lve.realname, data);
            break;
        }
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH2
(
    void, hook_func_preferred,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR,             obj,    A2)
)
{
    AROS_USERFUNC_INIT

    struct Language_DATA *data= hook->h_Data;
    char  *selstr;
    struct LanguageEntry *entry;

    D(bug("[LocalePrefs-LanguageClass] hook_func_preferred\n"));

    DoMethod(obj,MUIM_List_GetEntry,
            MUIV_List_GetEntry_Active, &selstr);

    if(selstr)
    {
        D(bug("move: %s\n",selstr));

        ForeachNode(&language_list, entry)
        {
            if (Stricmp(selstr, entry->lve.node.ln_Name) == 0)
            {
                DoMethod(data->available,
                        MUIM_List_InsertSingle, entry->lve.node.ln_Name,
                        MUIV_List_Insert_Sorted);
                entry->preferred = FALSE;
            }
        }
        DoMethod(obj, MUIM_List_Remove, MUIV_List_Remove_Active);
    }

    AROS_USERFUNC_EXIT
}

STATIC VOID func_clear(struct Language_DATA *data)
{
    struct LanguageEntry *entry;

    D(bug("[LocalePrefs-LanguageClass] func_clear\n"));

    /* clear it */
    DoMethod(data->preferred, MUIM_List_Clear);

    /* add all old preferred languages again */
    DoMethod(data->available, MUIM_Group_InitChange);
    ForeachNode(&language_list, entry)
    {
        if(entry->preferred)
        {
            entry->preferred = FALSE;
            DoMethod(data->available,
                    MUIM_List_InsertSingle, entry->lve.node.ln_Name,
                    MUIV_List_Insert_Bottom);
        }
    }
    DoMethod(data->available, MUIM_List_Sort);
    DoMethod(data->available, MUIM_Group_ExitChange);

}

AROS_UFH2(
        void, hook_func_clear,
        AROS_UFHA(struct Hook *,    hook,   A0),
        AROS_UFHA(APTR *,           obj,    A2)
        )
{
    AROS_USERFUNC_INIT

    struct Language_DATA *data = hook->h_Data;

    func_clear(data);
    SET(data->prefs, MUIA_PrefsEditor_Changed, TRUE);

    AROS_USERFUNC_EXIT
}

AROS_UFH3
(
    void, charset_popup_to_string,
    AROS_UFHA(struct Hook*, hook, A0),
    AROS_UFHA(Object *, list, A2),
    AROS_UFHA(Object *, string, A1)
)
{
    AROS_USERFUNC_INIT

    STRPTR listentry;
    STRPTR oldentry;

    GetAttr(MUIA_Text_Contents, string, (IPTR *)&oldentry);
    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&listentry);
    if (Stricmp(listentry, oldentry))
    {
        SetAttrs(string, MUIA_Text_Contents, listentry, TAG_DONE);
        SetAttrs(hook->h_Data, MUIA_PrefsEditor_Changed, TRUE, TAG_DONE);
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3
(
    ULONG, charset_string_to_popup,
    AROS_UFHA(struct Hook*, hook, A0),
    AROS_UFHA(Object *, list, A2),
    AROS_UFHA(Object *, str, A1)
)
{
    AROS_USERFUNC_INIT

    STRPTR strtext, listentry;
    LONG index;

    GetAttr(MUIA_Text_Contents, str, (IPTR *)&strtext);

    for(index = 0; ; index++)
    {
        DoMethod(list, MUIM_List_GetEntry, index, (IPTR)&listentry);

        if (!listentry)
        {
            SET(list, MUIA_List_Active, MUIV_List_Active_Off);
            break;
        }

        if (Stricmp(strtext, listentry) == 0)
        {
            SET(list, MUIA_List_Active, index);
            break;
        }
    }

    return TRUE;

    AROS_USERFUNC_EXIT
}

static struct Hook charset_popup_to_string_hook =
{
    {NULL, NULL},
    (HOOKFUNC)AROS_ASMSYMNAME(charset_popup_to_string),
    NULL,
    NULL
};

static struct Hook charset_string_to_popup_hook =
{
    {NULL, NULL},
    (HOOKFUNC)AROS_ASMSYMNAME(charset_string_to_popup),
    NULL,
    NULL
};

/*** Methods ****************************************************************
 *
 */

static void free_strings(struct Language_DATA *data)
{
    if (data->strings_charsets)
    {
        CodesetsFreeA(data->strings_charsets, NULL);
        data->strings_charsets = NULL;
    }
    if(data->strings_available)
    {
        FreeVec(data->strings_available);
        data->strings_available = NULL;
    }

    if(data->strings_preferred)
    {
        FreeVec(data->strings_preferred);
        data->strings_preferred = NULL;
    }

}

static Object *handle_New_error(Object *obj, struct IClass *cl, char *error)
{
    struct Language_DATA *data;

    ShowMessage(error);
    D(bug("[LocalePrefs-LanguageClass] %s\n"));

    if(!obj)
        return NULL;

    data = INST_DATA(cl, obj);

    if(data->clear)
    {
        DisposeObject(data->clear);
        data->clear=NULL;
    }

    if(data->available)
    {
        DisposeObject(data->available);
        data->available=NULL;
    }

    if(data->preferred)
    {
        DisposeObject(data->preferred);
        data->preferred = NULL;
    }

    if(data->child)
    {
        DisposeObject(data->child);
        data->child = NULL;
    }

    if (data->charset) {
        DisposeObject(data->charset);
        data->charset = NULL;
    }

    if (data->cslist) {
        DisposeObject(data->cslist);
        data->cslist = NULL;
    }

    if (data->cslistview) {
        DisposeObject(data->cslistview);
        data->cslist = NULL;
    }

    if (data->popup) {
        DisposeObject(data->popup);
        data->popup = NULL;
    }

    free_strings(data);

    CoerceMethod(cl, obj, OM_DISPOSE);
    return NULL;
}

Object *Language__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Language_DATA *data;
    struct TagItem *tstate, *tag;
    struct Object *avail_list, *pref_list;

    D(bug("[LocalePrefs-LanguageClass] Language Class New\n"));

    /*
     * we create self first and then create the child,
     * so we have self->data available already
     */

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
        TAG_DONE
    );

    if (obj == NULL)
    {
        return handle_New_error(obj, cl, "ERROR: Unable to create object!\n");
    }

    data = INST_DATA(cl, obj);

    tstate=((struct opSet *)msg)->ops_AttrList;
    while ((tag = (struct TagItem *) NextTagItem((APTR) &tstate)))
    {
        switch (tag->ti_Tag)
        {
            case MUIA_UserData:
                data->prefs = (Object *) tag->ti_Data;
                break;
        }
    }

    if(!data->prefs)
        return handle_New_error(obj, cl, "ERROR: MA_PrefsObject not supplied!\n");

    init_language_lists(data);
    data->strings_charsets = CodesetsSupportedA(NULL);

    data->clear = MUI_MakeObject(MUIO_Button, __(MSG_GAD_CLEAR_LANGUAGES));

    data->available = ListviewObject, MUIA_Listview_List,
        avail_list = NewObject(Languagelist_CLASS->mcc_Class, 0,
            InputListFrame,
            MUIA_Draggable, TRUE,
            MUIA_List_SourceArray, data->strings_available,
            TAG_DONE),
        End;

    data->preferred = ListviewObject, MUIA_Listview_List,
        pref_list = NewObject(Languagelist_CLASS->mcc_Class, 0,
            InputListFrame,
            MUIA_Draggable, TRUE,
            MUIA_List_SourceArray, data->strings_preferred,
            TAG_DONE),
        End;

    set(pref_list, MUIA_UserData, avail_list);
    set(avail_list, MUIA_UserData, pref_list);

    data->charset = TextObject,
        TextFrame,
        MUIA_Background, MUII_TextBack,
    End;

    data->cslist = ListObject,
        MUIA_Frame, MUIV_Frame_InputList,
        MUIA_Background, MUII_ListBack,
        MUIA_List_AutoVisible, TRUE,
        MUIA_List_SourceArray, data->strings_charsets,
    End;

    data->cslistview = ListviewObject,
        MUIA_Listview_List, data->cslist,
    End;

    charset_popup_to_string_hook.h_Data = data->prefs;

    data->popup = PopobjectObject,
        MUIA_Popobject_Object, data->cslistview,
        MUIA_Popobject_StrObjHook, &charset_string_to_popup_hook,
        MUIA_Popobject_ObjStrHook, &charset_popup_to_string_hook,
        MUIA_Popstring_Button, PopButton(MUII_PopUp),
        MUIA_Popstring_String, data->charset,
    End;

    if(!data->clear || !data->available || !data->preferred || !data->charset || !data->cslist ||
            !data->cslistview || !data->popup)
        return handle_New_error(obj,cl,"ERROR: MakeObject failed\n");

    DoMethod(data->cslist, MUIM_List_Sort);
    DoMethod(data->cslist, MUIM_List_InsertSingle, _(MSG_NOT_SPECIFIED), MUIV_List_Insert_Top);

    data->child = VGroup,
        Child, HGroup,
            Child, VGroup, /* Available Languages */
                Child, CLabel1(_(MSG_GAD_AVAIL_LANGUAGES)),
                Child, data->available,
            End,
            Child, VGroup, /* Preferred Languages */
                Child, CLabel1(_(MSG_GAD_PREF_LANGUAGES)),
                Child, data->preferred,
            End,
        End,
        Child, data->clear,
        Child, HGroup,
            Child, CLabel1(_(MSG_CHARACTER_SET)),
            Child, data->popup,
        End,
    End;

    if(!data->child)
        return handle_New_error(obj, cl, "ERROR: create child failed\n");

    /* add to self */
    DoMethod(obj, OM_ADDMEMBER, data->child);

    /* setup hooks */
    DoMethod(data->cslistview, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, data->popup, 2, MUIM_Popstring_Close, TRUE);

    /* move hooks */
    hook_available.h_Entry    = (HOOKFUNC) hook_func_available;
    hook_available.h_Data     = data ;
    DoMethod
    (
        data->available, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime, data->available,
        2, MUIM_CallHook, (IPTR) &hook_available
    );

    hook_preferred.h_Entry    = (HOOKFUNC) hook_func_preferred;
    hook_preferred.h_Data     = data ;
    DoMethod
    (
        data->preferred, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime, data->preferred,
        2, MUIM_CallHook, (IPTR) &hook_preferred
    );

    /* clear hook */
    hook_clear.h_Entry    = (HOOKFUNC) hook_func_clear;
    hook_clear.h_Data     = data ;
    DoMethod
    (
        data->clear, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, data->preferred,
        2, MUIM_CallHook, (IPTR) &hook_clear
    );

    /* changed hooks */
    DoMethod
    (
        pref_list, MUIM_Notify, MUIA_List_Entries, MUIV_EveryTime,
        (IPTR) data->prefs, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
    );

    return obj;
}

/*** Get ******************************************************************/

static IPTR Language__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Language_DATA *data = INST_DATA(cl, obj);
    IPTR rc;
    IPTR i;

    switch (msg->opg_AttrID)
    {
        case MUIA_Language_Preferred: /* return array of preferred language strings */
            memset(data->langpreferred, 0, sizeof(data->langpreferred));
            for (i = 0; i < 10; i++)
            {
                struct LanguageEntry *entry;
                char *langNative = NULL;

                DoMethod(data->preferred, MUIM_List_GetEntry, i, &langNative);
                D(bug("[LocalePrefs-LanguageClass] Get: MUIA_Language_Preferred %02d = '%s'\n", i, langNative));
                ForeachNode(&language_list, entry)
                {
                    if(Stricmp(langNative, entry->lve.node.ln_Name) == 0)
                    {
                        data->langpreferred[i] = entry->lve.realname;
                        D(bug("[LocalePrefs-LanguageClass] Get:                   BaseName = '%s'\n", data->langpreferred[i]));
                        break;
                    }
                 }
            }
            rc = (IPTR) data->langpreferred;
            break;
        case MUIA_Language_Characterset:
            GetAttr(MUIA_List_Active, data->cslist, (IPTR *)&i);
            D(bug("[LocalePrefs-LanguageClass] Get: MUIA_Language_Characterset = %d\n", i));
            if ((i == 0) || (i == MUIV_List_Active_Off))
                *msg->opg_Storage = 0;
            else
                GetAttr(MUIA_Text_Contents, data->charset, msg->opg_Storage);
            return TRUE;
        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    *msg->opg_Storage = rc;
    return TRUE;
}

/*** Set ******************************************************************/
static IPTR Language__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Language_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tstate, *tag;
    ULONG update;
    ULONG i;

    tstate = msg->ops_AttrList;
    update = FALSE;

    while ((tag = (struct TagItem *) NextTagItem((APTR) &tstate)))
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Language_Preferred:
                /* clear preferred */
                func_clear(data);
                for (i = 0; i < 10; i++)
                {
                    if(localeprefs.lp_PreferredLanguages[i][0] != '\0')
                    {
                        D(bug("[LocalePrefs-LanguageClass] OM_SET: MUIA_Language_Preferred %d = '%s'\n", i, localeprefs.lp_PreferredLanguages[i]));
                        func_move_to_selected(localeprefs.lp_PreferredLanguages[i], data);
                    }
                }

                update = TRUE;
                break;
            case MUIA_Language_Characterset:
                {
                    char *charset = (char *)tag->ti_Data;

                    D(bug("[LocalePrefs-LanguageClass] OM_SET: MUIA_Language_Characterset = '%s'\n", i, charset));

                    if (!charset || !*charset)
                        DoMethod(data->cslist, MUIM_List_GetEntry, 0, (IPTR *)&charset);
                    SetAttrs(data->charset, MUIA_Text_Contents, charset, TAG_DONE);
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

static IPTR Language__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Language_DATA *data = INST_DATA(cl, obj);

    D(bug("[LocalePrefs-LanguageClass] OM_DISPOSE()\n"));

    free_strings(data);

    return DoSuperMethodA(cl, obj, msg);
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    Language, NULL, MUIC_Group, NULL,
    OM_NEW, struct opSet *,
    OM_SET, struct opSet *,
    OM_GET, struct opGet *,
    OM_DISPOSE, Msg
);
