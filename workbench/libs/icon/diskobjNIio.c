/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id: diskobj35io.c 21620 2004-05-09 22:11:26Z stegerg $
*/

/****************************************************************************************/

#include "icon_intern.h"

#   define DEBUG 0
#   include <aros/debug.h>

/****************************************************************************************/

static void RemoveToolType(STRPTR *tt)
{
    if (tt[0])
    {
    	FreeVec(tt[0]);
	
    	for(;;)
    	{
       	    tt[0] = tt[1];
	    if (!tt[0]) break;
	
    	    tt++;
    	}
    }
}

/****************************************************************************************/

/* Decode3NI() based on ModifyIcon by Dirk Stöcker */

/****************************************************************************************/

static char *DecodeNI(STRPTR *tt, UBYTE *outbuffer, LONG bits, LONG entries, WORD which, BOOL is_palette)
{
    LONG   numbits = 0, curentry = 0, bitbuf = 0, loop = 0, mask, val;
    UBYTE  byte;
    STRPTR src;

    if(is_palette)
    {
	src = *tt + 9;
	RemoveToolType(tt);
    }
    else
    {
	src = ""; /* a dummy start */
    }
    mask = (1 << bits) - 1;

    while(curentry < entries)
    {
	if(loop)
	{
      	    byte = 0;
	    --loop;
	}
	else
	{
	    if(!*src)
	    {
              	src = *tt;
              	if(!src || src[0] != 'I' || src[1] != 'M' || src[2] != '1' + which || src[3] != '=')
	      	{
                    return "NewIcon data truncated";
	      	}
              	else
              	{
                    src += 4; numbits = 0;
              	}
              	RemoveToolType(tt);
	    }
	    
	    byte = *(src++);
	    
	    if(!byte)
	    {
	    	return "NewIcon data invalid";
	    }
	    else if(byte < 0xA0)
	    {
	    	byte -= 0x20;
	    }
	    else if(byte < 0xD1)
	    {
	    	byte -= 0x51;
	    }
	    else
	    {
	    	loop = byte - 0xD1;
		byte = 0;
	    }
	}

	bitbuf = (bitbuf << 7) + byte;
	numbits += 7;
	
	while(numbits >= bits && curentry < entries)
	{
	  val = (bitbuf >> (numbits - bits)) & mask;

	  *outbuffer++ = val;

	  numbits -= bits;
	  curentry++;
	}
    }
    
    return 0;
}

/****************************************************************************************/

static BOOL ReadImageNI(struct NativeIcon *icon, WORD which, STRPTR *tooltypes,
    	    	    	struct IconBase *IconBase)
{
    struct Image35 *img;
    STRPTR  	    tt;
    LONG    	    width, height, numcols;
    ULONG   	    size;
    BOOL    	    transp;

    img = which ? &icon->icon35.img2 : &icon->icon35.img1;
    
    while((tt = *tooltypes))
    {
    	if (tt[0] == 'I' && tt[1] == 'M' && tt[2] == '1' + which && tt[3] == '=')
	{
	    break;
	}
	tooltypes++;
    }
    
    if (!tt) return FALSE;
    
    tt += 4;
    
    if (strlen(tt) < 5) return FALSE;
    
    width = tt[1] - 0x21;
    height = tt[2] - 0x21;
    
    /* Selected image must have same size as normal image otherwise ignore it. */
    if (which && ((width != icon->icon35.width) || (height != icon->icon35.height)))
    {
    	return FALSE;
    }
    
    numcols = (((tt[3] - 0x21) << 6) + (tt[4] - 0x21));
    transp = (tt[0] =='B') ? TRUE : FALSE;
   
    size = width * height;
    size += numcols * sizeof(struct ColorRegister);
    if (transp) size += RASSIZE(width, height);
    
    img->imagedata = AllocVec(size, MEMF_ANY);
    if (!img->imagedata) return FALSE;

    if (!which)
    {
    	icon->icon35.width  = width;
	icon->icon35.height = height;
	icon->icon35.flags  = ICON35F_FRAMELESS;
	icon->icon35.aspect = 0x1111; /* ?? */
    }
    
    img->palette = img->imagedata + width * height;
    img->mask = transp ? (img->palette + numcols * sizeof(struct ColorRegister)) : 
    	    	    	 NULL;
    img->flags = transp ? (IMAGE35F_HASPALETTE | IMAGE35F_HASTRANSPARENTCOLOR) :
    	    	    	  IMAGE35F_HASPALETTE;
    img->transparentcolor = 0;
    img->numcolors = numcols;

    img->depth = 1;    
    while((1L << img->depth) < img->numcolors)
    {	
    	img->depth++;
    }

    DecodeNI(tooltypes, img->palette, 8, img->numcolors * sizeof(struct ColorRegister), which, TRUE);
    DecodeNI(tooltypes, img->imagedata, img->depth, width * height, which, FALSE);
    
    if (transp) MakeMask35(img->imagedata, img->mask, 0, width, height);

    return TRUE;
}
			

/****************************************************************************************/

BOOL ReadIconNI(struct NativeIcon *icon, struct Hook *streamhook,
    	    	void *stream, struct IconBase *IconBase)
{
    STRPTR *tooltypes, tt;
    
    D(bug("ReadIconNI\n"));
        
    if (icon->icon35.img1.imagedata)
    {
    	/* It's an 3.5 style icon. Ignore possible NewIcon */
    	return TRUE;
    }
    
    tooltypes = icon->dobj.do_ToolTypes;
    if ( ! tooltypes) return TRUE;
    while ((tt = *tooltypes))
    {
    	if (strcmp(tt, "*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0)
	{
	    break;
	}
	
	tooltypes++;
    }
    
    if (!tt) return TRUE;
    
    RemoveToolType(tooltypes);
    
    ReadImageNI(icon, 0, tooltypes, IconBase);
    ReadImageNI(icon, 1, tooltypes, IconBase);
        
    return TRUE;   
}

/****************************************************************************************/

BOOL WriteIconNI(struct NativeIcon *icon, struct Hook *streamhook,
    	    	 void *stream, struct IconBase *IconBase)
{
    D(bug("WriteIconNI\n"));
    
    return TRUE;   
}

/****************************************************************************************/

VOID FreeIconNI(struct NativeIcon *icon, struct IconBase *IconBase)
{
    D(bug("FreeIconNI\n"));
    
    /* Don't do anything */    
}

/****************************************************************************************/

