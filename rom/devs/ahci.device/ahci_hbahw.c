/*
    Copyright © 2010, The AROS Development Team. All rights reserved
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

/*
    Call every HBA function with pointer to ahci_hba_chip structure as a first argument
*/

static void ahci_hba_Interrupt(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw) {
    struct ahci_hba_chip *hba_chip = (struct ahci_hba_chip *)irq->h_Data;
    HBAHW_D("IRQ-Handler!\n");
}

BOOL ahci_create_interrupt(struct ahci_hba_chip *hba_chip) {

    OOP_Object *irqhidd;
    if ( (irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL)) ) {

        hba_chip->IntHandler->h_Node.ln_Pri = 10;
        hba_chip->IntHandler->h_Node.ln_Name = "HBA-chip irq";
        hba_chip->IntHandler->h_Code = ahci_hba_Interrupt;
        hba_chip->IntHandler->h_Data = hba_chip;

        struct OOP_Object *o;
        if ( (o = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL)) ) {
            HIDD_IRQ_AddHandler(irqhidd, hba_chip->IntHandler, hba_chip->IRQ);
            OOP_DisposeObject((OOP_Object *)o);
            OOP_DisposeObject(irqhidd);
            return TRUE;
        }else{
            OOP_DisposeObject(irqhidd);
            return FALSE;
        }
    }

    return FALSE;
}

static void ahci_taskcode_hba(struct ahci_hba_chip *hba_chip, struct Task* parent) {
    HBAHW_D("HBA task running...\n");

    HBAHW_D("Signaling parent %08x\n", parent);
    Signal(parent, SIGBREAKF_CTRL_C);

    if ( ahci_init_hba(hba_chip) ) {
        HBAHW_D("ahci_init_hba done!\n");
        Wait(0);
    } else {
        HBAHW_D("ahci_init_hba failed!\n");
        /*
            Something failed while setting up the HW part of the HBA
            Release all allocated memory and other resources for this HBA
            and remove us from the list (no ports for example... should not happen)
        */

    }
}

BOOL ahci_create_hbatask(struct ahci_hba_chip *hba_chip) {
    HBAHW_D("Setting up HBA task...\n");

    //move to hba_chip struct
    struct Task     *t;
    struct MemList  *ml;
    UBYTE *sp = NULL;

    struct TagItem tags[] = {
        { TASKTAG_ARG1, (IPTR)hba_chip },
        { TASKTAG_ARG2, (IPTR)FindTask(0) },
        { TAG_DONE,     0 }
    };

    t = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (t) {
        ml = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry), MEMF_PUBLIC | MEMF_CLEAR);
        if(ml) {
    	    sp = AllocMem(HBA_TASK_STACKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    	    if(sp) {
                t->tc_SPLower = sp;
                t->tc_SPUpper = sp + HBA_TASK_STACKSIZE;
            #if AROS_STACK_GROWS_DOWNWARDS
		        t->tc_SPReg = (UBYTE *)t->tc_SPUpper-SP_OFFSET;
            #else
		        t->tc_SPReg = (UBYTE *)t->tc_SPLower-SP_OFFSET;
            #endif

                ml->ml_NumEntries = 2;

                ml->ml_ME[0].me_Addr = t;
                ml->ml_ME[0].me_Length = sizeof(struct Task);

                ml->ml_ME[1].me_Addr = sp;
                ml->ml_ME[1].me_Length = HBA_TASK_STACKSIZE;
        
                NEWLIST(&t->tc_MemEntry);
                AddHead(&t->tc_MemEntry, &ml->ml_Node);

                t->tc_Node.ln_Name = "HBA task";
                t->tc_Node.ln_Type = NT_TASK;
                t->tc_Node.ln_Pri  = HBA_TASK_PRI;

                NewAddTask(t, ahci_taskcode_hba, NULL, tags);

                Wait(SIGBREAKF_CTRL_C);
                HBAHW_D("Signal from HBA task received\n");

                return TRUE;
            }
            FreeMem(ml,sizeof(struct MemList) + sizeof(struct MemEntry));
        }
        FreeMem(t,sizeof(struct Task));
    }

    return FALSE;
}

BOOL ahci_setup_hba(struct ahci_hba_chip *hba_chip) {
    HBAHW_D("HBA-setup...\n");

    if( ahci_init_hba(hba_chip) )
        return TRUE;

    return FALSE;
}

