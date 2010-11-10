/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCPL stubs that call into Exec and Dos libraries.
    Lang: english
*/

/* BCPL macros
 */
    .macro  BCPL name
    .balign 4
    .globl  BCPL_\name
BCPL_\name:
    .endm

    .macro  BRTS
    jmp     (%a6)
    .endm

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

    .globl  BCPL_rts  /* A6 */
BCPL_rts:
    movem.l -12(%a1),%a1/%a3   /* Restore old BCPL FP and return address */
    move.l  -4(%a1),%a4        /* Get old A4 */
    jmp     (%a3)              /* Go back */

    /* Global vector table entries */
BCPL sysRequest     /* -84, void, @line1, @line2, @line3 */
    BRTS

BCPL toCStr         /* -80, char *, @string */
    lsl.l   #2,%d1
    addq.l  #1,%d1
    BRTS

BCPL toBSTR         /* -7c, void, &src, @dest */
    BRTS

BCPL Execute        /* -6c, BOOL, &command, @input, @output */
    BRTS

BCPL IsInteractive  /* -68, BOOL, @filehandle */
    BRTS

BCPL DateStamp      /* -64, timeval *, &time */
    BRTS

BCPL SetProtection  /* -60, BOOL, &name, mask */
    BRTS

BCPL SetComment     /* -5c, BOOL, &name, &comment */
    BRTS

BCPL QueuePacket    /* -54, void */
    BRTS

BCPL clearmem       /* -50, void, @buff, lw_length */
    BRTS

BCPL LoadSeg        /* -4c, SegList ^, &name */
    BRTS

BCPL CreateProc     /* -48, MessagePort *, &name, pri, seg, stackbytes */
    BRTS

BCPL IoErr          /* -44, long */
    BRTS

BCPL CurrentDir     /* -40, FileLock ^, @FileLock */
    BRTS

BCPL CreateDir      /* -3c, FileLock ^, &name */
    BRTS

BCPL Info           /* -38, BOOL, @FileLock, &InfoData */
    BRTS

BCPL ExName         /* -34, BOOL, @FileLock, &FileInfo */
    BRTS

BCPL Examine        /* -30, BOOL, @FileLock, &FileInfo */
    BRTS

BCPL Lock           /* -2c, FileLock ^, &name, mode */
    BRTS

BCPL Rename         /* -28, BOOL, &oldname, &newname */
    BRTS

BCPL DeleteFile     /* -24, BOOL, &name */
    BRTS

BCPL Seek           /* -20, LONG, @FileHandle, pos, mode */
    BRTS

BCPL Write          /* -18, LONG, Write, @FileHandle, &buf, len */
    BRTS

BCPL Read           /* -0c, LONG, Read, @FileHandle, &buf, len */
    BRTS

BCPL strncpy        /* -08, void, @src, @dest, n */
    BRTS

BCPL Open           /* -04, FileHandle ^, &name, mode */
    BRTS

BCPL Exit           /*  08, void, code */
    BRTS

BCPL multiply       /*  0c, D1 = D1 * D2 */
    BRTS

BCPL divide         /*  10, D1 = D1 / D2 */
    BRTS

BCPL mod            /*  14, D1 = D1 % D2 */
    BRTS

BCPL setIO          /*  18, void, @IOStdReq, command, &data, len, offset */
    BRTS

BCPL packlw         /*  20, ?? */
    BRTS

BCPL unpacklw       /*  24, ?? */
    BRTS

BCPL Res2           /*  28, LONG, is_set_flag, result_code */
    BRTS

BCPL makeGVarea     /*  34, void, @buff */
    BRTS

BCPL findtask       /*  38, Process *, */
    BRTS

BCPL getbyte        /*  3c, BYTE, @lw_addr, bytenum */
    BRTS

BCPL putbyte        /*  40, BYTE, @lw_addr, bytenum, byte */
    BRTS

BCPL level          /*  44, LONG (or BPTR?) */
    move.l  %d0, %a1@(-16)  /* Return caller's BCPL frame */
    BRTS

