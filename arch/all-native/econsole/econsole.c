/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emergency console launcher for AROS
    Lang: english
*/

#include <aros/debug.h>

#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>

#include <exec/resident.h>
#include <graphics/modeid.h>
#include <utility/tagitem.h>
#include <libraries/expansion.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/alib.h>
#include <proto/expansion.h>

#include LC_LIBDEFS_FILE

/********* ECON: DOS Handler
 *
 * ECON:          - interactive file, accesing the Exec/Raw* functions
 * ECON:AROS.boot - synthetic file, to allow AROS to boot from ECON:
 *                  as a last resort.
 */
static void replyPkt(struct DosPacket *dp)
{
    struct MsgPort *mp;
    struct Message *mn;

    mp = dp->dp_Port;
    mn = dp->dp_Link;
    mn->mn_Node.ln_Name = (char*)dp;
    dp->dp_Port = &((struct Process*)FindTask(NULL))->pr_MsgPort;
    PutMsg(mp, mn);
}

struct econsole_file {
    CONST_STRPTR ef_Name;
    BOOL    ef_Interactive;
    SIPTR (*ef_Read)(APTR buff, SIPTR len, SIPTR *actual);
    SIPTR (*ef_Write)(CONST_APTR buff, SIPTR len, SIPTR *actual);
    SIPTR (*ef_ExamineFH)(struct FileInfoBlock *fib, SIPTR *actual);
};

SIPTR Raw_Read(APTR buff, SIPTR len, SIPTR *err)
{
    SIPTR actual = 0;
    UBYTE *cp = buff;

    D(bug("%s: buff=%p, len=%d\n", __func__, buff, len));
    while (actual < len) {
        LONG c = RawMayGetChar();
        if (c < 0) {
            Reschedule();
            continue;
        }

        /* Trivial line editing */
        switch (c) {
        case '\b':      /* Backspace */
            if ((APTR)cp > buff) {
                actual--;
                cp--;
            }
            RawPutChar('\b');
            RawPutChar(' ');
            RawPutChar('\b');
            break;
        case '\r':
        case '\n':       /* Done */
            *(cp++) = '\n';
            len = (++actual);
            RawPutChar('\n');
            break;
        default:
            *(cp++) = (UBYTE)c;
            actual++;
            RawPutChar(c);
            break;
        }
    }

    *err = 0;
    return actual;
}

SIPTR Raw_Write(CONST_APTR buff, SIPTR len, SIPTR *err)
{
    SIPTR actual;
    CONST UBYTE *cp = buff;

    D(bug("%s: buff=%p, len=%d\n", __func__, buff, len));
    for (actual = 0; actual < len; actual++, cp++) {
        RawPutChar(*cp);
    }

    *err = 0;
    return actual;
}
    
SIPTR Raw_ExamineFH(struct FileInfoBlock *fib, SIPTR *errcode)
{
    memset(fib, 0, sizeof(fib));
    fib->fib_Protection = FIBF_WRITE | FIBF_READ;

    *errcode = 0;
    return DOSTRUE;
}
    
CONST UBYTE CONST Boot_Data[] = AROS_CPU;

SIPTR Boot_Read(APTR buff, SIPTR len, SIPTR *err)
{
    SIPTR actual;
    UBYTE *cp = buff;

    D(bug("%s: len=%d \"%s\"\n", __func__, len, Boot_Data));
    for (actual = 0; actual < len; actual++, cp++) {
        if (Boot_Data[actual] == 0)
            break;
        *cp = Boot_Data[actual];
    }

    *err = 0;
    return actual;
}

SIPTR Boot_Write(CONST_APTR buff, SIPTR len, SIPTR *err)
{
    *err = ERROR_WRITE_PROTECTED;
    return (SIPTR)-1;
}

SIPTR Boot_ExamineFH(struct FileInfoBlock *fib, SIPTR *errcode)
{
    memset(fib, 0, sizeof(fib));
    fib->fib_FileName[0] = strlen("AROS.boot");
    CopyMem("AROS.boot", &fib->fib_FileName[1], fib->fib_FileName[0] + 1);
    fib->fib_Protection = FIBF_READ;
    fib->fib_Size = sizeof(Boot_Data) - 1;

    *errcode = 0;
    return DOSTRUE;
}

