                    INCLUDE   "exec/types.i"
                    INCLUDE   "exec/memory.i"
                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/exec_lib.i"
                    INCLUDe   "macros.i"

                    XDEF      _SetWindowTitlesPatch
                    XREF      _OrigSetWindowTitles
                    XREF      _strncmp
                    XREF      _sprintf
                    XREF      _VirtMem

                    MACHINE   68030

                    SECTION   CODE

_SetWindowTitlesPatch:
                    * called with:
                    * A0: Window
                    * A1: WindowTitle
                    * A2: ScreenTitle

                    tst.l     a2       
                    beq       CallOrigFunc2       ; check for 0 and -1
                    moveq     #-1,d0              ; otherwise Enforcer hits
                    cmp.l     d0,a2               ; would result
                    beq       CallOrigFunc2

                    movem.l   a0-a1/a3,-(sp)
                    lea       WBString1(pc),a0
                    pea       15                  ; length of WBString1
                    pea       (a0)
                    pea       (a2)
                    move.l    a0,a3               ; remember cmp string
                    jsr       _strncmp
                    add.w     #3*4,sp
                    tst.l     d0
                    beq       DoPatch

                    lea       WBString2(pc),a0
                    pea       11                  ; length of WBString2
                    pea       (a0)
                    pea       (a2)
                    move.l    a0,a3               ; remember cmp string
                    jsr       _strncmp
                    add.w     #3*4,sp
                    tst.l     d0
                    bne       CallOrigFunc1

* OK, the string is found produce a new one stating the amount of
* all memory (including virtual)

DoPatch:            PRINT_DEB "Patching screen title"
                    move.l    a6,-(sp)
                    move.l    4,a6
                    move.l    #MEMF_PUBLIC+MEMF_FAST,d1     ; Determine FAST PUBLIC mem
                    jsr       _LVOAvailMem(a6)
                    asr.l     #8,d0                         ; divide by 1024
                    asr.l     #2,d0
                    move.l    d0,-(sp)
                    move.l    _VirtMem,a0
                    move.l    MH_FREE(a0),d0                ; Determine VM
                    asr.l     #8,d0                         ; divide by 1024
                    asr.l     #2,d0
                    move.l    d0,-(sp)
                    move.l    #MEMF_PUBLIC+MEMF_CHIP,d1
                    jsr       _LVOAvailMem(a6)              ; Determine CHIP mem
                    asr.l     #8,d0                         ; divide by 1024
                    asr.l     #2,d0
                    move.l    d0,-(sp)
                    move.l    a3,-(sp)
                    lea       tmp_buffer(pc),a2
                    pea       PatchString(pc)
                    pea       (a2)
                    jsr       _sprintf
                    add.w     #6*4,sp
                    move.l    (sp)+,a6
                    PRINT_DEB "Done creating patched string"

                    * Fall through to calling original function

CallOrigFunc1       movem.l   (sp)+,a0-a1/a3
CallOrigFunc2       move.l    _OrigSetWindowTitles,-(sp)
                    rts

WBString1:          dc.b      "Amiga Workbench",0
WBString2:          dc.b      "AmigaOS 3.1",0
PatchString         dc.b      "%-15s     %ld KB Chip   %ld KB VM   %ld KB Public Fast",0
tmp_buffer          ds.b      150
