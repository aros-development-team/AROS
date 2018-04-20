/*
    Copyright © 2015-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Virtual USB host controller
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>
#include <devices/timer.h>

#include "vusbhci_device.h"

#include LC_LIBDEFS_FILE

struct VUSBHCIUnit *VUSBHCI_AddNewUnit200(void);
struct VUSBHCIUnit *VUSBHCI_AddNewUnit300(void);

static void handler_task(struct Task *parent, struct VUSBHCIBase *VUSBHCIBase) {
    Signal(parent, SIGF_CHILD);

    mybug(-1,("[handler_task] Starting\n"));

    const char animate[4]={'/','-','\\','|'};
    static ULONG i;

    struct VUSBHCIUnit *unit = VUSBHCIBase->usbunit200;

    struct timerequest *tr = NULL;
    struct MsgPort *mp = NULL;

    struct IOUsbHWReq *ioreq;

    mp = CreateMsgPort();
    if (mp) {
        tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest));
        if (tr) {
            FreeSignal(mp->mp_SigBit);
            if (!OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0)) {
                /* Allocate a signal within this task context */
                tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit = SIGB_SINGLE;
                tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);
                /* Specify the request */
                tr->tr_node.io_Command = TR_ADDREQUEST;

                /* FIXME: Use signals */
                while(VUSBHCIBase->handler_task_run) {
                    //mybug(-1,("[handler_task] Ping...\n"));

                    if(!unit->ctrlxfer_pending) {
                        ObtainSemaphore(&unit->ctrlxfer_queue_lock); {
                            ForeachNode(&unit->ctrlxfer_queue, ioreq) {
                                /* Now the iorequest lives only on our pointer */
                                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                                mybug(-1,("[handler_task] Control transfer caught...\n"));
                                unit->ctrlxfer_pending = TRUE;
                                do_libusb_ctrl_transfer(ioreq);
                            }
                        } ReleaseSemaphore(&unit->ctrlxfer_queue_lock);
                    }

                    if(!unit->intrxfer_pending) {
                        ObtainSemaphore(&unit->intrxfer_queue_lock); {
                            ForeachNode(&unit->intrxfer_queue, ioreq) {
                                /* Now the iorequest lives only on our pointer */
                                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                                unit->intrxfer_pending = TRUE;
                                do_libusb_intr_transfer(ioreq);
                            }
                        } ReleaseSemaphore(&unit->intrxfer_queue_lock);
                    }

                    if(!unit->bulkxfer_pending) {
                        ObtainSemaphore(&unit->bulkxfer_queue_lock); {
                            ForeachNode(&unit->bulkxfer_queue, ioreq) {
                                /* Now the iorequest lives only on our pointer */
                                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                                unit->bulkxfer_pending = TRUE;
                                do_libusb_bulk_transfer(ioreq);
                            }
                        } ReleaseSemaphore(&unit->bulkxfer_queue_lock);
                    }

                    if(!unit->isocxfer_pending) {
                        ObtainSemaphore(&unit->isocxfer_queue_lock); {
                            ForeachNode(&unit->isocxfer_queue, ioreq) {
                                /* Now the iorequest lives only on our pointer */
                                Remove(&ioreq->iouh_Req.io_Message.mn_Node);
                                unit->isocxfer_pending = TRUE;
                                do_libusb_isoc_transfer(ioreq);
                            }
                        } ReleaseSemaphore(&unit->isocxfer_queue_lock);
                    }

                    //Forbid();
                    call_libusb_event_handler();
                    //Permit();

                    /* Wait */
                    tr->tr_time.tv_secs = 0;
                    tr->tr_time.tv_micro = 1000;
                    DoIO((struct IORequest *)tr);
                }
                CloseDevice((struct IORequest *)tr);
            }
            DeleteIORequest((struct IORequest *)tr);
            mp->mp_SigBit = AllocSignal(-1);
        }
        DeleteMsgPort(mp);
    }

    mybug(-1,("[handler_task] Exiting\n"));

    Signal(parent, SIGF_CHILD);
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR VUSBHCIBase) {
    mybug(-1,("[VUSBHCI] Init: Entering function\n"));

    if(!libusb_bridge_init(VUSBHCIBase)) {
        return FALSE;
    }

    VUSBHCIBase->usbunit200 = VUSBHCI_AddNewUnit200();

    if(VUSBHCIBase->usbunit200 == NULL) {
        mybug(-1, ("[VUSBHCI] Init: Failed to create new USB2.0 unit!\n"));
        return FALSE;
    }

    VUSBHCIBase->usbunit300 = VUSBHCI_AddNewUnit300();

    if(VUSBHCIBase->usbunit300 == NULL) {
        mybug(-1, ("[VUSBHCI] Init: Failed to create new USB3.0 unit!\n"));
        return FALSE;
    }

    /* Create periodic handler task */
    VUSBHCIBase->handler_task_run = TRUE;
    VUSBHCIBase->handler_task = NewCreateTask(TASKTAG_NAME, "libusb handler task",
                                              TASKTAG_PC, handler_task,
                                              TASKTAG_ARG1, FindTask(NULL),
                                              TASKTAG_ARG2, VUSBHCIBase,
                                              TASKTAG_PRI, 5,
                                              TAG_END);
    Wait(SIGF_CHILD);

    return TRUE;
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR VUSBHCIBase, struct IOUsbHWReq *ioreq, ULONG unitnum, ULONG flags) {
    mybug(-1, ("[VUSBHCI] Open: Entering function\n"));
    mybug(-1, ("[VUSBHCI] Open: Unit %d\n", unitnum));

    struct VUSBHCIUnit *unit;

    /* Default to open failure. */
    ioreq->iouh_Req.io_Error = IOERR_OPENFAIL;
    ioreq->iouh_Req.io_Unit = NULL;

    if(unitnum == 0) {
        unit = VUSBHCIBase->usbunit200;
    } else if(unitnum == 1) {
        unit = VUSBHCIBase->usbunit300;
    } else {
        return FALSE;
    }
        
    if(ioreq->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq)) {
        mybug(-1, ("[VUSBHCI] Open: Invalid MN_LENGTH!\n"));
        ioreq->iouh_Req.io_Error = IOERR_BADLENGTH;
    }

    ioreq->iouh_Req.io_Unit = NULL;

    if(unit->allocated) {
        ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
        ioreq->iouh_Req.io_Unit = NULL;
        mybug(-1, ("Unit already in use!\n\n"));
        return FALSE;
    } else {
        unit->allocated = TRUE;
        ioreq->iouh_Req.io_Unit = (struct Unit *) unit;

        /* Opened ok! */
        ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        ioreq->iouh_Req.io_Error                   = 0;
        return TRUE;
    }

    return FALSE;
}

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR VUSBHCIBase, struct IOUsbHWReq *ioreq) {
    mybug(-1, ("[VUSBHCI] Close: Entering function\n"));

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *)ioreq->iouh_Req.io_Unit;

    mybug_unit(-1, ("Closing unit %p\n", unit));

    if(unit) {
        unit->allocated = FALSE;

        ioreq->iouh_Req.io_Unit   = (APTR) -1;
        ioreq->iouh_Req.io_Device = (APTR) -1;

        return TRUE;
    }

    ioreq->iouh_Req.io_Error = IOERR_BADADDRESS;
    ioreq->iouh_Req.io_Unit   = (APTR) -1;
    ioreq->iouh_Req.io_Device = (APTR) -1;

    mybug(-1, ("        Bad unit structure in ioreq, nothing done!\n\n"));
    return FALSE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, BeginIO, AROS_LHA(struct IOUsbHWReq *, ioreq, A1), struct VUSBHCIBase *, VUSBHCIBase, 5, VUSBHCI) {
    AROS_LIBFUNC_INIT

    WORD ret = RC_OK;

    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iouh_Req.io_Error				   = UHIOERR_NO_ERROR;

    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(0, ("Entering function\n"));

    if(unit != NULL) {

        switch (ioreq->iouh_Req.io_Command) {
            case CMD_RESET:
                mybug_unit(-1, ("CMD_RESET\n"));
                break;
            case CMD_FLUSH:
                mybug_unit(-1, ("CMD_FLUSH\n"));
                break;
            case UHCMD_QUERYDEVICE:
                mybug_unit(-1, ("UHCMD_QUERYDEVICE\n"));
                ret = cmdQueryDevice(ioreq);
                break;
            case UHCMD_USBRESET:
                mybug_unit(0, ("UHCMD_USBRESET\n"));
                ret = cmdUsbReset(ioreq);
                break;
            case UHCMD_USBRESUME:
                mybug_unit(-1, ("UHCMD_USBRESUME\n"));
                break;
            case UHCMD_USBSUSPEND:
                mybug_unit(-1, ("UHCMD_USBSUSPEND\n"));
                break;
            case UHCMD_USBOPER:
                mybug_unit(-1, ("UHCMD_USBOPER\n"));
                //ret = cmdUsbOper(ioreq);
                break;
            case UHCMD_CONTROLXFER:
                ret = cmdControlXFer(ioreq);
                break;
            case UHCMD_INTXFER:
                ret = cmdIntXFer(ioreq);
                break;
            case UHCMD_BULKXFER:
                ret = cmdBulkXFer(ioreq);
                break;
            case UHCMD_ISOXFER:
                ret = cmdISOXFer(ioreq);
                break;

            /* Poseidon doesn't actually check this, ever... */
            case NSCMD_DEVICEQUERY:
                mybug_unit(-1, ("NSCMD_DEVICEQUERY\n"));

                static const UWORD NSDSupported[] = {
                    CMD_FLUSH, CMD_RESET,
                    UHCMD_QUERYDEVICE,
                    UHCMD_USBRESET,
                    UHCMD_USBRESUME,
                    UHCMD_USBSUSPEND,
                    UHCMD_USBOPER,
                    UHCMD_CONTROLXFER ,
                    UHCMD_ISOXFER,
                    UHCMD_INTXFER,
                    UHCMD_BULKXFER,
                    NSCMD_DEVICEQUERY,
                    0
                };

                struct NSDeviceQueryResult *nsdq = (struct NSDeviceQueryResult *)((struct IOStdReq *)(ioreq))->io_Data;
                nsdq->DevQueryFormat    = 0;
                nsdq->SizeAvailable     = sizeof(struct NSDeviceQueryResult);
                nsdq->DeviceType        = NSDEVTYPE_USBHARDWARE;
                nsdq->DeviceSubType     = 0;
                nsdq->SupportedCommands = (UWORD *)NSDSupported;
                ret = RC_OK;
                break;
            default:
                mybug_unit(-1, ("IOERR_NOCMD\n"));
                ret = IOERR_NOCMD;
                break;
        }
    } else {
        /* We have aborted the request as unit is invalid */
        ret = IOERR_ABORTED;
    }

    if(ret != RC_DONTREPLY) {
        /* Set error codes */
        if (ret != RC_OK) {
            ioreq->iouh_Req.io_Error = ret & 0xff;
        }
        /* Terminate the iorequest */
        ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;
        /* If not quick I/O, reply the message */
        if(!(ioreq->iouh_Req.io_Flags & IOF_QUICK)) {
            ReplyMsg(&ioreq->iouh_Req.io_Message);
        }
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO, AROS_LHA(struct IOUsbHWReq *, ioreq, A1), struct VUSBHCIBase *, VUSBHCIBase, 6, VUSBHCI) {
    AROS_LIBFUNC_INIT
    mybug(-1, ("[VUSBHCI] AbortIO: Entering function\n"));

    if(ioreq->iouh_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE) {
        if(cmdAbortIO(ioreq)) {
            return(0);
        }
    }

    return(-1);
    AROS_LIBFUNC_EXIT
}

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(-1, ("Entering function\n"));

    struct TagItem *taglist = (struct TagItem *) ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    while((tag = LibNextTagItem(&taglist)) != NULL) {
        switch (tag->ti_Tag) {
            case UHA_Manufacturer:
                *((STRPTR *) tag->ti_Data) = "The AROS Development Team";
                count++;
                break;
            case UHA_Version:
                *((ULONG *) tag->ti_Data) = VERSION_NUMBER;
                count++;
                break;
            case UHA_Revision:
                *((ULONG *) tag->ti_Data) = REVISION_NUMBER;
                count++;
                break;
            case UHA_Copyright:
                *((STRPTR *) tag->ti_Data) = "©2015-2017 The AROS Development Team";
                count++;
                break;
            case UHA_ProductName:
                if(unit->roothub.devdesc.bcdUSB == AROS_WORD2LE(0x0200)) {
                    *((STRPTR *) tag->ti_Data) = "VUSBHCI Host Controller USB2.0";
                } else {
                    *((STRPTR *) tag->ti_Data) = "VUSBHCI Host Controller USB3.0";
                }
                count++;
                break;
            case UHA_Description:
                *((STRPTR *) tag->ti_Data) = "Hosted Host Controller Interface (libusb)";
                count++;
                break;
            case UHA_Capabilities:
                if(unit->roothub.devdesc.bcdUSB == AROS_WORD2LE(0x0200)) {
                    *((ULONG *) tag->ti_Data) = (UHCF_USB20|UHCF_ISO);
                } else {
                    *((ULONG *) tag->ti_Data) = (UHCF_USB30|UHCF_ISO);
                }
                count++;
                break;
            default:
                break;
        }
    }

    mybug_unit(-1, ("Done\n\n"));

    ioreq->iouh_Actual = count;
    return RC_OK;
}

