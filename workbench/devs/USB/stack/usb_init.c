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

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <oop/oop.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <hidd/hidd.h>
#include <usb/usb.h>

#include <devices/timer.h>

#define DEBUG 1

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include "usb.h"
#include LC_LIBDEFS_FILE

OOP_AttrBase HiddUSBAttrBase;
OOP_AttrBase HiddUSBDeviceAttrBase;
OOP_AttrBase HiddUSBHubAttrBase;
OOP_AttrBase HiddUSBDrvAttrBase;
OOP_AttrBase HiddAttrBase;

void usb_process();

static void USB_ProcessStarter(struct usb_staticdata *sd, struct Task *parent)
{
	struct Library *DOSBase;
	struct MsgPort *msgPort = CreateMsgPort();
	struct timerequest *tr = CreateIORequest(msgPort, sizeof(struct timerequest));

	OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0);

	D(bug("[USB] Process starter\n"));

	while(!(DOSBase = OpenLibrary("dos.library", 0)))
	{
		tr->tr_time.tv_usec = 0;
		tr->tr_time.tv_sec = 2;
		tr->tr_node.io_Command = TR_ADDREQUEST;

		DoIO((struct IORequest *)tr);
	};

	D(bug("[USB] Process starter: dos.library up and running\n"));
	D(bug("[USB] Process starter: Creating the main USB process\n"));
	struct usbEvent message;

	struct TagItem tags[] = {
			{ NP_Entry,		(IPTR)usb_process },
			{ NP_UserData,	(IPTR)sd },
			{ NP_Priority,	5 },
			{ NP_Name,		(IPTR)"USB" },
			{ NP_WindowPtr, -1 },
			{ TAG_DONE, 	0UL },
	};

	message.ev_Message.mn_ReplyPort = CreateMsgPort();
	message.ev_Type = evt_Startup;

	sd->usbProcess = CreateNewProc(tags);
	PutMsg(&sd->usbProcess->pr_MsgPort, (struct Message *)&message);
	WaitPort(message.ev_Message.mn_ReplyPort);
	DeleteMsgPort(message.ev_Message.mn_ReplyPort);

	CloseLibrary(DOSBase);
	CloseDevice((struct IORequest *)tr);
	DeleteIORequest(tr);
	DeleteMsgPort(msgPort);

	if (parent)
		Signal(parent, SIGF_SINGLE);
}

static int USB_Init(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = {
            { (STRPTR)IID_Hidd,             &HiddAttrBase },
            { (STRPTR)IID_Hidd_USB,         &HiddUSBAttrBase },
            { (STRPTR)IID_Hidd_USBDevice,   &HiddUSBDeviceAttrBase },
            { (STRPTR)IID_Hidd_USBHub,      &HiddUSBHubAttrBase },
            { (STRPTR)IID_Hidd_USBDrv,      &HiddUSBDrvAttrBase },
            { NULL, NULL }
    };

    D(bug("[USB] USB Init\n"));

    NEWLIST(&LIBBASE->sd.driverList);
    NEWLIST(&LIBBASE->sd.extClassList);
    InitSemaphore(&LIBBASE->sd.global_lock);
    InitSemaphore(&LIBBASE->sd.driverListLock);

    if (OOP_ObtainAttrBases(attrbases))
    {
        LIBBASE->sd.usb = OOP_NewObject(NULL, CLID_Hidd_USB, NULL);

        if ((LIBBASE->sd.MemPool = CreatePool(MEMF_PUBLIC|MEMF_CLEAR|MEMF_SEM_PROTECTED, 8192, 4096)) != NULL)
        {
        	struct Library *DOSBase = OpenLibrary("dos.library", 0);

        	struct TagItem tags[] = {
        			{ TASKTAG_ARG1,   (IPTR)&LIBBASE->sd },
        			{ TASKTAG_ARG2,   0UL },
        			{ TAG_DONE,       0UL }};

        	if (DOSBase)
        		tags[1].ti_Data = (IPTR)FindTask(NULL);

        	struct Task *t = AllocMem(sizeof(struct Task), MEMF_PUBLIC|MEMF_CLEAR);
        	struct MemList *ml = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry), MEMF_PUBLIC|MEMF_CLEAR);

        	if (t && ml)
        	{
        		uint8_t *sp = AllocMem(10240, MEMF_PUBLIC|MEMF_CLEAR);
        		uint8_t *name = "USB Process starter";

        		t->tc_SPLower = sp;
        		t->tc_SPUpper = sp + 10240;
#if AROS_STACK_GROWS_DOWNWARDS
        		t->tc_SPReg = (char *)t->tc_SPUpper - SP_OFFSET;
#else
        		t->tc_SPReg = (char *)t->tc_SPLower + SP_OFFSET;
#endif

        		ml->ml_NumEntries = 2;
        		ml->ml_ME[0].me_Addr = t;
        		ml->ml_ME[0].me_Length = sizeof(struct Task);
        		ml->ml_ME[1].me_Addr = sp;
        		ml->ml_ME[1].me_Length = 10240;

        		NEWLIST(&t->tc_MemEntry);
        		ADDHEAD(&t->tc_MemEntry, &ml->ml_Node);

        		t->tc_Node.ln_Name = name;
        		t->tc_Node.ln_Type = NT_TASK;
        		t->tc_Node.ln_Pri = 1;     /* same priority as input.device */

        		/* Add task. It will get back in touch soon */
        		NewAddTask(t, USB_ProcessStarter, NULL, &tags[0]);

        		if (DOSBase)
        			Wait(SIGF_SINGLE);

        		return TRUE;
        	}

        	if (DOSBase)
        		CloseLibrary(DOSBase);
        }
        OOP_ReleaseAttrBases(attrbases);
    }

    return FALSE;
}

static int USB_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = {
            { (STRPTR)IID_Hidd,                 &HiddAttrBase },
            { (STRPTR)IID_Hidd_USB,             &HiddUSBAttrBase },
            { (STRPTR)IID_Hidd_USBDevice,       &HiddUSBDeviceAttrBase },
            { (STRPTR)IID_Hidd_USBHub,          &HiddUSBHubAttrBase },
            { (STRPTR)IID_Hidd_USBDrv,          &HiddUSBDrvAttrBase },
            { NULL, NULL }
    };
    struct usbEvent message;

    D(bug("[USB] USB Expunge\n"));

    message.ev_Message.mn_ReplyPort = CreateMsgPort();
    message.ev_Type = evt_Cleanup;

    PutMsg(&LIBBASE->sd.usbProcess->pr_MsgPort, (struct Message *)&message);
    WaitPort(message.ev_Message.mn_ReplyPort);
    DeleteMsgPort(message.ev_Message.mn_ReplyPort);

    OOP_ReleaseAttrBases(attrbases);

    DeletePool(LIBBASE->sd.MemPool);

    return TRUE;
}

ADD2INITLIB(USB_Init, 0)
ADD2EXPUNGELIB(USB_Expunge, 0)
