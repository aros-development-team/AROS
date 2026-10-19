/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_client.c - the programs that use mdnsd through bonami.library
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/tasks.h>

#include <proto/exec.h>

#include "mdnsd.h"

#define MDNS_REAP_MAX   16

/* One call of the library. The caller replies to the message. */
void
mdnsClientMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg)
{
    if (msg->mi_Version != MDNS_IPC_VERSION ||
        msg->mi_Msg.mn_Length != sizeof(*msg) || msg->mi_TaskID == 0) {
        msg->mi_Result = MDNSERR_BADPARAM;
        return;
    }
    /* texts end where the library ended them, whoever sent this */
    msg->mi_Name[MDNS_MAX_NAME] = '\0';
    msg->mi_ServiceType[MDNS_MAX_TYPE] = '\0';
    msg->mi_Domain[MDNS_MAX_DOMAIN] = '\0';
    msg->mi_HostName[MDNS_MAX_HOST] = '\0';
    msg->mi_TXT[MDNS_MAX_TXT] = '\0';

    switch (msg->mi_Command) {
    case MDNS_IPC_REGISTER:
        msg->mi_Result = mdnsRegAdd(md, msg);
        break;
    case MDNS_IPC_UPDATE:
        msg->mi_Result = mdnsRegUpdate(md, msg);
        break;
    case MDNS_IPC_UNREGISTER:
        mdnsRegRemove(md, msg);
        msg->mi_Result = MDNS_OK;
        break;
    case MDNS_IPC_RELEASE:
        /* A client is gone: nothing of its may be announced any more. */
        mdnsRegRelease(md, msg->mi_TaskID);
        msg->mi_Result = MDNS_OK;
        break;
    default:
        msg->mi_Result = MDNSERR_BADPARAM;
        break;
    }
}

/*
 * A client that ends without closing bonami.library sends no release, and
 * what it registered would be announced for ever. So the owners are
 * checked by their stable Exec task IDs. FindTaskByPID() includes running
 * and spinning tasks on SMP, with the appropriate Exec list locks. The
 * returned pointer is only tested for NULL, never dereferenced here.
 */
void
mdnsClientReap(struct MDNSDaemon *md)
{
    ULONG dead[MDNS_REAP_MAX];
    const struct Node *n;
    LONG count = 0, i;

    for (n = md->md_Services.lh_Head; n->ln_Succ != NULL; n = n->ln_Succ) {
        ULONG taskID = ((const struct MDNSRegisteredService *)n)->rs_TaskID;

        for (i = 0; i < count && dead[i] != taskID; i++) {
        }
        if (i == count && count < MDNS_REAP_MAX && taskID != 0 &&
            FindTaskByPID(taskID) == NULL) {
            dead[count++] = taskID;
        }
    }
    for (i = 0; i < count; i++) {
        mdnsRegRelease(md, dead[i]);
    }
}
