/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCPL stubs that call into Exec and Dos libraries.
    Lang: english
*/
#define DEBUG 1
#include <aros/m68k/asm.h>

#include "bcpl.h"

    /* BCPL equivalents of JSR and RTS */

    .text
    .balign 2
    .globl  BCPL_jsr  /* A5 */
BCPL_jsr:
    move.l  %sp@+,%a3           /* Get return address from C stack */
    movem.l %a1/%a3-%a4,-12(%a1,%d0.l)
    adda.l  %d0,%a1             /* Move to next BCPL FP */
    movem.l %d1-%d4,(%a1)       /* Save argument on BCPL FP */
    jmp     (%a4)
    bra.s   BCPL_jsr_mult       /* This MUST be at BCPL_jsr+0x10 */
    bra.s   BCPL_jsr_divmod     /* This MUST be at BCPL_jsr+0x12 */
BCPL_jsr_mult:
    muls    %d2,%d1
    rts
BCPL_jsr_divmod:
    divs    %d2,%d1             /* Upper D1 is D1 % D2.w, lower is D1 / D2.w */
    move.l  %d1,%d2
    ext.w   %d1                 /* D1 = D1 / D2.w */
    asr.l   #8,%d2              /* D2 = D1 % D2.w */
    asr.l   #8,%d2              /* D2 = D1 % D2.w */
    rts

    .globl  BCPL_rts  /* A6 */
BCPL_rts:
    movem.l -12(%a1),%a1/%a3   /* Restore old BCPL FP and return address */
    move.l  -4(%a1),%a4        /* Get old A4 */
    jmp     (%a3)              /* Go back */


/* BCPL macros
 */
    .section .rodata
.LBCPL_format_trace:
    .string  "BCPL %s, D1=%p D2=%p D3=%p D4=%p, GV %p\n"
.LBCPL_format_ret:
    .string  "\tD1=%p\n"

    /* For dummy functions */
    .macro  BCPLd name
    .section .rodata
.LBCPL_func_\name:
    .string "\name"
    .text
    .balign 4
    .globl  BCPL_\name
BCPL_\name:
    movem.l %d1/%a0-%a1,%sp@-
    move.l  %a2,%sp@-
    move.l  %d4,%sp@-
    move.l  %d3,%sp@-
    move.l  %d2,%sp@-
    move.l  %d1,%sp@-
    move.l  #.LBCPL_func_\name,%sp@-
    move.l  #.LBCPL_format_trace,%sp@-
    jsr     kprintf
    lea.l   %sp@(7*4),%sp
    movem.l %sp@+,%d1/%a0-%a1
    clr.l   %d0		/* DOSFALSE */
    .endm

    .macro  BRTSd
    movem.l %d0/%a0-%a1,%sp@-
    move.l  %d0,%sp@-
    move.l  #.LBCPL_format_ret,%sp@-
    jsr     kprintf
    addq.l  #8,%sp
    movem.l %sp@+,%d0/%a0-%a1
    move.l  %d0,%d1
    jmp     (%a6)
    .endm

    /* For real functions */
    .macro  BCPL name
    .balign 4
    .globl  BCPL_\name
BCPL_\name:
    .endm

    .macro  BRTS
    move.l  %d0,%d1
    jmp     (%a6)
    .endm

#ifdef DEBUG
#define BCPL BCPLd
#define BRTS BRTSd
#endif

    /* Global vector table entries */
BCPLd sysRequest     /* -84, void, @line1, @line2, @line3 */
    BRTS

BCPL toCStr          /* -80, char *, @string */
    move.l  %d1,%d0
    lsl.l   #2,%d0
    addq.l  #1,%d0
    BRTS

BCPL toBSTR         /* -7c, void, &src, @dest */
    movem.l	%d1-%d2,%sp@-
    lsl.l	#2,%d2
    move.l	%d2,%d0
    addq.l	#1,%d2
    clr.b	%a0@(%d0)
0:
    move.b	%a0@(%d2),%a0@(%d1)
    tst.b	%a0@(%d1)
    beq		1f
    addq.l	#1,%d1
    addq.l	#1,%d2
    addq.l	#2,%a0@(%d0)
    jmp		0b
1:
    movem.l	%sp@+,%d1-%d2
    BRTS

