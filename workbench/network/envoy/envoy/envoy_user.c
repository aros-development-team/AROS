/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: envoy.library - UserRequestA(): pick a user and/or a group from
          accounts.library's lists or from lists the caller supplies
          (re/spec/envoy-library.md §5).
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <libraries/mui.h>
#include <exec/memory.h>
#include <string.h>

#include "envoy_intern.h"
#include <envoy/accounts.h>
#include <proto/accounts.h>

#define KIND_USER   1
#define KIND_GROUP  2

struct UserEntry
{
    struct Node         Node;
    struct ReqListEntry Entry;
};

struct UserReq
{
    STRPTR          UserBuff, GroupBuff;
    ULONG           UserLen, GroupLen;
    struct List     *UserList, *GroupList;
    struct Library  *AccountsBase;
    struct List     Entries;
};

static const struct ReqTagMap usermap[] =
{
    { UGREQ_Screen, RQS_SCREEN }, { UGREQ_Window, RQS_WINDOW }, { UGREQ_OptimWindow, RQS_OPTIM },
    { UGREQ_Title, RQS_TITLE }, { UGREQ_CallBack, RQS_CALLBACK }, { UGREQ_MsgPort, RQS_MSGPORT }, { 0, 0 }
};

static void AddEntry(struct Req *req, struct UserReq *ur, CONST_STRPTR name, UBYTE kind)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    struct UserEntry *e;

    if (!(e = AllocVec(sizeof(struct UserEntry), MEMF_CLEAR | MEMF_PUBLIC)))
        return;
    ReqCopyOut(e->Entry.Name, sizeof(e->Entry.Name), name);
    e->Entry.Kind = kind;
    e->Entry.Tag = (kind == KIND_GROUP) ? EnvoyStr(EnvoyBase, MSG_ELIB_GROUPTAG) : (CONST_STRPTR)"";
    AddTail(&ur->Entries, &e->Node);
    DoMethod(req->ListObj, MUIM_List_InsertSingle, (IPTR)&e->Entry, MUIV_List_Insert_Sorted);
}

static BOOL UserSetup(struct Req *req)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    struct UserReq *ur = req->ClientData;
    struct Library *AccountsBase;
    struct TagItem *tag, *tstate = req->Tags;
    struct Node *n;

    NEWLIST(&ur->Entries);
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case UGREQ_UserBuff:
            if ((ur->UserBuff = (STRPTR)tag->ti_Data))
                ur->UserBuff[0] = '\0';
            break;
        case UGREQ_UserBuffLen:  ur->UserLen = tag->ti_Data; break;
        case UGREQ_GroupBuff:
            if ((ur->GroupBuff = (STRPTR)tag->ti_Data))
                ur->GroupBuff[0] = '\0';
            break;
        case UGREQ_GroupBuffLen: ur->GroupLen = tag->ti_Data; break;
        case UGREQ_UserList:     ur->UserList = (struct List *)tag->ti_Data; break;
        case UGREQ_GroupList:    ur->GroupList = (struct List *)tag->ti_Data; break;
        }
    }
    if (!(ur->AccountsBase = AccountsBase = OpenLibrary("accounts.library", 0)))
        return FALSE;

    SetAttrs(req->ListObj, MUIA_List_Quiet, TRUE, TAG_DONE);
    if (ur->UserBuff)
    {
        if (ur->UserList)
        {
            ForeachNode(ur->UserList, n)
                AddEntry(req, ur, n->ln_Name, KIND_USER);
        }
        else
        {
            struct UserInfo *ui = AllocUserInfo();
            if (ui)
            {
                ui->ui_UserID = 0;
                while (NextUser(ui) == 0)
                    AddEntry(req, ur, ui->ui_UserName, KIND_USER);
                FreeUserInfo(ui);
            }
        }
    }
    if (ur->GroupBuff)
    {
        if (ur->GroupList)
        {
            ForeachNode(ur->GroupList, n)
                AddEntry(req, ur, n->ln_Name, KIND_GROUP);
        }
        else
        {
            struct GroupInfo *gi = AllocGroupInfo();
            if (gi)
            {
                gi->gi_GroupID = 0;
                while (NextGroup(gi) == 0)
                    AddEntry(req, ur, gi->gi_GroupName, KIND_GROUP);
                FreeGroupInfo(gi);
            }
        }
    }
    SetAttrs(req->ListObj, MUIA_List_Quiet, FALSE, TAG_DONE);
    return TRUE;
}

static BOOL UserEvent(struct Req *req, ULONG id)
{
    struct UserReq *ur = req->ClientData;
    struct ReqListEntry *e = NULL;

    switch (id)
    {
    case RQID_LISTCLICK:
    case RQID_LISTDOUBLE:
        DoMethod(req->ListObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&e);
        if (e)
        {
            /* the caller's buffers follow every click, as the original's do */
            if (e->Kind == KIND_USER)
            {
                ReqCopyOut(ur->UserBuff, ur->UserLen, e->Name);
                if (ur->GroupBuff)
                    ur->GroupBuff[0] = '\0';
            }
            else
            {
                ReqCopyOut(ur->GroupBuff, ur->GroupLen, e->Name);
                if (ur->UserBuff)
                    ur->UserBuff[0] = '\0';
            }
        }
        if (id == RQID_LISTCLICK)
            return TRUE;
        /* fall through */
    case RQID_OK:
        req->Result = TRUE;
        return FALSE;
    }
    return TRUE;
}

static void UserCleanup(struct Req *req)
{
    struct UserReq *ur = req->ClientData;
    struct Node *n;

    DoMethod(req->ListObj, MUIM_List_Clear);
    while ((n = RemHead(&ur->Entries)))
        FreeVec(n);
    if (ur->AccountsBase)
        CloseLibrary(ur->AccountsBase);
}

static const struct ReqClient userclient =
{
    MSG_ELIB_USER_REQUEST, { 0, 0, 0 }, 0, usermap, RQF_LISTCOLS,
    UserSetup, NULL, UserEvent, NULL, UserCleanup
};

/*****************************************************************************

    NAME */
#include <proto/envoy.h>

        AROS_LH1(BOOL, UserRequestA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct EnvoyBase *, EnvoyBase, 7, Envoy)

/*  FUNCTION
        Lists the users (UGREQ_UserBuff given) and/or groups
        (UGREQ_GroupBuff given) known to accounts.library, or the names in
        UGREQ_UserList/UGREQ_GroupList, and lets the user pick one.

    INPUTS
        tags - UGREQ_#? tags.

    RESULT
        TRUE on OK or a double click, FALSE on Cancel or failure. The chosen
        name is in UGREQ_UserBuff or UGREQ_GroupBuff; the other buffer is
        empty. Both buffers are emptied when the requester starts and follow
        every selection, so they hold the last selection even after Cancel.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct UserReq *ur;
    BOOL ok;

    if (!(ur = AllocVec(sizeof(struct UserReq), MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    ok = ReqRun(EnvoyBase, &userclient, ur, tags);
    FreeVec(ur);
    return ok;

    AROS_LIBFUNC_EXIT
}
