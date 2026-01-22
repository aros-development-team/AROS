*
* Copyright (c) 1990 Commodore-Amiga, Inc.
* 
* This example is provided in electronic form by Commodore-Amiga, Inc. for 
* use with the "Amiga ROM Kernel Reference Manual: Devices", 3rd Edition, 
* published by Addison-Wesley (ISBN 0-201-56775-X).
* 
* The "Amiga ROM Kernel Reference Manual: Devices" contains additional 
* information on the correct usage of the techniques and operating system 
* functions presented in these examples.  The source and executable code 
* of these examples may only be distributed in free electronic form, via 
* bulletin board or as part of a fully non-commercial and freely 
* redistributable diskette.  Both the source and executable code (including 
* comments) must be included, without modification, in any copy.  This 
* example may not be published in printed form or distributed with any
* commercial product.  However, the programming techniques and support
* routines set forth in these examples may be used in the development
* of original executable software products for Commodore Amiga computers.
* 
* All other rights reserved.
* 
* This example is provided "as-is" and is subject to change; no
* warranties are made.  All use is at your own risk. No liability or
* responsibility is assumed.
*
**********************************************************************
*
*------ startup.asm  v 36.13
*
**********************************************************************


*
* Copyright (c) 1990 Commodore-Amiga, Inc.
*
*
*------
*------ Conditional assembly flags
*------ ASTART:   1=Standard Globals Defined    0=Reentrant Only
*------ WINDOW:   1=AppWindow for WB startup    0=No AppWindow code
*------ XNIL:     1=Remove startup NIL: init    0=Default Nil: WB Output
*------ NARGS:    1=Argv[0] only                0=Normal cmd line arg parse
*------ DEBUG:    1=Set up old statics for Wack 0=No extra statics
*------ QARG:     1=No argv                     0=Passes argc,argv

* Include the appropriate startup.i (example rstartup.i) here or in
* your assem line

        INCLUDE "allstartup.i"

* Flags for  [A]start  AWstart  Rstart  RWstart  RXstart  QStart
* ASTART         1        1       0        0        0       0
* WINDOW         0        1       0        1        0       0
* XNIL           0        0       0        0        1       1
* NARGS          0        0       0        0        0       0
* DEBUG          0        0       0        0        0       0
* QARG           0        0       0        0        0       1

;------   Flag WB output initialization
NNIL	  SET	(1-XNIL)
WBOUT     SET   (ASTART!WINDOW!NNIL)

