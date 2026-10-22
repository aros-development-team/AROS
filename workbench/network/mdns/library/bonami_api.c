/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * bonami_api.c - the library's functions
 */

#include <exec/types.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <aros/libcall.h>
#include <string.h>

#include "bonami_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH0(LONG, OpenMDNSEngine,

/*  SYNOPSIS */

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 5, Bonami)

/*  FUNCTION
        Says whether mdnsd, the system's mDNS responder, is running.

    INPUTS
        None.

    RESULT
        MDNS_OK, or MDNSERR_NOENGINE when it is not.

    NOTES
        mdnsd is started by the stack's service manager, not by this call,
        and may stop at any time: every other function says so by itself.

    SEE ALSO
        CloseMDNSEngine()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    return bonamiEngineThere();
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH0(void, CloseMDNSEngine,

/*  SYNOPSIS */

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 6, Bonami)

/*  FUNCTION
        Withdraws everything the calling task registered. mdnsd itself
        stays: it is the system's.

    INPUTS
        None.

    RESULT
        None.

    NOTES
        This differs from upstream Bonami, where the call only dropped an
        engine reference. Here it releases the calling task's registrations.
        The task's last CloseLibrary() does the same. A registration is also
        withdrawn when its registering task ends, even if another task of
        the program still has the library open.

    SEE ALSO
        OpenMDNSEngine(), DeregisterService()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    bonamiDiscoveryRelease(BonamiBase);
    bonamiReleaseTask();
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, RegisterServiceA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 7, Bonami)

/*  FUNCTION
        Offers a service of this host on the link: an instance of a DNS-SD
        service type, with the port it listens on (RFC 6763).

    INPUTS
        tags - MDNS_ServiceName (STRPTR), the instance: required.
               MDNS_ServiceType (STRPTR), as "_http._tcp": required.
               MDNS_Port (UWORD): required, not 0.
               MDNS_TXTRecord (STRPTR): "key=value" items with a space
                   between them; see <bonami/mdns.h>.
               MDNS_TimeToLive (ULONG): seconds, 1 to 4500, of the
                   service's own records.
               MDNS_HostName (STRPTR) and MDNS_HostAddr (ULONG, network
                   order), both: the service is offered for another host,
                   one that does not speak mDNS, under that name and
                   address.
               MDNS_RegisterRef (MDNSRegisterRef *): where to put the
                   handle for UpdateServiceA() and DeregisterService().

    RESULT
        MDNS_OK, or MDNSERR_BADPARAM, MDNSERR_DUPLICATE when this host
        already offers the name for the type, MDNSERR_NOMEM,
        MDNSERR_NOENGINE.

    NOTES
        The call returns at once. The name is asked for on the link first,
        which takes about a second, and announced when no other host has
        it; when one has, the service is announced as "Name (2)", then
        "Name (3)".

        An instance is one DNS label, at most 63 bytes. This release rejects
        literal dots with MDNSERR_BADPARAM instead of escaping them. Local
        duplicate name/type registrations are rejected with MDNSERR_DUPLICATE,
        unlike upstream Bonami. Register from a task that stays alive for
        the desired lifetime; another task keeping the library open does
        not keep this registration alive.

        A proxy target is a single host label or a name in .local, with an
        address. It must differ from this host's current and base names.
        The proxy host name itself is not probed by this implementation;
        the caller must provide a name it is entitled to advertise.

    SEE ALSO
        UpdateServiceA(), DeregisterService()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MDNSIPCMsg msg;
    LONG rc;

    (void)BonamiBase;
    if (tags == NULL) {
        return MDNSERR_BADPARAM;
    }
    memset(&msg, 0, sizeof(msg));
    msg.mi_Command = MDNS_IPC_REGISTER;
    rc = bonamiTagsToMsg(tags, &msg);
    if (rc == MDNS_OK && (msg.mi_Name[0] == '\0' ||
            msg.mi_ServiceType[0] == '\0' || msg.mi_Port == 0)) {
        rc = MDNSERR_BADPARAM;
    }
    if (rc == MDNS_OK) {
        rc = bonamiSend(&msg);
    }
    if (rc == MDNS_OK) {
        struct TagItem *tag = FindTagItem(MDNS_RegisterRef, tags);

        if (tag != NULL && tag->ti_Data != 0) {
            *((MDNSRegisterRef *)tag->ti_Data) = msg.mi_Handle;
        }
    }
    return rc;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, UpdateServiceA,

/*  SYNOPSIS */
        AROS_LHA(MDNSRegisterRef, ref, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 8, Bonami)

/*  FUNCTION
        Changes the text or the port of a registered service and tells the
        link.

    INPUTS
        ref  - the handle RegisterServiceA() gave.
        tags - MDNS_TXTRecord (STRPTR): the new text, in place of the old.
               MDNS_Port (UWORD): the new port.
               MDNS_TimeToLive (ULONG): seconds, 1 to 4500.
               A tag that is not given leaves its value alone; the others
               of RegisterServiceA() are not looked at.

    RESULT
        MDNS_OK, or MDNSERR_BADPARAM, MDNSERR_NOTFOUND, MDNSERR_NOENGINE.

    NOTES
        Only the task that registered a service may change it; a call from
        another task returns MDNSERR_BADPARAM. This differs from upstream
        Bonami. The registration remains owned by its original task even
        if its handle is passed to another task.

    SEE ALSO
        RegisterServiceA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MDNSIPCMsg msg;

    (void)BonamiBase;
    if (ref == NULL || tags == NULL) {
        return MDNSERR_BADPARAM;
    }
    memset(&msg, 0, sizeof(msg));
    {
        LONG rc = bonamiTagsToMsg(tags, &msg);

        if (rc != MDNS_OK) {
            return rc;
        }
    }
    msg.mi_Command = MDNS_IPC_UPDATE;
    msg.mi_Handle = ref;
    msg.mi_Flags = FindTagItem(MDNS_TXTRecord, tags) != NULL ?
        MDNS_IPC_UPDF_TXT : 0;
    return bonamiSend(&msg);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, DeregisterService,

/*  SYNOPSIS */
        AROS_LHA(MDNSRegisterRef, ref, A0),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 9, Bonami)

/*  FUNCTION
        Withdraws a registered service: its records are sent once more
        with no time to live, which takes them out of the caches on the
        link (RFC 6762 section 10.1).

    INPUTS
        ref - the handle RegisterServiceA() gave. NULL does nothing.

    RESULT
        None.

    NOTES
        Unlike upstream Bonami, a call from a task other than the registering
        task is ignored. CloseMDNSEngine(), that task's final CloseLibrary(),
        or its termination withdraws its registrations as well.

    SEE ALSO
        RegisterServiceA(), CloseMDNSEngine()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MDNSIPCMsg msg;

    (void)BonamiBase;
    if (ref == NULL) {
        return;
    }
    memset(&msg, 0, sizeof(msg));
    msg.mi_Command = MDNS_IPC_UNREGISTER;
    msg.mi_Handle = ref;
    bonamiSend(&msg);
    AROS_LIBFUNC_EXIT
}
