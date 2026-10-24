/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* Bounded, daemon-owned queues for generic DNS record questions. No client
 * Hook or output buffer is retained. PTR and SRV names are expanded before
 * an event is queued, so an event never refers to a received packet. */

#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>
#include <netinet/in.h>

#include "mdnsd.h"

#define QUERY_SESSIONS 8
#define QUERY_QUEUE 8

struct QuerySession
{
    APTR handle;
    ULONG owner;
    char name[256];
    UWORD type;
    UWORD head, count;
    ULONG nextQuery, interval, deadline;
    BOOL timed, ended, failed;
    LONG endResult;
    struct MDNSRecordEvent queue[QUERY_QUEUE];
    struct MDNSEventRequest *request;
};

struct MDNSQueryState
{
    struct QuerySession sessions[QUERY_SESSIONS];
    IPTR nextHandle;
};

static BOOL
due(ULONG now, ULONG deadline)
{
    return (LONG)(now - deadline) >= 0;
}

static LONG
normalize_name(char *to, LONG size, const char *from)
{
    UBYTE wire[256];
    LONG length, used;

    if (from == NULL) {
        return MDNSERR_BADPARAM;
    }
    length = mdnsTextLength(from, size);
    if (length == 0 || length == size) {
        return MDNSERR_BADPARAM;
    }
    memcpy(to, from, length);
    to[length] = '\0';
    if (to[length - 1] == '.') {
        to[--length] = '\0';
    }
    if (length == 0) {
        return MDNSERR_BADPARAM;
    }
    if (strchr(to, '.') == NULL) {
        if (length + 6 >= size) {
            return MDNSERR_BADPARAM;
        }
        memcpy(to + length, ".local", 7);
    }
    return mdnsNameToWire(to, wire, sizeof(wire), &used) == 0 ?
        MDNS_OK : MDNSERR_BADPARAM;
}

static void
wake(struct QuerySession *session, LONG result)
{
    struct MDNSEventRequest *request = session->request;

    if (request == NULL) {
        return;
    }
    session->request = NULL;
    if (FindTaskByPID(session->owner) == NULL) {
        return;
    }
    request->er_Result = result;
    ReplyMsg(&request->er_Msg);
}

static BOOL
same_record(const struct MDNSRecordEvent *a, const struct MDNSRecordEvent *b)
{
    return a->re_Interface == b->re_Interface && a->re_Type == b->re_Type &&
        a->re_Class == b->re_Class && a->re_DataLength == b->re_DataLength &&
        Stricmp(a->re_Name, b->re_Name) == 0 &&
        memcmp(a->re_Data, b->re_Data, a->re_DataLength) == 0;
}

static void
enqueue(struct QuerySession *session, const struct MDNSRecordEvent *event)
{
    UWORD i;

    if (session->failed) {
        return;
    }
    for (i = 0; i < session->count; i++) {
        struct MDNSRecordEvent *old =
            &session->queue[(session->head + i) % QUERY_QUEUE];

        if (same_record(old, event)) {
            *old = *event;
            wake(session, MDNS_OK);
            return;
        }
    }
    if (session->count == QUERY_QUEUE) {
        session->count = 0;
        session->failed = TRUE;
        wake(session, MDNSERR_BUSY);
        return;
    }
    session->queue[(session->head + session->count++) % QUERY_QUEUE] = *event;
    if (session->timed) {
        session->ended = TRUE;
        session->endResult = MDNS_OK;
    }
    wake(session, MDNS_OK);
}

static void
send_question(struct MDNSDaemon *md, const struct QuerySession *session)
{
    UBYTE packet[512];
    LONG length;

    if (mdnsBuildQuery(packet, sizeof(packet), 0, session->name,
            session->type, DNS_CLASS_IN, &length) == 0) {
        mdnsNetSendAll(md, packet, length);
    }
}

static void
answer_own_host(struct MDNSDaemon *md, struct QuerySession *session)
{
    struct MDNSRecordEvent event;
    char host[MDNS_MAX_HOST + 8];

    if ((session->type != DNS_TYPE_A && session->type != DNS_TYPE_ANY) ||
        md->md_HostAddr == 0 || md->md_HostState != MDNS_NAME_ANNOUNCED) {
        return;
    }
    mdnsHostFqdn(md, host, sizeof(host));
    if (Stricmp(host, session->name) != 0) {
        return;
    }
    memset(&event, 0, sizeof(event));
    event.re_Code = MEVENT_RECORD;
    event.re_Interface = htonl(md->md_HostAddr);
    event.re_TTL = MDNS_TTL_HOST;
    event.re_Type = DNS_TYPE_A;
    event.re_Class = DNS_CLASS_IN;
    event.re_DataLength = 4;
    mdnsTextCopy(event.re_Name, sizeof(event.re_Name), host);
    mdnsAddrData(md->md_HostAddr, event.re_Data);
    enqueue(session, &event);
}

