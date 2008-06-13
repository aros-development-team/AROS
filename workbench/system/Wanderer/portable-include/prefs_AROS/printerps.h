#ifndef PREFS_PRINTERPS_H
#define PREFS_PRINTERPS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Printerps prefs definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif


#define ID_PSPD MAKE_ID('P','S','P','D')


struct PrinterPSPrefs
{
    LONG  ps_Reserved[4];

    UBYTE ps_DriverMode;
    UBYTE ps_PaperFormat;
    UBYTE ps_Reserved1[2];
    LONG  ps_Copies;
    LONG  ps_PaperWidth;
    LONG  ps_PaperHeight;
    LONG  ps_HorizontalDPI;
    LONG  ps_VerticalDPI;

    UBYTE ps_Font;
    UBYTE ps_Pitch;
    UBYTE ps_Orientation;
    UBYTE ps_Tab;
    UBYTE ps_Reserved2[8];

    LONG  ps_LeftMargin;
    LONG  ps_RightMargin;
    LONG  ps_TopMargin;
    LONG  ps_BottomMargin;
    LONG  ps_FontPointSize;
    LONG  ps_Leading;
    UBYTE ps_Reserved3[8];

    LONG  ps_LeftEdge;
    LONG  ps_TopEdge;
    LONG  ps_Width;
    LONG  ps_Height;
    UBYTE ps_Image;
    UBYTE ps_Shading;
    UBYTE ps_Dithering;
    UBYTE ps_Reserved4[9];

    UBYTE ps_Aspect;
    UBYTE ps_ScalingType;
    UBYTE ps_Reserved5;
    UBYTE ps_Centering;
    UBYTE ps_Reserved6[8];
};

#define DM_POSTSCRIPT  0
#define DM_PASSTHROUGH 1

#define PF_USLETTER 0
#define PF_USLEGAL  1
#define PF_A4	    2
#define PF_CUSTOM   3

#define FONT_COURIER	  0
#define FONT_TIMES	  1
#define FONT_HELVETICA	  2
#define FONT_HELV_NARROW  3
#define FONT_AVANTGARDE   4
#define FONT_BOOKMAN	  5
#define FONT_NEWCENT	  6
#define FONT_PALATINO	  7
#define FONT_ZAPFCHANCERY 8

#define PITCH_NORMAL	 0
#define PITCH_COMPRESSED 1
#define PITCH_EXPANDED	 2

#define ORIENT_PORTRAIT  0
#define ORIENT_LANDSCAPE 1

#define TAB_4	  0
#define TAB_8	  1
#define TAB_QUART 2
#define TAB_HALF  3
#define TAB_INCH  4

#define IM_POSITIVE 0
#define IM_NEGATIVE 1

#define SHAD_BW        0
#define SHAD_GREYSCALE 1
#define SHAD_COLOR     2

#define DITH_DEFAULT 0
#define DITH_DOTTY   1
#define DITH_VERT    2
#define DITH_HORIZ   3
#define DITH_DIAG    4

#define ASP_HORIZ 0
#define ASP_VERT  1

#define ST_ASPECT_ASIS	  0
#define ST_ASPECT_WIDE	  1
#define ST_ASPECT_TALL	  2
#define ST_ASPECT_BOTH	  3
#define ST_FITS_WIDE	  4
#define ST_FITS_TALL	  5
#define ST_FITS_BOTH	  6

#define CENT_NONE  0
#define CENT_HORIZ 1
#define CENT_VERT  2
#define CENT_BOTH  3

#endif /* PREFS_PRINTERPS_H */
