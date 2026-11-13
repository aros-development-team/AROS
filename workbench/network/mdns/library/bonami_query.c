/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* Generic DNS record queries use the same caller-owned wakeup request as
 * browse and resolve, but have a distinct event structure. LookupHostA()
 * is a bounded synchronous consumer of a one-shot A query. */

#include <exec/types.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <aros/libcall.h>
#include <string.h>

#include "bonami_intern.h"

#define DNS_TYPE_A 1

AROS_LH3(LONG, LookupHostA,
    AROS_LHA(STRPTR, host, A0), AROS_LHA(ULONG *, addr, A1),
    AROS_LHA(struct TagItem *, tags, A2),
    struct BonamiIntern *, BonamiBase, 22, Bonami)
{
    AROS_LIBFUNC_INIT
    struct MDNSEventRequest request;
    struct MDNSRecordEvent event;
    struct MsgPort *port;
    MDNSQueryRef ref = NULL;
    ULONG timeout;
    LONG rc;

    if (host == NULL || addr == NULL) {
        return MDNSERR_BADPARAM;
    }
    timeout = GetTagData(MDNS_Timeout, 3, tags);
    if (timeout < 1 || timeout > 60) {
        return MDNSERR_BADPARAM;
    }
    rc = bonamiQueryStart(BonamiBase, host, DNS_TYPE_A, (UWORD)timeout,
        &ref);
    if (rc != MDNS_OK) {
        return rc;
    }
    port = CreateMsgPort();
    if (port == NULL) {
        bonamiQueryStop(BonamiBase, ref);
        return MDNSERR_NOMEM;
    }
    memset(&request, 0, sizeof(request));
    request.er_Msg.mn_ReplyPort = port;
    request.er_Msg.mn_Length = sizeof(request);
    rc = bonamiRequestEvent(BonamiBase, ref, &request);
    if (rc == MDNS_OK) {
        WaitPort(port);
        GetMsg(port);
        rc = request.er_Result;
    }
    if (rc == MDNS_OK) {
        rc = bonamiQueryRead(BonamiBase, ref, &event);
        if (rc == MDNS_OK && event.re_Type == DNS_TYPE_A &&
            event.re_DataLength == sizeof(*addr)) {
            memcpy(addr, event.re_Data, sizeof(*addr));
        } else if (rc == MDNS_OK) {
            rc = MDNSERR_BADPARAM;
        }
    }
    bonamiQueryStop(BonamiBase, ref);
    DeleteMsgPort(port);
    return rc;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, QueryRecordA,
    AROS_LHA(STRPTR, name, A0), AROS_LHA(ULONG, rrtype, D0),
    AROS_LHA(struct TagItem *, tags, A1),
    struct BonamiIntern *, BonamiBase, 23, Bonami)
{
    AROS_LIBFUNC_INIT
    struct TagItem *output;
    MDNSQueryRef ref;
    LONG rc;

    if (GetTagData(MDNS_EventMode, 0, tags) != MDNS_EVENT_QUEUE) {
        return MDNSERR_NOTREADY;
    }
    if (name == NULL || rrtype == 0 || rrtype > 0xffff) {
        return MDNSERR_BADPARAM;
    }
    output = FindTagItem(MDNS_QueryRef, tags);
    if (output == NULL || output->ti_Data == 0) {
        return MDNSERR_BADPARAM;
    }
    rc = bonamiQueryStart(BonamiBase, name, (UWORD)rrtype, 0, &ref);
    if (rc == MDNS_OK) {
        *((MDNSQueryRef *)output->ti_Data) = ref;
    }
    return rc;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, StopQuery,
    AROS_LHA(MDNSQueryRef, ref, A0),
    struct BonamiIntern *, BonamiBase, 24, Bonami)
{
    AROS_LIBFUNC_INIT
    bonamiQueryStop(BonamiBase, ref);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(LONG, ReadMDNSRecordEvent,
    AROS_LHA(MDNSQueryRef, ref, A0),
    AROS_LHA(struct MDNSRecordEvent *, event, A1),
    struct BonamiIntern *, BonamiBase, 25, Bonami)
{
    AROS_LIBFUNC_INIT
    return bonamiQueryRead(BonamiBase, ref, event);
    AROS_LIBFUNC_EXIT
}