BCPL longcall       /*  48, LONG, level, label */
    BRTS

BCPL allocMem       /*  4c, BPTR, lwlength, reqs */
    BRTS

BCPL longjump       /*  50, LONG, label */
    BRTS

BCPL doIO           /*  54, LONG, @IORequest */
    BRTS

BCPL sendIO         /*  58, void, @IORequest */
    BRTS

    /* Coroutine support:
     * struct coroutine {
     *      BPTR colist;
     *      BPTR parent;
     *      APTR frame;
     *      APTR procedure;
     */
BCPL createco       /*  5c, void, @coroutine, lwlength */
    BRTS

BCPL deleteco       /*  60, void, @coroutine */
    BRTS

BCPL callco         /*  64, void, @coroutine, arg */
    BRTS

BCPL cowait         /*  68, void, result */
    BRTS

BCPL resumeco       /*  6c, void, @coroutine */
    BRTS

BCPL installSeg     /*  70, BOOL, @memseg_BCPL */
    BRTS

BCPL getvec         /*  74, LONG ^, lw_length */
    BRTS

BCPL freevec        /*  78, void, @vec */
    BRTS

BCPL openDevice     /*  7c, BOOL, @IORequest, @name, unit, flags */
    BRTS

BCPL closeDevice    /*  80, void, @IORequest */
    BRTS

BCPL createproc     /*  84, MessagePort *, @segarray, lw_stack_size, pri, @name */
    BRTS

BCPL remProcess     /*  88, void */
    BRTS

BCPL ParentDir      /*  8c, FileLock ^, @FileLock */
    BRTS

BCPL setSignals     /*  90, void, &MsgPort, mask */
    BRTS

BCPL clearSignals   /* 94, BOOL , mask */
    BRTS

BCPL DOSAlert       /* 98, void, alertnumber */
    BRTS

BCPL findRootNode   /* 9c, RootNode @,  */
    BRTS

BCPL readinput      /* a0, bytelength , &buf, bytelength */
    BRTS

BCPL taskwait       /* a4, DosPacket @,  */
    BRTS

BCPL putPkt         /* a8, void, @DosPacket */
    BRTS

BCPL writeoutput    /* ac, void, &buf, bytelength */
    BRTS

BCPL packstring     /* b0, void, @Array, @String */
    BRTS

BCPL unpackstring   /* b4, void, @String, @Array */
    BRTS

BCPL holdTask       /* b8, void, @memseg */
    BRTS

BCPL Delay          /* bc, void, ? */
    BRTS

BCPL sendPacket     /* c0, void, -, Process, Type, -, -, Arg1,... */
    BRTS

BCPL returnPacket   /* c4, void, @DosPacket, res1, res2 */
    BRTS

BCPL openWindow     /* c8, void, leftedge, topedge, width, height, @title */
    BRTS

BCPL setCurDir      /* cc, FileLock @, BOOL, @FileLock */
    BRTS

BCPL systemRequest  /* d0, void, @line1, @line2, @line3 */
    BRTS

BCPL writet         /* d4, void, n */
    BRTS

BCPL rdch           /* d8, char ,  */
    BRTS

BCPL unrdch         /* dc, void,  */
    BRTS

BCPL wrch           /* e0, void, char */
    BRTS

BCPL readinput_e4   /* e4, lwlength , @buf, lwlength */
    BRTS

BCPL writeoutput_e8 /* e8, void, @buf, lwlength */
    BRTS

BCPL findinput      /* ec, FileHandle ^, &name */
    BRTS

BCPL findoutput     /* f0, FileHandle ^, &name */
    BRTS

BCPL selectinput    /* f4, void , @FileHandle */
    BRTS

BCPL selectoutput   /* f8, void , @FileHandle */
    BRTS

BCPL endwrite       /* 100, void,  */
    BRTS

BCPL Input          /* 104, FileHandle @,  */
    BRTS

BCPL Output         /* 108, FileHandle @,  */
    BRTS

BCPL readn          /* 10c, void,  */
    BRTS