************************************************************************
*
*   startup.asm --- Reentrant C Program Startup/Exit (CLI and WB)
*                   v36.13  011/07/90
*
*   Copyright (c) 1988, 1990 Commodore-Amiga, Inc.
*
*   Title to this software and all copies thereof remain vested in the
*   authors indicated in the above copyright notice.  The object version
*   of this code may be used in software for Commodore Amiga computers.
*   All other rights are reserved.
*
*   NO REPRESENTATIONS OR WARRANTIES ARE MADE WITH RESPECT TO THE
*   ACCURACY, RELIABILITY, PERFORMANCE OR OPERATION OF THIS SOFTWARE,
*   AND ALL SUCH USE IS AT YOUR OWN RISK.  NEITHER COMMODORE NOR THE
*   AUTHORS ASSUME ANY RESPONSIBILITY OR LIABILITY WHATSOEVER WITH
*   RESPECT TO YOUR USE OF THIS SOFTWARE.
*
*
*   RSTARTUP.ASM
*
*  Changes for 2.0 - since commands may now receive a >256 character command
*  line, the argv buffer size is now dynamic, based on DosCmdLen passed in.
*  The argv count is also dynamic, based on number of spaces in the command
*  line.
*
*      This startup dynamically allocates a structure which includes
*   the argv buffers.  If you use this startup, your code must return
*   to this startup when it exits.  Use exit(n) or final curly brace
*   (rts) to return here.  Do not use AmigaDOS Exit( ) function.
*   Due to this dynamic allocation and some code consolidation, this
*   startup can make executables several hundred bytes smaller.
*
*       Because a static initialSP variable can not be used, this
*   code depends on the fact that AmigaDOS places the address of
*   the top of our stack in SP and proc->pr_ReturnAddr right before
*   JSR'ing to us.  This code uses pr_ReturnAddr when restoring SP.
*
*       Most versions of startup will initialize a Workbench process's
*   input and output streams (and stdio globals if present) to NIL:
*   if no other form of Workbench output (like WINDOW) is provided.
*   This should help prevent crashes if a user puts an icon on a CLI
*   program, and will also protect against careless stdio debugging
*   or error messages left in a Workbench program.  The code for
*   initializing Workbench IO streams only be removed by assembling
*   startup with ASTART and WINDOW set to 0, and XNIL set to 1.
*
*
*   Some startups which can be conditionally assembled:
*
*      1. Standard Astartup for non-reentrant code
*      2. Reentrant Rstartup (no unshareable globals)
*      3. Smaller reentrant-only RXstartup (no NIL: WB init code)
*      4. Standard AWstartup (WB output window) for non-reentrant code
*      5. Reentrant RWstartup (WB output window, no unshareable globals)
*      6. Smallest Qstartup  (No argv - argv is ptr to NULL string)
*
*
*   Explanation of conditional assembly flags:
*
*      ASTART (ASTART SET 1) startups will set up and XDEF the
*   global variables _stdin, _stdout, _stderr, _errno and  _WBenchMsg.
*   These startups can be used as smaller replacements for startups
*   like (A)startup.obj and TWstartup.obj.  Startups with ASTART
*   would generally be used for non-reentrant programs, although the
*   startup code itself is still reentrant if the globals are not
*   referenced.
*      Reentrant (ASTART SET 0) startups will NOT set up or
*   XDEF the stdio and WBenchMsg globals.  This not only makes the
*   startup slightly smaller, but also lets you know if your code
*   is referencing these non-reentrant globals (you will get an
*   unresolved external reference when you link).  Programs
*   get their input and output handles from Input( ) and Output( ),
*   and the WBenchMsg is passed in argv on Workbench startup.
*
*      WINDOW (WINDOW SET 1) startups use an XREF'd CON: string
*   named AppWindow, defined in your application, to open a stdio
*   console window when your application is started from Workbench.
*   For non-reentrant programs, this window can be used for normal
*   stdio (printf, getchar, etc).  For reentrant programs the window
*   is Input( ) and Output( ).  WINDOW is useful when adding Workbench
*   capability to a stdio application, and also for debugging other
*   Workbench applications.  To insure that applications requiring
*   a window startup are linked with a window startup, the label
*   _NeedWStartup can be externed and referenced in the application
*   so that a linker error will occur if linked with a standard
*   startup.
*
*       example:   /* Optional safety reference to NeedWStartup */
*                    extern UBYTE  NeedWStartup;
*                    UBYTE  *HaveWStartup = &NeedWStartup;
*                  /* Required window specification */
*                    char AppWindow[] = "CON:30/30/200/150/MyProgram";
*                    ( OR  char AppWindow[] = "\0";  for no window )
*
*
*      XNIL (XNIL SET 1) allows the creation of a smaller startup
*   by removing the code that initializes a Workbench process's
*   output streams to NIL:.  This flag can only remove the code
*   if it is not required for ASTART or WINDOW.
*
*      NARGS (NARGS SET 1) removes the code used to parse command line
*   arguments.  The command name is still passed to _main as argv[0].
*   This option can take about 120 bytes off the size of any program that
*   does not use command line args.
*
*      DEBUG (DEBUG SET 1) will cause the old startup.asm statics
*   initialSP, dosCmdLen and dosCmdBuf to be defined and initialized
*   by the startup code, for use as debugging symbols when using Wack.
*
*      QARG (QARG SET TO 1) will bypass all argument parsing.  A CLI
*   startup is passed argc == 1, and a Workbench startup is passed
*   argc == 0.  Argv[0] will be a pointer to a NULL string rather than
*   a pointer to the command name.  This option creates a very small
*   startup with no sVar structure allocation, and therefore must be used
*   with XNIL (it is incompatible with default or AWindow output options).
*
*
*   RULES FOR REENTRANT CODE
*
*      - Make no direct or indirect (printf, etc) references to the
*        globals _stdin, _stdout, _stderr, _errno, or _WBenchMsg.
*
*      - For stdio use either special versions of printf and getchar
*        that use Input( ) and Output( ) rather than _stdin and _stdout,
*        or use fprintf and fgetc with Input( ) and Output( ) file handles.
*
*      - Workbench applications must get the pointer to the WBenchMsg
*        from argv rather than from a global extern WBenchMsg.
*
*      - Use no global or static variables within your code.  Instead,
*        put all former globals in a dynamically allocated structure, and
*        pass around a pointer to that structure.  The only acceptable
*        globals are constants (message strings, etc) and global copies
*        of Library Bases to resolve Amiga.lib references.  Your code
*        must return all OpenLibrary's into non-global variables,
*        copy the result to the global library base only if successful,
*        and use the non-globals when deciding whether to Close any
*        opened libraries.
*
************************************************************************


