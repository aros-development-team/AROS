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

        dos.library/SetProtection()

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

            if (!error)
            {
                PrintFault(ioerr, "..error");
            }
            else
            {
                PutStr("..done\n");
            }
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
