/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Avail CLI command
    Lang: English
*/

/******************************************************************************


    NAME

        Avail [CHIP | FAST | TOTAL | FLUSH]

    SYNOPSIS

        CHIP/S, FAST/S, TOTAL/S, FLUSH/S        

    LOCATION

        Workbench:C

    FUNCTION

        Give a summary of the memory usage and availability in the system.
	To free up unused memory that still may be allocated (libraries,
	devices, fonts and such present in memory but whcih are currently
	not in use), use the FLUSH option.

    INPUTS

        CHIP   --  show only "chip" memory
	FAST   --  show only "fast" memory
	TOTAL  --  show information on memory regardless of type
	FLUSH  --  remove unnecessary things residing in memory

    RESULT

    NOTES

        "Chip" and "fast" memory are associated with the Amiga computer
	and may not be applicable on your hardware platform.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: Avail 41.1 (14.3.1997)\n";


#define  ARG_TEMPLATE  "CHIP/S,FAST/S,TOTAL/S,FLUSH/S"

enum
{
    ARG_CHIP = 0,
    ARG_FAST,
    ARG_TOTAL,
    ARG_FLUSH,
    NOOFARGS
};


int __nocommandline = 1;

int main(void)
{
    IPTR           args[NOOFARGS] = { (IPTR)FALSE,
				      (IPTR)FALSE,
				      (IPTR)FALSE, 
				      (IPTR)FALSE };
    struct RDArgs *rda;
    LONG           error = 0;
    
    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    
    if (rda != NULL)
    {
	BOOL  aChip  = (BOOL)args[ARG_CHIP];
	BOOL  aFast  = (BOOL)args[ARG_FAST];
	BOOL  aTotal = (BOOL)args[ARG_TOTAL];
	BOOL  aFlush = (BOOL)args[ARG_FLUSH];
	
	ULONG chip[4], fast[4], total[4];
	
	/* BOOL to int hack: */
	if (aChip + aFast + aTotal > 1)
	{
	    FPuts(Output(),"Only one of CHIP, FAST or TOTAL allowed\n");
	    FreeArgs(rda);

	    return RETURN_FAIL;
	}
	else
	{
	    if (aFlush)
	    {
		FreeVec(AllocVec(~0ul/2, MEMF_ANY));
	    }
	    
	    if(aChip)
	    {
		
		chip[0] = AvailMem(MEMF_CHIP);
		
		if (VPrintf("%ld\n",chip) < 0)
		{
		    error = RETURN_ERROR;
		}
	    }
	    else if(aFast)
	    {
		fast[0] = AvailMem(MEMF_FAST);

		if (VPrintf("%ld\n", fast) < 0)
		{
		    error = RETURN_ERROR;
		}
	    }
	    else if (aTotal)
	    {
		total[0] = AvailMem(MEMF_ANY);

		if (VPrintf("%ld\n", total) < 0)
		{
		    error = RETURN_ERROR;
		}
	    }
	    else
	    {
		Forbid();

		chip[0] = AvailMem(MEMF_CHIP);
		chip[2] = AvailMem(MEMF_CHIP | MEMF_TOTAL);
		chip[3] = AvailMem(MEMF_CHIP | MEMF_LARGEST);
		chip[1] = chip[2] - chip[0];
		fast[0] = AvailMem(MEMF_FAST);
		fast[2] = AvailMem(MEMF_FAST | MEMF_TOTAL);
		fast[3] = AvailMem(MEMF_FAST | MEMF_LARGEST);
		fast[1] = fast[2] - fast[0];
		total[0] = AvailMem(MEMF_ANY);
		total[2] = AvailMem(MEMF_ANY | MEMF_TOTAL);
		total[3] = AvailMem(MEMF_ANY | MEMF_LARGEST);
		total[1] = total[2] - total[0];

		Permit();

		if (PutStr("Type  Available    In-Use   Maximum   Largest\n") < 0 ||
		    VPrintf("chip %10.ld%10.ld%10.ld%10.ld\n", chip) < 0 ||
		    VPrintf("fast %10.ld%10.ld%10.ld%10.ld\n", fast) < 0 ||
		    VPrintf("total%10.ld%10.ld%10.ld%10.ld\n", total) < 0)
		{
		    error = RETURN_ERROR;
		}
	    }
	}
	
	FreeArgs(rda);
    }
    else
    {
	error = RETURN_FAIL;
    }
    
    if(error != RETURN_OK)
    {
	PrintFault(IoErr(), "Avail");
    }

    return error;
}