******* Included Files *************************************************

        INCLUDE "exec/types.i"
        INCLUDE "exec/alerts.i"
        INCLUDE "exec/memory.i"
        INCLUDE "libraries/dos.i"
        INCLUDE "libraries/dosextens.i"
        INCLUDE "workbench/startup.i"


******* MACROS *********************************************************

	CODE

xlib    macro
        xref    _LVO\1
        endm

callsys macro
        CALLLIB _LVO\1
        endm

******* Imported *******************************************************

ABSEXECBASE     EQU     4

        xref    _main           ; C code entry point

        IFGT    WINDOW
        xref    _AppWindow      ; CON: spec in application for WB stdio window
        xdef    _NeedWStartup   ; May be externed and referenced in application
        ENDC    WINDOW

        xlib    Alert
        xlib    AllocMem
        xlib    FindTask
        xlib    Forbid
        xlib    FreeMem
        xlib    GetMsg
        xlib    OpenLibrary
        xlib    CloseLibrary
        xlib    ReplyMsg
        xlib    Wait
        xlib    WaitPort

        xlib    CurrentDir
        xlib    Open
        xlib    Close
        xlib    Input
        xlib    Output

******* Exported *******************************************************

*----- These globals are set up for standard startup code only
        IFGT    ASTART
        xdef    _stdin
        xdef    _stdout
        xdef    _stderr
        xdef    _errno
        xdef    _WBenchMsg
        ENDC    ASTART

*----- These globals available to normal and reentrant code

        xdef    _SysBase
        xdef    _DOSBase
        xdef    _exit           ; standard C exit function


***** Startup Variables structure **********************************

        IFEQ    QARG

; NOTE - the ArgvArray must be last
; It and the ArgvBuffer which follow it are dynamically sized/allocated (2.0)

 STRUCTURE  SVar,0
    ULONG   sv_Size
    LONG    sv_WbOutput
    ULONG   sv_ArgvBufPtr
    ULONG   sv_MaxArgc
    LABEL   sv_ArgvArray
    LABEL   SV_SIZEOF
        ENDC    QARG

