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
 * 115200 8N1 with RTS/CTS: what the controller answers to from reset.
 *
 * After the first HCI_Reset it is moved to a faster rate (Broadcom
 * Update_UART_Baud_Rate); Launch_RAM drops it back to 115200.
 */
#define H4BTHCI_BAUD          115200

#define H4BTHCI_BAUD_FAST     3000000     /* with RTS/CTS */
#define H4BTHCI_BAUD_NOFLOW   460800

/* Speed change steps */
#define H4SPEED_IDLE          0
#define H4SPEED_CHANGE        1   /* 0xfc18 sent, at the old rate */
#define H4SPEED_PROBE_FAST    2   /* HCI_Reset sent at the new rate */
#define H4SPEED_PROBE_SLOW    3   /* HCI_Reset sent at 115200 */
#define H4SPEED_POWER         4   /* power cycled, HCI_Reset at 115200 */

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
    BOOL                     hu_Ready;          /* bring-up finished */
    BOOL                     hu_Open;
    ULONG                    hu_Baud;           /* current line rate */
    BOOL                     hu_NoFast;         /* the speed change failed once */
    UBYTE                    hu_Speed;          /* H4SPEED_*, see SpeedUp() */
    ULONG                    hu_SpeedFast;      /* the rate being moved to */
    ULONG                    hu_SpeedDue;       /* DOS tick the step times out at */
    UBYTE                    hu_Held[16];       /* the stack's HCI_Reset reply */
    ULONG                    hu_HeldLen;

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
