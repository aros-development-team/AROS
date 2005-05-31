/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function Text()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <string.h>
#include <proto/cybergraphics.h>
#include <aros/macros.h>
#include <aros/debug.h>

#include "gfxfuncsupport.h"

void BltTemplateBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
    	    	    	  struct GfxBase *GfxBase);

void BltTemplateAlphaBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
    	    	    	       struct GfxBase *GfxBase);

void ColorFontBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
    	    	    	struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH3(void, Text,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(CONST_STRPTR     , string, A0),
	AROS_LHA(ULONG            , count, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 10, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    if (count)
    {
	struct ColorTextFont *ctf = (struct ColorTextFont *)rp->Font;
    	BOOL	    	      antialias;
	BOOL	    	      colorfont;
		
	antialias = (ctf->ctf_TF.tf_Style & FSF_COLORFONT) &&
	    	    (ctf->ctf_Flags & CT_ANTIALIAS) &&
		    (GetBitMapAttr(rp->BitMap, BMA_DEPTH) >= 15);

	colorfont = (ctf->ctf_TF.tf_Style & FSF_COLORFONT) &&
	    	    (!(ctf->ctf_Flags & CT_ANTIALIAS));
	
	if (antialias)
	{
    	    BltTemplateAlphaBasedText(rp, string, count, GfxBase);
	}		
	else if (colorfont)
	{
	    ColorFontBasedText(rp, string, count, GfxBase);
	}
    	else if ((rp->DrawMode & INVERSVID) ||
	    	 (rp->AlgoStyle & (FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED)))
	{
    	    BltTemplateBasedText(rp, string, count, GfxBase);
	}
	else
	{
    	    driver_Text (rp, string, count, GfxBase);
	}
    }

    AROS_LIBFUNC_EXIT
    
} /* Text */

/***************************************************************************/

void BltTemplateBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
    	    	    	  struct GfxBase *GfxBase)
{
    struct TextExtent 	 te;
    struct TextFont 	*tf;
    WORD    	    	 raswidth, raswidth16, raswidth_bpr, rasheight, x, y, gx;
    UBYTE   	    	*raster;
    BOOL    	    	 is_bold, is_italic;
        
    TextExtent(rp, text, len, &te);
    
    raswidth  = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
    rasheight = te.te_Extent.MaxY - te.te_Extent.MinY + 1;
    
    raswidth16 = (raswidth + 15) & ~15;
    raswidth_bpr = raswidth16 / 8;
    
