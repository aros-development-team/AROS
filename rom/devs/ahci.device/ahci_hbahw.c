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

static void ahci_taskcode_hba(struct ahci_hba_chip *hba_chip, struct Task* parent) {
    D(bug("[AHCI] (%04x:%04x) Hello! HBA task running...\n", hba_chip->VendorID, hba_chip->ProductID));

    D(bug("Signaling parent %08x\n", parent));

    Signal(parent, SIGBREAKF_CTRL_C);

//    ahci_init_hba( hba_chip );

    while(1){
    }

    D(bug("[AHCI] (%04x:%04x) Bye!\n", hba_chip->VendorID, hba_chip->ProductID));
}

BOOL ahci_setup_hbatask(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA task setup...\n", hba_chip->VendorID, hba_chip->ProductID));

    APTR stack;

    hba_chip->hba_task = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (hba_chip->hba_task) {
    	NEWLIST(&hba_chip->hba_task->tc_MemEntry);
    	hba_chip->hba_task->tc_Node.ln_Type =NT_TASK;
    	hba_chip->hba_task->tc_Node.ln_Name = "HBA Task";
    	hba_chip->hba_task->tc_Node.ln_Pri  = HBA_TASK_PRI;

    	stack = AllocMem(HBA_TASK_STACKSIZE, MEMF_PUBLIC);
    	if(stack != NULL) {
	        struct TagItem tags[] = {
                {TASKTAG_ARG1, (IPTR)hba_chip},
                {TASKTAG_ARG2, (IPTR)FindTask(0)},
                {TAG_DONE}
	        };
	    
	        hba_chip->hba_task->tc_SPLower = stack;
	        hba_chip->hba_task->tc_SPUpper = (UBYTE *)stack + HBA_TASK_STACKSIZE;

        #if AROS_STACK_GROWS_DOWNWARDS
	        hba_chip->hba_task->tc_SPReg = (UBYTE *)hba_chip->hba_task->tc_SPUpper-SP_OFFSET;
        #else
	        hba_chip->hba_task->tc_SPReg = (UBYTE *)hba_chip->hba_task->tc_SPLower+SP_OFFSET;
        #endif

            if(NewAddTask(hba_chip->hba_task, ahci_taskcode_hba, NULL, tags) != NULL) {
                Wait(SIGBREAKF_CTRL_C);
                return TRUE;
            }	

            FreeMem(stack, HBA_TASK_STACKSIZE);	
        }
        FreeMem(hba_chip->hba_task,sizeof(struct Task));
    }
    return FALSE;
}

/*
    Set the AHCI HBA to a minimally initialized state.

        * Indicate that the system is AHCI aware by setting bit GHC_AE in HBA's GHC register
        * Determine number of ports implemented by the HBA
        * Ensure that all implemented ports are in IDLE state
        * Determine how many command slots the HBA supports, by reading CAP.NCS
        * Set CAP.S64A to ‘0’, no 64bit address support for now...
        * For each implemented port, allocate memory for and program registers:
            - PxCLB (and PxCLBU=0, upper 32bits of 64bit address space)
            - PxFB (and PxFBU=0, upper 32bits of 64bit address space)
        * For each implemented port, clear the PxSERR register
        * Allocate and initialize interrupts, and set each implemented port’s PxIE
            register with the appropriate enables. To enable the HBA to generate interrupts, system
            software must also set GHC.IE to a ‘1’.
*/

void ahci_init_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA init...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hba *hba;
    hba = hba_chip->abar;

    /*
        Enable AHCI mode
    */
	ahci_enable_hba(hba_chip);

    hba_chip->Version = hba->vs;
    D(bug("[AHCI] Version %x.%02x\n",
        ((hba_chip->Version >> 20) & 0xf0) + ((hba_chip->Version >> 16) & 0x0f),
        ((hba_chip->Version >> 4) & 0xf0) + (hba_chip->Version & 0x0f) ));

    /*
        BIOS handoff if HBA supports it and BIOS is the current owner of HBA
    */
	if ( (hba_chip->Version >= AHCI_VERSION_1_20) ) {
		if( (hba->cap2 && CAP2_BOH) ) {
            D(bug("[AHCI] HBA supports BIOS/OS handoff\n"));
            if( (hba->bohc && BOHC_BOS) ) {
                hba->bohc |= BOHC_OOS;
                while( (hba->bohc && BOHC_BOS) );
                //wait 25ms
                if( (hba->bohc && BOHC_BB) ) {
                    //Wait minimum of 2 seconds to give BIOS time for finishing outstanding commands.
                }
            }
        }
    }

    /*
        Reset the HBA
    */
    ahci_reset_hba( hba_chip );

    /*
        What about the staggered spin-up?
    */

    ULONG i;
    hba_chip->PortCount = 0;
    for (i = 0; i <= 31; i++) {
        if( ((hba->pi) & (1<<i)) ) {
            hba_chip->PortCount++;
        }
    }

}

void ahci_reset_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA reset...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hba *hba;
    hba = hba_chip->abar;

    ULONG Timeout;

    /*
        Enable AHCI mode
    */
	ahci_enable_hba( hba_chip );

    /*
        Reset HBA
    */
    hba->ghc |= GHC_HR;

    Timeout = 5000;
    while( (hba->ghc && GHC_HR) ) {
        if( (--Timeout == 0) )
            D(bug("[AHCI] Timeout while doing HBA reset!\n"));
            break;
    }

    /*
        Reenable AHCI mode
    */
    ahci_enable_hba( hba_chip );

	/*
        Clear interrupts
    */

	/*
        Configure CCC, if supporting
    */

}

void ahci_enable_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA enable...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hba *hba;
    hba = hba_chip->abar;

    /*
        Check first if bit GHC_AE is not set (HBA acts as legacy) otherwise we may not be allowed to touch the bit as it may be RO (AHCI mode only)
        Clear other bits while setting GHC_AE (GHC_MRSM, GHC_IE and GHC_HR)
    */

    if( !(hba->ghc && GHC_AE) ){
        hba->ghc = GHC_AE;
    }

}

void ahci_disable_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA disable...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hba *hba;
    hba = hba_chip->abar;

    /*
        Check first if bit GHC_AE is set and if so then check whether it is RO or RW (if CAP_SAM not set then HBA can be AHCI or legacy)
        If bit is RW then clear the bit along with the other bits
    */

    if( (hba->ghc && GHC_AE) ){
        if( !(hba->ghc && CAP_SAM) ){
            hba->ghc = 0x00000000;
        }
    }

}
