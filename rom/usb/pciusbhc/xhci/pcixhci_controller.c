/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI XHCI USB host controller
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <aros/io.h>
#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/stdc.h>
#include <proto/arossupport.h>

#include <devices/usb.h>
#include <devices/usb_hub.h>
#include <devices/newstyle.h>
#include <devices/usbhardware.h>
#include <devices/timer.h>

#include <asm/io.h>
#include <inttypes.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include "pcixhci_intern.h"

#include "pcixhci_controller.h"

#include LC_LIBDEFS_FILE

#define PCIXHCIBase unit->pcixhcibase

static AROS_INTH1(PCIXHCI_IntCode, struct PCIXHCIUnit *, unit) {
    AROS_INTFUNC_INIT

    ULONG usbsts, portsc;

    usbsts = operational_readl(XHCI_USBSTS);

    if(usbsts & XHCF_STS_PCD) {
        operational_writel(XHCI_USBSTS, XHCF_STS_PCD);
        mybug_unit(-1, ("cleared usbsts = %08x\n", operational_readl(XHCI_USBSTS)));
        struct PCIXHCIPort *port = NULL;

        ForeachNode(&unit->roothub.port_list, port) {
            portsc = operational_readl(XHCI_PORTSC(port->number));

            if(portsc & XHCF_PS_CSC) {
                operational_writel(XHCI_PORTSC(port->number), (portsc | ~XHCF_PS_CSC));
                mybug_unit(-1,("port %d XHCF_PS_CSC\n", port->number));
            }

            if(portsc & XHCF_PS_PEC) {
                operational_writel(XHCI_PORTSC(port->number), (portsc | ~XHCF_PS_PEC));
                mybug_unit(-1,("port %d XHCF_PS_PEC\n", port->number));
            }

            if(portsc & XHCF_PS_OCC) {
                operational_writel(XHCI_PORTSC(port->number), (portsc | ~XHCF_PS_OCC));
                mybug_unit(-1,("port %d XHCF_PS_OCC\n", port->number));
            }

            if(portsc & XHCF_PS_WRC) {
                operational_writel(XHCI_PORTSC(port->number), (portsc | ~XHCF_PS_WRC));
                mybug_unit(-1,("port %d XHCF_PS_WRC\n", port->number));
            }

            if(portsc & XHCF_PS_PRC) {
                operational_writel(XHCI_PORTSC(port->number), (portsc | ~XHCF_PS_PRC));
                mybug_unit(-1,("port %d XHCF_PS_PRC\n", port->number));
            }

            if(portsc & XHCF_PS_PLC) {
                operational_writel(XHCI_PORTSC(port->number), (portsc | ~XHCF_PS_PLC));
                mybug_unit(-1,("port %d XHCF_PS_PLC\n", port->number));
            }

            if(portsc & XHCF_PS_CEC) {
                operational_writel(XHCI_PORTSC(port->number), (portsc | ~XHCF_PS_CEC));
                mybug_unit(-1,("port %d XHCF_PS_CEC\n", port->number));
            }
        }
    }
    return 0;
    AROS_INTFUNC_EXIT
}

