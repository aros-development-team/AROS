/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#include <aros/debug.h>

#include "icon_intern.h"

/****************************************************************************************/

static STRPTR RemoveToolType(STRPTR *tt)
{
    STRPTR ret = tt[0];
    
    if (ret)
    {
    	for(;;)
    	{
       	    tt[0] = tt[1];
	    if (!tt[0]) break;
	
    	    tt++;
    	}
    }
    
    return ret;
}

/****************************************************************************************/

/* DecodeNI() based on ModifyIcon by Dirk Stöcker */

/****************************************************************************************/

static char *DecodeNI(struct DiskObject *icon, STRPTR *tt, UBYTE *outbuffer, LONG bits, LONG entries, WORD which, BOOL is_palette, APTR IconBase)
{
    LONG   numbits = 0, curentry = 0, bitbuf = 0, loop = 0, mask, val;
    UBYTE  byte;
    STRPTR src, dead = NULL;

    if(is_palette)
    {
	src = *tt + 9;
	dead = RemoveToolType(tt);
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
	    	if (dead)
		{
		    dead = NULL;
		}
              	src = *tt;
              	if(!src || src[0] != 'I' || src[1] != 'M' || src[2] != '1' + which || src[3] != '=')
	      	{
                    return "NewIcon data truncated";
	      	}
              	else
              	{
                    src += 4; numbits = 0;
              	}
              	dead = RemoveToolType(tt);
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
    struct NativeIconImage *img;
    STRPTR  	    tt;
    LONG    	    width, height, numcols;
    ULONG   	    size;
    BOOL    	    transp;
    int bits;

    img = which ? &icon->ni_Image[1] : &icon->ni_Image[0];
    
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
    if (which && ((width != icon->ni_Width) || (height != icon->ni_Height)))
    {
    	return FALSE;
    }
    
    numcols = (((tt[3] - 0x21) << 6) + (tt[4] - 0x21));
    transp = (tt[0] =='B') ? TRUE : FALSE;
   
    size = width * height;
    
    img->ImageData = AllocMemIcon(&icon->ni_DiskObject, size, MEMF_PUBLIC);
    if (!img->ImageData) return FALSE;

    if (!which)
    {
    	icon->ni_Width  = width;
	icon->ni_Height = height;
	icon->ni_Frameless = TRUE;
	icon->ni_Aspect = PACK_ICON_ASPECT_RATIO(1,1);
    }
    
    size = numcols * sizeof(struct ColorRegister);
    img->Palette = AllocMemIcon(&icon->ni_DiskObject, size, MEMF_PUBLIC);
    if (!img->Palette) return FALSE;

    img->TransparentColor = transp ? -1 : 0;
    img->Pens = numcols;

    DecodeNI(&icon->ni_DiskObject, tooltypes, (UBYTE *)img->Palette, 8, img->Pens * sizeof(struct ColorRegister), which, TRUE, IconBase);
    for (bits = 1; (1 << bits) < numcols; bits++);
    DecodeNI(&icon->ni_DiskObject, tooltypes, (UBYTE *)img->ImageData, bits, width * height, which, FALSE, IconBase);
    
    return TRUE;
}
			

/****************************************************************************************/

BOOL ReadIconNI(struct NativeIcon *icon, struct Hook *streamhook,
    	    	void *stream, struct IconBase *IconBase)
{
    STRPTR *tooltypes, tt;
    
    D(bug("ReadIconNI\n"));
        
    if (icon->ni_Image[0].ImageData)
    {
    	/* It's an 3.5 style icon. Ignore possible NewIcon */
    	return TRUE;
    }
    
    tooltypes = icon->ni_DiskObject.do_ToolTypes;
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

    if (!ReadImageNI(icon, 0, tooltypes, IconBase))
        return FALSE;

    if (!ReadImageNI(icon, 1, tooltypes, IconBase))
        return FALSE;
        
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

