/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCPL stubs that call into Exec and Dos libraries.
    Lang: english
*/
#define DEBUG 0
#define BCPLSTRINGDEBUG 0
#include <aros/m68k/asm.h>

#include "bcpl.h"

#define BCPL(id,name)	.equ GV_##name, id
#include "bcpl.inc"
#undef BCPL

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
    /* Call out to GCC's __mulsi3 */
    movem.l  %a0-%a1,%sp@-
    move.l  %d2, %sp@-
    move.l  %d1, %sp@-
    jsr     __mulsi3
    addq.l  #8,%sp
    move.l  %d0, %d1
    movem.l  %sp@+,%a0-%a1
    rts
BCPL_jsr_divmod:
    /* Call out to GCC's __divsi3 and __modsi3 */
    movem.l  %a0-%a1,%sp@-
    move.l  %d2, %sp@-
    move.l  %d1, %sp@-
    jsr     __modsi3
    move.l  %d0, %d2
    jsr     __divsi3
    move.l  %d0, %d1
    addq.l  #8, %sp
    movem.l  %sp@+,%a0-%a1
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
    .string  "     %s, D1=%p\n"
.LBCPL_format_err:
    .string  "     %s, D1=%p IoErr=%p\n"

    /* For dummy functions */
    .macro  BCPLd name
    .section .rodata
99:
    .string "\name"
    .text
    .balign 4
    .globl  BCPL_\name
BCPL_\name:
    /* Stash away pr_Result2 into the GV */
    move.l  SysBase,%a0
    move.l  %a0@(ThisTask),%a0
    move.l  %a0@(pr_Result2),%d0
    move.l  %d0,%a2@(GV_DEBUG_Result2)
    sub.l   %a0,%a0

    movem.l %d1/%a0-%a1,%sp@-
    move.l  %a2,%sp@-
    move.l  %d4,%sp@-
    move.l  %d3,%sp@-
    move.l  %d2,%sp@-
    move.l  %d1,%sp@-
    move.l  #99b,%sp@-
    move.l  #.LBCPL_format_trace,%sp@-
    jsr     bcpl_command_name
    jsr     kprintf
    lea.l   %sp@(7*4),%sp
    movem.l %sp@+,%d1/%a0-%a1
    clr.l   %d0		/* DOSFALSE */
    .endm

    .macro  BRTSd
    /* Retrieve old pr_Result2 */
    move.l  %a2@(GV_DEBUG_Result2), %d2

    movem.l %d0/%a0-%a1,%sp@-
    move.l  SysBase,%a0
    move.l  %a0@(ThisTask),%a0
    move.l  %a0@(pr_Result2),%a1
    move.l  %a1,%sp@-
    move.l  %d0,%sp@-
    move.l  #99b,%sp@-
    cmp.l   %sp@(8),%d2
    beq.s   0f
    move.l  #.LBCPL_format_err,%sp@-
    bra.s   1f
0:
    move.l  #.LBCPL_format_ret,%sp@-
1:
    jsr     bcpl_command_name
    jsr     kprintf
    lea.l   %sp@(4*4),%sp
    movem.l %sp@+,%d0/%a0-%a1
    move.l  %d0,%d1
    jmp     (%a6)
    .endm

    /* For real functions */
    .macro  BCPLnd name
    .balign 4
    .globl  BCPL_\name
BCPL_\name:
    .endm

    .macro  BRTSnd
    move.l  %d0,%d1
    jmp     (%a6)
    .endm

#if DEBUG
#define BCPL BCPLd
#define BRTS BRTSd
#else
#define BCPL BCPLnd
#define BRTS BRTSnd
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
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(37 * -6)	/* DOS/Execute() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL IsInteractive  /* -68, BOOL, @filehandle */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(36 * -6)	/* DOS/IsInteractive() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL DateStamp      /* -64, timeval *, &time */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(32 * -6)	/* DOS/DateStamp() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL SetProtection  /* -60, BOOL, &name, mask */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(31 * -6)	/* DOS/SetProtection() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL SetComment     /* -5c, BOOL, &name, &comment */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(30 * -6)	/* DOS/SetComment() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd QueuePacket    /* -54, void */
    BRTS

BCPL clearmem       /* -50, void, @buff, lw_length */
    lsl.l	#2,%d1
    bra		.Lclearmem_cond

.Lclearmem_loop:
    clr.l	%a0@(%d1)
    addq.l	#4,%d1
.Lclearmem_cond:
    dbf		%d2,.Lclearmem_loop

    BRTS

BCPL LoadSeg        /* -4c, SegList ^, &name */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(25 * -6)	/* DOS/LoadSeg() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL CreateProc     /* -48, MessagePort *, &name, pri, seg, stackbytes */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(23 * -6)	/* DOS/CreateProc() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL IoErr          /* -44, long */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(22 * -6)	/* DOS/IoErr() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL CurrentDir     /* -40, FileLock ^, @FileLock */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(21 * -6)	/* DOS/CurrentDir() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL CreateDir      /* -3c, FileLock ^, &name */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(20 * -6)	/* DOS/CreateDir() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Info           /* -38, BOOL, @FileLock, &InfoData */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(19 * -6)	/* DOS/Info() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL ExNext         /* -34, BOOL, @FileLock, &FileInfo */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(18 * -6)	/* DOS/ExNext() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Examine        /* -30, BOOL, @FileLock, &FileInfo */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(17 * -6)	/* DOS/Examine() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Lock           /* -2c, FileLock ^, &name, mode */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(14 * -6)	/* DOS/Lock() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Rename         /* -28, BOOL, &oldname, &newname */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(13 * -6)	/* DOS/Rename() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL DeleteFile     /* -24, BOOL, &name */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(12 * -6)	/* DOS/DeleteFile() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Seek           /* -20, LONG, @FileHandle, pos, mode */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(11 * -6)	/* DOS/Seek() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Write          /* -18, LONG, Write, @FileHandle, &buf, len */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(8 * -6)	/* DOS/Write() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Read           /* -0c, LONG, Read, @FileHandle, &buf, len */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(7 * -6)	/* DOS/Read() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL memcpy        /* -08, void, @src, @dest, n */
    movem.l	%d2-%d3/%a0-%a1,%sp@-
	lsl.l	#2,%d1
	move.l	%d1,%a0
	lsl.l	#2,%d2
	move.l	%d2,%a1
.copyloop:
	subq.l	#1,%d3
	bmi.s	.copydone
	move.l	%a0@+,%a1@+
	bra.s	.copyloop
.copydone:
    movem.l	%sp@+,%d2-%d3/%a0-%a1
    BRTS

BCPL Open           /* -04, FileHandle ^, &name, mode */
    movem.l	%a0-%a1/%a6,%sp@-

#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
	bsr		bstr_debug