const struct econsole_file econsole_files[] = {
   { .ef_Name = "", .ef_Interactive = TRUE,
     .ef_Read = Raw_Read, .ef_Write = Raw_Write,
     .ef_ExamineFH = Raw_ExamineFH
   },
   { .ef_Name = "AROS.boot", .ef_Interactive = FALSE,
     .ef_Read = Boot_Read, .ef_Write = Boot_Write,
     .ef_ExamineFH = Boot_ExamineFH
   },
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))
#endif

const struct econsole_file *econsole_file_of(BSTR bname, SIPTR *errcode)
{
    STRPTR name, file;
    int i;
    int blen = AROS_BSTR_strlen(bname);

    name = AllocMem(blen + 1, MEMF_ANY);
    CopyMem(AROS_BSTR_ADDR(bname), name, blen);
    name[blen] = 0;

    file = strchr(name, ':');
    if (file == NULL)
        file = name;
    else
        file++;

    D(bug("%s: bname=%b, name=%s, file=%s\n", __func__, bname, name, file));
    for (i = 0; i < ARRAY_SIZE(econsole_files); i++) {
        if (strcmp(econsole_files[i].ef_Name, file) == 0) {
            FreeMem(name, blen + 1);
            *errcode = 0;
            return &econsole_files[i];
        }
    }

    FreeMem(name, blen + 1);
    *errcode = ERROR_OBJECT_NOT_FOUND;
    return NULL;
}

