/*
    Copyright  1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

/**************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/datatypes.h>

#include <aros/symbolsets.h>

#include <png.h>

#include <setjmp.h>

#include "debug.h"

#include "methods.h"

ADD2LIBS("datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**************************************************************************************************/

#define HEADER_CHECK_SIZE 8 /* 1 .. 8 */

/**************************************************************************************************/

struct PNGStuff
{
    png_structp png_ptr;
    png_infop   png_info_ptr;
    png_infop   png_end_info_ptr;
    png_uint_32 png_width;
    png_uint_32 png_height;
    int 	png_bits;
    int 	png_type;
    int 	png_lace;
    int 	png_num_lace_passes;
    int 	png_depth;
    int 	png_format;
};

/**************************************************************************************************/

png_voidp my_malloc_fn(png_structp png_ptr, png_size_t size)
{
    png_voidp ret;

    ret = (png_voidp)AllocVec(size, 0);
    if (ret == NULL)
    {
    	if (png_ptr) png_error(png_ptr, "PNG: Out of memory!");
    }

    return ret;
}

/**************************************************************************************************/

void my_free_fn(png_structp png_ptr, png_voidp ptr)
{
    if (ptr) FreeVec(ptr);
}

/**************************************************************************************************/

void my_error_fn(png_structp png_ptr, png_const_charp error_msg)
{
    D(bug("PNG error: %s\n", error_msg ? error_msg : ""));

    if (png_ptr && png_jmpbuf(png_ptr)) longjmp(png_jmpbuf(png_ptr), 1);
}

/**************************************************************************************************/

void my_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
    D(bug("PNG warning: %s\n", warning_msg ? warning_msg : ""));
}

/**************************************************************************************************/

void my_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    BPTR    	file = png_get_io_ptr(png_ptr);
    png_uint_32 count;

    count = Read(file, data, length);
    if (count != length)
    {
    	png_error(png_ptr, "Read error!");
    }
}

void my_write_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    BPTR	file = png_get_io_ptr(png_ptr);
    png_uint_32 count;

    count = Write(file, data, length);
    if (count != length)
    {
    	png_error(png_ptr, "Write error!");
    }
}

void my_flush_fn(png_structp png_ptr)
{
    BPTR	file = png_get_io_ptr(png_ptr);

    if (Flush(file))
    {
    	png_error(png_ptr, "Flush error!");
    }
}

/**************************************************************************************************/

static void PNG_Exit(struct PNGStuff *png, LONG errorcode)
{
    D(if (errorcode) bug("png.datatype/PNG_Exit(): IoErr %ld\n", errorcode));
    SetIoErr(errorcode);
}

/**************************************************************************************************/