    if ((raster = AllocRaster(raswidth, rasheight)))
    {
    	memset(raster, 0, RASSIZE(raswidth, rasheight));
	
	tf = rp->Font;
	
	x = -te.te_Extent.MinX;
	
	is_bold   = (rp->AlgoStyle & FSF_BOLD) != 0;
	is_italic = (rp->AlgoStyle & FSF_ITALIC) != 0;
	
	while(len--)
	{
	    UBYTE c = *text++;
	    ULONG idx;
	    ULONG charloc;
	    UWORD glyphwidth, glyphpos, bold;
	    UBYTE *glyphdata;
	    UBYTE *dst;
	    ULONG srcmask;
	    ULONG dstmask;
	    
	    if (c < tf->tf_LoChar || c > tf->tf_HiChar)
	    {
	    	idx = tf->tf_HiChar - tf->tf_LoChar;
	    }
	    else
	    {
	    	idx = c - tf->tf_LoChar;
	    }

	    charloc = ((ULONG *)tf->tf_CharLoc)[idx];

    	    glyphwidth = charloc & 0xFFFF;
	    glyphpos = charloc >> 16;
	    
    	    if (tf->tf_CharKern)
	    {
	    	x += ((WORD *)tf->tf_CharKern)[idx];
	    }
	    	
	       
	    for(bold = 0; bold <= is_bold; bold++)
	    {
	    	WORD wx;
		WORD italicshift, italiccheck = 0;
		
		if (is_italic)
		{
	    	    italiccheck = tf->tf_Baseline;
	    	    italicshift = italiccheck / 2;		
		}
		else
		{
	    	    italicshift = 0;
		}
		
		wx = x + italicshift + (bold ? tf->tf_BoldSmear : 0);
		
		glyphdata = ((UBYTE *)tf->tf_CharData) + glyphpos / 8;
		dst = raster + wx / 8;

		for(y = 0; y < rasheight; y++)
		{
	    	    UBYTE *glyphdatax = glyphdata;
		    UBYTE *dstx = dst;
		    UBYTE srcdata;

	    	    srcmask = 0x80 >> (glyphpos & 7);
	    	    dstmask = 0x80 >> (wx & 7);

    	    	    srcdata = *glyphdatax;

	    	    for(gx = 0; gx < glyphwidth; gx++)
		    {
			if (srcdata & srcmask)
			{
		    	    *dstx |= dstmask;
			}

			if (dstmask == 0x1)
			{
		    	    dstmask = 0x80;
			    dstx++;
			}
			else
			{
		    	    dstmask >>= 1;
			}

			if (srcmask == 0x1)
			{
		    	    srcmask = 0x80;
			    glyphdatax++;
			    srcdata =*glyphdatax;
			}
			else
			{
		    	    srcmask >>= 1;
			}
			
		    } /* for(gx = 0; gx < glyphwidth; gx++) */

		    glyphdata += tf->tf_Modulo;
		    dst += raswidth_bpr;
		    
		    if (is_italic)
		    {
		    	italiccheck--;
			if (italiccheck & 1)
			{
			    italicshift--;
			    
			    wx--;
			    if ((wx & 7) == 7) dst--;
			    
			}
		    }
		    
		} /* for(y = 0; y < rasheight; y++) */
		
	    } /* for(bold = 0; bold < ((rp->AlgoStyle & FSF_BOLD) ? 2 : 1); bold++) */
	    
	    if (tf->tf_CharSpace)
	    {
	    	x += ((WORD *)tf->tf_CharSpace)[idx];
	    }
	    else
	    {
	    	x += tf->tf_XSize;
	    }
	    
	    x += rp->TxSpacing;
	    
	} /* while(len--) */
	
	if (rp->AlgoStyle & FSF_UNDERLINED)
	{
	    UBYTE *dst;
	    ULONG prev_word, act_word = 0, next_word, word;
	    WORD count;
	    LONG underline;
	    
	    underline = rp->TxBaseline + 1;
	    if (underline < rasheight - 1) underline++;
	    
	    if (underline < rasheight)
	    {
		dst = raster + underline * (LONG)raswidth_bpr;
		next_word = *(UWORD *)dst;
    	    #if !AROS_BIG_ENDIAN
	    	next_word = AROS_WORD2BE(next_word);
	    #endif
		count  = raswidth16 / 16;

		while(count--)
		{
	    	    prev_word = act_word;
		    act_word = next_word;
		    if (count > 1)
		    {
			next_word = ((UWORD *)dst)[1];
			
		    #if !AROS_BIG_ENDIAN
		    	next_word = AROS_WORD2BE(next_word);
		    #endif
		    }
		    else
		    {
			next_word = 0;
		    }
		    word = ((act_word << 1) & 0xFFFF) + (next_word >> 15);
		    word |= (act_word >> 1) + ((prev_word << 15) & 0xFFFF);
		    word &= ~act_word;

		    word = 0xFFFF &~ word;
    	    #if !AROS_BIG_ENDIAN
	    	    word = AROS_BE2WORD(word);
	    #endif

		    *(UWORD *)dst = word;
		    dst += 2;

		} /* while(count--) */
		
	    } /* if (underline < rasheight) */
	    
	} /* if (rp->AlgoStyle & FSF_UNDERLINED) */
	
	BltTemplate(raster,
	    	    0,
		    raswidth_bpr,
		    rp,
		    rp->cp_x + te.te_Extent.MinX,
		    rp->cp_y - rp->TxBaseline,
		    raswidth,
		    rasheight);
		    	
    	FreeRaster(raster, raswidth, rasheight);
	
    } /* if ((raster = AllocRaster(raswidth, rasheight))) */
    