#endif

    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(5 * -6)	/* DOS/Open() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

                     /* 4, void main */
BCPL Entry
    jsr         BCPL_RunHandler
    /* FALLTHROUGH to BCPL_Exit */

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
    jsr	%a5@(0x10)
    BRTS

BCPL divide         /*  10, D1 = D1 / D2 */
    jsr	%a5@(0x12)
    move.l	%a1@(4),%d2
    BRTS

BCPL mod            /*  14, D1 = D1 % D2 */
    jsr	%a5@(0x12)
    move.l	%d2,%d0 /* BRTS does move.l %d0,%d1 */
    move.l	%a1@(4),%d2
    BRTS

BCPL setIO          /*  18, void, @IOStdReq, command, &data, len, offset */
    lsl.l	#2,%d1
    move.w	%d2,%a0@(28,%d1)
    move.l	%d3,%d0
    lsl.l	#2,%d0
    move.l	%d0,%a0@(40,%d1)
    move.l	%d4,%a0@(36,%d1)
    /* only first 4 params are in registers */
    move.l	%a1@(4*4),%a0@(44,%d1)
    BRTS

BCPLd packlw         /*  20, ?? */
    BRTS

BCPLd unpacklw       /*  24, ?? */
    BRTS

BCPL Res2           /*  28, LONG, is_set_flag, result_code */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    tst.l	%d1
    beq         .LRes2_get
.LRes2_set:
    move.l	%d2,%d1
    jsr		%a6@(77 * -6) /* DOS/SetIoErr */
.LRes2_get:
    jsr		%a6@(22 * -6) /* DOS/IoErr */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

                     /*  30, BCPL_FrameLower */

BCPLd makeGVarea     /*  34, void, @buff */
    BRTS

BCPL findtask        /*  38, &Process->MsgPort *, */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    sub.l	%a1,%a1
    jsr		%a6@(FindTask)
    add.l	#92,%d0	/* point to &MsgPort */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd getbyte        /*  3c, BYTE, @lw_addr, bytenum */
    BRTS

BCPLd putbyte        /*  40, BYTE, @lw_addr, bytenum, byte */
    BRTS

BCPL level          /*  44, LONG (or BPTR?) */
    move.l  %a1@(-12),%d0  /* Return caller's BCPL frame */
    BRTS

BCPL longcall       /*  48, LONG, level, label */
    move.l	%d1,%a1
    move.l	%a1@(-4),%a4
    jmp		%a0@(%d2.l)

BCPL allocMem       /*  4c, BPTR, lwlength, reqs */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    move.l	%d1,%d0
    addq.l	#4,%d0         /* 4 additional words for padding */
    lsl.l	#2,%d0
    move.l	%d2,%d1
    bset.l	#16,%d1        /* MEMF_CLEAR */
    jsr		%a6@(114 * -6) /* Exec/AllocVec() */
    lsr.l	#2,%d0
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd longjump       /*  50, LONG, label */
    BRTS

BCPL doIO           /*  54, LONG, @IORequest */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    lsl.l	#2,%d1
    move.l	%d1,%a1
    jsr		%a6@(76 * -6)	/* Exec/DoIO() */
    ext.w	%d0
    ext.l	%d0
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL sendIO         /*  58, void, @IORequest */
    movem.l     %a0-%a1/%a6,%sp@-
    lsl.l       #2,%d1
    moveq.l     #0,%d2
    move.l      %d2,%a0@(ln_Name,%d1)    /* NULL dos packet */
    move.l      %d1, %a1
    move.l      SysBase,%a6
    jsr         %a6@(77 * -6)   /* Exec/SendIO */
    movem.l     %sp@+,%a0-%a1/%a6
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
    moveq	#1,%d1
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
    movem.l	%d5/%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6

#if BCPLSTRINGDEBUG
    move.l	%d2,%d0
	bsr		bstr_debug
#endif

    lsl.l	#2,%d1
    move.l	%d1,%d5

    sub.l	%a1,%a1
    jsr		%a6@(FindTask)
    
    lsl.l	#2,%d2
    addq.l	#1,%d2
    move.l	%d2,%a0
    move.l	%d5,%a1
    add.l	#92,%d0
    move.l	%d0,%a1@(14)	/* replyport = &Process->pr_MsgPort */
    move.l	%d4,%d1
    move.l	%d3,%d0
    jsr		%a6@(74 * -6)	/* Exec/OpenDevice() */
    moveq	#-1,%d1
    tst.b	%d0
    beq.s	od_ok
    moveq	#0,%d1
od_ok:
	move.l	%d1,%d0
    movem.l	%sp@+,%d5/%a0-%a1/%a6
    BRTS

BCPL closeDevice    /*  80, void, @IORequest */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    lsl.l	#2,%d1
    move.l	%d1,%a1
    jsr		%a6@(75 * -6)	/* Exec/CloseDevice() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL createProcBCPL     /*  84, MessagePort *, @segarray, lw_stack_size, pri, @name,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6

    move.l	%d3,%sp@-
    lsl.l	#2,%d2
    move.l	%d2,%sp@-
    lsl.l	#2,%d1
    move.l	%d1,%sp@-
    move.l	%d4,%d1
    moveq.l	#4, %d0
    bsr		bstr_to_cstr
    move.l	%d1,%sp@-
    move.l	%a6,%sp@-
    jsr		BCPL_CreateProcBCPL
    lea.l	%sp@(5 * 4),%sp
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL remProcess     /*  88, void */
    jmp BCPL_Exit	/* TODO: Review if this is correct */

BCPL ParentDir      /*  8c, FileLock ^, @FileLock */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(35 * -6)	/* DOS/ParentDir() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL setSignals     /*  90, void, &MsgPort, mask */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    add.l	#mp_SigTask,%d1
    move.l	%a0@(%d1),%a1
    move.l	%d2,%d0
    lsl.l	#8,%d0
    lsl.l	#4,%d0			/* shift to C-F signals */
    and.l	#0xf000,%d0		/* mask only C-F signals */
    jsr		%a6@(54 * -6)	/* Signal */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL clearSignals   /* 94, BOOL , mask */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    clr.l	%d0
    lsl.l	#8,%d1
    lsl.l	#4,%d1
    and.l	#0xf000,%d1
    move.l	%d1,%d2
    jsr		%a6@(51 * -6)	/* SetSignal */
    moveq	#0,%d1
    and.l	%d2,%d0
    beq.s	0f
    moveq	#-1,%d1
0:	move.l	%d1,%d0
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL DOSAlert       /* 98, void, alertnumber */
    movem.l	%d7/%a0-%a1/%a6,%sp@-
    move.l	SysBase,%a6
    move.l	%d1,%d7
    jsr		%a6@(18 * -6)	/* Exec/Alert() */
    movem.l	%sp@+,%d7/%a0-%a1/%a6
    BRTS