BCPL Execute        /* -6c, BOOL, &command, @input, @output */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(37 * -6)	/* DOS/Execute() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL IsInteractive  /* -68, BOOL, @filehandle */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(36 * -6)	/* DOS/IsInteractive() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL DateStamp      /* -64, timeval *, &time */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(32 * -6)	/* DOS/DateStamp() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL SetProtection  /* -60, BOOL, &name, mask */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(31 * -6)	/* DOS/SetProtection() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL SetComment     /* -5c, BOOL, &name, &comment */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(30 * -6)	/* DOS/SetComment() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd QueuePacket    /* -54, void */
    BRTS

BCPL clearmem       /* -50, void, @buff, lw_length */
    move.l	%d2,%sp@-
    tst.l	%d2
    ble		1f
    lsl.l	#2,%d1
0:
    clr.l	%a0@(%d1)
    addq.l	#4,%d1
    dbf		%d2,0b
1:
    BRTS

BCPL LoadSeg        /* -4c, SegList ^, &name */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(25 * -6)	/* DOS/LoadSeg() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL CreateProc     /* -48, MessagePort *, &name, pri, seg, stackbytes */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(23 * -6)	/* DOS/CreateProc() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL IoErr          /* -44, long */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(22 * -6)	/* DOS/IoErr() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL CurrentDir     /* -40, FileLock ^, @FileLock */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(21 * -6)	/* DOS/CurrentDir() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL CreateDir      /* -3c, FileLock ^, &name */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(20 * -6)	/* DOS/CreateDir() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Info           /* -38, BOOL, @FileLock, &InfoData */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(19 * -6)	/* DOS/Info() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL ExNext         /* -34, BOOL, @FileLock, &FileInfo */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(18 * -6)	/* DOS/ExNext() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Examine        /* -30, BOOL, @FileLock, &FileInfo */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(17 * -6)	/* DOS/Examine() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Lock           /* -2c, FileLock ^, &name, mode */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(14 * -6)	/* DOS/Lock() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Rename         /* -28, BOOL, &oldname, &newname */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(13 * -6)	/* DOS/Rename() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL DeleteFile     /* -24, BOOL, &name */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(12 * -6)	/* DOS/DeleteFile() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Seek           /* -20, LONG, @FileHandle, pos, mode */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(11 * -6)	/* DOS/Seek() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Write          /* -18, LONG, Write, @FileHandle, &buf, len */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(8 * -6)	/* DOS/Write() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Read           /* -0c, LONG, Read, @FileHandle, &buf, len */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(7 * -6)	/* DOS/Read() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL strncpy        /* -08, void, @src, @dest, n */
    movem.l	%d2-%d5,%sp@-
    lsl.l	#2,%d1
    lsl.l	#2,%d2
    clr.l	%d4
    clr.l	%d5
    move.b	%a0@(%d1),%d4
    move.b	%a0@(%d2),%d5
    cmp.l	%d5,%d3		/* Dest already bigger than N? Exit. */
    ble		2f
    add.l	%d5,%d4		/* D4 is strlen(src) + strlen(dest) */
    cmp.l	%d4,%d3		/* Total > N? */
    bge		0f
    move.l	%d3,%d4		/* Force N as the max length */
0:
    sub.l	%d5,%d4		/* D4 -= strlen(dest) */
    ble		2f
    add.l	%d2,%d5		/* Set D5 as the end of dest */
1:
    move.b	%a0@(%d1),%a0@(%d5)
    addq.l	#1,%d1
    addq.l	#1,%d5
    dbf		%d4,1b
    sub.l	%d2,%d5
    move.b	%d5,%a0@(-1,%d2)	/* Fix up dest len */
2:
    movem.l	%sp@+,%d2-%d5
    BRTS

BCPL Open           /* -04, FileHandle ^, &name, mode */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(5 * -6)	/* DOS/Open() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

                     /* 4, void main */

BCPL Exit            /*  08, void, code */
    move.l	SysBase,%a6
    sub.l	%a1,%a1
    move.l	%d1,%d2
    jsr		%a6@(FindTask)
    move.l	%d0,%a0
    move.l	%a0@(pr_ReturnAddr),%a0
    lea.l	%a0@(-4),%sp
    move.l	%d2,%d0
    rts

