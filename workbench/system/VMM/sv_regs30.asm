               INCLUDE   "exec/funcdef.i"
               INCLUDE   "exec/exec_lib.i"
               INCLUDE   "macros.i"
               INCLUDE   "shared_defs.i"

* $Id: sv_regs30.asm,v 3.5 95/12/16 18:36:06 Martin_Apel Exp $

               XDEF      _PFlushA30
               XDEF      _PFlushP30

               XDEF      _SetMMUState30
               XDEF      _ReadMMUState30
               XDEF      _SaveMMUState30
               XDEF      _RestoreMMUState30
               XDEF      _GetPageSize30

               XDEF      _GenDescr30

               XDEF      _ColdRebootPatch
               XREF      _OrigColdReboot

               XDEF      _ColdRebootPatchStart
               XDEF      _ColdRebootPatchEnd
               XDEF      _PatchLoc4          ; for ColdRebootPatch
                                             ; after VMM exits
               * A few defines for the MMU registers
TCB_E          EQU       31                  ; Translation control: enable
TCF_E          EQU       (1<<31)
TTB_E          EQU       15                  ; Transparent translation: enable
TTF_E          EQU       (1<<15)
TTB_CI         EQU       10                  ; Transparent translation: cache inhibit
TTF_CI         EQU       (1<<10)
TTF_RW_STAT    EQU       ((1<<9)|(1<<8))
SRB_I          EQU       10                  ; MMUSR: Invalid
SRF_I          EQU       (1<<10)
SRB_T          EQU       6                   ; MMUSR: TT hit
SRF_T          EQU       (1<<6)
PDB_CI         EQU       6                   ; Page descr: cache inhibit
PDF_CI         EQU       (1<<6)
PDB_WP         EQU       2                   ; Page descr: write protected
PDF_WP         EQU       (1<<2)

               MACHINE   68030

               SECTION   CODE

