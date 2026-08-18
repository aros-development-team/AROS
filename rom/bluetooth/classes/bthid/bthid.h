#ifndef BTHID_H
#define BTHID_H

/*
 * bthid.class -- Bluetooth HID class driver (HID over GATT now, HIDP later).
 * Structure follows the Poseidon boot classes: one binding per service, a
 * task per binding that reads input via bluetooth.library channels and
 * feeds decoded events into input.device.
 */

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <exec/libraries.h>
#include <exec/tasks.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <libraries/bluetooth.h>
#include <libraries/btclass.h>

#include <btcore/hid_report.h>
#include <btcore/hid_input.h>
#include <btcore/aros_input_bridge.h>

struct BTHidBase;

struct BTHidBinding
{
    struct Node         nhb_Node;         /* in nh_Bindings */
    struct BTHidBase   *nhb_ClsBase;
    struct Library     *nhb_Base;         /* bluetooth.library (task's own) */
    struct BtDevice    *nhb_Device;
    struct BtService   *nhb_Service;
    struct Task        *nhb_ReadySigTask;
    LONG                nhb_ReadySignal;
    struct Task        *nhb_Task;
    BOOL                nhb_Classic;      /* HIDP (BR/EDR) rather than HOGP */

    /* task-owned state */
    struct MsgPort     *nhb_ChannelPort;
    struct MsgPort     *nhb_InputPort;
    struct IOStdReq    *nhb_InputIO;
    BOOL                nhb_InputOpen;

    struct bt_hid_report_descriptor nhb_Descriptor;
    struct bt_hid_input nhb_Input;
    struct bt_aros_input_bridge nhb_Bridge;
    BOOL                nhb_HaveDescriptor;
};

struct BTHidBase
{
    struct Library      nh_Library;
    UWORD               nh_Flags;
    struct Library     *nh_UtilityBase;
    struct List         nh_Bindings;
};

#endif /* BTHID_H */
