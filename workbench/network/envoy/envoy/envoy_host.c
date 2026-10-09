/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: envoy.library - HostRequestA(): browse the hosts (and realms)
          that answer an NIPC inquiry (re/spec/envoy-library.md §3).
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
#include <proto/nipc.h>

struct HostReq
{
    struct Library          *NIPCBase;
    struct Hook             Hook;
    struct SignalSemaphore  Sem;
    struct List             Pending;        /* names reported by the inquiry hook, not yet shown */
    struct Task             *Task;
    LONG                    SigBit;
    BOOL                    Running;
    BOOL                    RealmMode;
    BOOL                    NoRealms;
    char                    Realm[65];
    char                    Host[65];
    char                    Local[128];
    char                    Text[160];
    STRPTR                  Buffer;
    ULONG                   BuffSize;
    struct TagItem          *MatchClone;    /* the caller's MATCH_#? tags */
    struct TagItem          InqTags[3];
};

static const Tag matchtags[] =
{
    MATCH_REALM, MATCH_IPADDR, MATCH_HOSTNAME, MATCH_SERVICE, MATCH_ENTITY, MATCH_OWNER,
    MATCH_MACHDESC, MATCH_ATTNFLAGS, MATCH_LIBVERSION, MATCH_CHIPREVBITS, MATCH_MAXFASTMEM,
    MATCH_AVAILFASTMEM, MATCH_MAXCHIPMEM, MATCH_AVAILCHIPMEM, MATCH_KICKVERSION, MATCH_WBVERSION, 0
};

static const struct ReqTagMap hostmap[] =
{
    { HREQ_Screen, RQS_SCREEN }, { HREQ_Window, RQS_WINDOW }, { HREQ_OptimWindow, RQS_OPTIM },
    { HREQ_Title, RQS_TITLE }, { HREQ_CallBack, RQS_CALLBACK }, { HREQ_MsgPort, RQS_MSGPORT }, { 0, 0 }
};

/* Runs in the nipc process: queue the name, wake the requester. No Zune here. */
AROS_UFH3S(IPTR, InqHook,
           AROS_UFHA(struct Hook *, hook, A0),
           AROS_UFHA(struct Task *, caller, A2),
           AROS_UFHA(struct TagItem *, tags, A1))
{
    AROS_USERFUNC_INIT
    struct HostReq *hr = hook->h_Data;
    struct TagItem *tag;

    ELOG("[envoy] InqHook tags %p\n", tags);
    if (!tags)
        hr->Running = FALSE;
    else
    {
        for (tag = tags; tag->ti_Tag != TAG_DONE; tag++)
        {
            CONST_STRPTR s, colon;
            struct Node *n;
            ULONG len;

            ELOG("[envoy]   tag %08lx data %p\n", (unsigned long)tag->ti_Tag, (APTR)tag->ti_Data);
            if (tag->ti_Tag != QUERY_HOSTNAME && tag->ti_Tag != QUERY_REALMS)
                continue;
            s = (CONST_STRPTR)tag->ti_Data;
            if (!s)
                continue;
            if ((colon = strchr(s, ':')) && colon[1])
                s = colon + 1;                      /* "realm:host" -> "host" */
            len = strlen(s);
            if (!(n = AllocVec(sizeof(struct Node) + len + 1, MEMF_PUBLIC)))
                continue;
            n->ln_Name = (char *)(n + 1);
            CopyMem((APTR)s, n->ln_Name, len + 1);
            ObtainSemaphore(&hr->Sem);
            AddTail(&hr->Pending, n);
            ReleaseSemaphore(&hr->Sem);
        }
    }
    Signal(hr->Task, 1UL << hr->SigBit);
    return 1;
    AROS_USERFUNC_EXIT
}

static void FreePending(struct HostReq *hr)
{
    struct Node *n;

    ObtainSemaphore(&hr->Sem);
    while ((n = RemHead(&hr->Pending)))
        FreeVec(n);
    ReleaseSemaphore(&hr->Sem);
}

static void StartInquiry(struct Req *req, struct HostReq *hr, BOOL realms)
{
    struct Library *NIPCBase = hr->NIPCBase;

    NIPCInquiryA(&hr->Hook, 0, 0, NULL);            /* abort whatever runs */
    FreePending(hr);
    DoMethod(req->ListObj, MUIM_List_Clear);
    if (realms)
    {
        hr->InqTags[0].ti_Tag = QUERY_REALMS;  hr->InqTags[0].ti_Data = 0;
        hr->InqTags[1].ti_Tag = TAG_DONE;
    }
    else
    {
        hr->InqTags[0].ti_Tag = hr->Realm[0] ? MATCH_REALM : TAG_IGNORE;
        hr->InqTags[0].ti_Data = (IPTR)hr->Realm;
        hr->InqTags[1].ti_Tag = QUERY_HOSTNAME; hr->InqTags[1].ti_Data = 0;
        hr->InqTags[2].ti_Tag = hr->MatchClone ? TAG_MORE : TAG_DONE;
        hr->InqTags[2].ti_Data = (IPTR)hr->MatchClone;
    }
    hr->Running = NIPCInquiryA(&hr->Hook, 10, 500, hr->InqTags);
    ELOG("[envoy] StartInquiry realms %d realm '%s' -> %d\n", realms, hr->Realm, hr->Running);
}