/*
    Set the AHCI HBA to a minimally initialized state.

        * Indicate that the system is AHCI aware by setting bit GHC_AE in HBA's GHC register
        * Determine number of ports implemented by the HBA
        * Ensure that all implemented ports are in IDLE state
        * Determine how many command slots the HBA supports, by reading CAP.NCS
        * Set CAP.S64A to ‘0’, if not compiled for x86_64...
        * For each implemented port, allocate memory for and program registers:
            - PxCLB (and PxCLBU=0, upper 32bits of 64bit address space)
            - PxFB (and PxFBU=0, upper 32bits of 64bit address space)
        * For each implemented port, clear the PxSERR register
        * Allocate and initialize interrupts, and set each implemented port’s PxIE
            register with the appropriate enables. To enable the HBA to generate interrupts, system
            software must also set GHC.IE to a ‘1’.
*/

BOOL ahci_init_hba(struct ahci_hba_chip *hba_chip) {
    HBAHW_D("HBA-init...\n");

    struct ahci_hwhba *hwhba;
    hwhba = hba_chip->abar;

    /* BIOS handoff if HBA supports it and BIOS is the current owner of HBA */
    hba_chip->Version = hwhba->vs;

    if ( (hba_chip->Version >= AHCI_VERSION_1_20) ) {
        if( (hwhba->cap2 && CAP2_BOH) ) {
            HBAHW_D("HBA supports BIOS/OS handoff\n");
            if( (hwhba->bohc && BOHC_BOS) ) {
                hwhba->bohc |= BOHC_OOS;
                /* Spin on BOHC_BOS bit FIXME: Possible dead lock. No maximum time given on AHCI1.3 specs... */
                while( (hwhba->bohc && BOHC_BOS) );
                delay_ms(hba_chip, 25);
                /* If after 25ms BOHC_BB bit is still set give bios a minimum of 2 seconds more time to run */
                if( (hwhba->bohc && BOHC_BB) ) {
                    delay_ms(hba_chip, 2500);
                }
            }
        }
    }

    /* Enable AHCI mode of communication to the HBA */
    ahci_enable_hba(hba_chip);

    /* Reset the HBA */
    if ( ahci_reset_hba(hba_chip) ) {
        HBAHW_D("Reset done\n");

    /* FIXME: Is there something better for this? */
    #if defined(__i386__)
        hwhba->cap &= ~CAP_S64A;
    #endif

        hba_chip->CommandSlotCount = 1 + ((hwhba->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
        hba_chip->PortCountMax = 1 + ((hwhba->cap >> CAP_NP_SHIFT) & CAP_NP_MASK);

	    hba_chip->PortImplementedMask = hwhba->pi;
	    if (hba_chip->PortImplementedMask == 0) {
		    hba_chip->PortImplementedMask = 0xffffffff >> (32 - hba_chip->PortCountMax);
		    HBAHW_D("ports-implemented mask is zero, using 0x%x instead.\n", hba_chip->PortImplementedMask);
	    }

	    hba_chip->PortCount = count_bits_set(hba_chip->PortImplementedMask);
/*
        HBAHW_D("Interface Speed Support: generation %u\n",    (hwhba->cap >> CAP_ISS_SHIFT) & CAP_ISS_MASK);
        HBAHW_D("Number of Command Slots: %d (raw %x)\n",      hba_chip->CommandSlotCount, (hwhba->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
        HBAHW_D("Supports Port Multiplier: %s\n",               (hwhba->cap & CAP_SPM) ? "yes" : "no");
        HBAHW_D("Supports External SATA: %s\n",                 (hwhba->cap & CAP_SXS) ? "yes" : "no");
        HBAHW_D("Enclosure Management Supported: %s\n",         (hwhba->cap & CAP_EMS) ? "yes" : "no");
        HBAHW_D("Supports Command List Override: %s\n",         (hwhba->cap & CAP_SCLO) ? "yes" : "no");
        HBAHW_D("Supports Staggered Spin-up: %s\n",             (hwhba->cap & CAP_SSS) ? "yes" : "no");
        HBAHW_D("Supports Mechanical Presence Switch: %s\n",    (hwhba->cap & CAP_SMPS) ? "yes" : "no");
        HBAHW_D("Supports 64-bit Addressing: %s\n",             (hwhba->cap & CAP_S64A) ? "yes" : "no");
        HBAHW_D("Supports Native Command Queuing: %s\n",        (hwhba->cap & CAP_SNCQ) ? "yes" : "no");
        HBAHW_D("Supports SNotification Register: %s\n",        (hwhba->cap & CAP_SSNTF) ? "yes" : "no");
        HBAHW_D("Supports Command List Override: %s\n",         (hwhba->cap & CAP_SCLO) ? "yes" : "no");
        HBAHW_D("AHCI Version %u.%u (raw %x)\n",                ((hba_chip->Version >> 24) & 0xf)*10 + ((hba_chip->Version >> 16) & 0xf),
                                                                ((hba_chip->Version >> 8) & 0xf)*10 + (hba_chip->Version & 0xf), hwhba->vs );
        HBAHW_D("Interrupt %u\n",                               hba_chip->IRQ);
*/

        /* Set timer.device up for this HBA-chip and within this task context */
        /*
            FIXME: For now ahci_init_hba() is called from ahci_setup_hba(), 
                    which in turn is called from Init(), but ahci_setup_hba() should create HBA_Task_(hba_number) that would call ahci_init_hba()
                    and HBA_Task_(hba_number) would also direct commands for the device.
        */
        hba_chip->MsgPort.mp_SigBit = SIGB_SINGLE;
        hba_chip->MsgPort.mp_Flags = PA_SIGNAL;
        hba_chip->MsgPort.mp_SigTask = FindTask(NULL);
        hba_chip->MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        NEWLIST(&hba_chip->MsgPort.mp_MsgList);

        hba_chip->tr.tr_node.io_Message.mn_ReplyPort = &hba_chip->MsgPort;
        hba_chip->tr.tr_node.io_Message.mn_Length = sizeof(hba_chip->tr);

        if ( (OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)&hba_chip->tr, 0)) ) {
            HBAHW_D("Could not open timer.device\n");
            return FALSE;
        }

        if( !ahci_create_interrupt(hba_chip) )
            return FALSE;

        HBAHW_D("port count %d\n", hba_chip->PortCount);

        /*
            Get the pointer to previous HBA-chip struct in the list (if any)
            Port numbering starts from 1 if no previous HBA exist else the port number will be
            starting number of previous HBA-chip ports + the number of implemented ports in that chip
            Port number 0 (e.g. unit number 0) is reserved/not implemented
        */
        struct ahci_hba_chip *prev_hba_chip = (struct ahci_hba_chip*) GetPred(hba_chip);

        if ( prev_hba_chip != NULL ) {
            hba_chip->StartingPortNumber = (prev_hba_chip->StartingPortNumber) + (prev_hba_chip->PortCount);
        }else{
            hba_chip->StartingPortNumber = 1;
        }
        HBAHW_D("Port numbering starts at %d\n", hba_chip->StartingPortNumber);

        ObtainSemaphore(&hba_chip->port_list_lock);
        for (int i = 0; i <= hba_chip->PortCountMax; i++) {
    		if (hba_chip->PortImplementedMask & (1 << i)) {
                ahci_add_port(hba_chip, hba_chip->StartingPortNumber+i, i);
    		}
    	}
        ReleaseSemaphore(&hba_chip->port_list_lock);

    	/* Enable interrupts for this HBA */
    	hwhba->ghc |= GHC_IE;

        return TRUE;
    }

    return FALSE;
}

