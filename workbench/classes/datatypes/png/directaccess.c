/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/png.h>
#include <datatypes/pictureclass.h>

#include <png.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

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

struct PNGHandle
{
    struct PNGStuff stuff;
    APTR    	    data;
    APTR    	    pal;
};

/***************************************************************************************************/

png_voidp my_malloc_fn(png_structp png_ptr, png_size_t size);
void my_free_fn(png_structp png_ptr, png_voidp ptr);
void my_error_fn(png_structp png_ptr, png_const_charp error_msg);
void my_warning_fn(png_structp png_ptr, png_const_charp warning_msg);
void my_read_fn(png_structp png_ptr, png_bytep data, png_size_t length);

/***************************************************************************************************/

AROS_LH1(LONG, PNG_CheckSig,
    AROS_LHA(STRPTR, name, A0),
    struct Library *, PNGBase, 6, PNG)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    BPTR fh;
    LONG ret = -1;
    
    if ((fh = Open(name, MODE_OLDFILE)))
    {
    	png_byte header[8];
	
	if (Read(fh, header, sizeof(header)) == sizeof(header))
	{
	    if (png_sig_cmp(header, 0, sizeof(header)) == 0)
	    {
	    	ret = 8;
	    }
	    else
	    {
	    	ret = 0;
	    }
    	}
	
	Close(fh);
	
    }
    
    return ret;
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/