static void SetText(struct Req *req, struct HostReq *hr)
{
    if (hr->Realm[0])
    {
        strcpy(hr->Text, hr->Realm);
        strcat(hr->Text, ":");
        strcat(hr->Text, hr->Host);
    }
    else
        strcpy(hr->Text, hr->Host);
    ReqSetString(req, 0, hr->Text);
}

static BOOL HostSetup(struct Req *req)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    struct HostReq *hr = req->ClientData;
    struct Library *NIPCBase;
    struct TagItem *tag, *tstate = req->Tags;
    ULONG nmatch = 0;
    char *colon;

    NEWLIST(&hr->Pending);
    InitSemaphore(&hr->Sem);
    hr->Task = FindTask(NULL);
    hr->SigBit = -1;

    if (!(hr->NIPCBase = NIPCBase = OpenLibrary("nipc.library", 50)))
        return FALSE;
    if (!GetHostName(NULL, hr->Local, sizeof(hr->Local)))
        return FALSE;

    while ((tag = NextTagItem(&tstate)))
    {
        const Tag *m;
        switch (tag->ti_Tag)
        {
        case HREQ_Buffer:       hr->Buffer = (STRPTR)tag->ti_Data; break;
        case HREQ_BuffSize:     hr->BuffSize = tag->ti_Data; break;
        case HREQ_DefaultRealm:
            if (tag->ti_Data)
                ReqCopyOut(hr->Local, sizeof(hr->Local), (CONST_STRPTR)tag->ti_Data);
            break;
        case HREQ_NoRealms:     hr->NoRealms = tag->ti_Data != 0; break;
        }
        for (m = matchtags; *m; m++)
            if (*m == tag->ti_Tag)
                nmatch++;
    }
    if (nmatch)
    {
        ULONG i = 0;
        if (!(hr->MatchClone = AllocVec((nmatch + 1) * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR)))
            return FALSE;
        tstate = req->Tags;
        while ((tag = NextTagItem(&tstate)))
        {
            const Tag *m;
            for (m = matchtags; *m; m++)
                if (*m == tag->ti_Tag)
                    hr->MatchClone[i++] = *tag;
        }
        hr->MatchClone[i].ti_Tag = TAG_DONE;
    }

    /* the realm the local host is in, if any */
    if ((colon = strchr(hr->Local, ':')))
    {
        *colon = '\0';
        ReqCopyOut(hr->Realm, sizeof(hr->Realm), hr->Local);
    }
    else
        hr->NoRealms = TRUE;                        /* no realm here: no Realms button either */
    if (hr->NoRealms && req->Middle)
        SetAttrs(req->Middle, MUIA_ShowMe, FALSE, TAG_DONE);
    SetText(req, hr);

    if ((hr->SigBit = AllocSignal(-1)) < 0)
        return FALSE;
    req->ExtraSigs = 1UL << hr->SigBit;
    hr->Hook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(InqHook);
    hr->Hook.h_Data = hr;
    StartInquiry(req, hr, FALSE);
    return TRUE;
}

/* the inquiry hook left names in the pending list: show them, sorted, once each */
static void HostSignals(struct Req *req, ULONG sigs)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    struct HostReq *hr = req->ClientData;
    struct Node *n;
    BOOL added = FALSE;

    SetAttrs(req->ListObj, MUIA_List_Quiet, TRUE, TAG_DONE);
    for (;;)
    {
        IPTR count = 0, i;
        BOOL dup = FALSE;

        ObtainSemaphore(&hr->Sem);
        n = RemHead(&hr->Pending);
        ReleaseSemaphore(&hr->Sem);
        if (!n)
            break;
        ELOG("[envoy] HostSignals: '%s'\n", n->ln_Name);
        GetAttr(MUIA_List_Entries, req->ListObj, &count);
        for (i = 0; i < count && !dup; i++)
        {
            CONST_STRPTR e = NULL;
            DoMethod(req->ListObj, MUIM_List_GetEntry, i, (IPTR)&e);
            if (e && !Stricmp(e, n->ln_Name))
                dup = TRUE;
        }
        if (!dup)
        {
            DoMethod(req->ListObj, MUIM_List_InsertSingle, (IPTR)n->ln_Name, MUIV_List_Insert_Sorted);
            added = TRUE;
        }
        FreeVec(n);
    }
    /* leaving quiet mode redraws the list once with everything that came in */
    SetAttrs(req->ListObj, MUIA_List_Quiet, FALSE, TAG_DONE);
    if (added)
        ELOG("[envoy] HostSignals: list now has entries\n");
}