struct VUSBHCIUnit *VUSBHCI_AddNewUnit200(void) {

    struct VUSBHCIUnit *unit;

    static const char name[] = {"[VUSBHCI2.00]"};

    unit = AllocVec(sizeof(struct VUSBHCIUnit), MEMF_ANY|MEMF_CLEAR);

    if(unit == NULL) {
        mybug(-1, ("[VUSBHCI] VUSBHCI_AddNewUnit: Failed to create new unit structure\n"));
        return NULL;
    } else {
        unit->state = UHSF_SUSPENDED;
        unit->allocated = FALSE;

        unit->ctrlxfer_pending = FALSE;
        unit->intrxfer_pending = FALSE;
        unit->bulkxfer_pending = FALSE;
        unit->isocxfer_pending = FALSE;

        NEWLIST(&unit->ctrlxfer_queue);
        NEWLIST(&unit->intrxfer_queue);
        NEWLIST(&unit->bulkxfer_queue);
        NEWLIST(&unit->isocxfer_queue);

        NEWLIST(&unit->roothub.intrxfer_queue);

        /* This is our root hub device descriptor */
        unit->roothub.devdesc.bLength                       = sizeof(struct UsbStdDevDesc);
        unit->roothub.devdesc.bDescriptorType               = UDT_DEVICE;
        unit->roothub.devdesc.bcdUSB                        = AROS_WORD2LE(0x0200);
        unit->roothub.devdesc.bDeviceClass                  = HUB_CLASSCODE;
        unit->roothub.devdesc.bDeviceSubClass               = 0;
        unit->roothub.devdesc.bDeviceProtocol               = 0;
        unit->roothub.devdesc.bMaxPacketSize0               = 64; // Valid values are 8, 9(SuperSpeed), 16, 32, 64
        unit->roothub.devdesc.idVendor                      = AROS_WORD2LE(0x0000);
        unit->roothub.devdesc.idProduct                     = AROS_WORD2LE(0x0000);
        unit->roothub.devdesc.bcdDevice                     = AROS_WORD2LE(0x0200);
        unit->roothub.devdesc.iManufacturer                 = 1;
        unit->roothub.devdesc.iProduct                      = 2;
        unit->roothub.devdesc.iSerialNumber                 = 0;
        unit->roothub.devdesc.bNumConfigurations            = 1;

        /* This is our root hub config descriptor */
        unit->roothub.config.cfgdesc.bLength                = sizeof(struct UsbStdCfgDesc);
        unit->roothub.config.cfgdesc.bLength                = sizeof(struct UsbStdCfgDesc);
        unit->roothub.config.cfgdesc.bDescriptorType        = UDT_CONFIGURATION;
        unit->roothub.config.cfgdesc.wTotalLength           = AROS_WORD2LE(sizeof(struct RHConfig));
        unit->roothub.config.cfgdesc.bNumInterfaces         = 1;
        unit->roothub.config.cfgdesc.bConfigurationValue    = 1;
        unit->roothub.config.cfgdesc.iConfiguration         = 0;
        unit->roothub.config.cfgdesc.bmAttributes           = (USCAF_SELF_POWERED);
        unit->roothub.config.cfgdesc.bMaxPower              = 0;

        unit->roothub.config.ifdesc.bLength                 = sizeof(struct UsbStdIfDesc);
        unit->roothub.config.ifdesc.bDescriptorType         = UDT_INTERFACE;
        unit->roothub.config.ifdesc.bInterfaceNumber        = 0;
        unit->roothub.config.ifdesc.bAlternateSetting       = 0;
        unit->roothub.config.ifdesc.bNumEndpoints           = 1;
        unit->roothub.config.ifdesc.bInterfaceClass         = HUB_CLASSCODE;
        unit->roothub.config.ifdesc.bInterfaceSubClass      = 0;
        unit->roothub.config.ifdesc.bInterfaceProtocol      = 0;
        unit->roothub.config.ifdesc.iInterface              = 0;

        unit->roothub.config.epdesc.bLength                 = sizeof(struct UsbStdEPDesc);
        unit->roothub.config.epdesc.bDescriptorType         = UDT_ENDPOINT;
        unit->roothub.config.epdesc.bEndpointAddress        = (URTF_IN|1);
        unit->roothub.config.epdesc.bmAttributes            = USEAF_INTERRUPT;
        unit->roothub.config.epdesc.wMaxPacketSize          = AROS_WORD2LE(4);
        unit->roothub.config.epdesc.bInterval               = 12;

        /* This is our root hub hub descriptor */
        unit->roothub.hubdesc.bLength             = sizeof(struct UsbHubDesc);
        unit->roothub.hubdesc.bDescriptorType     = UDT_HUB;
        unit->roothub.hubdesc.bNbrPorts           = 1;
        unit->roothub.hubdesc.wHubCharacteristics = AROS_WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP);
        unit->roothub.hubdesc.bPwrOn2PwrGood      = 1;
        unit->roothub.hubdesc.bHubContrCurrent    = 1;
        unit->roothub.hubdesc.DeviceRemovable     = 0;
        unit->roothub.hubdesc.PortPwrCtrlMask     = (1<<1);

