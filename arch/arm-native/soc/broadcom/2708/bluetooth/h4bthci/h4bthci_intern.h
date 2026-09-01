/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: HCI transport for the Raspberry Pi's onboard controller.
*/

#ifndef H4BTHCI_INTERN_H
#define H4BTHCI_INTERN_H

#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <devices/bluetoothhci.h>

/*
 * The Bluetooth stack talks to HCI devices in DEVS:Bluetooth; upstream ships
 * one, vbthci, and it is virtual. This is the other kind: a real controller,
 * reached over the PL011 that pl011bt.resource beside this one owns.
 *
 * The division of labour is the one that resource's README already states and
 * this file does not revisit. Everything below the HCI boundary -- pin muxing,
 * the 32.768 kHz LPO on GPCLK2, BT_REG_EN, baud rate, flow control -- belongs
 * to pl011bt.resource. Everything here is H4 framing and the AROS device
 * protocol, and it would work over any transport that can move bytes.
 *
 * H4 is the UART transport encoding from the Bluetooth spec: one type byte,
 * then the packet. It is the whole difference between what the stack hands
 * down and what goes on the wire.
 */

#define H4BTHCI_COMMAND    0x01
#define H4BTHCI_ACL        0x02
#define H4BTHCI_SCO        0x03
#define H4BTHCI_EVENT      0x04

/*
 * 115200 8N1 with RTS/CTS.
 *
 * What a BCM43438 answers to from reset, and it stays there until a firmware
 * upload changes it. This device does not upload firmware -- the Broadcom
 * patchram protocol is brcmbt.fwl beside it -- so the
 * higher rates the controller supports afterwards are not reachable yet and
 * naming one here would be a guess about a state we never enter.
 */
#define H4BTHCI_BAUD          115200

#define H4BTHCI_RXBUF         4096

struct H4BTHCIBase
{
    struct Device            hu_Device;
    struct SignalSemaphore   hu_Lock;
    struct H4BTHCIUnit    *hu_Unit;
    struct Library          *hu_PL011BTBase;
};

struct H4BTHCIUnit
{
    struct Unit              hu_Unit;
    struct H4BTHCIBase    *hu_Base;
    struct Task             *hu_Task;
    struct Task             *hu_ReadySigTask;
    BYTE                     hu_ReadySignal;
    BOOL                     hu_Open;

    /* Requests waiting for something to arrive from the controller. */
    struct MinList           hu_EventQueue;
    struct MinList           hu_ACLQueue;
    struct SignalSemaphore   hu_QueueLock;

    /* Ports registered through BTCMD_ADDMSGPORT, told about every event. */
    struct MinList           hu_Listeners;

    /* Reassembly. A UART delivers bytes, not packets. */
    UBYTE                    hu_RX[H4BTHCI_RXBUF];
    ULONG                    hu_RXLen;
};

struct H4BTHCIListener
{
    struct MinNode           hl_Node;
    struct MsgPort          *hl_Port;
};

void h4bthci_UnitTask(void);
LONG h4bthci_QueueRequest(struct H4BTHCIUnit *unit, struct IOBTHCIReq *ioreq);

#endif /* H4BTHCI_INTERN_H */
