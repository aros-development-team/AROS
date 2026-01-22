               INCLUDE   "exec/types.i"
               INCLUDE   "exec/funcdef.i"
               INCLUDE   "exec/exec_lib.i"
               INCLUDE   "dos/dosextens.i"
               INCLUDE   "dos/dos_lib.i"

               XDEF      _SysBase
               XDEF      _DOSBase
               XDEF      _stdout
               XDEF      _stdin
               XDEF      ___main
               XREF      _main

               SECTION   ,CODE

Start:         move.l    4,a6
               move.l    a6,_SysBase
               lea       dos_name(pc),a1
               moveq     #37,d0
               jsr       _LVOOpenLibrary(a6)
               move.l    d0,_DOSBase
               beq       Error
               suba.l    a1,a1
               jsr       _LVOFindTask(a6)
               move.l    d0,a4
               tst.l     pr_CLI(a4)
               beq       WBStartup

CLIStartup:    move.l    _DOSBase,a6
               jsr       _LVOOutput(a6)
               move.l    d0,_stdout
               jsr       _LVOInput(a6)
               move.l    d0,_stdin
               jsr       _main
               move.l    d0,-(sp)
               move.l    _DOSBase,a1
               move.l    4,a6
               jsr       _LVOCloseLibrary(a6)
               move.l    (sp)+,d0
Error          rts


WBStartup:     * called with
               * A4: Process
               * A6: SysBase
               lea       pr_MsgPort(a4),a0
               jsr       _LVOWaitPort(a6)
               lea       pr_MsgPort(a4),a0
               jsr       _LVOGetMsg(a6)
               move.l    d0,-(sp)            ; save it for later replying
               move.l    _DOSBase,a6
               move.l    #WinName,d1
               move.l    #MODE_NEWFILE,d2
               jsr       _LVOOpen(a6)
               move.l    d0,_WinFile
               beq       ReturnMsg
               move.l    d0,_stdout
               move.l    d0,_stdin
               jsr       _main
               move.l    _WinFile,d1
               move.l    _DOSBase,a6
               jsr       _LVOClose(a6)

ReturnMsg:     move.l    4,a6
               move.l    _DOSBase,a1
               jsr       _LVOCloseLibrary(a6)
               jsr       _LVOForbid(a6)
               move.l    (sp)+,a1
               jmp       _LVOReplyMsg(a6)

___main        rts

dos_name       dc.b      'dos.library',0
WinName        dc.b      'CON:0/0/640/200/CLOSE/AUTO/WAIT/VMM Window',0

               SECTION   ,DATA

_SysBase       ds.l      1
_DOSBase       ds.l      1
_stdout        ds.l      1
_stdin         ds.l      1
_WinFile       ds.l      1

               end
