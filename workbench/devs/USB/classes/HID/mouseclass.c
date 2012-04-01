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

#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/rawkeycodes.h>

#include <usb/usb.h>
#include <usb/usb_core.h>
#include <usb/hid.h>
#include "hid.h"

#include <proto/oop.h>
#include <proto/dos.h>
#include <proto/input.h>

static void mouse_process();

void METHOD(USBMouse, Hidd_USBHID, ParseReport)
{
	MouseData *mouse = OOP_INST_DATA(cl, o);

	if (mouse->mouse_task)
	{
		int x=0,y=0,z=0,buttons=0;
		int i;

		if (msg->id == mouse->mouse_report)
		{

			int fill = mouse->head - mouse->tail;

			if (mouse->head < mouse->tail)
				fill += RING_SIZE;

			/* Parse the movement and button data stored in this report */
			x = hid_get_data(msg->report, &mouse->loc_x);
			y = hid_get_data(msg->report, &mouse->loc_y);
			z = hid_get_data(msg->report, &mouse->loc_wheel);

			for (i=0; i < mouse->loc_btncnt; i++)
				if (hid_get_data(msg->report, &mouse->loc_btn[i]))
					buttons |= (1 << i);

			if (fill >= (RING_SIZE - 1))
			{
				bug("[Mouse] EVENT QUEUE FULL.\n");
				return;
			}

			/*
			 * Store the parsed event in the ring.
			 */
			mouse->report_ring[mouse->head].dx = x;
			mouse->report_ring[mouse->head].dy = y;
			mouse->report_ring[mouse->head].dz = z;
			mouse->report_ring[mouse->head].btn = buttons;
			mouse->head = (mouse->head+1) % RING_SIZE;

			/*
			 * Signal mouse task. Once triggered it should pop all events
			 * from the ring and pass them to the input.device
			 */
			Signal(mouse->mouse_task, SIGBREAKF_CTRL_F);
		}
		else
		{
			D(bug("[Mouse] Report ID %d. We don't need it\n", msg->id));
		}
	}
}

OOP_Object *METHOD(USBMouse, Root, New)
{
	struct Task *t;
	struct MemList *ml;

	D(bug("[USBMouse] USBMouse::New()\n"));

	o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
	if (o)
	{
		MouseData *mouse = OOP_INST_DATA(cl, o);
		int i;

		mouse->sd = SD(cl);
		mouse->o = o;
		mouse->hd = HIDD_USBHID_GetHidDescriptor(o);
		uint32_t flags;

		mouse->buttonstate = 0;
		mouse->head = mouse->tail = 0;

		HIDD_USBHID_SetProtocol(o, 1);

		D(bug("[USBMouse::New()] Hid descriptor @ %p\n", mouse->hd));
		D(bug("[USBMouse::New()] Number of Report descriptors: %d\n", mouse->hd->bNumDescriptors));

		mouse->reportLength = AROS_LE2WORD(mouse->hd->descrs[0].wDescriptorLength);
		mouse->report = AllocVecPooled(SD(cl)->MemPool, mouse->reportLength);

		D(bug("[USBMouse::New()] Getting report descriptor of size %d\n", mouse->reportLength));

		HIDD_USBHID_GetReportDescriptor(o, mouse->reportLength, mouse->report);

		mouse->nreport = hid_maxrepid(mouse->report, mouse->reportLength) + 1;
		mouse->mouse_report = -1;

		for (i=0; i < mouse->nreport; i++)
		{
			D(bug("[USBMouse::New() Checking report %d\n", i));

			if (hid_locate(mouse->report, mouse->reportLength, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_X),
					i, hid_input, &mouse->loc_x, &flags, &mouse->range_x))
			{
				D(bug("[USBMouse::New()] Mouse coordinates found in report %d\n", i));

				mouse->mouse_report = i;
				mouse->rel_x = flags & HIO_RELATIVE;
				D(bug("[USBMouse::New()] Has %s X ranging from %d to %d\n", mouse->rel_x?"relative":"absolute",
						mouse->range_x.minimum, mouse->range_x.maximum));
			}
			else
				continue;

			if (hid_locate(mouse->report, mouse->reportLength, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Y),
					i, hid_input, &mouse->loc_y, &flags, &mouse->range_y))
			{
				mouse->rel_y = flags & HIO_RELATIVE;
				D(bug("[USBMouse::New()] Has %s Y ranging from %d to %d\n", mouse->rel_y?"relative":"absolute",
						mouse->range_y.minimum, mouse->range_y.maximum));
			}

			if (hid_locate(mouse->report, mouse->reportLength, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Z),
					i, hid_input, &mouse->loc_wheel, &flags, NULL) ||
					hid_locate(mouse->report, mouse->reportLength, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_WHEEL),
							i, hid_input, &mouse->loc_wheel, &flags, NULL))
			{
				if (mouse->loc_wheel.size) {
					mouse->rel_z = flags & HIO_RELATIVE;
					D(bug("[USBMouse::New()] Has %s Z\n", mouse->rel_z ? "relative":"absolute"));
				}
			}

			for (mouse->loc_btncnt = 1; mouse->loc_btncnt <= MAX_BTN; mouse->loc_btncnt++)
			{
				if (!hid_locate(mouse->report, mouse->reportLength, HID_USAGE2(HUP_BUTTON, mouse->loc_btncnt),
						i, hid_input, &mouse->loc_btn[mouse->loc_btncnt-1], &flags, NULL)) {
					break;
				}
			}
			mouse->loc_btncnt--;

			if (mouse->mouse_report != -1)
				break;
		}

		D(bug("[USBMouse::New()] Pointing device has %d buttons\n", mouse->loc_btncnt));
		struct TagItem tags[] = {
				{ TASKTAG_ARG1,   (IPTR)cl },
				{ TASKTAG_ARG2,   (IPTR)o },
				{ TAG_DONE,       0UL },
		};

		t = AllocMem(sizeof(struct Task), MEMF_PUBLIC|MEMF_CLEAR);
		ml = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry), MEMF_PUBLIC|MEMF_CLEAR);

		if (t && ml)
		{
			char *sp = AllocMem(10240, MEMF_PUBLIC|MEMF_CLEAR);
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

	t->tc_Node.ln_Name = "HID USB Mouse";
	t->tc_Node.ln_Type = NT_TASK;
	t->tc_Node.ln_Pri = 20;     /* same priority as input.device */

	NewAddTask(t, mouse_process, NULL, &tags[0]);
	mouse->mouse_task = t;
		}
	}

	return o;
}