static void econ_handler(void)
{
    struct DosPacket *dp;
    struct MsgPort *mp;
    BOOL dead = FALSE;
    struct FileHandle *fh;
    const struct econsole_file *efile;
    struct DeviceNode *dn;

    mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;

    WaitPort(mp);

    dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);

    dn = ((struct DeviceNode *)BADDR(dp->dp_Arg3));
    dn->dn_Task = mp;

    dp->dp_Res2 = 0;
    dp->dp_Res1 = DOSTRUE;

    RawIOInit();

    D(bug("%s: started\n", __func__));
    do {
        replyPkt(dp);
        WaitPort(mp);
        dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);
        D(bug("%s: type=%d\n", __func__, dp->dp_Type));

        switch (dp->dp_Type) {
        case ACTION_LOCATE_OBJECT:
            if ((efile = econsole_file_of((BSTR)dp->dp_Arg2, &dp->dp_Res2))) {
                D(bug("%s: efile=%p (%s)\n", __func__, efile, efile->ef_Name));
                struct FileLock *fl = AllocMem(sizeof(*fl), MEMF_PUBLIC | MEMF_CLEAR);
                fl->fl_Link = BNULL;
                fl->fl_Key = (IPTR)efile;
                fl->fl_Access = dp->dp_Arg3;
                fl->fl_Task = mp;
                fl->fl_Volume = MKBADDR(dn);
                dp->dp_Res1 = (SIPTR)MKBADDR(fl);
            } else {
                dp->dp_Res1 = DOSFALSE;
            }
            break;
        case ACTION_FH_FROM_LOCK:
            if (dp->dp_Arg2) {
                struct FileLock *fl = BADDR(dp->dp_Arg2);
                efile = (const struct econsole_file *)fl->fl_Key;
                D(bug("%s: efile=%p (%s)\n", __func__, efile, efile->ef_Name));
                fh = BADDR(dp->dp_Arg1);
                fh->fh_Arg1 = (SIPTR)efile;
                fh->fh_Type = mp;
                fh->fh_Interactive = efile->ef_Interactive;
                FreeMem(fl, sizeof(*fl));
                dp->dp_Res2 = 0;
            } else {
                dp->dp_Res2 = ERROR_OBJECT_NOT_FOUND;
            }
            dp->dp_Res1 = dp->dp_Res2 ? DOSFALSE : DOSTRUE;
            break;
        case ACTION_COPY_DIR_FH:
            if (dp->dp_Arg1) {
                struct FileLock *fl = AllocMem(sizeof(*fl), MEMF_PUBLIC | MEMF_CLEAR);
                efile = (const struct econsole_file *)dp->dp_Arg1;
                D(bug("%s: efile=%p (%s)\n", __func__, efile, efile->ef_Name));
                fl->fl_Link = BNULL;
                fl->fl_Key = (SIPTR)efile;
                fl->fl_Access = dp->dp_Arg3;
                fl->fl_Task = mp;
                fl->fl_Volume = MKBADDR(dn);
                dp->dp_Res1 = (SIPTR)MKBADDR(fl);
                dp->dp_Res2 = 0;
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_NOT_FOUND;
            }
            break;
        case ACTION_FREE_LOCK:
            if (dp->dp_Arg1) {
                struct FileLock *fl = BADDR(dp->dp_Arg1);
                FreeMem(fl, sizeof(*fl));
            }
            dp->dp_Res1 = DOSTRUE;
            dp->dp_Res2 = 0;
            break;
        case ACTION_FINDINPUT:
        case ACTION_FINDOUTPUT:
        case ACTION_FINDUPDATE:
            if ((efile = econsole_file_of((BSTR)dp->dp_Arg3, &dp->dp_Res2))) {
                D(bug("%s: efile=%p (%s)\n", __func__, efile, efile->ef_Name));
                fh = BADDR(dp->dp_Arg1);
                fh->fh_Arg1 = (SIPTR)efile;
                fh->fh_Type = mp;
                fh->fh_Interactive = efile->ef_Interactive;
            }
            dp->dp_Res1 = efile ? DOSTRUE : DOSFALSE;
            break;
        case ACTION_READ:
            if ((efile = (const struct econsole_file *)dp->dp_Arg1)) {
                D(bug("%s: efile=%p (%s)\n", __func__, efile, efile->ef_Name));
                dp->dp_Res1 = efile->ef_Read((APTR)dp->dp_Arg2, dp->dp_Arg3, &dp->dp_Res2);
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
            }
            break;
        case ACTION_WRITE:
            if ((efile = (const struct econsole_file *)dp->dp_Arg1)) {
                D(bug("%s: efile=%p (%s)\n", __func__, efile, efile->ef_Name));
                dp->dp_Res1 = efile->ef_Write((CONST_APTR)dp->dp_Arg2, dp->dp_Arg3, &dp->dp_Res2);
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
            }
            break;
        case ACTION_EXAMINE_FH:
            if ((efile = (const struct econsole_file *)dp->dp_Arg1)) {
                dp->dp_Res1 = efile->ef_ExamineFH((struct FileInfoBlock *)BADDR(dp->dp_Arg2), &dp->dp_Res2);
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
            }
            break;
        case ACTION_END:
            dp->dp_Res1 = DOSTRUE;
            dp->dp_Res2 = 0;
            break;
        case ACTION_DIE:
            dp->dp_Res1 = DOSTRUE;
            dp->dp_Res2 = 0;
            dead = TRUE;
            break;
        default:
            dp->dp_Res1 = DOSFALSE;
            dp->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
            break;
        }
        D(bug("%s: Type %d, Res1=%p, Res2=%p\n", __func__, dp->dp_Type, dp->dp_Res1, dp->dp_Res2));
    } while (!dead);

    /* ACTION_DIE ends up here... */
    D(bug("%s: Exiting\n", __func__));

    replyPkt(dp);
}
            
static BOOL EConsole_Init(void)
{
    struct Library *ExpansionBase;

    if ((ExpansionBase = TaggedOpenLibrary(TAGGEDOPEN_EXPANSION))) {
        IPTR pp[] = {
            (IPTR)"ECON",
            (IPTR)NULL,         /* device */
            (IPTR)0,            /* unit */
            (IPTR)0,            /* flags */
            (IPTR)DE_TABLESIZE, /* DE_TABLESIZE */
        };
        struct DeviceNode *dn;

        if ((dn = MakeDosNode(pp))) {
            dn->dn_SegList   = CreateSegList(econ_handler);
            dn->dn_GlobalVec = (BPTR)(SIPTR)-1;

            D(bug("%s: AddBootNode: %p\n", __func__, dn));
            AddBootNode(-127, ADNF_STARTPROC, dn, NULL);

            CloseLibrary(ExpansionBase);
            return TRUE;
        }

        CloseLibrary(ExpansionBase);
    }

    D(bug("%s: failed\n", __func__));
    return FALSE;
}

ADD2INIT(EConsole_Init, 0)
