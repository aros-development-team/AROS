/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_client.c - the programs that use mdnsd through bonami.library
 *
 * Their registrations are text: an instance name, a type "_http._tcp", a
 * domain, TXT items "key=value" with spaces between them, a host and one
 * IPv4 address. Each becomes a canonical registration (dnssd_reg.h) here,
 * which mdns_reg.c keeps as it keeps dnssd.library's; whose it is is the
 * client task's ID.
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/tasks.h>

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include "mdns_bonami.h"

#define MDNS_REAP_MAX   16

/* A DNS-SD subtype is one underscored DNS label. Keep the same conservative
 * LDH alphabet used for service labels; comparison on the wire is
 * case-insensitive. */
static BOOL
valid_subtype(const char *subtype)
{
    LONG i;
    LONG length = mdnsTextLength(subtype, MDNS_MAX_SUBTYPE + 1);

    if (length < 2 || length > MDNS_MAX_SUBTYPE || subtype[0] != '_') {
        return FALSE;
    }
    for (i = 1; i < length; i++) {
        UBYTE c = (UBYTE)subtype[i];

        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
        if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') &&
            c != '-') {
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * TXT data from a client's text: "key=value" items with a space between
 * them, each a string of its own (RFC 6763 section 6). Its length.
 */
static UWORD
txt_from_text(const char *text, UBYTE *data, LONG size)
{
    LONG length = 0;
    LONG pos = 0;

    for (;;) {
        UBYTE item[255];
        LONG len = mdnsTxtNext(text, &pos, item, (LONG)sizeof(item));

        if (len < 0) {
            break;
        }
        if (len > 0 && length + 1 + len <= size) {
            data[length++] = (UBYTE)len;
            memcpy(data + length, item, (size_t)len);
            length += len;
        }
    }
    return (UWORD)length;
}

/*
 * A client registers a service. Its texts as a registration: the name the
 * instance's label as it is, the type and domain ("local" when it gave
 * none) as wire names. A host it stands in for is "name" or
 * "name.local", with its IPv4 address.
 */
static LONG
reg_add(struct MDNSDaemon *md, struct MDNSIPCMsg *msg)
{
    struct dnssd_reg reg;
    struct dnssd_reg_parts p;
    struct dnssd_model_wire_address address;
    UBYTE type[256], domain[256], subtypes[MDNS_MAX_SUBTYPES * 64];
    UBYTE txt[MDNS_MAX_TXT + 1];
    LONG used, at = 0, i, j;
    ULONG sublen = 0;
    UWORD txtLength;
    APTR ref;

    if (msg->mi_Name[0] == '\0' || msg->mi_ServiceType[0] == '\0' ||
        msg->mi_Port == 0 || msg->mi_TTL > MDNS_MAX_TTL ||
        mdnsTextLength(msg->mi_ServiceType, MDNS_MAX_BROWSE_TYPE + 1) >
            MDNS_MAX_TYPE ||
        ((msg->mi_HostName[0] != '\0') != (msg->mi_HostAddr != 0)) ||
        mdnsNameToWire(msg->mi_ServiceType, type, (LONG)sizeof(type), &used) != 0) {
        return MDNSERR_BADPARAM;
    }
    memset(&p, 0, sizeof(p));
    p.instance = (const UBYTE *)msg->mi_Name;
    p.instance_len = (ULONG)strlen(msg->mi_Name);
    p.type = type;
    p.type_len = (ULONG)used;
    if (mdnsNameToWire(msg->mi_Domain[0] != '\0' ? msg->mi_Domain :
            MDNS_DOMAIN_LOCAL, domain, (LONG)sizeof(domain), &used) != 0) {
        return MDNSERR_BADPARAM;
    }
    p.domain = domain;
    p.domain_len = (ULONG)used;
    for (i = 0; i < msg->mi_SubtypeCount; i++) {
        LONG length = mdnsTextLength(msg->mi_Subtypes[i], MDNS_MAX_SUBTYPE);

        if (!valid_subtype(msg->mi_Subtypes[i])) {
            return MDNSERR_BADPARAM;
        }
        for (j = 0; j < i; j++) {
            if (Stricmp(msg->mi_Subtypes[i], msg->mi_Subtypes[j]) == 0) {
                return MDNSERR_BADPARAM;
            }
        }
        subtypes[sublen++] = (UBYTE)length;
        memcpy(subtypes + sublen, msg->mi_Subtypes[i], (size_t)length);
        sublen += (ULONG)length;
    }
    p.subtypes = subtypes;
    p.subtypes_len = sublen;
    if (msg->mi_HostName[0] != '\0') {
        /* announced on behalf of a host that does not speak mDNS itself:
         * its first label, in the mDNS domain */
        while (at < MDNS_MAX_HOST && msg->mi_HostName[at] != '\0' &&
               msg->mi_HostName[at] != '.') {
            at++;
        }
        if (msg->mi_HostName[at] != '\0' &&
            Stricmp(msg->mi_HostName + at, ".local") != 0 &&
            Stricmp(msg->mi_HostName + at, ".local.") != 0) {
            return MDNSERR_BADPARAM;
        }
        memset(&address, 0, sizeof(address));
        address.family = DNSSD_MODEL_ADDR_IPV4;
        address.length = 4;
        mdnsAddrData(msg->mi_HostAddr, address.bytes);
        p.host = (const UBYTE *)msg->mi_HostName;
        p.host_len = (ULONG)at;
        p.addresses = &address;
        p.address_count = 1;
    }
    p.port = msg->mi_Port;
    if (dnssd_reg_init(&reg, &p) != DNSSD_REG_OK) {
        return MDNSERR_BADPARAM;
    }
    txtLength = txt_from_text(msg->mi_TXT, txt, (LONG)sizeof(txt));
    i = mdnsRegAddCanonical(md, &reg, txt, txtLength, msg->mi_TTL,
        msg->mi_TaskID, NULL, NULL, &ref);
    if (i == MDNSD_OK) {
        msg->mi_Handle = ref;
    }
    return i;
}

/* Changes a registered service's text, port or time to live, for the
 * client that registered it. An update that changes nothing is not
 * announced, and a client may use one to learn that the daemon still
 * knows its handle. */
static LONG
reg_update(struct MDNSDaemon *md, const struct MDNSIPCMsg *msg)
{
    const struct MDNSRegisteredService *rs = mdnsRegFind(md, msg->mi_Handle);
    UBYTE txt[MDNS_MAX_TXT + 1];
    UWORD length = 0;

    if (rs == NULL) {
        return MDNSERR_NOTFOUND;
    }
    if (rs->rs_Owner != msg->mi_TaskID || msg->mi_TTL > MDNS_MAX_TTL) {
        return MDNSERR_BADPARAM;
    }
    if (msg->mi_Flags & MDNS_IPC_UPDF_TXT) {
        length = txt_from_text(msg->mi_TXT, txt, (LONG)sizeof(txt));
    }
    return mdnsRegUpdateRef(md, msg->mi_Handle,
        (msg->mi_Flags & MDNS_IPC_UPDF_TXT) ? txt : NULL, length,
        msg->mi_Port, NULL, -1, msg->mi_TTL);
}

/* Withdraws a service, for the client that registered it. */
static void
reg_remove(struct MDNSDaemon *md, const struct MDNSIPCMsg *msg)
{
    const struct MDNSRegisteredService *rs = mdnsRegFind(md, msg->mi_Handle);

    if (rs != NULL && rs->rs_Owner == msg->mi_TaskID) {
        mdnsRegRemoveRef(md, msg->mi_Handle);
    }
}

/* One call of the library. The caller replies to the message. */
void
mdnsClientMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg)
{
    LONG i;

    if (msg->mi_Version != MDNS_IPC_VERSION ||
        msg->mi_Msg.mn_Length != sizeof(*msg) || msg->mi_TaskID == 0) {
        msg->mi_Result = MDNSERR_BADPARAM;
        return;
    }
    /* texts end where the library ended them, whoever sent this */
    msg->mi_Name[MDNS_MAX_NAME] = '\0';
    msg->mi_ServiceType[MDNS_MAX_BROWSE_TYPE] = '\0';
    if (msg->mi_SubtypeCount > MDNS_MAX_SUBTYPES) {
        msg->mi_Result = MDNSERR_BADPARAM;
        return;
    }
    for (i = 0; i < msg->mi_SubtypeCount; i++) {
        msg->mi_Subtypes[i][MDNS_MAX_SUBTYPE] = '\0';
    }
    msg->mi_Domain[MDNS_MAX_DOMAIN] = '\0';
    msg->mi_HostName[MDNS_MAX_HOST] = '\0';
    msg->mi_TXT[MDNS_MAX_TXT] = '\0';

    if (msg->mi_Command == MDNS_IPC_BROWSE ||
        msg->mi_Command == MDNS_IPC_STOPBROWSE ||
        msg->mi_Command == MDNS_IPC_RESOLVE ||
        msg->mi_Command == MDNS_IPC_STOPRESOLVE ||
        msg->mi_Command == MDNS_IPC_READ ||
        msg->mi_Command == MDNS_IPC_NOTIFY) {
        msg->mi_Result = mdnsDiscoveryMessage(md, msg);
        return;
    }
    if (msg->mi_Command == MDNS_IPC_QUERY ||
        msg->mi_Command == MDNS_IPC_STOPQUERY ||
        msg->mi_Command == MDNS_IPC_READQUERY ||
        msg->mi_Command == MDNS_IPC_NOTIFYQUERY) {
        msg->mi_Result = mdnsQueryMessage(md, msg);
        return;
    }
    switch (msg->mi_Command) {
    case MDNS_IPC_STATUS:
        msg->mi_StatusRxPackets = md->md_RxPackets;
        msg->mi_StatusTxPackets = md->md_TxPackets;
        msg->mi_StatusRxAnswers = md->md_RxAnswers;
        msg->mi_StatusDbgRecs = md->md_DiscoveryRecords;
        msg->mi_StatusDbgPtr = md->md_DiscoveryPTR;
        msg->mi_StatusDbgNotify = md->md_DiscoveryNotify;
        msg->mi_Result = MDNS_OK;
        break;
    case MDNS_IPC_REGISTER:
        msg->mi_Result = reg_add(md, msg);
        break;
    case MDNS_IPC_UPDATE:
        msg->mi_Result = reg_update(md, msg);
        break;
    case MDNS_IPC_UNREGISTER:
        reg_remove(md, msg);
        msg->mi_Result = MDNS_OK;
        break;
    case MDNS_IPC_RELEASE:
        /* A client is gone: nothing of its may be announced any more. */
        mdnsRegRemoveOwner(md, msg->mi_TaskID);
        mdnsDiscoveryRelease(md, msg->mi_TaskID);
        mdnsQueryRelease(md, msg->mi_TaskID);
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
        ULONG taskID = ((const struct MDNSRegisteredService *)n)->rs_Owner;

        for (i = 0; i < count && dead[i] != taskID; i++) {
        }
        if (i == count && count < MDNS_REAP_MAX && taskID != 0 &&
            FindTaskByPID(taskID) == NULL) {
            dead[count++] = taskID;
        }
    }
    for (i = 0; i < count; i++) {
        mdnsRegRemoveOwner(md, dead[i]);
    }
    mdnsQueryReap(md);
}
