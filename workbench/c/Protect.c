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
                    H - Hold
                    R - Read
                    W - Write
                    E - Execute
                    D - Delete

        ADD    --  Allows the bits to be set and hence allowable.
        SUB    --  Allows the bits to be cleared and hence not allowable.
        ALL    --  Allows a recursive scan of the volume/directory.
        QUIET  --  Suppresses progress output; errors are still reported.
        GROUP  --  Group RWED permissions, available with security.library.
                  Prefix + to add, - to remove; otherwise replace.
        OTHER  --  Other-user RWED permissions, with the same syntax.

    RESULT

        Standard DOS return codes.

    NOTES

        Owner RWED bits are active low; group and other RWED bits
        are active high. FLAGS without ADD or SUB replace the
        classic protection flags only. Extended rights are preserved
        unless GROUP or OTHER explicitly requests their change.
        An omitted FLAGS argument with no GROUP or OTHER denies
        owner RWED access and clears the classic attributes.

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

/* Parse one GROUP/OTHER RWED argument into the active-high FIBF mask.
 * A leading '+' or '-' selects additive or subtractive updates; an absent
 * prefix requests replacement. Empty and invalid flag lists are rejected.
 * Caller supplies the relevant group/other bit offset and output storage.
 * Returns FALSE after reporting invalid input, without changing a file.
 */
static BOOL parseRWED(STRPTR s, LONG shift, LONG *mask, BOOL *add, BOOL *sub)
{
    *mask = 0;
    if (*s == '+') { *add = TRUE; s++; }
    else if (*s == '-') { *sub = TRUE; s++; }
    if (!*s)
    {
        Printf("GROUP/OTHER flags must contain at least one of RWED\n");
        return FALSE;
    }
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


#define MAX_PATH_LEN    512

const TEXT version[] = "$VER: Protect 41.2 (28.10.2026)\n";

int doProtect(struct AnchorPath *ap, STRPTR file, LONG flags, BOOL flagsSet,
              BOOL add, BOOL sub, BOOL all, BOOL quiet);
BOOL setProtection(STRPTR file, LONG oldFlags, LONG flags, BOOL flagsSet,
                  BOOL add, BOOL sub);

int __nocommandline;

/* The Shell entry point uses the Protect command contract documented above.
 * Parse errors return RETURN_FAIL before directory matching or mutation;
 * the optional security.library handle is closed on every exit path.
 */
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
        apath->ap_BreakBits = SIGBREAKF_CTRL_C;

        rda = ReadArgs(ARG_TEMPLATE, args, NULL);

        if (rda != NULL)
        {
            STRPTR  file = (STRPTR)args[ARG_FILE];
            STRPTR  flags = (STRPTR)args[ARG_FLAGS];
            BOOL    add = (BOOL)args[ARG_ADD];
            BOOL    sub = (BOOL)args[ARG_SUB];
            BOOL    all = (BOOL)args[ARG_ALL];
            BOOL    quiet = (BOOL)args[ARG_QUIET];
            BOOL    hasFlags = FALSE;

            /* Only explicit GROUP/OTHER changes need the unrestricted API. */
            if (args[ARG_GROUP] || args[ARG_OTHER])
            {
                if (FindResident(SECURITYNAME))
                    secBase = OpenLibrary(SECURITYNAME, 0);
                if (!secBase)
                {
                    Printf("GROUP and OTHER require security.library (multi-user system)\n");
                    retval = RETURN_FAIL;
                }
            }
            if (retval == RETURN_OK && args[ARG_GROUP])
            {
                grpSet = parseRWED((STRPTR)args[ARG_GROUP], FIBB_GRP_DELETE, &grpMask, &grpAdd, &grpSub);
                if (!grpSet) retval = RETURN_FAIL;
            }
            if (retval == RETURN_OK && args[ARG_OTHER])
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

                    if (retval == RETURN_OK) hasFlags = TRUE;
                    flags++;
                } /* while (*flags != 0) */
            }

            if (add && sub)
            {
                Printf("ADD and SUB are mutually exclusive\n");
                retval = RETURN_FAIL;
            }
            if ((add || sub) && !hasFlags && !grpSet && !otrSet)
            {
                Printf("ADD or SUB requires protection flags\n");
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
        PrintFault(ERROR_NO_FREE_STORE, "Protect");
        retval = RETURN_FAIL;
    }

    if (secBase)
    {
        CloseLibrary(secBase);
        secBase = NULL;
    }
    FreeVec(apath);

    return retval;
}

#define  isDir(fib) ((fib)->fib_DirEntryType >= 0)

