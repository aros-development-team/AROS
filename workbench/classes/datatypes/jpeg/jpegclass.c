/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/**********************************************************************/

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

#include <jinclude.h>
#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>

#include "compilerspecific.h"
#include "debug.h"

#include "methods.h"

/**************************************************************************************************/

#define QUALITY 90	/* compress quality for saving */

typedef	struct {
    struct IFFHandle	*filehandle;
    UBYTE		*linebuf;
} JpegHandleType;

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */
  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  D(bug("jpeg.datatype/libjpeg: Fatal Error !\n"));
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

METHODDEF(void)
my_output_message (j_common_ptr cinfo)
{
  D(
  char buffer[JMSG_LENGTH_MAX];

  /* Create the message */
  (*cinfo->err->format_message) (cinfo, buffer);

  /* Display debug output, adding a newline */
  bug("jpeg.datatype/libjpeg: %s\n", buffer)
  );
}

typedef struct {
  struct jpeg_source_mgr pub;	/* public fields */

  FILE * infile;		/* source stream */
  JOCTET * buffer;		/* start of buffer */
  boolean start_of_file;	/* have we gotten any data yet? */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

#define INPUT_BUF_SIZE  4096	/* choose an efficiently fread'able size */

METHODDEF(boolean)
my_fill_input_buffer (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;
  size_t nbytes;

//  D(bug("jpeg.datatype/my_fill_input_buffer\n"));
  nbytes = Read(src->infile, src->buffer, INPUT_BUF_SIZE);

  if (nbytes <= 0) {
    if (src->start_of_file)	/* Treat empty input file as fatal error */
      ERREXIT(cinfo, JERR_INPUT_EMPTY);
    WARNMS(cinfo, JWRN_JPEG_EOF);
    /* Insert a fake EOI marker */
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    nbytes = 2;
  }

  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->start_of_file = FALSE;

  return TRUE;
}

/* Dummy function for the linker */
void exit(int bla)
{
  D(bug("jpeg.datatype/exit\n"));
}

/**************************************************************************************************/

static void JPEG_Exit(JpegHandleType *jpeghandle, LONG errorcode)
{
    D(if (errorcode) bug("jpeg.datatype/JPEG_Exit(): IoErr %ld\n", errorcode));
    if( jpeghandle->linebuf )
	FreeVec( jpeghandle->linebuf );
    FreeMem(jpeghandle, sizeof(JpegHandleType));
    SetIoErr(errorcode);
}

/**************************************************************************************************/

static BOOL LoadJPEG(struct IClass *cl, Object *o)
{
    JpegHandleType          *jpeghandle;
    struct IFFHandle	    *filehandle;
    long		    width, height;
    IPTR                    sourcetype;
    struct BitMapHeader     *bmhd;
    STRPTR                  name;

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPARRAY buffer;		/* Output row buffer */
    int row_stride;		/* physical row width in output buffer */
    my_src_ptr src;

    D(bug("jpeg.datatype/LoadJPEG()\n"));

    if( !(jpeghandle = AllocMem(sizeof(JpegHandleType), MEMF_ANY)) )
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    jpeghandle->linebuf = NULL;
    if( GetDTAttrs(o,   DTA_SourceType    , (IPTR)&sourcetype ,
			DTA_Handle        , (IPTR)&filehandle,
			PDTA_BitMapHeader , (IPTR)&bmhd,
			TAG_DONE) != 3 )
    {
	JPEG_Exit(jpeghandle, ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
    
    if ( sourcetype == DTST_RAM && filehandle == NULL && bmhd )
    {
	D(bug("jpeg.datatype/LoadJPEG(): Creating an empty object\n"));
	JPEG_Exit(jpeghandle, ERROR_NOT_IMPLEMENTED);
	return TRUE;
    }
    if ( sourcetype != DTST_FILE || !filehandle || !bmhd )
    {
	D(bug("jpeg.datatype/LoadJPEG(): unsupported mode\n"));
	JPEG_Exit(jpeghandle, ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }

    D(bug("jpeg.datatype/LoadJPEG(): Setting error handler\n"));
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    jerr.pub.output_message = my_output_message;
    if (setjmp(jerr.setjmp_buffer)) {
	/* If we get here, the JPEG code has signaled an error.
	 * We need to clean up the JPEG object, close the input file, and return.
	 */
	jpeg_destroy_decompress(&cinfo);
	JPEG_Exit(jpeghandle, 1);
	return FALSE;
    }

    D(bug("jpeg.datatype/LoadJPEG(): Create decompressor\n"));
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, (FILE *)filehandle);
    src = (my_src_ptr) cinfo.src;
    src->pub.fill_input_buffer = my_fill_input_buffer;
    
    D(bug("jpeg.datatype/LoadJPEG(): Read Header\n"));
    (void) jpeg_read_header(&cinfo, TRUE);
    D(bug("jpeg.datatype/LoadJPEG(): Starting decompression\n"));
    (void) jpeg_start_decompress(&cinfo);
    /* set BitMapHeader with image size */
    bmhd->bmh_Width  = bmhd->bmh_PageWidth  = width = cinfo.output_width;
    bmhd->bmh_Height = bmhd->bmh_PageHeight = height = cinfo.output_height;
    bmhd->bmh_Depth  = 24;
    D(bug("jpeg.datatype/LoadJPEG(): Size %ld x %ld x %d bit\n", width, height, (int)(cinfo.output_components*8)));
    if (cinfo.output_components != 3)
    {
	D(bug("jpeg.datatype/LoadJPEG(): unsupported colormode\n"));
	JPEG_Exit(jpeghandle, ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }

    /* Make a one-row-high sample array that will go away when done with image */
    row_stride = width * 3;
    buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    
    /* Here we use the library's state variable cinfo.output_scanline as the
    * loop counter, so that we don't have to keep track ourselves.
    */
    while (cinfo.output_scanline < height)
    {
	/* jpeg_read_scanlines expects an array of pointers to scanlines.
	 * Here the array is only one element long, but you could ask for
	 * more than one scanline at a time if that's more convenient.
	 */
	(void) jpeg_read_scanlines(&cinfo, buffer, 1);
	// D(bug("jpeg.datatype/LoadJPEG(): Copy line %ld\n", (long)cinfo.output_scanline));
	if(!DoSuperMethod(cl, o,
			PDTM_WRITEPIXELARRAY,		// Method_ID
			(IPTR) buffer[0],		// PixelData
			PBPAFMT_RGB,			// PixelFormat
			row_stride,			// PixelArrayMod (number of bytes per row)
			0,				// Left edge
			cinfo.output_scanline-1,	// Top edge
			width,				// Width
			1))				// Height (here: one line)
	{
	    D(bug("jpeg.datatype/LoadJPEG(): WRITEPIXELARRAY failed\n"));
	    JPEG_Exit(jpeghandle, ERROR_OBJECT_NOT_FOUND);
	    return FALSE;
	} 
    }
    D(bug("jpeg.datatype/LoadJPEG(): WRITEPIXELARRAY of whole picture done\n"));
    
    D(bug("jpeg.datatype/LoadJPEG(): Clean up\n"));
    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    /* Pass picture size to picture.datatype */
    GetDTAttrs( o, DTA_Name, (IPTR)&name, TAG_DONE );
    SetDTAttrs(o, NULL, NULL, DTA_NominalHoriz, width,
			      DTA_NominalVert , height,
			      DTA_ObjName     , (IPTR)name,
			      TAG_DONE);

    D(bug("jpeg.datatype/LoadJPEG(): Normal Exit\n"));
    JPEG_Exit(jpeghandle, 0);
    return TRUE;
}

/**************************************************************************************************/

typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  FILE * outfile;		/* target stream */
  JOCTET * buffer;		/* start of buffer */
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

METHODDEF(boolean)
my_empty_output_buffer (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  // D(bug("jpeg.datatype/my_empty_output_buffer\n"));
  if (Write(dest->outfile, dest->buffer, OUTPUT_BUF_SIZE) !=
      (size_t) OUTPUT_BUF_SIZE)
    ERREXIT(cinfo, JERR_FILE_WRITE);

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

  return TRUE;
}

METHODDEF(void)
my_term_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

  /* Write any data remaining in the buffer */
  if (datacount > 0) {
    if (Write(dest->outfile, dest->buffer, datacount) != datacount)
      ERREXIT(cinfo, JERR_FILE_WRITE);
  }
}

/**************************************************************************************************/

static BOOL SaveJPEG(struct IClass *cl, Object *o, struct dtWrite *dtw )
{
    JpegHandleType          *jpeghandle;
    BPTR		    filehandle;
    unsigned int            width, height, numplanes;
    UBYTE		    *linebuf;
    struct BitMapHeader     *bmhd;
    long                    *colorregs;

    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
    int row_stride;		/* physical row width in image buffer */
    my_dest_ptr dest;

    D(bug("jpeg.datatype/SaveJPEG()\n"));

    /* A NULL file handle is a NOP */
    if( !dtw->dtw_FileHandle )
    {
	D(bug("jpeg.datatype/SaveJPEG(): empty Filehandle - just testing\n"));
	return TRUE;
    }
    filehandle = dtw->dtw_FileHandle;

    /* Get BitMapHeader and color palette */
    if( GetDTAttrs( o,  PDTA_BitMapHeader, (IPTR) &bmhd,
			PDTA_CRegs,        (IPTR) &colorregs,
			TAG_DONE ) != 2UL ||
	!bmhd || !colorregs )
    {
	D(bug("jpeg.datatype/SaveJPEG(): missing attributes\n"));
	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }

    width = bmhd->bmh_Width;
    height = bmhd->bmh_Height;
    numplanes = bmhd->bmh_Depth;
    if( numplanes != 24 )
    {
	D(bug("jpeg.datatype/SaveJPEG(): color depth %d, can save only depths of 24\n", numplanes));
	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    D(bug("jpeg.datatype/SaveJPEG(): Picture size %d x %d (x %d bit)\n", width, height, numplanes));

    if( !(jpeghandle = AllocMem(sizeof(JpegHandleType), MEMF_ANY)) )
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    jpeghandle->linebuf = NULL;

    D(bug("jpeg.datatype/SaveJPEG(): Setting error handler\n"));
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    jerr.pub.output_message = my_output_message;
    if (setjmp(jerr.setjmp_buffer)) {
	/* If we get here, the JPEG code has signaled an error.
	 * We need to clean up the JPEG object, close the input file, and return.
	 */
	jpeg_destroy_compress(&cinfo);
	JPEG_Exit(jpeghandle, 1);
	return FALSE;
    }

    D(bug("jpeg.datatype/SaveJPEG(): Create compressor\n"));
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, (FILE *)filehandle);
    dest = (my_dest_ptr) cinfo.dest;
    dest->pub.empty_output_buffer = my_empty_output_buffer;
    dest->pub.term_destination = my_term_destination;

    cinfo.image_width = width;	 	/* image width and height, in pixels */
    cinfo.image_height = height;
    cinfo.input_components = 3;		/* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, QUALITY, TRUE /* limit to baseline-JPEG values */);
    D(bug("jpeg.datatype/SaveJPEG(): Starting compression\n"));
    jpeg_start_compress(&cinfo, TRUE);

    /* Now read the picture data line by line and write it to a chunky buffer */
    if( !(linebuf = AllocVec(width*3, MEMF_ANY)) )
    {
	JPEG_Exit(jpeghandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
    jpeghandle->linebuf = linebuf;

    row_stride = width * 3;	/* JSAMPLEs per row in image_buffer */
    row_pointer[0] = linebuf;
    while (cinfo.next_scanline < cinfo.image_height)
    {
	// D(bug("jpeg.datatype/SaveJPEG(): READPIXELARRAY line %ld\n", (long)cinfo.next_scanline));
	if(!DoSuperMethod(cl, o,
			PDTM_READPIXELARRAY,	// Method_ID
			(IPTR)linebuf,		// PixelData
			PBPAFMT_RGB,		// PixelFormat
			width,			// PixelArrayMod (number of bytes per row)
			0,			// Left edge
			cinfo.next_scanline,	// Top edge
			width,			// Width
			1))			// Height
	{
	    D(bug("jpeg.datatype/SaveJPEG(): READPIXELARRAY failed!\n"));
	    JPEG_Exit(jpeghandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
	}
	(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    D(bug("jpeg.datatype/SaveJPEG() --- Normal Exit\n"));
    JPEG_Exit(jpeghandle, 0);
    return TRUE;
}

/**************************************************************************************************/

#ifdef __AROS__
AROS_UFH3S(IPTR, DT_Dispatcher,
       AROS_UFHA(Class *, cl, A0),
       AROS_UFHA(Object *, o, A2),
       AROS_UFHA(Msg, msg, A1))
#else
ASM IPTR DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 Msg msg)
#endif
{
#ifdef __AROS__
    AROS_USERFUNC_INIT
#endif

    IPTR retval;
    struct dtWrite *dtw;

    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);        /* Small Data */

//    D(bug("jpeg.datatype/DT_Dispatcher: Entering\n"));

    switch(msg->MethodID)
    {
	case OM_NEW:
	    D(bug("jpeg.datatype/DT_Dispatcher: Method OM_NEW\n"));
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    if (retval)
	    {
		if (!LoadJPEG(cl, (Object *)retval))
		{
		    CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
		    retval = 0;
		}
	    }
	    break;

	case DTM_WRITE:
	    D(bug("jpeg.datatype/DT_Dispatcher: Method DTM_WRITE\n"));
	    dtw = (struct dtWrite *)msg;
	    if( (dtw -> dtw_Mode) == DTWM_RAW )
	    {
		/* Local data format requested */
		retval = SaveJPEG(cl, o, dtw );
	    }
	    else
	    {
		/* Pass msg to superclass (which writes an IFF ILBM picture)... */
		retval = DoSuperMethodA( cl, o, msg );
	    }
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    
    } /* switch(msg->MethodID) */

//    D(bug("jpeg.datatype/DT_Dispatcher: Leaving\n"));

    return retval;
    
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *gifbase)
{
    struct IClass *cl;
    
    cl = MakeClass("jpeg.datatype", "picture.datatype", 0, 0, 0);

    D(bug("jpeg.datatype/DT_MakeClass: DT_Dispatcher 0x%lx\n", (unsigned long) DT_Dispatcher));

    if (cl)
    {
#ifdef __AROS__
    cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
    cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
    cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
    cl->cl_UserData = (IPTR)gifbase; /* Required by datatypes (see disposedtobject) */
    }

    return cl;
}

/**************************************************************************************************/

