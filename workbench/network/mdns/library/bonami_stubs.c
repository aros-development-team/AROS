/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 */

/* Local helpers and the upstream Bonami ABI slots whose engines are not yet
 * implemented. The remaining stubs never deliver hooks or take ownership. */
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <aros/libcall.h>
#include <string.h>
#include "bonami_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, MakeTXTRecordA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),
        AROS_LHA(APTR *, record, A1),
        AROS_LHA(ULONG *, recordLen, A2),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 14, Bonami)

/*  FUNCTION
        Allocates a separate copy of the string supplied through the
        MDNS_TXTRecord tag. RegisterServiceA() accepts that tag directly;
        this helper is for clients that need an owned TXT buffer.

    INPUTS
        tags      - Tag list containing MDNS_TXTRecord with a STRPTR value.
        record    - Receives the allocated buffer.
        recordLen - Receives its length in bytes, excluding the trailing NUL.

    RESULT
        MDNS_OK, MDNSERR_BADPARAM, or MDNSERR_NOMEM. On failure, record and
        recordLen are not changed.

    NOTES
        The source may contain at most MDNS_MAX_TXT bytes. Release a
        successful result with FreeTXTRecord().

    SEE ALSO
        FreeTXTRecord(), RegisterServiceA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct TagItem *tag;
    STRPTR text;
    const UBYTE *terminator;
    UBYTE *buffer;
    ULONG length;

    (void)BonamiBase;
    if (tags == NULL || record == NULL || recordLen == NULL) {
        return MDNSERR_BADPARAM;
    }
    tag = FindTagItem(MDNS_TXTRecord, tags);
    text = tag != NULL ? (STRPTR)tag->ti_Data : NULL;
    terminator = text != NULL ?
        memchr(text, '\0', MDNS_MAX_TXT + 1) : NULL;
    if (terminator == NULL) {
        return MDNSERR_BADPARAM;
    }
    length = (ULONG)(terminator - text);
    buffer = AllocMem(MDNS_MAX_TXT + 1, MEMF_CLEAR);
    if (buffer == NULL) {
        return MDNSERR_NOMEM;
    }
    memcpy(buffer, text, length + 1);
    *record = buffer;
    *recordLen = length;
    return MDNS_OK;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeTXTRecord,

/*  SYNOPSIS */
        AROS_LHA(APTR, record, A0),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 15, Bonami)

/*  FUNCTION
        Releases a buffer returned by MakeTXTRecordA().

    INPUTS
        record - Buffer returned by MakeTXTRecordA(), or NULL.

    RESULT
        None.

    SEE ALSO
        MakeTXTRecordA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    if (record != NULL) {
        FreeMem(record, MDNS_MAX_TXT + 1);
    }
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, GetMDNSStatusA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 16, Bonami)

/*  FUNCTION
        Requests one daemon-wide diagnostic snapshot. Recognized tags contain
        ULONG * output pointers. Unknown tags and NULL output pointers are
        ignored.

        MDNS_StatusRxPackets and MDNS_StatusTxPackets count successful socket
        receives and sends. MDNS_StatusRxAnswers counts answer-section records
        parsed from responses. The three debug values count records offered to
        discovery, PTR records among them, and attempts to deliver discovery
        events to client queues.

    INPUTS
        tags - Output tags, or NULL to test daemon reachability.

    RESULT
        MDNS_OK, or an MDNSERR_* value. Outputs are changed only on success.

    NOTES
        Counters reset when mdnsd starts. The daemon reads them in its single
        task, so all values belong to one dispatch point.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MDNSIPCMsg msg;
    struct TagItem *state = tags;
    struct TagItem *tag;
    ULONG *out;
    LONG rc;

    (void)BonamiBase;
    memset(&msg, 0, sizeof(msg));
    msg.mi_Command = MDNS_IPC_STATUS;
    rc = bonamiSend(&msg);
    while (rc == MDNS_OK && state != NULL &&
           (tag = NextTagItem(&state)) != NULL) {
        out = (ULONG *)tag->ti_Data;
        if (out == NULL) {
            continue;
        }
        switch (tag->ti_Tag) {
        case MDNS_StatusRxPackets:
            *out = msg.mi_StatusRxPackets;
            break;
        case MDNS_StatusTxPackets:
            *out = msg.mi_StatusTxPackets;
            break;
        case MDNS_StatusRxAnswers:
            *out = msg.mi_StatusRxAnswers;
            break;
        case MDNS_StatusDbgRecs:
            *out = msg.mi_StatusDbgRecs;
            break;
        case MDNS_StatusDbgPtr:
            *out = msg.mi_StatusDbgPtr;
            break;
        case MDNS_StatusDbgNotify:
            *out = msg.mi_StatusDbgNotify;
            break;
        default:
            break;
        }
    }
    return rc;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, InquiryFromNIPCTagsA,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),
        AROS_LHA(ULONG, maxTime, D0),
        AROS_LHA(ULONG, maxResponses, D1),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 17, Bonami)

/*  FUNCTION
        Compatibility stub for the upstream Bonami ABI. This registration
        release does not implement this operation. No callback is invoked,
        output buffer modified, or memory/reference freed.

    RESULT
        MDNSERR_NOTREADY.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    (void)hook;
    (void)maxTime;
    (void)maxResponses;
    (void)tags;
    return MDNSERR_NOTREADY;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, PublishNIPCEntitiesA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 18, Bonami)

/*  FUNCTION
        Compatibility stub for the upstream Bonami ABI. This registration
        release does not implement this operation. No callback is invoked,
        output buffer modified, or memory/reference freed.

    RESULT
        MDNSERR_NOTREADY.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    (void)tags;
    return MDNSERR_NOTREADY;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH0(void, UnpublishNIPCEntitiesA,

/*  SYNOPSIS */

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 19, Bonami)

/*  FUNCTION
        Compatibility stub for the upstream Bonami ABI. This registration
        release does not implement this operation. No callback is invoked,
        output buffer modified, or memory/reference freed.

    RESULT
        None; this call does nothing.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    AROS_LIBFUNC_EXIT
}

