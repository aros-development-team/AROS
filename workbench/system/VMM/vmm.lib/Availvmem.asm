               SECTION   CODE
               XREF      _VMMBase
               XDEF      _AvailVMem

_AvailVMem     move.l    4(sp),d1
               move.l    a6,-(sp)
               move.l    _VMMBase,a6
               jsr       -$2A(a6)
               move.l    (sp)+,a6
               rts

               end