    Move(rp, rp->cp_x + te.te_Width, rp->cp_y);
    
}

/***************************************************************************/

#define CyberGfxBase (PrivGBase(GfxBase)->cybergfxbase)

/***************************************************************************/

void BltTemplateAlphaBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
    	    	    	       struct GfxBase *GfxBase)
{
    struct TextExtent 	 te;
    struct TextFont 	*tf;
    WORD    	    	 raswidth, raswidth_bpr, rasheight, x, y, gx;
    UBYTE   	    	*raster;
    BOOL    	    	 is_bold, is_italic;

    if (!CyberGfxBase)
    {
    	CyberGfxBase = OpenLibrary("cybergraphics.library", 0);
	if (!CyberGfxBase) return;
    }
    
    TextExtent(rp, text, len, &te);
    
    raswidth  = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
    rasheight = te.te_Extent.MaxY - te.te_Extent.MinY + 1;
    
    raswidth_bpr = raswidth;
    
    if ((raster = AllocVec(raswidth * rasheight, MEMF_CLEAR)))
    {
 	tf = rp->Font;
	
	x = -te.te_Extent.MinX;
	
	is_bold   = (rp->AlgoStyle & FSF_BOLD) != 0;
	is_italic = (rp->AlgoStyle & FSF_ITALIC) != 0;
	
	while(len--)
	{
	    UBYTE c = *text++;
	    ULONG idx;
	    ULONG charloc;
	    UWORD glyphwidth, glyphpos, bold;
	    UBYTE *glyphdata;
	    UBYTE *dst;
	    
	    if (c < tf->tf_LoChar || c > tf->tf_HiChar)
	    {
	    	idx = tf->tf_HiChar - tf->tf_LoChar;
	    }
	    else
	    {
	    	idx = c - tf->tf_LoChar;
	    }

	    charloc = ((ULONG *)tf->tf_CharLoc)[idx];

    	    glyphwidth = charloc & 0xFFFF;
	    glyphpos = charloc >> 16;
	    
    	    if (tf->tf_CharKern)
	    {
	    	x += ((WORD *)tf->tf_CharKern)[idx];
	    }
	    	
	       
	    for(bold = 0; bold <= is_bold; bold++)
	    {
	    	WORD wx;
		WORD italicshift, italiccheck = 0;
		
		if (is_italic)
		{
	    	    italiccheck = tf->tf_Baseline;
	    	    italicshift = italiccheck / 2;		
		}
		else
		{
	    	    italicshift = 0;
		}
		
		wx = x + italicshift + (bold ? tf->tf_BoldSmear : 0);
		
		glyphdata = ((UBYTE *)((struct ColorTextFont *)tf)->ctf_CharData[0]) + glyphpos;
		dst = raster + wx;

		for(y = 0; y < rasheight; y++)
		{
	    	    UBYTE *glyphdatax = glyphdata;
		    UBYTE *dstx = dst;

	    	    for(gx = 0; gx < glyphwidth; gx++)
		    {
		    	UWORD old = *dstx;
			
			old += *glyphdatax++;
			if (old > 255) old = 255;
    	    	    	*dstx++ = old;
		    }

		    glyphdata += tf->tf_Modulo * 8;
		    dst += raswidth_bpr;
		    
		    if (is_italic)
		    {
		    	italiccheck--;
			if (italiccheck & 1)
			{
			    italicshift--;
			    dst--;			    
			}
		    }
		    
		} /* for(y = 0; y < rasheight; y++) */
		
	    } /* for(bold = 0; bold < ((rp->AlgoStyle & FSF_BOLD) ? 2 : 1); bold++) */
	    
	    if (tf->tf_CharSpace)
	    {
	    	x += ((WORD *)tf->tf_CharSpace)[idx];
	    }
	    else
	    {
	    	x += tf->tf_XSize;
	    }
	    
	    x += rp->TxSpacing;
	    
	} /* while(len--) */

	if (rp->AlgoStyle & FSF_UNDERLINED)
	{
	    UBYTE *dst;
	    UBYTE prev_byte, act_byte = 0, next_byte;
	    WORD count;
	    LONG underline;
	    
	    underline = rp->TxBaseline + 1;
	    if (underline < rasheight - 1) underline++;
	    
	    if (underline < rasheight)
	    {
		dst = raster + underline * (LONG)raswidth_bpr;
		count  = raswidth;

    	    	next_byte = *dst;
		
		while(count--)
		{
	    	    prev_byte = act_byte;
		    act_byte = next_byte;
		    if (count > 1)
		    {
			next_byte = dst[1];
		    }
		    else
		    {
			next_byte = 0;
		    }

		    *dst++ = (act_byte || (!prev_byte && !next_byte)) ? 255 : 0;

		} /* while(count--) */
		
	    } /* if (underline < rasheight) */
	    
	} /* if (rp->AlgoStyle & FSF_UNDERLINED) */
	
	BltTemplateAlpha(raster,
	    		 0,
			 raswidth_bpr,
			 rp,
			 rp->cp_x + te.te_Extent.MinX,
			 rp->cp_y - rp->TxBaseline,
			 raswidth,
			 rasheight);
		    	
    	FreeVec(raster);
	
    } /* if ((raster = AllocVec(raswidth * rasheight, MEMF_CLEAR))) */
    
