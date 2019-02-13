#include <aros/debug.h>
#include <proto/alib.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <aros/asmcall.h>
#include <utility/hooks.h>
#include <libraries/mui.h>

#define NO_INLINE_STDARG

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/alib.h>

#include <stddef.h>
#include <string.h>

#include "fontlist_class.h"
#include "globals.h"

struct FontListData
{
    struct MinList  ScanDirTasks;
};
typedef struct FontListData FontListData;

struct ScanDirTaskInfo
{
    struct MinNode  Node;
    struct Process  *Proc;
    STRPTR          DirName;
    char            NameBuf[256];
    char            Buffer[4096];
};


AROS_UFH3(APTR, flConstructFunc,
                AROS_UFHA(struct Hook *, hook, A0),
        AROS_UFHA(APTR, pool, A2),
        AROS_UFHA(struct MUIS_FontList_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct MUIS_FontList_Entry *new_entry;
    size_t len1 = strlen(entry->FileName) + 1;
    size_t len2 = strlen(entry->FamilyName) + 1;
    size_t len3 = strlen(entry->StyleName) + 1;

    new_entry = AllocPooled(pool, sizeof(*entry) + len1 + len2 + len3);
    if (new_entry)
    {
        STRPTR p = (STRPTR)(new_entry + 1);
        new_entry->FileName = p;
        memcpy(p, entry->FileName, len1);
        p += len1;
        new_entry->FamilyName = p;
        memcpy(p, entry->FamilyName, len2);
        p += len2;
        new_entry->StyleName = p;
        memcpy(p, entry->StyleName, len3);
    }

    return new_entry;

    AROS_USERFUNC_EXIT
}


struct Hook flConstructHook = {{NULL, NULL}, UFHN(flConstructFunc) };

AROS_UFH3(void, flDestructFunc,
                AROS_UFHA(struct Hook *, hook, A0),
        AROS_UFHA(APTR, pool, A2),
        AROS_UFHA(struct MUIS_FontList_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    size_t len1 = strlen(entry->FileName) + 1;
    size_t len2 = strlen(entry->FamilyName) + 1;
    size_t len3 = strlen(entry->StyleName) + 1;

    FreePooled(pool, entry, sizeof(*entry) + len1 + len2 + len3);

    AROS_USERFUNC_EXIT
}


struct Hook flDestructHook = {{NULL, NULL}, UFHN(flDestructFunc) };

AROS_UFH3(ULONG, flDisplayFunc,
                AROS_UFHA(struct Hook *, hook, A0),
        AROS_UFHA(STRPTR *, array, A2),
        AROS_UFHA(struct MUIS_FontList_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    array[0] = entry->FamilyName;
    array[1] = entry->StyleName;
    return 0;

    AROS_USERFUNC_EXIT
}


struct Hook flDisplayHook = {{NULL, NULL}, UFHN(flDisplayFunc) };

AROS_UFH3(LONG, flCompareFunc,
        AROS_UFHA(struct Hook *, hook, A0),
        AROS_UFHA(struct MUIS_FontList_Entry *, entry2, A2),
        AROS_UFHA(struct MUIS_FontList_Entry *, entry1, A1))
{
    AROS_USERFUNC_INIT

    LONG ret = strcmp(entry1->FamilyName, entry2->FamilyName);
    if (ret == 0)
        ret = strcmp(entry1->StyleName, entry2->StyleName);
    return ret;

    AROS_USERFUNC_EXIT
}


struct Hook flCompareHook = {{NULL, NULL}, UFHN(flCompareFunc) };


IPTR flNew(Class *cl, Object *o, struct opSet *msg)
{
    struct opSet method;
    struct TagItem tags[8];

    tags[0].ti_Tag = MUIA_Frame;
    tags[0].ti_Data = MUIV_Frame_InputList;
    tags[1].ti_Tag = MUIA_Background;
    tags[1].ti_Data = MUII_ListBack;
    tags[2].ti_Tag = MUIA_List_ConstructHook;
    tags[2].ti_Data = (IPTR)&flConstructHook;
    tags[3].ti_Tag = MUIA_List_DestructHook;
    tags[3].ti_Data = (IPTR)&flDestructHook;
    tags[4].ti_Tag = MUIA_List_DisplayHook;
    tags[4].ti_Data = (IPTR)&flDisplayHook;
    tags[4].ti_Tag = MUIA_List_DisplayHook;
    tags[4].ti_Data = (IPTR)&flDisplayHook;
    tags[5].ti_Tag = MUIA_List_CompareHook;
    tags[5].ti_Data = (IPTR)&flCompareHook;
    tags[6].ti_Tag = MUIA_List_Format;
    tags[6].ti_Data = (IPTR)",";
    tags[7].ti_Tag = TAG_MORE;
    tags[7].ti_Data = (IPTR)msg->ops_AttrList;

    method.MethodID = OM_NEW;
    method.ops_AttrList = tags;
    method.ops_GInfo = NULL;

    o = (Object *)DoSuperMethodA(cl, o, (Msg)&method);
    if (o)
    {
        FontListData *dat = INST_DATA(cl, o);
        NewList((struct List *) &dat->ScanDirTasks);
    }

    DEBUG_FONTWINDOW(dprintf("FontList: created object 0x%lx.\n", o));

    return (IPTR)o;
}


IPTR flDispose(Class *cl, Object *o, Msg msg)
{
    FontListData *dat = INST_DATA(cl, o);
    struct ScanDirTaskInfo *info;
    BOOL done;

    do
    {
        done = TRUE;

        Forbid();
        for (info = (APTR)dat->ScanDirTasks.mlh_Head; info->Node.mln_Succ; info = (APTR)info->Node.mln_Succ)
        {
            done = FALSE;
            Signal(&info->Proc->pr_Task, SIGBREAKF_CTRL_C);
        }
        Permit();

        if (!done)
            Wait(SIGBREAKF_CTRL_F);
    }
    while (!done);

    return DoSuperMethodA(cl, o, msg);
}


struct MUIP_FontList_AddDir
{
    STACKED ULONG MethodID;
    STACKED STRPTR DirName;
};

struct ScanDirTaskInfo *_pass_info;
Object *_pass_app;
struct Task *_pass_parent;
Object *_pass_fl;

void ScanDirTask(void)
{
    struct ScanDirTaskInfo *info = _pass_info;
    Object *app = _pass_app;
    struct Task *parent = _pass_parent;
    Object *fl = _pass_fl;
    BPTR lock;
    struct ExAllControl *eac;
    struct ExAllData *ead;
    ULONG more;
    BPTR olddir;
    FT_Library ftlibrary;

    Signal(parent, SIGBREAKF_CTRL_F);

        DEBUG_ADDDIR(dprintf("flScanDirTask: dir <%s>\n", info->DirName));

    if (FT_Init_FreeType(&ftlibrary) == 0)
    {
        DEBUG_ADDDIR(dprintf("flScanDirTask: ftlibrary 0x%x\n", ftlibrary));

        lock = Lock(info->DirName, ACCESS_READ);
        if (lock)
        {
            DEBUG_ADDDIR(dprintf("flScanDirTask: lock 0x%lx\n", lock));

            olddir = CurrentDir(lock);

            eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
            if (eac)
            {
                DEBUG_ADDDIR(dprintf("flScanDirTask: eac 0x%lx\n", eac));

                eac->eac_LastKey = 0;

                do
                {
                    more = ExAll(lock, (struct ExAllData *) info->Buffer, sizeof(info->Buffer), ED_NAME, eac);

                    DEBUG_ADDDIR(dprintf("flScanDirTask: more %d entries %d\n", more, eac->eac_Entries));


                    if (!more && IoErr() != ERROR_NO_MORE_ENTRIES)
                    {
                        DEBUG_ADDDIR(dprintf("flScanDirTask: err %d\n", IoErr()));
                        break;
                    }

                    if (eac->eac_Entries == 0)
                        continue;

                    ead = (APTR)info->Buffer;

                    do
                    {
                        FT_Face face;
                        FT_Error error;

                        DEBUG_ADDDIR(dprintf("flScanDirTask: ead 0x%x name %x <%s>\n", ead, ead->ed_Name, ead->ed_Name));
                        error = FT_New_Face(ftlibrary, ead->ed_Name, 0, &face);
                        DEBUG_ADDDIR(dprintf("flScanDirTask: error %d\n", error));
                        if (error == 0)
                        {
                            struct MUIS_FontList_Entry *entry;
                            size_t len1, len2, len3;

                            DEBUG_ADDDIR(dprintf("flScanDirTask: family 0x <%s> style 0x%x <%s>\n", face->family_name, face->family_name, face->style_name, face->style_name));

                            strncpy(info->NameBuf, info->DirName, sizeof(info->NameBuf) - 1);
                            AddPart(info->NameBuf, ead->ed_Name, sizeof(info->NameBuf));

                            len1 = strlen(info->NameBuf) + 1;
                            len2 = strlen(face->family_name) + 1;
                            len3 = strlen(face->style_name) + 1;

                            entry = AllocVec(sizeof(*entry) + len1 + len2 + len3, MEMF_PUBLIC);
                            if (entry)
                            {
                                char *p = (char *)(entry + 1);
                                entry->FileName = p;
                                memcpy(p, info->NameBuf, len1);
                                p += len1;
                                entry->FamilyName = p;
                                memcpy(p, face->family_name, len2);
                                p += len2;
                                entry->StyleName = p;
                                memcpy(p, face->style_name, len3);

                                if (!DoMethod(app, MUIM_Application_PushMethod,
                                            fl, 2, MUIM_FontList_AddEntry, entry))
                                    FreeVec(entry);
                            }

                            FT_Done_Face(face);
                        }

                        ead = ead->ed_Next;
                    }
                    while (ead);
                }
                while (more);

                DEBUG_ADDDIR(dprintf("flScanDirTask: done\n"));

                FreeDosObject(DOS_EXALLCONTROL, eac);
            }

            CurrentDir(olddir);
            UnLock(lock);
        }

        FT_Done_FreeType(ftlibrary);
    }

    Forbid();
    REMOVE(&info->Node);
    FreeVec(info);
    Signal(parent, SIGBREAKF_CTRL_F);
}


ULONG flAddDir(Class *cl, Object *o, struct MUIP_FontList_AddDir *msg)
{
    FontListData *dat = INST_DATA(cl, o);
    struct ScanDirTaskInfo *info;
    int dirname_len = strlen(msg->DirName) + 1;

    info = AllocVec(sizeof(*info) + dirname_len, MEMF_CLEAR | MEMF_PUBLIC);
    if (info)
    {
        info->DirName = (STRPTR)(info + 1);
        memcpy(info->DirName, msg->DirName, dirname_len);

            _pass_info = info;
            _pass_app = _app(o);
            _pass_parent = FindTask(NULL);
            _pass_fl = o;
            Forbid();
        info->Proc = CreateNewProcTags(
                NP_Entry, ScanDirTask,
                TAG_END);
        if (info->Proc)
        {
            ADDTAIL((APTR)&dat->ScanDirTasks, (APTR)info);
            Permit();

            Wait(SIGBREAKF_CTRL_F);
            return TRUE;
        }

            Permit();
        FreeVec(info);
    }

    return FALSE;
}


struct MUIP_FontList_AddEntry
{
    STACKED ULONG   MethodID;
    STACKED struct MUIS_FontList_Entry *Entry;
};

ULONG flAddEntry(Class *cl, Object *o, struct MUIP_FontList_AddEntry *msg)
{
    DoMethod(o, MUIM_List_InsertSingle, msg->Entry, MUIV_List_Insert_Sorted);
    FreeVec(msg->Entry);
    return TRUE;
}


AROS_UFH3(ULONG, FontListDispatch,
        AROS_UFHA(Class *, cl, A0),
        AROS_UFHA(Object *, o, A2),
        AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    ULONG ret;

    switch (msg->MethodID)
    {
        case OM_NEW:
            ret = flNew(cl, o, (struct opSet *)msg);
            break;

        case OM_DISPOSE:
            ret = flDispose(cl, o, msg);
            break;

        case MUIM_FontList_AddDir:
            ret = flAddDir(cl, o, (struct MUIP_FontList_AddDir *)msg);
            break;

        case MUIM_FontList_AddEntry:
            ret = flAddEntry(cl, o, (struct MUIP_FontList_AddEntry *)msg);
            break;

        default:
            ret = DoSuperMethodA(cl, o, msg);
            break;
    }

    return ret;

    AROS_USERFUNC_EXIT
}


void CleanupFontListClass(void)
{
    if (FontListClass)
    {
        MUI_DeleteCustomClass(FontListClass);
        FontListClass = NULL;
    }
}

int InitFontListClass(void)
{
    FontListClass = MUI_CreateCustomClass(NULL, MUIC_List, NULL,
            sizeof(FontListData), UFHN(FontListDispatch));
    return FontListClass != NULL;
}