static BOOL LoadPNG(struct IClass *cl, Object *o)
{
    struct PNGStuff 	    png;
    struct IFFHandle	    *filehandle;
    struct BitMapHeader     *bmhd;
    UBYTE   	    	    *buffer = NULL;
    IPTR                    sourcetype;
    STRPTR                  name;
    UBYTE   	    	    fileheader[HEADER_CHECK_SIZE];

    D(bug("png.datatype/LoadPNG()\n"));

    if( GetDTAttrs(o,   DTA_SourceType    , (IPTR)&sourcetype ,
			DTA_Handle        , (IPTR)&filehandle,
			PDTA_BitMapHeader , (IPTR)&bmhd,
			TAG_DONE) != 3 )
    {
	PNG_Exit(&png, ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }

    if ( sourcetype == DTST_RAM && filehandle == NULL && bmhd )
    {
	D(bug("png.datatype/LoadPNG(): Creating an empty object\n"));
	PNG_Exit(&png, 0);
	return TRUE;
    }

    if ( sourcetype != DTST_FILE || !filehandle || !bmhd )
    {
	D(bug("png.datatype/LoadPNG(): unsupported mode\n"));
	PNG_Exit(&png, ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }

    if (Read(filehandle, fileheader, sizeof(fileheader)) != sizeof(fileheader))
    {
    	return FALSE;
    }

    if (png_sig_cmp(fileheader, 0, sizeof(fileheader)) != 0)
    {
    	PNG_Exit(&png, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }

    memset(&png, 0, sizeof(png));

    png.png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
    	    	    	    		   0, 	    	    /* error ptr */
					   my_error_fn,     /* error function */
					   my_warning_fn,   /* warning function */
					   0, 	    	    /* mem ptr */
					   my_malloc_fn,    /* malloc function */
					   my_free_fn 	    /* free function */
					   );

    if (!png.png_ptr)
    {
    	D(bug("png.datatype/LoadPNG(): Can't create png read struct!"));
	PNG_Exit(&png, ERROR_NO_FREE_STORE);
	return FALSE;
    }

    png.png_info_ptr = png_create_info_struct(png.png_ptr);
    if (!png.png_info_ptr)
    {
    	D(bug("png.datatype/LoadPNG():Can't create png info struct!\n"));
    	png_destroy_read_struct(&png.png_ptr, NULL, NULL);
	PNG_Exit(&png, ERROR_NO_FREE_STORE);
	return FALSE;

    }

    png.png_end_info_ptr = png_create_info_struct(png.png_ptr);
    if (!png.png_end_info_ptr)
    {
    	D(bug("png.datatype/LoadPNG():Can't create png end info struct!\n"));
    	png_destroy_read_struct(&png.png_ptr, &png.png_info_ptr, NULL);
	PNG_Exit(&png, ERROR_NO_FREE_STORE);
	return FALSE;
    }

    png_set_read_fn(png.png_ptr, filehandle, my_read_fn);

    png_set_sig_bytes(png.png_ptr, HEADER_CHECK_SIZE);

    if (setjmp(png_jmpbuf(png.png_ptr)))
    {
    	D(bug("png.datatype/LoadPNG(): Error!\n"));
    	png_destroy_read_struct(&png.png_ptr, &png.png_info_ptr, &png.png_end_info_ptr);
	if (buffer) FreeVec(buffer);
	PNG_Exit(&png, ERROR_UNKNOWN);
	return FALSE;
    }


    png_read_info(png.png_ptr, png.png_info_ptr);
    png_get_IHDR(png.png_ptr, png.png_info_ptr,
    	    	 &png.png_width, &png.png_height, &png.png_bits,
		 &png.png_type, &png.png_lace, NULL, NULL);

    D(bug("png.datatype/LoadPNG():PNG IHDR: Size %ld x %ld  Bits %d  Type %d Lace %d\n",
    	   png.png_width,
	   png.png_height,
	   png.png_bits,
	   png.png_type,
	   png.png_lace));

    if (png.png_bits == 16)
    {
    	png_set_strip_16(png.png_ptr);
    }
    else if (png.png_bits < 8)
    {
    	png_set_packing(png.png_ptr);
	if (png.png_type == PNG_COLOR_TYPE_GRAY)
	{
	    png_set_gray_1_2_4_to_8(png.png_ptr);
	}
    }

    if (png.png_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
    	png_set_strip_alpha(png.png_ptr);
    }

    {
    	double png_file_gamma;
	double png_screen_gamma = 2.2;

	if (!(png_get_gAMA(png.png_ptr, png.png_info_ptr, &png_file_gamma)))
	{
	    png_file_gamma = 0.45455;
	}

	png_set_gamma(png.png_ptr, png_file_gamma, png_screen_gamma);
    }

    png.png_num_lace_passes = png_set_interlace_handling(png.png_ptr);

    switch(png.png_type)
    {
    	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	    png.png_depth = 8;
	    png.png_format = PBPAFMT_GREY8;
	    break;

	case PNG_COLOR_TYPE_PALETTE:
	    png.png_depth = 8;
	    png.png_format = PBPAFMT_LUT8;
	    break;

	case PNG_COLOR_TYPE_RGB:
	    png.png_depth = 24;
	    png.png_format = PBPAFMT_RGB;
	    break;

	case PNG_COLOR_TYPE_RGB_ALPHA:
	    png.png_depth = 32;
	#if 0
	    png.png_format = PBPAFMT_RGBA;
	#else
	#warning "PBPAFMT_RGBA not supported by picture.datatype, therefore using PBPAFMT_ARGB"
	    png.png_format = PBPAFMT_ARGB;
	    png_set_swap_alpha(png.png_ptr);
	#endif

	    bmhd->bmh_Masking = mskHasAlpha;
	    break;

	default:
	    png_error(png.png_ptr, "Unknown PNG Color Type!");
	    break;
    }

    png_read_update_info(png.png_ptr, png.png_info_ptr);

    bmhd->bmh_Width = png.png_width;
    bmhd->bmh_Height = png.png_height;
    bmhd->bmh_Depth = png.png_depth;

    /* Mask? */
    if (png.png_type == PNG_COLOR_TYPE_PALETTE)
    {
    	png_bytep trans;
	int 	  num_trans;

    	if (png_get_tRNS(png.png_ptr, png.png_info_ptr, &trans, &num_trans, NULL))
	{
	    png_byte best_trans = 255;
	    int      i, best_index = 0;

	    for(i = 0; i < num_trans; i++, trans++)
	    {
	    	if (*trans < best_trans)
		{
		    best_trans = *trans;
		    best_index = i;
		}
	    }

	    if (best_trans < 128) /* 128 = randomly choosen */
	    {
	    	bmhd->bmh_Masking = mskHasTransparentColor;
		bmhd->bmh_Transparent = best_index;
	    }

	} /* if (png_get_tRNS(png.png_ptr, png.png_info_ptr, &trans, &num_trans, NULL)) */

    } /* if (png.png_type == PNG_COLOR_TYPE_PALETTE) */

    /* Palette? */

    if ((png.png_type == PNG_COLOR_TYPE_PALETTE) ||
    	(png.png_type == PNG_COLOR_TYPE_GRAY) ||
	(png.png_type == PNG_COLOR_TYPE_GRAY_ALPHA))
    {
    	struct ColorRegister    *colorregs = 0;
	ULONG	    	    	*cregs = 0;
    	png_colorp  	    	col = 0;
    	int 	    	    	numcolors = 1L << png.png_depth;

	if (png.png_type == PNG_COLOR_TYPE_PALETTE)
	{
   	    if (!png_get_PLTE(png.png_ptr, png.png_info_ptr, &col, &numcolors))
	    {
	    	png_error(png.png_ptr, "PLTE chunk missing!");
	    }
	}

	SetDTAttrs(o, NULL, NULL, PDTA_NumColors, numcolors, TAG_DONE);

    	/* Gray images should in theory not require the following code,
	   as picture.datatype should handle it automatically when
	   we use PBPAFMT_GREY8. But just to be sure ... */

	if (GetDTAttrs(o, PDTA_ColorRegisters, (IPTR) &colorregs,
	    	    	  PDTA_CRegs	     , (IPTR) &cregs	 ,
			  TAG_DONE                                ) == 2)
	{
	    int i;

	    for(i = 0; i < numcolors; i++)
	    {
	    	if (png.png_type == PNG_COLOR_TYPE_PALETTE)
		{
		    colorregs->red   = col->red;
		    colorregs->green = col->green;
		    colorregs->blue  = col->blue;

		    col++;
		}
		else
		{
		    colorregs->red =
		    colorregs->green =
		    colorregs->blue = i;
		}

		*cregs++ = ((ULONG)colorregs->red)   * 0x01010101;
		*cregs++ = ((ULONG)colorregs->green) * 0x01010101;
		*cregs++ = ((ULONG)colorregs->blue)  * 0x01010101;

		colorregs++;

	    } /* for(i = 0; i < numcolors; i++) */

	} /* if (GetDTAttrs(o, ... */

    } /* if image needs palette */

    {
    	ULONG buffersize, modulo, y;
	UBYTE *buf;

	buffersize = png_get_rowbytes(png.png_ptr, png.png_info_ptr);
	if (png.png_lace == PNG_INTERLACE_NONE)
	{
    	    modulo = 0;
	}
	else
	{
    	    modulo = buffersize; buffersize *= png.png_height;
	}

	buffer = AllocVec(buffersize, 0);
    	if (!buffer) png_error(png.png_ptr, "Out of memory!");

	while(png.png_num_lace_passes--)
	{
    	    for(y = 0, buf = buffer; y < png.png_height; y++, buf += modulo)
	    {
		png_read_row(png.png_ptr, buf, NULL);

		if (png.png_num_lace_passes == 0)
		{
		    if(!DoSuperMethod(cl, o,
				      PDTM_WRITEPIXELARRAY, /* Method_ID */
				      (IPTR) buf,	    /* PixelData */
				      png.png_format,	    /* PixelFormat */
				      0,		    /* PixelArrayMod (number of bytes per row) */
				      0,		    /* Left edge */
				      y,	    	    /* Top edge */
				      png.png_width,	    /* Width */
				      1))		    /* Height (here: one line) */
		    {
		    	png_error(png.png_ptr, "Out of memory!");
		    }

		}

	    } /* for(y = 0, buf = buffer; y < png.png_height; y++, buf += modulo) */

	} /* while(png.png_num_lace_passes--) */

	FreeVec(buffer); buffer = NULL;

    } /**/

    png_read_end(png.png_ptr, png.png_end_info_ptr);
    png_destroy_read_struct(&png.png_ptr, &png.png_info_ptr, &png.png_end_info_ptr);

    /* Pass picture size to picture.datatype */
    GetDTAttrs( o, DTA_Name, (IPTR) &name, TAG_DONE );
    SetDTAttrs(o, NULL, NULL, DTA_NominalHoriz,        png.png_width,
			      DTA_NominalVert ,        png.png_height,
			      DTA_ObjName     , (IPTR) name,
			      TAG_DONE);

    D(bug("png.datatype/LoadPNG(): Normal Exit\n"));
    PNG_Exit(&png, 0);

    return TRUE;
}

static BOOL SavePNG(struct IClass *cl, Object *o, struct dtWrite *dtw)
{
    struct PNGStuff         png;
    ULONG                   width, height, numplanes, line;
    struct BitMapHeader     *bmhd;
    BPTR                    filehandle;
    ULONG                   *colorregs;
    UBYTE                   *linebuf = NULL;
    png_colorp              png_palette = NULL;

    D(bug("png.datatype/SavePNG()\n"));

    /* No filehandle is not an error, just NOP */
    if (!dtw->dtw_FileHandle)
    {
	D(bug("png.datatype/SavePNG(): empty filehandle\n"));
	PNG_Exit(&png, 0);
        return TRUE;
    }
    filehandle = dtw->dtw_FileHandle;

    /* Obtain most important information about the BitMap */
    if (GetDTAttrs(o,   PDTA_BitMapHeader,    (IPTR)&bmhd,
                        PDTA_CRegs,         (IPTR)&colorregs,
                        TAG_DONE) != 2UL ||
        !bmhd || !colorregs)
    {
        D(bug("png.datatype/SavePNG(): missing attributes\n"));
        PNG_Exit(&png, ERROR_OBJECT_WRONG_TYPE);
        return FALSE;
    }

    width = bmhd->bmh_Width;
    height = bmhd->bmh_Height;
    numplanes = bmhd->bmh_Depth;

    D(bug("png.datatype/SavePNG(): Picture size %d x %d (x %d bit)\n", width, height, numplanes));

    /* Setup. Clear PNGStuff structure. */
    memset(&png, 0, sizeof(png));

    /* PNG Write structure with custom stdio functions */
    png.png_ptr = png_create_write_struct_2(PNG_LIBPNG_VER_STRING,
                                           0,               /* error ptr */
                                           my_error_fn,     /* error function */
                                           my_warning_fn,   /* warning function */
                                           0,               /* mem ptr */
                                           my_malloc_fn,    /* malloc function */
                                           my_free_fn       /* free function */
                                           );

    if (!png.png_ptr)
    {
        D(bug("png.datatype/SavePNG(): Can't create png read struct!"));
        PNG_Exit(&png, ERROR_NO_FREE_STORE);
        return FALSE;
    }

    /* The procedure will enter this piece of code in case of any error */
    if (setjmp(png_jmpbuf(png.png_ptr)))
    {
        D(bug("png.datatype/SavePNG(): Error!\n"));
        png_destroy_write_struct(&png.png_ptr, &png.png_info_ptr);
        if (linebuf) FreeVec(linebuf);
        if (png_palette) FreeVec(png_palette);
        PNG_Exit(&png, ERROR_UNKNOWN);
        return FALSE;
    }

    /* Replace default write functions with our own */
    png_set_write_fn(png.png_ptr, filehandle, my_write_fn, my_flush_fn);

    png.png_info_ptr = png_create_info_struct(png.png_ptr);
    if (!png.png_info_ptr)
    {
        D(bug("png.datatype/SavePNG(): Can't create png info struct!\n"));
        png_destroy_write_struct(&png.png_ptr, NULL);
        PNG_Exit(&png, ERROR_NO_FREE_STORE);
        return FALSE;
    }

    /* Now the png_info structure. Depending on the BitMap depth (either LUT or Truecolor),
       the proper type must be set. */
    if (numplanes > 8)
    {
        png.png_depth = 24;
        png.png_format = PBPAFMT_RGB;
        png.png_type = PNG_COLOR_TYPE_RGB;

        png_set_IHDR(png.png_ptr, png.png_info_ptr, width, height, 8, png.png_type,
                    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    }
    else
    {
        int i;
        png.png_depth = 8;
        png.png_format = PBPAFMT_LUT8;
        png.png_type = PNG_COLOR_TYPE_PALETTE;

        png_set_IHDR(png.png_ptr, png.png_info_ptr, width, height, 8, png.png_type,
                    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        /* LUT images *MUST* have palette set */
        png_palette = AllocVec((1 << numplanes) * sizeof(png_color), MEMF_ANY);

        for (i=0; i < (1 << numplanes); i++) {
            png_palette[i].red = (*colorregs++) >> 24;
            png_palette[i].green = (*colorregs++) >> 24;
            png_palette[i].blue = (*colorregs++) >> 24;
        }

        png_set_PLTE(png.png_ptr, png.png_info_ptr, png_palette, (1 << numplanes));
    }

    /* Write info structure out */
    png_write_info(png.png_ptr, png.png_info_ptr);

    /* Read the picture data line by line and write PNG */
    if (!(linebuf = AllocVec(width * 3, MEMF_ANY)))
    {
        D(bug("png.datatype/SavePNG(): No free store\n"));
        PNG_Exit(&png, ERROR_NO_FREE_STORE);
        return FALSE;
    }

    for (line = 0; line < height; line++)
    {
        /* Get single line of image */
        if (!DoSuperMethod(cl, o,
                PDTM_READPIXELARRAY,
                (IPTR)linebuf,
                png.png_format,
                width,
                0,
                line,
                width,
                1))
        {
            D(bug("png.datatype/SavePNG(): READPIXELARRAY failed!\n"));
            FreeVec(linebuf);
            PNG_Exit(&png, ERROR_UNKNOWN);
            return FALSE;
        }
        /* png_write_line */
        png_write_row(png.png_ptr, linebuf);
    }

    /* cleanup */
    png_write_end(png.png_ptr, png.png_info_ptr);

    FreeVec(linebuf);
    if (png_palette) FreeVec(png_palette);

    png_destroy_write_struct(&png.png_ptr, &png.png_info_ptr);

    D(bug("png.datatype/SavePNG() --- Normal Exit\n"));

    return TRUE;
}

/**************************************************************************************************/

IPTR PNG__OM_NEW(struct IClass *cl, Object *o, Msg msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval != (IPTR)0)
    {
	if (!LoadPNG(cl, (Object *)retval))
	{
	    CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
	    return (IPTR)0;
	}
    }
    return retval;
}

/**************************************************************************************************/

IPTR PNG__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    D(bug("png.datatype/DT_Dispatcher: Method DTM_WRITE\n"));
    if( (dtw -> dtw_Mode) == DTWM_RAW )
    {
        /* Local data format requested */
        return SavePNG(cl, o, dtw );
    }
    else
    {
        /* Pass msg to superclass (which writes an IFF ILBM picture)... */
        return DoSuperMethodA( cl, o, (Msg)dtw );
    }
}

/**************************************************************************************************/
