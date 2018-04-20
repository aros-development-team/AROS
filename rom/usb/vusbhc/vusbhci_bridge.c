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

#include <proto/exec.h>
#include <proto/hostlib.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>

#include "vusbhci_device.h"
#include "vusbhci_bridge.h"

APTR HostLibBase;
struct libusb_func libusb_func;

static void *libusbhandle;

static libusb_device_handle *dev_handle = NULL;

int hotplug_callback_event_handler(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {

    struct VUSBHCIBase *VUSBHCIBase = (struct VUSBHCIBase *)user_data;
    struct VUSBHCIUnit *unit = VUSBHCIBase->usbunit200;

    struct libusb_device_descriptor desc;
    int rc, speed, num_interfaces;

    mybug(-1, ("\n"));
    mybug_unit(-1, ("Hotplug callback event!\n"));

    switch(event) {

        case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
            mybug_unit(-1, ("- Device attached\n"));

            if(unit->allocated && (!dev_handle)) {
                rc = LIBUSBCALL(libusb_get_device_descriptor, dev, &desc);
                if (LIBUSB_SUCCESS != rc) {
                    mybug_unit(-1, ("Failed to read device descriptor\n"));
                    return 0;
                } else {
                    mybug_unit(-1, ("Device attach: %04x:%04x\n", desc.idVendor, desc.idProduct));

                    rc = LIBUSBCALL(libusb_open, dev, &dev_handle);
                    if(dev_handle) {
                        
                        speed = LIBUSBCALL(libusb_get_device_speed, dev);
                        switch(speed) {
                            case LIBUSB_SPEED_LOW:
                                bug("\nLIBUSB_SPEED_LOW...\n");
                            break;
                            case LIBUSB_SPEED_FULL:
                                bug("\nLIBUSB_SPEED_FULL...\n");
                            break;
                            case LIBUSB_SPEED_HIGH:
                                bug("\nLIBUSB_SPEED_HIGH...\n");
                            break;
                            case LIBUSB_SPEED_SUPER:
                                bug("\nLIBUSB_SPEED_SUPER...\n");
                            break;
                        }

                        if(speed != LIBUSB_SPEED_SUPER) {
                        
                            //LIBUSBCALL(libusb_set_auto_detach_kernel_driver, dev_handle, 1);

                            struct libusb_config_descriptor *config = NULL;
                            unsigned int i;

                            rc = LIBUSBCALL(libusb_get_active_config_descriptor, dev, &config);
                            if(rc == LIBUSB_SUCCESS) {
                            	bug("\nGot active config descriptor %d...\n\n", i);
                            }

							num_interfaces = config->bNumInterfaces;
							//LIBUSBCALL(libusb_free_config_descriptor, config);

                            for (i=0; i<num_interfaces; i++) {
                                if (LIBUSBCALL(libusb_kernel_driver_active, dev_handle, i) == 1) {
                                    bug("Kernel driver is active on interface %d...\n", i);

									rc = LIBUSBCALL(libusb_detach_kernel_driver, dev_handle, i);

                                    rc = LIBUSBCALL(libusb_claim_interface, dev_handle, i);
                                    if (rc != LIBUSB_SUCCESS) {
                                        bug("    Failed to detach it...\n");
                                    }
                                } else {
                                	bug("Kernel driver is not active on interface %d...\n", i);
                                }

                                if (LIBUSBCALL(libusb_kernel_driver_active, dev_handle, i) == 1) {
                                    bug("Kernel driver is active on interface %d...\n", i);

									rc = LIBUSBCALL(libusb_detach_kernel_driver, dev_handle, i);

                                    rc = LIBUSBCALL(libusb_claim_interface, dev_handle, i);
                                    if (rc != LIBUSB_SUCCESS) {
                                        bug("    Failed to detach it...\n");
                                    }
                                } else {
                                	bug("Kernel driver is not active on interface %d...\n", i);
                                }
                            }
                            
                            LIBUSBCALL(libusb_free_config_descriptor, config);


                            //speed = LIBUSBCALL(libusb_get_device_speed, dev);
                            switch(speed) {
                                case LIBUSB_SPEED_LOW:
                                    unit->roothub.portstatus.wPortStatus |= AROS_WORD2LE(UPSF_PORT_LOW_SPEED);
                                break;
                                case LIBUSB_SPEED_FULL:
                                    unit->roothub.portstatus.wPortStatus &= ~(AROS_WORD2LE(UPSF_PORT_HIGH_SPEED)|AROS_WORD2LE(UPSF_PORT_LOW_SPEED));
                                break;
                                case LIBUSB_SPEED_HIGH:
                                    unit->roothub.portstatus.wPortStatus |= AROS_WORD2LE(UPSF_PORT_HIGH_SPEED);
                                break;
                                //case LIBUSB_SPEED_SUPER:
                                //break;
                            }

                			unit->roothub.portstatus.wPortStatus &= ~UPSF_PORT_CONNECTION;
                			unit->roothub.portstatus.wPortChange |= UPSF_PORT_CONNECTION;
                			uhwCheckRootHubChanges(unit);

                            unit->roothub.portstatus.wPortStatus |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                            unit->roothub.portstatus.wPortChange |= AROS_WORD2LE(UPSF_PORT_CONNECTION);
                            uhwCheckRootHubChanges(unit);
                        } else {
                            LIBUSBCALL(libusb_close, dev_handle);
                            dev_handle = NULL;
                        }
                    } else {
                        if(rc == LIBUSB_ERROR_ACCESS) {
                            mybug_unit(-1, ("libusb_open, access error, try running as superuser\n\n"));
                        }
                    }
                }
            }
        break;

        case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
            mybug_unit(-1, (" - Device detached\n"));

            if(unit->allocated) {

                unit->roothub.portstatus.wPortStatus &= ~UPSF_PORT_CONNECTION;
                unit->roothub.portstatus.wPortChange |= UPSF_PORT_CONNECTION;

                uhwCheckRootHubChanges(unit);
            }

            if(dev_handle != NULL) {
                LIBUSBCALL(libusb_close, dev_handle);
                dev_handle = NULL;
            }

        break;

        default:
            mybug_unit(-1, (" - Unknown event arrived\n"));
        break;

    }

  return 0;
}

void *hostlib_load_so(const char *sofile, const char **names, int nfuncs, void **funcptr) {
    void *handle;
    char *err;
    int i;

    if ((handle = HostLib_Open(sofile, &err)) == NULL) {
        (bug("[LIBUSB] failed to open '%s': %s\n", sofile, err));
        return NULL;
    }else{
        bug("[LIBUSB] opened '%s'\n", sofile);
    }

    for (i = 0; i < nfuncs; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        if (err != NULL) {
            bug("[LIBUSB] failed to get symbol '%s' (%s)\n", names[i], err);
            HostLib_Close(handle, NULL);
            return NULL;
        }else{
            bug("[LIBUSB] managed to get symbol '%s'\n", names[i]);
        }
    }

    return handle;
}

BOOL libusb_bridge_init(struct VUSBHCIBase *VUSBHCIBase) {

    int rc;

    HostLibBase = OpenResource("hostlib.resource");

    if (!HostLibBase)
        return FALSE;

    libusbhandle = hostlib_load_so("libusb.so", libusb_func_names, LIBUSB_NUM_FUNCS, (void **)&libusb_func);

    if (!libusbhandle)
        return FALSE;

    if(!LIBUSBCALL(libusb_init, NULL)) {
        LIBUSBCALL(libusb_set_debug, NULL, 1);
        bug("[LIBUSB] Checking hotplug support of libusb\n");
        if (LIBUSBCALL(libusb_has_capability, LIBUSB_CAP_HAS_HOTPLUG)) {
            bug("[LIBUSB]  - Hotplug supported\n");

            rc = (LIBUSBCALL(libusb_hotplug_register_callback,
                            NULL,
                            (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED|LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                            0,
                            LIBUSB_HOTPLUG_MATCH_ANY,
                            LIBUSB_HOTPLUG_MATCH_ANY,
                            LIBUSB_HOTPLUG_MATCH_ANY,
                            hotplug_callback_event_handler,
                            (void *)VUSBHCIBase,
                            NULL)
            );

            if(rc == LIBUSB_SUCCESS) {
                bug("[LIBUSB]  - Hotplug callback installed rc = %d\n", rc);
                return TRUE;
            }

            bug("[LIBUSB]  - Hotplug callback installation failure! rc = %d\n", rc);

        } else {
            bug("[LIBUSB]  - Hotplug not supported, failing...\n");
            LIBUSBCALL(libusb_exit, NULL);
        }
        libusb_bridge_cleanup();
    }

    return FALSE;
}

VOID libusb_bridge_cleanup() {
    HostLib_Close(libusbhandle, NULL);
}

void call_libusb_event_handler() {
    LIBUSBCALL(libusb_handle_events, NULL);
}

void callbackUSBTransferComplete(struct libusb_transfer *xfr) {

    struct IOUsbHWReq *ioreq = (struct IOUsbHWReq *) xfr->user_data;;
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    mybug_unit(0, ("callbackUSBTransferComplete\n"));

    int err, xfr_type;

    switch(xfr->status) {
        case LIBUSB_TRANSFER_COMPLETED:
            mybug_unit(0, ("LIBUSB_TRANSFER_COMPLETED\n"));
            ioreq->iouh_Actual = xfr->actual_length;
            err = UHIOERR_NO_ERROR;
        break;

        case LIBUSB_TRANSFER_NO_DEVICE:
            mybug_unit(-1, ("LIBUSB_TRANSFER_NO_DEVICE\n"));
            err = UHIOERR_USBOFFLINE;
        break;

        case LIBUSB_TRANSFER_TIMED_OUT:
            mybug_unit(-1, ("LIBUSB_TRANSFER_TIMED_OUT\n"));
            err = UHIOERR_TIMEOUT;
        break;

        case LIBUSB_TRANSFER_STALL:
            mybug_unit(-1, ("LIBUSB_TRANSFER_STALL\n"));
            err = UHIOERR_STALL;
        break;

        case LIBUSB_TRANSFER_OVERFLOW:
            mybug_unit(-1, ("LIBUSB_TRANSFER_OVERFLOW\n"));
            err = UHIOERR_OVERFLOW;
        break;

        default:
            mybug_unit(-1, ("LIBUSB_TRANSFER_CANCELLED/LIBUSB_TRANSFER_ERROR/LIBUSB_OTHER_ERROR\n"));
            err = UHIOERR_HOSTERROR;
        break;

    }

    xfr_type = xfr->type;

    mybug_unit(0, ("Releasing libusb transfer structure...\n"));
    LIBUSBCALL(libusb_free_transfer, xfr);

    /* Set error codes */
    ioreq->iouh_Req.io_Error = err & 0xff;

    /* Terminate the iorequest */
    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;
    ReplyMsg(&ioreq->iouh_Req.io_Message);

    switch(xfr_type) {
    	case LIBUSB_TRANSFER_TYPE_CONTROL:
    		unit->ctrlxfer_pending = FALSE;
    	break;
    	case LIBUSB_TRANSFER_TYPE_INTERRUPT:
    		unit->intrxfer_pending = FALSE;
    	break;
    	case LIBUSB_TRANSFER_TYPE_BULK:
    		unit->bulkxfer_pending = FALSE;
    	break;
    	case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
    		unit->isocxfer_pending = FALSE;
    	break;
    }

}

/*
    Control transfers are syncronous
*/
int do_libusb_ctrl_transfer(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    int rc, err;

    UWORD bmRequestType      = (ioreq->iouh_SetupData.bmRequestType);
    UWORD bmRequestType_raw  = bmRequestType&~URTF_IN;

    UWORD bRequest           = (ioreq->iouh_SetupData.bRequest);
    UWORD wValue             = (ioreq->iouh_SetupData.wValue);
    UWORD wIndex             = (ioreq->iouh_SetupData.wIndex);
    UWORD wLength            = (ioreq->iouh_SetupData.wLength);

	mybug_unit(-1, ("bmRequestType   ( "));

	if(bmRequestType&URTF_IN) {
		mybug(-1, ("URTF_IN "));
	} else {
		mybug(-1, ("URTF_OUT "));
	}
	if(bmRequestType_raw == URTF_STANDARD) mybug(-1, ("URTF_STANDARD "));
	//if(bmRequestType_raw == URTF_DEVICE) mybug(-1, ("URTF_DEVICE "));
	mybug(-1, (")\n"));

	if( (bmRequestType_raw == URTF_STANDARD) ) {
		mybug_unit(-1, ("bRequest        "));
		if(bRequest == USR_GET_STATUS) mybug(-1, ("USR_GET_STATUS\n"));
		if(bRequest == USR_CLEAR_FEATURE) mybug(-1, ("USR_CLEAR_FEATURE\n"));
		if(bRequest == USR_SET_FEATURE) mybug(-1, ("USR_SET_FEATURE\n"));
		if(bRequest == USR_SET_ADDRESS) mybug(-1, ("USR_SET_ADDRESS\n"));
		if(bRequest == USR_GET_DESCRIPTOR) mybug(-1, ("USR_GET_DESCRIPTOR\n"));
		if(bRequest == USR_SET_DESCRIPTOR) mybug(-1, ("USR_SET_DESCRIPTOR\n"));
		if(bRequest == USR_GET_CONFIGURATION) mybug(-1, ("USR_GET_CONFIGURATION\n"));
		if(bRequest == USR_SET_CONFIGURATION) mybug(-1, ("USR_SET_CONFIGURATION\n"));
		if(bRequest == USR_GET_INTERFACE) mybug(-1, ("USR_GET_INTERFACE\n"));
		if(bRequest == USR_SET_INTERFACE) mybug(-1, ("USR_SET_INTERFACE\n"));
		if(bRequest == USR_SYNCH_FRAME) mybug(-1, ("USR_SYNCH_FRAME\n"));
	}

    mybug_unit(-1, ("ioreq->iouh_SetupData.bmRequestType %x\n", ioreq->iouh_SetupData.bmRequestType));
    mybug_unit(-1, ("ioreq->iouh_SetupData.bRequest      %x\n", ioreq->iouh_SetupData.bRequest));
    mybug_unit(-1, ("ioreq->iouh_SetupData.wValue        %x\n", ioreq->iouh_SetupData.wValue));
    mybug_unit(-1, ("ioreq->iouh_SetupData.wIndex        %x\n", ioreq->iouh_SetupData.wIndex));
    mybug_unit(-1, ("ioreq->iouh_SetupData.wLength       %x\n", ioreq->iouh_SetupData.wLength));

    switch(ioreq->iouh_SetupData.bmRequestType) {

        case (URTF_OUT|URTF_STANDARD|URTF_DEVICE):
            mybug_unit(-1, ("    Maybe filtering out\n"));

            switch(ioreq->iouh_SetupData.bRequest) {

                case (USR_SET_ADDRESS):
                    mybug_unit(-1, (" - SET_ADDRESS\n"));
                    mybug_unit(-1, ("Filtering out SET_ADDRESS\n\n"));
                    ioreq->iouh_Actual = ioreq->iouh_Length;

    				/* Set error codes */
    				ioreq->iouh_Req.io_Error = UHIOERR_NO_ERROR & 0xff;

    				/* Terminate the iorequest */
    				ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;
    				ReplyMsg(&ioreq->iouh_Req.io_Message);

					unit->ctrlxfer_pending = FALSE;

                    return UHIOERR_NO_ERROR;
                break;
            }
            mybug_unit(-1, ("    NOT filtered\n"));
        break;
    }

    mybug_unit(-1, ("ioreq->iouh_Length %d\n", ioreq->iouh_Length));

    rc = LIBUSBCALL(libusb_control_transfer, dev_handle, bmRequestType, bRequest, wValue, wIndex, ioreq->iouh_Data, ioreq->iouh_Length, ioreq->iouh_NakTimeout);

    if(rc<0) {
    	switch(rc) {
        	case LIBUSB_ERROR_TIMEOUT:
            	mybug_unit(-1, ("LIBUSB_TRANSFER_TIMED_OUT\n"));
            	err = UHIOERR_TIMEOUT;
        	break;

        	case LIBUSB_ERROR_PIPE:
            	mybug_unit(-1, ("LIBUSB_ERROR_PIPE\n"));
            	err = UHIOERR_STALL;
        	break;

        	case LIBUSB_ERROR_NO_DEVICE:
            	mybug_unit(-1, ("LIBUSB_TRANSFER_NO_DEVICE\n"));
            	err = UHIOERR_USBOFFLINE;
        	break;

        	case LIBUSB_ERROR_BUSY:
            	mybug_unit(-1, ("LIBUSB_ERROR_BUSY\n"));
            	err = UHIOERR_STALL;
        	break;

        	case LIBUSB_ERROR_INVALID_PARAM:
            	mybug_unit(-1, ("LIBUSB_ERROR_INVALID_PARAM\n"));
            	err = UHIOERR_STALL;
        	break;
    	}
	} else {
		mybug_unit(-1, ("LIBUSB_TRANSFER_COMPLETED\n"));
		ioreq->iouh_Actual = rc;
        err = UHIOERR_NO_ERROR;
	}

    /* Set error codes */
    ioreq->iouh_Req.io_Error = err & 0xff;

    /* Terminate the iorequest */
    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;
    ReplyMsg(&ioreq->iouh_Req.io_Message);

	unit->ctrlxfer_pending = FALSE;

    mybug_unit(-1, ("Done!\n\n"));
    return UHIOERR_NO_ERROR;   
}


int do_libusb_intr_transfer(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    int rc = 0, transferred = 0;
    UBYTE endpoint = ioreq->iouh_Endpoint;

    mybug_unit(0, ("ioreq->iouh_Length %d\n", ioreq->iouh_Length));
    mybug_unit(0, ("direction %d\n", (ioreq->iouh_Dir)));

    struct libusb_transfer *xfr;

    mybug_unit(0, ("Allocating transfer...\n"));

    xfr = LIBUSBCALL(libusb_alloc_transfer, 0);
	if (xfr == NULL) {
	    mybug_unit(-1, ("   Failed.\n"));
	}

    UWORD timeout;

    if( (ioreq->iouh_Flags & UHFF_NAKTIMEOUT)) {
        timeout = ioreq->iouh_NakTimeout;
    } else {
        timeout = 0;
    }

    if( (ioreq->iouh_Flags & UHFF_NOSHORTPKT)) {
        xfr->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;
    }

    //xfr->flags |= LIBUSB_TRANSFER_ADD_ZERO_PACKET;

    switch(ioreq->iouh_Dir) {
        case UHDIR_IN:
            mybug_unit(0, ("ioreq->iouh_Endpoint %d (IN)\n", endpoint));
                libusb_fill_interrupt_transfer(xfr, dev_handle, (endpoint|LIBUSB_ENDPOINT_IN),
                          (UBYTE *)ioreq->iouh_Data,
                          ioreq->iouh_Length,
                          callbackUSBTransferComplete,
                          ioreq,
                          timeout
                          );
            LIBUSBCALL(libusb_submit_transfer, xfr);
            mybug_unit(0, ("Called libusb_submit_transfer\n"));
        break;

        case UHDIR_OUT:
            mybug_unit(0, ("ioreq->iouh_Endpoint %d (OUT)\n", endpoint));
                libusb_fill_interrupt_transfer(xfr, dev_handle, (endpoint|LIBUSB_ENDPOINT_OUT),
                          (UBYTE *)ioreq->iouh_Data,
                          ioreq->iouh_Length,
                          callbackUSBTransferComplete,
                          ioreq,
                          timeout
                          );

            LIBUSBCALL(libusb_submit_transfer, xfr);
            mybug_unit(0, ("Called libusb_submit_transfer\n"));
        break;
    }

    return UHIOERR_NO_ERROR;  
}

int do_libusb_bulk_transfer(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    int rc = 0, transferred = 0;
    UBYTE endpoint = ioreq->iouh_Endpoint;

    mybug_unit(0, ("ioreq->iouh_Length %d\n", ioreq->iouh_Length));
    mybug_unit(0, ("direction %d\n", (ioreq->iouh_Dir)));

    struct libusb_transfer *xfr;

    mybug_unit(0, ("Allocating transfer...\n"));

    xfr = LIBUSBCALL(libusb_alloc_transfer, 0);
	if (xfr == NULL) {
	    mybug_unit(-1, ("   Failed.\n"));
	}

    UWORD timeout;

    if( (ioreq->iouh_Flags & UHFF_NAKTIMEOUT)) {
        timeout = ioreq->iouh_NakTimeout;
    } else {
        timeout = 0;
    }

    if( (ioreq->iouh_Flags & UHFF_NOSHORTPKT)) {
        xfr->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;
    }

    //xfr->flags |= LIBUSB_TRANSFER_ADD_ZERO_PACKET;

    switch(ioreq->iouh_Dir) {
        case UHDIR_IN:
            mybug_unit(0, ("ioreq->iouh_Endpoint %d (IN)\n", endpoint));
                libusb_fill_bulk_transfer(xfr, dev_handle, (endpoint|LIBUSB_ENDPOINT_IN),
                          (UBYTE *)ioreq->iouh_Data,
                          ioreq->iouh_Length,
                          callbackUSBTransferComplete,
                          ioreq,
                          timeout
                          );

            LIBUSBCALL(libusb_submit_transfer, xfr);
            mybug_unit(0, ("Called libusb_submit_transfer\n"));
        break;

        case UHDIR_OUT:
            mybug_unit(0, ("ioreq->iouh_Endpoint %d (OUT)\n", endpoint));
                libusb_fill_bulk_transfer(xfr, dev_handle, (endpoint|LIBUSB_ENDPOINT_OUT),
                          (UBYTE *)ioreq->iouh_Data,
                          ioreq->iouh_Length,
                          callbackUSBTransferComplete,
                          ioreq,
                          timeout
                          );

            LIBUSBCALL(libusb_submit_transfer, xfr);
            mybug_unit(0, ("Called libusb_submit_transfer\n"));
        break;
    }

    return UHIOERR_NO_ERROR;
}

int do_libusb_isoc_transfer(struct IOUsbHWReq *ioreq) {
    struct VUSBHCIUnit *unit = (struct VUSBHCIUnit *) ioreq->iouh_Req.io_Unit;

    /*

    int rc = 0, transferred = 0;
    UBYTE endpoint = ioreq->iouh_Endpoint;

    mybug_unit(-1, ("ioreq->iouh_Length %d\n", ioreq->iouh_Length));
    mybug_unit(-1, ("direction %d\n", (ioreq->iouh_Dir)));

    struct libusb_transfer *xfr;

    mybug_unit(-1, ("Allocating transfer...\n"));

    xfr = LIBUSBCALL(libusb_alloc_transfer, 1);
	if (xfr == NULL) {
	    mybug_unit(-1, ("   Failed.\n"));
	}

    switch(ioreq->iouh_Dir) {
        case UHDIR_IN:
            mybug_unit(-1, ("ioreq->iouh_Endpoint %d (IN)\n", endpoint));
            libusb_fill_iso_transfer(xfr, dev_handle, (endpoint|LIBUSB_ENDPOINT_IN),
                          (UBYTE *)ioreq->iouh_Data,
                          ioreq->iouh_Length,
                          1,
                          callbackUSBTransferComplete,
                          ioreq,
                          ioreq->iouh_NakTimeout
                          );

            LIBUSBCALL(libusb_submit_transfer, xfr);
            mybug_unit(-1, ("Called libusb_submit_transfer\n"));
            call_libusb_event_handler();
        break;

        case UHDIR_OUT:
            mybug_unit(-1, ("ioreq->iouh_Endpoint %d (OUT)\n", endpoint));
            libusb_fill_iso_transfer(xfr, dev_handle, (endpoint|LIBUSB_ENDPOINT_OUT),
                          (UBYTE *)ioreq->iouh_Data,
                          ioreq->iouh_Length,
                          1,
                          callbackUSBTransferComplete,
                          ioreq,
                          ioreq->iouh_NakTimeout
                          );

            LIBUSBCALL(libusb_submit_transfer, xfr);
            mybug_unit(-1, ("Called libusb_submit_transfer\n"));
            call_libusb_event_handler();
        break;
    }
    */
    return UHIOERR_NO_ERROR;    
}

