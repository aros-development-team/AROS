#ifndef HIDD_GRAPHICS_H
#define HIDD_GRAPHICS_H

/*
    Copyright (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Definitions for the Graphics HIDD system.
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif

#define GRAPHICSHIDD       "graphics.hidd"
#define GRAPHICSHIDDBITMAP "graphicsbitmap.hidd"
#define GRAPHICSHIDDGC     "graphicsgc.hidd"


typedef struct Object *HIDDT_BitMap;
typedef struct Object *HIDDT_GC;


/* Methods implemented by graphics hidd */
enum
{
    HIDDM_GraphicsBase = 0x80200,
    HIDDM_SpeedTest,         /* method for speedtest */
    HIDDM_Graphics_QCmd,
    HIDDM_Graphics_Cmd,
    HIDDM_Graphics_MCmd,
    HIDDM_Graphics_MQCmd,

    /* graphics context */
    HIDDM_Graphics_GCBase = 0x80400,
    HIDDM_Graphics_GC_CreateGC,
    HIDDM_Graphics_GC_DeleteGC,
    HIDDM_Graphics_GC_CopyArea,
    HIDDM_Graphics_GC_WritePixelDirect,
    HIDDM_Graphics_GC_WritePixelDirect_Q,
    HIDDM_Graphics_GC_WritePixel,
    HIDDM_Graphics_GC_WritePixel_Q,
    HIDDM_Graphics_GC_ReadPixel,
    HIDDM_Graphics_GC_ReadPixel_Q,
    HIDDM_Graphics_GC_DrawLine,
    HIDDM_Graphics_GC_DrawLine_Q,
    HIDDM_Graphics_GC_DrawRect,
    HIDDM_Graphics_GC_DrawRect_Q,
    HIDDM_Graphics_GC_FillRect,
    HIDDM_Graphics_GC_FillRect_Q,
    HIDDM_Graphics_GC_DrawEllipse,
    HIDDM_Graphics_GC_DrawEllipse_Q,
    HIDDM_Graphics_GC_FillEllipse,
    HIDDM_Graphics_GC_FillEllipse_Q,
    HIDDM_Graphics_GC_DrawPolygon,
    HIDDM_Graphics_GC_DrawPolygon_Q,
    HIDDM_Graphics_GC_FillPolygon,
    HIDDM_Graphics_GC_FillPolygon_Q,
    HIDDM_Graphics_GC_DrawText,
    HIDDM_Graphics_GC_DrawText_Q,
    HIDDM_Graphics_GC_FillText,
    HIDDM_Graphics_GC_FillText_Q,
    HIDDM_Graphics_GC_FillSpan,
    HIDDM_Graphics_GC_FillSpan_Q,
    HIDDM_Graphics_GC_Clear,
    HIDDM_Graphics_GC_Clear_Q
};


/* Attributes for the Graphics HIDD */
enum {
    HIDDA_TimerBase = HIDDA_Base + 0x02000,

    /* Attributes for a bitmap */
    HIDDA_BitMap_BitMap,        /* [..G] pointer to bitmap structure        */
    HIDDA_BitMap_Width,         /* [ISG] Bitmap with                        */
    HIDDA_BitMap_Height,        /* [ISG] Bitmap height                      */
    HIDDA_BitMap_Depth,         /* [I.G] Bitmap depth                       */
    HIDDA_BitMap_Displayable,   /* [I.G] BOOL bitmap is displayable         */
    HIDDA_BitMap_Visible,       /* [..G] Check if a bitmap is visible       */
    HIDDA_BitMap_Mode,          /* [ISG] The display mode of this bitmap    */
    HIDDA_BitMap_BaseAddress,   /* [ISG] Bitmap adress in RAM               */
    HIDDA_BitMap_Format,        /* [..G] Tell the format of the bitmap data */
    HIDDA_BitMap_BytesPerRow,   /* [..G] Number of bytes in a row           */
    HIDDA_BitMap_BytesPerPixel, /* [..G] Number of byter per pixel          */
    HIDDA_BitMap_BestSize,      /* [..G] Best size for depth                */
    HIDDA_BitMap_LeftEdge,      /* [I.G] Left edge position of the bitmap   */
    HIDDA_BitMap_TopEdge,       /* [I.G] Top edge position of the bitmap    */

