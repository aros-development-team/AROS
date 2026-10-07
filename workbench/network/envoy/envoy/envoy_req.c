/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: envoy.library - the requester engine shared by the four public
          requesters (re/spec/envoy-library.md §2): an optional list, up
          to three labelled string gadgets (plain or password) and an
          OK / optional middle / Cancel button row, in a Zune window.
          The client supplies the descriptor and reacts to events.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <libraries/mui.h>
#include <exec/memory.h>
#include <dos/var.h>
#include <devices/timer.h>
#include <string.h>

#include "envoy_intern.h"

/* ---- list hooks -------------------------------------------------------- */

/* entries are strings: sort without regard to case */
AROS_UFH3S(LONG, StrCompareHook,
           AROS_UFHA(struct Hook *, hook, A0),
           AROS_UFHA(CONST_STRPTR, e2, A2),
           AROS_UFHA(CONST_STRPTR, e1, A1))
{
    AROS_USERFUNC_INIT
    struct EnvoyBase *EnvoyBase = hook->h_Data;
    return Stricmp(e1, e2);
    AROS_USERFUNC_EXIT
}

/* entries are struct ReqListEntry: sort by name, show name and tag */
AROS_UFH3S(LONG, EntryCompareHook,
           AROS_UFHA(struct Hook *, hook, A0),
           AROS_UFHA(struct ReqListEntry *, e2, A2),
           AROS_UFHA(struct ReqListEntry *, e1, A1))
{
    AROS_USERFUNC_INIT
    struct EnvoyBase *EnvoyBase = hook->h_Data;
    return Stricmp(e1->Name, e2->Name);
    AROS_USERFUNC_EXIT
}

AROS_UFH3S(IPTR, EntryDisplayHook,
           AROS_UFHA(struct Hook *, hook, A0),
           AROS_UFHA(CONST_STRPTR *, array, A2),
           AROS_UFHA(struct ReqListEntry *, e, A1))
{
    AROS_USERFUNC_INIT
    if (e)
    {
        array[0] = e->Name;
        array[1] = e->Tag ? e->Tag : (CONST_STRPTR)"";
    }
    else
    {
        array[0] = "";
        array[1] = "";
    }
    return 0;
    AROS_USERFUNC_EXIT
}

/* ---- helpers ------------------------------------------------------------ */

void ReqCopyOut(STRPTR dst, ULONG size, CONST_STRPTR src)
{
    ULONG n;

    if (!dst || !size)
        return;
    n = strlen(src);
    if (n > size - 1)
        n = size - 1;
    CopyMem((APTR)src, dst, n);
    dst[n] = '\0';
}

void ReqGetString(struct Req *req, int index, STRPTR dst, ULONG size)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    IPTR s = 0;

    if (req->Str[index])
        GetAttr(MUIA_String_Contents, req->Str[index], &s);
    ReqCopyOut(dst, size, s ? (CONST_STRPTR)s : (CONST_STRPTR)"");
}

void ReqSetString(struct Req *req, int index, CONST_STRPTR text)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    if (req->Str[index])
        SetAttrs(req->Str[index], MUIA_String_Contents, (IPTR)text, TAG_DONE);
}

void ReqActivate(struct Req *req, int index)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    if (req->Str[index])
        SetAttrs(req->Win, MUIA_Window_ActiveObject, (IPTR)req->Str[index], TAG_DONE);
}

/* ---- the common tags ---------------------------------------------------- */

static void ParseCommonTags(struct Req *req)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    struct TagItem *tag, *tstate = req->Tags;
    const struct ReqTagMap *m;

    while ((tag = NextTagItem(&tstate)))
    {
        for (m = req->Client->Map; m->Tag; m++)
        {
            if (m->Tag != tag->ti_Tag)
                continue;
            switch (m->Slot)
            {
            case RQS_SCREEN:   req->Screen = (struct Screen *)tag->ti_Data; break;
            case RQS_WINDOW:   req->BlockWindow = (struct Window *)tag->ti_Data; break;
            case RQS_OPTIM:    req->OptimWindow = (struct Window *)tag->ti_Data; break;
            case RQS_TITLE:    req->Title = (CONST_STRPTR)tag->ti_Data; break;
            case RQS_CALLBACK: req->CallBack = (struct Hook *)tag->ti_Data; break;
            case RQS_MSGPORT:  req->CallPort = (struct MsgPort *)tag->ti_Data; break;
            }
        }
    }
    if (!req->OptimWindow)
        req->OptimWindow = req->BlockWindow;
    if (!req->Screen && req->OptimWindow)
        req->Screen = req->OptimWindow->WScreen;
    if (!req->Title)
        req->Title = EnvoyStr(req->EnvoyBase, req->Client->TitleID);
}

