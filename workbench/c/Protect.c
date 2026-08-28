/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Protect CLI command
*/

/*****************************************************************************

    NAME

        Protect

    SYNOPSIS

        FILE/A,FLAGS,ADD/S,SUB/S,ALL/S,QUIET/S,GROUP/K,OTHER/K

    LOCATION

        C:

    FUNCTION

        Add or remove protection bits from a file or directory.
        
        Protect allows the use of pattern matching and recursive directory
        scans to protect many files/directories at any one time.

    INPUTS

        FILE   --  Either a file, a directory or a pattern to match.
        FLAGS  --  One or more of the following flags:

                    S - Script
                    P - Pure
                    A - Archive
                    R - Read
                    W - Write
                    E - Execute
                    D - Delete

        ADD    --  Allows the bits to be set and hence allowable.
        SUB    --  Allows the bits to be cleared and hence not allowable.
        ALL    --  Allows a recursive scan of the volume/directory.
        QUIET  --  Suppresses any output to the shell.

    RESULT

        Standard DOS return codes.

    NOTES

    EXAMPLE

        Protect ram: e add all

            Recurses the ram: volume and attaches the executable bit.

    BUGS

    SEE ALSO

        dos.library/SetProtection()

    INTERNALS

******************************************************************************/

#define DEBUG 0

#include <aros/debug.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include <dos/rdargs.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <utility/utility.h>

#include <proto/arossupport.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/security.h>
#include <libraries/security.h>

/* Multi-user support: GROUP/OTHER bits, security.library when resident */
struct Library *secBase = NULL;
static LONG grpMask = 0, otrMask = 0;       /* FIBF_GRP_#? / FIBF_OTR_#? bits to set */
static BOOL grpSet = FALSE, otrSet = FALSE;
static BOOL grpAdd = FALSE, grpSub = FALSE, otrAdd = FALSE, otrSub = FALSE;

/* Parse "[+|-]rwed" into active-high bits shifted to 'shift' */
static BOOL parseRWED(STRPTR s, LONG shift, LONG *mask, BOOL *add, BOOL *sub)
{
    *mask = 0;
    if (*s == '+') { *add = TRUE; s++; }
    else if (*s == '-') { *sub = TRUE; s++; }
    for (; *s; s++)
    {
        switch (ToUpper(*s))
        {
        case 'R': *mask |= FIBF_READ << shift; break;
        case 'W': *mask |= FIBF_WRITE << shift; break;
        case 'E': *mask |= FIBF_EXECUTE << shift; break;
        case 'D': *mask |= FIBF_DELETE << shift; break;
        default:
            Printf("Invalid GROUP/OTHER flags - must be one of RWED\n");
            return FALSE;
        }
    }
    return TRUE;
}

#define CTRL_C         (SetSignal(0L,0L) & SIGBREAKF_CTRL_C)

#define Bit_Mask(bit)           (1L << bit)
#define Bit_Clear(name, bit)    name &= ~Bit_Mask(bit)
#define Bit_Set(name, bit)      name |= Bit_Mask(bit)

#define ARG_TEMPLATE    "FILE/A,FLAGS,ADD/S,SUB/S,ALL/S,QUIET/S,GROUP/K,OTHER/K"

#ifndef FIBB_HOLD
#define FIBB_HOLD 7
#endif
#ifndef FIBF_HOLD
#define FIBF_HOLD (1<<FIBB_HOLD)
#endif

enum
{
    ARG_FILE = 0,
    ARG_FLAGS,
    ARG_ADD,
    ARG_SUB,
    ARG_ALL,
    ARG_QUIET,
    ARG_GROUP,
    ARG_OTHER,
    NOOFARGS
};


/* To define whether a command line switch was set or not.
 */
#define NOT_SET(x)      (x == 0)
#define IS_SET(x)       (!NOT_SET(x))

#define MAX_PATH_LEN    512

const TEXT version[] = "$VER: Protect 41.1 (2.12.2000)\n";

int Do_Protect(struct AnchorPath *, STRPTR, STRPTR, BOOL, BOOL, BOOL, BOOL);

int doProtect(struct AnchorPath *ap, STRPTR file, LONG flags, BOOL flagsSet,
              BOOL add, BOOL sub, BOOL all, BOOL quiet);
BOOL setProtection(STRPTR file, LONG oldFlags, LONG flags, BOOL flagsSet,
                  BOOL add, BOOL sub);

int __nocommandline;