BCPL findRootNode   /* 9c, RootNode @,  */
    move.l	%a2@(GV_DOSBase),%d0
    add.l	#dl_Root,%d0
    move.l	%a0@(%d0),%d0	/* %a0 is always 0 in BCPL */
    lsr.l	#2,%d0
    BRTS

BCPL readinput      /* a0, bytelength , &buf, bytelength */
    movem.l	%d1-%d3/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    move.l	%d2,%d3
    move.l	%d1,%d2
    jsr		%a6@(9 * -6)	/* DOS/Input() */
    move.l	%d0,%d1
    jsr		%a6@(7 * -6)	/* DOS/Read() */
    movem.l	%sp@+,%d1-%d3/%a0-%a1/%a6
    BRTS

    /* Wait on the process's pr_MsgPort for a packet.
     */
BCPL taskwait       /* a4, DosPacket @,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(42 * -6)	/* Dos/WaitPkt() */
    lsr.l	#2,%d0
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

    /* When passed in, DosPacket's dp_Link points to
     * an allocated, but empty, memory location. 
     * Sent to the port of the current directory's handler.
     */
BCPL putPkt         /* a8, void, @DosPacket */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		Dos_BCPL_putpkt
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL writeoutput    /* ac, void, &buf, bytelength */
    movem.l	%d2-%d3/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    move.l	%d2,%d3
    move.l	%d1,%d2
    jsr		%a6@(10 * -6)	/* DOS/Output() */
    move.l	%d0,%d1
    jsr		%a6@(8 * -6)	/* DOS/Write() */
    movem.l	%sp@+,%d2-%d3/%a0-%a1/%a6
    BRTS

    /* copy src LONG array to destination, LONG -> BYTE */
BCPL packstring     /* b0, long, @Array, @String */
    movem.l	%d2-%d3/%a0-%a1,%sp@-
#if BCPLSTRINGDEBUG
    move.l	%d2,%d3
#endif
    lsl.l	#2,%d1
    move.l	%d1,%a0
    lsl.l	#2,%d2
    move.l	%d2,%a1
    moveq	#0,%d2
    move.l	(%a0),%d2
    move.l	%d2,%d0
    lsr.l	#2,%d0
1:  move.l	(%a0)+,%d1
    move.b	%d1,(%a1)+
    dbf		%d2,1b

#if BCPLSTRINGDEBUG
	move.l	%d0,%d2
    move.l	%d3,%d0
    bsr		bstr_debug
    move.l	%d2,%d0
#endif

    movem.l	%sp@+,%d2-%d3/%a0-%a1
    BRTS

    /* copy src BSTR to destination array, BYTE -> LONG */
BCPL unpackstring   /* b4, void, @String, @Array */
    movem.l	%a0-%a1,%sp@-

#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
    bsr		bstr_debug
#endif

    lsl.l	#2,%d1
    move.l	%d1,%a0
    lsl.l	#2,%d2
    move.l	%d2,%a1
    moveq	#0,%d0
    move.b	(%a0),%d0
    moveq	#0,%d1
1:  move.b	(%a0)+,%d1
    move.l	%d1,(%a1)+
    dbf		%d0,1b
    movem.l	%sp@+,%a0-%a1
    BRTS

BCPLd holdTask       /* b8, void, @memseg */
    BRTS

BCPL Delay          /* bc, void, ticks */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(33 * -6)	/* DOS/Delay() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

    /* Call the DOS internal 'dopacket' function
     */
BCPL sendPacket     /* c0, BOOL, -, MsgPort, Type, -, -, Arg1,... */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a1@((4 + 7) * 4), %sp@-        /* Arg7 */
    move.l	%a1@((4 + 6) * 4), %sp@-        /* Arg6 */
    move.l	%a1@((4 + 5) * 4), %sp@-        /* Arg5 */
    move.l	%a1@((4 + 4) * 4), %sp@-        /* Arg4 */
    move.l	%a1@((4 + 3) * 4), %sp@-        /* Arg3 */
    move.l	%a1@((4 + 2) * 4), %sp@-        /* Arg2 */
    move.l	%a1@((4 + 1) * 4), %sp@-        /* Arg1 */
    move.l	%d3, %sp@-                      /* Type */
    move.l	%d2, %sp@-                      /* Port */
    move.l	%a0, %sp@-                      /* A0 is still 0 */
    jsr		dopacket
    lea.l	%sp@(10 * 4), %sp
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL returnPacket   /* c4, void, @DosPacket, res1, res2 */
    tst.l	%d1     /* Don't try to reply NULL Dos Packets */
    beq.s	0f
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    lsl.l	#2, %d1
    jsr		%a6@(43 * -6)	/* DOS/ReplyPkt() */
    movem.l	%sp@+,%a0-%a1/%a6
0:
    BRTS

#define TAG_END 0
#define TAG_USER	(1 << 31)
#define WA_Dummy	(TAG_USER + 99)
#define WA_Left		(WA_Dummy + 1)
#define WA_Top		(WA_Dummy + 2)
#define WA_Width	(WA_Dummy + 3)
#define WA_Height	(WA_Dummy + 4)
#define WA_Title	(WA_Dummy + 11)
#define WA_PubScreen	(WA_Dummy + 22)
#define WA_AutoAdjust	(WA_Dummy + 45)

BCPL openWindow     /* c8, struct Window *, leftedge, topedge, width, height, @title */
	movem.l	%a0-%a1/%a6,%sp@-
	move.l	%a2@(GV_DOSBase),%a6
	move.l	%a6@(dl_IntuitionBase),%a6
	move.l	#TAG_END, %sp@-
	move.l	#-1, %sp@-
	move.l	#WA_AutoAdjust, %sp@-
	move.l	%a0, %sp@-
	move.l	#WA_PubScreen, %sp@-
	move.l	%a1@(4*4),%sp@-
	move.l	#WA_Title, %sp@-
	move.l	%d4, %sp@-
	move.l	#WA_Height, %sp@-
	move.l	%d3, %sp@-
	move.l	#WA_Width, %sp@-
	move.l	%d2, %sp@-
	move.l	#WA_Top, %sp@-
	move.l	%d1, %sp@-
	move.l	#WA_Left, %sp@-
	move.l	%sp, %a1			/* a0 is still 0 here */
	jsr		%a6@(101 * -6)		/* OpenWindowTagList */
	lea.l	%sp@(15 * 4),%sp
	movem.l	%sp@+,%a0-%a1/%a6
    BRTS

    /* If BOOL is DOSTRUE, set current directory
     * Always return old current directory
     */