/* ---- blocking the caller's window ---------------------------------------- */

static void BlockWindow(struct Req *req, BOOL block)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    struct Window *win = req->BlockWindow;

    if (!win)
        return;
    if (block)
    {
        struct TagItem busy[] = { { WA_BusyPointer, TRUE }, { TAG_DONE, 0 } };
        InitRequester(&req->BlockReq);
        req->Blocked = Request(&req->BlockReq, win);
        SetWindowPointerA(win, busy);
    }
    else
    {
        struct TagItem normal[] = { { WA_Pointer, 0 }, { WA_BusyPointer, FALSE }, { TAG_DONE, 0 } };
        SetWindowPointerA(win, normal);
        if (req->Blocked)
            EndRequest(&req->BlockReq, win);
        req->Blocked = FALSE;
    }
}

/* ---- the caller's IDCMP port ---------------------------------------------- */

static void DrainCallPort(struct Req *req)
{
    struct Library *GadToolsBase = req->GadToolsBase;
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    struct IntuiMessage *msg;

    while ((msg = GT_GetIMsg(req->CallPort)))
    {
        CallHookPkt(req->CallBack, req->CallPort, msg);
        GT_ReplyIMsg(msg);
    }
}

/* ---- auto-close (test aid) ------------------------------------------------ */

static void StartTimer(struct Req *req, ULONG secs)
{
    req->TimerReq->tr_node.io_Command = TR_ADDREQUEST;
    req->TimerReq->tr_time.tv_secs = secs;
    req->TimerReq->tr_time.tv_micro = 0;
    SendIO(&req->TimerReq->tr_node);
    req->TimerPending = TRUE;
}

static void SetupAutoClose(struct Req *req)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    char var[32];
    LONG secs = 0;

    if (GetVar("Envoy/RequesterAutoClose", var, sizeof(var), GVF_GLOBAL_ONLY) <= 0)
        return;
    if (StrToLong(var, &secs) <= 0 || secs <= 0)
        return;
    req->AutoOK = strstr(var, "OK") != NULL;
    if (!(req->TimerPort = CreateMsgPort()))
        return;
    if (!(req->TimerReq = (struct timerequest *)CreateIORequest(req->TimerPort, sizeof(struct timerequest))))
        return;
    if (OpenDevice("timer.device", UNIT_VBLANK, &req->TimerReq->tr_node, 0))
        return;
    req->TimerOpen = TRUE;
    StartTimer(req, secs);
}

static void FreeAutoClose(struct Req *req)
{
    if (req->TimerPending)
    {
        AbortIO(&req->TimerReq->tr_node);
        WaitIO(&req->TimerReq->tr_node);
    }
    if (req->TimerOpen)
        CloseDevice(&req->TimerReq->tr_node);
    if (req->TimerReq)
        DeleteIORequest(&req->TimerReq->tr_node);
    if (req->TimerPort)
        DeleteMsgPort(req->TimerPort);
}

/* ---- events ----------------------------------------------------------------- */

static void HandleID(struct Req *req, ULONG id)
{
    if (id == RQID_CANCEL)
    {
        req->Result = FALSE;
        req->Done = TRUE;
    }
    else if (!req->Client->Event(req, id))
        req->Done = TRUE;
}

static void AutoCloseFired(struct Req *req)
{
    GetMsg(req->TimerPort);
    req->TimerPending = FALSE;
    if (req->AutoOK)
    {
        HandleID(req, RQID_OK);
        req->AutoOK = FALSE;
        if (!req->Done)
            StartTimer(req, 5);          /* OK did not close it: cancel later */
    }
    else
        HandleID(req, RQID_CANCEL);
}

/* ---- building and running ------------------------------------------------------ */