BCPL multiply       /*  0c, D1 = D1 * D2 */
    muls	%d2, %d1
    BRTS

BCPL divide         /*  10, D1 = D1 / D2 */
    divs	%d2, %d1
    ext.w	%d1
    BRTS

BCPL mod            /*  14, D1 = D1 % D2 */
    divs	%d2, %d1
    asr.l	#8, %d1
    asr.l	#8, %d1
    BRTS

BCPLd setIO          /*  18, void, @IOStdReq, command, &data, len, offset */
    BRTS

BCPLd packlw         /*  20, ?? */
    BRTS

BCPLd unpacklw       /*  24, ?? */
    BRTS

BCPLd Res2           /*  28, LONG, is_set_flag, result_code */
    BRTS

                     /*  30, BCPL_FrameLower */

BCPLd makeGVarea     /*  34, void, @buff */
    BRTS

BCPL findtask        /*  38, Process *, */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    sub.l	%a1,%a1
    jsr		%a6@(FindTask)
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd getbyte        /*  3c, BYTE, @lw_addr, bytenum */
    BRTS

BCPLd putbyte        /*  40, BYTE, @lw_addr, bytenum, byte */
    BRTS

BCPL level          /*  44, LONG (or BPTR?) */
    move.l  %d0, %a1@(-16)  /* Return caller's BCPL frame */
    BRTS

BCPLd longcall       /*  48, LONG, level, label */
    BRTS

BCPLd allocMem       /*  4c, BPTR, lwlength, reqs */
    BRTS

BCPLd longjump       /*  50, LONG, label */
    BRTS

BCPLd doIO           /*  54, LONG, @IORequest */
    BRTS

BCPLd sendIO         /*  58, void, @IORequest */
    BRTS

    /* Coroutine support:
     * struct coroutine {
     *      BPTR colist;
     *      BPTR parent;
     *      APTR frame;
     *      APTR procedure;
     */
BCPLd createco       /*  5c, void, @coroutine, lwlength */
    BRTS

BCPLd deleteco       /*  60, void, @coroutine */
    BRTS

BCPLd callco         /*  64, void, @coroutine, arg */
    BRTS

BCPLd cowait         /*  68, void, result */
    BRTS

BCPLd resumeco       /*  6c, void, @coroutine */
    BRTS

BCPL installSeg     /*  70, BOOL, @memseg_BCPL */
    movem.l	%a0-%a1,%sp@-
    move.l	%a2,%sp@-
    move.l	%d1,%sp@-
    jsr		BCPL_InstallSeg
    addq.l	#8,%sp
    movem.l	%sp@+,%a0-%a1
    BRTS

BCPL getvec         /*  74, LONG ^, lw_length */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    move.l	%d1,%d0
    addq.l	#4,%d0         /* 4 additional words for padding */
    lsl.l	#2,%d0
    clr.l	%d1
    bset.l	#16,%d1        /* MEMF_CLEAR */
    jsr		%a6@(114 * -6) /* Exec/AllocVec() */
    lsr.l	#2,%d0
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL freevec        /*  78, void, @vec */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    lsl.l	#2,%d1
    move.l	%d1,%a1
    jsr		%a6@(115 * -6) /* Exec/FreeVec() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL openDevice     /*  7c, BOOL, @IORequest, @name, unit, flags */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    lsl.l	#2,%d1
    lsl.l	#2,%d2
    addq.l	#1,%d2
    move.l	%d2,%a0
    move.l	%d1,%a1
    move.l	%d4,%d1
    move.l	%d3,%d0
    jsr		%a6@(74 * -6)	/* Exec/OpenDevice() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL closeDevice    /*  80, void, @IORequest */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    lsl.l	#2,%d1
    move.l	%d1,%a1
    jsr		%a6@(75 * -6)	/* Exec/CloseDevice() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL createproc     /*  84, MessagePort *, @segarray, lw_stack_size, pri, @name */
    movem.l	%d1-%d4/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    lsl.l	#2,%d4
    addq.l	#1,%d4
    move.l	%d4,%d0
    move.l	%d2,%d4
    move.l	%d3,%d2
    move.l	%d1,%d3
    move.l	%d0,%d1
    jsr		%a6@(23 * -6)	/* DOS/CreateProc() */
    movem.l	%sp@+,%d1-%d4/%a0-%a1/%a6
    BRTS

