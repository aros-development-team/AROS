/*
    Copyright (C) 2006 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
/*
 * CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2009-03-07  T. Wiszkowski       Taught hub to re-attempt device discovery at later time 
 *                                 if no suitable driver is present initially
 * 2008-06-02  T. Wiszkowski       Updated device detection mechanism so the stack is aware of already
 *                                 connected devices
 */

#define DEBUG 0

#include <inttypes.h>

#include <aros/debug.h>

#include <exec/types.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <usb/usb.h>
#include <usb/usb_core.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <devices/timer.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <stdio.h>

#include "usb.h"
#include "misc.h"

#define STACK_SIZE 10240

static void hub_process();

/*
 * The HubInterrupt() is a tiny software interrupt routine that signals the
 * hub process about the need of hub exploration
 */
static AROS_UFH3(void, HubInterrupt,
          AROS_UFHA(APTR, interruptData, A1),
          AROS_UFHA(APTR, interruptCode, A5),
          AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    /* Signal the HUB process about incoming interrupt */
    HubData *hub = interruptData;

    if (hub->hub_task)
        Signal(hub->hub_task, 1 << hub->sigInterrupt);

    AROS_USERFUNC_EXIT
}

/*
 * Initialize HUB class properly. Find out all relevant data and start the
 * HUB process.
 */
OOP_Object *METHOD(USBHub, Root, New)
{
    struct Task    *t;
    struct MemList *ml;

    D(bug("[USB] USBHub::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        HubData *hub = OOP_INST_DATA(cl, o);
        char *name;

        /* HUB devices do need a bit longer timeout than anything else */
        HIDD_USBDevice_SetTimeout(o, NULL, 5000);

        hub->root = GetTagData(aHidd_USBHub_IsRoot, FALSE, msg->attrList);
        hub->enabled = FALSE;
        hub->got_descriptor = FALSE;

        OOP_GetAttr(o, aHidd_USBDevice_ProductName, (intptr_t *)&name);

        hub->got_descriptor = HIDD_USBHub_GetHubDescriptor(o, &hub->descriptor);

        if (hub->got_descriptor)
            DumpDescriptor((usb_descriptor_t *)&hub->descriptor);
        else
        {
            D(bug("[USBHub] HUB descriptor not present. I will try later...\n"));
            hub->descriptor.bNbrPorts = GetTagData(aHidd_USBHub_NumPorts, 1, msg->attrList);
        }

        D(bug("[USBHub] %s hub with %d ports\n",
                hub->root ? "Root" : "A", hub->descriptor.bNbrPorts));

        hub->children = AllocVecPooled(SD(cl)->MemPool, hub->descriptor.bNbrPorts * sizeof(OOP_Object *));

        D(bug("[USB] USBHub has name \"%s\"\n", name));

        hub->sd = SD(cl);
        hub->hub_name = name ? name : "unknown";
        hub->proc_name = AllocVecPooled(SD(cl)->MemPool, 10 + strlen(name ? name : "unknown"));

        sprintf(hub->proc_name, "USBHub (%s)", name ? name : "unknown");

        HIDD_USBDevice_Configure(o, 0);

        usb_endpoint_descriptor_t *ep = HIDD_USBDevice_GetEndpoint(o, 0, 0);

        D(bug("[USBHub] Endpoint descriptor %p\n", ep));

        if (ep)
        {
            DumpDescriptor((usb_descriptor_t *)ep);

            if ((ep->bmAttributes & UE_XFERTYPE) != UE_INTERRUPT)
            {
                bug("[USBHub] Wrong endpoint type\n");
                HIDD_USBDevice_Configure(o, USB_UNCONFIG_INDEX);
// TODO: unconfigure, error, coercemethod
            }

            OOP_Object *drv = NULL;
            OOP_GetAttr(o, aHidd_USBDevice_Bus, (IPTR *)&drv);

            if (drv)
            {
                hub->interrupt.is_Data = hub;
                hub->interrupt.is_Code = HubInterrupt;
                hub->intr_pipe = HIDD_USBDevice_CreatePipe(o, PIPE_Interrupt, ep->bEndpointAddress, ep->bInterval, AROS_LE2WORD(ep->wMaxPacketSize), 0);
//                HIDD_USBDrv_AddInterrupt(drv, hub->intr_pipe, &hub->status[0], AROS_LE2WORD(ep->wMaxPacketSize), &hub->interrupt);
                HIDD_USBDrv_AddInterrupt(drv, hub->intr_pipe, &hub->status[0], 1, &hub->interrupt);
            }

        }

        struct TagItem tags[] = {
                { TASKTAG_ARG1,   (IPTR)hub },
                { TASKTAG_ARG2,   (IPTR)o   },
                { TASKTAG_ARG3,   (IPTR)FindTask(NULL) },
                { TAG_DONE,       0UL },
        };

        t = AllocMem(sizeof(struct Task), MEMF_PUBLIC|MEMF_CLEAR);
        ml = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry), MEMF_PUBLIC|MEMF_CLEAR);

        if (t && ml)
        {
            char *sp = AllocMem(STACK_SIZE, MEMF_PUBLIC|MEMF_CLEAR);
            t->tc_SPLower = sp;
            t->tc_SPUpper = sp + STACK_SIZE;
#if AROS_STACK_GROWS_DOWNWARDS
            t->tc_SPReg = (char *)t->tc_SPUpper - SP_OFFSET;
#else
            t->tc_SPReg = (char *)t->tc_SPLower + SP_OFFSET;
#endif

            ml->ml_NumEntries = 2;
            ml->ml_ME[0].me_Addr = t;
            ml->ml_ME[0].me_Length = sizeof(struct Task);
            ml->ml_ME[1].me_Addr = sp;
            ml->ml_ME[1].me_Length = STACK_SIZE;

            NEWLIST(&t->tc_MemEntry);
            ADDHEAD(&t->tc_MemEntry, &ml->ml_Node);

            t->tc_Node.ln_Name = hub->proc_name;
            t->tc_Node.ln_Type = NT_TASK;
            t->tc_Node.ln_Pri = 0;

            NewAddTask(t, hub_process, NULL, &tags[0]);
            hub->hub_task = t;
        }

        Wait(SIGF_SINGLE);
    }

    D(bug("[USB] USBHub::New() = %p\n",o));

    return o;
}

