               INCLUDE   "exec/funcdef.i"
               INCLUDE   "exec/exec_lib.i"
               INCLUDE   "macros.i"
               INCLUDE   "shared_defs.i"

* $Id: sv_regs851.asm,v 3.5 95/12/16 18:36:39 Martin_Apel Exp $

               XDEF      _MMU68851
               XDEF      _GenDescr851

               XDEF      _SetMMUState851
               XDEF      _ReadMMUState851
               XDEF      _SaveMMUState851
               XDEF      _RestoreMMUState851


TCB_E          EQU       31                  ; Translation control: enable
TCF_E          EQU       (1<<31)
SRB_I          EQU       10                  ; MMUSR: Invalid
SRF_I          EQU       (1<<10)
SRB_T          EQU       6                   ; MMUSR: TT hit
SRF_T          EQU       (1<<6)
PDB_CI         EQU       6                   ; Page descr: cache inhibit
PDF_CI         EQU       (1<<6)
PDB_WP         EQU       2                   ; Page descr: write protected
PDF_WP         EQU       (1<<2)


               MACHINE   68020
               PMMU

               SECTION   CODE

_MMU68851:     * Checks if there is a working 68851 MMU in the system
               * No need to take care of caches because this is only called
               * on 68020 systems

               PRINT_DEB "Checking for 68851"
               movem.l   a5-a6,-(sp)
               move.l    4,a6
               lea       Check851(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts

Check851:      movec     VBR,a0
               lea       11*4(a0),a0
               move.l    (a0),d1                  ; old LINE-F vector
               lea       Trap851(pc),a1
               move.l    a1,(a0)
               moveq     #1,d0
               pmove     tc,-4(sp)                ; might cause trap
               move.l    d1,(a0)                  ; restore original LINE-F vector
               * d0 contains if valid 68851 recognized
               rte
               

Trap851:       moveq     #0,d0
               PRINT_DEB "68851 NOT detected"
               add.l     #6,2(sp)                 ; skip causing instruction
               rte

***********************************************

_SaveMMUState851:
               * Saves the current MMU registers into a private buffer.
               * No inputs, no outputs
               lea       _PrivateMMUState(pc),a0
               move.l    a0,-(sp)
               bsr       _ReadMMUState851
               addq      #4,sp
               lea       _MMUStateValid(pc),a0
               move.w    #1,(a0)
               rts

***********************************************

_RestoreMMUState851:
               * Restore the MMU registers from the private buffer.
               * No inputs, no outputs

               tst.w     _MMUStateValid(pc)
               beq       Ready
               lea       _PrivateMMUState(pc),a0
               move.l    a0,-(sp)
               bsr       _SetMMUState851
               addq      #4,sp
Ready          rts

***********************************************

_MMUStateValid dc.w       0
_PrivateMMUState:
               ds.b       MS30_SIZE

***********************************************

_SetMMUState851:
               * Called from C as:
               * void SetMMUState851 (MMUState851*)

               movem.l   a5-a6,-(sp)
               move.l    12(sp),a0
               move.l    4,a6
               lea       SetMMUState851(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts

SetMMUState851 lea       -4(sp),a1                ; reserve long-word on stack
               clr.l     (a1)
               pmove     (a1),TC                  ; MMU ausschalten

               pmove     MS30_CRP_HI(a0),CRP
               pmove     MS30_SRP_HI(a0),SRP
               pmove     MS30_TC(a0),TC           ; MMU einschalten
               PRINT_DEB "MMU registers set"

               rte

***********************************************

_ReadMMUState851:
               * Called from C as:
               * void ReadMMUState851 (MMUState851*)

               movem.l   a5-a6,-(sp)
               move.l    12(sp),a0                   ; start address of MMUState
               move.l    4,a6
               lea       ReadMMUState851(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts

ReadMMUState851:
               pmove     TC,MS30_TC(a0)
               pmove     CRP,MS30_CRP_HI(a0)
               pmove     SRP,MS30_SRP_HI(a0)
               rte

***********************************************

_GenDescr851:  * This function generates an MMU page descriptor for
               * a given logical address. It includes all the standard
               * information a page descriptor contains. 
               * C prototype:
               *  ULONG GenDescr (ULONG LogAddr);

               movem.l   a5-a6,-(sp)
               move.l    4,a6
               move.l    12(sp),a0
               PRINT_DEB "GenDescr851 called for address %08lx",a0
               lea       GenDescr(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts

GenDescr:      * If MMU is turned off, no PTEST instruction must be executed.
               sub.w     #4,sp
               pmove     tc,(sp)
               move.l    (sp)+,d0
               btst.l    #TCB_E,d0
               beq       MMUOff

               * Check if there is a valid table mapping for this address
               sub.w     #2,sp
               ptestr    #1,(a0),#7,a1
               pmove     mmusr,(sp)
               move.w    (sp)+,d0
               btst.l    #SRB_I,d0
               bne       InvalidTranslation

               * there is a valid mapping: Extract the physical address,
               * the WP and cache mode.
               move.l    (a1),d0
               and.w     #((~(PAGESIZE-1))|PDF_CI|PDF_WP),d0
               bset.l    #0,d0                    ; make resident
               rte
               
MMUOff:        * MMU is turned off. Return the logical address as
               * the physical address
               move.l    a0,d0
               and.w     #~(PAGESIZE-1),d0
               bset.l    #0,d0                    ; make resident, cacheable
               rte

InvalidTranslation:
               * return illegal descriptor
               moveq     #0,d0
               rte

               END