static BOOL BuildObjects(struct Req *req)
{
    struct Library *MUIMasterBase = req->MUIMasterBase;
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    const struct ReqClient *c = req->Client;
    Object *root, *strgroup = NULL, *buttons, *listobj = NULL;
    static struct Hook strcmphook, entrycmphook, entrydisphook;
    int i;

    strcmphook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(StrCompareHook);
    strcmphook.h_Data = EnvoyBase;
    entrycmphook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(EntryCompareHook);
    entrycmphook.h_Data = EnvoyBase;
    entrydisphook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(EntryDisplayHook);

    root = VGroup, End;
    if (!root)
        return FALSE;

    if (!(c->Flags & RQF_NOLIST))
    {
        if (c->Flags & RQF_LISTCOLS)
            listobj = ListObject, InputListFrame,
                MUIA_List_Format, (IPTR)",",
                MUIA_List_CompareHook, (IPTR)&entrycmphook,
                MUIA_List_DisplayHook, (IPTR)&entrydisphook,
                End;
        else
            listobj = ListObject, InputListFrame,
                MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                MUIA_List_DestructHook, MUIV_List_DestructHook_String,
                MUIA_List_CompareHook, (IPTR)&strcmphook,
                End;
        if (!listobj)
            return FALSE;
        req->List = ListviewObject, MUIA_CycleChain, 1, MUIA_Listview_List, (IPTR)listobj, End;
        if (!req->List)
            return FALSE;
        req->ListObj = listobj;
        DoMethod(root, OM_ADDMEMBER, (IPTR)req->List);
    }

    for (i = 0; i < 3; i++)
    {
        BOOL masked;
        if (!c->LabelID[i])
            continue;
        if (!strgroup)
        {
            strgroup = ColGroup(2), End;
            if (!strgroup)
                return FALSE;
            DoMethod(root, OM_ADDMEMBER, (IPTR)strgroup);
        }
        masked = (i == 0) ? (c->Flags & RQF_MASK1) != 0 : (c->Flags & RQF_MASK23) != 0;
        req->Lbl[i] = Label(EnvoyStr(EnvoyBase, c->LabelID[i]));
        req->Str[i] = StringObject, StringFrame,
            MUIA_String_MaxLen, (i == 0) ? 128 : 32,
            MUIA_String_Secret, masked,
            MUIA_CycleChain, 1,
            End;
        if (!req->Lbl[i] || !req->Str[i])
            return FALSE;
        DoMethod(strgroup, OM_ADDMEMBER, (IPTR)req->Lbl[i]);
        DoMethod(strgroup, OM_ADDMEMBER, (IPTR)req->Str[i]);
    }

    req->OK = SimpleButton(EnvoyStr(EnvoyBase, MSG_OK_GAD));
    req->Cancel = SimpleButton(EnvoyStr(EnvoyBase, MSG_CANCEL_GAD));
    if (c->MiddleID)
        req->Middle = SimpleButton(EnvoyStr(EnvoyBase, c->MiddleID));
    if (!req->OK || !req->Cancel || (c->MiddleID && !req->Middle))
        return FALSE;
    buttons = HGroup, MUIA_Group_SameWidth, TRUE, End;
    if (!buttons)
        return FALSE;
    DoMethod(buttons, OM_ADDMEMBER, (IPTR)req->OK);
    if (req->Middle)
        DoMethod(buttons, OM_ADDMEMBER, (IPTR)req->Middle);
    DoMethod(buttons, OM_ADDMEMBER, (IPTR)req->Cancel);
    DoMethod(root, OM_ADDMEMBER, (IPTR)buttons);

    req->Win = WindowObject,
        MUIA_Window_Title, (IPTR)req->Title,
        MUIA_Window_CloseGadget, FALSE,
        MUIA_Window_NoMenus, TRUE,
        MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered,
        MUIA_Window_TopEdge, MUIV_Window_TopEdge_Centered,
        req->OptimWindow ? MUIA_Window_RefWindow : TAG_IGNORE, (IPTR)req->OptimWindow,
        req->Screen ? MUIA_Window_Screen : TAG_IGNORE, (IPTR)req->Screen,
        WindowContents, (IPTR)root,
        End;
    if (!req->Win)
        return FALSE;
    req->App = ApplicationObject,
        MUIA_Application_Title, (IPTR)"Envoy",
        MUIA_Application_Base, (IPTR)"ENVOYREQ",
        MUIA_Application_UseCommodities, FALSE,
        MUIA_Application_UseRexx, FALSE,
        MUIA_Application_Window, (IPTR)req->Win,
        End;
    if (!req->App)
    {
        MUI_DisposeObject(req->Win);
        req->Win = NULL;
        return FALSE;
    }

    DoMethod(req->OK, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)req->App, 2, MUIM_Application_ReturnID, RQID_OK);
    DoMethod(req->Cancel, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)req->App, 2, MUIM_Application_ReturnID, RQID_CANCEL);
    if (req->Middle)
        DoMethod(req->Middle, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)req->App, 2, MUIM_Application_ReturnID, RQID_MIDDLE);
    for (i = 0; i < 3; i++)
    {
        if (!req->Str[i])
            continue;
        DoMethod(req->Str[i], MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)req->App, 2, MUIM_Application_ReturnID, RQID_STRING1 + i);
        if (c->Flags & RQF_WATCH)
            DoMethod(req->Str[i], MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, (IPTR)req->App, 2, MUIM_Application_ReturnID, RQID_CHANGED);
    }
    if (req->List)
    {
        DoMethod(req->ListObj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)req->App, 2, MUIM_Application_ReturnID, RQID_LISTCLICK);
        DoMethod(req->List, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, (IPTR)req->App, 2, MUIM_Application_ReturnID, RQID_LISTDOUBLE);
    }
    return TRUE;
}