/*
    We get called only once (per controller) when the driver inits
    We own the controller until our driver expunges so we assume that nobody messes with our stuff...
    Driver NEVER expunges, it messes Poseidon.
*/
BOOL PCIXHCI_HCInit(struct PCIXHCIUnit *unit) {
    //mybug(0, ("[PCIXHCI] PCIXHCI_HCInit: Entering function\n"));

    /* Our unit is in suspended state until it is reset */
    unit->state = UHSF_SUSPENDED;

    /* Init the port list but do not fill it */
    NEWLIST(&unit->roothub.port_list);

    snprintf(unit->name, 255, "PCIXHCI[%02x:%02x.%01x]", (UBYTE)unit->hc.bus, (UBYTE)unit->hc.dev, (UBYTE)unit->hc.sub);
    unit->node.ln_Name = (STRPTR)&unit->name;

    if(!PCIXHCI_CreateTimer(unit)) {
        return FALSE;
    }

    /* capability base(=base0) has been stored in the enumerator */

    /* Store operational base */
    unit->hc.operational_base = (APTR) ((IPTR) (unit->hc.capability_base) + capability_readb(XHCI_CAPLENGTH));

    /* Store doorbell base */
    unit->hc.doorbell_base = (APTR) ((IPTR) (unit->hc.capability_base) + XHCV_DBOFF(capability_readl(XHCI_DBOFF)));

    /* Store runtime base */
    unit->hc.runtime_base = (APTR) ((IPTR) (unit->hc.capability_base) + XHCV_RTSOFF(capability_readl(XHCI_RTSOFF)));

    mybug_unit(-1, ("unit node name %s\n", unit->node.ln_Name));
    mybug_unit(-1, ("pcidevice    = %p\n", unit->hc.pcidevice));
    mybug_unit(-1, ("pcidriver    = %p\n", unit->hc.pcidriver));
    mybug_unit(-1, ("bus          = %x\n", unit->hc.bus));
    mybug_unit(-1, ("dev          = %x\n", unit->hc.dev));
    mybug_unit(-1, ("sub          = %x\n", unit->hc.sub));
    mybug_unit(-1, ("intline      = %d\n", unit->hc.intline));

    mybug_unit(-1, ("capability  = %p\n", unit->hc.capability_base));
    mybug_unit(-1, ("operational = %p\n", unit->hc.operational_base));
    mybug_unit(-1, ("doorbell    = %p\n", unit->hc.doorbell_base));
    mybug_unit(-1, ("runtime     = %p\n", unit->hc.runtime_base));

/*
    PCIXHCI[03:00.0] PCIXHCI_HCInit: capability  = f3dfe000
    PCIXHCI[03:00.0] PCIXHCI_HCInit: operational = f3dfe020
    PCIXHCI[03:00.0] PCIXHCI_HCInit: doorbell    = f3dfe800 up to 256 32 bit doorbell registers (4*256 = 0x400)
    PCIXHCI[03:00.0] PCIXHCI_HCInit: runtime     = f3dfe600

    PCIXHCI[04:00.0] PCIXHCI_HCInit: capability  = f3efe000
    PCIXHCI[04:00.0] PCIXHCI_HCInit: operational = f3efe020
    PCIXHCI[04:00.0] PCIXHCI_HCInit: doorbell    = f3efe800 up to 256 32 bit doorbell registers (4*256 = 0x400)
    PCIXHCI[04:00.0] PCIXHCI_HCInit: runtime     = f3efe600
*/

    /* We use pointers and stuff... */
    struct TagItem pciActivateMemAndBusmaster[] = {
            { aHidd_PCIDevice_isIO,     FALSE },
            { aHidd_PCIDevice_isMEM,    TRUE },
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    OOP_SetAttrs(unit->hc.pcidevice, (struct TagItem *)pciActivateMemAndBusmaster);

    /* Get the host controller from BIOS if possible */
    IPTR cap_legacy;
    ULONG usblegsup, timeout;

    cap_legacy = PCIXHCI_SearchExtendedCap(unit, XHCI_EXT_CAPS_LEGACY, (IPTR) NULL);
    if(cap_legacy) {
        usblegsup = READREG32(cap_legacy, XHCI_USBLEGSUP);
        mybug_unit(-1, ("usblegsup1 = %08x\n", usblegsup));

        /* Check if not OS owned */
        if( ((!(usblegsup & XHCF_OSOWNED)) || (usblegsup & XHCF_BIOSOWNED)) ){
            WRITEMEM32(cap_legacy, (usblegsup|XHCF_OSOWNED));

            usblegsup = READREG32(cap_legacy, XHCI_USBLEGSUP);
            mybug_unit(-1, ("usblegsup2 = %08x\n", usblegsup));

            /* Spec says "no more than a second", we give it a little more */
            timeout = 250;

            while(1) {
                usblegsup = READREG32(cap_legacy, XHCI_USBLEGSUP);
                mybug_unit(-1, ("usblegsup3 = %08x\n", usblegsup));
                if( (usblegsup & XHCF_OSOWNED) && (!(usblegsup & XHCF_BIOSOWNED)) ){
                    break;
                }

                /* Wait 10ms and check again */
                PCIXHCI_Delay(unit, 10);

                if(--timeout) {
                    mybug_unit(-1, ("BIOS didn't release XHCI. Forcing and praying...\n"));
                    WRITEMEM32(cap_legacy, ((usblegsup|XHCF_OSOWNED)&~XHCF_BIOSOWNED));
                    break;
                }
            }
        } else {
            mybug_unit(-1, ("Controller is already owned by the OS\n"));
        }
    }

    mybug(-1,("XHCV_MaxIntrs = %d\n",XHCV_MaxIntrs(capability_readl(XHCI_HCSPARAMS1))));

    /* Add interrupt handler */
    snprintf(unit->hc.intname, 255, "%s interrupt handler", unit->node.ln_Name);
    unit->hc.inthandler.is_Node.ln_Name = (STRPTR)&unit->hc.intname;
    unit->hc.inthandler.is_Node.ln_Pri = 15;
    unit->hc.inthandler.is_Node.ln_Type = NT_INTERRUPT;
    unit->hc.inthandler.is_Code = (VOID_FUNC)PCIXHCI_IntCode;
    unit->hc.inthandler.is_Data = unit;
    if(!HIDD_PCIDevice_AddInterrupt(unit->hc.pcidevice, &unit->hc.inthandler)) {
        mybug_unit(-1, ("Failed setting up interrupt handler!\n"));
        return FALSE;
    }

    return TRUE;
}

/*
    We get called everytime the driver is trying to get online
    We make use of it by also filling the port list of our unit,
    that way toggling unit offline and online may give different results for ports after reset
*/
BOOL PCIXHCI_HCReset(struct PCIXHCIUnit *unit) {
    mybug_unit(0, ("Entering function\n"));

    ULONG timeout;

    if(!PCIXHCI_HCHalt(unit)) {
        return FALSE;
    }

    /* our unit is in reset state until the higher level usb reset is called */
    unit->state = UHSF_RESET;

    /* Reset controller by setting HCRST-bit */
    operational_writel(XHCI_USBCMD, (operational_readl(XHCI_USBCMD) | XHCF_CMD_HCRST));

    /*
        Controller clears HCRST bit when reset is done, wait for it and the CNR-bit to be cleared
    */
    timeout = 250;  //FIXME: arbitrary value of 2500ms
    while ((operational_readl(XHCI_USBCMD) & XHCF_CMD_HCRST) && (timeout-- != 0)) {
        if(!timeout) {
            mybug_unit(-1, ("Time is up for XHCF_CMD_HCRST bit!\n"));
            return FALSE;
        }
        /* Wait 10ms and check again */
        PCIXHCI_Delay(unit, 10);
    }

    timeout = 250;  //FIXME: arbitrary value of 2500ms
    while ((operational_readl(XHCI_USBSTS) & XHCF_STS_CNR) && (timeout-- != 0)) {
        if(!timeout) {
            mybug_unit(-1, ("Time is up for XHCF_STS_CNR bit!\n"));
            return FALSE;
        }
        /* Wait 10ms and check again */
        PCIXHCI_Delay(unit, 10);
    }

    if(!PCIXHCI_FindPorts(unit)) {
        return FALSE;
    }

    struct PCIXHCIPort *port = NULL;

    mybug_unit(-1, ("Unit %d at %p %s\n", unit->number, unit, unit->name));
    ForeachNode(&unit->roothub.port_list, port) {
        mybug_unit(-1, ("     port %d at %p %s\n", port->number, port, port->name));
    }
    mybug(-1,("\n"));

    /* Enable host controller to issue interrupts */
    mybug_unit(-1, ("usbcmd = %08x\n", operational_readl(XHCI_USBCMD)));
    mybug_unit(-1, ("usbsts = %08x\n", operational_readl(XHCI_USBSTS)));
    operational_writel(XHCI_USBCMD, (operational_readl(XHCI_USBCMD) | (XHCF_CMD_RS | XHCF_CMD_INTE | XHCF_CMD_HSEE) ));
    mybug_unit(-1, ("usbcmd = %08x\n", operational_readl(XHCI_USBCMD)));
    mybug_unit(-1, ("usbsts = %08x\n", operational_readl(XHCI_USBSTS)));
    return TRUE;
}

BOOL PCIXHCI_HCHalt(struct PCIXHCIUnit *unit) {
    mybug_unit(-1, ("Entering function\n"));

    ULONG timeout, temp;

    /* Halt the controller by clearing Run/Stop bit */
    temp = operational_readl(XHCI_USBCMD);
    operational_writel(XHCI_USBCMD, (temp & ~XHCF_CMD_RS));

    /* Our unit advertises that it is in suspended state */
    unit->state = UHSF_SUSPENDED;

    /*
        The xHC shall halt within 16 ms. after software clears the Run/Stop bit if certain conditions have been met.
        The HCHalted (HCH) bit in the USBSTS register indicates when the xHC has finished its
        pending pipelined transactions and has entered the stopped state. 
    */
    timeout = 250;  //FIXME: arbitrary value of 2500ms
    do {
        temp = operational_readl(XHCI_USBSTS);
        if( (temp & XHCF_STS_HCH) ) {
            mybug_unit(-1, ("controller halted!\n"));
            return TRUE;
        }
        /* Wait 10ms and check again */
        PCIXHCI_Delay(unit, 10);
    } while(--timeout);

    mybug_unit(-1, ("halt failed!\n"));
    return FALSE;
}

IPTR PCIXHCI_SearchExtendedCap(struct PCIXHCIUnit *unit, ULONG id, IPTR extcapoff) {

    IPTR extcap = (IPTR) NULL;

    mybug_unit(-1,("searching for extended capability id(%ld)\n", id));

    if(extcapoff) {
        /* Last known good */
        if(XHCV_EXT_CAPS_NEXT(READMEM32(extcapoff))) {
            extcap = extcapoff + XHCV_EXT_CAPS_NEXT(READMEM32(extcapoff));
        } else {
            extcap = (IPTR) NULL;
        }
    } else {
        extcap = XHCV_xECP(capability_readl(XHCI_HCCPARAMS1)) + (IPTR) unit->hc.capability_base;
        mybug_unit(-1, ("searching from beginning %p\n", extcap));
    }

    /* Either the first (if exist) or the next from last known (if exist) else (IPTR) NULL */
    while(extcap != (IPTR) NULL) {
        if((XHCV_EXT_CAPS_ID(READMEM32(extcap)) == id)) {
            mybug_unit(-1, ("found matching extended capability id at %lx\n", extcap));
            break;
        } else {
            if(XHCV_EXT_CAPS_NEXT(READMEM32(extcap))) {
                mybug_unit(-1, ("skipping extended capability id at %lx\n", extcap));
                extcap += XHCV_EXT_CAPS_NEXT(READMEM32(extcap));
            } else {
                extcap = (IPTR) NULL;
                break;
            }
        }
    }

    return (IPTR) extcap;
}

BOOL PCIXHCI_FindPorts(struct PCIXHCIUnit *unit) {

    struct PCIXHCIPort *port = NULL;

    IPTR cap_protocol = (IPTR) NULL;

    ULONG portnum = 0, portcount = 0, temp, major, minor, po, pc;

    /* (Re)build the port list of our unit */
    ForeachNode(&unit->roothub.port_list, port) {
        mybug_unit(-1, ("Deleting port %d named %s at %p\n", port->number, port->name, port));
        REMOVE(port);
        FreeVec(port);
    }

    portcount = XHCV_MaxPorts(capability_readl(XHCI_HCSPARAMS1));
    mybug_unit(-1, ("Controller advertises port count to be %d\n", portcount));

    do {
        port = AllocVec(sizeof(struct PCIXHCIPort), MEMF_ANY|MEMF_CLEAR);
        if(port == NULL) {
            mybug_unit(-1, ("Failed to create new port structure\n"));
            ForeachNode(&unit->roothub.port_list, port) {
                mybug_unit(-1, ("Deleting port %d named %s at %p\n", port->number, port->name, port));
                REMOVE(port);
                FreeVec(port);
            }
            return FALSE;
        } else {
            port->node.ln_Type = NT_USER;
            port->number = ++portnum;
            AddTail(&unit->roothub.port_list, (struct Node *)port);

            mybug_unit(-1, ("Created new port %d at %p\n", port->number, port));
        }
    } while(--portcount);


    /*
        We may get more than one capability protocol header or just one (Which is case OnMyHardware(TM))
    */
    while((cap_protocol = PCIXHCI_SearchExtendedCap(unit, XHCI_EXT_CAPS_PROTOCOL, cap_protocol))) {
        temp = READREG32(cap_protocol, XHCI_SPFD);
        major = XHCV_SPFD_RMAJOR(temp);
        minor = XHCV_SPFD_RMINOR(temp);

        temp = READREG32(cap_protocol, XHCI_SPPORT);
        po = XHCV_SPPORT_CPO(temp);
        pc = XHCV_SPPORT_CPCNT(temp);

        mybug_unit(-1, ("Version %ld.%ld port offset %d port count %d\n", major, minor, po, pc));

        /* Iterate through port list and place the name for one that fits inside the offset and count */
        ForeachNode(&unit->roothub.port_list, port) {
            if( (port->number>=po) && (port->number<(po+pc)) ){
                snprintf(port->name, 255, "%s USB %d.%d port %d", unit->node.ln_Name, major, minor, port->number);
                port->node.ln_Name = (STRPTR)&port->name;
            }
        }
    }

    /* Check if any port is left unnamed */
    ForeachNode(&unit->roothub.port_list, port) {
        if(port->node.ln_Name == NULL){
            snprintf(port->name, 255, "%s USB 2.0 port %d (guessed)", unit->node.ln_Name, port->number);
            port->node.ln_Name = (STRPTR)&port->name;
        }
    }

    return TRUE;
}

BOOL PCIXHCI_PortPower(struct PCIXHCIUnit *unit, ULONG portnum, BOOL poweron) {
    /*
        Check for port power control
    */
    if(capability_readl(XHCI_HCCPARAMS1) & XHCF_PPC) {
        mybug_unit(-1, ("Port has power switch\n"));
    } else {
        mybug_unit(-1, ("Port does not have power switch\n"));
    }

    /* We have power on by default, skip this for now */
/*
    ULONG portsc;

    portsc = operational_readl(XHCI_PORTSC(portnum));

    mybug_unit(-1, ("portsc = %08x\n", portsc));
    portsc = portsc & 0x7F00FF08;
    mybug_unit(-1, ("portsc = %08x\n", portsc));

    if(poweron) {
        portsc = (portsc | XHCF_PS_PP);
        mybug_unit(-1, ("Port powering up\n"));
    } else {
        mybug_unit(-1, ("Port powering down\n"));
        portsc = (portsc & ~XHCF_PS_PP);
    }

    operational_writel(XHCI_PORTSC(portnum), portsc);

    portsc = operational_readl(XHCI_PORTSC(portnum));
    mybug_unit(-1, ("portsc = %08x\n", portsc));
*/
    return TRUE;
}