void METHOD(USBHub, Root, Dispose)
{
    D(bug("[USB] USBHub::Dispose\n"));
    HubData *hub = OOP_INST_DATA(cl, o);
    struct usbEvent message;

    message.ev_Message.mn_ReplyPort = CreateMsgPort();
    message.ev_Type = evt_Cleanup;

    PutMsg(hub->hub_port, (struct Message *)&message);
    WaitPort(message.ev_Message.mn_ReplyPort);
    DeleteMsgPort(message.ev_Message.mn_ReplyPort);

    FreeVecPooled(SD(cl)->MemPool, hub->children);
    FreeVecPooled(SD(cl)->MemPool, hub->proc_name);

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

void METHOD(USBHub, Root, Get)
{
    uint32_t idx;
    HubData *hub = OOP_INST_DATA(cl, o);

    if (IS_USBHUB_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_USBHub_IsRoot:
                *msg->storage = hub->root;
                break;
            case aoHidd_USBHub_IsCompound:
                *msg->storage = 0;
                break;
            case aoHidd_USBHub_HubCurrent:
                *msg->storage = 0;
                break;
            case aoHidd_USBHub_NumPorts:
                *msg->storage = hub->descriptor.bNbrPorts;
                break;
            default:
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
    else
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

void METHOD(USBHub, Root, Set)
{
    uint32_t idx;
    struct TagItem *tag;
    struct TagItem *tags = msg->attrList;

    while ((tag = NextTagItem(&tags)))
    {
        if (IS_USBHUB_ATTR(tag->ti_Tag, idx))
        {

        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object * METHOD(USBHub, Hidd_USBHub, GetChild)
{
    HubData *hub = OOP_INST_DATA(cl, o);
    if ((msg->port > 0) && (msg->port <= hub->descriptor.bNbrPorts))
        return (hub->children[msg->port-1]);
    else
        return NULL;
}

BOOL METHOD(USBHub, Hidd_USBHub, OnOff)
{
    HubData *hub = OOP_INST_DATA(cl, o);
    struct usbEvent message;

    D(bug("[USBHub] USBHub::OnOff(%d)\n", msg->on));

    hub->enabled = msg->on;

    message.ev_Message.mn_ReplyPort = CreateMsgPort();
    message.ev_Type = evt_OnOff;

    PutMsg(hub->hub_port, (struct Message *)&message);
    WaitPort(message.ev_Message.mn_ReplyPort);
    DeleteMsgPort(message.ev_Message.mn_ReplyPort);

    return FALSE;
}

BOOL METHOD(USBHub, Hidd_USBHub, PortEnable)
{
    D(bug("[USBHub] USBHub::PortEnable(%d, %d)\n", msg->portNummer, msg->enable));
    return FALSE;
}

BOOL METHOD(USBHub, Hidd_USBHub, PortReset)
{
    HubData *hub = OOP_INST_DATA(cl, o);
    BOOL retval = FALSE;
    USBDevice_Request req;
    int n = 10;
    usb_port_status_t ps;

    req.bmRequestType = UT_WRITE_CLASS_OTHER;
    req.bRequest = UR_SET_FEATURE;
    req.wValue = AROS_WORD2LE(UHF_PORT_RESET);
    req.wIndex = AROS_WORD2LE(msg->portNummer);
    req.wLength = AROS_WORD2LE(0);

    retval = HIDD_USBDevice_ControlMessage(o, NULL, &req, NULL, 0);

    if (retval) do
    {
        USBDelay(hub->tr, USB_PORT_RESET_DELAY);

        retval = HIDD_USBHub_GetPortStatus(o, msg->portNummer, &ps);

        if (!retval)
            break;

        if (!(AROS_LE2WORD(ps.wPortStatus) & UPS_CURRENT_CONNECT_STATUS))
        {
            retval = TRUE;
            break;
        }
    } while ((AROS_LE2WORD(ps.wPortChange) & UPS_C_PORT_RESET) == 0 && --n > 0);

    if (n==0)
        retval = FALSE;
    else
    {
        HIDD_USBHub_ClearPortFeature(o, msg->portNummer, UHF_C_PORT_RESET);
        USBDelay(hub->tr, USB_PORT_RESET_RECOVERY);
    }

    D(bug("[USBHub] USBHub::PortReset(%d) %s\n", msg->portNummer, retval ? "OK":"Error"));

    return retval;
}

BOOL METHOD(USBHub, Hidd_USBHub, GetPortStatus)
{
    D(bug("[USBHub] USBHub::GetPortStatus()\n"));

    USBDevice_Request req;

    req.bmRequestType = UT_READ_CLASS_OTHER;
    req.bRequest = UR_GET_STATUS;
    req.wValue = AROS_WORD2LE(0);
    req.wIndex = AROS_WORD2LE(msg->port);
    req.wLength = AROS_WORD2LE(sizeof(usb_port_status_t));

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, msg->status, sizeof(usb_port_status_t));
}

BOOL METHOD(USBHub, Hidd_USBHub, GetHubStatus)
{
    D(bug("[USBHub] USBHub::GetHubStatus()\n"));

    USBDevice_Request req;

    req.bmRequestType = UT_READ_CLASS_DEVICE;
    req.bRequest = UR_GET_STATUS;
    req.wValue = AROS_WORD2LE(0);
    req.wIndex = AROS_WORD2LE(0);
    req.wLength = AROS_WORD2LE(sizeof(usb_hub_status_t));

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, msg->status, sizeof(usb_hub_status_t));
}

BOOL METHOD(USBHub, Hidd_USBHub, ClearHubFeature)
{
    D(bug("[USBHub] USBHub::ClearHubFeature()\n"));

    USBDevice_Request req;

    req.bmRequestType = UT_WRITE_CLASS_DEVICE;
    req.bRequest = UR_CLEAR_FEATURE;
    req.wValue = AROS_WORD2LE(msg->feature);
    req.wIndex = AROS_WORD2LE(0);
    req.wLength = AROS_WORD2LE(0);

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, NULL, 0);
}

BOOL METHOD(USBHub, Hidd_USBHub, SetHubFeature)
{
    D(bug("[USBHub] USBHub::SetHubFeature()\n"));

    USBDevice_Request req;

    req.bmRequestType = UT_WRITE_CLASS_DEVICE;
    req.bRequest = UR_SET_FEATURE;
    req.wValue = AROS_WORD2LE(msg->feature);
    req.wIndex = AROS_WORD2LE(0);
    req.wLength = AROS_WORD2LE(0);

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, NULL, 0);
}

BOOL METHOD(USBHub, Hidd_USBHub, ClearPortFeature)
{
    D(bug("[USBHub] USBHub::ClearPortFeature(%p, %d, %d)\n", o, msg->port, msg->feature));

    USBDevice_Request req;

    req.bmRequestType = UT_WRITE_CLASS_OTHER;
    req.bRequest = UR_CLEAR_FEATURE;
    req.wValue = AROS_WORD2LE(msg->feature);
    req.wIndex = AROS_WORD2LE(msg->port);
    req.wLength = AROS_WORD2LE(0);

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, NULL, 0);
}

BOOL METHOD(USBHub, Hidd_USBHub, SetPortFeature)
{
    D(bug("[USBHub] USBHub::SetPortFeature(%p, %d, %d)\n", o, msg->port, msg->feature));

    USBDevice_Request req;

    req.bmRequestType = UT_WRITE_CLASS_OTHER;
    req.bRequest = UR_SET_FEATURE;
    req.wValue = AROS_WORD2LE(msg->feature);
    req.wIndex = AROS_WORD2LE(msg->port);
    req.wLength = AROS_WORD2LE(0);

    return HIDD_USBDevice_ControlMessage(o, NULL, &req, NULL, 0);
}

BOOL METHOD(USBHub, Hidd_USBHub, GetHubDescriptor)
{
    USBDevice_Request request = {
            bmRequestType:  UT_READ_CLASS_DEVICE,
            bRequest:       UR_GET_DESCRIPTOR,
            wValue:         AROS_WORD2LE(((uint8_t)UDESC_HUB) << 8),
            wIndex:         AROS_WORD2LE(0),
            wLength:        AROS_WORD2LE(USB_HUB_DESCRIPTOR_SIZE)
    };

    return HIDD_USBDevice_ControlMessage(o, NULL, &request, msg->descriptor, USB_HUB_DESCRIPTOR_SIZE);
}



static void hub_enable(OOP_Class *cl, OOP_Object *o)
{
    HubData *hub = OOP_INST_DATA(cl, o);
    int pwrdly, port;

    if (!hub->got_descriptor)
        hub->got_descriptor = HIDD_USBHub_GetHubDescriptor(o, &hub->descriptor);

    pwrdly = hub->descriptor.bPwrOn2PwrGood * UHD_PWRON_FACTOR + USB_EXTRA_POWER_UP_TIME;

    for (port = 1; port <= hub->descriptor.bNbrPorts; port++)
    {
        if (!HIDD_USBHub_SetPortFeature(o, port, UHF_PORT_POWER))
            bug("[USBHub] PowerOn on port %d failed\n", port);

        USBDelay(hub->tr, pwrdly);
    }
}

static void hub_disable(OOP_Class *cl, OOP_Object *o)
{
    HubData *hub = OOP_INST_DATA(cl, o);
    int pwrdly, port;

    if (!hub->got_descriptor)
        hub->got_descriptor = HIDD_USBHub_GetHubDescriptor(o, &hub->descriptor);

    pwrdly = hub->descriptor.bPwrOn2PwrGood * UHD_PWRON_FACTOR + USB_EXTRA_POWER_UP_TIME;

    for (port = 1; port <= hub->descriptor.bNbrPorts; port++)
    {
        if (!HIDD_USBHub_ClearPortFeature(o, port, UHF_PORT_POWER))
            bug("[USBHub] PowerOff on port %d failed\n", port);

        USBDelay(hub->tr, pwrdly);
    }
}

static BOOL hub_explore(OOP_Class *cl, OOP_Object *o)
{
    HubData *hub = OOP_INST_DATA(cl, o);
    BOOL more = FALSE;
    int port;

    D(bug("[USBHub Process] hub_explore()\n"));

    if (!hub->got_descriptor)
        hub->got_descriptor = HIDD_USBHub_GetHubDescriptor(o, &hub->descriptor);

    for (port=1; port <= hub->descriptor.bNbrPorts; port++)
    {
        usb_port_status_t port_status;
        uint16_t status, change;

        if (!HIDD_USBHub_GetPortStatus(o, port, &port_status))
        {
            D(bug("[USBHub Process] HIDD_USBHub_GetPortStatus(%p, %d, %p) failed\n",
                  o, port, &status));
            continue;
        }

        status = AROS_LE2WORD(port_status.wPortStatus);
        change = AROS_LE2WORD(port_status.wPortChange);

        D(bug("[USBHub Process]   Port %d, status %04x, change %04x\n", port, status, change));

        if (change & UPS_C_PORT_ENABLED)
        {
            D(bug("[USBHub Process]   C_PORT_ENABLED\n"));
            HIDD_USBHub_ClearPortFeature(o, port, UHF_C_PORT_ENABLE);
// TODO: Extend
        }

        if (change & UPS_C_CONNECT_STATUS)
        {
            D(bug("[USBHub Process]   C_CONNECT_STATUS\n"));
            HIDD_USBHub_ClearPortFeature(o, port, UHF_C_PORT_CONNECTION);
        }

	/*
	 * if connection status has not changed and device is still disconnected skip port.
	 * the original method did not analyse ports after reset.
	 */
        if ((0 == (status & UPS_CURRENT_CONNECT_STATUS)) == (0 == hub->children[port-1]))
        {
	    // D(bug("[USBHub Process]   !C_CONNECT_STATUS\n"));
	    D(bug("[USBHub Process]    UPS_CURRENT_CONNECT_STATUS reflects actual mapping\n"));
            continue;
        }

        if (hub->children[port-1])
        {
            OOP_DisposeObject(hub->children[port-1]);
            hub->children[port-1] = NULL;
        }

        if (!(status & UPS_CURRENT_CONNECT_STATUS))
        {
            D(bug("[USBHub Process]   !CURRENT_CONNECT_STATUS\n"));
            continue;
        }

        if (!(status & UPS_PORT_POWER))
            D(bug("[USBHub Process]   Port %d without power???\n", port));

	/*
	 * i am not sure we want to keep restarting USB ports
	 * in case where we have no driver to utilize the device
	 * however i will leave it here for now
	 */
        USBDelay(hub->tr, USB_PORT_POWERUP_DELAY);
	
	D(bug("[USBHub Process]\tRestarting device in port %d\n", port));
        if (!HIDD_USBHub_PortReset(o, port))
        {
            D(bug("[USBHub Process]   Port %d reset failed\n", port));
            continue;
        }

        if (!HIDD_USBHub_GetPortStatus(o, port, &port_status))
        {
            D(bug("[USBHub Process]   HIDD_USBHub_GetPortStatus(%p, %d, %p) failed\n",
                  o, port, &status));
            continue;
        }
        status = AROS_LE2WORD(port_status.wPortStatus);
        change = AROS_LE2WORD(port_status.wPortChange);

        D(bug("[USBHub Process]   Port %d, status %04x, change %04x\n", port, status, change));

        if (!(status & UPS_CURRENT_CONNECT_STATUS))
        {
            D(bug("[USBHub Process]   Device on port %d disappeared after reset???\n", port));
            continue;
        }

        hub->children[port-1] = HIDD_USB_NewDevice(SD(cl)->usb, o, !(status & UPS_LOW_SPEED));
	if (NULL == hub->children[port-1])
	{
	    D(bug("[USBHub Process]\tNo known handler for selected drive. Restoring connection flag.\n"));
	    more = TRUE;
	    //HIDD_USBHub_SetPortFeature(o, port, UHF_C_PORT_CONNECTION);
	}
    }

    return more;
}

static void hub_process(HubData *hub, OOP_Object *o, struct Task *parent)
{
    struct Task *hub_task = FindTask(NULL);
    struct usb_staticdata *sd = hub->sd;
    OOP_Object *drv = NULL;
    OOP_Class *cl = sd->hubClass;
    struct usbEvent *ev = NULL;
    struct timerequest *rescan;
    uint32_t sigset = 0;
    uint32_t sigmask = 0;

    hub->tr = USBCreateTimer();
    hub->hub_port = CreateMsgPort();

    rescan = USBCreateTimer();

    OOP_GetAttr(o, aHidd_USBDevice_Bus, (IPTR *)&drv);
    SetTaskPri(hub_task, 10);

    D(bug("[USBHub Process] HUB process (%p)\n", FindTask(NULL)));

    Signal(parent, SIGF_SINGLE);

    for (;;)
    {
        D(bug("[USBHub Process] YAWN...\n"));

        sigset = Wait( (1 << hub->hub_port->mp_SigBit) |
		       (sigmask) |
                       (1 << hub->sigInterrupt)
                     );
        D(bug("[USBHub Process] signals rcvd: %p\n", sigset));

        /* handle messages */
        while ((ev = (struct usbEvent *)GetMsg(hub->hub_port)) != NULL)
        {
            BOOL reply = TRUE;

            switch (ev->ev_Type)
            {
                case evt_Startup:
                    D(bug("[USBHub Process] Startup MSG\n"));
                    break;

                case evt_Method:
                    D(bug("[USBHub Process] Sync method call\n"));
                    ev->ev_RetVal = OOP_DoMethod(ev->ev_Target, (OOP_Msg)&ev->ev_Event);
                    break;

                case evt_AsyncMethod:
                    D(bug("[USBHub Process] Async method call\n"));
                    OOP_DoMethod(ev->ev_Target, (OOP_Msg)&ev->ev_Event);
                    FreeVecPooled(sd->MemPool, ev);
                    reply = FALSE;
                    break;

                case evt_Cleanup:
                    D(bug("[USBHub Process] Cleanup MSG\n"));
                    USBDeleteTimer(hub->tr);
                    ReplyMsg(&ev->ev_Message);
                    return;

                case evt_OnOff:
                    D(bug("[USBHub Process] Hub %s\n", hub->enabled ? "on" : "off"));
                    D(bug("----->MARKER 1<-----\n"));
                    if (hub->enabled)
                        hub_enable(cl, o);
                    else
                        hub_disable(cl, o);
                    D(bug("----->MARKER 2<-----\n"));
                    break;

                default:
                    break;
            }
            D(bug("----->MARKER 3<-----\n"));
            if (reply)
                ReplyMsg(&ev->ev_Message);
        }

        D(bug("----->MARKER 4<-----\n"));

        /* handle signals */
        if ((sigset & (1 << hub->sigInterrupt)) ||
	    (sigset & sigmask))
        {
            struct usb_driver *d = NULL, *d2 = NULL;

	    if (sigset & sigmask)
		USBTimerDone(rescan);

            D(bug("[USBHub Process] Interrupt signalled\n"));

            ObtainSemaphore(&sd->driverListLock);
            ForeachNode(&sd->driverList, d)
            {
                if (d->d_Driver == drv)
                {
                    d2 = d;
                    break;
                }
            }
            ReleaseSemaphore(&sd->driverListLock);

            if (d2)
            {
                ObtainSemaphore(&d2->d_Lock);

		/*
		 * check if some devices failed to be created
		 * if so, attempt to re-detect in 10 seconds,
		 * maybe we will have adequate drivers available
		 */
                if (hub_explore(cl, o))
		{
		    /* wake up in 10 secs and check again */
		    sigmask = USBTimer(rescan, 10000);
		}
                ReleaseSemaphore(&d2->d_Lock);
            }
        }
	else
	{
	    sigset = 0;
	}
    }
}
