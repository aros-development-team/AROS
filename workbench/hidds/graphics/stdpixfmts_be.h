/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Standard pixel formats. Big endian machines.
    Lang: english
*/

/****************************************************************************************/

/* IMPORTANT: The order of these must match the order of the vHidd_StdPixFmt
              enum in <hidds/graphics.h> !!! */

const HIDDT_PixelFormat stdpfs[] = 
{
    {
    	  /* R8G8B8 */
	     
	  24, 24, 3
	, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000
	, 8, 16, 24, 0
	, 0, 0
	, vHidd_StdPixFmt_RGB24
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* B8G8R8 */
	  
	  24, 24, 3
	, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000
	, 24, 16, 8, 0
	, 0, 0
	, vHidd_StdPixFmt_BGR24
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* R5G6B5 */


	  16, 16, 2
	, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000
	, 16, 21, 27, 0
	, 0, 0
	, vHidd_StdPixFmt_RGB16
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* R5G6B5 little endian */

	  16, 16, 2
	, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000
	, 16, 21, 27, 0
	, 0, 0
	, vHidd_StdPixFmt_RGB16_LE
	, PF_GRAPHTYPE(TrueColor, Chunky) | vHidd_PixFmt_SwapPixelBytes_Flag
    }, {
    	  /* B5G6R5 */
    
	  16, 16, 2
	, 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000
	, 27, 21, 16, 0
	, 0, 0
	, vHidd_StdPixFmt_BGR16
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* B5G6R5 little endian */
    
	  16, 16, 2
	, 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000
	, 27, 21, 16, 0
	, 0, 0
	, vHidd_StdPixFmt_BGR16_LE
	, PF_GRAPHTYPE(TrueColor, Chunky) | vHidd_PixFmt_SwapPixelBytes_Flag
    }, {
    	  /* X1R5G5B5 */
	  
	  15, 15, 2
	, 0x00007C00, 0x000003E0, 0x0000001F, 0x00000000
	, 17, 22, 27, 0
	, 0, 0
	, vHidd_StdPixFmt_RGB15
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* X1R5G5B5 little endian */
	  
	  15, 15, 2
	, 0x00007C00, 0x000003E0, 0x0000001F, 0x00000000
	, 17, 22, 27, 0
	, 0, 0
	, vHidd_StdPixFmt_RGB15_LE
	, PF_GRAPHTYPE(TrueColor, Chunky) | vHidd_PixFmt_SwapPixelBytes_Flag
    }, {
    	  /* X1B5G5R5 */
	  
	  15, 15, 2
	, 0x0000001F, 0x000003E0, 0x00007C00, 0x00000000
	, 27, 22, 17, 0
	, 0, 0
	, vHidd_StdPixFmt_BGR15
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {   
    	  /* X1B5G5R5 little endian */
	  
	  15, 15, 2
	, 0x0000001F, 0x000003E0, 0x00007C00, 0x00000000
	, 27, 22, 17, 0
	, 0, 0
	, vHidd_StdPixFmt_BGR15_LE
	, PF_GRAPHTYPE(TrueColor, Chunky) | vHidd_PixFmt_SwapPixelBytes_Flag
    }, {   
    	  /* A8R8G8B8 */
	  
	  32, 32, 4
	, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
	, 8, 16, 24, 0
	, 0, 0
	, vHidd_StdPixFmt_ARGB32
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* B8G8R8A8 */
	  
	  32, 32, 4
	, 0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF
	, 16, 8, 0, 24
	, 0, 0
	, vHidd_StdPixFmt_BGRA32
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* R8G8B8A8 */
	  
	  32, 32, 4
	, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
	, 0, 8, 16, 24
	, 0, 0
	, vHidd_StdPixFmt_RGBA32
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* X8R8G8B8 */
	  
	  24, 24, 4
	, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000
	, 8, 16, 24, 0
	, 0, 0
	, vHidd_StdPixFmt_0RGB32
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* B8G8R8X8 */
	  
	  24, 24, 4
	, 0x0000FF00, 0x00FF0000, 0xFF000000, 0x00000000
	, 16, 8, 0, 0
	, 0, 0
	, vHidd_StdPixFmt_BGR032
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* R8G8B8X8 */
	  
	  24, 24, 4
	, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x00000000
	, 0, 8, 16, 0
	, 0, 0
	, vHidd_StdPixFmt_RGB032
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
    	  /* 8 Bit chunky */
	  
	  8, 8, 1
	, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000
	, 0, 0, 0, 0
	, 0x000000FF, 0
	, vHidd_StdPixFmt_LUT8
	, PF_GRAPHTYPE(Palette, Chunky)
    }, {
    	  /* 1 Bit planar */
	  
    	  1, 1, 1
	, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000
	, 0, 0, 0, 0
	, 0x0000000F, 0
	, vHidd_StdPixFmt_Plane
	, PF_GRAPHTYPE(Palette, Planar)
    }
    
};
