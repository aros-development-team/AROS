#include <cybergraphx/cybergraphics.h>

#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#define NUM_PIXFMT 14

static char *pixfmt_str[14]=
{
    "LUT8",
    "RGB15",
    "BGR15",
    "RGB15PC",
    "BGR15PC",
    "RGB16",
    "BGR16",
    "RGB16PC",
    "BGR16PC",
    "RGB24",
    "BGR24",
    "ARGB32",
    "BGRA32",
    "RGBA32"
};

int main(void)
{
    struct Screen *scr = IntuitionBase->ActiveScreen;
    
    if (scr)
    {
    	struct BitMap *bm = scr->RastPort.BitMap;
	LONG pixfmt;
	
	pixfmt = GetCyberMapAttr(bm, CYBRMATTR_PIXFMT);
	
	printf("Pixel Format: #%ld (%s)\n",
	       (long)pixfmt,
	       ((pixfmt >= 0) && (pixfmt < NUM_PIXFMT)) ? pixfmt_str[pixfmt] : "<unknown>");
	
    }
    
    return 0;
    
}
