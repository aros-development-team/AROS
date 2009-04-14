#ifndef FILEFORMAT_H
#define FILEFORMAT_H

/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/iffparse.h>

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

struct MUI_RGBcolorArray
{
    UBYTE  red[4];
    UBYTE  green[4];
    UBYTE  blue[4];
};

struct MUI_PenSpecArray
{
    UBYTE ps_buf[32];
};

struct MUI_PubScreenDescArray
{
    UBYTE  Version[4];

    char  Name[PSD_MAXLEN_NAME];
    char  Title[PSD_MAXLEN_TITLE];
    char  Font[PSD_MAXLEN_FONT];
    char  Background[PSD_MAXLEN_BACKGROUND];

    UBYTE DisplayID[4];

    UBYTE DisplayWidth[2];
    UBYTE DisplayHeight[2];

    UBYTE DisplayDepth;
    UBYTE OverscanType;
    UBYTE AutoScroll;
    UBYTE NoDrag;
    UBYTE Exclusive;
    UBYTE Interleaved;
    UBYTE SysDefault;
    UBYTE Behind;
    UBYTE AutoClose;
    UBYTE CloseGadget;
    UBYTE DummyWasForeign;

    BYTE SystemPens[PSD_MAXSYSPENS];
    UBYTE Reserved[1+7*4-PSD_MAXSYSPENS];

    struct MUI_RGBcolorArray Palette[PSD_NUMCOLS];
    struct MUI_RGBcolorArray rsvd[PSD_MAXSYSPENS-PSD_NUMCOLS];

    struct MUI_PenSpecArray rsvd2[PSD_MAXMUIPENS];

    UBYTE Changed[4];
    UBYTE UserData[4];
};

#define ARRAY_TO_LONG(x) ( ((x)[0] << 24UL) + \
                           ((x)[1] << 16UL) + \
                           ((x)[2] << 8UL) +  \
                           ((x)[3]) )

#define ARRAY_TO_WORD(x) ( ((x)[0] << 8UL) + ((x)[1]) )

#define LONG_TO_ARRAY(x,y) (y)[0] = (UBYTE)(ULONG)((x) >> 24UL); \
    	    	    	   (y)[1] = (UBYTE)(ULONG)((x) >> 16UL); \
			   (y)[2] = (UBYTE)(ULONG)((x) >>  8UL); \
			   (y)[3] = (UBYTE)(ULONG)((x));

#define WORD_TO_ARRAY(x,y) (y)[0] = (UBYTE)(ULONG)((x) >>  8UL); \
			   (y)[1] = (UBYTE)(ULONG)((x));

#define ARRAY_TO_COLS(x,y) do {                                              \
                               int i;                                        \
                               for(i = 0; i < sizeof(x) / sizeof(x[0]); i++) \
                               {                                             \
				    struct MUI_RGBcolorArray *src = &x[i];   \
				    struct MUI_RGBcolor *dst = &y[i];        \
				    dst->red = ARRAY_TO_LONG(src->red);      \
				    dst->green = ARRAY_TO_LONG(src->green);  \
				    dst->blue = ARRAY_TO_LONG(src->blue);    \
			       }                                             \
                           } while(0);

#define COLS_TO_ARRAY(x,y) do {                                              \
                               int i;                                        \
                               for(i = 0; i < sizeof(x) / sizeof(x[0]); i++) \
                               {                                             \
				    struct MUI_RGBcolor *src = &x[i];   \
				    struct MUI_RGBcolorArray *dst = &y[i];        \
				    LONG_TO_ARRAY(src->red, dst->red);       \
				    LONG_TO_ARRAY(src->green, dst->green);   \
				    LONG_TO_ARRAY(src->blue, dst->blue);     \
			       }                                             \
                           } while(0);

#define ID_MPUB MAKE_ID('M','P','U','B')

#endif
