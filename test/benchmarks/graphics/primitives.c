/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$

    Originally written by by Rune Elvemo.
    Improved and adapted to gcc by Fabio Alemagna.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <exec/types.h>
#include <exec/io.h>
#include <devices/timer.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/alib.h>

struct Device      *TimerBase;
struct MsgPort     *TimerMP;
struct timerequest *TimerIO;

struct Window *benchwin;
struct Screen *wb_screen;

typedef struct
{
    ULONG x,y;
} xypoint;

#define NUMITERATIONS (16*1024)
xypoint xytable[NUMITERATIONS];

int init(void);
void cleanup(void);

#define DOTEST(function)                                                     \
{                                                                            \
    int i = 0, pen;                                                          \
    struct timeval start, end;                                               \
    float total;                                                             \
                                                                             \
    GetSysTime(&start);                                                      \
    for (pen = 0; pen < NUMITERATIONS; pen++)                                \
    {                                                                        \
        const ULONG x = xytable[i].x;                                        \
        const ULONG y = xytable[i].y;                                        \
	struct RastPort *rport = benchwin->RPort;                            \
	                                                                     \
        SetAPen(benchwin->RPort, pen);                                       \
                                                                             \
	function;                                                            \
                                                                             \
	i++;                                                                 \
    }                                                                        \
    GetSysTime(&end);                                                        \
    SubTime(&end,&start);                                                    \
                                                                             \
    printf("\nResults for " #function ":\n");                                \
                                                                             \
    total = end.tv_secs + ((float)end.tv_micro / 1000000);                   \
                                                                             \
    printf("total time: %f\n", total);                                       \
    printf("iterations/sec: %f\n", pen/total);                               \
                                                                             \
    SetAPen(benchwin->RPort, 0);                                             \
    RectFill(benchwin->RPort, 0,0, benchwin->GZZWidth, benchwin->GZZHeight); \
}

int main(int argc, char **argv)
{
    if (init())
    {
	DOTEST(DrawCircle(rport, x, y, 10));
	DOTEST(DrawEllipse(rport, x, y, 10, 20));
	DOTEST({Move(rport, x, y); Text(rport, "BenchMark", 9);});
	DOTEST(WritePixel(rport, x, y));
	DOTEST(RectFill(rport, x, y, x+50, y+50));
	DOTEST
	(({
	    WORD pnts[] = {x, y, x+10, y+10, x+50, y+50}; PolyDraw(rport, 3,pnts);
	}));
    }
    cleanup();

    return 0;
}

int init() {
  wb_screen = LockPubScreen(NULL);
  if (!wb_screen) return 0;

  benchwin = OpenWindowTags
  (
      NULL,
      WA_Left,   10, WA_Top,     20,
      WA_Width, 640, WA_Height, 480,
      WA_DragBar,       TRUE,
      WA_SmartRefresh,  TRUE,
      WA_NoCareRefresh, TRUE,
      WA_GimmeZeroZero, TRUE,
      WA_Title,         (IPTR)"Gfxlib benchmark",
      WA_PubScreen,     (IPTR)wb_screen,
      TAG_END
  );

  UnlockPubScreen(NULL, wb_screen);

  if (!benchwin) return 0;

  TimerMP = CreatePort(NULL, 0);
  if (!TimerMP) return 0;

  TimerIO = (struct timerequest*)CreateExtIO(TimerMP, sizeof(struct timerequest));
  if (!TimerIO) return 0;

  if (OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest*)TimerIO, 0))
      return 0;

  TimerBase = TimerIO->tr_node.io_Device;

  {
      struct timeval start;
      int i;

      GetSysTime(&start);
      srand48(start.tv_secs);

      for (i = 0; i < sizeof(xytable)/sizeof(xytable[0]); i++)
      {
          xytable[i].x = drand48() * benchwin->GZZWidth;
          xytable[i].y = drand48() * benchwin->GZZHeight;
      }
  }

  return 1;
}

void cleanup(void)
{
    if (TimerBase) CloseDevice((struct IORequest*)TimerIO);
    if (TimerIO)   DeleteExtIO((struct IORequest*)TimerIO);
    if (TimerMP)   DeletePort(TimerMP);
    if (benchwin)  CloseWindow(benchwin);
}


