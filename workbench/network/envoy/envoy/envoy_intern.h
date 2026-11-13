#ifndef ENVOY_INTERN_H
#define ENVOY_INTERN_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: envoy.library - internal definitions. The four public requesters
          share one engine (envoy_req.c), as the original's do
          (re/spec/envoy-library.md §2); the window itself is Zune.
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <devices/timer.h>
#include <libraries/locale.h>
#include <aros/debug.h>

#include <envoy/envoy.h>

#define ELOG(...)               do { D(bug(__VA_ARGS__);) } while (0)

struct EnvoyBase
{
    struct Library              eb_Lib;
    struct SignalSemaphore      eb_Sem;
    struct Library              *eb_DOSBase;
    struct Library              *eb_UtilityBase;
    struct Library              *eb_IntuitionBase;
    struct Library              *eb_LocaleBase;        /* optional */
    struct Catalog              *eb_Catalog;           /* optional */
};

#define DOSBase                 ((struct DosLibrary *)EnvoyBase->eb_DOSBase)
#define UtilityBase             (EnvoyBase->eb_UtilityBase)
#define IntuitionBase           ((struct IntuitionBase *)EnvoyBase->eb_IntuitionBase)
#define LocaleBase              (EnvoyBase->eb_LocaleBase)

/* Catalog string IDs (Sys/envoyprefs.catalog, the editors' catalog) */
#define MSG_CANCEL_GAD              22
#define MSG_OK_GAD                  23
#define MSG_ELIB_HOST               46000
#define MSG_ELIB_HOST_REQUEST       46001
#define MSG_ELIB_USERNAME           46002
#define MSG_ELIB_PASSWORD           46003
#define MSG_ELIB_LOGIN_REQUEST      46004
#define MSG_ELIB_GROUPTAG           46005
#define MSG_ELIB_GROUPLISTFMT       46006
#define MSG_ELIB_USER_REQUEST       46007
#define MSG_ELIB_REALMS             46008
#define MSG_ELIB_PASSWORD_REQUEST   46009
#define MSG_ELIB_OLDPASSWORD        46010
#define MSG_ELIB_PASSWORD1          46011
#define MSG_ELIB_PASSWORD2          46012

CONST_STRPTR EnvoyStr(struct EnvoyBase *EnvoyBase, ULONG id);

/* ---- the requester engine ------------------------------------------- */

/* Slots of the common tags; each requester maps its own tag names onto them */
#define RQS_SCREEN      0
#define RQS_WINDOW      1
#define RQS_OPTIM       2
#define RQS_TITLE       3
#define RQS_CALLBACK    4
#define RQS_MSGPORT     5
#define RQS_COUNT       6

struct ReqTagMap
{
    Tag     Tag;
    UWORD   Slot;
};

/* Event IDs handed to the client's Event() */
#define RQID_OK         1
#define RQID_CANCEL     2
#define RQID_MIDDLE     3
#define RQID_STRING1    11      /* Return in string gadget 1..3 */
#define RQID_STRING2    12
#define RQID_STRING3    13
#define RQID_LISTCLICK  20      /* a list entry became active */
#define RQID_LISTDOUBLE 21      /* double click on a list entry */
#define RQID_CHANGED    30      /* text of some string gadget changed (RQF_WATCH) */

#define RQF_NOLIST      (1 << 0)    /* no list view */
#define RQF_MASK1       (1 << 1)    /* string gadget 1 is a password gadget */
#define RQF_MASK23      (1 << 2)    /* string gadgets 2 and 3 are password gadgets */
#define RQF_WATCH       (1 << 3)    /* report every text change as RQID_CHANGED */
#define RQF_LISTCOLS    (1 << 4)    /* list entries are struct ReqListEntry with two columns */

struct Req;

struct ReqClient
{
    ULONG                   TitleID;        /* default window title */
    ULONG                   LabelID[3];     /* labels of the string gadgets, 0 = absent */
    ULONG                   MiddleID;       /* label of the middle button, 0 = absent */
    const struct ReqTagMap  *Map;
    ULONG                   Flags;
    BOOL (*Setup)(struct Req *req);                 /* objects exist, window not yet open; FALSE aborts */
    void (*Setup2)(struct Req *req);                /* window is open (may be NULL) */
    BOOL (*Event)(struct Req *req, ULONG id);       /* FALSE closes the requester with req->Result */
    void (*Signals)(struct Req *req, ULONG sigs);   /* one of req->ExtraSigs arrived */
    void (*Cleanup)(struct Req *req);               /* window closed; frees client data */
};

/* A two-column list entry (RQF_LISTCOLS) */
struct ReqListEntry
{
    char            Name[32];
    CONST_STRPTR    Tag;            /* second column, may be "" */
    UBYTE           Kind;
};

struct Req
{
    struct EnvoyBase        *EnvoyBase;
    struct Library          *MUIMasterBase;
    struct Library          *GadToolsBase;
    const struct ReqClient  *Client;
    APTR                    ClientData;
    struct TagItem          *Tags;

    /* common tags */
    struct Screen           *Screen;
    struct Window           *BlockWindow;
    struct Window           *OptimWindow;
    CONST_STRPTR            Title;
    struct Hook             *CallBack;
    struct MsgPort          *CallPort;

    /* Zune objects */
    Object                  *App, *Win, *List, *ListObj, *Lbl[3], *Str[3], *OK, *Middle, *Cancel;

    BOOL                    Result;         /* preset TRUE; Cancel makes it FALSE */
    BOOL                    Done;
    ULONG                   ExtraSigs;      /* client's signals, waited for alongside Zune's */

    struct Requester        BlockReq;
    BOOL                    Blocked;

    /* test aid: ENV:Envoy/RequesterAutoClose = "<seconds>[,OK]" closes the requester by itself */
    struct MsgPort          *TimerPort;
    struct timerequest      *TimerReq;
    BOOL                    TimerPending;
    BOOL                    TimerOpen;
    BOOL                    AutoOK;
};

BOOL ReqRun(struct EnvoyBase *EnvoyBase, const struct ReqClient *client, APTR clientdata, struct TagItem *tags);
void ReqGetString(struct Req *req, int index, STRPTR dst, ULONG size);
void ReqSetString(struct Req *req, int index, CONST_STRPTR text);
void ReqActivate(struct Req *req, int index);
void ReqCopyOut(STRPTR dst, ULONG size, CONST_STRPTR src);

#endif /* ENVOY_INTERN_H */