************************************************************************
*   Standard Program Entry Point
************************************************************************
*
*       Entered with
*           d0  dosCmdLen
*           a0  dosCmdBuf
*       Any registers (except sp) are allowed to be modified
*
*       Calls
*           main (argc, argv)
*               int   argc;
*               char *argv[];
*
*           For Workbench startup, argc=0, argv=WBenchMsg
*
************************************************************************
startup:
        IFGT    DEBUG
                move.l  sp,initialSP
                move.l  d0,dosCmdLen
                move.l  a0,dosCmdBuf
        ENDC    DEBUG

        IFEQ    QARG
                move.l  d0,d2
                move.l  a0,a2
        ENDC    QARG

        ;------ get Exec library base pointer
                movea.l ABSEXECBASE,a6
                move.l  a6,_SysBase

        ;------ get the address of our task
                suba.l  a1,a1           ; clear a1
                callsys FindTask
                move.l  d0,a4           ; keep task address in a4

        ;------ get DOS library base pointer
                moveq   #0,d0
                lea     DOSName(pc),A1  ; dos.library
                callsys OpenLibrary

                tst.l   d0
                beq     alertDOS        ; fail on null with alert
                move.l  d0,_DOSBase     ; Else set the global


        IFEQ    QARG
        ;------ branch over argv calculations if not a CLI process
                move.l  pr_CLI(A4),d0
		bne.s	vcnt
		moveq	#2,d4		; if wb startup alloc 2 argv's
		moveq   #8,d2		; fake dos cmd line size for alloc
                bra.s   wbnoargs

vcnt:
	;------ estimate max number of argv's for CLI command line
	;------ command + number spaces + 1 + null argv
		movea.l	a2,a0		; dos command line
		moveq	#3,d4		; start count at 3, add spaces
vcnt1:
		cmp.b	#' ',(a0)
		bne.s	vcnt2
		addq	#1,d4
vcnt2:
		tst.b	(a0)+
		bne.s	vcnt1


	;------ branch to here has d4=2, d2=8 for wb startup
wbnoargs:

	;------ d4 now equals at least max number argv's needed
		move.l	d4,d0
		lsl.l	#2,d0		; convert to bytes needed for argv ptrs
		move.l	d0,d5		; save in d5
		add.l	d2,d0		; add dos command line size for buffer

                add.l  #SV_SIZEOF+1,d0	; add structure size + 1 for null term

        ;------ alloc the argument structure	
		move.l	d0,d3		; save total size for de-allocation
                move.l  #(MEMF_PUBLIC!MEMF_CLEAR),d1
                callsys AllocMem
                tst.l   d0
                beq     alertMem        ; fail on null with alert
                move.l  d0,-(sp)        ; save sVar ptr on stack
                move.l  d0,a5           ; sVar ptr to a5
		move.l	d3,sv_Size(a5)  ; size for de-allocation later (2.0)
		subq.l	#1,d4
		move.l	d4,sv_MaxArgc(a5) ; max argc
		lea.l	sv_ArgvArray(a5),a0
		adda.l	d5,a0
		move.l  a0,sv_ArgvBufPtr(a5)

        ENDC    QARG
        IFGT    QARG
                clr.l   -(sp)
        ENDC    QARG

                clr.l   -(sp)           ; reserve space for WBenchMsg if any

        ;------ branch to Workbench startup code if not a CLI process
                move.l  pr_CLI(A4),d0
                beq     fromWorkbench

;=======================================================================
;====== CLI Startup Code ===============================================
;=======================================================================
;       d0  process CLI BPTR (passed in), then temporary
;       d2  dos command length (passed in)
;       d3  argument count
;       a0  temporary
;       a1  argv buffer
;       a2  dos command buffer (passed in)
;       a3  argv array
;       a4  Task (passed in)
;       a5  SVar structure if not QARG (passed in)
;       a6  AbsExecBase (passed in)
;       sp  WBenchMsg (still 0), sVar or 0, then RetAddr (passed in)
;       sp  argc, argv, WBenchMsg, sVar or 0,RetAddr (at bra domain)

        IFEQ    QARG
        ;------ find command name
                lsl.l   #2,d0           ; pr_CLI bcpl pointer conversion
                move.l  d0,a0
                move.l  cli_CommandName(a0),d0
                lsl.l   #2,d0           ; bcpl pointer conversion

                ;-- start argv array
                move.l	sv_ArgvBufPtr(a5),a1
                lea     sv_ArgvArray(a5),a3

                ;-- copy command name
                move.l  d0,a0
                moveq.l #0,d0
                move.b  (a0)+,d0        ; size of command name
                clr.b   0(a0,d0.l)      ; terminate the command name
                move.l  a0,(a3)+
                moveq   #1,d3           ; start counting arguments

        IFEQ    NARGS
        ;------ null terminate the arguments, eat trailing control characters
                lea     0(a2,d2.l),a0
