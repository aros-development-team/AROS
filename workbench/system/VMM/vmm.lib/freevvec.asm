               SECTION   CODE
               XREF      _VMMBase
               XDEF      _FreeVVec

_FreeVVec      move.l    4(sp),a1

               move.l    a6,-(sp)
               move.l    _VMMBase,a6
               jsr       -$36(a6)
               move.l    (sp)+,a6
               rts

               end
