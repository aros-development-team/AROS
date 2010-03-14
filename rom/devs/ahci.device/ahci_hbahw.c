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
    D(bug("Hello! HBA task running...\n"));

    D(bug("Signaling parent %08x\n", parent));

    Signal(parent, SIGBREAKF_CTRL_C);

//    ahci_init_hba( hba_chip );

//    for(;;);

    D(bug("[AHCI] (%04x:%04x) Bye!\n", hba_chip->VendorID, hba_chip->ProductID));
}

BOOL ahci_setup_hbatask(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA task setup...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct Task     *t;
    struct MemList  *ml;


    struct TagItem tags[] = {
        { TASKTAG_ARG1, (IPTR)hba_chip },
        { TASKTAG_ARG2, (IPTR)FindTask(0) },
        { TAG_DONE,     0 }
    };

    t = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (t) {
        ml = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry), MEMF_PUBLIC | MEMF_CLEAR);
        if(ml) {
    	    UBYTE *sp = AllocMem(HBA_TASK_STACKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
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

                D(bug("[AHCI] Waiting HBA task signal\n"));
                Wait(SIGBREAKF_CTRL_C);
                D(bug("[AHCI] Signal from HBA task received\n"));
                return TRUE;
            }else{
                FreeMem(ml,sizeof(struct MemList) + sizeof(struct MemEntry));
                FreeMem(t,sizeof(struct Task));
                return FALSE;
            }
        }
    }
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
	if( ahci_enable_hba( hba_chip ) ) {

        /*
            Reset HBA
        */
        hba->ghc |= GHC_HR;

        Timeout = 5000;
        while( (hba->ghc && GHC_HR) ) {
            if( (--Timeout == 0) ) {
                D(bug("[AHCI] Timeout while doing HBA reset!\n"));
                break;
            }
        }

        /*
            Re-enable AHCI mode
        */
        ahci_enable_hba( hba_chip );

	    /*
            Clear interrupts
        */

	    /*
            Configure CCC, if supporting
        */
    }

}

BOOL ahci_enable_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA enable...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hba *hba;
    hba = hba_chip->abar;

    /*
        Check first if bit GHC_AE is not set (HBA acts as legacy) otherwise we may not be allowed to touch the bit as it may be RO (AHCI mode only)
        Clear other bits while setting GHC_AE (GHC_MRSM, GHC_IE and GHC_HR)
    */

    if( !(hba->ghc && GHC_AE) ){
        hba->ghc = GHC_AE;
        return TRUE;
    }

    return FALSE;
}

BOOL ahci_disable_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA disable...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hba *hba;
    hba = hba_chip->abar;

    /*
        Check first if bit GHC_AE is set and if so then check whether it is RO or RW
        If bit CAP_SAM is not set then HBA can be set to AHCI or legacy mode
        If bit GHC_AE is RW then clear the bit along with the other bits
    */

    if( (hba->ghc && GHC_AE) ){
        if( !(hba->ghc && CAP_SAM) ){
            hba->ghc = 0x00000000;
            return TRUE;
        }
    }

    return FALSE;
}
