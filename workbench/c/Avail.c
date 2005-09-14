/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Avail CLI command
    Lang: English
*/

/******************************************************************************


    NAME

        Avail [CHIP | FAST | TOTAL | FLUSH] [H | HUMAN]

    SYNOPSIS

        CHIP/S, FAST/S, TOTAL/S, FLUSH/S, H=HUMAN/S        

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
	HUMAN  --  display more human-readable values (gigabytes as "G",
		   megabytes as "M", kilobytes as "K")

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
#include <string.h>

static const char version[] = "$VER: Avail 42.0 (13.9.2005)\n";


#define  ARG_TEMPLATE  "CHIP/S,FAST/S,TOTAL/S,FLUSH/S,H=HUMAN/S"

enum
{
    ARG_CHIP = 0,
    ARG_FAST,
    ARG_TOTAL,
    ARG_FLUSH,
    ARG_HUMAN,
    NOOFARGS
};

LONG printm(CONST_STRPTR head, ULONG *array, LONG num);

int __nocommandline = 1;

BOOL aHuman;

int main(void)
{
    IPTR           args[NOOFARGS] = { (IPTR)FALSE,
				      (IPTR)FALSE,
				      (IPTR)FALSE, 
				      (IPTR)FALSE,
				      (IPTR)FALSE };
    struct RDArgs *rda;
    LONG           error = 0;
    BOOL bPrintErr = TRUE;
    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    
    if (rda != NULL)
    {
	BOOL  aChip  = (BOOL)args[ARG_CHIP];
	BOOL  aFast  = (BOOL)args[ARG_FAST];
	BOOL  aTotal = (BOOL)args[ARG_TOTAL];
	BOOL  aFlush = (BOOL)args[ARG_FLUSH];
	
	aHuman = (BOOL)args[ARG_HUMAN];
	
	ULONG chip[4], fast[4], total[4];
	
	/* BOOL to int hack: */
	if (aChip + aFast + aTotal > 1)
	{
	    FPuts(Output(),"Only one of CHIP, FAST or TOTAL allowed\n");
	    bPrintErr = FALSE;
            FreeArgs(rda);

	    return RETURN_FAIL;
	}
	else
	{
	    if (aFlush)
	    {
                    APTR Mem;

                    Mem = AllocMem(0x7ffffff0, MEMF_PUBLIC);
                    if (Mem)
                        FreeMem(Mem, 0x7ffffff0);
	    }
	    
	    if(aChip)
	    {
		
		chip[0] = AvailMem(MEMF_CHIP);
		
		if (printm(NULL, chip, 1) < 0)
		{
		    error = RETURN_ERROR;
		}
	    }
	    else if(aFast)
	    {
		fast[0] = AvailMem(MEMF_FAST);

		if (printm(NULL, fast, 1) < 0)
		{
		    error = RETURN_ERROR;
		}
	    }
	    else if (aTotal)
	    {
		total[0] = AvailMem(MEMF_ANY);

		if (printm(NULL, total, 1) < 0)
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
		    printm("chip", chip, 4) < 0 ||
		    printm("fast", fast, 4) < 0 ||
		    printm("total", total, 4) < 0)
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
    
    if(error != RETURN_OK && bPrintErr)
    {
	PrintFault(IoErr(), "Avail");
    }

    return error;
}

static
void fmtlarge(UBYTE *buf, ULONG num)
{
    UQUAD d;
    UBYTE ch;
    struct
    {
        ULONG val;
        LONG  dec;
    } array =
    {
        num,
        0
    };

    if (num >= 1073741824)
    {
        array.val = num >> 30;
        d = ((UQUAD)num * 10 + 536870912) / 1073741824;
        array.dec = d % 10;
        ch = 'G';
    }
    else if (num >= 1048576)
    {
        array.val = num >> 20;
        d = ((UQUAD)num * 10 + 524288) / 1048576;
        array.dec = d % 10;
        ch = 'M';
    }
    else if (num >= 1024)
    {
        array.val = num >> 10;
        d = (num * 10 + 512) / 1024;
        array.dec = d % 10;
        ch = 'K';
    }
    else
    {
        array.val = num;
        array.dec = 0;
        d = 0;
        ch = 'B';
    }

    if (!array.dec && (d > array.val * 10))
    {
        array.val++;
    }

    RawDoFmt(array.dec ? "%lu.%lu" : "%lu", &array, NULL, buf);
    while (*buf) { buf++; }
    *buf++ = ch;
    *buf   = '\0';
}

LONG printm(CONST_STRPTR head, ULONG *array, LONG num)
{
    LONG res = -1;
    CONST_STRPTR fmt;
    UBYTE buf[10];

    if (head)
    {
        LONG len = 16 - strlen(head);
        RawDoFmt(aHuman ? "%%%lds" : "%%%ldlu", &len, NULL, buf);
        fmt = buf;
        PutStr(head);
    }
    else
    {
        fmt = aHuman ? "%9s" : "%9lu";
    }

    if (aHuman)
    {
        if (num == 1)
        {
            fmtlarge(buf, *array);
            res = PutStr(buf);
        }
        else
        {
            while (num--)
            {
                UBYTE tmp[10];

                fmtlarge(tmp, *array);
                res = Printf(fmt, (ULONG) tmp);
                if (res < 0)
                    break;

                fmt = " %9s";
                array++;
            }
        }
    }
    else
    {
        if (num == 1)
        {
            res = VPrintf("%lu", array);
        }
        else
        {
            while (num--)
            {
                res = VPrintf(fmt, array);
                if (res < 0)
                    break;

                fmt = " %9lu";
                array++;
            }
        }
    }
    PutStr("\n");

    return res;
}
