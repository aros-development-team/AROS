               SECTION   CODE
               XREF      _VMMBase
               XDEF      _AllocVMem

_AllocVMem     movem.l   4(sp),d0-d1
               move.l    a6,-(sp)
               move.l    _VMMBase,a6
               jsr       -$1E(a6)
               move.l    (sp)+,a6
               rts

               end