struct pRoot_Dispose {
	OOP_MethodID        mID;
};

void METHOD(USBMouse, Root, Dispose)
{
	MouseData *mouse = OOP_INST_DATA(cl, o);

	Signal(mouse->mouse_task, SIGBREAKF_CTRL_C);

	if (mouse->report)
		FreeVecPooled(SD(cl)->MemPool, mouse->report);

	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

static void mouse_process(OOP_Class *cl, OOP_Object *o)
{
	MouseData *mouse = OOP_INST_DATA(cl, o);
	uint32_t sigset;

	uint8_t dos_not_ready = 1;

	struct MsgPort *port = CreateMsgPort();
	struct IOStdReq *req = (struct IOStdReq *)CreateIORequest(port, sizeof(struct IOStdReq));
	struct Device *InputBase;
	struct IENewTablet          iet;

	struct InputEvent *ie = AllocVec(10 * sizeof(struct InputEvent), MEMF_PUBLIC | MEMF_CLEAR);

	if (OpenDevice("input.device", 0, (struct IORequest *)req, 0))
	{
		DeleteIORequest((struct IORequest *)req);
		DeleteMsgPort(port);
		mouse->mouse_task = NULL;

		bug("[Mouse] Failed to open input.device\n");

		return;
	}

	InputBase = req->io_Device;

	for (;;)
	{
		sigset = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);

		if (dos_not_ready)
		{
			struct Library *l = OpenLibrary("dos.library", 0);
			if (l)
			{
				dos_not_ready = 0;
				CloseLibrary(l);
			}
		}
		else
		{
			if (sigset & SIGBREAKF_CTRL_C)
			{
				D(bug("[Mouse] USB mouse detached. Cleaning up\n"));

				CloseDevice((struct IORequest *)req);
				DeleteIORequest((struct IORequest *)req);
				DeleteMsgPort(port);
				FreeVec(ie);
				return;
			}

			if (sigset & SIGBREAKF_CTRL_F)
			{
				while (mouse->tail != mouse->head)
				{
					int x=0,y=0,z=0,buttons=0,b_down=0,b_up=0;
					int iec = 0;

					x = mouse->report_ring[mouse->tail].dx;
					y = mouse->report_ring[mouse->tail].dy;
					z = mouse->report_ring[mouse->tail].dz;
					buttons = mouse->report_ring[mouse->tail].btn;

					mouse->tail = (mouse->tail + 1 ) % RING_SIZE;

					b_down = (buttons^mouse->buttonstate) & buttons;
					b_up = (buttons^mouse->buttonstate) & ~buttons;

					UWORD qual = PeekQualifier() & ~(IEQUALIFIER_MIDBUTTON | IEQUALIFIER_RBUTTON | IEQUALIFIER_LEFTBUTTON);

					qual |= IEQUALIFIER_RELATIVEMOUSE;

					/* Update the initial qualifier according to the buttonstate */

					if ((buttons & ~b_down) & 1)
						qual |= IEQUALIFIER_LEFTBUTTON;

					if ((buttons & ~b_down) & 2)
						qual |= IEQUALIFIER_RBUTTON;

					if ((buttons & ~b_down) & 4)
						qual |= IEQUALIFIER_MIDBUTTON;

					if (!mouse->rel_x || !mouse->rel_y)
					{
						iet.ient_TabletX = x;
						iet.ient_TabletY = y;
						iet.ient_RangeX = 1 + mouse->range_x.maximum - mouse->range_x.minimum;
						iet.ient_RangeY = 1 + mouse->range_y.maximum - mouse->range_y.minimum;

						ie[iec].ie_Class = IECLASS_NEWPOINTERPOS;
						ie[iec].ie_Code = IECODE_NOBUTTON;
						ie[iec].ie_Qualifier = qual & ~IEQUALIFIER_RELATIVEMOUSE;
						ie[iec].ie_SubClass = IESUBCLASS_NEWTABLET;
						ie[iec].ie_X = 0;
						ie[iec].ie_Y = 0;
						ie[iec].ie_EventAddress = &iet;

						req->io_Data = &ie[iec];
						req->io_Length = sizeof(struct InputEvent);
						req->io_Command = IND_WRITEEVENT;

						DoIO((struct IORequest *)req);
					}

					else if (x!=0 || y!=0)
					{
						ie[iec].ie_Class = IECLASS_RAWMOUSE;
						ie[iec].ie_Code = IECODE_NOBUTTON;
						ie[iec].ie_Qualifier = qual;
						ie[iec].ie_SubClass = 0;
						ie[iec].ie_X = x;
						ie[iec].ie_Y = y;

						iec++;
					}

					if (buttons!=mouse->buttonstate)
					{
						if (b_up & 1)
						{
							ie[iec].ie_Class = IECLASS_RAWMOUSE;
							ie[iec].ie_Code = IECODE_LBUTTON | IECODE_UP_PREFIX;
							ie[iec].ie_Qualifier = qual;
							ie[iec].ie_SubClass = 0;
							ie[iec].ie_X = 0;
							ie[iec].ie_Y = 0;

							iec++;
						}
						if (b_down & 1)
						{
							qual |= IEQUALIFIER_LEFTBUTTON;

							ie[iec].ie_Class = IECLASS_RAWMOUSE;
							ie[iec].ie_Code = IECODE_LBUTTON;
							ie[iec].ie_Qualifier = qual;
							ie[iec].ie_SubClass = 0;
							ie[iec].ie_X = 0;
							ie[iec].ie_Y = 0;

							iec++;
						}
						if (b_up & 2)
						{
							ie[iec].ie_Class = IECLASS_RAWMOUSE;
							ie[iec].ie_Code = IECODE_RBUTTON | IECODE_UP_PREFIX;
							ie[iec].ie_Qualifier = qual;
							ie[iec].ie_SubClass = 0;
							ie[iec].ie_X = 0;
							ie[iec].ie_Y = 0;

							iec++;
						}
						if (b_down & 2)
						{
							qual |= IEQUALIFIER_RBUTTON;

							ie[iec].ie_Class = IECLASS_RAWMOUSE;
							ie[iec].ie_Code = IECODE_RBUTTON;
							ie[iec].ie_Qualifier = qual;
							ie[iec].ie_SubClass = 0;
							ie[iec].ie_X = 0;
							ie[iec].ie_Y = 0;

							iec++;
						}
						if (b_up & 4)
						{
							ie[iec].ie_Class = IECLASS_RAWMOUSE;
							ie[iec].ie_Code = IECODE_MBUTTON | IECODE_UP_PREFIX;
							ie[iec].ie_Qualifier = qual;
							ie[iec].ie_SubClass = 0;
							ie[iec].ie_X = 0;
							ie[iec].ie_Y = 0;

							iec++;
						}
						if (b_down & 4)
						{
							qual |= IEQUALIFIER_MIDBUTTON;

							ie[iec].ie_Class = IECLASS_RAWMOUSE;
							ie[iec].ie_Code = IECODE_MBUTTON;
							ie[iec].ie_Qualifier = qual;
							ie[iec].ie_SubClass = 0;
							ie[iec].ie_X = 0;
							ie[iec].ie_Y = 0;

							iec++;
						}

						mouse->buttonstate = buttons;
					}

					if (z!=0)
					{
						ie[iec].ie_Class = IECLASS_RAWKEY;
						ie[iec].ie_Code = (z > 0) ? RAWKEY_NM_WHEEL_UP : RAWKEY_NM_WHEEL_DOWN;
						ie[iec].ie_Qualifier = qual;
						ie[iec].ie_SubClass = 0;
						ie[iec].ie_X = 0;
						ie[iec].ie_Y = 0;

						iec++;

						ie[iec].ie_Class = IECLASS_RAWKEY;
						ie[iec].ie_Code = ((z > 0) ? RAWKEY_NM_WHEEL_UP : RAWKEY_NM_WHEEL_DOWN) |
						IECODE_UP_PREFIX;
						ie[iec].ie_Qualifier = qual;
						ie[iec].ie_SubClass = 0;
						ie[iec].ie_X = 0;
						ie[iec].ie_Y = 0;

						iec++;

					}

					if (iec)
					{
						req->io_Data = &ie[0];
						req->io_Length = iec * sizeof(struct InputEvent);
						req->io_Command = IND_ADDEVENT;

						DoIO((struct IORequest *)req);
					}

				}
			}
		}
	}
}