AROS_LH4(APTR, PNG_LoadImageFH,
    AROS_LHA(BPTR, fh, A0),
    AROS_LHA(STRPTR *, chunkstoread, A1),
    AROS_LHA(APTR *,  chunkstore, A2),
    AROS_LHA(BOOL, makeARGB, D0),
    struct Library *, PNGBase, 7, PNG)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)
    
    struct PNGStuff   png;
    struct PNGHandle *pnghandle = NULL;
    APTR    	      buffer = NULL;
    UBYTE   	      header[8];
    APTR    	      retval = 0;
    BOOL    	      ok = TRUE;
    
    if (!fh) return NULL;
    
    if (Read(fh, header, sizeof(header)) != sizeof(header)) ok = FALSE;

    memset(&png, 0, sizeof(png));
    
    if (ok)
    {
    	if (png_sig_cmp(header, 0, sizeof(header)) != 0) ok = FALSE;
    }
    
    if (ok)
    {
    	memset(&png, 0, sizeof(png));
	    
	png.png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
    	    	    	    		       0, 	    	/* error ptr */
					       my_error_fn,     /* error function */
					       my_warning_fn,   /* warning function */
					       0, 	    	/* mem ptr */
					       my_malloc_fn,    /* malloc function */
					       my_free_fn 	/* free function */
					       );
	    
	if (!png.png_ptr) ok = FALSE;
    }
    
    if (ok)
    {
    	png.png_info_ptr = png_create_info_struct(png.png_ptr);
	if (!png.png_info_ptr) ok = FALSE;
    }
    
    
    if (ok)
    {
    	png.png_end_info_ptr = png_create_info_struct(png.png_ptr);
    	if (!png.png_end_info_ptr) ok = FALSE;
    }
    
    if (ok)
    {
    	png_set_read_fn(png.png_ptr, fh, my_read_fn);
    	png_set_sig_bytes(png.png_ptr, sizeof(header));
	
    }
    
    if (ok)
    {
	if (setjmp(png_jmpbuf(png.png_ptr)))
	{
    	    /* Error */

	    ok = FALSE;
	}
    }
    
    if (ok)
    {
	if (chunkstoread)
	{
	    int i;

    	    /* CHECKME:
	    
	       Do this first, because it doesn't work otherwise. Maybe
	       a bug in libpng. The problem is that libpng/pngrutil.c
	       in png_handle_unknown() checks PNG_FLAG_KEEP_UNKNOWN_CHUNKS
	       flag. But this flag is set only when num_chunks param passed
	       to png_set_keep_unknown_chunks() is 0.
	       
	    */
	       
	    png_set_keep_unknown_chunks(png.png_ptr, 3, 0, 0);
	    
	    for(i = 0; chunkstoread[i]; i++)
	    {
kprintf("** Calling png_set_keep_unknown chunks(%s)\n", chunkstoread[i]);
	    	png_set_keep_unknown_chunks(png.png_ptr, 3, chunkstoread[i], 1);	    	
	    }
	    	    	  				    
	}

	png_read_info(png.png_ptr, png.png_info_ptr);

	png_get_IHDR(png.png_ptr, png.png_info_ptr,
    	    	     &png.png_width, &png.png_height, &png.png_bits,
		     &png.png_type, &png.png_lace, NULL, NULL);


	if (png.png_bits == 16)
	{
    	    png_set_strip_16(png.png_ptr);
	    png.png_bits = 8;
	}
	else if (png.png_bits < 8)
	{
    	    png_set_packing(png.png_ptr);
	    if (png.png_type == PNG_COLOR_TYPE_GRAY)
	    {
		png_set_gray_1_2_4_to_8(png.png_ptr);
	    }
	    png.png_bits = 8;
	}

	if (png.png_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
    	    png_set_strip_alpha(png.png_ptr);
	    png.png_type = PNG_COLOR_TYPE_GRAY;
	}

	if (makeARGB)
	{
    	    if (png.png_type == PNG_COLOR_TYPE_RGB)
	    {
		png_set_filler(png.png_ptr, 0xFF, PNG_FILLER_BEFORE);	    
	    }
	    else if (png.png_type == PNG_COLOR_TYPE_GRAY)
	    {
		png_set_gray_to_rgb(png.png_ptr);
		png_set_filler(png.png_ptr, 0xFF, PNG_FILLER_BEFORE);
	    }
	    else if (png.png_type == PNG_COLOR_TYPE_PALETTE)
	    {
		png_set_palette_to_rgb(png.png_ptr);
		if (png_get_valid(png.png_ptr, png.png_info_ptr, PNG_INFO_tRNS))
		{
	    	    png_set_tRNS_to_alpha(png.png_ptr);
		}
		else
		{
	    	    png_set_filler(png.png_ptr, 0xFF, PNG_FILLER_BEFORE);
		}
	    }

	    png.png_type = PNG_COLOR_TYPE_RGB_ALPHA;
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
		break;

	    default:
		png_error(png.png_ptr, "Unknown PNG Color Type!");
		break;
	}

	png_read_update_info(png.png_ptr, png.png_info_ptr);
	
	{
    	    ULONG buffersize, modulo, y, bpr, bpp;
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

    	    bpp = png.png_depth / 8;
    	    bpr = bpp * png.png_width;
	    buffersize = bpr * png.png_height;
	    
	    pnghandle = AllocVec(sizeof(*pnghandle) + buffersize, MEMF_ANY);
	    if (!pnghandle) png_error(png.png_ptr, "Out of memory!");
	    
	    pnghandle->data = ((UBYTE *)pnghandle) + sizeof(*pnghandle);
	    
	    while(png.png_num_lace_passes--)
	    {
    		for(y = 0, buf = buffer; y < png.png_height; y++, buf += modulo)
		{
		    png_read_row(png.png_ptr, buf, NULL);

		    if (png.png_num_lace_passes == 0)
		    {
		    	memcpy(pnghandle->data + y * bpr, buf, bpr);			
		    }

		} /* for(y = 0, buf = buffer; y < png.png_height; y++, buf += modulo) */

	    } /* while(png.png_num_lace_passes--) */

    	    png_read_end(png.png_ptr, png.png_end_info_ptr);

    	    if (chunkstore && chunkstoread)
	    {
	    	png_unknown_chunkp entries;
		png_infop info;
		
		int infoloop;
		
		info = png.png_info_ptr;
		
		for(infoloop = 0; infoloop < 2; infoloop++)
		{
		    int numchunks = png_get_unknown_chunks(png.png_ptr, info, &entries);
kprintf("png_get_unknown_chunks(%x) %d\n", info, numchunks);

		    while(numchunks--)
		    {
		    	int i;
			
			for(i = 0; chunkstoread[i]; i++)
			{
			    if (chunkstore[i]) continue;
			    
		    	    if (memcmp(entries->name, chunkstoread[i], 4) == 0)
			    {
			    	png_unknown_chunkp p;
				
    	    	    	    	chunkstore[i] = AllocVec(sizeof(*p) + entries->size, MEMF_ANY);
				if (!chunkstore[i]) png_error(png.png_ptr, "Out of memory!");
				
				p = (png_unknown_chunkp)chunkstore[i];
				
				*p = *entries;
				p->data = (png_byte *)(p + 1);
				memcpy(p->data, entries->data, entries->size);
				break;
			    }

			} /* for(i = 0; chunkstoread[i]; i++) */

			entries++;
			
		    } /* while(numchunks--) */
		    
		    info = png.png_end_info_ptr;
		    
		} /* for(infoloop = 0, infoloop < 2; infoloop++) */
		
	    } /* if (chunkstore && chunkstoread) */

    	    pnghandle->pal = 0;
	    pnghandle->stuff = png;
	    	    
	    /* We're done :-) */
	    
    	    retval = pnghandle;
	    
	} /**/
	
    } /* if (ok) */
    
    if (png.png_ptr)
    {
    	if (buffer) FreeVec(buffer);
    	png_destroy_read_struct(&png.png_ptr, &png.png_info_ptr, &png.png_end_info_ptr);
    }
        
    if (!ok)
    {
    	if (pnghandle) FreeVec(pnghandle);
    }
	    
    return retval;
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/

