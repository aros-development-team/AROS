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
    mybug_unit(-1, ("Excuse me...\n"));

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

    /* Store opregbase */
    unit->hc.opregbase = (APTR) ((ULONG) (unit->hc.capregbase) + capreg_readb(XHCI_CAPLENGTH));

    mybug_unit(-1, ("unit node name %s\n", unit->node.ln_Name));
    mybug_unit(-1, ("pcidevice  = %p\n",   unit->hc.pcidevice));
    mybug_unit(-1, ("pcidriver  = %p\n",   unit->hc.pcidriver));
    mybug_unit(-1, ("bus        = %x\n",   unit->hc.bus));
    mybug_unit(-1, ("dev        = %x\n",   unit->hc.dev));
    mybug_unit(-1, ("sub        = %x\n",   unit->hc.sub));
    mybug_unit(-1, ("intline    = %d\n",   unit->hc.intline));
    mybug_unit(-1, ("capregbase = %p\n",   unit->hc.capregbase));
    mybug_unit(-1, ("opregbase  = %p\n",   unit->hc.opregbase));

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
    ULONG temp, timeout;

    cap_legacy = PCIXHCI_SearchExtendedCap(unit, XHCI_EXT_CAPS_LEGACY, 0);
    if(cap_legacy) {
        temp = READMEM32(cap_legacy);
        if( (temp & XHCF_BIOSOWNED) ){
            mybug_unit(-1, ("controller owned by BIOS\n"));

            /* Spec says "no more than a second", we give it a little more */
            timeout = 250;

            WRITEMEM32(cap_legacy, (temp | XHCF_OSOWNED) );
            do {
                temp = READMEM32(cap_legacy);
                if(!(temp & XHCF_BIOSOWNED)) {
                    mybug_unit(-1, ("BIOS gave up on XHCI. Pwned!\n"));
                    break;
                }
                /* Wait 10ms and check again */
                PCIXHCI_Delay(unit, 10);
            } while(--timeout);

            if(!timeout) {
                mybug_unit(-1, ("BIOS didn't release XHCI. Forcing and praying...\n"));
                WRITEMEM32(cap_legacy, (temp & ~XHCF_BIOSOWNED) );
            }

        } else {
            mybug_unit(-1, ("controller was not owned by BIOS\n"));
        }
    }

    //unit->roothub.devdesc.bcdDevice = capreg_readw(XHCI_HCIVERSION);

    /* Add interrupt handler */
    snprintf(unit->hc.intname, 255, "%s interrupt handler", unit->node.ln_Name);
    unit->hc.inthandler.is_Node.ln_Name = (STRPTR)&unit->hc.intname;
    unit->hc.inthandler.is_Node.ln_Pri = 5;
    unit->hc.inthandler.is_Node.ln_Type = NT_INTERRUPT;
    unit->hc.inthandler.is_Code = (VOID_FUNC)PCIXHCI_IntCode;
    unit->hc.inthandler.is_Data = unit;

    HIDD_PCIDevice_AddInterrupt(unit->hc.pcidevice, &unit->hc.inthandler);

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
    opreg_writel(XHCI_USBCMD, (opreg_readl(XHCI_USBCMD) | XHCF_CMD_HCRST));

    /*
        Controller clears HCRST bit when reset is done, wait for it and the CNR-bit to be cleared
    */
    timeout = 250;  //FIXME: arbitrary value of 2500ms
    while ((opreg_readl(XHCI_USBCMD) & XHCF_CMD_HCRST) && (timeout-- != 0)) {
        if(!timeout) {
            mybug_unit(-1, ("Time is up for XHCF_CMD_HCRST bit!\n"));
            return FALSE;
        }
        /* Wait 10ms and check again */
        PCIXHCI_Delay(unit, 10);
    }

    timeout = 250;  //FIXME: arbitrary value of 2500ms
    while ((opreg_readl(XHCI_USBSTS) & XHCF_STS_CNR) && (timeout-- != 0)) {
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

    return TRUE;
}

IPTR PCIXHCI_SearchExtendedCap(struct PCIXHCIUnit *unit, ULONG id, IPTR extcap) {
    IPTR extcapoff = (IPTR) NULL;

    mybug_unit(-1,("searching for extended capability id(%ld)\n", id));

    if(extcap) {
        mybug_unit(-1, ("continue search from %p\n", extcap));
        extcap = (IPTR) XHCV_EXT_CAPS_NEXT(READMEM32(extcap));
    } else {  
        extcap = (IPTR) unit->hc.capregbase + XHCV_xECP(capreg_readl(XHCI_HCCPARAMS));
        mybug_unit(-1, ("searching from beginning %p\n", extcap));
    }

    do {
        extcap += extcapoff;
        if((XHCV_EXT_CAPS_ID(READMEM32(extcap)) == id)) {
            mybug_unit(-1, ("found matching extended capability id at %lx\n", extcap));
            return (IPTR) extcap;
        }
        if(extcap)
            mybug_unit(-1, ("skipping extended capability id(%ld)\n", XHCV_EXT_CAPS_ID(READMEM32(extcap))));
        extcapoff = (IPTR) XHCV_EXT_CAPS_NEXT(READMEM32(extcap));
    } while(extcapoff);

    mybug_unit(-1, ("not found!\n"));
    return (IPTR) NULL;
}

BOOL PCIXHCI_HCHalt(struct PCIXHCIUnit *unit) {
    mybug_unit(-1, ("Entering function\n"));

    ULONG timeout, temp;

    /* Halt the controller by clearing Run/Stop bit */
    temp = opreg_readl(XHCI_USBCMD);
    opreg_writel(XHCI_USBCMD, (temp & ~XHCF_CMD_RS));

    /* Our unit advertises that it is in suspended state */
    unit->state = UHSF_SUSPENDED;

    /*
        The xHC shall halt within 16 ms. after software clears the Run/Stop bit if certain conditions have been met.
        The HCHalted (HCH) bit in the USBSTS register indicates when the xHC has finished its
        pending pipelined transactions and has entered the stopped state. 
    */
    timeout = 250;  //FIXME: arbitrary value of 2500ms
    do {
        temp = opreg_readl(XHCI_USBSTS);
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

    portcount = XHCV_MaxPorts(capreg_readl(XHCI_HCSPARAMS1));
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