static void EventLoop(struct Req *req)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    ULONG sigs = 0, portsig = 0, timersig = 0, got;
    IPTR open = FALSE;

    if (req->CallBack && req->CallPort && req->GadToolsBase)
        portsig = 1UL << req->CallPort->mp_SigBit;
    if (req->TimerPort)
        timersig = 1UL << req->TimerPort->mp_SigBit;

    SetAttrs(req->Win, MUIA_Window_Open, TRUE, TAG_DONE);
    GetAttr(MUIA_Window_Open, req->Win, &open);
    if (!open)
    {
        req->Result = FALSE;
        return;
    }
    if (req->Client->Setup2)
        req->Client->Setup2(req);

    while (!req->Done)
    {
        ULONG id;
        while ((id = DoMethod(req->App, MUIM_Application_NewInput, (IPTR)&sigs)) != 0 && !req->Done)
            HandleID(req, id);
        if (req->Done)
            break;
        got = Wait(sigs | req->ExtraSigs | portsig | timersig);
        if (got & portsig)
            DrainCallPort(req);
        if (got & timersig)
            AutoCloseFired(req);
        if ((got & req->ExtraSigs) && req->Client->Signals)
            req->Client->Signals(req, got & req->ExtraSigs);
        sigs &= got;
    }
    SetAttrs(req->Win, MUIA_Window_Open, FALSE, TAG_DONE);
}

BOOL ReqRun(struct EnvoyBase *EnvoyBase, const struct ReqClient *client, APTR clientdata, struct TagItem *tags)
{
    struct Req *req;
    BOOL result = FALSE, setup = FALSE;

    if (!(req = AllocVec(sizeof(struct Req), MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    req->EnvoyBase = EnvoyBase;
    req->Client = client;
    req->ClientData = clientdata;
    req->Tags = tags;
    req->Result = TRUE;

    ParseCommonTags(req);
    if (req->CallBack && req->CallPort)
        req->GadToolsBase = OpenLibrary("gadtools.library", 36);

    if ((req->MUIMasterBase = OpenLibrary("muimaster.library", 0)))
    {
        if (BuildObjects(req))
        {
            if ((setup = client->Setup(req)))
            {
                SetupAutoClose(req);
                BlockWindow(req, TRUE);
                EventLoop(req);
                BlockWindow(req, FALSE);
                FreeAutoClose(req);
                result = req->Result;
            }
        }
        if (setup && client->Cleanup)
            client->Cleanup(req);
        if (req->App)
        {
            struct Library *MUIMasterBase = req->MUIMasterBase;
            MUI_DisposeObject(req->App);
        }
        CloseLibrary(req->MUIMasterBase);
    }
    if (req->GadToolsBase)
        CloseLibrary(req->GadToolsBase);
    FreeVec(req);
    return result;
}
