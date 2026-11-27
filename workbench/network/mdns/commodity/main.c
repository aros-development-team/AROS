/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * NetServices - find, reach and share services on the local network
 *               (DNS-SD over mDNS), a commodity on dnssd.library.
 */

/******************************************************************************

    NAME

        NetServices [CX_PRIORITY <n>] [CX_POPKEY <key>] [CX_POPUP]

    SYNOPSIS

        CX_PRIORITY/N/K,CX_POPKEY/K,CX_POPUP/S

    LOCATION

        Tools/Commodities

    FUNCTION

        What Nearby, Handshake and Share do in the shell, in one window:

        Nearby lists the kinds of service on the network, or the services
        of one kind (web servers, file shares, printers...), as they come
        and go. Picking a kind lists its services; picking a service puts
        it into Handshake.

        Handshake tells how to reach a service: its host, its addresses
        (IPv4 and IPv6), its port and what it says of itself.

        Share announces a service of this computer until it is stopped, or
        the commodity quits. If another computer has the name, the service
        is announced under another, and the status line says which.

        The window opens with the hot key, from Exchange, or at start with
        CX_POPUP; closing it hides it, and a service shared stays
        shared.

    INPUTS

        CX_PRIORITY  the broker's priority (default 0)
        CX_POPKEY    the hot key (default "control alt n")
        CX_POPUP     open the window at start

        From Workbench, the same as tool types (CX_POPUP=YES).

    NOTES

        mdnsd must run; until it does, the status line says so.

    SEE ALSO

        Nearby, Handshake, Share, dnssd

******************************************************************************/

#include <exec/types.h>
#include <dos/dos.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <libraries/commodities.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/utility.h>
#include <proto/commodities.h>

#include "netservices.h"

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#include "../strings.h"

#define NS_POPKEY_ID    1

static const char version_tag[] __attribute__((used)) =
    "\0$VER: NetServices 42.0 (21.11.2026)";

/* the startup code's, when started from Workbench */
extern struct WBStartup *WBenchMsg;

CONST_STRPTR NSPopKey = (CONST_STRPTR)NS_POPKEY;

static struct MsgPort *BrokerPort;
static CxObj *Broker;

/* 0, or a CBERR_*: CBERR_DUP when it runs already (that one is told to
 * open its window, NBU_NOTIFY) */
static LONG
ns_broker(LONG priority)
{
    struct NewBroker nb;
    CxObj *filter;
    LONG error = 0;

    nb.nb_Version = NB_VERSION;
    nb.nb_Name = (STRPTR)NS_NAME;
    nb.nb_Title = (STRPTR)SDMsg(MSG_NS_CX_TITLE);
    nb.nb_Descr = (STRPTR)SDMsg(MSG_NS_CX_DESCRIPTION);
    nb.nb_Unique = NBU_NOTIFY | NBU_UNIQUE;
    nb.nb_Flags = COF_SHOW_HIDE;
    nb.nb_Pri = (BYTE)priority;
    nb.nb_Port = BrokerPort;
    nb.nb_ReservedChannel = 0;
    if ((Broker = CxBroker(&nb, &error)) == NULL) {
        return error != 0 ? error : CBERR_SYSERR;
    }
    filter = CxFilter((STRPTR)NSPopKey);
    AttachCxObj(filter, CxSender(BrokerPort, NS_POPKEY_ID));
    AttachCxObj(filter, CxTranslate(NULL));
    AttachCxObj(Broker, filter);
    if (CxObjError(filter) != 0) {
        DeleteCxObjAll(Broker);
        Broker = NULL;
        return CBERR_SYSERR;
    }
    ActivateCxObj(Broker, TRUE);
    return 0;
}

/* FALSE: quit. */
static BOOL
ns_broker_messages(void)
{
    CxMsg *msg;
    BOOL ok = TRUE;

    while ((msg = (CxMsg *)GetMsg(BrokerPort)) != NULL) {
        ULONG id = CxMsgID(msg), type = CxMsgType(msg);

        ReplyMsg((struct Message *)msg);
        if (type == CXM_IEVENT && id == NS_POPKEY_ID) {
            NSWindowOpen();
        } else if (type == CXM_COMMAND) {
            switch (id) {
            case CXCMD_DISABLE:
                ActivateCxObj(Broker, FALSE);
                break;
            case CXCMD_ENABLE:
                ActivateCxObj(Broker, TRUE);
                break;
            case CXCMD_APPEAR:
            case CXCMD_UNIQUE:
                NSWindowOpen();
                break;
            case CXCMD_DISAPPEAR:
                NSWindowClose();
                break;
            case CXCMD_KILL:
                ok = FALSE;
                break;
            }
        }
    }
    return ok;
}

