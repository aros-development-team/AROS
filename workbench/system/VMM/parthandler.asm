                    INCLUDE   "exec/funcdef.i"
                    INCLUDE   "exec/exec_lib.i"
                    INCLUDE   "exec/ables.i"
                    INCLUDE   "exec/ports.i"
                    INCLUDE   "exec/io.i"
                    INCLUDE   "shared_defs.i"
                    INCLUDE   "macros.i"

* $Id: parthandler.asm,v 3.5 95/12/16 18:36:32 Martin_Apel Exp $
                    XDEF      _PartHandler

                    XREF      _OrigBeginIO
                    XREF      _PrePagerTask
                    XREF      _PrePagerPort
                    XREF      _VirtAddrStart
                    XREF      _VirtAddrEnd
                    XREF      _FileHandlerProcess
                    XREF      _CurrentConfig

                    SECTION   CODE

                    * Invoked with
                    * a1: IORequest
                    * a6: DevicePointer

_PartHandler:       * Is io_Data in Virtual memory ?

                    move.l    IO_DATA(a1),d0
                    IN_VM     d0,NoVirtMem

                    PRINT_DEB "BeginIO: IO to virtual memory. Addr: %lx",IO_DATA(a1)

                    * yes it is
                    * Is Prepager requesting this IO ?
                    move.l    4,a0
                    move.l    ThisTask(a0),d0
                    cmp.l     _PrePagerTask,d0
                    beq       PrePagerIO
                    lea       _CurrentConfig,a0
                    tst.w     PageDev(a0)
                    bne       prepage                         ; not PD_FILE
                    cmp.l     _FileHandlerProcess,d0
                    beq       FileHandlerIO

                    * make the application wait for the reply
prepage             move.l    a6,-(sp)
                    bclr.b    #IOB_QUICK,IO_FLAGS(a1)
                    move.l    4,a6
                    move.l    _PrePagerPort,a0
                    jsr       _LVOPutMsg(a6)
                    move.l    (sp)+,a6
                    rts

                    ALIGN_LONG

FileHandlerIO       PRINT_DEB "FileHandlerIO"
PrePagerIO          
NoVirtMem           move.l    _OrigBeginIO,a0
                    jmp       (a0)

                    end