BCPL remProcess     /*  88, void */
    jmp BCPL_Exit	/* TODO: Review if this is correct */

BCPL ParentDir      /*  8c, FileLock ^, @FileLock */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(35 * -6)	/* DOS/ParentDir() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL setSignals     /*  90, void, &MsgPort, mask */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    add.l	#mp_SigTask,%d1
    move.l	%a0@(%d1),%a1
    move.l	%d2,%d0
    jsr		%a6@(54 * -6)	/* Signal */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL clearSignals   /* 94, BOOL , mask */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    clr.l	%d0
    jsr		%a6@(51 * -6)	/* SetSignal */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL DOSAlert       /* 98, void, alertnumber */
    movem.l	%d7/%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    move.l	%d1,%d7
    jsr		%a6@(18 * -6)	/* Exec/Alert() */
    movem.l	%sp@+,%d7/%a0-%a1/%a6
    BRTS

BCPL findRootNode   /* 9c, RootNode @,  */
    move.l	%a2@(BCPL_DOSBase),%d0
    add.l	#dl_Root,%d0
    move.l	%a0@(%d0),%d0	/* %a0 is always 0 in BCPL */
    lsr.l	#2,%d0
    BRTS

BCPL readinput      /* a0, bytelength , &buf, bytelength */
    movem.l	%d1-%d3/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    move.l	%d2,%d3
    move.l	%d1,%d2
    jsr		%a6@(9 * -6)	/* DOS/Input() */
    move.l	%d0,%d1
    jsr		%a6@(7 * -6)	/* DOS/Read() */
    movem.l	%sp@+,%d1-%d3/%a0-%a1/%a6
    BRTS

BCPLd taskwait       /* a4, DosPacket @,  */
    BRTS

BCPLd putPkt         /* a8, void, @DosPacket */
    BRTS

BCPL writeoutput    /* ac, void, &buf, bytelength */
    movem.l	%d1-%d3/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    move.l	%d2,%d3
    move.l	%d1,%d2
    jsr		%a6@(10 * -6)	/* DOS/Output() */
    move.l	%d0,%d1
    jsr		%a6@(8 * -6)	/* DOS/Write() */
    movem.l	%sp@+,%d1-%d3/%a0-%a1/%a6
    BRTS

BCPLd packstring     /* b0, void, @Array, @String */
    BRTS

BCPLd unpackstring   /* b4, void, @String, @Array */
    BRTS

BCPLd holdTask       /* b8, void, @memseg */
    BRTS

BCPL Delay          /* bc, void, ?? */
    /* TODO: Actually delay. For now, just say we did. */
    moveq.l	#-1,%d0
    BRTS

    /* Due to the way BCPL arguments work, A1 is the fully
     * synthesized DosPacket
     */
BCPL sendPacket     /* c0, BOOL, -, MsgPort, Type, -, -, Arg1,... */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6	/* DOSBase */
    move.l	%a1@(9 * 4),%sp@-	/* dp_Arg5 */
    move.l	%a1@(8 * 4),%sp@-	/* dp_Arg4 */
    move.l	%a1@(7 * 4),%sp@-	/* dp_Arg3 */
    move.l	%a1@(6 * 4),%sp@-	/* dp_Arg2 */
    move.l	%a1@(5 * 4),%sp@-	/* dp_Arg1 */
    move.l	%d3,%sp@-
    move.l	%d2,%sp@-
    lea.l	%a1@(4 * 4),%a0		/* Pointer to dp_Res2 */
    move.l	%a0,%sp@-
    move.l	%a6,%sp@-
    jsr		dopacket5
    lea.l	%sp@(9 * 4),%sp
    move.l	%d0,%sp@-
    jsr		%a6@(77 * -6)	/* DOS/SetIoError */
    move.l	%sp@+,%d0
    movem.l	%sp@+,%a0-%a1/%a6
    /* Patch up DOS packet with dp_Res */
    move.l	%d0,%a1@(3 * 4)
    moveq.l	#-1,%d0
    BRTS