int main(void)
{
    struct RDArgs     *rda;
    struct AnchorPath *apath;

    IPTR args[NOOFARGS] = { 0,
                            0,
                            (IPTR)FALSE,
                            (IPTR)FALSE,
                            (IPTR)FALSE,
                            (IPTR)FALSE };

    int retval = RETURN_OK;

    apath = AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN,
                     MEMF_ANY | MEMF_CLEAR);

    if (apath != NULL)
    {
        /* Make sure DOS knows the buffer size. */
        apath->ap_Strlen = MAX_PATH_LEN;

        rda = ReadArgs(ARG_TEMPLATE, args, NULL);

        if (rda != NULL)
        {
            STRPTR  file = (STRPTR)args[ARG_FILE];
            STRPTR  flags = (STRPTR)args[ARG_FLAGS];
            BOOL    add = (BOOL)args[ARG_ADD];
            BOOL    sub = (BOOL)args[ARG_SUB];
            BOOL    all = (BOOL)args[ARG_ALL];
            BOOL    quiet = (BOOL)args[ARG_QUIET];

            /* GROUP/OTHER only exist on multi-user systems (security.library resident) */
            if (FindResident(SECURITYNAME))
                secBase = OpenLibrary(SECURITYNAME, 0);
            if ((args[ARG_GROUP] || args[ARG_OTHER]) && !secBase)
            {
                Printf("GROUP and OTHER require security.library (multi-user system)\n");
                retval = RETURN_FAIL;
            }
            else if (args[ARG_GROUP])
            {
                grpSet = parseRWED((STRPTR)args[ARG_GROUP], FIBB_GRP_DELETE, &grpMask, &grpAdd, &grpSub);
                if (!grpSet) retval = RETURN_FAIL;
            }
            if (args[ARG_OTHER] && secBase)
            {
                otrSet = parseRWED((STRPTR)args[ARG_OTHER], FIBB_OTR_DELETE, &otrMask, &otrAdd, &otrSub);
                if (!otrSet) retval = RETURN_FAIL;
            }

            LONG    flagValues = FIBF_READ | FIBF_WRITE | FIBF_DELETE |
                                 FIBF_EXECUTE;

            if (flags != NULL)
            {
                D(Printf("Flags: %s\n", flags));

                if (*flags == '+')
                {
                        add = TRUE;
                        flags++;
                }
                if (*flags == '-')
                {
                        sub = TRUE;
                        flags++;
                }

                while (*flags != 0 && retval == RETURN_OK)
                {
                    char f = ToUpper(*flags);

                    D(Printf("Checking flag: %lc\n", f));
                    switch (f)
                    {
                        /* Active low */
                        case 'R':
                            flagValues &= ~FIBF_READ;
                            break;

                        case 'W':
                            flagValues &= ~FIBF_WRITE;
                            break;

                        case 'D':
                            flagValues &= ~FIBF_DELETE;
                            break;

                        case 'E':
                            flagValues &= ~FIBF_EXECUTE;
                            break;

                            /* Active high */
                        case 'A':
                            flagValues |= FIBF_ARCHIVE;
                            break;

                        case 'S':
                            flagValues |= FIBF_SCRIPT;
                            break;

                        case 'P':
                            flagValues |= FIBF_PURE;
                            break;

                        case 'H':
                            flagValues |= FIBF_HOLD;
                            break;
                            
                        default:
                            Printf("Invalid flags - must be one of HSPARWED\n");
                            retval = RETURN_FAIL;
                    }

                    flags++;
                } /* while (*flags != 0) */
            }

            if (add && sub)
            {
                Printf("ADD and SUB are mutually exclusive\n");
                retval = RETURN_FAIL;
            }

            if (retval == RETURN_OK)
            {
                if (!all && IsDosEntryA(file, LDF_VOLUMES | LDF_DEVICES))
                {
                    Printf("Can't set protection for %s - ", file);
                    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
                    PrintFault(IoErr(), NULL);

                    retval = RETURN_FAIL;
                }
                else
                {
                    retval = doProtect(apath, file, flagValues,
                                       flags != NULL, add, sub, all,
                                       quiet);
                }
            }

            FreeArgs(rda);
        }
        else
        {
            PrintFault(IoErr(), "Protect");
            retval = RETURN_FAIL;
        }
    }
    else
    {
        retval = RETURN_FAIL;
    }

    FreeVec(apath);

    return retval;
}

#define  isDir(fib) ((fib)->fib_DirEntryType >= 0)