static LONG
copy_rdata(struct MDNSRecordEvent *event, const struct MDNSRecord *record,
    const UBYTE *packet, LONG length)
{
    char name[256];
    LONG used;

    if (record->rdlength > MDNS_RECORD_DATA_MAX) {
        return MDNSERR_BADPARAM;
    }
    if (record->type == DNS_TYPE_PTR) {
        if (mdnsWireToName(packet, length, record->rdoffset, packet, length,
                name, sizeof(name)) < 0 ||
            mdnsNameToWire(name, event->re_Data, MDNS_RECORD_DATA_MAX,
                &used) != 0) {
            return MDNSERR_BADPARAM;
        }
        event->re_DataLength = (UWORD)used;
        return MDNS_OK;
    }
    if (record->type == DNS_TYPE_SRV) {
        if (record->rdlength < 7 ||
            mdnsWireToName(packet, length, record->rdoffset + 6, packet,
                length, name, sizeof(name)) < 0 ||
            mdnsNameToWire(name, event->re_Data + 6,
                MDNS_RECORD_DATA_MAX - 6, &used) != 0) {
            return MDNSERR_BADPARAM;
        }
        memcpy(event->re_Data, record->rdata, 6);
        event->re_DataLength = (UWORD)(used + 6);
        return MDNS_OK;
    }
    memcpy(event->re_Data, record->rdata, record->rdlength);
    event->re_DataLength = record->rdlength;
    return MDNS_OK;
}

void
mdnsQueryRecord(struct MDNSDaemon *md, const struct MDNSRecord *record,
    const UBYTE *packet, LONG length)
{
    struct MDNSQueryState *state = md->md_Query;
    struct MDNSRecordEvent event;
    ULONG link;
    LONG i;
    BOOL filled = FALSE;

    if (state == NULL || (record->rclass & 0x7fff) != DNS_CLASS_IN ||
        md->md_SendIf < 0) {
        return;
    }
    link = md->md_Ifs[md->md_SendIf].mi_Addr;
    for (i = 0; i < QUERY_SESSIONS; i++) {
        struct QuerySession *session = &state->sessions[i];

        if (session->handle == NULL || session->failed || session->ended ||
            (session->type != DNS_TYPE_ANY && session->type != record->type) ||
            Stricmp(session->name, record->name) != 0) {
            continue;
        }
        if (!filled) {
            memset(&event, 0, sizeof(event));
            event.re_Code = MEVENT_RECORD;
            event.re_Interface = htonl(link);
            event.re_TTL = record->ttl;
            event.re_Type = record->type;
            event.re_Class = (UWORD)(record->rclass & 0x7fff);
            mdnsTextCopy(event.re_Name, sizeof(event.re_Name), record->name);
            if (copy_rdata(&event, record, packet, length) != MDNS_OK) {
                return;
            }
            filled = TRUE;
        }
        enqueue(session, &event);
    }
}

