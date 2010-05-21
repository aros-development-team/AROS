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
    D(bug("[HBATASK] HBA task running...\n"));

    D(bug("[HBATASK] Signaling parent %08x\n", parent));
    Signal(parent, SIGBREAKF_CTRL_C);

    if ( ahci_init_hba(hba_chip) ) {
        D(bug("[HBATASK] ahci_init_hba done!\n"));
        Wait(0);
    } else {
        D(bug("[HBATASK] ahci_init_hba failed!\n"));
        /*
            Something failed while setting up the HW part of the HBA
            Release all allocated memory and other resources for this HBA
            and remove us from the list
        */

    }
    D(bug("[HBATASK] (%04x:%04x) Bye!\n", hba_chip->VendorID, hba_chip->ProductID));
}

BOOL ahci_setup_hbatask(struct ahci_hba_chip *hba_chip) {
    D(bug("[HBAHW] (%04x:%04x) Setting up HBA task...\n", hba_chip->VendorID, hba_chip->ProductID));

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

                D(bug("[HBAHW] Waiting HBA task signal\n"));
                Wait(SIGBREAKF_CTRL_C);
                D(bug("[HBAHW] Signal from HBA task received\n"));
            }
        }else{
            FreeMem(ml,sizeof(struct MemList) + sizeof(struct MemEntry));
        }
    }else{
        FreeMem(t,sizeof(struct Task));
    }

    return (sp != NULL);
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

BOOL ahci_init_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[HBAHW] (%04x:%04x) HBA init...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hwhba *hwhba;
    hwhba = hba_chip->abar;

    /*
        BIOS handoff if HBA supports it and BIOS is the current owner of HBA
    */
    hba_chip->Version = hwhba->vs;
    D(bug("[HBAHW] AHCI Version %x.%02x\n",
        ((hba_chip->Version >> 20) & 0xf0) + ((hba_chip->Version >> 16) & 0x0f),
        ((hba_chip->Version >> 4) & 0xf0) + (hba_chip->Version & 0x0f) ));

    if ( (hba_chip->Version >= AHCI_VERSION_1_20) ) {
        if( (hwhba->cap2 && CAP2_BOH) ) {
            D(bug("[HBAHW] HBA supports BIOS/OS handoff\n"));
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

    /*
        Reset the HBA
    */
    if ( ahci_reset_hba(hba_chip) ) {
        D(bug("[HBAHW] HBA reseted...\n"));

        /*
            Get the pointer to previous HBA-chip struct in the list (if any)
            Port numbering starts from 1 if no previous HBA exist else the port number will be
            starting number for previous HBA-chips ports + the number of implemented ports in that (previous) HBA-chip
            Port number 0 (e.g. unit number 0) is reserved/not implemented
        */
        struct ahci_hba_chip *prev_hba_chip = (struct ahci_hba_chip*) GetPred(hba_chip);

        if ( prev_hba_chip != NULL ) {
            hba_chip->StartingPortNumber = (prev_hba_chip->StartingPortNumber) + (prev_hba_chip->PortsImplemented);
        }else{
            hba_chip->StartingPortNumber = 1;
        }
        D(bug("[HBAHW] Port numbers start at %d\n",hba_chip->StartingPortNumber));
        D(bug("[HBAHW] Ports implemented\n        ["));

        hba_chip->PortsImplemented = 0;
        LONG i;

        for (i = 31; i >= 0; i--) {
            if( ((hwhba->pi) & (1<<i)) ) {
                hba_chip->PortsImplemented = hba_chip->PortsImplemented++;
                D(bug("x"));
            }else{
                D(bug("."));
            }
        }
        D(bug("] %d\n",hba_chip->PortsImplemented));

    }else{
        return FALSE;
    }

    return TRUE;
}

BOOL ahci_reset_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[HBAHW] (%04x:%04x) HBA reset...\n", hba_chip->VendorID, hba_chip->ProductID));

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
            D(bug("[AHCI] Resetting HBA timed out!\n"));
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
    D(bug("[HBAHW] (%04x:%04x) HBA enable...\n", hba_chip->VendorID, hba_chip->ProductID));

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
    D(bug("[HBAHW] (%04x:%04x) HBA disable...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hwhba *hwhba;
    hwhba = hba_chip->abar;

    /*
        Check first if bit GHC_AE is set and if so then check whether it is RO or RW
        If bit CAP_SAM is not set then HBA can be set to AHCI or legacy mode
        If bit GHC_AE is RW then clear the bit along with the other bits (no bits are allowed to be set at the same time)
    */
    if( (hwhba->ghc && GHC_AE) ){
        if( !(hwhba->ghc && CAP_SAM) ){
            hwhba->ghc = 0x00000000;
            return TRUE;
        }
    }

    return FALSE;
}
