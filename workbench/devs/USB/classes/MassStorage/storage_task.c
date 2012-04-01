/*
 * storage_task.c
 *
 *  Created on: Oct 27, 2008
 *      Author: misc
 */

#define DEBUG 1
#include <aros/debug.h>
#include <aros/macros.h>
#include <devices/timer.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <devices/newstyle.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include "storage.h"

static void DoChangeInt(mss_unit_t *unit)
{
	struct IORequest *msg;

	if (unit->msu_removeInt)
		Cause(unit->msu_removeInt);

	Forbid();
	ForeachNode(&unit->msu_diskChangeList, msg)
	{
		Cause((struct Interrupt *)IOStdReq(msg)->io_Data);
	}
	Permit();

	unit->msu_flags &= ~ MSF_DiskChanged;
}


/* Allocate and initialize the unit structure */
static mss_unit_t *CreateUnit(OOP_Class *cl, OOP_Object *o, uint32_t lun)
{
	mss_unit_t *unit = (mss_unit_t *)AllocVecPooled(SD(cl)->MemPool, sizeof(mss_unit_t));
	unit->msu_unit.unit_MsgPort.mp_SigBit = AllocSignal(-1);
	unit->msu_unit.unit_MsgPort.mp_Flags = PA_SIGNAL;
	unit->msu_unit.unit_MsgPort.mp_SigTask = FindTask(NULL);
	unit->msu_unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
	unit->msu_class = cl;
	unit->msu_object = o;
	unit->msu_flags = 0;
	unit->msu_changeNum = 0;
	unit->msu_lun = lun;
	unit->msu_handler = FindTask(NULL);
	unit->msu_removeInt = NULL;
	NEWLIST(&unit->msu_unit.unit_MsgPort.mp_MsgList);
	NEWLIST(&unit->msu_diskChangeList);

	return unit;
}

/*
 * Unit task.
 * Every single mass storage unit in the system becomes it's own handler task.
 * The task's job is to request periodically the media sense and report any potential
 * disk change soft interrupts. Additionally, it is used to perform all *slow*
 * IO operations.
 *
 * NOTE: Multi-lun usb devices will get more than one unit helper task.
 */
