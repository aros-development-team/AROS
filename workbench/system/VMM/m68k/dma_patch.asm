                    INCLUDE   "shared_defs.i"
                    INCLUDE   "macros.i"
                    INCLUDE   "exec/execbase.i"
                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/exec_lib.i"

                    XDEF      _CachePreDMAPatch
                    XDEF      _CachePostDMAPatch
                    XREF      _OrigCachePreDMA
                    XREF      _OrigCachePostDMA

                    XREF      _VirtAddrStart
                    XREF      _VirtAddrEnd

                    XREF      _GenDescr
                    XREF      _CPushP
                    XREF      _FlushVirt

                    IFD       DEBUG
                    XDEF      _OpenPatch
                    XREF      _OrigOpen
                    ENDC


_CachePreDMAPatch:  * inputs:
                    *   a0: APTR  vaddr,
                    *   a1: LONG *length
                    *   d0: ULONG flags
                    * outputs:
                    *   d0: APTR paddr
                    *   a1: LONG *length (modified)

                    IN_VM     a0,PreDMANotInVM

                    PRINT_DEB "CachePreDMA in VM. Addr = %lx",a0
                    PRINT_DEB "                   Length = %lx",(a1)
                    move.b    (a0),d1        ; touch vaddr, so we are sure
                                             ; that the page is in
                    movem.l   a0-a1,-(sp)
                    move.l    a0,-(sp)

                    move.l    _GenDescr,a0
                    jsr       (a0)

                    add.w     #4,sp
                    movem.l   (sp)+,a0-a1
                    move.l    a0,d1
                    add.l     #PAGESIZE,d1
                    and.w     #~(PAGESIZE-1),d1
                    sub.l     a0,d1
                    cmp.l     (a1),d1
                    bgt       FullLength
                    move.l    d1,(a1)
FullLength:
                    move.l    d0,-(sp)
                    move.l    _CPushP,a0
                    jsr       (a0)
                    move.l    (sp)+,d0
                    rts

PreDMANotInVM       move.l    _OrigCachePreDMA,-(sp)
                    rts


_CachePostDMAPatch: * inputs:
                    *   a0: APTR  vaddr,
                    *   a1: LONG *length
                    *   d0: ULONG flags
                    * outputs:
                    *   none

                    IN_VM     a0,PostDMANotInVM

                    PRINT_DEB "CachePostDMA in VM. Addr = %lx",a0
                    PRINT_DEB "                    Length = %lx",(a1)

                    and.b     #(DMAF_NoModify+DMAF_ReadFromRAM),d0

                    bne       PostDMAReady
                    move.l    _FlushVirt,a0
                    jmp       (a0)

PostDMANotInVM      move.l    _OrigCachePostDMA,-(sp)
PostDMAReady        rts


                    IFD       DEBUG

_OpenPatch          PRINT_DEB "Opening file"
                    move.l    d1,-(sp)
                    move.l    #0,-(sp)
                    move.l    d1,-(sp)
                    jsr       _PrintDebugMsg
                    add.w     #8,sp
                    move.l    (sp)+,d1
                    move.l    _OrigOpen,a0
                    jmp       (a0)

                    ENDC

                    end