BCPLd returnPacket   /* c4, void, @DosPacket, res1, res2 */
    BRTS

BCPLd openWindow     /* c8, void, leftedge, topedge, width, height, @title */
    BRTS

    /* If BOOL is DOSTRUE, set current directory
     * Always return old current directory
     */
BCPL setCurrentDir  /* cc, FileLock @, BOOL, @FileLock */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    move.l	%d1,%sp@-
    move.l	%d2,%d1
    jsr		%a6@(21 * -6)	/* DOS/CurrentDir() */
    move.l	%sp@+,%d1
    tst.l	%d1
    bne		0f

    /* Restore CurrentDir */
    move.l	%d0,%sp@-
    move.l	%d0, %d1
    jsr		%a6@(21 * -6)	/* DOS/CurrentDir() */
    move.l	%sp@+,%d0
0:
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd systemRequest  /* d0, void, @line1, @line2, @line3 */
    BRTS

BCPL writet         /* d4, void, @string, n */
    move.l	%d2,%sp@-
    move.l	#(3*4 + 2*4),%d0
    lea.l	BCPL_writes, %a4
    jsr		(%a5)
    move.l	%d1,%sp@-	/* Save string length */

    move.l	%d2,%d1
    ble		1f
0:
    move.l	#(3*4 + 2*4),%d0
    moveq.l	#' ',%d1
    lea.l	BCPL_wrch, %a4
    jsr		(%a5)
    dbf		%d2,0b
1:
    move.l	%sp@+,%d0	/* Get string length */
    move.l	%sp@+,%d2
    cmp.l	%d2,%d0		/* D0 > D2? */
    ble		2f
    move.l	%d2,%d0		/* Return field length */
2:
    BRTS

BCPL rdch           /* d8, char ,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(9 * -6)	/* DOS/Input() */
    move.l	%d0, %d1
    jsr		%a6@(51 * -6)	/* DOS/FGetC() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd unrdch         /* dc, void,  */
    BRTS

BCPL wrch           /* e0, void, char */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    move.l	%d1, %d2
    jsr		%a6@(10 * -6)	/* DOS/Output() */
    move.l	%d0, %d1
    jsr		%a6@(52 * -6)	/* DOS/FPutC() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPLd readinput_e4   /* e4, lwlength , @buf, lwlength */
    BRTS

BCPL writeoutput_e8 /* e8, void, @buf, lwlength */
    movem.l	%d2-%d4/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    move.l	%d2, %d4	/* Num blocks */
    moveq.l	#4, %d3		/* Block len (sizeof(ULONG)) */
    move.l	%d1, %d2	/* Buffer */
    lsl.l	#2, %d2
    jsr		%a6@(10 * -6)	/* DOS/Output() */
    move.l	%d0, %d1	/* Outfile */
    jsr		%a6@(55 * -6)	/* DOS/FWrite() */
    movem.l	%sp@+,%d2-%d4/%a0-%a1/%a6
    BRTS

BCPL findinput      /* ec, FileHandle ^, &name */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    move.l	#1005 /* MODE_OLDFILE */,%d2
    jsr		%a6@(5 * -6)	/* DOS/Open() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL findoutput     /* f0, FileHandle ^, &name */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    move.l	#1005 /* MODE_NEWFILE */,%d2
    jsr		%a6@(5 * -6)	/* DOS/Open() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL SelectInput    /* f4, void , @FileHandle */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(49 * -6)	/* DOS/SelectInput() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL SelectOutput   /* f8, void , @FileHandle */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(50 * -6)	/* DOS/SelectOutput() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd endwrite       /* 100, void,  */
    BRTS

BCPL Input          /* 104, FileHandle @,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(9 * -6)	/* DOS/Input() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Output         /* 108, FileHandle @,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(10 * -6)	/* DOS/Output() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd readn          /* 10c, void,  */
    BRTS

BCPL newline        /* 110, void,  */
    moveq.l	#'\n',%d1
    jmp		BCPL_wrch

BCPL writei         /* 114, void, number, width */
    BRTS

BCPL writen         /* 118, void, number */
    clr.l	%d2
    jmp		BCPL_writei

BCPLd writehex       /* 11c, void, number, width */
    BRTS

BCPLd writeoct       /* 120, void, number, width */
    BRTS