BCPL newline        /* 110, void,  */
    BRTS

BCPL writei         /* 114, void, number, width */
    BRTS

BCPL writen         /* 118, void, number */
    BRTS

BCPL writehex       /* 11c, void, number, width */
    BRTS

BCPL writeoct       /* 120, void, number, width */
    BRTS

BCPL writes         /* 124, void, @string */
    BRTS

BCPL writef         /* 128, void, @format, arg1, arg2, ... */
    BRTS

BCPL toupper        /* 12c, void, char */
    BRTS

BCPL charcmp        /* 130, long , char1, char2 */
    BRTS

BCPL strcmp         /* 134, long , @String1, @String2 */
    BRTS

BCPL rdargs         /* 138, void, @template */
    BRTS

BCPL rdline         /* 13c, void,  */
    BRTS

BCPL subString      /* 140, void, @string, @substring */
    BRTS

BCPL load           /* 144, SegList ^, @filename */
    BRTS

BCPL UnLoadSeg      /* 148, void, @seg */
    BRTS

BCPL tidyup         /* 150, void,  */
    BRTS

BCPL addDevice      /* 154, void, @name */
    BRTS

BCPL datestamp      /* 158, void, @time */
    BRTS

BCPL WaitForChar    /* 15c, BOOL , @FileHandle, time */
    BRTS

BCPL execLib        /* 160, long , LVOffset, d0, d1, a0, a1, a2 */
    BRTS

BCPL findSegArray   /* 164, Array @,  */
    BRTS

BCPL deleteObject   /* 168, void, @name */
    BRTS

BCPL rename         /* 16c, void, @oldname, @newname */
    BRTS

/*      170 - contains APTR to IntuitionBase */

BCPL Close          /* 174, void, @FileHandle */
    BRTS

BCPL getword        /* 178, void, longaddress, wordnumber */
    BRTS

BCPL putword        /* 17c, void, longaddress, wordnumber */
    BRTS

BCPL taskwait_190   /* 190, DosPacket ^,  */
    BRTS

BCPL execute        /* 194, void, ? , @in, @out */
    BRTS

BCPL deviceproc     /* 198, FileLock ^, @filename */
    BRTS

BCPL libcall        /* 19c, void, libbase, LVOffset, d0, d1, a0, a1 */
    BRTS

BCPL findConsoleHandler /* 1a4, MsgPort ^,  */
    BRTS

BCPL findFileHandler    /* 1a8, MsgPort ^,  */
    BRTS

BCPL extractDeviceName  /* 1ac, void, @buf, bufsize, @name , flag */
    BRTS

BCPL lock           /* 1b0, FileLock ^, @name */
    BRTS

BCPL UnLock         /* 1b4, void, @FileLock */
    BRTS

BCPL getlong        /* 1b8, void, longoffset, @struct */
    BRTS

BCPL putlong        /* 1bc, void, longoffset, @struct, data */
    BRTS

BCPL openfile       /* 1c0, FileHandle ^, @devinfo, @filename */
    BRTS

BCPL DupLock        /* 1c4, FileLock ^, @FileLock */
    BRTS

BCPL makesysreq     /* 1c8, void, count, pkt, proc */
    BRTS

BCPL strcpy         /* 1cc, void, @src, @dest */
    BRTS

BCPL runLoadedCode  /* 1e4, void, @codeseg, stackbytes, @cmdline, cmdlinelength */
    BRTS

BCPL lock_1ec       /* 1ec, FileLock ^, @name */
    BRTS

BCPL findDevInfo    /* 1f0, DevInfo ^, @name */
    BRTS

BCPL createDir      /* 1f4, void, @name */
    BRTS

BCPL compareTime    /* 1f8, SIGN , @timeval, @timeval */
    BRTS

BCPL timerIO        /* 1fc, void, command, @IOStdReq, secs, usecs */
    BRTS

BCPL settime        /* 200, void, @timeval */
    BRTS

/*      214 - contains CLI command */

BCPL findCLI        /* 218, CommandLineInterpreter ^,  */
    BRTS

