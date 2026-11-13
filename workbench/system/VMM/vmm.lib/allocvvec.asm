               SECTION   CODE
               XREF      _VMMBase
               XDEF      _AllocVVec

_AllocVVec     movem.l   4(sp),d0-d1
               move.l    a6,-(sp)
               move.l    _VMMBase,a6
               jsr       -$30(a6)
               move.l    (sp)+,a6
               rts

               end
