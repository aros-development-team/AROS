/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Cumulative graphics benchmarks
    Lang: English
*/
/*****************************************************************************

    NAME

        gfxbench

    SYNOPSIS

    LOCATION

    FUNCTION
    
    RESULT

    NOTES

    BUGS

    INTERNALS

******************************************************************************/

#include <proto/exec.h>
#include <devices/timer.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <resources/processor.h>
#include <proto/processor.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct Window * win;
LONG            width = 1280;
LONG            height = 720;

/* COMMON */
static void printresults(LONG timems, LONG blits)
{
    printf("%.2f|\n", blits * 1000000.0 / timems);
}

static void cleanup(STRPTR msg, ULONG retcode)
{
    if (msg) 
        fprintf(stderr, "text: %s\n", msg);

    exit(retcode);
}

/* TEXT BENCH */
#define ONLY_BENCH_CODE

STRPTR          consttext = "The AROS Development Team. All rights reserved.";
LONG            mode = JAM1;
BOOL            antialias = FALSE;
LONG            linelen = 100;

#include "text.c"

static void textbenchmark(LONG optmode, BOOL optantialias, LONG optlen)
{
    STRPTR modestr = "UNKNOWN";
    STRPTR aastr = "UNKNOWN";
    TEXT lenstr[10] = {0};
    
    switch(optmode)
    {
    case(JAM1): modestr = "JAM1"; break;
    case(JAM2): modestr = "JAM2"; break;
    case(COMPLEMENT): modestr = "COMPLEMENT"; break;
    }
    mode = optmode;
    
    if (optantialias)
        aastr = "ANTIALIASED";
    else
        aastr = "NON-ANTIALIASED";
    antialias = optantialias;
    
    sprintf(lenstr, "LEN %d", optlen);
    linelen = optlen;
    
    printf("|%s, %s, %s|", modestr, aastr, lenstr);
    
    action();
}

static void detectsystem()
{
    printf("*System information*\n");

    APTR ProcessorBase = OpenResource(PROCESSORNAME);
    
    if (ProcessorBase)
    {
        ULONG processorcount;
        ULONG i;
        struct TagItem tags [] = 
        {
            { GCIT_NumberOfProcessors, (IPTR)&processorcount },
            { 0, (IPTR)NULL }
        };
        
        GetCPUInfo(tags);
        
        printf("|Processor count|%d|\n", processorcount);

        for (i = 0; i < processorcount; i++)
        {
            UQUAD frequency = 0;
            STRPTR modelstr = NULL;

            struct TagItem tags [] = 
            {
                { GCIT_SelectedProcessor, (IPTR)i },
                { GCIT_ProcessorSpeed, (IPTR)&frequency },
                { GCIT_ModelString, (IPTR)&modelstr },
                { TAG_DONE, TAG_DONE }
            };

            GetCPUInfo(tags);

            frequency /= 1000000;
            
            printf("|Processor #%d|%s - %d Mhz|\n", i, modelstr, (LONG)frequency);
        }
    }
    
    printf("\n\n");
}

static void textbenchmarkset()
{
    printf("*Text benchmark*\n");
    printf("||MODE||RESULT||\n");
    textbenchmark(JAM1,         FALSE,  100);
    textbenchmark(JAM2,         FALSE,  100);
    textbenchmark(COMPLEMENT,   FALSE,  100);
    textbenchmark(JAM1,         TRUE,   100);
    textbenchmark(JAM2,         TRUE,   100);
    textbenchmark(COMPLEMENT,   TRUE,   100);
    textbenchmark(JAM1,         FALSE,  5);
    textbenchmark(JAM2,         FALSE,  5);
    textbenchmark(COMPLEMENT,   FALSE,  5);
    textbenchmark(JAM1,         TRUE,   5);
    textbenchmark(JAM2,         TRUE,   5);
    textbenchmark(COMPLEMENT,   TRUE,   5);
}

int main(void)
{
    detectsystem();

    textbenchmarkset();

    cleanup(NULL, 0);
    
    return 0;
}