    Move(rp, rp->cp_x + te.te_Width, rp->cp_y);
    
}

/***************************************************************************/

#undef CyberGfxBase

/***************************************************************************/

void ColorFontBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
    	    	    	struct GfxBase *GfxBase)
{
    struct TextExtent 	 te;
    struct TextFont 	*tf;
    WORD    	    	 raswidth, raswidth_bpr, rasheight, x, y, gx;
    UBYTE   	    	*raster, *chunky;
    BOOL    	    	 is_bold, is_italic;

    tf = rp->Font;
    if (!ExtendFont(tf, NULL)) return;
    
    chunky = TFE_INTERN(tf->tf_Extension)->hash->chunky_colorfont;
   
    TextExtent(rp, text, len, &te);

    if ((rp->DrawMode & ~INVERSVID) == JAM2)
    {
    	ULONG 	    	  old_drmd = GetDrMd(rp);

	SetDrMd(rp, old_drmd ^ INVERSVID);
	RectFill(rp, rp->cp_x + te.te_Extent.MinX,
	    	     rp->cp_y + te.te_Extent.MinY,
		     rp->cp_x + te.te_Extent.MaxX,
		     rp->cp_y + te.te_Extent.MaxY);
	SetDrMd(rp, old_drmd);
    }

    
    raswidth  = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
    rasheight = te.te_Extent.MaxY - te.te_Extent.MinY + 1;
    
    raswidth_bpr = raswidth;
    