BOOL ahci_reset_hba(struct ahci_hba_chip *hba_chip) {
    HBAHW_D("HBA-reset...\n");

    struct ahci_hwhba *hwhba;
    hwhba = hba_chip->abar;

    uint32_t Timeout;

	uint32_t saveCaps = hwhba->cap & (CAP_SMPS | CAP_SSS | CAP_SPM | CAP_EMS | CAP_SXS);
	uint32_t savePI = hwhba->pi;

    /* Enable AHCI mode of communication to the HBA */
	ahci_enable_hba(hba_chip);

    /* Reset HBA */
    hwhba->ghc |= GHC_HR;

    Timeout = 5000;
    while( (hwhba->ghc && GHC_HR) ) {
        if( (--Timeout == 0) ) {
            HBAHW_D("Resetting HBA timed out!\n");
            return FALSE;
            break;
        }
    }

    /* Re-enable AHCI mode */
    ahci_enable_hba(hba_chip);

    /* Write back values of CAP & PI that were saved before reset, maybe useless */
    hwhba->cap |= saveCaps;
    hwhba->pi = savePI;

    return TRUE;
}

void ahci_enable_hba(struct ahci_hba_chip *hba_chip) {
    HBAHW_D("HBA-enable...\n");

    struct ahci_hwhba *hwhba;
    hwhba = hba_chip->abar;

    /*
        Check if HBA supports legacy mode, otherwise GHC_AE(31) may be RO and the bit GHC_AE(31) is set
        Leave bits MRSM(2), IE(1) and HR(0) untouched
        Rest of the GHC register is RO (AHCI v1.3) do not set/clr other bits
    */
    if( !(hwhba->ghc && CAP_SAM) ){
        hwhba->ghc |= GHC_AE;
    }

}

BOOL ahci_disable_hba(struct ahci_hba_chip *hba_chip) {
    HBAHW_D("HBA-disable...\n");

    struct ahci_hwhba *hwhba;
    hwhba = hba_chip->abar;

    /*
        Check first if bit GHC_AE(31) is set and if so then check whether it is RO or RW
        If bit CAP_SAM(18) is not set then HBA can be set to AHCI or legacy mode
        If bit GHC_AE(31) is RW then clear the bit along with the other bits (no bits are allowed to be set at the same time)
    */
    if( (hwhba->ghc && GHC_AE) ){
        if( !(hwhba->ghc && CAP_SAM) ){
            hwhba->ghc = 0x00000000;
            return TRUE;
        }
    }

    return FALSE;
}