_PFlushA30:    movem.l   a5/a6,-(sp)
               lea       PFlushA30(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               rts

PFlushA30:     pflusha
               rte

***********************************************

               ALIGN_LONG
_PFlushP30:    movem.l   a5/a6,-(sp)
               move.l    12(sp),a0
               lea       PFlushP30(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               rts

               ALIGN_LONG
PFlushP30:     pflush    #0,#0,(a0)
               rte

***********************************************

_SaveMMUState30:
               * Saves the current MMU registers into a private buffer.
               * No inputs, no outputs
               lea       _PrivateMMUState(pc),a0
               move.l    a0,-(sp)
               bsr       _ReadMMUState30
               addq      #4,sp
               lea       _MMUStateValid(pc),a0
               move.w    #1,(a0)
               rts

***********************************************
* The following routines have to stand in this order to copy them
* to a buffer in one go if they should be installed after exit still.

_ColdRebootPatchStart
_ColdRebootPatch:
_PatchLoc4     move.l    _OrigColdReboot,-(sp)    * step into RestoreMMU
               and.b     #$7f,$de0002

***********************************************

_RestoreMMUState30:
               * Restore the MMU registers from the private buffer.
               * No inputs, no outputs

               tst.w     _MMUStateValid(pc)
               beq       Ready
               lea       _PrivateMMUState(pc),a0
               move.l    a0,-(sp)
               bsr       _SetMMUState30
               addq      #4,sp
Ready          rts

***********************************************

_MMUStateValid dc.w       0
_PrivateMMUState:
               ds.b       MS30_SIZE

***********************************************

_SetMMUState30:
               * Called from C as:
               * void SetMMUState30 (MMUState30*)

               movem.l   a5-a6,-(sp)
               move.l    12(sp),a0
               move.l    4,a6
               lea       SetMMUState30(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts

SetMMUState30: lea       -4(sp),a1                ; reserve long-word on stack
               clr.l     (a1)
               pmove     (a1),TC                  ; MMU ausschalten

               pmove     MS30_CRP_HI(a0),CRP
               pmove     MS30_SRP_HI(a0),SRP
               pmove     MS30_TT0(a0),TT0
               pmove     MS30_TT1(a0),TT1
               pmove     MS30_TC(a0),TC           ; MMU einschalten
               PRINT_DEB "MMU registers set"

               rte

_ColdRebootPatchEnd

***********************************************

_ReadMMUState30:
               * Called from C as:
               * void ReadMMUState30 (MMUState30*)

               movem.l   a5-a6,-(sp)
               move.l    12(sp),a0                   ; start address of MMUState
               move.l    4,a6
               lea       ReadMMUState30(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts

ReadMMUState30:
               pmove     TC,MS30_TC(a0)
               pmove     TT0,MS30_TT0(a0)
               pmove     TT1,MS30_TT1(a0)
               pmove     CRP,MS30_CRP_HI(a0)
               pmove     SRP,MS30_SRP_HI(a0)
               rte

***********************************************

_GenDescr30:   * This function generates an MMU page descriptor for
               * a given logical address. It includes all the standard
               * information a page descriptor contains. In addition
               * the U bit is misused to detect a transparent translation
               * register hit. This way there is no need for separate
               * handling of cache modes and such for each processor
               * type.
               * C prototype:
               *  ULONG GenDescr (ULONG LogAddr);

               movem.l   a5-a6,-(sp)
               move.l    4,a6
               move.l    12(sp),a0
               PRINT_DEB "GenDescr30 called for address %08lx",a0
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

               * Check if the address hits into one of the TT registers
               sub.w     #2,sp
               ptestr    #1,(a0),#0
               pmove     mmusr,(sp)
               move.w    (sp)+,d0
               btst.l    #SRB_T,d0
               bne       TTHit

               * Check if there is a valid table mapping for this address
               sub.w     #2,sp
               ptestr    #1,(a0),#7,a1
               pmove     mmusr,(sp)
               move.w    (sp)+,d0
               btst.l    #SRB_I,d0
               bne       InvalidTranslation

               * there is a valid mapping: Extract the physical address,
               * the WP and cache mode.
               sub.w     #4,sp
               pmove     tc,(sp)
               move.l    (sp)+,d1

               movem.l   d2/d3,-(sp)
               and.b     #7,d0          ; mask number of levels
               moveq     #4,d2
               sub.b     d0,d2
               lsl.b     #2,d2          ; * 4
               lsr.l     d2,d1          ; ignore lower TC bits

               moveq     #0,d2          ; number of bits taken
                                        ; from upper part of physical address
count_bits     move.l    d1,d3
               lsr.l     #4,d1
               and.b     #$f,d3
               add.b     d3,d2          ; accumulate number of bits
               subq.b    #1,d0
               bne       count_bits

               sub.b     #32,d2         ; 32 - number of bits
               neg.b     d2
               moveq     #1,d1          ; generate mask
               lsl.l     d2,d1
               subq.l    #1,d1
               not.l     d1

               move.l    (a1),d0        ; take upper x bits from
               and.l     d1,d0          ; virtual address
               not.l     d1
               move.l    a0,d2
               and.l     d2,d1          ; and the rest from the
               or.l      d1,d0          ; logical address
               and.w     #~(PAGESIZE-1),d0
               move.l    (a1),d1
               and.w     #(PDF_CI|PDF_WP),d1      ; don't forget those special
               bset.l    #0,d1                    ; bits
               or.w      d1,d0
               movem.l   (sp)+,d2-d3
               PRINT_DEB "GenDescr30: returns %08lx",d0

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

TTHit:         * This address hits into one of the TT registers.
               * Find out which of the two is hit and return the
               * corresponding cache mode and WP status
               movem.l   d2-d4,-(sp)

               * First read the TT registers
               sub.w     #8,sp
               pmove     tt0,(sp)
               move.l    (sp)+,d0
               pmove     tt1,(sp)
               move.l    (sp)+,d1

               btst.l    #TTB_E,d0
               beq       TT1Hit
               move.l    a0,d4                    ; save address for eor
               move.l    d0,d2                    ; save DTT0 for mask
               move.l    d0,d3                    ; save DTT0 for CM
               eor.l     d4,d0                    ; cmp with TT0
               asl.l     #8,d2                    ; shift mask
               not.l     d2
               and.l     d2,d0                    ; only look at non-masked bits
               and.l     #$ff000000,d0            ; only A31 - A24 valid here
               beq       TT0Hit
TT1Hit:        move.l    d1,d3

TT0Hit:        move.l    a0,d0
               and.w     #~(PAGESIZE-1),d0
               or.w      #$9,d0                   ; mark as resident and TT hit
               btst.l    #TTB_CI,d3
               beq       CachingAllowed
               bset.l    #PDB_CI,d0
CachingAllowed and.w     #TTF_RW_STAT,d3
               bne       NotWP
               bset.l    #PDB_WP,d0
NotWP
               movem.l   (sp)+,d2-d4
               rte

***********************************************

_GetPageSize30:
               movem.l   a5/a6,-(sp)
               lea       GetPageSize30(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               btst.l    #TCB_E,d1
               bne       MMUIsOn
               moveq     #0,d0
               rts
MMUIsOn        moveq     #20,d0
               lsr.l     d0,d1
               and.w     #$f,d1
               moveq     #1,d0
               lsl.l     d1,d0
               rts

GetPageSize30: sub.w     #4,sp
               pmove     tc,(sp)
               move.l    (sp)+,d1
               rte
               
               END