    if ((raster = AllocVec(raswidth * rasheight, MEMF_CLEAR)))
    {	
	x = -te.te_Extent.MinX;
	
	is_bold   = (rp->AlgoStyle & FSF_BOLD) != 0;
	is_italic = (rp->AlgoStyle & FSF_ITALIC) != 0;

kprintf("is_bold %d\n", is_bold);
	
	while(len--)
	{
	    UBYTE c = *text++;
	    ULONG idx;
	    ULONG charloc;
	    UWORD glyphwidth, glyphpos, bold;
	    UBYTE *glyphdata;
	    UBYTE *dst;
	    
	    if (c < tf->tf_LoChar || c > tf->tf_HiChar)
	    {
	    	idx = tf->tf_HiChar - tf->tf_LoChar;
	    }
	    else
	    {
	    	idx = c - tf->tf_LoChar;
	    }

	    charloc = ((ULONG *)tf->tf_CharLoc)[idx];

    	    glyphwidth = charloc & 0xFFFF;
	    glyphpos = charloc >> 16;
	    
    	    if (tf->tf_CharKern)
	    {
	    	x += ((WORD *)tf->tf_CharKern)[idx];
	    }
	    	
	       
	    for(bold = 0; bold <= is_bold; bold++)
	    {
	    	WORD wx;
		WORD italicshift, italiccheck = 0;
		
		if (is_italic)
		{
	    	    italiccheck = tf->tf_Baseline;
	    	    italicshift = italiccheck / 2;		
		}
		else
		{
	    	    italicshift = 0;
		}
		
		wx = x + italicshift + (bold ? tf->tf_BoldSmear : 0);
		
		glyphdata = chunky + glyphpos;
		dst = raster + wx;

		for(y = 0; y < rasheight; y++)
		{
	    	    UBYTE *glyphdatax = glyphdata;
		    UBYTE *dstx = dst;

	    	    for(gx = 0; gx < glyphwidth; gx++)
		    {
		    	UBYTE p = *glyphdatax++;
    	    	    	
			if (p || !bold) *dstx = p;
			
			dstx++;
		    }

		    glyphdata += tf->tf_Modulo * 8;
		    dst += raswidth_bpr;
		    
		    if (is_italic)
		    {
		    	italiccheck--;
			if (italiccheck & 1)
			{
			    italicshift--;
			    dst--;			    
			}
		    }
		    
		} /* for(y = 0; y < rasheight; y++) */
		
	    } /* for(bold = 0; bold < ((rp->AlgoStyle & FSF_BOLD) ? 2 : 1); bold++) */
	    
	    if (tf->tf_CharSpace)
	    {
	    	x += ((WORD *)tf->tf_CharSpace)[idx];
	    }
	    else
	    {
	    	x += tf->tf_XSize;
	    }
	    
	    x += rp->TxSpacing;
	    
	} /* while(len--) */

#if 0
	if (rp->AlgoStyle & FSF_UNDERLINED)
	{
	    UBYTE *dst;
	    UBYTE prev_byte, act_byte = 0, next_byte;
	    WORD count;
	    LONG underline;
	    
	    underline = rp->TxBaseline + 1;
	    if (underline < rasheight - 1) underline++;
	    
	    if (underline < rasheight)
	    {
		dst = raster + underline * (LONG)raswidth_bpr;
		count  = raswidth;

    	    	next_byte = *dst;
		
		while(count--)
		{
	    	    prev_byte = act_byte;
		    act_byte = next_byte;
		    if (count > 1)
		    {
			next_byte = dst[1];
		    }
		    else
		    {
			next_byte = 0;
		    }

		    *dst++ = (act_byte || (!prev_byte && !next_byte)) ? 255 : 0;

		} /* while(count--) */
		
	    } /* if (underline < rasheight) */
	    
	} /* if (rp->AlgoStyle & FSF_UNDERLINED) */
	
#endif

#if 1
    	{
	    HIDDT_PixelLUT pixlut;

	    pixlut.entries = AROS_PALETTE_SIZE;
	    pixlut.pixels  = IS_HIDD_BM(rp->BitMap) ? HIDD_BM_PIXTAB(rp->BitMap) : NULL;
	    
	    write_transp_pixels_8(rp, raster,raswidth_bpr,
	    	    	    	  rp->cp_x + te.te_Extent.MinX,
				  rp->cp_y - rp->TxBaseline,
				  rp->cp_x + te.te_Extent.MinX + raswidth - 1,
				  rp->cp_y - rp->TxBaseline + rasheight - 1,
				  &pixlut, 0, GfxBase);
	    
	}
	
#else
    	WriteChunkyPixels(rp,
	    	    	  rp->cp_x + te.te_Extent.MinX,
			  rp->cp_y - rp->TxBaseline,
			  rp->cp_x + te.te_Extent.MinX + raswidth - 1,
			  rp->cp_y - rp->TxBaseline + rasheight - 1,
			  raster,
			  raswidth_bpr);
#endif

			  
    	FreeVec(raster);
	
    } /* if ((raster = AllocVec(raswidth * rasheight, MEMF_CLEAR))) */
    
    Move(rp, rp->cp_x + te.te_Width, rp->cp_y);

}
			