void StorageTask(OOP_Class *cl, OOP_Object *o, uint32_t lun, struct Task *parent)
{
	const char *name = FindTask(NULL)->tc_Node.ln_Name;
	StorageData *mss = OOP_INST_DATA(cl, o);
	uint8_t unitnum = lun + mss->unitNum;
	uint32_t sigset = SIGBREAKF_CTRL_C | SIGF_SINGLE;
	uint32_t rcvd;
	mss_unit_t *unit;
	struct timerequest *timer_io;

	/* Make sure the access is exclusive as long as the init is not ready */
	ObtainSemaphore(&mss->lock);

	unit = CreateUnit(cl, o, lun);

	if (unit)
	{
		sigset |= 1 << unit->msu_unit.unit_MsgPort.mp_SigBit;
		char cmd[6] = {0x1b, 0, 0, 0, 1, 0};

		D(bug("[%s] Good morning!\n", name));

		HIDD_USBStorage_DirectSCSI(o, lun, cmd, 6, NULL, 0, 1);

		HIDD_USBStorage_Inquiry(o, lun, unit->msu_inquiry, 8);

		D(bug("[%s] Inquiry results: %02x %02x %02x %02x %02x %02x %02x %02x\n", name,
				unit->msu_inquiry[0], unit->msu_inquiry[1], unit->msu_inquiry[2], unit->msu_inquiry[3],
				unit->msu_inquiry[4], unit->msu_inquiry[5], unit->msu_inquiry[6], unit->msu_inquiry[7]
				));

		/* Check whether the unit is ready */
		if (HIDD_USBStorage_TestUnitReady(o, lun))
		{
			D(bug("[%s] TestUnitReady OK\n", name));

			/* Success. The capacity should be read soon */
			unit->msu_flags = MSF_DiskPresent | MSF_DiskChanged;
		}
		else
		{
			uint8_t sense[18];

			/* Unit was not ready? Get the sense */
			D(bug("[%s] TestUnitReady NOT OK\n", name));

			HIDD_USBStorage_RequestSense(o, lun, sense, 18);

			/*
			 * If sense reports no media, clear the flags. Otherwise, the unit was OK, just wanted
			 * to report its sense data.
			 */
			if (sense[2] == 0x02 && sense[12] == 0x3a)
			{
				D(bug("[%s] Unit is not ready. No media present.\n", name));

				unit->msu_flags &= ~(MSF_DiskPresent | MSF_DiskChanged);
			}
			else
			{
				D(bug("[%s] Unit ready. Disk changed??\n", name));
				unit->msu_flags |= MSF_DiskPresent | MSF_DiskChanged;
			}
		}

		if (unit->msu_flags & MSF_DiskChanged)
		{
			if (HIDD_USBStorage_ReadCapacity(o, lun, &unit->msu_blockCount, &unit->msu_blockSize))
			{
				D(bug("[%s] Detected capacity: %dMB\n", name, (((unit->msu_blockCount+1) >> 10)*unit->msu_blockSize) >> 10));

				unit->msu_blockShift = AROS_LEAST_BIT_POS(unit->msu_blockSize);
			}
			else {
				D(bug("[%s] Failed to get the capacity\n", name));
				unit->msu_flags &= ~MSF_DiskPresent;
			}
		}

		unit->msu_unitNum = unitnum;

		/* timer request will be used to pool periodically for disk changes */
		timer_io = CreateIORequest(CreateMsgPort(), sizeof(struct timerequest));
		OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)timer_io, 0);

		sigset |= 1 << timer_io->tr_node.io_Message.mn_ReplyPort->mp_SigBit;

		AddTail((struct List *)&SD(cl)->unitList, (struct Node *)unit);

		/* Get rid of exclusive access */
		ReleaseSemaphore(&mss->lock);

		/* Set up idle timer */
		timer_io->tr_node.io_Command = TR_ADDREQUEST;
		timer_io->tr_time.tv_secs = 2;
		timer_io->tr_time.tv_micro = 0;
		SendIO((struct IORequest *)timer_io);

		mss->unit[lun] = unit;

		/* Tell the initiator that we're done with setup */
		Signal(parent, SIGF_SINGLE);

		/* Enter almost endless loop */
		do {
			rcvd = Wait(sigset);

			/* SIGF_SINGLE is sent when the usb stick was removed (object disposed) or reinserted again */
			if (rcvd & SIGF_SINGLE)
			{
				D(bug("[%s] SINGLE sig received\n", name));

				/* Device was plugged back now */
				if (unit->msu_flags & MSF_DeviceRemoved)
				{
					/* Update the object's pointer */
					o = unit->msu_object;
					mss = OOP_INST_DATA(cl, o);

					/* Not removed anymore */
					unit->msu_flags &= ~MSF_DeviceRemoved;
					unit->msu_flags |= MSF_DiskChanged;

					if (unit->msu_flags & MSF_DiskChanged)
					{
						if (HIDD_USBStorage_ReadCapacity(o, lun, &unit->msu_blockCount, &unit->msu_blockSize))
						{
							D(bug("[%s] Detected capacity: %dMB\n", name, (((unit->msu_blockCount+1) >> 10)*unit->msu_blockSize) >> 10));

							unit->msu_blockShift = AROS_LEAST_BIT_POS(unit->msu_blockSize);

							D(bug("[%s] Block size: 0x%x, Shift: %d\n", name, unit->msu_blockSize, unit->msu_blockShift));

							unit->msu_flags |= MSF_DiskPresent;
						}
						else {
							D(bug("[%s] Failed to get the capacity\n", name));
						}
					}

					/* Force the device to re-request the media state */
					SetSignal(0xffffffff, 1 << timer_io->tr_node.io_Message.mn_ReplyPort->mp_SigBit);
				}
				else /* Handle removal of the device */
				{
					/* Cancel the timer */
					if (!CheckIO((struct IORequest *)timer_io))
						AbortIO((struct IORequest *)timer_io);
					WaitIO((struct IORequest *)timer_io);
					SetSignal(0, 1 << timer_io->tr_node.io_Message.mn_ReplyPort->mp_SigBit);

					unit->msu_flags |= MSF_DeviceRemoved | MSF_DiskChanged;
					unit->msu_flags &= ~MSF_DiskPresent;
				}

				/*
				 * If DiskChanged flag is set, call the change ints
				 */

				if (unit->msu_flags & MSF_DiskChanged)
					DoChangeInt(unit);
			}

			if (!(unit->msu_flags & MSF_DeviceRemoved))
			{

				/* Message in IORequest queue? */
				if (rcvd & (1 << unit->msu_unit.unit_MsgPort.mp_SigBit))
				{
//					/* If unit is active, do not handle the IO request */
//					if (!(unit->msu_unit.unit_flags & UNITF_ACTIVE))
//					{
//						unit->msu_unit.unit_flags |= UNITF_ACTIVE;
						struct IORequest *msg = NULL;

						/* Empty the queue */
						while ((msg = (struct IORequest *)GetMsg(&unit->msu_unit.unit_MsgPort)))
						{
							HandleIO(msg, (mss_device_t *)msg->io_Device, unit);

							if (msg->io_Command != TD_ADDCHANGEINT)
								ReplyMsg((struct Message *)msg);
						}

//						unit->msu_unit.unit_flags &= ~(UNITF_ACTIVE);
//					}
				}

				/* Got a timer request? Handle it (TestUnitReady) now */
				if (rcvd & (1 << timer_io->tr_node.io_Message.mn_ReplyPort->mp_SigBit))
				{
					/* Timer request. Perform periodic media sense */

					while (GetMsg(timer_io->tr_node.io_Message.mn_ReplyPort));

					uint8_t unit_ready;

					/* Device is idling. Do media change sense */
					unit_ready = HIDD_USBStorage_TestUnitReady(o, lun);

					if (!unit_ready)
					{
						uint8_t sense[18];
						int i;
						int p1;
						int p2 = (unit->msu_flags & MSF_DiskPresent) ? 1:0;

						D(bug("[%s] Unit was not ready. Requesting sense.\n", name));

						HIDD_USBStorage_RequestSense(o, lun, sense, 18);

						D(bug("[%s] Sense: ", name));
						for (i=0; i < 18; i++)
							D(bug("%02x ", sense[i]));
						D(bug("\n"));

						p1 = (sense[2] == 0x02) ? 1:0;

						if (p1 == p2)
						{
							if (p1 == 0)
							{
								unit->msu_flags |= MSF_DiskPresent;

								D(bug("[%s] Media inserted!\n", name));

								if (HIDD_USBStorage_ReadCapacity(o, lun, &unit->msu_blockCount, &unit->msu_blockSize))
								{
									D(bug("[%s] Detected capacity: %dMB\n", name, (((unit->msu_blockCount+1) >> 10)*unit->msu_blockSize) >> 10));

									unit->msu_blockShift = AROS_LEAST_BIT_POS(unit->msu_blockSize);

									D(bug("[%s] Block size: 0x%x, Shift: %d\n", name, unit->msu_blockSize, unit->msu_blockShift));
								}
							}
							else
							{
								D(bug("[%s] Media removed\n", name));

								unit->msu_flags &= ~MSF_DiskPresent;
							}

							unit->msu_flags |= MSF_DiskChanged;
							unit->msu_changeNum++;
						}
					}
				}
				else
				{
					/*
					 * Something else appeared? Cancel the timer request.
					 */
					if (!CheckIO((struct IORequest *)timer_io))
						AbortIO((struct IORequest *)timer_io);
					WaitIO((struct IORequest *)timer_io);
					SetSignal(0, 1 << timer_io->tr_node.io_Message.mn_ReplyPort->mp_SigBit);
				}

				/*
				 * If DiskChanged flag is set, call the change ints
				 */

				if (unit->msu_flags & MSF_DiskChanged)
					DoChangeInt(unit);

				/*
				 * Set up idle timer. The main loop of the task will be waken up
				 * after two seconds
				 */
				timer_io->tr_node.io_Command = TR_ADDREQUEST;
				timer_io->tr_time.tv_secs = 2;
				timer_io->tr_time.tv_micro = 0;
				SendIO((struct IORequest *)timer_io);
			}
		} while(!(rcvd & SIGBREAKF_CTRL_C));

		if (!CheckIO((struct IORequest *)timer_io))
			AbortIO((struct IORequest *)timer_io);
		WaitIO((struct IORequest *)timer_io);

		/* CTRL_C sent? Make a nice cleanup. */
		DeleteMsgPort(timer_io->tr_node.io_Message.mn_ReplyPort);
		DeleteIORequest((struct IORequest *)timer_io);

		FreeVecPooled(SD(cl)->MemPool, unit);
	}

	D(bug("[%s] Goodbye!\n", name));
}