        //unit->roothub.portstatus.wPortStatus = 0;
        //unit->roothub.portstatus.wPortChange = 0;

        //unit->roothub.hubstatus->wHubStatus = 0;
        //unit->roothub.hubstatus->wHubChange = 0;

        unit->name = name;

        InitSemaphore(&unit->ctrlxfer_queue_lock);
        InitSemaphore(&unit->intrxfer_queue_lock);
        InitSemaphore(&unit->bulkxfer_queue_lock);
        InitSemaphore(&unit->isocxfer_queue_lock);

        InitSemaphore(&unit->roothub.intrxfer_queue_lock);

        return unit;
    }
}

struct VUSBHCIUnit *VUSBHCI_AddNewUnit300(void) {

    struct VUSBHCIUnit *unit;

    static const char name[] = {"[VUSBHCI3.00]"};

    unit = AllocVec(sizeof(struct VUSBHCIUnit), MEMF_ANY|MEMF_CLEAR);

    if(unit == NULL) {
        mybug(-1, ("[VUSBHCI] VUSBHCI_AddNewUnit: Failed to create new unit structure\n"));
        return NULL;
    } else {
        unit->state = UHSF_SUSPENDED;
        unit->allocated = FALSE;

        NEWLIST(&unit->ctrlxfer_queue);
        NEWLIST(&unit->intrxfer_queue);
        NEWLIST(&unit->bulkxfer_queue);
        NEWLIST(&unit->isocxfer_queue);

        NEWLIST(&unit->roothub.intrxfer_queue);

        /* This is our root hub device descriptor */
        unit->roothub.devdesc.bLength                       = sizeof(struct UsbStdDevDesc);
        unit->roothub.devdesc.bDescriptorType               = UDT_DEVICE;
        unit->roothub.devdesc.bcdUSB                        = AROS_WORD2LE(0x0300);
        unit->roothub.devdesc.bDeviceClass                  = HUB_CLASSCODE;
        unit->roothub.devdesc.bDeviceSubClass               = 0;
        unit->roothub.devdesc.bDeviceProtocol               = 0;
        unit->roothub.devdesc.bMaxPacketSize0               = 9; // Valid values are 8, 9(SuperSpeed), 16, 32, 64
        unit->roothub.devdesc.idVendor                      = AROS_WORD2LE(0x0000);
        unit->roothub.devdesc.idProduct                     = AROS_WORD2LE(0x0000);
        unit->roothub.devdesc.bcdDevice                     = AROS_WORD2LE(0x0300);
        unit->roothub.devdesc.iManufacturer                 = 1;
        unit->roothub.devdesc.iProduct                      = 2;
        unit->roothub.devdesc.iSerialNumber                 = 0;
        unit->roothub.devdesc.bNumConfigurations            = 1;

        /* This is our root hub config descriptor */
        unit->roothub.config.cfgdesc.bLength                = sizeof(struct UsbStdCfgDesc);
        unit->roothub.config.cfgdesc.bLength                = sizeof(struct UsbStdCfgDesc);
        unit->roothub.config.cfgdesc.bDescriptorType        = UDT_CONFIGURATION;
        unit->roothub.config.cfgdesc.wTotalLength           = AROS_WORD2LE(sizeof(struct RHConfig));
        unit->roothub.config.cfgdesc.bNumInterfaces         = 1;
        unit->roothub.config.cfgdesc.bConfigurationValue    = 1;
        unit->roothub.config.cfgdesc.iConfiguration         = 0;
        unit->roothub.config.cfgdesc.bmAttributes           = (USCAF_SELF_POWERED);
        unit->roothub.config.cfgdesc.bMaxPower              = 0;

        unit->roothub.config.ifdesc.bLength                 = sizeof(struct UsbStdIfDesc);
        unit->roothub.config.ifdesc.bDescriptorType         = UDT_INTERFACE;
        unit->roothub.config.ifdesc.bInterfaceNumber        = 0;
        unit->roothub.config.ifdesc.bAlternateSetting       = 0;
        unit->roothub.config.ifdesc.bNumEndpoints           = 1;
        unit->roothub.config.ifdesc.bInterfaceClass         = HUB_CLASSCODE;
        unit->roothub.config.ifdesc.bInterfaceSubClass      = 0;
        unit->roothub.config.ifdesc.bInterfaceProtocol      = 0;
        unit->roothub.config.ifdesc.iInterface              = 0;

        unit->roothub.config.epdesc.bLength                 = sizeof(struct UsbStdEPDesc);
        unit->roothub.config.epdesc.bDescriptorType         = UDT_ENDPOINT;
        unit->roothub.config.epdesc.bEndpointAddress        = (URTF_IN|1);
        unit->roothub.config.epdesc.bmAttributes            = USEAF_INTERRUPT;
        unit->roothub.config.epdesc.wMaxPacketSize          = AROS_WORD2LE(4);
        unit->roothub.config.epdesc.bInterval               = 12;

        /* This is our root hub hub descriptor */
        unit->roothub.sshubdesc.bLength             = sizeof(struct UsbSSHubDesc);
        unit->roothub.sshubdesc.bDescriptorType     = UDT_SSHUB;
        unit->roothub.sshubdesc.bNbrPorts           = 1;
        unit->roothub.sshubdesc.wHubCharacteristics = AROS_WORD2LE(UHCF_INDIVID_POWER|UHCF_INDIVID_OVP);
        unit->roothub.sshubdesc.bPwrOn2PwrGood      = 1;
        unit->roothub.sshubdesc.bHubContrCurrent    = 1;
        unit->roothub.sshubdesc.bHubHdrDecLat       = 0;
        unit->roothub.sshubdesc.wHubDelay           = 0;
        unit->roothub.sshubdesc.DeviceRemovable     = 0;

        unit->name = name;

        InitSemaphore(&unit->ctrlxfer_queue_lock);
        InitSemaphore(&unit->intrxfer_queue_lock);
        InitSemaphore(&unit->bulkxfer_queue_lock);
        InitSemaphore(&unit->isocxfer_queue_lock);

        InitSemaphore(&unit->roothub.intrxfer_queue_lock);
        
        return unit;
    }
}
