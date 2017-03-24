/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <graphics/clip.h>
#include <graphics/gfx.h>
#include <exec/exec.h>
#include <proto/layers.h>
#include <proto/exec.h>

#include <stdio.h>

struct Library *LayersBase;
struct Layer dummy_lay;

struct ClipRect dummy_cr[100];

void setcr(WORD which, WORD x1, WORD y1, WORD x2, WORD y2)
{
    dummy_cr[which].bounds.MinX = x1;
    dummy_cr[which].bounds.MinY = y1;
    dummy_cr[which].bounds.MaxX = x2;
    dummy_cr[which].bounds.MaxY = y2;
    dummy_cr[which].Flags = which;
    
    if (which > 0) dummy_cr[which - 1].Next = &dummy_cr[which];
}

void makecrs(void)
{
/*    0000000000011111111122
      0123456789012345678901

00    +---+---+------------+
01    | 0 |   |            |
02    +---+   |     2      |
03    |   | 1 +---+--------+
04    | 3 |   | 4 |        |
05    +---+   +---+        |
06    | 6 |   |   |     5  |
07    +---+---+ 8 +--------+
08    |  7    |   |     9  |
09    +-------+---+--------+

*/
    setcr(0, 0, 0, 39, 19);
    setcr(1, 40, 0, 79, 69);
    setcr(2, 80, 0, 209, 29);
    setcr(3, 0, 20, 39, 49);
    setcr(4, 80, 30, 119, 49);
    setcr(5, 120, 30, 209, 69);
    setcr(6, 0, 50, 39, 69);
    setcr(7, 0, 70, 79, 89);
    setcr(8, 80, 50, 119, 89);
    setcr(9, 120, 70, 209, 89);
    
    dummy_lay.ClipRect = dummy_cr;
}

void doit(char *msg, WORD dx, WORD dy)
{
    struct ClipRect *cr;
    
    SortLayerCR(&dummy_lay, dx, dy);
    
    printf("\n%s\n----------------------------\n", msg);
    cr = dummy_lay.ClipRect;
    while(cr)
    {
        printf("%ld ", (long)cr->Flags);
        cr = cr->Next;
    }
    printf("\n");
    
}

void action(void)
{
    doit("UP", 0, -1);
    doit("DOWN", 0, 1);
    doit("LEFT", -1, 0);
    doit("RIGHT", 1, 0);
    doit("UP LEFT", -1, -1);
    doit("UP RIGHT", 1, -1);
    doit("DOWN LEFT", -1, 1);
    doit("DOWN RIGHT", 1, 1);
    
}

int main(void)
{
    LayersBase = OpenLibrary("layers.library", 0);
    if (LayersBase)
    {
        makecrs();
	action();
	
        CloseLibrary(LayersBase);
    }
    
    return 0;
}