BCPL setCurrentDir  /* cc, FileLock @, BOOL, @FileLock */
    movem.l	%d3-%d4/%a0-%a1/%a6,%sp@-

	move.l	%d1,%d3

    move.l	SysBase,%a6
    sub.l	%a1,%a1
    jsr		%a6@(FindTask)
    move.l	%d0,%a0
    /* sanity check in CurrentDir() complained so I did this */
    move.l	%a0@(152),%d4	/* pr_CurrentDir */

    tst.l	%d3
    beq.s	0f

    move.l	%a2@(GV_DOSBase),%a6
    move.l	%d2,%d1
    jsr		%a6@(21 * -6)	/* DOS/CurrentDir() */
0:
	move.l	%d4,%d0
    movem.l	%sp@+,%d3-%d4/%a0-%a1/%a6
    BRTS

BCPLd systemRequest  /* d0, void, @line1, @line2, @line3 */
    BRTS

BCPL writet         /* d4, void, @string, n */
    movem.l	%d2-%d4,%sp@-
    move.l	%d1, %d3
    lsl.l	#2, %d3		/* D3 = BADDR(string) */
    clr.l	%d4
    move.b	%a0@(%d3),%d4	/* D4 = string[0] (strlen) */
    tst.l	%d4
    beq		.LBCPL_writet_pad

    bra		.LBCPL_writet_loop_cond

.LBCPL_writet_loop:
    addq.l	#1,%d3		/* D3 = pointer to next char */
    subq.l	#1,%d2		/* D2 = reduce pad amount by 1 */
    move.l	#(3*4 + 1*4),%d0
    clr.l	%d1
    move.b	%a0@(%d3),%d1	/* D1 = next char */
    move.l	%a2@(GV_wrch), %a4
    jsr		(%a5)
.LBCPL_writet_loop_cond:
    dbf		%d4,.LBCPL_writet_loop

.LBCPL_writet_pad:
    tst.l	%d2
    ble		.LBCPL_writet_exit
    bra		.LBCPL_writet_pad_cond

.LBCPL_writet_pad_loop:
    move.l	#(3*4 + 2*4),%d0
    moveq.l	#' ',%d1
    move.l	%a2@(GV_wrch), %a4
    jsr		(%a5)
.LBCPL_writet_pad_cond:
    dbf		%d2,.LBCPL_writet_pad_loop

.LBCPL_writet_exit:
    movem.l	%sp@+,%d2-%d4
    BRTS

BCPLnd rdch           /* d8, char ,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(9 * -6)	/* DOS/Input() */
    move.l	%d0, %d1
    jsr		%a6@(51 * -6)	/* DOS/FGetC() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTSnd

BCPL unrdch         /* dc, void,  */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(9 * -6)	/* DOS/Input() */
    move.l	%d0, %d1
    moveq	#-1,%d2
    jsr		%a6@(53 * -6)	/* DOS/UnGetC() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPLnd wrch           /* e0, void, char */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    move.l	%d1, %d2
    jsr		%a6@(10 * -6)	/* DOS/Output() */
    move.l	%d0, %d1
    jsr		%a6@(52 * -6)	/* DOS/FPutC() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTSnd

BCPL readinput_long   /* e4, lwlength , @buf, lwlength */
    movem.l	%d1-%d3/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    move.l	%d2,%d3
    move.l	%d1,%d2
    jsr		%a6@(9 * -6)	/* DOS/Input() */
    move.l	%d0,%d1
    lsl.l	#2,%d2
    lsl.l	#2,%d3
    jsr		%a6@(7 * -6)	/* DOS/Read() */
    asr.l	#2,%d0
    movem.l	%sp@+,%d1-%d3/%a0-%a1/%a6
    BRTS

BCPL writeoutput_long /* e8, void, @buf, lwlength */
    movem.l	%d2-%d4/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    move.l	%d2, %d4	/* Num blocks */
    moveq.l	#4, %d3		/* Block len (sizeof(ULONG)) */
    move.l	%d1, %d2	/* Buffer */
    lsl.l	#2, %d2
    jsr		%a6@(10 * -6)	/* DOS/Output() */
    move.l	%d0, %d1	/* Outfile */
    jsr		%a6@(55 * -6)	/* DOS/FWrite() */
    movem.l	%sp@+,%d2-%d4/%a0-%a1/%a6
    BRTS

BCPL findinput      /* ec, FileHandle ^, @name */
    movem.l	%d2/%a0-%a1/%a6,%sp@-

#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
    bsr		bstr_debug
#endif

    moveq.l	#1, %d0
    bsr		bstr_to_cstr

    move.l	%a2@(GV_DOSBase),%a6
    move.l	#1005 /* MODE_OLDFILE */,%d2
    jsr		%a6@(5 * -6)	/* DOS/Open() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL findoutput     /* f0, FileHandle ^, @name */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
  
#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
    bsr		bstr_debug
#endif

    moveq.l	#1, %d0
    bsr		bstr_to_cstr

    move.l	%a2@(GV_DOSBase),%a6
    move.l	#1006 /* MODE_NEWFILE */,%d2
    jsr		%a6@(5 * -6)	/* DOS/Open() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL SelectInput    /* f4, void , @FileHandle */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(49 * -6)	/* DOS/SelectInput() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL SelectOutput   /* f8, void , @FileHandle */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(50 * -6)	/* DOS/SelectOutput() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL endread        /* fc, void,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(9 * -6)	/* DOS/Input() */
    move.l	%d0,%d1
    jsr		%a6@(6 * -6)	/* DOS/Close() */
    moveq.l	#0,%d1
    jsr		%a6@(49 * -6)	/* DOS/SelectInput() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL endwrite       /* 100, void,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(10 * -6)	/* DOS/Output() */
    move.l	%d0,%d1
    jsr		%a6@(6 * -6)	/* DOS/Close() */
    moveq.l	#0,%d1
    jsr		%a6@(50 * -6)	/* DOS/SelectOutput() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Input          /* 104, FileHandle @,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(9 * -6)	/* DOS/Input() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL Output         /* 108, FileHandle @,  */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(10 * -6)	/* DOS/Output() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd readn          /* 10c, void,  */
    BRTS

BCPL newline        /* 110, void,  */
    moveq.l	#'\n',%d1
    jmp		BCPL_wrch

BCPL_write_base:	/* number, width, base, canneg */
    movem.l	%d2-%d7,%sp@-

    move.l	%d3,%d6		/* D6 = base */
    move.l	%d2,%d5		/* D5 = width */
    move.l	%d1,%d3		/* D3 = Number */
    moveq	#0,%d7

    tst.l	%d4
    beq		.Lwrite_base_pos

    tst.l	%d3		/* Number < 0? */
    bge		.Lwrite_base_pos

    bset	#31,%d7	/* D7 bit 31 = negative sign needed */
    neg.l	%d3
    subq.l	#1,%d5

.Lwrite_base_pos:
    clr.l	%d4		/* D4 = Digit */
    clr.w	%d7		/* D7 = Num pushed chars */