LONG
mdnsQueryMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg)
{
    struct MDNSQueryState *state = md->md_Query;
    struct QuerySession *session = NULL;
    BOOL create = msg->mi_Command == MDNS_IPC_QUERY;
    LONG i;

    if (!create && msg->mi_DaemonID != md->md_DaemonID) {
        return MDNSERR_NOENGINE;
    }
    if (create && state == NULL) {
        state = AllocMem(sizeof(*state), MEMF_CLEAR);
        if (state == NULL) {
            return MDNSERR_NOMEM;
        }
        state->nextHandle = 1;
        md->md_Query = state;
    }
    if (state == NULL) {
        return MDNSERR_BADPARAM;
    }
    if (create) {
        LONG owned = 0;
        LONG rc;

        if (msg->mi_QueryType == 0 || msg->mi_QueryTimeout > 60) {
            return MDNSERR_BADPARAM;
        }
        for (i = 0; i < QUERY_SESSIONS; i++) {
            if (state->sessions[i].handle != NULL &&
                state->sessions[i].owner == msg->mi_TaskID) {
                owned++;
            }
        }
        if (owned >= 4 || state->nextHandle == 0) {
            return MDNSERR_BUSY;
        }
        for (i = 0; i < QUERY_SESSIONS; i++) {
            if (state->sessions[i].handle == NULL) {
                session = &state->sessions[i];
                break;
            }
        }
        if (session == NULL) {
            return MDNSERR_BUSY;
        }
        memset(session, 0, sizeof(*session));
        rc = normalize_name(session->name, sizeof(session->name),
            msg->mi_QueryName);
        if (rc != MDNS_OK) {
            return rc;
        }
        session->owner = msg->mi_TaskID;
        session->handle = (APTR)state->nextHandle++;
        session->type = msg->mi_QueryType;
        session->timed = msg->mi_QueryTimeout != 0;
        session->deadline = mdnsTicks() + (ULONG)msg->mi_QueryTimeout * 50;
        session->interval = 50;
        session->nextQuery = mdnsTicks();
        msg->mi_Handle = session->handle;
        msg->mi_DaemonID = md->md_DaemonID;
        answer_own_host(md, session);
        return MDNS_OK;
    }
    for (i = 0; i < QUERY_SESSIONS; i++) {
        if (state->sessions[i].handle == msg->mi_Handle &&
            state->sessions[i].owner == msg->mi_TaskID) {
            session = &state->sessions[i];
            break;
        }
    }
    if (session == NULL) {
        return MDNSERR_BADPARAM;
    }
    if (msg->mi_Command == MDNS_IPC_READQUERY) {
        if (session->failed) {
            return MDNSERR_BUSY;
        }
        if (session->count == 0) {
            return session->ended && session->endResult != MDNS_OK ?
                session->endResult : MDNSERR_NOTREADY;
        }
        msg->mi_RecordEvent = session->queue[session->head];
        session->head = (session->head + 1) % QUERY_QUEUE;
        session->count--;
        return MDNS_OK;
    }
    if (msg->mi_Command == MDNS_IPC_NOTIFYQUERY) {
        if (msg->mi_Request == NULL || session->request != NULL) {
            return MDNSERR_BADPARAM;
        }
        session->request = msg->mi_Request;
        if (session->failed) {
            wake(session, MDNSERR_BUSY);
        } else if (session->count != 0) {
            wake(session, MDNS_OK);
        } else if (session->ended) {
            wake(session, session->endResult);
        }
        return MDNS_OK;
    }
    if (msg->mi_Command != MDNS_IPC_STOPQUERY) {
        return MDNSERR_BADPARAM;
    }
    wake(session, MDNSERR_CANCELLED);
    memset(session, 0, sizeof(*session));
    return MDNS_OK;
}

void
mdnsQueryTick(struct MDNSDaemon *md)
{
    struct MDNSQueryState *state = md->md_Query;
    ULONG now = mdnsTicks();
    LONG i;

    if (state == NULL) {
        return;
    }
    for (i = 0; i < QUERY_SESSIONS; i++) {
        struct QuerySession *session = &state->sessions[i];

        if (session->handle == NULL || session->failed || session->ended) {
            continue;
        }
        if (session->timed && due(now, session->deadline)) {
            session->ended = TRUE;
            session->endResult = MDNSERR_TIMEOUT;
            wake(session, MDNSERR_TIMEOUT);
            continue;
        }
        if (md->md_Socket < 0 || md->md_IfCount == 0 ||
            !due(now, session->nextQuery)) {
            continue;
        }
        send_question(md, session);
        session->nextQuery = now + session->interval;
        if (session->interval < 180000) {
            session->interval *= 2;
        }
        if (session->interval > 180000) {
            session->interval = 180000;
        }
    }
}

void
mdnsQueryReset(struct MDNSDaemon *md)
{
    struct MDNSQueryState *state = md->md_Query;
    LONG i;

    if (state == NULL) {
        return;
    }
    for (i = 0; i < QUERY_SESSIONS; i++) {
        state->sessions[i].nextQuery = mdnsTicks();
        state->sessions[i].interval = 50;
    }
}

void
mdnsQueryRelease(struct MDNSDaemon *md, ULONG owner)
{
    struct MDNSQueryState *state = md->md_Query;
    LONG i;

    if (state == NULL) {
        return;
    }
    for (i = 0; i < QUERY_SESSIONS; i++) {
        if (state->sessions[i].owner == owner) {
            memset(&state->sessions[i], 0, sizeof(state->sessions[i]));
        }
    }
}

void
mdnsQueryReap(struct MDNSDaemon *md)
{
    struct MDNSQueryState *state = md->md_Query;
    LONG i;

    if (state == NULL) {
        return;
    }
    for (i = 0; i < QUERY_SESSIONS; i++) {
        struct QuerySession *session = &state->sessions[i];

        if (session->handle != NULL && FindTaskByPID(session->owner) == NULL) {
            memset(session, 0, sizeof(*session));
        }
    }
}

void
mdnsQueryCancel(struct MDNSDaemon *md)
{
    struct MDNSQueryState *state = md->md_Query;
    LONG i;

    if (state == NULL) {
        return;
    }
    for (i = 0; i < QUERY_SESSIONS; i++) {
        wake(&state->sessions[i], MDNSERR_NOENGINE);
    }
}

void
mdnsQueryFree(struct MDNSDaemon *md)
{
    if (md->md_Query != NULL) {
        FreeMem(md->md_Query, sizeof(*md->md_Query));
        md->md_Query = NULL;
    }
}