/* Walk the DOS pattern rooted at file and update each matched entry.
 * ap owns the AnchorPath storage and is passed to MatchFirst/MatchNext;
 * MatchEnd releases the matching state before returning. When all is set,
 * request traversal of matched directories via APF_DODIR.
 * Stop on the first SetProtection failure, matching failure or Ctrl-C;
 * return RETURN_OK, RETURN_WARN for a break, or RETURN_ERROR for failures.
 * Quiet mode suppresses per-entry progress, not diagnostics.
 */
int doProtect(struct AnchorPath *ap, STRPTR file, LONG flags, BOOL flagsSet,
              BOOL add, BOOL sub, BOOL all, BOOL quiet)
{
    LONG match = MatchFirst(file, ap);
    int retval = RETURN_OK;
    LONG indent = 0;
    ULONG match_count = 0;

    while (match == 0)
    {
        LONG fileError = 0;
        BOOL success;

        if (CTRL_C)
        {
            match = ERROR_BREAK;
            break;
        }

        if (isDir(&ap->ap_Info))
        {
            if (ap->ap_Flags & APF_DIDDIR)
            {
                if (indent > 0) indent--;
                ap->ap_Flags &= ~APF_DIDDIR;
                match = MatchNext(ap);
                continue;
            }
            if (all)
            {
                ap->ap_Flags |= APF_DODIR;
                indent++;
            }
        }

        match_count++;
        success = setProtection(ap->ap_Buf, ap->ap_Info.fib_Protection,
                                flags, flagsSet, add, sub);
        if (!success)
            fileError = IoErr();

        if (!quiet)
        {
            int i;

            for (i = 0; i < indent; i++)
                PutStr("     ");
            if (!isDir(&ap->ap_Info))
                PutStr("   ");
            PutStr(ap->ap_Info.fib_FileName);
            if (isDir(&ap->ap_Info))
                PutStr(" (dir)");
            if (success)
                PutStr("..done\n");
            else
                PutStr("..error\n");
        }

        if (!success)
        {
            if (fileError)
                PrintFault(fileError, "Protect");
            else
                Printf("Protect: unable to set protection for %s\n", ap->ap_Buf);
            retval = RETURN_ERROR;
            break;
        }
        match = MatchNext(ap);
    }

    if (retval == RETURN_OK)
    {
        if (match == ERROR_NO_MORE_ENTRIES)
        {
            if (match_count == 0)
            {
                PrintFault(ERROR_OBJECT_NOT_FOUND, "Protect");
                retval = RETURN_ERROR;
            }
        }
        else
        {
            PrintFault(match, "Protect");
            retval = match == ERROR_BREAK ? RETURN_WARN : RETURN_ERROR;
        }
    }

    MatchEnd(ap);
    return retval;
}

#define  ALL_OFF  (FIBF_READ | FIBF_WRITE | FIBF_DELETE | FIBF_EXECUTE)
#define  CLASSIC_MASK (ALL_OFF | FIBF_ARCHIVE | FIBF_PURE | FIBF_SCRIPT | FIBF_HOLD)
#define addFlags(new, old)  ((~(~old | ~new) & ALL_OFF) | \
                              ((old | new) & ~ALL_OFF))
     
#define  subFlags(new, old)  (((old | ~new) & ALL_OFF) | \
                              ((old & ~new) & ~ALL_OFF))
     
/* Compute and apply the effective protection bits for one DOS path.
 * oldFlags is the protection word supplied by MatchFirst/MatchNext.
 * Owner RWED is active-low; GROUP/OTHER is active-high. Preserve unrelated
 * bits on replacement and use security.library only for requested extended
 * rights changes; otherwise use dos.library/SetProtection().
 * Returns the selected API's success status; the caller checks IoErr() on
 * failure. Does not own file, oldFlags or any security.library handle.
 */
BOOL setProtection(STRPTR file, LONG oldFlags, LONG flags, BOOL flagsSet,
                   BOOL add, BOOL sub)
{
    LONG newFlags;

    /* Owner flags are active low. Do not erase unrelated protection bits. */
    if (flagsSet)
    {
        if (add)
            newFlags = addFlags(flags, oldFlags);
        else if (sub)
            newFlags = subFlags(flags, oldFlags);
        else
            newFlags = (oldFlags & ~CLASSIC_MASK) | flags;
    }
    else if (grpSet || otrSet)
    {
        newFlags = oldFlags;
    }
    else
    {
        /* No FLAGS argument: retain extended rights, deny owner RWED. */
        newFlags = (oldFlags & ~CLASSIC_MASK) | ALL_OFF;
    }

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

    /* Only explicit GROUP/OTHER changes require unrestricted setting. */
    if (grpSet || otrSet)
        return secSetProtection(file, newFlags) ? TRUE : FALSE;
    return SetProtection(file, newFlags) ? TRUE : FALSE;
}