BCPL writes         /* 124, void, @string */
    movem.l	%d2-%d3,%sp@-
    lsl.l	#2, %d1
    clr.l	%d2
    move.b	%a0@(%d1),%d2
    move.l	%d2,%sp@-	/* Save string length */
    move.l	%d1,%d3
0:
    addq.l	#1,%d3
    move.l	#(3*4 + 1*4),%d0
    clr.l	%d1
    move.b	%a0@(%d3),%d1
    lea.l	BCPL_wrch, %a4
    jsr		(%a5)
    dbf		%d2,0b
    move.l	%sp@+,%d0	/* Return string length */
    movem.l	%sp@+,%d2-%d3
    BRTS

BCPL writef         /* 128, void, @format, arg1, arg2, ... */
    /* TODO: Actually do this right. For now,
     * just print the format string */
    jmp		BCPL_writes

BCPL toupper        /* 12c, void, char */
    move.l	%d1,%d0
    bclr	#5,%d0
    BRTS

BCPL charcmp        /* 130, long , char1, char2 */
    move.l	%d2,%d0
    sub.l	%d1,%d0
    BRTS

BCPL strcmp         /* 134, long , @String1, @String2 */
    movem.l	%d1-%d2/%a0-%a1,%sp@-
    lsl.l	#2,%d1
    addq.l	#1,%d1
    lsl.l	#2,%d2
    addq.l	#1,%d2
    move.l	%d2, %sp@-
    move.l	%d1, %sp@-
    jsr		strcmp
    addq.l	#8,%sp
    movem.l	%sp@+,%d1-%d2/%a0-%a1
    BRTS

BCPL rdargs         /* 138, BOOL, @template, array, array_size */
    /* BCPL rdargs and DOS/ReadArgs() don't match up
     *      very nicely. We'll need to call a BCPL/ReadArgs
     *      routine that works the way this expects
     */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		BCPL_ReadArgs
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd rdline         /* 13c, void,  */
    BRTS

BCPLd subString      /* 140, void, @string, @substring */
    BRTS

BCPLd load           /* 144, SegList ^, @filename */
    BRTS

BCPL UnLoadSeg      /* 148, void, @seg */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(26 * -6)	/* DOS/UnLoadSeg */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd tidyup         /* 150, void,  */
    BRTS

BCPLd addDevice      /* 154, void, @name */
    BRTS

BCPLd datestamp      /* 158, void, @time */
    BRTS

BCPLd WaitForChar    /* 15c, BOOL , @FileHandle, time */
    BRTS

BCPLd execLib        /* 160, long , LVOffset, d0, d1, a0, a1, a2 */
    BRTS

BCPL findSegArray   /* 164, Array @,  */
    move.l	%a2,%d0
    addi.l	#BCPL_SegArray_0,%d0
    lsr.l	#2,%d0
    BRTS

BCPLd deleteObject   /* 168, void, @name */
    BRTS

BCPLd rename         /* 16c, void, @oldname, @newname */
    BRTS

/*      170 - contains APTR to IntuitionBase */

BCPLd Close          /* 174, void, @FileHandle */
    BRTS

BCPLd getword        /* 178, void, longaddress, wordnumber */
    BRTS

BCPLd putword        /* 17c, void, longaddress, wordnumber */
    BRTS
                     /* 180, ??? */

BCPLd taskwait_190   /* 190, DosPacket ^,  */
    BRTS

BCPL execute        /* 194, void, &string , @in, @out */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(37 * -6)	/* DOS/Execute() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL deviceproc     /* 198, FileLock ^, @filename */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    lsl.l	#2,%d1
    addq.l	#1,%d1
    jsr		%a6@(29 * -6)	/* DOS/DeviceProc() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd libcall        /* 19c, void, libbase, LVOffset, d0, d1, a0, a1 */
    BRTS

BCPLd findConsoleHandler /* 1a4, MsgPort ^,  */
    BRTS

BCPLd findFileHandler    /* 1a8, MsgPort ^,  */
    BRTS