.Lwrite_base_loop:
    move.l	%d3,%d1
    move.l	%d6,%d2
    jsr		%a5@(0x12)	/* divmod: D1 = D3/base, D2 = D3 % base */
    move.l	%d1, %d3

    /* Push char */
    move.l	%d2, %d1
    cmp.b	#10, %d1
    blt		0f
    add.b	#(('A'-10)-'0'),%d1
0:
    add.b	#'0',%d1	/* D1 = '0' .. '9' */
    move.b	%d1, %a1@(3*4 + 4*4, %d7.w)
    addq.w	#1, %d7
    subq.l	#1, %d5
    tst.l	%d3
    bgt.s	.Lwrite_base_loop

    /* Any extra field bits? */
    bra		.LBCPL_write_base_pad_cond
.LBCPL_write_base_pad:
    moveq	#' ',%d1
    cmp.b	#10,%d6
    beq.s	.LBCPL_write_base_pad_char
    moveq	#'0',%d1
.LBCPL_write_base_pad_char:
    move.l	#(3*4 + 4*4 + 8*4), %d0
    move.l	%a2@(GV_wrch), %a4
    jsr		(%a5)
.LBCPL_write_base_pad_cond:
    subq.l	#1,%d5
    bpl.s	.LBCPL_write_base_pad

	btst	#31,%d7
	beq.s	.Lwrite_base_emit
    /* Negative sign */
    move.l	#'-',%d1
    move.l	#(3*4 + 4*4 + 8*4), %d0
    move.l	%a2@(GV_wrch), %a4
    jsr		(%a5)

    /* Write chars */
.Lwrite_base_emit:
    subq.w	#1, %d7
    bmi.s	0f
    clr.l	%d1
    move.b	%a1@(3*4 + 4*4, %d7.w),%d1
    move.l	#(3*4 + 4*4 + 8*4), %d0
    move.l	%a2@(GV_wrch), %a4
    jsr		(%a5)
    jmp		.Lwrite_base_emit
0:

    movem.l	%sp@+,%d2-%d7
    BRTS

BCPL writei         /* 114, void, number, width */
    movem.l	%d3-%d4,%sp@-

    move.l	#(3*4 + 2*4), %d0
    move.l	#10,%d3
    move.l	#-1,%d4
    lea.l	BCPL_write_base, %a4
    jsr		(%a5)

    movem.l	%sp@+,%d3-%d4
    BRTS

BCPL writen         /* 118, void, number */
    movem.l	%d2-%d4,%sp@-

    move.l	#(3*4 + 1*4), %d0
    clr.l	%d2
    move.l	#10,%d3
    move.l	#-1,%d4
    lea.l	BCPL_write_base, %a4
    jsr		(%a5)

    movem.l	%sp@+,%d2-%d4
    BRTS

BCPL writehex       /* 11c, void, number, width */
    movem.l	%d3-%d4,%sp@-

    move.l	#(3*4 + 2*4), %d0
    move.l	#16,%d3
    clr.l	%d4
    lea.l	BCPL_write_base, %a4
    jsr		(%a5)

    movem.l	%sp@+,%d3-%d4
    BRTS

BCPL writeoct       /* 120, void, number, width */
    movem.l	%d3-%d4,%sp@-

    move.l	#(3*4 + 2*4), %d0
    move.l	#8,%d3
    clr.l	%d4
    lea.l	BCPL_write_base, %a4
    jsr		(%a5)

    movem.l	%sp@+,%d3-%d4
    BRTS

BCPL writes         /* 124, void, @string */
    move.l	%d2,%sp@-
    clr.l	%d2
    move.l	#(3*4 + 1*4),%d0
    move.l	%a2@(GV_writet), %a4
    jsr		(%a5)
    move.l	%sp@+,%d2
    BRTS

#if 0 /* This is defined in bcpl_writef.S */
BCPLd writef         /* 128, void, @format, arg1, arg2, ... */
    BRTS
#endif

BCPL toupper        /* 12c, void, char */
    move.l	%d1,%d0
    cmp.b	#'a',%d0
    bcs.s	1f
    cmp.b	#'z',%d0
    bhi.s	1f
    bclr	#5,%d0
1:  BRTS

BCPL charicmp        /* 130, long , char1, char2 */
    move.l	%d2,%d0
    cmp.b	#'a',%d0
    bcs.s	1f
    cmp.b	#'z',%d0
    bhi.s	1f
    bclr	#5,%d0
1:  cmp.b	#'a',%d1
    bcs.s	2f
    cmp.b	#'z',%d1
    bhi.s	2f
    bclr	#5,%d1
2:  sub.l	%d1,%d0
    BRTS

BCPL stricmp         /* 134, long , @String1, @String2 */
    movem.l	%d1-%d3/%a0-%a1,%sp@-

#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
	bsr		bstr_debug
	move.l	%d2,%d0
	bsr		bstr_debug
#endif

    lsl.l	#2,%d1
    move.l	%d1,%a0
    lsl.l	#2,%d2
    move.l	%d2,%a1
    move.b	%a0@+,%d2
    move.b	%a1@+,%d3
    moveq	#0,%d0
sicmp1:
    tst.b	%d2
    beq.b	sicmp2
    tst.b	%d3
    beq.b	sicmp2
    subq.b	#1,%d2
    subq.b	#1,%d3
    moveq	#0,%d0
    move.b	%a0@+,%d0
    bclr	#5,%d0
    moveq	#0,%d1
    move.b	%a1@+,%d1
    bclr	#5,%d1
    sub.w	%d1,%d0
    beq.s	sicmp1
	bra.s	sicmp0
sicmp2:
	cmp.b	%d2,%d3
	beq.s	sicmp0
	moveq	#-1,%d0
	tst.b	%d2
	beq.s	sicmp0
	moveq	#1,%d0
sicmp0:
    ext.l	%d0
	movem.l	%sp@+,%d1-%d3/%a0-%a1
    BRTS

BCPL rdargs         /* 138, ULONG, @template, array, array_size */
    /* BCPL rdargs and DOS/ReadArgs() don't match up
     *      very nicely. We'll need to call a BCPL/ReadArgs
     *      routine that works the way this expects
     */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		BCPL_ReadArgs
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL rditem         /* 13c, void, @buf, maxlongs  */
    movem.l	%d2-%d3/%a0-%a2/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    lsl.l	#2,%d1
    addq.l	#1,%d1
    move.l	%d1,%a2
    clr.b	%a2@
    lsl.l	#2,%d2
    subq.l	#1,%d2
    moveq	#0,%d3
    /* this can't be right but at least c:execute does something now */
    jsr		%a6@(135 * -6)	/* DOS/ReadItem() */

    move.l	%a2,%a0
    moveq	#-1,%d2
