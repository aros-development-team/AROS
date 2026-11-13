#ifndef DEVICES_PRTGFX_H
#define DEVICES_PRTGFX_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: graphics printer driver structures
    Lang: english
*/

#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

#define	PCMYELLOW	0
#define	PCMMAGENTA	1
#define	PCMCYAN		2
#define	PCMBLACK	3
#define PCMBLUE		PCMYELLOW
#define PCMGREEN	PCMMAGENTA
#define PCMRED		PCMCYAN
#define PCMWHITE	PCMBLACK

union colorEntry
{
    ULONG	colorLong;
    UBYTE	colorByte[4];
    BYTE	colorSByte[4];
};

/****************************************************************************/

struct PrtInfo
{
    LONG		(*pi_render)();         /* PRIVATE */
    struct RastPort 	 *pi_rp;                /* PRIVATE */
    struct RastPort 	 *pi_temprp;            /* PRIVATE */
    UWORD   	    	 *pi_RowBuf;            /* PRIVATE */
    UWORD   	    	 *pi_HamBuf;            /* PRIVATE */
    union colorEntry 	 *pi_ColorMap;          /* PRIVATE */
    union colorEntry 	 *pi_ColorInt;      /* Colors for the row */
    union colorEntry 	 *pi_HamInt;            /* PRIVATE */
    union colorEntry 	 *pi_Dest1Int;          /* PRIVATE */
    union colorEntry 	 *pi_Dest2Int;          /* PRIVATE */
    UWORD   	    	 *pi_ScaleX;        /* Array of X scale values */
    UWORD   	    	 *pi_ScaleXAlt;         /* PRIVATE */
    UBYTE   	    	 *pi_dmatrix;       /* Pointer to dither matrix */
    UWORD   	    	 *pi_TopBuf;            /* PRIVATE */
    UWORD   	    	 *pi_BotBuf;            /* PRIVATE */

    UWORD		  pi_RowBufSize;        /* PRIVATE */
    UWORD		  pi_HamBufSize;        /* PRIVATE */
    UWORD		  pi_ColorMapSize;      /* PRIVATE */
    UWORD		  pi_ColorIntSize;      /* PRIVATE */
    UWORD		  pi_HamIntSize;        /* PRIVATE */
    UWORD		  pi_Dest1IntSize;      /* PRIVATE */
    UWORD		  pi_Dest2IntSize;      /* PRIVATE */
    UWORD		  pi_ScaleXSize;        /* PRIVATE */
    UWORD		  pi_ScaleXAltSize;     /* PRIVATE */

    UWORD		  pi_PrefsFlags;        /* PRIVATE */
    ULONG		  pi_special;           /* PRIVATE */
    UWORD		  pi_xstart;            /* PRIVATE */
    UWORD		  pi_ystart;            /* PRIVATE */
    UWORD		  pi_width;         /* Source width */
    UWORD		  pi_height;        /* Source height */
    ULONG		  pi_pc;                /* PRIVATE */
    ULONG		  pi_pr;                /* PRIVATE */
    UWORD		  pi_ymult;             /* PRIVATE */
    UWORD		  pi_ymod;              /* PRIVATE */
    WORD		  pi_ety;               /* PRIVATE */
    UWORD		  pi_xpos;          /* Offset of the line */
    UWORD		  pi_threshold;     /* Theshold from Preferences */
    UWORD		  pi_tempwidth;         /* PRIVATE */
    UWORD		  pi_flags;             /* PRIVATE */

    /* New in V44 */
    UWORD   	    	 *pi_ReduceBuf;         /* PRIVATE */
    UWORD	    	  pi_ReduceBufSize;     /* PRIVATE */
    struct Hook      	 *pi_SourceHook;        /* PRIVATE */
    ULONG   	    	 *pi_InvertHookBuf;     /* PRIVATE */
};


#endif /* DEVICES_PRTGFX_H */
