/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Protect CLI command
    Lang: english
*/

/*****************************************************************************

    NAME

        Protect

    SYNOPSIS

        FILE/A,FLAGS,ADD/S,SUB/S,ALL/S,QUIET/S

    LOCATION

        Workbench:c

    FUNCTION

        Add or remove protection bits from a file or directory.
        
        Protect allows the use of pattern matching and recursive directory
        scans to protect many files/directories at any one time.

    INPUTS

        FILE    - Either a file, a directory or a pattern to match.

        FLAGS   - One or more of the following flags:

                    S - Script
                    P - Pure
                    A - Archive
                    R - Read
                    W - Write
                    E - Execute
                    D - Delete

        ADD     - Allows the bits to be set and hence allowable.

        SUB     - Allows the bits to be cleared and hence not allowable.

        ALL     - Allows a recursive scan of the volume/directory.

        QUIET   - Suppresses any output to the shell.

    RESULT

        Standard DOS return codes.

    NOTES

        Does not yet support multi-assigns.

        Does not yet support User Groups.

    EXAMPLE

        Protect ram: e add all

            Recurses the ram: volume and attaches the executable bit.

    BUGS

    SEE ALSO

        SetProtection()

    INTERNALS

    HISTORY

        27-Jul-1997     laguest     Initial inclusion into the AROS tree

******************************************************************************/

#include <proto/arossupport.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <utility/utility.h>

#define Bit_Mask(bit)           (1L << bit)
#define Bit_Clear(name, bit)    name &= ~Bit_Mask(bit)
#define Bit_Set(name, bit)      name |= Bit_Mask(bit)

#define ARG_TEMPLATE    "FILE/A,FLAGS,ADD/S,SUB/S,ALL/S,QUIET/S"
#define ARG_FILE        0
#define ARG_FLAGS       1
#define ARG_ADD         2
#define ARG_SUB         3
#define ARG_ALL         4
#define ARG_QUIET       5
#define TOTAL_ARGS      6

/* To define whether a command line switch was set or not.
 */
#define NOT_SET(x)      (x == 0)
#define IS_SET(x)       (!NOT_SET(x))

#define MAX_PATH_LEN    512

static const char version[] = "$VER: Protect 41.0 (24.6.1997)\n";

struct UtilityBase *UtilityBase;

int Do_Protect(struct AnchorPath *, STRPTR, STRPTR, LONG, LONG, LONG, LONG);

int main(int argc, char *argv[])
{
    struct RDArgs     * rda;
    struct AnchorPath * apath;
    IPTR              * args[TOTAL_ARGS] = { NULL, NULL, NULL, NULL, NULL, NULL };
    int                 Return_Value;

    Return_Value = RETURN_OK;

    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 39);
    if (UtilityBase != NULL)
    {
        apath = AllocVec(sizeof(struct AnchorPath) + MAX_PATH_LEN,
                         MEMF_ANY | MEMF_CLEAR);
        if (apath)
        {
            /* Make sure DOS knows the buffer size.
             */
            apath->ap_Strlen = MAX_PATH_LEN;

            /* The cast in ReadArgs() stops the compiler from complaining.
             */
            rda = ReadArgs(ARG_TEMPLATE, (LONG *)args, NULL);
            if (rda) {
                Return_Value = Do_Protect(apath,
                                          (STRPTR)args[ARG_FILE],
                                          (STRPTR)args[ARG_FLAGS],
                                          (LONG)args[ARG_ADD],
                                          (LONG)args[ARG_SUB],
                                          (LONG)args[ARG_ALL],
                                          (LONG)args[ARG_QUIET]
                );
            }
            else
            {
                PrintFault(IoErr(), "Protect");
                Return_Value = RETURN_FAIL;
            }
    
            FreeArgs(rda);
        }
        else
        {
            Return_Value = RETURN_FAIL;
        }

        FreeVec(apath);

        CloseLibrary((struct Library *)UtilityBase);
    }
    else
    {
        VPrintf("Need \'utility.library\' version 39 or above\n", NULL);
        Return_Value = RETURN_FAIL;
    }

    return (Return_Value);

} /* main */


/* Defines whether MatchFirst(), etc has matched a file.
 */
#define MATCHED_FILE    0