    /* Attributes for a graphics context */
    HIDDA_GC_UserData,          /* [.SG] User data                          */
    HIDDA_GC_BitMap,            /* [I.G] Bitmap which this gc uses          */
    HIDDA_GC_Foreground,        /* [.SG] Foreground color                   */
    HIDDA_GC_Background,        /* [.SG] Background color                   */
    HIDDA_GC_DrawMode,          /* [.SG] Draw mode                          */
    HIDDA_GC_Font,              /* [.SG] Current font                       */
    HIDDA_GC_ColorMask,         /* [.SG] Prevents some color bits from      */
                                /*       changing                           */
    HIDDA_GC_LinePattern,       /* [.SG] Pattern foor line drawing          */
    HIDDA_GC_PlaneMask          /* [.SG] Shape bitmap                       */
};


/* Drawmodes for a graphics context */
#define HIDDV_GC_DrawMode_Copy 0x03 /* Copy src into destination            */
#define HIDDV_GC_DrawMode_XOr  0x06 /* XOR                                  */


enum {

    /* bitmap */
    HIDDV_Graphics_Cmd_CreateBitMap,
    HIDDV_Graphics_Cmd_DeleteBitMap,
    HIDDV_Graphics_Cmd_ShowBitMap,
    HIDDV_Graphics_Cmd_MoveBitMap,
    HIDDV_Graphics_Cmd_DepthArrangeBitMap,

    /* graphics context */
    HIDDV_Graphics_Cmd_CreateGC,
    HIDDV_Graphics_Cmd_DeleteGC,
    HIDDV_Graphics_Cmd_CopyArea,
    HIDDV_Graphics_Cmd_WritePixelDirect,
    HIDDV_Graphics_Cmd_WritePixel,
    HIDDV_Graphics_Cmd_ReadPixel,
    HIDDV_Graphics_Cmd_DrawLine,
    HIDDV_Graphics_Cmd_DrawRect,
    HIDDV_Graphics_Cmd_FillRect,
    HIDDV_Graphics_Cmd_DrawEllipse,
    HIDDV_Graphics_Cmd_FillEllipse,
    HIDDV_Graphics_Cmd_DrawPolygon,
    HIDDV_Graphics_Cmd_FillPolygon,
    HIDDV_Graphics_Cmd_DrawText,
    HIDDV_Graphics_Cmd_FillText,
    HIDDV_Graphics_Cmd_FillSpan,
    HIDDV_Graphics_Cmd_Clear,
    
    /* misc */
    HIDDV_Graphics_Cmd_WaitTOF,
    HIDDV_Graphics_Cmd_CheckTOF,
    HIDDV_Graphics_Cmd_Special
};


/* Flags for HIDD_Graphics_CreateBitMap */
#define HIDD_Graphics_BitMap_Flags_Displayable 0x01
#define HIDD_Graphics_BitMap_Flags_Planar      0x02
#define HIDD_Graphics_BitMap_Flags_Chunky      0x04


/* Messages */
struct hGfx_Point
{
    STACKULONG MethodID;
    STACKWORD  x, y;        /* Position in hidd units */
};


struct hGfx_PixelDirect
{
    STACKULONG MethodID;
    STACKWORD  x, y;        /* Position in hidd units  */
    STACKULONG val;         /* set pixel to this value */
};


struct hGfx_2Coords
{
    STACKULONG MethodID;
    STACKWORD  x1, y1;      /* eg. draw line from (x1, y1) to (x1,y2) */
    STACKWORD  x2, y2;
};


/* Message for HIDDM_Graphics_SpeedTest */
struct hGfx_SpeeTest
{
    STACKULONG  MethodID;
    STACKULONG  val1;
    STACKULONG  val2;
    STACKULONG  val3;
};

#endif /* HIDD_GRAPHICS_H */