stripjunk:
                cmp.b   #' ',-(a0)
                dbhi    d2,stripjunk

                clr.b   1(a0)

        ;------ start gathering arguments into buffer
newarg:
                ;-- skip spaces
                move.b  (a2)+,d1
                beq.s   parmExit
                cmp.b   #' ',d1
                beq.s   newarg
                cmp.b   #9,d1           ; tab
                beq.s   newarg

                ;-- check for argument count overflow
                cmp.l   sv_MaxArgc(a5),d3
                beq.s   parmExit

                ;-- push address of the next parameter
                move.l  a1,(a3)+
                addq.w  #1,d3

                ;-- process quotes
                cmp.b   #'"',d1
                beq.s   doquote

                ;-- copy the parameter in
                move.b  d1,(a1)+

nextchar:
                ;------ null termination check
                move.b  (a2)+,d1
                beq.s   parmExit
                cmp.b   #' ',d1
                beq.s   endarg

                move.b  d1,(a1)+
                bra.s   nextchar

endarg:
                clr.b   (a1)+
                bra.s   newarg

doquote:
        ;------ process quoted strings
                move.b  (a2)+,d1
                beq.s   parmExit
                cmp.b   #'"',d1
                beq.s   endarg

                ;-- '*' is the BCPL escape character
                cmp.b   #'*',d1
                bne.s   addquotechar

                move.b  (a2)+,d1
                move.b  d1,d2
                and.b   #$df,d2         ;d2 is temp toupper'd d1

                cmp.b   #'N',d2         ;check for dos newline char
                bne.s   checkEscape

                ;--     got a *N -- turn into a newline
                moveq   #10,d1
                bra.s   addquotechar

checkEscape:
                cmp.b   #'E',d2
                bne.s   addquotechar

                ;--     got a *E -- turn into a escape
                moveq   #27,d1

addquotechar:
                move.b  d1,(a1)+
                bra.s   doquote

parmExit:
        ;------ all done -- null terminate the arguments
                clr.b   (a1)
                clr.l   (a3)
        ENDC NARGS

                pea     sv_ArgvArray(a5) ; argv
                move.l  d3,-(sp)         ; argc
        ENDC    QARG

        IFGT    QARG
                pea     nullArgV(pc)    ; pointer to pointer to null string
                pea     1               ; only one pointer
        ENDC

        IFGT    ASTART
                movea.l _DOSBase,a6
        ;------ get standard input handle:
                callsys Input
                move.l  d0,_stdin

        ;------ get standard output handle:
                callsys Output
                move.l  d0,_stdout
                move.l  d0,_stderr
                movea.l ABSEXECBASE,a6
        ENDC ASTART

                bra     domain


;=======================================================================
;====== Workbench Startup Code =========================================
;=======================================================================
;       a2  WBenchMsg
;       a4  Task (passed in)
;       a5  SVar structure if not QARG (passed in)
;       a6  AbsExecBase (passed in)
;       sp  WBenchMsg (still 0), sVar or 0, then RetAddr (passed in)
;       sp  argc=0,argv=WBenchMsg,WBenchMsg,sVar or 0,RetAddr (at domain)

