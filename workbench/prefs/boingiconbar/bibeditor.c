/*
    Copyright © 2003-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

// #define DEBUG 1
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/asl.h>
#include <proto/dos.h>

#include <stdlib.h>

#include <aros/debug.h>

#include "locale.h"
#include "bibeditor.h"
#include "prefs.h"

/*** Instance Data **********************************************************/

struct BibEditor_DATA
{
    Object *dock_lst, *add_dock_btn, *del_dock_btn;
    Object *dock_name_str;
    Object *program_lst, *add_program_btn, *del_program_btn;
    struct Hook add_dock_hook, del_dock_hook, change_dock_hook;
    struct Hook add_program_hook, del_program_hook, change_program_hook;
};

STATIC VOID BibPrefs2Gadgets(struct BibEditor_DATA *data);
STATIC VOID Gadgets2BibPrefs(struct BibEditor_DATA *data);
STATIC CONST_STRPTR AllocExpandPath(CONST_STRPTR path, CONST_STRPTR file);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct BibEditor_DATA *data = INST_DATA(CLASS, self)

/*** Hook Functions *********************************************************/

AROS_UFH3S(void, add_dock_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[add_dock_func] called\n"));

    struct BibEditor_DATA *data = h->h_Data;

    LONG i;
    STRPTR newentry = (STRPTR)XGET(data->dock_name_str, MUIA_String_Contents);
    LONG dock_count = XGET(data->dock_lst, MUIA_List_Entries);

    if (dock_count < BIB_MAX_DOCKS - 1 && newentry[0] != '\0')
    {
        DoMethod
        (
            data->dock_lst,
            MUIM_List_InsertSingle, newentry, MUIV_List_Insert_Bottom
        );
        strcpy(bibprefs.docks[dock_count].name, newentry);
        for (i = 0; i < BIB_MAX_PROGRAMS; i++)
        {
            bibprefs.docks[dock_count].programs[i][0] = '\0';
        }
        SET(data->dock_lst, MUIA_List_Active, MUIV_List_Active_Bottom);
        SET(obj, MUIA_PrefsEditor_Changed, TRUE);
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, del_dock_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[del_dock_func] called\n"));

    struct BibEditor_DATA *data = h->h_Data;

    LONG current_dock = XGET(data->dock_lst, MUIA_List_Active);
    LONG dock_count = XGET(data->dock_lst, MUIA_List_Entries);

    if( current_dock != MUIV_List_Active_Off)
    {
        int i;

        for (i = current_dock; i < dock_count; i++ )
        {
            if (bibprefs.docks[i + 1].name[0] != '\0')
            {
                memcpy(&bibprefs.docks[i], &bibprefs.docks[i + 1], sizeof(struct Dock));
            }
            else
            {
                bibprefs.docks[i].name[0] = 0;
            }
        }

        for (i = 0; i < BIB_MAX_PROGRAMS; i++ )
        {
            if (bibprefs.docks[dock_count].programs[i][0] != '\0')
            {
                bibprefs.docks[dock_count].programs[i][0] = '\0';
            }
        }
        DoMethod(data->dock_lst, MUIM_List_Remove, current_dock);
        SET(obj, MUIA_PrefsEditor_Changed, TRUE);
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, change_dock_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct BibEditor_DATA *data = h->h_Data;

    LONG i;
    LONG current_dock = XGET(data->dock_lst, MUIA_List_Active);

    D(bug("[change_dock_func] current_dock %d\n", current_dock));

    SET(data->program_lst, MUIA_List_Quiet, TRUE);
    DoMethod(data->program_lst, MUIM_List_Clear);

    if (current_dock != MUIV_List_Active_Off)
    {
        for
        (
            i = 0;
            i < BIB_MAX_PROGRAMS && bibprefs.docks[current_dock].programs[i][0] != '\0';
            i++
        )
        {
            DoMethod
            (
                data->program_lst,
                MUIM_List_InsertSingle, bibprefs.docks[current_dock].programs[i],
                MUIV_List_Insert_Bottom
            );
        }
        SET(data->del_dock_btn, MUIA_Disabled, FALSE);
        SET(data->add_program_btn, MUIA_Disabled, FALSE);
    }
    else
    {
        SET(data->del_dock_btn, MUIA_Disabled, TRUE);
        SET(data->add_program_btn, MUIA_Disabled, TRUE);
    }
    SET(data->program_lst, MUIA_List_Quiet, FALSE);

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, add_program_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[add_program_func] called\n"));

    struct BibEditor_DATA *data = h->h_Data;

    struct FileRequester *freq;
    CONST_STRPTR name = NULL;
    
    LONG current_dock = XGET(data->dock_lst, MUIA_List_Active);
    LONG program_count = XGET(data->program_lst, MUIA_List_Entries);

    if (current_dock ==  MUIV_List_Active_Off)
    {
        D(bug("[IconBarPrefs] Dock not selected, cannot add program!\n"));
        return;
    }

    if (program_count >=  BIB_MAX_PROGRAMS)
    {
        D(bug("[IconBarPrefs] Too many programs in dock!\n"));
        return;
    }

    if ((freq = AllocAslRequestTags(ASL_FileRequest, TAG_END)) != NULL)
    {
        if
        (
            AslRequestTags
            (
                freq,
                ASLFR_TitleText, _(MSG_ADD_P),
                ASLFR_DoPatterns, TRUE,
                ASLFR_RejectIcons, TRUE,
                ASLFR_InitialDrawer, "SYS:",
                TAG_END
            )
        )
        {
            if ((name = AllocExpandPath(freq->fr_Drawer, freq->fr_File)) != NULL)
            {
                DoMethod(data->program_lst, MUIM_List_InsertSingle, name, MUIV_List_Insert_Bottom);
                strcpy(bibprefs.docks[current_dock].programs[program_count], name);
                SET(obj, MUIA_PrefsEditor_Changed, TRUE);
            }
            FreeVec((APTR)name);
        }
        FreeAslRequest(freq);
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, del_program_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[del_program_func] called\n"));

    struct BibEditor_DATA *data = h->h_Data;

    LONG current_dock = XGET(data->dock_lst, MUIA_List_Active);
    LONG delete_program = XGET(data->program_lst, MUIA_List_Active);
    LONG program_count = XGET(data->program_lst, MUIA_List_Entries);

    if ((current_dock != MUIV_List_Active_Off) && (delete_program != MUIV_List_Active_Off))
    {
        LONG i;

        for (i = delete_program; i < program_count; i++ )
        {
            if (bibprefs.docks[current_dock].programs[i + 1][0] != '\0')
            {
                strcpy
                (
                    bibprefs.docks[current_dock].programs[i],
                    bibprefs.docks[current_dock].programs[i + 1]
                );
            }
            else
            {
                bibprefs.docks[current_dock].programs[i][0] = '\0';
            }
        }
        DoMethod(data->program_lst, MUIM_List_Remove, MUIV_List_Remove_Selected);
        SET(obj, MUIA_PrefsEditor_Changed, TRUE);
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, change_program_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct BibEditor_DATA *data = h->h_Data;

    LONG current_program = XGET(data->program_lst, MUIA_List_Active);

    D(bug("[change_dock_func] current_program %d\n", current_program));

    if (current_program != MUIV_List_Active_Off)
    {
        SET(data->del_program_btn, MUIA_Disabled, FALSE);
    }
    else
    {
        SET(data->del_program_btn, MUIA_Disabled, TRUE);
    }

    AROS_USERFUNC_EXIT
}

/*** Methods ****************************************************************/
Object *BibEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    D(bug("[bibedit class] BibEdit Class New\n"));

    Object *dock_lst, *add_dock_btn, *del_dock_btn, *dock_name_str;
    Object *program_lst, *add_program_btn, *del_program_btn;

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, _(MSG_TITLE),
        MUIA_PrefsEditor_Path, (IPTR) "Iconbar.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/BoingIconBar",
        //MUIA_Group_Horiz, TRUE,
        MUIA_Group_Columns, 2,
        Child, VGroup,
            Child, TextObject,
                MUIA_Text_Contents, _(MSG_DOCK),
            End,
            Child, dock_lst = ListviewObject,
                MUIA_Listview_List, ListObject,
                    MUIA_Frame, MUIV_Frame_InputList,
                    MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                    MUIA_List_DestructHook, MUIV_List_DestructHook_String,
                End,
            End,
        End,
        Child, VGroup,
            Child, TextObject,
                MUIA_Text_Contents, _(MSG_PROGRAMS),
            End,
            Child, program_lst = ListviewObject,
                MUIA_Listview_List, ListObject,
                    MUIA_Frame, MUIV_Frame_InputList,
                    MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                    MUIA_List_DestructHook, MUIV_List_DestructHook_String,
                End,
            End,
        End,
        Child, VGroup,
            Child, dock_name_str = StringObject,
                MUIA_String_AdvanceOnCR, TRUE,
                StringFrame,
            End,
            Child, HGroup,
                Child, add_dock_btn = SimpleButton(_(MSG_ADD_DOCK)),
                Child, del_dock_btn = SimpleButton(_(MSG_DEL_DOCK)),
            End,
        End,
        Child, VGroup,
            Child, HVSpace,
            Child, HGroup,
                Child, add_program_btn = SimpleButton(_(MSG_ADD_P)),
                Child, del_program_btn = SimpleButton(_(MSG_DEL_P)),
            End,
        End,
        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->dock_lst = dock_lst;
        data->add_dock_btn = add_dock_btn;
        data->del_dock_btn = del_dock_btn;
        data->dock_name_str = dock_name_str;
        data->program_lst = program_lst;
        data->add_program_btn = add_program_btn;
        data->del_program_btn = del_program_btn;
        data->dock_name_str = dock_name_str;

        data->add_dock_hook.h_Entry = (HOOKFUNC)add_dock_func;
        data->add_dock_hook.h_Data = data;
        data->del_dock_hook.h_Entry = (HOOKFUNC)del_dock_func;
        data->del_dock_hook.h_Data = data;
        data->change_dock_hook.h_Entry = (HOOKFUNC)change_dock_func;
        data->change_dock_hook.h_Data = data;
        data->add_program_hook.h_Entry = (HOOKFUNC)add_program_func;
        data->add_program_hook.h_Data = data;
        data->del_program_hook.h_Entry = (HOOKFUNC)del_program_func;
        data->del_program_hook.h_Data = data;
        data->change_program_hook.h_Entry = (HOOKFUNC)change_program_func;
        data->change_program_hook.h_Data = data;

        SET(data->del_dock_btn, MUIA_Disabled, TRUE);
        SET(data->add_program_btn, MUIA_Disabled, TRUE);
        SET(data->del_program_btn, MUIA_Disabled, TRUE);

        DoMethod
        (
            data->add_dock_btn, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 2, MUIM_CallHook, &data->add_dock_hook
        );

        DoMethod
        (
            data->del_dock_btn, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 2, MUIM_CallHook, &data->del_dock_hook
        );

        DoMethod
        (
            data->dock_lst, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            self, 2, MUIM_CallHook, &data->change_dock_hook
        );

        DoMethod
        (
            data->add_program_btn, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 2, MUIM_CallHook, &data->add_program_hook
        );

        DoMethod
        (
            data->del_program_btn, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 2, MUIM_CallHook, &data->del_program_hook
        );

        DoMethod
        (
            data->program_lst, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            self, 2, MUIM_CallHook, &data->change_program_hook
        );

        BibPrefs2Gadgets(data);
    }

    return self;
}

/*
 * update struct bibprefs with actual data selected in gadgets
 */
STATIC VOID Gadgets2BibPrefs (struct BibEditor_DATA *data)
{
    D(bug("Gadgets2BibPrefs\n"));

    // no-op because we are immediately changing bibprefs
}

/*
 * update gadgets with values of struct bibprefs
 */
STATIC VOID BibPrefs2Gadgets(struct BibEditor_DATA *data)
{
    LONG dock;

    DoMethod(data->dock_lst, MUIM_List_Clear);
    DoMethod(data->program_lst, MUIM_List_Clear);

    for (dock = 0; bibprefs.docks[dock].name[0] != '\0'; dock++)
    {
        DoMethod
        (
            data->dock_lst,
            MUIM_List_InsertSingle, bibprefs.docks[dock].name, MUIV_List_Insert_Bottom
        );
    }
    SET(data->dock_lst, MUIA_List_Active, 0);
}

/* 
 * combine path and file and expand it to an absolute path if necessary
 */
STATIC CONST_STRPTR AllocExpandPath(CONST_STRPTR path, CONST_STRPTR file)
{
    STRPTR result = NULL;
    BPTR lock;

    if ((result = AllocVec(BIB_MAX_PATH, MEMF_ANY | MEMF_CLEAR)) != NULL)
    {
        strlcpy(result, path, BIB_MAX_PATH);
        AddPart(result, file, BIB_MAX_PATH);
        if (strchr(result, ':') == NULL) // relative path
        {
            if ((lock = Lock(result, ACCESS_READ)) != BNULL)
            {
                NameFromLock(lock, result, BIB_MAX_PATH);
                UnLock(lock);
            }
        }
    }
    return result;
}

IPTR BibEditor__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[bibedit class] BibEdit Class Import\n"));

    success = Prefs_ImportFH(message->fh);
    if (success) BibPrefs2Gadgets(data);

    return success;
}

IPTR BibEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[bibedit class] BibEdit Class Export\n"));

    Gadgets2BibPrefs(data);
    success = Prefs_ExportFH(message->fh);

    return success;
}

IPTR BibEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[bibedit class] BibEdit Class SetDefaults\n"));

    success = Prefs_Default();
    if (success) BibPrefs2Gadgets(data);

    return success;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_4
(
    BibEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);