BCPL extractDeviceName  /* 1ac, void, @buf, bufsize, @name , flag */
    movem.l	%d1-%d4/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    lsl.l	#2,%d1
    addq.l	#1,%d1
    lsl.l	#2,%d2
    subq.l	#1,%d2
    lsl.l	#2,%d3
    addq.l	#1,%d3
    move.l	%d2,%sp@-
    clr.l	%sp@-
    move.l	%d1,%sp@-
    move.l	#':',%sp@-
    move.l	%d3,%sp@-
    jsr		%a6@(69 * -6)	/* SplitName */
    addq.l	#8,%sp
    move.l	%sp@+,%a0	/* Get buffer back */
    addq.l	#8,%sp
    move.b	%d0,%a0@(-1)	/* Set BCPL size byte */
    clr.b	%a0@(%d0)	/* Make C string end */
    moveq.l	#-1,%d0
    movem.l	%sp@+,%d1-%d4/%a0-%a1/%a6
    BRTS

BCPL lock           /* 1b0, FileLock ^, @name */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    lsl.l	#2,%d1
    addq.l	#1,%d1		/* STRPTR Name from BSTR */
    moveq.l	#-2,%d2		/* Shared lock */
    jsr		%a6@(14 * -6)	/* DOS/Lock() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL UnLock         /* 1b4, void, @FileLock */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(15 * -6)	/* DOS/UnLock() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL getlong        /* 1b8, void, longoffset, @struct */
    add.l	%d2,%d1
    lsl.l	#2,%d1
    move.l	%a0@(%d1),%d0
    BRTS

BCPL putlong        /* 1bc, void, longoffset, @struct, data */
    add.l	%d2,%d1
    lsl.l	#2,%d1
    move.l	%a0@(%d1),%d0
    move.l	%d3,%a0@(%d1)
    BRTS

BCPLd openfile       /* 1c0, FileHandle ^, @devinfo, @filename */
    BRTS

BCPL DupLock        /* 1c4, FileLock ^, @FileLock */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(16 * -6)	/* DOS/DupLock() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd makesysreq     /* 1c8, BOOL, count, pkt, proc */
    BRTS

BCPL strcpy         /* 1cc, void, @src, @dest */
    move.l	%d3,%sp@-
    move.l	#255,%d3
    move.l	#(3*4 + 2*4),%d0
    lea.l	BCPL_strncpy, %a4
    jsr		(%a5)
    move.l	%sp@+,%d3
    BRTS

BCPLd runLoadedCode  /* 1e4, void, @codeseg, stackbytes, @cmdline, cmdlinelength */
    BRTS

    /* Lock via C string */
BCPL lock_1ec       /* 1ec, FileLock ^, &name */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    moveq.l	#-2,%d2		/* Shared lock */
    jsr		%a6@(14 * -6)	/* DOS/Lock() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL findDevInfo    /* 1f0, DevInfo ^, @name */
    movem.l	%d1-%d3/%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    lsl.l	#2,%d1
    addq.l	#1,%d1
    move.l	%d1,%sp@-
    move.l	#0x1d,%d1	/* LDF_ALL | LDF_READ */
    jsr		%a6@(109 * -6)	/* DOS/LockDosList */
    move.l	%d0,%d1
    move.l	%sp@+,%d2
    move.l	#0x1d,%d3	/* LDF_ALL | LDF_READ */
    jsr		%a6@(114 * -6)	/* DOS/FindDosEntry */
    move.l	%d0,%sp@-
    move.l	#0x1d,%d1	/* LDF_ALL | LDF_READ */
    jsr		%a6@(110 * -6)	/* DOS/UnlockDosList */
    move.l	%sp@+,%d0
    lsr.l	#2,%d0
    movem.l	%sp@+,%d1-%d3/%a0-%a1/%a6
    BRTS

BCPL createDir      /* 1f4, FileLock^, &name */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(20 * -6)	/* DOS/CreateDir() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd compareTime    /* 1f8, SIGN , @timeval, @timeval */
    BRTS

BCPLd timerIO        /* 1fc, void, command, @IOStdReq, secs, usecs */
    BRTS

BCPLd settime        /* 200, void, @timeval */
    BRTS

/*      214 - contains CLI command */

BCPLd findCLI        /* 218, CommandLineInterpreter ^,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(BCPL_DOSBase),%a6
    jsr		%a6@(82 * -6)	/* DOS/Cli() */
    lsr.l	#2,%d0
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

