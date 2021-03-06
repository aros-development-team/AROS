/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

    Desc: Avail CLI command
*/

/******************************************************************************


    NAME

        Avail [CHIP | FAST | TOTAL | FLUSH] [H | HUMAN]

    SYNOPSIS

        CHIP/S, FAST/S, TOTAL/S, FLUSH/S, H=HUMAN/S

    LOCATION

        C:

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

#include <aros/cpu.h> // for __WORDSIZE
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <string.h>

const TEXT version[] = "$VER: Avail 42.2 (24.2.2016)\n";

#if (__WORDSIZE == 64)
#define AVAIL_ARCHSTR   "%13s"
#define AVAIL_ARCHVAL   "%13iu"
#else
#define AVAIL_ARCHSTR   "%9s"
#define AVAIL_ARCHVAL   "%9iu"
#endif

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

LONG printm(CONST_STRPTR head, IPTR *array, LONG num);

int __nocommandline = 1;

BOOL aHuman;

/* Allocate all memory (even for >2G systems), then free it.
 * This will force all expungable items out of memory
 */
static void FlushMem(struct ExecBase *SysBase)
{
    APTR Mem;

    Mem = AllocMem(0x7ffffff0, MEMF_PUBLIC);
    if (Mem) {
        FlushMem(SysBase);
        FreeMem(Mem, 0x7ffffff0);
    }
}

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
        UWORD typeCount = 0;
        BOOL  aChip  = (BOOL)args[ARG_CHIP];
        BOOL  aFast  = (BOOL)args[ARG_FAST];
        BOOL  aTotal = (BOOL)args[ARG_TOTAL];
        BOOL  aFlush = (BOOL)args[ARG_FLUSH];
        aHuman = (BOOL)args[ARG_HUMAN];

        if (aChip)
        {
            typeCount++;
        }
        if (aFast)
        {
            typeCount++;
        }
        if (aTotal)
        {
            typeCount++;
        }

        IPTR chip[4], fast[4], total[4];

        if (typeCount > 1)
        {
            FPuts(Output(), "Only one of CHIP, FAST or TOTAL allowed\n");
            bPrintErr = FALSE;
            FreeArgs(rda);

            return RETURN_FAIL;
        }
        else
        {
            if (aFlush)
            {
                FlushMem(SysBase);
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

#if (__WORDSIZE == 64)
                if (PutStr("Type     Available        In-Use       Maximum       Largest\n") < 0 ||
#else
                if (PutStr("Type    Available    In-Use   Maximum   Largest\n") < 0 ||
#endif
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
void fmtlarge(UBYTE *buf, IPTR num)
{
    UQUAD d;
    UBYTE ch;
    struct
    {
        IPTR val;
        IPTR  dec;
    } array =
    {
        num,
        0
    };

    if (num >= 0x40000000)
    {
        array.val = num >> 30;
        d = ((UQUAD)num * 100 + 0x20000000) / 0x40000000;
        array.dec = d % 100;
        ch = 'G';
    }
    else if (num >= 0x100000)
    {
        array.val = num >> 20;
        d = ((UQUAD)num * 100 + 0x80000) / 0x100000;
        array.dec = d % 100;
        ch = 'M';
    }
    else if (num >= 0x400)
    {
        array.val = num >> 10;
        d = (num * 100 + 0x200) / 0x400;
        array.dec = d % 100;
        ch = 'K';
    }
    else
    {
        array.val = num;
        array.dec = 0;
        d = 0;
        ch = 'B';
    }

    if (!array.dec && (d > array.val * 100))
    {
        array.val++;
    }

    RawDoFmt(array.dec ? "%iu.%02iu" : "%iu", (RAWARG)&array, NULL, buf);
    while (*buf) { buf++; }
    *buf++ = ch;
    *buf   = '\0';
}

LONG printm(CONST_STRPTR head, IPTR *array, LONG num)
{
    LONG res = -1;
    CONST_STRPTR fmt;
    UBYTE buf[10];

    if (head)
    {
        ULONG len = 18 - strlen(head);
        RawDoFmt(aHuman ? "%%%lds" : "%%%ldiu", (RAWARG)&len, NULL, buf);
        fmt = buf;
        PutStr(head);
    }
    else
    {
        fmt = aHuman ? AVAIL_ARCHSTR : AVAIL_ARCHVAL;
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
                res = Printf(fmt, tmp);
                if (res < 0)
                    break;

                fmt = " " AVAIL_ARCHSTR;
                array++;
            }
        }
    }
    else
    {
        if (num == 1)
        {
            res = VPrintf("%iu", (RAWARG)array);
        }
        else
        {
            while (num--)
            {
                res = VPrintf(fmt, (RAWARG)array);
                if (res < 0)
                    break;

                fmt = " " AVAIL_ARCHVAL;
                array++;
            }
        }
    }
    PutStr("\n");

    return res;
}