static BOOL HostAccept(struct Req *req, struct HostReq *hr)
{
    SetText(req, hr);
    ReqCopyOut(hr->Buffer, hr->BuffSize, hr->Text);
    req->Result = TRUE;
    return FALSE;
}

static BOOL HostEvent(struct Req *req, ULONG id)
{
    struct HostReq *hr = req->ClientData;
    CONST_STRPTR entry = NULL;

    switch (id)
    {
    case RQID_LISTCLICK:
    case RQID_LISTDOUBLE:
        DoMethod(req->ListObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&entry);
        if (!entry)
            return TRUE;
        if (hr->RealmMode)
        {
            ReqCopyOut(hr->Realm, sizeof(hr->Realm), entry);
            hr->Host[0] = '\0';
            hr->RealmMode = FALSE;
            SetText(req, hr);
            StartInquiry(req, hr, FALSE);
            return TRUE;
        }
        ReqCopyOut(hr->Host, sizeof(hr->Host), entry);
        SetText(req, hr);
        if (id == RQID_LISTDOUBLE)
            return HostAccept(req, hr);
        ReqActivate(req, 0);
        return TRUE;

    case RQID_MIDDLE:
        hr->RealmMode = TRUE;
        StartInquiry(req, hr, TRUE);
        return TRUE;

    case RQID_STRING1:
    case RQID_OK:
    {
        char text[128], *colon;
        BOOL typedcolon;

        ReqGetString(req, 0, text, sizeof(text));
        typedcolon = (colon = strchr(text, ':')) != NULL;
        if (typedcolon)
        {
            *colon = '\0';
            ReqCopyOut(hr->Realm, sizeof(hr->Realm), text);
            ReqCopyOut(hr->Host, sizeof(hr->Host), colon + 1);
        }
        else
            ReqCopyOut(hr->Host, sizeof(hr->Host), text);
        SetText(req, hr);
        if (typedcolon && hr->Realm[0] && !hr->Host[0])
        {
            hr->RealmMode = FALSE;
            StartInquiry(req, hr, FALSE);           /* "realm:" + Return lists that realm */
        }
        else if (hr->Host[0])
            return HostAccept(req, hr);
        ReqActivate(req, 0);
        return TRUE;
    }
    }
    return TRUE;
}

static void HostCleanup(struct Req *req)
{
    struct HostReq *hr = req->ClientData;
    struct Library *NIPCBase = hr->NIPCBase;

    if (NIPCBase)
    {
        if (hr->SigBit >= 0)
            NIPCInquiryA(&hr->Hook, 0, 0, NULL);    /* synchronous: no hook call after this */
        FreePending(hr);
        CloseLibrary(NIPCBase);
    }
    if (hr->SigBit >= 0)
        FreeSignal(hr->SigBit);
    FreeVec(hr->MatchClone);
}

static const struct ReqClient hostclient =
{
    MSG_ELIB_HOST_REQUEST, { MSG_ELIB_HOST, 0, 0 }, MSG_ELIB_REALMS, hostmap, 0,
    HostSetup, NULL, HostEvent, HostSignals, HostCleanup
};

/*****************************************************************************

    NAME */
#include <proto/envoy.h>

        AROS_LH1(BOOL, HostRequestA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct EnvoyBase *, EnvoyBase, 5, Envoy)

/*  FUNCTION
        Opens a requester listing the hosts that answer an NIPC inquiry
        and lets the user pick or type one. With realms configured the
        middle button lists the realms instead, and a "realm:" prefix in
        the string gadget lists that realm's hosts.

    INPUTS
        tags - HREQ_#? tags and any MATCH_#? inquiry tags, which restrict
               the hosts shown.

    RESULT
        TRUE when a host was chosen and copied to HREQ_Buffer as "host" or
        "realm:host"; FALSE on Cancel or failure.

    NOTES
        The buffer is always NUL-terminated within HREQ_BuffSize.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct HostReq *hr;
    BOOL ok;

    if (!(hr = AllocVec(sizeof(struct HostReq), MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    ok = ReqRun(EnvoyBase, &hostclient, hr, tags);
    FreeVec(hr);
    return ok;

    AROS_LIBFUNC_EXIT
}
