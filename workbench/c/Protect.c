/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Protect CLI command
    Lang: English
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

        SetProtection()

    INTERNALS

    HISTORY

        27-Jul-1997  laguest  --  Initial inclusion into the AROS tree
	3.12.2000    SDuvan   --  Rewrote, simplified and implemented missing
	                          functionality

******************************************************************************/


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

#include <ctype.h>

#define CTRL_C         (SetSignal(0L,0L) & SIGBREAKF_CTRL_C)

#define Bit_Mask(bit)           (1L << bit)
#define Bit_Clear(name, bit)    name &= ~Bit_Mask(bit)
#define Bit_Set(name, bit)      name |= Bit_Mask(bit)

#define ARG_TEMPLATE    "FILE/A,FLAGS,ADD/S,SUB/S,ALL/S,QUIET/S"

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
    NOOFARGS
};


/* To define whether a command line switch was set or not.
 */
#define NOT_SET(x)      (x == 0)
#define IS_SET(x)       (!NOT_SET(x))

#define MAX_PATH_LEN    512

static const char version[] = "$VER: Protect 41.1 (2.12.2000)\n";

struct UtilityBase *UtilityBase;

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

    IPTR args[NOOFARGS] = { NULL,
			    NULL,
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

	    LONG    flagValues = FIBF_READ | FIBF_WRITE | FIBF_DELETE |
	                         FIBF_EXECUTE;

	    if (flags != NULL)
	    {
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
		    switch (toupper(*flags))
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
		if (!all &&IsDosEntryA(file, LDF_VOLUMES | LDF_DEVICES))
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


/* Defines whether MatchFirst(), etc has matched a file. */
#define MATCHED_FILE    0

int Do_Recursion(struct AnchorPath *, LONG, LONG, LONG *, LONG);
void AddBitMask(struct AnchorPath *, STRPTR, LONG *, BOOL);
void SubBitMask(struct AnchorPath *, STRPTR, LONG *, BOOL);
void NewBitMask(struct AnchorPath *, STRPTR, LONG *);
void PrintFileName(struct AnchorPath *, LONG);
int SafeSetProtection(struct AnchorPath *, LONG);


#define  isDir(fib) ((fib)->fib_DirEntryType >= 0)

int doProtect(struct AnchorPath *ap, STRPTR file, LONG flags, BOOL flagsSet,
	      BOOL add, BOOL sub, BOOL all, BOOL quiet)
{
    LONG  match;
    int   retval = RETURN_OK;
    LONG  indent = 0;
    int   i;			/* Loop variable */
    BOOL  error;

    for (match = MatchFirst(file, ap);
    	 match == 0 && retval == RETURN_OK && !CTRL_C;
	 match = MatchNext(ap))
    {
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

	error = setProtection(ap->ap_Buf, ap->ap_Info.fib_Protection, flags,
			      flagsSet, add, sub);

	/* Fix indentation level */
	for (i = 0; i < indent; i++)
	{
	    Printf("  ");
	}

	if (!quiet || error)
	{
	    Printf(FilePart(ap->ap_Buf));

	    if (isDir(&ap->ap_Info))
	    {
		Printf(" (dir)");
	    }

	    Printf("...Done\n");
	}
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
    
    if (flags != ALL_OFF)
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
	else
	{
	    /* Clear all permissions then set the ones given. */
	    newFlags = flags;
	}
    }
    else
    {
	/* No flags were given */
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

    if (!SetProtection(file, newFlags))
    {
	return FALSE;
    }
    
    return TRUE;
}



int Do_Protect(struct AnchorPath *a, STRPTR file, STRPTR flags,
               BOOL add, BOOL sub, BOOL all, BOOL quiet)
{
    LONG Result,
	TabValue,
	BitMask;
    int  retval = RETURN_OK;
    IPTR DevArg[2];

    TabValue     = 0L;
    BitMask      = 0L;

    if(IsDosEntryA(file, LDF_VOLUMES | LDF_DEVICES) && !all)
    {
        DevArg[0] = (IPTR)file;
	
        VPrintf("Can't set protection for %s - ", DevArg);
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
        PrintFault(IoErr(), NULL);
        
        retval = RETURN_FAIL;
    }
    else
    {
        Result = MatchFirst(file, a);
	
        if (Result == MATCHED_FILE)
        {
            do
            {
                if (flags != NULL)  /* Flags are given */
                {
                    if (add)
                    {
                        /* Add the permission bits to the bitmask.
                         * In other words - Enable permissions.
                         */
    
                        AddBitMask(a, flags, &BitMask, TRUE);
                        retval = Do_Recursion(a, all, quiet, &TabValue, BitMask);
                    }
                    else if (sub)
                    {
                        /* Remove the permission bits from the bitmask.
                         * In other words - Disable permissions. */

                        SubBitMask(a, flags, &BitMask, TRUE);
                        retval = Do_Recursion(a, all, quiet, &TabValue, BitMask);
                    }
                    else
                    {
                        /* Clear all permissions then set the ones given.
                         */
			
                        NewBitMask(a, flags, &BitMask);
                        retval = Do_Recursion(a, all, quiet, &TabValue, BitMask);
                    }
                }
                else    /* No flags are given */
                {
                    if (!add && !sub)
                    {
                        /* Remove all permissions bits from the bitmask.
                         * In other words - Disable ALL permissions. */

                        SubBitMask(a, "sparwed", &BitMask, TRUE);
                        retval = Do_Recursion(a, all, quiet, &TabValue, BitMask);
                    }
                }
            }

            while (((Result = MatchNext(a)) == MATCHED_FILE) &&
                retval != RETURN_FAIL);
        }
        else
        {
            PrintFault(IoErr(), "Protect");
            retval = RETURN_FAIL;
        }

        MatchEnd(a);
    }

    return retval;    
}


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
	    case 'h':	/* Hold */
	    	Bit_Set(*b, FIBB_HOLD);
		break;
		
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
	    case 'h':	/* Hold */
	    	Bit_Clear(*b, FIBB_HOLD);
		break;
		
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

    SubBitMask(a, "hsparwed", b, TRUE);
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
