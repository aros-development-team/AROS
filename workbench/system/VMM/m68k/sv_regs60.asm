               INCLUDE   "exec/funcdef.i"
               INCLUDE   "exec/exec_lib.i"
               INCLUDE   "macros.i"
               INCLUDE   "shared_defs.i"

* $Id: sv_regs60.asm,v 1.3 95/12/16 18:36:08 Martin_Apel Exp $

               XDEF      _Is68060
               XDEF      _CPushP60
               XDEF      _CPushL60

               MACHINE   68060

               SECTION   CODE

_Is68060:      movem.l   a5/a6,-(sp)
               move.l    4,a6
               lea       Check68060(pc),a5
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5-a6
               rts
               
Check68060:    move.w    #$2700,sr            ; disable interrupts
               movec     VBR,a0
               lea       FLineTrap(pc),a1
               moveq     #1,d0
               lea       11*4(a0),a0
               move.l    (a0),d1             ; save old F-line vector
               move.l    a1,(a0)
               cpusha    bc
               plpar     (a0)
               PRINT_DEB "PLPA did not generate exception"
GoOn
               move.l    d1,(a0)
               cpusha    bc
               move.w    #$2000,sr            ; enable interrupts
               rte

FLineTrap:     moveq     #0,d0
               PRINT_DEB "PLPA generated exception"
               lea       GoOn(pc),a1
               move.l    a1,2(sp)
               cpusha    bc
               rte

******************************************************************************

               ALIGN_LONG
_CPushP60:     movem.l   a5/a6,-(sp)
               move.l    12(sp),a0
               lea       CPushP(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               rts

               ALIGN_LONG
CPushP:        movec     cacr,d0
               move.l    d0,d1
               bclr.l    #28,d0         ; invalidate pushed data
               movec     d0,cacr
               CPUSHP    BC,(a0)
               movec     d1,cacr

               rte

******************************************************************************

               ALIGN_LONG
_CPushL60:     movem.l   a5/a6,-(sp)
               move.l    12(sp),a0
               lea       CPushL(pc),a5
               move.l    $4,a6
               jsr       _LVOSupervisor(a6)
               movem.l   (sp)+,a5/a6
               rts

               ALIGN_LONG
CPushL:        movec     cacr,d0
               move.l    d0,d1
               bclr.l    #28,d0         ; invalidate pushed data
               movec     d0,cacr
               CPUSHL    BC,(a0)
               movec     d1,cacr

               rte

               END
