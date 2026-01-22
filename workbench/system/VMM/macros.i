               XREF           _PrintDebugMsg
               XREF           _PrintCallingTask
               XREF           _CheckMemList
               XREF           _VirtMemSema
               XREF           _OrigAllocMem
               XREF           _OrigFreeMem

**********************************************************************

PRINT_DEB      MACRO
               IFD            DEBUG

               movem.l        d0-d1/a0-a1/a6,-(sp)
               move.l         4,a6

               IFC            '\2',''
               
               clr.l          -(sp)

               ELSE

               move.l         \2,-(sp)

               ENDC

               pea            DebugString\@(pc)
               jsr            _PrintDebugMsg
               add.w          #8,sp
               movem.l        (sp)+,d0-d1/a0-a1/a6
               bra            GoOn\@

DebugString\@  dc.b           \1,0

               ds.w           0
GoOn\@
               ENDC
               ENDM

**********************************************************************

CHECK_CONSISTENCY   MACRO
               IFD            DEBUG
               IFD            TRACE_MEM
               movem.l        d0-d1/a0-a1,-(sp)
               jsr            _CheckMemList
               movem.l        (sp)+,d0-d1/a0-a1
               ENDC
               ENDC
               ENDM

**********************************************************************

PRINT_MEM      MACRO
               IFD            TRACE_MEM
               PRINT_DEB      \1,\2
               ENDC
               CHECK_CONSISTENCY
               ENDM

**********************************************************************

MEM_FORBID     MACRO          * to be called with SysBase in A6
               PRINT_MEM      "Obtaining semaphore"

               lea            _VirtMemSema,a0
               IFD            USE_OWN_SEMAPHORES
               jsr            VMMObtainSemaphore
               ELSE
               jsr            _LVOObtainSemaphore(a6)
               ENDC
               PRINT_MEM      "Semaphore obtained"
               ENDM

MEM_PERMIT     MACRO
               PRINT_MEM      "Releasing semaphore"
               lea            _VirtMemSema,a0
               IFD            USE_OWN_SEMAPHORES
               jsr            VMMReleaseSemaphore
               ELSE
               jsr            _LVOReleaseSemaphore(a6)
               ENDC
               ENDM

**********************************************************************

IN_VM          MACRO  * register, label to branch if out of range
               cmp.l          _VirtAddrEnd,\1
               bhi            \2
               cmp.l          _VirtAddrStart,\1
               bcs            \2
               ENDM

**********************************************************************

GET_SIGNAL     MACRO          * Called with A6 = SysBase
                              * d0-d1,a0-a1 are scratch
                              * upon return d0.b contains the signal number
               moveq          #-1,d0
               jsr            _LVOAllocSignal(a6)
               tst.b          d0
               bge            GotSignal\@

               PRINT_DEB      "Had to use SIGB_SINGLE"
               moveq          #EMERGENCY_SIGNAL,d0
GotSignal\@    
               ENDM

**********************************************************************

RELEASE_SIGNAL MACRO          * Called with A6 = SysBase
                              * d0 = SigBit
                              * d0-d1,a0-a1 are scratch

               cmp.b          #EMERGENCY_SIGNAL,d0
               beq            DontFree\@
               jsr            _LVOFreeSignal(a6)
DontFree\@
               ENDM

**********************************************************************

ALIGN_LONG     MACRO
               cnop           0,4
               ENDM

**********************************************************************

PRINT_RANGE    MACRO
               * a0: start of range, not modified
               * prints 32 longwords starting at a0
               PRINT_DEB      "0000: %lx",$00(a0)
               PRINT_DEB      "0004: %lx",$04(a0)
               PRINT_DEB      "0008: %lx",$08(a0)
               PRINT_DEB      "000c: %lx",$0c(a0)
               PRINT_DEB      "0010: %lx",$10(a0)
               PRINT_DEB      "0014: %lx",$14(a0)
               PRINT_DEB      "0018: %lx",$18(a0)
               PRINT_DEB      "001c: %lx",$1c(a0)
               PRINT_DEB      "0020: %lx",$20(a0)
               PRINT_DEB      "0024: %lx",$24(a0)
               PRINT_DEB      "0028: %lx",$28(a0)
               PRINT_DEB      "002c: %lx",$2c(a0)
               PRINT_DEB      "0030: %lx",$30(a0)
               PRINT_DEB      "0034: %lx",$34(a0)
               PRINT_DEB      "0038: %lx",$38(a0)
               PRINT_DEB      "003c: %lx",$3c(a0)
               PRINT_DEB      "0040: %lx",$40(a0)
               PRINT_DEB      "0044: %lx",$44(a0)
               PRINT_DEB      "0048: %lx",$48(a0)
               PRINT_DEB      "004c: %lx",$4c(a0)
               PRINT_DEB      "0050: %lx",$50(a0)
               PRINT_DEB      "0054: %lx",$54(a0)
               PRINT_DEB      "0058: %lx",$58(a0)
               PRINT_DEB      "005c: %lx",$5c(a0)
               PRINT_DEB      "0060: %lx",$60(a0)
               PRINT_DEB      "0064: %lx",$64(a0)
               PRINT_DEB      "0068: %lx",$68(a0)
               PRINT_DEB      "006c: %lx",$6c(a0)
               PRINT_DEB      "0070: %lx",$70(a0)
               PRINT_DEB      "0074: %lx",$74(a0)
               PRINT_DEB      "0078: %lx",$78(a0)
               PRINT_DEB      "007c: %lx",$7c(a0)
               ENDM
