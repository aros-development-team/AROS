/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Benchmark for: 
          cybergraphics.library/WritePixelArray
          cybergraphics.library/WritePixelArrayAlpha
          cybergraphics.library/ReadPixelArray
    Lang: English
*/
/*****************************************************************************

    NAME

        pixelarray

    SYNOPSIS

    LOCATION

    FUNCTION
    
    RESULT

    NOTES
        By default, WritePixelArray() is tested.
    BUGS

    INTERNALS

******************************************************************************/

#if !defined(ONLY_BENCH_CODE)
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

#define ARG_TEMPLATE    "WIDTH=W/N/K,HEIGHT=H/N/K,PIXELFMT=P/K,FUNCTION=F/K"
#define ARG_W           0
#define ARG_H           1
#define ARG_PIXFMT      2
#define ARG_FUNCTION    3
#define NUM_ARGS        4

/****************************************************************************************/
#endif /* #if !defined(ONLY_BENCH_CODE) */

#define P(x) {#x, x}

struct
{
    STRPTR name;
    LONG id;
}
pixfmt_table[] =
{    
    P(RECTFMT_RGB),
    P(RECTFMT_RGBA),
    P(RECTFMT_ARGB),
    P(RECTFMT_LUT8),
    P(RECTFMT_GREY8),
    P(RECTFMT_RAW),
    P(RECTFMT_RGB15),
    P(RECTFMT_BGR15),
    P(RECTFMT_RGB15PC),
    P(RECTFMT_BGR15PC),
    P(RECTFMT_RGB16),
    P(RECTFMT_BGR16),
    P(RECTFMT_RGB16PC),
    P(RECTFMT_BGR16PC),
    P(RECTFMT_RGB24),
    P(RECTFMT_BGR24),
    P(RECTFMT_ARGB32),
    P(RECTFMT_BGRA32),
    P(RECTFMT_RGBA32),
    P(RECTFMT_ABGR32),
    P(RECTFMT_0RGB32),
    P(RECTFMT_BGR032),
    P(RECTFMT_RGB032),
    P(RECTFMT_0BGR32),
    {0,0}
};

/****************************************************************************************/

#define FUNCTION_READ           0
#define FUNCTION_WRITE          1
#define FUNCTION_WRITE_ALPHA    2

#if !defined(ONLY_BENCH_CODE)
struct RDArgs   *myargs;
IPTR            args[NUM_ARGS];
UBYTE           s[256];
LONG            pixfmt = RECTFMT_ARGB;
LONG            pixfmt_index = 2;
LONG            width = 1280;
LONG            height = 720;
LONG            function = FUNCTION_WRITE;
STRPTR          functionname = "WritePixelArray";
struct Window   *win;

/****************************************************************************************/

static void cleanup(STRPTR msg, ULONG retcode)
{
    if (msg) 
    {
        fprintf(stderr, "pixelarray: %s\n", msg);
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

    if (args[ARG_PIXFMT])
    {
        LONG i;
    
        for(i = 0; pixfmt_table[i].name; i++)
        {
            if ((strcasecmp((STRPTR)args[ARG_PIXFMT], pixfmt_table[i].name) == 0) ||
                (strcasecmp((STRPTR)args[ARG_PIXFMT], strchr(pixfmt_table[i].name, '_') + 1) == 0))
            {
                pixfmt = pixfmt_table[i].id;
                pixfmt_index = i;
                break;
            }
        }
    
        if (pixfmt_table[i].name == NULL)
        {
            fprintf(stderr, "pixelarray: Bad pixel format! Valid ones are:\n\n");
            
            for(i = 0; pixfmt_table[i].name; i++)
            {
                printf("%s\t== %s\n", pixfmt_table[i].name, strchr(pixfmt_table[i].name, '_') + 1);
            }

            cleanup(NULL, RETURN_WARN);
        }
    }
    
    if (args[ARG_FUNCTION])
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
    }
    
}
/****************************************************************************************/
static void printresults(LONG t, LONG i)
{
    LONG bpp;
    QUAD q;

    printf("Function             : %s\n", functionname);
    printf("Pixel format         : %s   Width: %d   Height %d\n\n", 
        pixfmt_table[pixfmt_index].name, (int)width, (int)height);
    printf("Elapsed time         : %d us (%f s)\n", (int)t, (double)t / 1000000);
    printf("Blits                : %d\n", (int)i);
    printf("Blits/sec            : %f\n", i * 1000000.0 / t);
    printf("Time/blit            : %f us (%f s) (%d%% of 25Hz Frame)\n", 
        (double)t / i,
        (double)t / i / 1000000.0,
        (int)(100.0 * ((double)t / i) / (1000000.0 / 25.0)));
    
    bpp = GetCyberMapAttr(win->WScreen->RastPort.BitMap, CYBRMATTR_BPPIX);
    printf("\nScreen Bytes Per Pixel: %d\n", (int)bpp);
    printf("Area size in Pixels   : %d\n", (int)(width * height));
    printf("Area size in Bytes    : %d\n", (int)(width * height * bpp));
   
    q = ((QUAD)width) * ((QUAD)height) * ((QUAD)bpp) * ((QUAD)i) * ((QUAD)1000000) / (QUAD)t;
    printf("Bytes/sec to gfx card : %lld (%lld MB)\n", (long long)q, (long long)q / 1048576);
}

/****************************************************************************************/
#endif /* !defined(ONLY_BENCH_CODE) */

static void action_pixelarray(void)
{
    struct timeval tv_start, tv_end;
    LONG t, i;
    APTR buffer;
    
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
    
    /* Allocate and randomize the largest possible buffer */
    buffer = AllocMem(width * height * 4, MEMF_ANY);
    for (i = 0; i < width * height; i++)
        *(((ULONG *)buffer) + i) = *(((ULONG*)GfxBase) + i);
    
    SetAPen(win->RPort, 1);
    RectFill(win->RPort, 0, 0, width, height);
    
    Delay(2 * 50);
    
    CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
    
    for(i = 0; ; i++)
    {
        CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
        t = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
        if (t >= 10 * 1000000) break;
        
        switch(function)
        {
        case(FUNCTION_WRITE_ALPHA):
            WritePixelArrayAlpha((ULONG *)buffer + ( i % 1000000), 0, 0, width * 4, win->RPort, 0, 0, width, height, 0);
            break;
        case(FUNCTION_WRITE):
            WritePixelArray((ULONG *)buffer + ( i % 1000000), 0, 0, width * 4, win->RPort, 0, 0, width, height, pixfmt);
            break;
        case(FUNCTION_READ):
            ReadPixelArray(buffer, 0, 0, width * 4, win->RPort, 0, 0, width, height, pixfmt);
            break;
        }
    }
    
    printresults(t, i);
    
    FreeMem(buffer, width * height * 4);
    
    CloseWindow(win);
}

#if !defined(ONLY_BENCH_CODE)
/****************************************************************************************/

int main(void)
{
    getarguments();
    action_pixelarray();
    cleanup(NULL, 0);
    
    return 0;
}
#endif