0:  addq.w	#1,%d2
    tst.b	%a0@+
	bne		0b
	move.b	%d2,%a2@(-1)	/* fix buf BSTR length */

#if BCPLSTRINGDEBUG
    move.l      %d0,%d2
    lea.l	%a2@(-1),%a2
    move.l      %a2,%d0
    lsr.l       #2,%d0
    bsr		bstr_debug
    move.l      %d2,%d0
#endif

    movem.l	%sp@+,%d2-%d3/%a0-%a2/%a6
    BRTS

BCPL subString      /* 140, void, @string, @substring */

    movem.l	%d2-%d3/%a0-%a6,%sp@-
    move.l	SysBase,%a6
    move.l	%a2@(GV_DOSBase),%a5

#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
	bsr		bstr_debug
	move.l	%d2,%d0
	bsr		bstr_debug
#endif

    moveq	#0,%d3

    lsl.l	#2,%d1
    move.l	%d1,%a2
    lsl.l	#2,%d2
    move.l	%d2,%a3

	moveq	#0,%d0
	move.b	(%a2),%d0
	moveq	#0,%d1
	move.b	(%a3),%d1
	add.l	%d1,%d0
	addq.l	#2,%d0
	moveq	#1,%d1
	jsr		%a6@(114 * -6)	/* AllocVec */
	move.l	%d0,%a4
	tst.l	%d0
	beq.s	findarg_end
	move.l	%a4,%a0

	move.l	%a0,%d1
	move.b	%a2@+,%d0
findarg_copy12:
	beq.s	findarg_copy11
	move.b	%a2@+,%a0@+
	subq.b	#1,%d0
	bra.s	findarg_copy12
findarg_copy11:
	clr.b	%a0@+

	move.l	%a0,%d2
    move.b	%a3@+,%d0
findarg_copy22:
 	beq.s	findarg_copy21
	move.b	%a3@+,%a0@+
	subq.b	#1,%d0
	bra.s	findarg_copy22
findarg_copy21:
    clr.b	%a0@+

	exg		%a5,%a6
    jsr		%a6@(134 * -6)	/* DOS/FindArg() */
    exg		%a5,%a6
	move.l	%d0,%d3

findarg_end:
	move.l	%a4,%a1
	jsr		%a6@(115 * -6)	/* FreeVec */

	move.l	%d3,%d0
    movem.l	%sp@+,%d2-%d3/%a0-%a6
    BRTS

	/* C:Resident uses this */
BCPL load           /* 144, SegList ^, @filename */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6

#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
	bsr		bstr_debug
#endif

    moveq	#1,%d0
    bsr		bstr_to_cstr
    jsr		%a6@(25 * -6)	/* DOS/LoadSeg() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL UnLoadSeg      /* 148, void, @seg */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(26 * -6)	/* DOS/UnLoadSeg */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd tidyup         /* 150, void,  */
    BRTS

BCPL addDevice      /* 154, void, @name */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6

#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
	bsr		bstr_debug
#endif

    moveq.l	#1,%d0
    bsr		bstr_to_cstr
    moveq	#0,%d2			/* DLT_DEVICE */
    jsr		%a6@(116 * -6)	/* DOS/MakeDosEntry */
    move.l	%d0,%d2
    beq.s	0f
    move.l	%d2,%d1
    lsr.l	#2,%d2
    jsr		%a6@(113 * -6)	/* DOS/AddDosEntry */
    move.l	%d0,%d1
    bne.s	0f
    jsr		%a6@(117 * -6)	/* DOS/FreeDosEntry */
    moveq	#0,%d2
0:	move.l	%d2,%d0
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL datestamp      /* 158, @DateStamp, @DateStamp */
    movem.l	%a0-%a1/%a6,%sp@-
    lsl.l	#2,%d1
    move.l	%a2@(GV_DOSBase),%a6
    jsr.l	%a6@(32 * -6)   /* DOS/DateStamp() */
    lsr.l	#2,%d0
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd WaitForChar    /* 15c, BOOL , @FileHandle, time */
    BRTS

BCPL execLib        /* 160, long , LVOffset, d0, d1, a0, a1, a2 */
    movem.l	%d2/%a0-%a3/%a6,%sp@-
    move.l	SysBase,%a6
    move.l	%a1,%a3
    move.w	%d1,%d2
    movem.l %a3@(4),%d0-%d1/%a0-%a2
    jsr		%a6@(%d2.w)
    movem.l	%sp@+,%d2/%a0-%a3/%a6
    BRTS

BCPL findSegArray   /* 164, Array @,  */
    movem.l	%a0-%a1/%a6,%sp@-
    sub.l	%a1,%a1
    move.l	SysBase,%a6
    jsr		FindTask(%a6)
    move.l	%d0,%a0
    move.l	%a0@(pr_SegList),%d0
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL deleteObject   /* 168, void, @name */
    movem.l	%a0-%a1/%a6,%sp@-
    moveq	#1,%d0
    bsr		bstr_to_cstr
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(12 * -6)	/* DOS/DeleteFile() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL rename         /* 16c, void, @oldname, @newname */
    movem.l	%d2/%a0-%a2/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    moveq	#1,%d0
    bsr		bstr_to_cstr
    lea		%sp@(-256),%sp
    move.l	%sp,%a2
    move.l	%a2,%a1
    move.l	%d1,%a0
0:  move.b	%a0@+,%a1@+
    bne.s	0b
    move.l	%d2,%d1
    moveq	#1,%d0
    bsr		bstr_to_cstr
    move.l	%d1,%d2
    move.l	%a2,%d1
    jsr		%a6@(13 * -6)	/* DOS/Rename() */
    lea		%sp@(256),%sp
    movem.l	%sp@+,%d2/%a0-%a2/%a6
    BRTS

/*      170 - contains APTR to IntuitionBase */

BCPL Close          /* 174, void, @FileHandle */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(6 * -6)	/* DOS/Close() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd getword        /* 178, void, longaddress, wordnumber */
    BRTS

BCPLd putword        /* 17c, void, longaddress, wordnumber */
    BRTS
                     /* 180, ??? */

BCPLd taskwait_190   /* 190, DosPacket ^,  */
    jmp	BCPL_taskwait

BCPL execute        /* 194, void, &string , @in, @out */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(37 * -6)	/* DOS/Execute() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL deviceproc     /* 198, FileLock ^, @filename */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6

    moveq.l	#1, %d0
    bsr		bstr_to_cstr

    jsr		%a6@(29 * -6)	/* DOS/DeviceProc() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL libcall        /* 19c, void, libbase, LVOffset, d0, d1, a0, a1 */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%d1,%a6
    move.l	%a1@(0x08),%d0
    move.l	%a1@(0x0c),%d1
    move.l	%a1@(0x10),%a0
    move.l	%a1@(0x14),%a1
    jsr		%a6@(%d2)
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd writeErr		/* 1a0, ??, ?? */
    moveq.l	#-1,%d0
    BRTS