static BOOL
ns_yes(CONST_STRPTR s)
{
    return Stricmp(s, (CONST_STRPTR)"NO") != 0;
}

int
main(void)
{
    static CONST_STRPTR template = "CX_PRIORITY/N/K,CX_POPKEY/K,CX_POPUP/S";
    IPTR args[3] = { 0, 0, 0 };
    struct RDArgs *rda = NULL;
    struct DiskObject *icon = NULL;
    LONG priority = 0;
    BOOL popup = FALSE, ok = TRUE;
    LONG error = CBERR_SYSERR;
    int result = RETURN_FAIL;

    SDLocaleOpen();
    if (WBenchMsg != NULL) {
        BPTR old = CurrentDir(WBenchMsg->sm_ArgList->wa_Lock);

        if ((icon = GetDiskObject(WBenchMsg->sm_ArgList->wa_Name)) != NULL) {
            STRPTR *types = icon->do_ToolTypes;
            STRPTR value;

            if ((value = FindToolType(types, (STRPTR)"CX_PRIORITY")) != NULL) {
                StrToLong(value, &priority);
            }
            if ((value = FindToolType(types, (STRPTR)"CX_POPKEY")) != NULL) {
                NSPopKey = value;
            }
            if ((value = FindToolType(types, (STRPTR)"CX_POPUP")) != NULL) {
                popup = ns_yes(value);
            }
        }
        CurrentDir(old);
    } else if ((rda = ReadArgs(template, args, NULL)) != NULL) {
        if (args[0] != 0) {
            priority = *(LONG *)args[0];
        }
        if (args[1] != 0) {
            NSPopKey = (CONST_STRPTR)args[1];
        }
        popup = args[2] != 0;
    } else {
        PrintFault(IoErr(), (CONST_STRPTR)NS_NAME);
        SDLocaleClose();
        return RETURN_FAIL;
    }

    /* without dnssd.library the commodity runs, and its window says why */
    DNSSDBase = OpenLibrary((CONST_STRPTR)DNSSDNAME, DNSSD_VERSION);
    if (NSDiscoveryInit() && (BrokerPort = CreateMsgPort()) != NULL &&
        (error = ns_broker(priority)) == 0) {
        if (DNSSDBase == NULL) {
            SDFormat(NSD.ds_Status, (LONG)sizeof(NSD.ds_Status),
                SDMsg(MSG_NS_NO_LIBRARY), (CONST_STRPTR)DNSSDNAME);
        }
        if (popup) {
            NSWindowOpen();
        }
        while (ok) {
            ULONG window = NSWindowPort != NULL ? 1UL << NSWindowPort->mp_SigBit : 0;
            ULONG session = NSDiscoverySignal();
            ULONG got = Wait(SIGBREAKF_CTRL_C | (1UL << BrokerPort->mp_SigBit) |
                window | session);

            if (got & SIGBREAKF_CTRL_C) {
                break;
            }
            if (got & session) {
                ULONG changed;

                NSWindowDetach();
                changed = NSDiscoveryEvents();
                NSWindowAttach();
                NSWindowShow(changed);
            }
            if (got & window) {
                ok = NSWindowMessages();
            }
            if (ok) {
                ok = ns_broker_messages();
            }
        }
        result = RETURN_OK;
    } else if (error == CBERR_DUP) {
        result = RETURN_OK;
    } else {
        PrintFault(ERROR_NO_FREE_STORE, (CONST_STRPTR)NS_NAME);
    }

    NSWindowClose();
    NSDiscoveryFree();
    if (Broker != NULL) {
        DeleteCxObjAll(Broker);
    }
    if (BrokerPort != NULL) {
        struct Message *msg;

        while ((msg = GetMsg(BrokerPort)) != NULL) {
            ReplyMsg(msg);
        }
        DeleteMsgPort(BrokerPort);
    }
    if (DNSSDBase != NULL) {
        CloseLibrary(DNSSDBase);
    }
    if (icon != NULL) {
        FreeDiskObject(icon);
    }
    if (rda != NULL) {
        FreeArgs(rda);
    }
    SDLocaleClose();
    return result;
}
