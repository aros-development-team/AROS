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

    /*
        BIOS handoff if HBA supports it and BIOS is the current owner of HBA
    */
    hba_chip->Version = hwhba->vs;

    if ( (hba_chip->Version >= AHCI_VERSION_1_20) ) {
        if( (hwhba->cap2 && CAP2_BOH) ) {
            HBAHW_D("HBA supports BIOS/OS handoff\n");
            if( (hwhba->bohc && BOHC_BOS) ) {
                hwhba->bohc |= BOHC_OOS;
                while( (hwhba->bohc && BOHC_BOS) );
                //wait 25ms
                if( (hwhba->bohc && BOHC_BB) ) {
                    //Wait minimum of 2 seconds to give BIOS time for finishing outstanding commands.
                }
            }
        }
    }

    /*
        Enable AHCI mode of communication to the HBA
    */
    ahci_enable_hba(hba_chip);

#if defined(__i386__)
    hwhba->cap = ~CAP_S64A;
#endif

    /*
        Reset the HBA
    */
    if ( ahci_reset_hba(hba_chip) ) {
        HBAHW_D("Reset done\n");

        hba_chip->CommandSlotCount = 1 + ((hwhba->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
        hba_chip->PortCountMax = 1 + ((hwhba->cap >> CAP_NP_SHIFT) & CAP_NP_MASK);

	    hba_chip->PortImplementedMask = hwhba->pi;
	    if (hba_chip->PortImplementedMask == 0) {
		    hba_chip->PortImplementedMask = 0xffffffff >> (32 - hba_chip->PortCountMax);
		    HBAHW_D("ports-implemented mask is zero, using 0x%lx instead.\n", hba_chip->PortImplementedMask);
	    }

	    hba_chip->PortCount = count_bits_set(hba_chip->PortImplementedMask);

        HBAHW_D("cap: Interface Speed Support: generation %lu\n",   (hwhba->cap >> CAP_ISS_SHIFT) & CAP_ISS_MASK);
        HBAHW_D("cap: Number of Command Slots: %d (raw %lx)\n",     hba_chip->CommandSlotCount, (hwhba->cap >> CAP_NCS_SHIFT) & CAP_NCS_MASK);
        HBAHW_D("cap: Supports Port Multiplier: %s\n",              (hwhba->cap & CAP_SPM) ? "yes" : "no");
        HBAHW_D("cap: Supports External SATA: %s\n",                (hwhba->cap & CAP_SXS) ? "yes" : "no");
        HBAHW_D("cap: Enclosure Management Supported: %s\n",        (hwhba->cap & CAP_EMS) ? "yes" : "no");
        HBAHW_D("cap: Supports Command List Override: %s\n",        (hwhba->cap & CAP_SCLO) ? "yes" : "no");
        HBAHW_D("cap: Supports Staggered Spin-up: %s\n",            (hwhba->cap & CAP_SSS) ? "yes" : "no");
        HBAHW_D("cap: Supports Mechanical Presence Switch: %s\n",   (hwhba->cap & CAP_SMPS) ? "yes" : "no");
        HBAHW_D("cap: Supports 64-bit Addressing: %s\n",            (hwhba->cap & CAP_S64A) ? "yes" : "no");
        HBAHW_D("cap: Supports Native Command Queuing: %s\n",       (hwhba->cap & CAP_SNCQ) ? "yes" : "no");
        HBAHW_D("cap: Supports SNotification Register: %s\n",       (hwhba->cap & CAP_SSNTF) ? "yes" : "no");
        HBAHW_D("cap: Supports Command List Override: %s\n",        (hwhba->cap & CAP_SCLO) ? "yes" : "no");
        HBAHW_D("AHCI Version %lu.%lu (raw %lx)\n",                 ((hba_chip->Version >> 24) & 0xf)*10 + ((hba_chip->Version >> 16) & 0xf),
                                                                    ((hba_chip->Version >> 8) & 0xf)*10 + (hba_chip->Version & 0xf), hwhba->vs );
        HBAHW_D("Interrupt %u\n",                                   hba_chip->IRQ);

        if( !ahci_create_interrupt(hba_chip) )
            return FALSE;

        HBAHW_D("ports count %ld\n", hba_chip->PortCount);

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

        for (int i = 0; i <= hba_chip->PortCountMax; i++) {
    		if (hba_chip->PortImplementedMask & (1 << i)) {
                ahci_add_port(hba_chip, i, hba_chip->StartingPortNumber+i);
    		}
    	}

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

    ULONG Timeout;

	ULONG saveCaps = hwhba->cap & (CAP_SMPS | CAP_SSS | CAP_SPM | CAP_EMS | CAP_SXS);
	ULONG savePI = hwhba->pi;

    /*
        Enable AHCI mode of communication to the HBA
    */
	ahci_enable_hba(hba_chip);

    /*
        Reset HBA
    */
    hwhba->ghc |= GHC_HR;

    Timeout = 5000;
    while( (hwhba->ghc && GHC_HR) ) {
        if( (--Timeout == 0) ) {
            HBAHW_D("Resetting HBA timed out!\n");
            return FALSE;
            break;
        }
    }

    /*
        Re-enable AHCI mode
    */
    ahci_enable_hba(hba_chip);

    /*
        Write back values of CAP & PI that were saved before reset, maybe useless
    */
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
        Rest of the GHC registers are RO (AHCI v1.3) do not set/clr other bits
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

BOOL ahci_add_port(struct ahci_hba_chip *hba_chip, ULONG port_unit_num, ULONG port_hba_num) {
    HBAHW_D("HBA-add_port...");

    HBAHW_D("added HBA-port %d as UNIT:%d\n",port_hba_num, port_unit_num);

    struct ahci_hba_port *hba_port;
    if( (hba_port = (struct ahci_hba_port*) AllocVec(sizeof(struct ahci_hba_port), MEMF_CLEAR|MEMF_PUBLIC)) ) {
        HBAHW_D("hba_port @ %p\n",hba_port);

        hba_port->parent_hba = hba_chip;
        hba_port->Port_HBA_Number = port_hba_num;
        hba_port->Port_Unit_Number = port_unit_num;

        AddTail((struct List*)&hba_chip->port_list, (struct Node*)hba_port);

        return TRUE;
    }

    return FALSE;
}