int doProtect(struct AnchorPath *ap, STRPTR file, LONG flags, BOOL flagsSet,
              BOOL add, BOOL sub, BOOL all, BOOL quiet)
{
    LONG  match;
    int   retval = RETURN_OK;
    LONG  indent = 0;
    int   i;                    /* Loop variable */
    BOOL  success;
    ULONG match_count = 0;

    for (match = MatchFirst(file, ap);
         match == 0 && retval == RETURN_OK && !CTRL_C;
         match = MatchNext(ap))
    {
        match_count++;
        if (isDir(&ap->ap_Info))
        {
            if (ap->ap_Flags & APF_DIDDIR)
            {
                indent--;
                ap->ap_Flags &= ~APF_DIDDIR; /* Should not be necessary */
                continue;
            }
            else if (all)
            {
                ap->ap_Flags |= APF_DODIR;
                indent++;
            }


        }

        success = setProtection(ap->ap_Buf, ap->ap_Info.fib_Protection, flags,
                              flagsSet, add, sub);

        if (!quiet)
        {
            LONG ioerr = IoErr();

            /* Fix indentation level */
            for (i = 0; i < indent; i++)
            {
                PutStr("     ");
            }

            if (!isDir(&ap->ap_Info))
            {
                PutStr("   ");
            }

            PutStr(ap->ap_Info.fib_FileName);

            if (isDir(&ap->ap_Info))
            {
                PutStr(" (dir)");
            }

            if (!success)
            {
                PrintFault(ioerr, "..error");
                retval = RETURN_ERROR;
            }
            else
            {
                PutStr("..done\n");
            }
        }
    }

    if (match_count == 0 || IoErr() != ERROR_NO_MORE_ENTRIES)
    {
        PrintFault(IoErr(), NULL);
        retval = RETURN_ERROR;
    }

    MatchEnd(ap);

    return retval;
}

#define  ALL_OFF  (FIBF_READ | FIBF_WRITE | FIBF_DELETE | FIBF_EXECUTE)
#define addFlags(new, old)  ((~(~old | ~new) & ALL_OFF) | \
                              ((old | new) & ~ALL_OFF))
     
#define  subFlags(new, old)  (((old | ~new) & ALL_OFF) | \
                              ((old & ~new) & ~ALL_OFF))
     
BOOL setProtection(STRPTR file, LONG oldFlags, LONG flags, BOOL flagsSet,
                   BOOL add, BOOL sub)
{
    LONG  newFlags;
    
    if (flags != ALL_OFF || grpSet || otrSet)
    {
        if (add)
        {
            /* Enable permission */
            newFlags = addFlags(flags, oldFlags);
        }
        else if (sub)
        {
            /* Disable permissions */
            newFlags = subFlags(flags, oldFlags);
        }
        else if (flags == ALL_OFF && !flagsSet)
        {
            /* Only GROUP/OTHER given: leave the owner bits alone */
            newFlags = oldFlags;
        }
        else
        {
            /* Clear all permissions then set the ones given. */
            newFlags = flags;
        }
    }
    else
    {
        /* No flags were given */
        newFlags = oldFlags;
        if (!add && !sub)
        {
            /* Disable all permissions */
            newFlags = ALL_OFF;
        }
        else
        {
            /* Do nothing */
            return FALSE;
        }
    }

    /* GROUP and OTHER bits are active high; ADD/SUB from the switches or the
       +/- prefix of the GROUP/OTHER argument */
    if (grpSet)
    {
        LONG grpBits = FIBF_GRP_READ | FIBF_GRP_WRITE | FIBF_GRP_EXECUTE | FIBF_GRP_DELETE;
        if (grpAdd || (add && !grpSub))
            newFlags |= grpMask;
        else if (grpSub || sub)
            newFlags &= ~grpMask;
        else
            newFlags = (newFlags & ~grpBits) | grpMask;
    }
    if (otrSet)
    {
        LONG otrBits = FIBF_OTR_READ | FIBF_OTR_WRITE | FIBF_OTR_EXECUTE | FIBF_OTR_DELETE;
        if (otrAdd || (add && !otrSub))
            newFlags |= otrMask;
        else if (otrSub || sub)
            newFlags &= ~otrMask;
        else
            newFlags = (newFlags & ~otrBits) | otrMask;
    }

    /* On a multi-user system use the unrestricted call, so that the GROUP/OTHER
       bits are not filtered by LIMITDOSSETPROTECTION */
    if (secBase)
        return secSetProtection(file, newFlags) ? TRUE : FALSE;
    if (!SetProtection(file, newFlags))
    {
        return FALSE;
    }
    return TRUE;
}
