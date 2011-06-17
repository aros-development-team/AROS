#include <stdio.h>
#include <stdlib.h>

#include <exec/memory.h>
#include <libraries/iffparse.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/iffparse.h>

extern struct Library * IFFParseBase;
extern struct GfxBase * GfxBase;

static UBYTE Buffer[1024], * ptr;
static int   Fill;

#define MAX_PLANES	8

int GetByte (struct IFFHandle * iff)
{
    if (!Fill)
    {
	Fill = ReadChunkBytes (iff, Buffer, sizeof (Buffer));

	if (Fill < 0)
	    return Fill;

	ptr = Buffer;
    }

    Fill --;

    return *ptr ++;
}

int ReadRow (struct IFFHandle * iff,
    BYTE * planes[],
    ULONG Width, UWORD Depth, BOOL Compression, BOOL masking)
{
    int x,i,n;
    BYTE c;

    Width = (Width + 7) / 8;

    for (i=0; i<Depth; i++)
    {
	if (!Compression)
	{
	    for (x=0; x<Width; x++)
		planes[i][x] = GetByte (iff);
	}
	else
	{
	    for (x=0; x<Width; )
	    {
		c = GetByte (iff);

		if (c > 0)
		{
		    n = c+1;

		    while (n--)
			planes[i][x++] = GetByte (iff);
		}
		else if (c != -128)
		{
		    n = -c + 1;
		    c = GetByte (iff);

		    while (n--)
			planes[i][x++] = c;
		}
	    }
	}
    }

    return TRUE;
}

int ReadILBM (struct IFFHandle * iff,
    struct Window * window, ULONG Width, ULONG Height, UWORD Depth,
	BOOL Compression, BOOL masking)
{
    struct RastPort * rp = window->RPort;
    BYTE * planes[MAX_PLANES];
    int t,x,bit,byte,row,pen = 0,lastpen;

    printf ("ReadILBM iff=%p win=%p Size=%ldx%ld Depth=%d %s%s\n",
	iff, window, (long)Width, (long)Height, Depth,
	Compression ? "C":"",
	masking ? "M":""
    );

    planes[0] = AllocMem (Width*Depth + ((masking) ? Width : 0), MEMF_ANY);

    if (!planes[0])
	return FALSE;

    for (t=1; t<Depth; t++)
	planes[t] = planes[t-1] + Width;

    if (masking)
	planes[t] = planes[t-1] + Width;

    for ( ; t<MAX_PLANES; t++)
	planes[t] = NULL;

    for (row=0; row<Height; row++)
    {
	if (!ReadRow (iff, planes, Width, Depth, Compression, masking))
	{
	    FreeMem (planes[0], Width*Depth + ((masking) ? Width : 0));
	    return FALSE;
	}

	/* printf ("row %d, %08lx\n", row, *(ULONG*)planes[0]); */

	lastpen = -1;

	Move (rp, 0, row);

	for (x=0; x<Width; x++)
	{
	    bit = 0x80 >> (x & 7);
	    byte = x / 8;

	    for (pen=t=0; t<Depth; t++)
		if (planes[t][byte] & bit)
		    pen |= 1L << t;

	    if (lastpen == -1)
		lastpen = pen;

	    /* SetAPen (rp, pen);
	    WritePixel (rp, x, row); */

	    if (lastpen != pen)
	    {
		SetAPen (rp, lastpen);
		Draw (rp, x+1, row);
		lastpen = pen;
	    }
	}

	SetAPen (rp, pen);
	Draw (rp, Width, row);
    }

    return TRUE;
}

