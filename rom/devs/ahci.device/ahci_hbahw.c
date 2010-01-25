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
        BIOS handoff if HBA supports it
    */
	if ( (hba_chip->Version >= AHCI_VERSION_1_20) ) {
		if( (hba->cap2 && CAP2_BOH) ) {
            D(bug("[AHCI] HBA supports BIOS/OS handoff\n"));
            hba->bohc |= BOHC_OOS;
            while( (hba->bohc && BOHC_BOS) );
            //wait 25ms
            if( (hba->bohc && BOHC_BB) ) {
                //Wait minimum of 2 seconds to give BIOS time for finishing outstanding commands.
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
    hba_chip->PortNBR = 0;
    for (i = 0; i <= 31; i++) {
        if( ((hba->pi) & (1<<i)) ) {
            hba_chip->PortNBR++;
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
	ahci_enable_hba(hba_chip);

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
    ahci_enable_hba(hba_chip);

	/*
        Clear interrupts
    */

	/*
        Configure CCC, if supporting
    */

}

BOOL ahci_enable_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA enable...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hba *hba;
    hba = hba_chip->abar;

    /*
        Access to any other register than to GHC is not allowed when GHC_AE = 0,
        ...but the implementation of GHC_AE bit depends on CAP_SAM -> RW or RO ?!?
        Clear other bits while setting AE (MRSM, IE and HR)
    */

    if( !(hba->cap == CAP_SAM) )
	    hba->ghc = GHC_AE;

    return ((hba->ghc && GHC_AE) ? TRUE:FALSE);
}

void ahci_disable_hba(struct ahci_hba_chip *hba_chip) {
    D(bug("[AHCI] (%04x:%04x) HBA disable...\n", hba_chip->VendorID, hba_chip->ProductID));

    struct ahci_hba *hba;
    hba = hba_chip->abar;

    if( !(hba->cap == CAP_SAM) )
        hba->ghc = 0x00000000;

}