fromWorkbench:
        ;------ get the startup message that workbench will send to us.
        ;       must get this message before doing any DOS calls
                bsr.s   getWbMsg

        ;------ save the message so we can return it later
                move.l  d0,(sp)
        IFGT    ASTART
                move.l  d0,_WBenchMsg
        ENDC    ASTART

        ;------ push the message on the stack for wbmain (as argv)
                move.l  d0,-(sp)
                clr.l   -(sp)           ; indicate run from Workbench (argc=0)

        IFNE    (1-QARG)+WBOUT
        ;------ put DOSBase in a6 for next few calls
                move.l  _DOSBase,a6
        ENDC    (1-QARG)+WBOUT

        IFEQ    QARG
        ;------ get the first argument
                move.l  d0,a2
                move.l  sm_ArgList(a2),d0
                beq.s   doCons

        ;------ and set the current directory to the same directory
                move.l  d0,a0
                move.l  wa_Lock(a0),d1
                ;should be a  beq.s doCons  here
                callsys CurrentDir
doCons:
        ENDC    QARG

        IFGT    WBOUT

        ;------ Open NIL: or AppWindow for WB Input()/Output() handle
        ;       Also for possible initialization of stdio globals
        ;       Stdio used to be initialized to -1


        IFGT    WINDOW
        ;------ Get AppWindow defined in application
                lea     _AppWindow,a0
                cmp.b   #0,(a0)
                bne.s   doOpen          ; Open if not null string
        ENDC    WINDOW

        ;------ Open NIL: if no window provided
        lea     NilName(PC),a0

doOpen:
        ;------ Open up the file whose name is in a0
        ;       DOSBase still in a6
                move.l  a0,d1
                move.l  #MODE_OLDFILE,d2
                callsys Open
        ;------ d0 now contains handle for Workbench Output
        ;------ save handle for closing on exit
                move.l  d0,sv_WbOutput(a5)
                bne.s   gotOpen
                moveq.l #RETURN_FAIL,d2
                bra     exit2
