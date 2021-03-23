/*
	Copyright © 1995-2021, The AROS Development Team. All rights reserved.
	$Id$ TIFF Class by MikeR. 03-17-2021
*/

/**********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <intuition/intuition.h>

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

#include "debug.h"
#include "methods.h"

//#include <tiff.h>
#include <tiffio.h>

#define TIFFGetR(abgr) ((abgr) & 0xff)
#define TIFFGetG(abgr) (((abgr) >> 8) & 0xff)
#define TIFFGetB(abgr) (((abgr) >> 16) & 0xff)
#define TIFFGetA(abgr) (((abgr) >> 24) & 0xff)

/* Open superclass */
ADD2LIBS("datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**********************************************************************/

void WriteFile(char *outFileName, char *data, LONG offset, LONG fileSize)
{
	FILE *file;
	long w1 = 0;
	
	file = fopen(outFileName, "wb");
	if (!file)
	{
		//printf("Unable to open file %s", outFileName);		
	}
	else
	{       
           fseek(file, offset, SEEK_SET);	   
	    w1 = fwrite(data , 1, fileSize, file);
	    //printf("\nDone writing file: %s, Length: %d\n\n", outFileName,  fileSize);
        }    
	fclose(file);
}

/**************************************************************************************************/

static BOOL Load_TIFF(struct IClass *cl, Object *o)
{
	BOOL err;
	char *Title;
        ULONG row;
        ULONG w, h;
	ULONG* Raster;
	IPTR sourcetype;
	BPTR FileHandle;	
	BOOL retval = FALSE;
	LONG pconfig, pmetric;
        STRPTR namebuffer = NULL;        
	struct BitMapHeader *bmhd;        
	

	D(bug("tiff.datatype/LoadTIFF(): Starting. (24 bit mode)\n"));	

	
	 if( GetDTAttrs(o,   DTA_SourceType    , (IPTR)&sourcetype ,
			DTA_Handle        , (IPTR)&FileHandle,	
			DTA_Name          , (IPTR)&Title,
			PDTA_BitMapHeader , (IPTR)&bmhd,
			TAG_DONE) != 4 )
	{
		D(bug("tiff.datatype/LoadTIFF(): GetDTAttrs(DTA_Handle, DTA_BitMapHeader) error !\n"));
		SetIoErr(ERROR_OBJECT_NOT_FOUND);
		return FALSE;
	}
	D(bug("tiff.datatype/LoadTIFF(): GetDTAttrs(DTA_Handle, DTA_BitMapHeader) successful\n"));
	
	if ( sourcetype == DTST_RAM && FileHandle == BNULL )
	{
		D(bug("tiff.datatype/LoadTIFF(): Creating an empty object\n"));
		return TRUE;
	}
	if ( sourcetype != DTST_FILE || !FileHandle || !bmhd )
	{
		D(bug("tiff.datatype/LoadTIFF(): Unsupported sourcetype mode\n"));
		SetIoErr(ERROR_NOT_IMPLEMENTED);
		return FALSE;
	}
	
	
	// Check file Signature. Read file header. See "isTIF" function.
	//Seek(FileHandle, 0, OFFSET_BEGINNING);
	D(bug("tiff.datatype/LoadTIFF(): Seek was successful\n"));
    
	//Read TIFF Header: 'I I' or 'M M' (Intel or Motorola Byte Order)
	//Read TIFF Version: 42
	//Read TIFF FirstOffset: (LONG Offset to First Image IFD Entry)
	
    
	//*  Convert filehandle *//
	D(bug("\ntiff.datatype/LoadTIFF(): Attempting to Get File Name from FileHandle.\n"));
	
	// NameFromLock(BPTR lock, STRPTR buffer, LONG length )
	namebuffer = malloc(256 * sizeof(char));
	err= NameFromFH(FileHandle, namebuffer, 256);
	if (err != 0)
	{
		D(bug("\ntiff.datatype/LoadTIFF(): File Name: %s\n", namebuffer));                  
	}
	else
	{
		D(bug("\ntiff.datatype/LoadTIFF(): Failed to get File Name.\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
	
    
	//* Open Tiff file to get new filehandle *//
	TIFF* tif = TIFFOpen(namebuffer,"r");
	if (tif)
	{
		D(bug("tiff.datatype/LoadTIFF(): Successfully Opened Tiff File.\n"));
		
		int index = 1;
		int dircount = 0;
		int numpictures = 0;
		do {
			dircount++;
		} while (TIFFReadDirectory(tif));
		
		//index = whichpicture;
		//numpictures = dircount;

		//LoadTIFF_DT(image, 0); //Filename, SetDirNum.
		
		
		D(bug("\ntiff.datatype/LoadTIFF(): Found %d directories in %s\n", dircount, namebuffer));
		
		if(index > 0 && index <= dircount-1)
			TIFFSetDirectory(tif, index); //Set Index Directory as Current.
		else
			TIFFSetDirectory(tif, 0); //Set First Directory as Current.
		
        
		//#define uint32 unsigned long - ULONG
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		//TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &pconfig);
		TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &pmetric);
  	    
		D(bug("\ntiff.datatype/LoadTIFF(): Picture Data: Width %d x Height %d\n", w, h));		
		//D(bug("tiff.datatype/LoadTIFF(): Reading Width & Height Tags.\n"));

		//Photometric must be RGB.
		
		//* Setup raster image buffer for ULONG RGBA pixel array *//		
		LONG npixels= w*h;
		Raster=(ULONG *) _TIFFmalloc(npixels *sizeof(ULONG));	
		if(!Raster)
		{
			D(bug("tiff.datatype/LoadTIFF(): AllocVec(RGBA Buffer) failed.\n"));
			SetIoErr(ERROR_NO_FREE_STORE);
			return FALSE;
		}
		D(bug("tiff.datatype/LoadTIFF(): RGBA buffer successfully allocated.\n"));
		
		
		//TIFFReadRGBAImageOriented( ... ); //Use raster RGBA Image Buffer
		TIFFReadRGBAImageOriented(tif, w, h, Raster, ORIENTATION_TOPLEFT, 0);		
		
		 
		//Close TIFF
		TIFFClose(tif);
		//retval = TRUE;
	}
	else
	{
		D(bug("tiff.datatype/LoadTIFF(): Not a valid TIFF File\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
	
	int x, y;    
	int source = 0;
	int target = 0;
	unsigned char a,r,g,b;	
	
	
	//* Setup RGB Image Buffer for RGB pixel array *//	
	unsigned char *RGBImage = NULL;		
	RGBImage=(unsigned char *) _TIFFmalloc(w * h *3);
	if(!RGBImage)
	{
		D(bug("tiff.datatype/LoadTIFF(): AllocVec(RGB Image Buffer) failed.\n"));
		SetIoErr(ERROR_NO_FREE_STORE);
		return FALSE;
	}
	D(bug("tiff.datatype/LoadTIFF(): RGB Image Buffer successfully allocated.\n"));
	
		

	D(bug("tiff.datatype/LoadTIFF(): Convert RGBA Image to RGB Image.\n"));	
	
	//Convert RGBA to RGB to Save As 24bit.
	for (y = 0; y < h; y++)
	{ 
		for (x = 0; x < w; x++)
		{   
			int arrayOffset = (y * w + x);
			int rgba = Raster[arrayOffset];
			r = (unsigned char)TIFFGetR(rgba);
			g =  (unsigned char)TIFFGetG(rgba);
			b =  (unsigned char)TIFFGetB(rgba);
			//a =  (unsigned char)TIFFGetA(rgba);
			
			RGBImage[source] = r; 
			RGBImage[source+1] = g; 
			RGBImage[source+2] = b; 
			//RGBImage[x+3] = a;
			
			source += 3;
		}
	}
	//Free raster image buffer
	_TIFFfree(Raster);
	//Free namebuffer
	
    
        //* Set Width & Height *//
	bmhd->bmh_Width  = w;
	bmhd->bmh_Height = h;
	bmhd->bmh_PageWidth = bmhd->bmh_Width;
	bmhd->bmh_PageHeight = bmhd->bmh_Height;

	D(bug("tiff.datatype/LoadTIFF(): Using 24 bit colors\n"));
	bmhd->bmh_Depth = 24;
	
	

	
	
	//* Write RGB image buffer to new datatype object. PBPAFMT_RGB *//
	D(bug("\ntiff.datatype/OM_NEW: WRITEPIXELARRAY of whole picture\n"));	
	
	//LONG source_offset;		
	//LONG rgbSize = (w *3);
	//for(row=0; row<h; row++)
	//{
		//Copy RGB Rows from RGB Image Buffer to RGB Row Buffer
		//source_offset = (row * rgbSize);		
		//memcpy(RGBBuffer, RGBImage + source_offset, rgbSize);			
		
		//Write RGB Row to new datatype object.
		if(!DoSuperMethod(cl, (Object *) o,
						PDTM_WRITEPIXELARRAY,	/* Method_ID */
						(IPTR) RGBImage,	/* PixelData */
						PBPAFMT_RGB,		/* PixelFormat */
						w*3,		/* PixelArrayMod (number of bytes per row) */
						0,			/* Left edge */
						0,			/* Top edge */
						w,			/* Width */
						h))			/* Height (here: one line) */
		{
			D(bug("tiff.datatype/OM_NEW: WRITEPIXELARRAY failed\n"));
			//FreeVec(RGBBuffer);
			return FALSE;
		}		
	//}
	D(bug("\ntiff.datatype/OM_NEW: WRITEPIXELARRAY of whole picture done\n"));	
	
	//Free image buffer
	_TIFFfree(RGBImage);
	

	//* Pass picture size to picture.datatype *//
	SetDTAttrs((Object *) o, NULL, NULL, DTA_ObjName,      (IPTR) Title,
	DTA_NominalHoriz, w,
	DTA_NominalVert,  h,
	TAG_DONE);

	D(bug("tiff.datatype/OM_NEW: Ending. (24 bit mode)\n"));
	return TRUE;
}

/**************************************************************************************************/

static BOOL Save_TIFF(struct IClass *cl, Object *o, struct dtWrite *dtw )
{
	BOOL err;
        ULONG row;
	ULONG depth;
        ULONG width, height;	
	//BOOL hascolormap = FALSE;
	UWORD photometric;
	UWORD compression;
	UWORD samplesperpixel;	
	IPTR sourcetype;
	BPTR filehandle;
	ULONG *colorregs;
	BOOL retval = FALSE;	
        STRPTR namebuffer = NULL;
	struct BitMapHeader *bmhd;	
	ULONG rowsperstrip = (ULONG) -1;
	
	//#define UWORD = uint16
	D(bug("\ntiff.datatype/Save_TIFF(): Beginning. (24 bit mode)\n"));
	

	//* A NULL file handle is a NOP *//
	if( !dtw->dtw_FileHandle )
	{
		D(bug("\ntiff.datatype/Save_TIFF(): --- empty Filehandle - just testing\n"));
		return TRUE;
	}
	filehandle = dtw->dtw_FileHandle;	
	
	//*  Convert filehandle *//
	D(bug("\ntiff.datatype/Save_TIFF(): Attempting to Get File Name from FileHandle.\n"));
	
	namebuffer = malloc(256 * sizeof(char));
	err= NameFromFH(filehandle, namebuffer, 256);
	if (err != 0)
	{
		D(bug("\ntiff.datatype/Save_TIFF(): File Name: %s\n", namebuffer));                  
	}
	else
	{
		D(bug("\ntiff.datatype/Save_TIFF(): Failed to get File Name.\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
	int length = strlen(namebuffer);
	namebuffer[length] = '\0';

	/* Get BitMapHeader and color palette */
	if( GetDTAttrs( o,  PDTA_BitMapHeader, (IPTR) &bmhd,
				PDTA_CRegs,        (IPTR) &colorregs,
				TAG_DONE ) != 2UL ||
			!bmhd || !colorregs )
	{
		D(bug("\ntiff.datatype/Save_TIFF(): --- missing attributes\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

	width = bmhd->bmh_Width;
	height = bmhd->bmh_Height;
	depth = bmhd->bmh_Depth;
	
	
	if( depth != 24 )
	{
		D(bug("\ntiff.datatype/Save_TIFF(): --- color depth %d, can save only depths of 24\n", depth));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
	D(bug("\ntiff.datatype/Save_TIFF(): --- Picture size %d x %d (x %d bit)\n", width, height, depth));
	
	
	// Open Tiff file to get new filehandle  for writing.	
	TIFF* tif = TIFFOpen(namebuffer, "w");
	if (tif)
	{
		// Get compression sheme for the output file.
		compression = COMPRESSION_LZW;
		//compression = COMPRESSION_PACKBITS;
		//compression = COMPRESSION_NONE;
		
		if (depth == 24)
		{
			samplesperpixel = 3; // there is no alpha channel
			photometric = PHOTOMETRIC_RGB;			
		}
		else
		{
			retval = FALSE;	
		}
		
		/* ----------------------------------------------------- */
		/*  Prepare output file.                                               */
		/* ----------------------------------------------------- */
		
		D(bug("\ntiff.datatype/Save_TIFF(): Prepare Output File..."));
		
		TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
		TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
		TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
		TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric);
		//TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);	
		TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, rowsperstrip));
		TIFFSetField(tif, TIFFTAG_COMPRESSION, compression);
		
		/* ----------------------------------------------------- */
		/*  Prepare image data.                                              */
		/* ----------------------------------------------------- */
		
		D(bug("\ntiff.datatype/Save_TIFF(): Prepare Image Data..."));
		
		int x, y;
		int offset;
		int rgbSize;
		unsigned char *rowbuffer;
		rowbuffer = (unsigned char *) _TIFFmalloc(width * samplesperpixel);	
		if(!rowbuffer)
		{
			D(bug("\ntiff.datatype/Save_TIFF(): Failed to Allocate Row Buffer.\n"));
			retval = FALSE;
		}

		// Write out the image data line by line.
		D(bug("\ntiff.datatype/Save_TIFF(): Write TIFF Scanlines...\n"));
		for (y = 0; y < height; y++ )
		{
			row = y;
			if(!DoSuperMethod(cl, o,
					PDTM_READPIXELARRAY,	/* Method_ID */
					(IPTR)rowbuffer,		/* PixelData */
					PBPAFMT_RGB,		/* PixelFormat */
					width*3,			/* PixelArrayMod (number of bytes per row) */
					0,			/* Left edge */
					y,			/* Top edge */
					width,			/* Width */
					1))			/* Height */
			{
				D(bug("tiff.datatype/Save_TIFF(): --- READPIXELARRAY line %d failed !\n", y));
				_TIFFfree(rowbuffer);
				SetIoErr(ERROR_OBJECT_WRONG_TYPE);
				return FALSE;
			}			
			TIFFWriteScanline(tif, rowbuffer, row, 0);			
		}
		_TIFFfree(rowbuffer);
		TIFFClose(tif);
		retval = TRUE;
	}
	else
	{
		D(bug("\ntiff.datatype/Save_TIFF(): Failed to Open File for Writing...%s\n", namebuffer));
		retval = FALSE;
		return retval;
	}
	D(bug("\ntiff.datatype/Save_TIFF(): Ending. (24 bit mode)\n"));
	
	

	D(bug("tiff.datatype/Save_TIFF(): --- Normal Exit\n"));	
	SetIoErr(0);
	
	return TRUE;
}

/**************************************************************************************************/

IPTR TIFF__OM_NEW(Class *cl, Object *o, Msg msg)
{
	Object *newobj;
	
	D(bug("tiff.datatype/DT_Dispatcher: Method OM_NEW\n"));
	
	newobj = (Object *)DoSuperMethodA(cl, o, msg);
	if (newobj)
	{
		if (!Load_TIFF(cl, newobj))
		{
			CoerceMethod(cl, newobj, OM_DISPOSE);
			newobj = NULL;
		}
	}

	return (IPTR)newobj;
}

/**************************************************************************************************/

IPTR TIFF__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
	D(bug("tiff.datatype/DT_Dispatcher: Method DTM_WRITE\n"));
	if( (dtw -> dtw_Mode) == DTWM_RAW )
	{
		/* Local data format requested */
		return Save_TIFF(cl, o, dtw );
	}
	else
	{
		/* Pass msg to superclass (which writes an IFF ILBM picture)... */
		return DoSuperMethodA( cl, o, (Msg)dtw );
	}
}