AROS_LH4(APTR, PNG_LoadImage,
    AROS_LHA(STRPTR, name, A0),
    AROS_LHA(STRPTR *, chunkstoread, A1),
    AROS_LHA(APTR *,  chunkstore, A2),
    AROS_LHA(BOOL, makeARGB, D0),
    struct Library *, PNGBase, 8, PNG)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)
    
    BPTR    	      fh;
    APTR    	      retval = 0;
    
    if ((fh = Open(name, MODE_OLDFILE)))
    {
    	retval = PNG_LoadImageFH(fh, chunkstoread, chunkstore, makeARGB);
	Close(fh);
    }
   
    return retval;
    
    AROS_LIBFUNC_EXIT
}
    
/***************************************************************************************************/

AROS_LH5(void, PNG_GetImageInfo,
    AROS_LHA(APTR, pnghandle, A0),
    AROS_LHA(LONG *, widthptr, A1),
    AROS_LHA(LONG *, heightptr, A2),
    AROS_LHA(LONG *, depthptr, A3),
    AROS_LHA(LONG *, typeptr, A4),
    struct Library *, PNGBase, 9, PNG)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    struct PNGHandle *h = (struct PNGHandle *)pnghandle;
    
    if (!h) return;
    
    if (widthptr)  *widthptr  = h->stuff.png_width;
    if (heightptr) *heightptr = h->stuff.png_height;
    if (depthptr)  *depthptr  = h->stuff.png_depth;
    if (typeptr)   *typeptr   = h->stuff.png_type;
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/

AROS_LH3(void, PNG_GetImageData,
    AROS_LHA(APTR, pnghandle, A0),
    AROS_LHA(APTR *, gfxdataptr, A1),
    AROS_LHA(APTR *, paldataptr, A2),
    struct Library *, PNGBase, 10, PNG)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    struct PNGHandle *h = (struct PNGHandle *)pnghandle;
    
    if (!h) return;
    
    if (gfxdataptr) *gfxdataptr = h->data;
    if (paldataptr) *paldataptr = h->pal;
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/

AROS_LH1(void, PNG_FreeImage,
    AROS_LHA(APTR, pnghandle, A0),
    struct Library *, PNGBase, 11, PNG)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    if (pnghandle) FreeVec(pnghandle);
    
    AROS_LIBFUNC_EXIT
}
     
/***************************************************************************************************/

AROS_LH1(void, PNG_FreeChunk,
    AROS_LHA(APTR, chunk, A0),
    struct Library *, PNGBase, 12, PNG)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    if (chunk) FreeVec(chunk);
    
    AROS_LIBFUNC_EXIT
}
 
/***************************************************************************************************/

AROS_LH3(void, PNG_GetChunkInfo,
    AROS_LHA(APTR, chunk, A0),
    AROS_LHA(APTR *, dataptr, A1),
    AROS_LHA(ULONG *, sizeptr, A2),
    struct Library *, PNGBase, 13, PNG)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    png_unknown_chunkp p = (png_unknown_chunkp)chunk;
    if (dataptr) *dataptr = p->data;
    if (sizeptr) *sizeptr = p->size;
    
    AROS_LIBFUNC_EXIT
}
 
/***************************************************************************************************/