BCPL findConsoleHandler /* 1a4, MsgPort *,  */
    movem.l     %a0-%a1/%a6,%sp@-
    move.l      SysBase,%a6
    sub.l       %a1,%a1
    jsr         %a6@(FindTask)
    move.l      %d0,%a0
    move.l      %a0@(pr_ConsoleTask),%d0
    movem.l     %sp@+,%a0-%a1/%a6
    BRTS

BCPL findFileHandler    /* 1a8, MsgPort *,  */
    movem.l     %a0-%a1/%a6,%sp@-
    move.l      SysBase,%a6
    sub.l       %a1,%a1
    jsr         %a6@(FindTask)
    move.l      %d0,%a0
    move.l      %a0@(pr_FileSystemTask),%d0
    movem.l     %sp@+,%a0-%a1/%a6
    BRTS

BCPL extractDeviceName  /* 1ac, void, @buf, separator, @name , oldpos */
    movem.l	%d1-%d5/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6

#if BCPLSTRINGDEBUG
    move.l	%d3,%d0
	bsr		bstr_debug
#endif

							/* D2 = SplitName 'separator' */
    subq.l	#1,%d4			/* D4 = oldpos - 1 (?) */
							
    move.l	%d3, %d1
    moveq	#4, %d0
    bsr		bstr_to_cstr
    						/* D1 = SplitName 'name' */

    move.l	%a1@(0), %d3	/* Recover D1 (buf) from A1[0] */
    lsl.l	#2,%d3
    addq.l	#1,%d3			/* D3 = SplitName 'buf' */

	moveq	#30,%d5			/* D5 = SplitName 'size' */

    jsr		%a6@(69 * -6)	/* SplitName */

    move.l	%d3,%a0
    moveq	#-1,%d1
0:  addq.w	#1,%d1
    tst.b	%a0@+
	bne		0b
	move.l	%d3,%a0
	move.b	%d1,%a0@(-1)	/* fix buf BSTR length */
	
    addq.l	#1,%d0			/* newpos + 1 (?) */
    movem.l	%sp@+,%d1-%d5/%a0-%a1/%a6
    BRTS

BCPL lock           /* 1b0, FileLock ^, @name */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6

    moveq.l	#1, %d0
    bsr		bstr_to_cstr

    moveq.l	#-2,%d2		/* Shared lock */
    jsr		%a6@(14 * -6)	/* DOS/Lock() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL UnLock         /* 1b4, void, @FileLock */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(15 * -6)	/* DOS/UnLock() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL getlong        /* 1b8, void, longoffset, &struct */
    lsl.l	#2,%d1
    add.l	%d2,%d1
    move.l	%a0@(%d1),%d0
    BRTS

BCPL putlong        /* 1bc, void, longoffset, &struct, data */
    lsl.l	#2,%d1
    add.l	%d2,%d1
    move.l	%a0@(%d1),%d0
    move.l	%d3,%a0@(%d1)
    BRTS

BCPL openDevInfo       /* 1c0, FileHandle ^, @DevInfo, @filename/window */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    move.l	%a6,%sp@-
    move.l	%d2,%sp@-
    move.l	%d1,%sp@-
    jsr		RunHandler
    lea.l	%sp@(12),%sp
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL DupLock        /* 1c4, FileLock ^, @FileLock */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6
    jsr		%a6@(16 * -6)	/* DOS/DupLock() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

/* Calls ErrorReport(IoErr(), ...)
 * Why this is called 'makesysreq' is a question
 * for the TripOS guys.
 */
BCPL makesysreq     /* 1c8, BOOL, type, arg, *msgport */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%d3,%d4
    move.l	%d2,%d3
    move.l	%d1,%d2
    move.l	%a2@(GV_DOSBase),%a6
    jsr 	%a6@(22 * -6)	/* IoErr */
    jsr 	%a6@(80 * -6)	/* ErrorReport */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPL strcpy         /* 1cc, void, @src, @dest */
    move.l	%d2,%sp@-
    lsl.l	#2,%d1
    lsl.l	#2,%d2
    moveq	#0,%d0
    move.b	%a0@(%d1),%d0
0:  move.b	%a0@(%d1),%a0@(%d2)
    addq.l	#1,%d1
    addq.l	#1,%d2
    dbf		%d0,0b
    move.l	%sp@+,%d2
    BRTS

BCPL runCommand    /* 1e4, void, @codeseg, stacklongs, @cmdline, cmdlinelength */
    movem.l     %a0-%a1/%a6,%sp@-
    lsl.l       #2,%d2
    lsl.l       #2,%d3
    move.l      %a2@(GV_DOSBase),%a6
    jsr         %a6@(84 * -6)   /* DOS/RunCommand() */
    movem.l     %sp@+,%a0-%a1/%a6
    BRTS

BCPL lock_1ec       /* 1ec, FileLock ^, &name */
    movem.l	%d2/%a0-%a1/%a6,%sp@-
#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
    bsr		bstr_debug
#endif
    move.l	%a2@(GV_DOSBase),%a6
    moveq.l	#1, %d0
    bsr		bstr_to_cstr
    moveq.l	#-2,%d2		/* Shared lock */
    jsr		%a6@(14 * -6)	/* DOS/Lock() */
    movem.l	%sp@+,%d2/%a0-%a1/%a6
    BRTS

BCPL findDevInfo    /* 1f0, DevInfo ^, @name */
    movem.l	%d2-%d3/%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6

#if BCPLSTRINGDEBUG
    move.l	%d1,%d0
	bsr		bstr_debug
#endif

    moveq.l	#1,%d0
    bsr		bstr_to_cstr

    move.l	%d1,%sp@-
    moveq	#0x1d,%d1	/* LDF_ALL | LDF_READ */
    jsr		%a6@(109 * -6)	/* DOS/LockDosList */
    move.l	%d0,%d1
    move.l	%sp@+,%d2
    moveq	#0x1d,%d3	/* LDF_ALL | LDF_READ */
    jsr		%a6@(114 * -6)	/* DOS/FindDosEntry */
    move.l	%d0,%sp@-
    moveq	#0x1d,%d1	/* LDF_ALL | LDF_READ */
    jsr		%a6@(110 * -6)	/* DOS/UnlockDosList */
    move.l	%sp@+,%d0
    lsr.l	#2,%d0
    movem.l	%sp@+,%d2-%d3/%a0-%a1/%a6
    BRTS

BCPL createDir      /* 1f4, FileLock^, @name */
    movem.l	%a0-%a1/%a6,%sp@-
    move.l	%a2@(GV_DOSBase),%a6

    moveq.l	#1,%d0
    bsr		bstr_to_cstr

    jsr		%a6@(20 * -6)	/* DOS/CreateDir() */
    movem.l	%sp@+,%a0-%a1/%a6
    BRTS

