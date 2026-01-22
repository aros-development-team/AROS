               INCLUDE   "exec/funcdef.i"
               INCLUDE   "exec/exec_lib.i"
               INCLUDE   "macros.i"
               INCLUDE   "shared_defs.i"

* $Id: sv_regs40.asm,v 3.6 95/12/16 18:36:07 Martin_Apel Exp $

               XDEF      _ReadVBR
               XDEF      _CPushP40
               XDEF      _CPushL40
               XDEF      _PFlushP40
               XDEF      _PFlushA40
               XDEF      _SetMMUState40
               XDEF      _ReadMMUState40
               XDEF      _SaveMMUState40
               XDEF      _RestoreMMUState40
               XDEF      _GetPageSize40

               XDEF      _GenDescr40
               XDEF      _EmptyFunc

               MACHINE   68040


               SECTION   CODE

_ReadVBR:      movem.l   a5/a6,-(sp)
               lea       ReadVBR(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               rts

ReadVBR:       movec     VBR,d0
               rte

**************************************

               ALIGN_LONG
_CPushP40:     movem.l   a5/a6,-(sp)
               move.l    12(sp),a0
               lea       CPushP(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               rts

               ALIGN_LONG
CPushP:        CPUSHP    BC,(a0)

               rte

**************************************

               ALIGN_LONG
_CPushL40:     movem.l   a5/a6,-(sp)
               move.l    12(sp),a0
               lea       CPushL(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               rts

               ALIGN_LONG
CPushL:        CPUSHL    BC,(a0)

               rte

**************************************

               ALIGN_LONG
_PFlushP40:    movem.l   a5/a6,-(sp)
               move.l    12(sp),a0
               lea       PFlush_cmd(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               rts

               ALIGN_LONG
PFlush_cmd:    dc.w      $f508               ; pflush (a0)
               rte

**************************************

_PFlushA40:    movem.l   a5/a6,-(sp)
               lea       PFlushA_cmd(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               rts

PFlushA_cmd:   dc.w      $f518               ; pflusha
               rte

**************************************

_SetMMUState40:
               * Called from C as:
               * void SetMMUState40 (MMUState40*)

               movem.l   a5-a6,-(sp)
               move.l    12(sp),a0                   ; start address of MMUState

               PRINT_DEB "New URP = %08lx",MS40_URP(a0)
               PRINT_DEB "New SRP = %08lx",MS40_SRP(a0)
               PRINT_DEB "New ITT0 = %08lx",MS40_ITT0(a0)
               PRINT_DEB "New ITT1 = %08lx",MS40_ITT1(a0)
               PRINT_DEB "New DTT0 = %08lx",MS40_DTT0(a0)
               PRINT_DEB "New DTT1 = %08lx",MS40_DTT1(a0)
               IFD       DEBUG
               move.l    MS40_TC(a0),d0
               PRINT_DEB "New TC = %08lx",d0
               ENDC
               
               move.l    4,a6
               lea       SetMMUState40(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts

SetMMUState40: suba.l    a1,a1
               movec     a1,TC                       ; disable MMU, assumes
                                                     ; direct mapping of RAM
                                                     ; address
               move.l    MS40_URP(a0),d0
               movec     d0,URP
               move.l    MS40_SRP(a0),d0
               movec     d0,SRP
               move.l    MS40_ITT0(a0),d0
               movec     d0,ITT0
               move.l    MS40_ITT1(a0),d0
               movec     d0,ITT1
               move.l    MS40_DTT0(a0),d0
               movec     d0,DTT0
               move.l    MS40_DTT1(a0),d0
               movec     d0,DTT1
               move.l    MS40_TC(a0),d0
               dc.w      $f518                       ; pflusha
               movec     d0,TC                       ; set TC to desired state
               rte

**************************************

_ReadMMUState40:
               * Called from C as:
               * void ReadMMUState40 (MMUState40*)

               movem.l   a5-a6,-(sp)
               move.l    12(sp),a0                   ; start address of MMUState
               move.l    4,a6
               lea       ReadMMUState40(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts

ReadMMUState40: 
               movec     URP,d0
               PRINT_DEB "Old URP = %08lx",d0
               move.l    d0,MS40_URP(a0)
               movec     SRP,d0
               PRINT_DEB "Old SRP = %08lx",d0
               move.l    d0,MS40_SRP(a0)
               movec     ITT0,d0
               PRINT_DEB "Old ITT0 = %08lx",d0
               move.l    d0,MS40_ITT0(a0)
               movec     ITT1,d0
               PRINT_DEB "Old ITT1 = %08lx",d0
               move.l    d0,MS40_ITT1(a0)
               movec     DTT0,d0
               PRINT_DEB "Old DTT0 = %08lx",d0
               move.l    d0,MS40_DTT0(a0)
               movec     DTT1,d0
               PRINT_DEB "Old DTT1 = %08lx",d0
               move.l    d0,MS40_DTT1(a0)
               moveq     #0,d0
               movec     TC,d0
               PRINT_DEB "Old TC = %08lx",d0
               move.l    d0,MS40_TC(a0)
               rte

**************************************

_SaveMMUState40:
               * Saves the current MMU registers into a private buffer.
               * No inputs, no outputs
               lea       PrivateMMUState,a0
               move.l    a0,-(sp)
               bsr       _ReadMMUState40
               addq      #4,sp
               move.w    #1,MMUStateValid
               rts

**************************************

_RestoreMMUState40:
               * Restore the MMU registers from the private buffer.
               * No inputs, no outputs
               tst.w     MMUStateValid
               beq       Ready
               lea       PrivateMMUState,a0
               move.l    a0,-(sp)
               bsr       _SetMMUState40
               addq      #4,sp
Ready          rts

**************************************

GetMMURegs:    * Returns TC   in d3
               *         URP  in d4
               *         DTT0 in d5
               *         DTT1 in d6
               * Does not modify any other registers

               movem.l   a5-a6,-(sp)
               move.l    4,a6
               lea       ReadMMURegs(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts

ReadMMURegs:   moveq     #0,d3          ; on the 68040 TC is only 16 bit
               movec     TC,d3
               movec     URP,d4
               movec     DTT0,d5
               movec     DTT1,d6
               rte

******************************************************************************

               BITDEF    TC,E,15        ; TC: Enable 
               BITDEF    TT,E,15        ; TT: Enable

_GenDescr40:   * This function generates an MMU page descriptor for
               * a given logical address. It includes all the standard
               * information a page descriptor contains. In addition
               * the U bit is misused to detect a transparent translation
               * register hit. This way there is no need for separate 
               * handling of cache modes and such for each processor
               * type.
               * C prototype:
               *  ULONG GenDescr (ULONG LogAddr);

               move.l    4(sp),a0
               movem.l   d2-d6,-(sp)
               bsr       GetMMURegs
               PRINT_DEB "GenDescr40 called for address %08lx",a0
               btst.l    #TTB_E,d5
               beq       DTT0_Disabled
               
               PRINT_DEB "GenDescr40: DTT0 enabled"
               move.l    a0,d0                    ; save address for eor
               move.l    d5,d1                    ; save DTT0 for mask
               move.l    d5,d2                    ; save DTT0 for CM and WP
               eor.l     d5,d0                    ; cmp with DTT0
               asl.l     #8,d1                    ; shift mask
               not.l     d1
               and.l     d1,d0                    ; only look at non-masked bits
               and.l     #$ff000000,d0            ; only A31 - A24 valid here
               bne       NotTT0Hit

GenDescrFromTT:
               * a0: logical address
               * d2: contents of appropriate DTTx

               PRINT_DEB "GenDescr40: Address matches DTTx"
               and.w     #$64,d2                  ; CM and WP
               move.l    a0,d0
               and.w     #~(PAGESIZE-1),d0
               or.w      d2,d0                    ; put CM and WP into descr
               or.w      #$9,d0                   ; mark as resident and TT hit
               PRINT_DEB "GenDescr40: Returns %08lx from TTx",d0
               movem.l   (sp)+,d2-d6
               rts

NotTT0Hit
DTT0_Disabled:
               btst.l    #TTB_E,d6
               beq       DTT1_Disabled
               PRINT_DEB "GenDescr40: DTT1 enabled"
               move.l    a0,d0                    ; save address for eor
               move.l    d6,d1                    ; save DTT1 for mask
               move.l    d6,d2                    ; save DTT1 for CM and WP
               eor.l     d6,d0                    ; cmp with DTT1
               asl.l     #8,d1                    ; shift mask
               not.l     d1
               and.l     d1,d0                    ; only look at non-masked bits
               and.l     #$ff000000,d0            ; only A31 - A24 valid here
               beq       GenDescrFromTT
               PRINT_DEB "GenDescr40: DTT1 does not match"
DTT1_Disabled:
               PRINT_DEB "GenDescr40: Checking page tables"
               btst.l    #TCB_E,d3                ; Check MMU enabled
               beq       MMUTurnedOff
               PRINT_DEB "GenDescr40: MMU is turned on"

               * Work through the page tables
               move.l    d4,a1                    ; URP
               
               move.l    a0,d0               
               ROOTINDEX
               PRINT_DEB "Rootindex is %08lx",d0
               move.l    (a1,d0.w*4),d1
               btst.l    #1,d1
               beq       InvalidTranslation
               and.l     #~(POINTERS_PER_TABLE*4-1),d1
               move.l    d1,a1
               move.l    a0,d0
               POINTERINDEX
               PRINT_DEB "Pointerindex is %08lx",d0
               move.l    (a1,d0.w*4),d1
               btst.l    #1,d1
               beq       InvalidTranslation
               and.l     #~(POINTERS_PER_TABLE*4-1),d1
               move.l    d1,a1
               move.l    a0,d0
               btst.l    #14,d3         ; Test pagesize in TC
               beq       Use4KPages
               moveq     #13,d1
               lsr.l     d1,d0
               and.w     #$1f,d0
               bra       GoOn
Use4KPages
               moveq     #12,d1
               lsr.l     d1,d0
               and.w     #$3f,d0
GoOn
               PRINT_DEB "Pageindex is %08lx",d0
               move.l    (a1,d0.w*4),d1
               btst.l    #0,d1
               bne       PageResident
               btst.l    #1,d1
               beq       InvalidTranslation

               * This is an indirect table entry
               PRINT_DEB "Following indirection, Entry = %08lx",d1
               bclr.l    #1,d1
               move.l    d1,a1
               move.l    (a1),d1
PageResident               
               bclr.l    #3,d1                    ; delete used bit to mark it
                                                  ; as not transparently translated
               move.l    d1,d0
               PRINT_DEB "Returning %08lx",d0
               movem.l   (sp)+,d2-d6
               rts

InvalidTranslation
               PRINT_DEB "Returning invalid descriptor"
               moveq     #0,d0
               movem.l   (sp)+,d2-d6
               rts

MMUTurnedOff:  PRINT_DEB "GenDescr40: MMU turned off"
               move.l    a0,d0
               and.w     #~(PAGESIZE-1),d0
               and.w     #$0320,d3                ; mask DCO and DWO in TC
               lsr.w     #3,d3                    ; shift to same position as 
                                                  ; in page-descriptor
               bset.l    #0,d3                    ; resident
               or.w      d3,d0
               movem.l   (sp)+,d2-d6
               rts

******************************************************************************

_GetPageSize40:
               movem.l   a5-a6,-(sp)
               move.l    4,a6
               lea       GetPageSize40(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               btst.l    #15,d1
               bne       MMUIsOn
               moveq     #0,d0
               rts
MMUIsOn        move.l    #4096,d0
               btst.l    #14,d1
               beq       Pages4K
               add.l     d0,d0
Pages4K        rts

GetPageSize40: movec     tc,d1
               rte
               

***********************************************

_EmptyFunc     * Used as a dummy for the MMU function variables
               rts

***********************************************

               DSEG

MMUStateValid  dc.w      0
PrivateMMUState:
               ds.b      MS40_SIZE

               end