/* Ensure that the port is in idle state (It REALLY should be as we are just out of reset) */
BOOL ahci_init_port(struct ahci_hba_chip *hba_chip, uint32_t port_hba_num) {
    HBAHW_D("HBA-init_port...\n");

    struct ahci_hwhba *hwhba;
    hwhba = hba_chip->abar;

    /* These bits in port command register should all be cleared if the port is in idle state */
    HBAHW_D("P%dCMD = %.08x\n", port_hba_num, hwhba->port[port_hba_num].cmd);
    HBAHW_D("Start DMA? %s\n",              (hwhba->port[port_hba_num].cmd & PORT_CMD_ST) ? "yes" : "no");
    HBAHW_D("Command List Running? %s\n",   (hwhba->port[port_hba_num].cmd & PORT_CMD_CR) ? "yes" : "no");
    HBAHW_D("FIS Receive Enable? %s\n",     (hwhba->port[port_hba_num].cmd & PORT_CMD_FRE) ? "yes" : "no");
    HBAHW_D("FIS Receive Running? %s\n",    (hwhba->port[port_hba_num].cmd & PORT_CMD_FR) ? "yes" : "no");

    if ( (hwhba->port[port_hba_num].cmd & PORT_CMD_ST) ) {
    	/* Port is not in idle state */
        HBAHW_D("Running of command list is enabled! Disabling it...\n");
	    hwhba->port[port_hba_num].cmd &= ~PORT_CMD_ST;
	    if ( !(wait_until_clr(hba_chip, &hwhba->port[port_hba_num].cmd, PORT_CMD_CR, 500000)) ) {
            HBAHW_D("ERROR, timeout!\n");
            return FALSE;
        }
    }

    if ( (hwhba->port[port_hba_num].cmd & PORT_CMD_FRE) ) {
    	/* Port is not in idle state */
        HBAHW_D("FIS receive is enabled! Disabling it...\n");
	    hwhba->port[port_hba_num].cmd &= ~PORT_CMD_FRE;
	    if ( !(wait_until_clr(hba_chip, &hwhba->port[port_hba_num].cmd, PORT_CMD_FR, 500000)) ) {
            HBAHW_D("ERROR, timeout!\n");
            return FALSE;
        }
    }

    uint32_t tmp = hwhba->port[port_hba_num].serr;
    HBAHW_D("P%dERR = %.08x\n",port_hba_num, tmp);
	/* Clear all implemented bits by setting them to '1' */
    hwhba->port[port_hba_num].serr = tmp;

    /* TODO:
        Determine which events should cause an interrupt, and set each implemented port’s PxIE
        register with the appropriate enables. To enable the HBA to generate interrupts, system
        software must also set GHC.IE to a ‘1’.
        Note: Due to the multi-tiered nature of the AHCI HBA’s interrupt architecture, system
        software must always ensure that the PxIS (clear this first) and IS.IPS (clear this second)
        registers are cleared to ‘0’ before programming the PxIE and GHC.IE registers. This will
        prevent any residual bits set in these registers from causing an interrupt to be asserted.
    */

    return TRUE;
}

/*
    Add a port to the HBA-port list.
    Physical port number is "port_hba_num" and it is added as unit number "port_unit_num" to the system
    Make sure the port in question is free and ready for use, if not make it so
*/
BOOL ahci_add_port(struct ahci_hba_chip *hba_chip, uint32_t port_unit_num, uint32_t port_hba_num) {
    HBAHW_D("HBA-add_port...\n");

    HBAHW_D("add HBA-port %d as UNIT:%d\n",port_hba_num, port_unit_num);

    struct ahci_hba_port *hba_port;
    if( (hba_port = (struct ahci_hba_port*) AllocVec(sizeof(struct ahci_hba_port), MEMF_CLEAR|MEMF_PUBLIC)) ) {
        HBAHW_D("hba_port struct @ %p\n",hba_port);

        struct ahci_hwhba *hwhba;
        hwhba = hba_chip->abar;

        hba_port->port_unit.parent_hba = hba_chip;
        hba_port->port_unit.Port_HBA_Number = port_hba_num;
        hba_port->port_unit.Port_Unit_Number = port_unit_num;

        if( ahci_init_port(hba_chip, port_hba_num) ) {
            /* HBA-port list is protected for us in ahci_init_hba */
            AddTail((struct List*)&hba_chip->port_list, (struct Node*)hba_port);
            return TRUE;
        }
    }

    /* Failed in setting the port up, skipping system unit number, e.g. there is no unit for this port_unit_num */
    return FALSE;
}

