/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Benchmark for: 
          graphics.library/Text
    Lang: English
*/
/*****************************************************************************

    NAME

        text

    SYNOPSIS

    LOCATION

    FUNCTION
    
    RESULT

    NOTES
        By default 
    BUGS

    INTERNALS

******************************************************************************/

#include <cybergraphx/cybergraphics.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/****************************************************************************************/

#define ARG_TEMPLATE    "WIDTH=W/N/K,HEIGHT=H/N/K,MODE=P/K"
#define ARG_W           0
#define ARG_H           1
#define ARG_MODE        2
#define NUM_ARGS        3

/****************************************************************************************/

struct RDArgs   *myargs;
IPTR            args[NUM_ARGS];
UBYTE           s[256];
LONG            width = 1280;
LONG            height = 720;
STRPTR          functionname = "Text";
struct Window   *win;

/****************************************************************************************/

static void cleanup(STRPTR msg, ULONG retcode)
{
    if (msg) 
    {
        fprintf(stderr, "text: %s\n", msg);
    }
    
    if (myargs) FreeArgs(myargs);

    exit(retcode);
}

/****************************************************************************************/

static void getarguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
        Fault(IoErr(), 0, s, 255);
        cleanup(s, RETURN_FAIL);
    }
    
    if (args[ARG_W]) width = *(LONG *)args[ARG_W];

    if (args[ARG_H]) height = *(LONG *)args[ARG_H];

 /*   if (args[ARG_FUNCTION])
    {
        if (strcasecmp((STRPTR)args[ARG_FUNCTION], "READ") == 0)
        {
            function = FUNCTION_READ;
            functionname = "ReadPixelArray";
        }

        if (strcasecmp((STRPTR)args[ARG_FUNCTION], "WRITE_ALPHA") == 0)
        {
            function = FUNCTION_WRITE_ALPHA;
            functionname = "WritePixelArrayAlpha";
        }
    }*/
    
}

/****************************************************************************************/

static void action(void)
{
    struct timeval tv_start, tv_end;
    LONG t, i, bpp;
    QUAD q;
    STRPTR buffer = NULL;
    ULONG bufferlen;
    
    struct TextExtent extend;
    
    win = OpenWindowTags(NULL, WA_Borderless, TRUE,
                               WA_InnerWidth, width,
                               WA_InnerHeight, height,
                               WA_Activate, TRUE,
                               WA_IDCMP, IDCMP_VANILLAKEY,
                               TAG_DONE);

    if (!win)
    {
        cleanup("Can't open window!", RETURN_FAIL);    
    }
    
    width = win->Width;
    height = win->Height;
    
    SetAPen(win->RPort, 2);
    RectFill(win->RPort, 0, 0, width, height);
    
    Delay(2 * 50);
    
    CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
    
    /* Generate random buffer */
    buffer = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
    for (i = 0; i < 99; i++)
        buffer[i] = i + 60;
    
    bufferlen = strlen(buffer);
    TextExtent(win->RPort, buffer, bufferlen, &extend);
    
    for(i = 0; ; i++)
    {
        CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
        t = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
        if (t >= 10 * 1000000) break;

        for (ULONG y = i % extend.te_Height; y < height; y += extend.te_Height)
            for (ULONG x = 0; x < width; x += extend.te_Width)
            {
                Move(win->RPort, x, y);
                Text(win->RPort, buffer, bufferlen);
            }
    }
    
    printf("Function             : %s\n", functionname);
    printf("Elapsed time         : %d us (%f s)\n", t, (double)t / 1000000);
    printf("Blits                : %d\n", i);
    printf("Blits/sec            : %f\n", i * 1000000.0 / t);
    printf("Time/blit            : %f us (%f s) (%d%% of 25Hz Frame)\n", 
        (double)t / i,
        (double)t / i / 1000000.0,
        (LONG)(100.0 * ((double)t / i) / (1000000.0 / 25.0)));
    
    bpp = GetCyberMapAttr(win->WScreen->RastPort.BitMap, CYBRMATTR_BPPIX);
    printf("\nScreen Bytes Per Pixel: %d\n", bpp);
    printf("Area size in Pixels   : %d\n", width * height);
    printf("Area size in Bytes    : %d\n", width * height * bpp);
   
    q = ((QUAD)width) * ((QUAD)height) * ((QUAD)bpp) * ((QUAD)i) * ((QUAD)1000000) / (QUAD)t;
    printf("Bytes/sec to gfx card : %lld (%lld MB)\n", q, q / 1048576);
    
    CloseWindow(win);
    
    FreeVec(buffer);
}

/****************************************************************************************/

int main(void)
{
    getarguments();
    action();
    cleanup(NULL, 0);
    
    return 0;
}