gotOpen:
        IFGT ASTART
        ;------ set the C input and output descriptors
                move.l  d0,_stdin
                move.l  d0,_stdout
                move.l  d0,_stderr
        ENDC ASTART

        ;------ set the console task (so Open( "*", mode ) will work
        ;       task pointer still in A4
                move.l  d0,pr_CIS(A4)
                move.l  d0,pr_COS(A4)
                lsl.l   #2,d0
                move.l  d0,a0
                move.l  fh_Type(a0),d0
                beq.s   noConTask
                move.l  d0,pr_ConsoleTask(A4)
noConTask:
        ENDC WBOUT

        ;------ Fall though to common WB/CLI code


****************************************************
** This code now used by both CLI and WB startup  **
****************************************************

domain:
                jsr     _main
        ;------ main didn't use exit(n) so provide success return code
                moveq.l #RETURN_OK,d2
                bra.s   exit2


****************************************************
**    subroutines here to allow short branches    **
****************************************************

getWbMsg:
        ;------ a6 = ExecBase
                lea     pr_MsgPort(A4),a0       ; our process base
                callsys WaitPort
                lea     pr_MsgPort(A4),a0       ; our process base
                callsys GetMsg
                rts

****************************************************

alertDOS:
        ;------ do recoverable alert for no DOS and exit
                ALERT   (AG_OpenLib!AO_DOSLib)

        ;------ do recoverable alert for no memory and exit
        ;------ If we got this far, DOS is open, so close it
        IFEQ QARG
                bra.s   failExit
alertMem:
                movea.l _DOSBase,a1
                callsys CloseLibrary
                ALERT   AG_NoMemory
        ENDC QARG
failExit:
                tst.l   pr_CLI(a4)
                bne.s   fail2
                bsr.s   getWbMsg
                movea.l d0,a2
                bsr.s   repWbMsg
fail2:
                moveq.l #RETURN_FAIL,d0
                rts

****************************************************

repWbMsg:
        ;------ return the startup message to our parent
        ;       a6 = ExecBase (passed)
        ;       a2 = WBenchMsg (passed)
        ;       we forbid so workbench can't UnLoadSeg() us before we are done
                callsys Forbid
                move.l  a2,a1
                callsys ReplyMsg
                rts


*******************************************************
**  C Program exit() Function, return code on stack  **
**                                                   **
**  pr_ReturnAddr points to our RTS addr on stack    **
**  and we use this to calculate our stack ptr:      **
**                                                   **
**      SP ->   WBenchMsg or 0 (CLI)                 **
**              sVar ptr or 0 (QARG)                 **
**              Address for RTS to DOS               **
*******************************************************

_exit:
                move.l  4(sp),d2        ; exit(n) return code to d2

exit2:                                  ;exit code in d2
        ;------ restore initial stack ptr
                ;-- FindTask
                movea.l ABSEXECBASE,a6
                suba.l  a1,a1
                callsys FindTask
                ;-- get SP as it was prior to DOS's jsr to us
                move.l  d0,a4
                move.l  pr_ReturnAddr(a4),a5
                ;-- subtract 4 for return address, 4 for SVar, 4 for WBenchMsg
                suba.l  #12,a5

                ;-- restore sp
                move.l  a5,sp

                ;-- recover WBenchMsg
                move.l  (sp)+,a2
                ;-- recover SVar
                move.l  (sp)+,a5


        IFGT    WBOUT
        ;------ Close any WbOutput file before closing dos.library
                move.l  sv_WbOutput(a5),d1
                beq.s   noWbOut
                move.l  _DOSBase,a6
                callsys Close
noWbOut:
        ;------ Restore a6 = ExecBase
                movea.l ABSEXECBASE,a6
        ENDC    WBOUT

        ;------ Close DOS library, if we got here it was opened
        ;       SysBase still in a6
                movea.l _DOSBase,a1
                callsys CloseLibrary

        ;------ if we ran from CLI, skip workbench reply
checkWB:
                move.l  a2,d0
                beq.s   deallocSV

                bsr.s   repWbMsg

deallocSV:
        IFEQ    QARG
        ;------ deallocate the SVar structure
                move.l  a5,a1
                move.l  sv_Size(a5),d0
                callsys FreeMem
        ENDC    QARG

        ;------ this rts sends us back to DOS:
                move.l  d2,d0
                rts


**********************************************************************

;----- PC relative data

DOSName         DOSNAME
NilName         dc.b    'NIL:',0
        IFGT    QARG
		CNOP	0,2
nullArgV        dc.l    nullArg
nullArg         dc.l    0               ; "" & the null entry after nullArgV
        ENDC

**********************************************************************
		DATA
**********************************************************************

_SysBase        dc.l    0
_DOSBase        dc.l    0

        IFGT    ASTART
_WBenchMsg      dc.l    0
_stdin          dc.l    0
_stdout         dc.l    0
_stderr         dc.l    0
_errno          dc.l    0
        ENDC    ASTART

        IFGT    DEBUG
initialSP       dc.l    0
dosCmdLen       dc.l    0
dosCmdBuf       dc.l    0
        ENDC    DEBUG

VerRev          dc.w    36,13
        IFGT    ASTART
                dc.b    'A'
        ENDC    ASTART
        IFEQ    ASTART
                dc.b    'R'
        ENDC    ASTART
        IFGT    WINDOW
_NeedWStartup:
                dc.b    'W'
        ENDC    WINDOW
        IFEQ    WBOUT
                dc.b    'X'
        ENDC    WBOUT
        IFGT    NARGS
                dc.b    'N'
        ENDC    NARGS
        IFGT    DEBUG
                dc.b    'D'
        ENDC    DEBUG
        IFGT    QARG
                dc.b    'Q'
        ENDC    QARG

        END