BCPLd compareTime    /* 1f8, SIGN , @timeval, @timeval */
    /* I suspect this is actually Dos/CompareDates(), but
     * I haven't found a user of this routine yet.
     */
    BRTS

BCPL timerIO        /* 1fc, void, command, @IOStdReq, secs, usecs */
    movem.l     %a0-%a1/%a6,%sp@-
    lsl.l       #2,%d2
    move.l      %d2,%a1
    move.w      %d1,%a1@(io_Command)
    move.l      %d3,%a1@(tr_time + 0)
    move.l      %d4,%a1@(tr_time + 4)
    move.l      SysBase, %a6
    jsr.l       %a6@(76 * -6)   /* Exec/DoIO() */
    moveq.l     #-1,%d0 
    movem.l     %sp@+,%a0-%a1/%a6
    BRTS

BCPL settime        /* 200, void, @DateStamp */
    move.l      %a2@(GV_DOSBase),%d0
    move.l      %a0@(dl_TimeReq,%d0),%d2
    lsl.l       #2,%d1
    move.l      %a0@(0,%d1),%d3 /* ds_Days */
    mulu.w      #(60*24),%d3    /* mulu.w is fine until 2162 AD,  */
                                /* but tv_secs is too small in 2047 AD */
    add.l       %a0@(4,%d1),%d3 /* ds_Minute */
    move.l      %d3,%d4
    mulu.w      #(60),%d3
    lsr.l       #8,%d4
    lsr.l       #8,%d4
    mulu.w      #(60),%d4
    lsl.l       #8,%d4
    lsl.l       #8,%d4
    add.l       %d4,%d3
    mov.l       %a0@(8,%d1),%d4 /* ds_Tick */
    divu.w      #50,%d4
    ext.l       %d4
    add.l       %d4,%d3
    moveq.l     #0,%d4
    lsr.l       #2,%d2
    move.l      #11,%d1         /* TR_SETSYSTIME */
    move.l      #(1*4 + 3*4),%d0
    move.l      %a2@(GV_timerIO),%a4
    jsr         %a5@
    BRTS

    /*
     * On AOS, this routine called the internal
     * CliInit(), which handled setting up the
     * core SYS: lock, assigns, and paths.
     *
     * The (normally NULL) DOS Packet is always ignored.
     *
     * Although (for various reasons) this has been done
     * before this routine has been called, we still need
     * to call CliInit* to process the inbound AROS packet.
     *
     * If the passed in DOS Packet is NULL, then we have
     * nothing to do, since that's not valid under AROS.
     *
     * If the DOS Packet is *not* NULL, then pass it
     * to the appropriate CliInit* routine.
     *
     */
BCPL CliInit         /* 214, APTR, @DosPacket */
    tst.l       %d1
    beq.s       .L_CliInit_exit

    lsl.l       #2,%d1
    move.l      %a0@(dp_Res1,%d1),%d2
    tst.l       %d2
    beq.s       .L_CliInit_Run

.L_CliInit_Newcli:
    /* CliInitNewcli */
    move.l      #(155*-6),%d2
    bra.s       .L_CliInit_Init

.L_CliInit_Run:
    /* CliInitRun */
    move.l      #(156*-6),%d2

.L_CliInit_Init:
    movem.l      %a0-%a1/%a6,%sp@-
    move.l      %d1,%a0
    move.l	%a2@(GV_DOSBase),%a6
    jsr         %a6@(%d2)
    movem.l      %sp@+,%a0-%a1/%a6
.L_CliInit_exit:
    BRTS

BCPLnd CliInit_NEWCLI
    lsl.l       #2, %d1
    move.l      #1, %a0@(dp_Type, %d1)
    jmp         .L_CliInit_Newcli

BCPLnd CliInit_RUN
    lsl.l       #2, %d1
    move.l      #-1, %a0@(dp_Type, %d1)
    jmp         .L_CliInit_Run

BCPLnd CliInit_SYSTEM
    lsl.l       #2, %d1
    move.l      #-2, %a0@(dp_Type, %d1)
    jmp         .L_CliInit_Run

BCPLnd CliInit_ASYSTEM
    lsl.l       #2, %d1
    move.l      #-3, %a0@(dp_Type, %d1)
    jmp         .L_CliInit_Run

BCPLnd CliInit_BOOT
    lsl.l       #2, %d1
    move.l      #-4, %a0@(dp_Type, %d1)
    jmp         .L_CliInit_Run


BCPL findCLI        /* 218, CommandLineInterpreter ^,  */
    movem.l		%a0-%a1/%a6,%sp@-
    move.l		%a2@(GV_DOSBase),%a6
    jsr			%a6@(82 * -6)	/* DOS/Cli() */
    lsr.l		#2,%d0
    movem.l		%sp@+,%a0-%a1/%a6
    BRTS


/* Helper routines */
    /* Convert BCPL string to Cstring inline in the A1 vector
     * In:
     *   D0: Index into A1, in words, to hold dest
     *   D1: BSTR to convert
     *   A1: ULONG vector
     * Out:
     *   D0: Clobbered
     *   D1: Memory address of Cstring
     *   A1: Unchanged
     */
bstr_to_cstr:
    movem.l	%d0/%a0-%a1,%sp@-
    sub.l	%a0,%a0
    lsl.l	#2,%d1
    addq.l	#1,%d1
    clr.l	%sp@-
    move.b	%a0@(-1,%d1),%sp@(3)	/* Arg3 = strlen */
    move.l	%d1,%sp@-		/* Arg2 = src */
    lsl.l	#2,%d0
    pea.l	%a1@(%d0)		/* Arg1 = dest */
    jsr		memcpy
    move.l	%sp@+,%a0		/* A0 = dest */
    addq.l	#4,%sp			/* Drop src */
    move.l	%sp@+,%d1		/* D1 = strlen */
    move.b	#0,%a0@(%d1)		/* Terminate string with an ASCIIZ */
    move.l	%a0,%d1			/* D1 = dest */
    movem.l	%sp@+,%d0/%a0-%a1
    rts

#if BCPLSTRINGDEBUG
.bstr_format:
    .string  "%p->%p:%d:'%b'\n"
bstr_debug:
	movem.l	%d1/%a0-%a1,%sp@-
    move.l	%d0,%a0
    add.l	%a0,%a0
    add.l	%a0,%a0
    moveq	#0,%d1
    move.b	(%a0),%d1
    move.l	%d0,%sp@-
    move.l	%d1,%sp@-
    move.l  %a0,%sp@-
    move.l	%d0,%sp@-
    pea		.bstr_format(%pc)
    jsr     kprintf
    lea		20(%sp),%sp
    movem.l	%sp@+,%d1/%a0-%a1
    rts
#endif