int Do_Recursion(struct AnchorPath *, LONG, LONG, LONG *, LONG);
void AddBitMask(struct AnchorPath *, STRPTR, LONG *, BOOL);
void SubBitMask(struct AnchorPath *, STRPTR, LONG *, BOOL);
void NewBitMask(struct AnchorPath *, STRPTR, LONG *);
void PrintFileName(struct AnchorPath *, LONG);
int SafeSetProtection(struct AnchorPath *, LONG);

int Do_Protect(struct AnchorPath *a,
               STRPTR File,
               STRPTR Flags,
               LONG Add,
               LONG Sub,
               LONG All,
               LONG Quiet)
{
    LONG Result,
         TabValue,
         BitMask;
    int  Return_Value;
    IPTR DevArg[2];

    Return_Value = RETURN_OK;
    TabValue     = 0L;
    BitMask      = 0L;

    if
    (
        IsDosEntryA((STRPTR)File, LDF_VOLUMES | LDF_DEVICES) == TRUE
    &&
        NOT_SET(All)
    )
    {
        DevArg[0] = (IPTR)File;

        VPrintf("Can't set protection for %s - ", DevArg);
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
        PrintFault(IoErr(), NULL);
        
        Return_Value = RETURN_FAIL;
    }
    else
    {
        Result = MatchFirst((STRPTR)File, a);

        if (Result == MATCHED_FILE)
        {
            do
            {
                if (Flags != NULL)  /* Flags are given */
                {
                    if (IS_SET(Add))
                    {
                        /* Add the permission bits to the bitmask.
                         * In other words - Enable permissions.
                         */
    
                        AddBitMask(a, Flags, &BitMask, TRUE);
                        Return_Value = Do_Recursion(a, All, Quiet, &TabValue, BitMask);
                    }
                    else if (IS_SET(Sub))
                    {
                        /* Remove the permission bits from the bitmask.
                         * In other words - Disable permissions.
                         */

                        SubBitMask(a, Flags, &BitMask, TRUE);
                        Return_Value = Do_Recursion(a, All, Quiet, &TabValue, BitMask);
                    }
                    else
                    {
                        /* Clear all permissions then set the ones given.
                         */

                        NewBitMask(a, Flags, &BitMask);
                        Return_Value = Do_Recursion(a, All, Quiet, &TabValue, BitMask);
                    }
                }
                else    /* No flags are given */
                {
                    if (NOT_SET(Add) && NOT_SET(Sub))
                    {
                        /* Remove all permissions bits from the bitmask.
                         * In other words - Disable ALL permissions.
                         */

                        SubBitMask(a, "sparwed", &BitMask, TRUE);
                        Return_Value = Do_Recursion(a, All, Quiet, &TabValue, BitMask);
                    }
                }
            }
            while
            (
                ((Result = MatchNext(a)) == MATCHED_FILE)
            &&
                Return_Value != RETURN_FAIL
            );
        }
        else
        {
            PrintFault(IoErr(), "Protect");
            Return_Value = RETURN_FAIL;
        }

        MatchEnd(a);
    }

    return (Return_Value);
    
} /* Do_Protect */


int Do_Recursion(struct AnchorPath *a,
                  LONG  All,
                  LONG  Quiet,
                  LONG *TabValue,
                  LONG  BitMask)
{
    BOOL Return_Value;

    Return_Value = RETURN_OK;

    if (NOT_SET(All))
    {
        Return_Value = SafeSetProtection(a, BitMask);
        if (NOT_SET(Quiet))
        {
            PrintFileName(a, *TabValue);
        }
    }    
    else
    {
        /* Allow a recursive scan.
         */
            
        if (a->ap_Info.fib_DirEntryType > 0)
        {
            /* Enter directory.
             */
            if (!(a->ap_Flags & APF_DIDDIR))
            {
                a->ap_Flags |= APF_DODIR;

                Return_Value = SafeSetProtection(a, BitMask);

                if (NOT_SET(Quiet))
                {
                    PrintFileName(a, *TabValue);
                }
                (*TabValue)++;
            }
            else
            {
                /* Leave directory.
                 */

                a->ap_Flags &= ~APF_DIDDIR;
                (*TabValue)--;
            }
        }
        else
        {
            Return_Value = SafeSetProtection(a, BitMask);
            if (NOT_SET(Quiet))
            {
                PrintFileName(a, *TabValue);
            }
        }
    }

    return (Return_Value);

} /* Do_Recursion */


