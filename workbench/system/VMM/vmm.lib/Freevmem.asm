               SECTION   CODE
               XREF      _VMMBase
               XDEF      _FreeVMem

_FreeVMem      move.l    4(sp),a1
               move.l    8(sp),d0

               move.l    a6,-(sp)
               move.l    _VMMBase,a6
               jsr       -$24(a6)
               move.l    (sp)+,a6
               rts

               end