/* AddBitMask
 *
 * Create a bitmask that unsets the appropriate bits.
 */
void AddBitMask(struct AnchorPath *a, STRPTR f, LONG *b, BOOL copy)
{
    int LoopCount;

    LoopCount = 0;

    if (copy == TRUE)
    {
        *b        = a->ap_Info.fib_Protection;
    }

    while (f[LoopCount] != NULL)
    {
        /* The SetProtect() function uses a clear bit to denote
         * that one of the RWED permissions are allowed.
         *
         * It also uses a set bit to denote that one of the
         * SPA permissions are allowed.
         */

        switch (f[LoopCount])
        {
            case 's':   /* Script */
                Bit_Set(*b, FIBB_SCRIPT);
                break;
            case 'p':   /* Pure */
                Bit_Set(*b, FIBB_PURE);
                break;
            case 'a':   /* Archive */
                Bit_Set(*b, FIBB_ARCHIVE);
                break;
            case 'r':   /* Read */
                Bit_Clear(*b, FIBB_READ);
                break;
            case 'w':   /* Write */
                Bit_Clear(*b, FIBB_WRITE);
                break;
            case 'e':   /* Execute */
                Bit_Clear(*b, FIBB_EXECUTE);
                break;
            case 'd':   /* Delete */
                Bit_Clear(*b, FIBB_DELETE);
                break;
            default:
                break;
        }

        LoopCount++;
    }
} /* AddBitMask */


/* SubBitMask
 *
 * Create a bitmask that sets the appropriate bits.
 */
void SubBitMask(struct AnchorPath *a, STRPTR f, LONG *b, BOOL copy)
{
    int LoopCount;

    LoopCount = 0;

    if (copy == TRUE)
    {
        *b        = a->ap_Info.fib_Protection;
    }

    while (f[LoopCount] != NULL)
    {
        /* The SetProtect() function uses a set bit to denote
         * that one of the RWED permissions is not allowed.
         *
         * It also uses a set bit to denote that one of the
         * SPA permissions is not allowed.
         */

        switch (f[LoopCount])
        {
            case 's':   /* Script */
                Bit_Clear(*b, FIBB_SCRIPT);
                break;
            case 'p':   /* Pure */
                Bit_Clear(*b, FIBB_PURE);
                break;
            case 'a':   /* Archive */
                Bit_Clear(*b, FIBB_ARCHIVE);
                break;
            case 'r':   /* Read */
                Bit_Set(*b, FIBB_READ);
                break;
            case 'w':   /* Write */
                Bit_Set(*b, FIBB_WRITE);
                break;
            case 'e':   /* Execute */
                Bit_Set(*b, FIBB_EXECUTE);
                break;
            case 'd':   /* Delete */
                Bit_Set(*b, FIBB_DELETE);
                break;
            default:
                break;
        }

        LoopCount++;
    }
} /* SubBitMask */


/* NewBitMask
 *
 * Create a new bitmask that has the appropriate bits set.
 */
void NewBitMask(struct AnchorPath *a, STRPTR f, LONG *b)
{
    int LoopCount;

    LoopCount = 0;

    SubBitMask(a, "sparwed", b, TRUE);
    AddBitMask(a, f, b, FALSE);
} /* NewBitMask */


void PrintFileName(struct AnchorPath *a, LONG t)
{
    int i;
    IPTR args[2];

    args[0] = (IPTR)FilePart(&a->ap_Buf[0]);
    args[1] = (IPTR)NULL;

    VPrintf("   ", NULL);

    for (i = 0; i != t; i++)
    {
        VPrintf("  ", NULL);
    }

    VPrintf("%s", args);

    if (a->ap_Info.fib_DirEntryType > 0)
    {
        VPrintf(" (dir)", NULL);
    }

    VPrintf("...Done\n", NULL);

} /* PrintFileName */


int SafeSetProtection(struct AnchorPath *a, LONG b)
{
    int Return_Value;

    Return_Value = RETURN_OK;

    if (!SetProtection(a->ap_Buf, b))
    {
        Return_Value = RETURN_WARN;
    }

    return(Return_Value);

} /* SafeSetProtection */
