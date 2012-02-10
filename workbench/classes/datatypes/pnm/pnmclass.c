/*
	Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

#include <aros/symbolsets.h>

#include "debug.h"

#include "methods.h"

/* Open superclass */
ADD2LIBS("datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**********************************************************************/

static int ReadNextInt(BPTR FileHandle)
{
	int c;
	int retval = 0;

	c = FGetC(FileHandle);
	while (c==' ' || c=='\t' || c=='\r' || c=='\n' || c==12) 
	{
		c = FGetC(FileHandle);
	}
	
	retval = c - '0';
	c = FGetC(FileHandle);
	while (c!=' ' && c!='\t' && c!='\r' && c!='\n' && c!=12 && c!=EOF) 
	{
		retval = retval * 10 + (c - '0');
		c = FGetC(FileHandle);
	}
	
	return retval;	
}

/**************************************************************************************************/
static BOOL LoadPNM(struct IClass *cl, Object *o)
{
	char *Title;
	IPTR sourcetype;
	BPTR FileHandle;
	struct BitMapHeader *bmhd;
	char LineBuffer[128];
	long Width, Height, MaxVal, NumChars;
	unsigned int i,j,k;
	unsigned char *RGBBuffer;
	unsigned char *ReadBuffer;
	unsigned int PNMTYPE;
	int c;
	int bytePerRow;
	int curbit, nbbyte, curcol;
	unsigned char currentgray;
	char *curbyte;

	D(bug("pnm.datatype/LoadPNM()\n"));

	if( GetDTAttrs((Object *) o,
				DTA_SourceType    , (IPTR)&sourcetype ,
				DTA_Handle        , (IPTR)&FileHandle,
				DTA_Name          , (IPTR)&Title,
				PDTA_BitMapHeader , (IPTR)&bmhd,
				TAG_DONE) != 4 )
	{
		D(bug("pnm.datatype/LoadPNM(): GetDTAttrs(DTA_Handle, DTA_BitMapHeader) error !\n"));
		SetIoErr(ERROR_OBJECT_NOT_FOUND);
		return FALSE;
	}
	D(bug("pnm.datatype/LoadPNM(): GetDTAttrs(DTA_Handle, DTA_BitMapHeader) successful\n"));
	
	if ( sourcetype == DTST_RAM && FileHandle == BNULL )
	{
		D(bug("pnm.datatype/LoadPNM(): Creating an empty object\n"));
		return TRUE;
	}
	if ( sourcetype != DTST_FILE || !FileHandle || !bmhd )
	{
		D(bug("pnm.datatype/LoadPNM(): Unsupported sourcetype mode\n"));
		SetIoErr(ERROR_NOT_IMPLEMENTED);
		return FALSE;
	}

	D(bug("pnm.datatype/LoadPNM(): Title: %s\n", Title?Title:"[none]"));
	
	// Read file header
	Seek(FileHandle, 0, OFFSET_BEGINNING);
	D(bug("pnm.datatype/LoadPNM(): Seek successfull\n"));

	if(!FGets(FileHandle, LineBuffer, 128))
	{
		D(bug("pnm.datatype/LoadPNM(): FGets line 1 failed\n"));

		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

	// Check for file type
	if(LineBuffer[0] == 'P')
	{
		switch (LineBuffer[1])
		{
		case '1':
			PNMTYPE = 1;
			break;
		case '2':
			PNMTYPE = 2;
			break;
		case '3':
			PNMTYPE = 3;
			break;
		case '4':
			PNMTYPE = 4;
			break;
		case '5':
			PNMTYPE = 5;
			break;
		case '6':
			PNMTYPE = 6;
			break;
			default	:
			D(bug("pnm.datatype/LoadPNM(): Not a supported PNM\n"));
			SetIoErr(ERROR_OBJECT_WRONG_TYPE);
			return FALSE;
		}
	}
	else
	{
		D(bug("pnm.datatype/LoadPNM(): Not a PNM File\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

	D(bug("pnm.datatype/LoadPNM(): It's a P%d PNM\n"),PNMTYPE);

	if(!FGets(FileHandle, LineBuffer, 128))
	{
		D(bug("pnm.datatype/LoadPNM(): FGets line 2 failed\n"));

		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

	while(LineBuffer[0]=='#')
	{
		D(bug("pnm.datatype/LoadPNM(): current line is a comment\n"));

		if(!FGets(FileHandle, LineBuffer, 128))
		{
			D(bug("pnm.datatype/LoadPNM(): FGets line 3 after comment failed\n"));

			SetIoErr(ERROR_OBJECT_WRONG_TYPE);
			return FALSE;
		}
	}

	NumChars=StrToLong(LineBuffer, (LONG *)&Width);

	if(!((NumChars>0) && (Width>0)))
	{
		D(bug("pnm.datatype/LoadPNM(): StrToLong(Width) failed\n"));

		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

	D(bug("pnm.datatype/LoadPNM(): Width: %ld\n", (long) Width));
	D(bug("pnm.datatype/LoadPNM(): NumChars: %ld\n", (long) NumChars));

	NumChars=StrToLong(LineBuffer+NumChars, (LONG *)&Height);

	if(!((NumChars>0) && (Height>0)))
	{
		D(bug("pnm.datatype/LoadPNM(): StrToLong(Height) failed\n"));

		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

	D(bug("pnm.datatype/LoadPNM(): Height: %ld\n", (long) Height));
	D(bug("pnm.datatype/LoadPNM(): NumChars: %ld\n", (long) NumChars));

	if (PNMTYPE!=1 && PNMTYPE!=4)
	{
		if(!FGets(FileHandle, LineBuffer, 128))
		{
			D(bug("pnm.datatype/LoadPNM(): FGets line 3 (4) failed\n"));

			SetIoErr(ERROR_OBJECT_WRONG_TYPE);
			return FALSE;
		}

		NumChars=StrToLong(LineBuffer, (LONG *)&MaxVal);
		
		if(MaxVal > 65535)
		{
			D(bug("pnm.datatype/LoadPNM(): Wrong depth\n"));

			SetIoErr(ERROR_OBJECT_WRONG_TYPE);
			return(0);
		}
	}
	else
	{
		MaxVal = 1;
	}

	D(bug("pnm.datatype/LoadPNM(): Header successfull read\n"));
	// end of header

	bmhd->bmh_Width  = Width;
	bmhd->bmh_Height = Height;
	bmhd->bmh_PageWidth = bmhd->bmh_Width;
	bmhd->bmh_PageHeight = bmhd->bmh_Height;

	D(bug("pnm.datatype/LoadPNM(): Using 24 bit colors\n"));
	bmhd->bmh_Depth = 24;

	/* Get a buffer for one line of RGB triples */
	RGBBuffer=AllocVec(Width*3, MEMF_ANY | MEMF_CLEAR);
	if(!RGBBuffer)
	{
		D(bug("pnm.datatype/LoadPNM(): AllocVec(RGBBuffer) failed\n"));
		SetIoErr(ERROR_NO_FREE_STORE);
		return FALSE;
	}
	D(bug("pnm.datatype/LoadPNM(): RGBBuffer successfully allocated\n"));

	switch (PNMTYPE)
	{
	case 1:
		// PBM ASCII format
		for(i=0; i<Height; i++)
		{
			for (j=0; j<Width; j++)
			{
				c = FGetC(FileHandle);
				while (c==' ' || c=='\t' || c=='\r' || c=='\n' || c==12) 
				{
					c = FGetC(FileHandle);
				}
				if (c==EOF)
				{
					D(bug("pnm.datatype/LoadPNM(): EOF reached\n"));
					FreeVec(RGBBuffer);
					return FALSE;
				}
				RGBBuffer[j*3 + 0] = 255 * (c == '0');
				RGBBuffer[j*3 + 1] = 255 * (c == '0');
				RGBBuffer[j*3 + 2] = 255 * (c == '0');
			}
			
			if(!DoSuperMethod(cl, (Object *) o,
						PDTM_WRITEPIXELARRAY,	/* Method_ID */
						(IPTR) RGBBuffer,	/* PixelData */
						PBPAFMT_RGB,		/* PixelFormat */
						Width*3,		/* PixelArrayMod (number of bytes per row) */
						0,			/* Left edge */
						i,			/* Top edge */
						Width,			/* Width */
						1))			/* Height (here: one line) */
			{
				D(bug("pnm.datatype/OM_NEW: WRITEPIXELARRAY failed\n"));
				FreeVec(RGBBuffer);
				return FALSE;
			}
		}
		D(bug("pnm.datatype/OM_NEW: WRITEPIXELARRAY of whole picture done\n"));
		break;
	case 2:
		// PGM ASCII FORMAT
		for(i=0; i<Height; i++)
		{
			for (j=0;j<Width;j++)
			{
				curcol = ReadNextInt(FileHandle);
				RGBBuffer[j*3 + 0] = (unsigned char)(curcol * 255 / MaxVal);
				RGBBuffer[j*3 + 1] = (unsigned char)(curcol * 255 / MaxVal);
				RGBBuffer[j*3 + 2] = (unsigned char)(curcol * 255 / MaxVal);
			}

			if(!DoSuperMethod(cl, (Object *) o,
						PDTM_WRITEPIXELARRAY,	/* Method_ID */
						(IPTR) RGBBuffer,	/* PixelData */
						PBPAFMT_RGB,		/* PixelFormat */
						Width*3,		/* PixelArrayMod (number of bytes per row) */
						0,			/* Left edge */
						i,			/* Top edge */
						Width,			/* Width */
						1))			/* Height (here: one line) */
			{
				D(bug("pnm.datatype/LoadPNM(): WRITEPIXELARRAY failed\n"));
				FreeVec(RGBBuffer);
				return FALSE;
			}
		}
		D(bug("pnm.datatype/LoadPNM(): WRITEPIXELARRAY of whole picture done\n"));
		break;

	case 3:
		// PPM ASCII FORMAT
		for(i=0; i<Height; i++)
		{
			for (j=0;j<Width;j++)
			{
				for (k=0;k<3;k++)
				{
					curcol = ReadNextInt(FileHandle);
					RGBBuffer[j*3 + k] = (unsigned char)(curcol * 255 / MaxVal);
				}
			}

			if(!DoSuperMethod(cl, (Object *) o,
						PDTM_WRITEPIXELARRAY,	/* Method_ID */
						(IPTR) RGBBuffer,	/* PixelData */
						PBPAFMT_RGB,		/* PixelFormat */
						Width*3,		/* PixelArrayMod (number of bytes per row) */
						0,			/* Left edge */
						i,			/* Top edge */
						Width,			/* Width */
						1))			/* Height (here: one line) */
			{
				D(bug("pnm.datatype/LoadPNM(): WRITEPIXELARRAY failed\n"));
				FreeVec(RGBBuffer);
				return FALSE;
			}
		}
		D(bug("pnm.datatype/LoadPNM(): WRITEPIXELARRAY of whole picture done\n"));
		break;
	case 4:
		// PBM BINARY FORMAT
		
		/* Flush filehandle, so that unbuffered Read() can be used after buffered FGets() */
		Flush(FileHandle);
	
		bytePerRow = (Width+7)/8;
		ReadBuffer = AllocVec(bytePerRow, MEMF_ANY | MEMF_CLEAR);
		
		for(i=0; i<Height; i++)
		{
			if(!(Read(FileHandle, ReadBuffer, (bytePerRow))==(bytePerRow)))
			{
				D(bug("pnm.datatype/LoadPNM(): Read(RGBBuffer) failed, maybe file too short\n"));
				FreeVec(ReadBuffer);
				FreeVec(RGBBuffer);
				SetIoErr(ERROR_OBJECT_WRONG_TYPE);
				return FALSE;
			}
			
			curbit = 0;
			curbyte = ReadBuffer;
			for (j=0; j<Width; j++)
			{
				currentgray = *curbyte & (0x80 >> curbit++) ? 0 : 255;
				if (curbit>7)
				{
					curbit = 0;
					curbyte++;
				}
				RGBBuffer[j*3 + 0] = currentgray;
				RGBBuffer[j*3 + 1] = currentgray;
				RGBBuffer[j*3 + 2] = currentgray;
			}

			if(!DoSuperMethod(cl, (Object *) o,
						PDTM_WRITEPIXELARRAY,	/* Method_ID */
						(IPTR) RGBBuffer,	/* PixelData */
						PBPAFMT_RGB,		/* PixelFormat */
						Width*3,		/* PixelArrayMod (number of bytes per row) */
						0,			/* Left edge */
						i,			/* Top edge */
						Width,			/* Width */
						1))			/* Height (here: one line) */
			{
				D(bug("pnm.datatype/OM_NEW: WRITEPIXELARRAY failed\n"));
				FreeVec(ReadBuffer);
				FreeVec(RGBBuffer);
				return FALSE;
			}
		}

		FreeVec(ReadBuffer);
		D(bug("pnm.datatype/OM_NEW: WRITEPIXELARRAY of whole picture done\n"));
		break;
	case 5:
		// PGM BINARY FORMAT

		/* Flush filehandle, so that unbuffered Read() can be used after buffered FGets() */
		Flush(FileHandle);

		if (MaxVal < 256)
		{
			ReadBuffer = AllocVec(Width, MEMF_ANY | MEMF_CLEAR);
			nbbyte = 1;
		}
		else
		{
			ReadBuffer = AllocVec(Width*2, MEMF_ANY | MEMF_CLEAR);
			nbbyte = 2;
		}
		
		for(i=0; i<Height; i++)
		{
			if(!(Read(FileHandle, ReadBuffer, (Width*nbbyte))==(Width*nbbyte)))
			{
				D(bug("pnm.datatype/LoadPNM(): Read(RGBBuffer) failed, maybe file too short\n"));
				FreeVec(ReadBuffer);
				FreeVec(RGBBuffer);
				SetIoErr(ERROR_OBJECT_WRONG_TYPE);
				return FALSE;
			}
			
			if (nbbyte==2)
			for (j=0; j<Width; j++)
			{
				currentgray = (unsigned char)((ReadBuffer[j*nbbyte] * 256 + ReadBuffer[j*nbbyte + 1]) * 255 / MaxVal);
				RGBBuffer[j*3 + 0] = currentgray;
				RGBBuffer[j*3 + 1] = currentgray;
				RGBBuffer[j*3 + 2] = currentgray;
			}
			else
			for (j=0; j<Width; j++)
			{
				currentgray = (unsigned char)(ReadBuffer[j] * 255 / MaxVal);
				RGBBuffer[j*3 + 0] = currentgray;
				RGBBuffer[j*3 + 1] = currentgray;
				RGBBuffer[j*3 + 2] = currentgray;
			}
			
			if(!DoSuperMethod(cl, (Object *) o,
						PDTM_WRITEPIXELARRAY,	/* Method_ID */
						(IPTR) RGBBuffer,	/* PixelData */
						PBPAFMT_RGB,		/* PixelFormat */
						Width*3,		/* PixelArrayMod (number of bytes per row) */
						0,			/* Left edge */
						i,			/* Top edge */
						Width,			/* Width */
						1))			/* Height (here: one line) */
			{
				D(bug("pnm.datatype/OM_NEW: WRITEPIXELARRAY failed\n"));
				FreeVec(ReadBuffer);
				FreeVec(RGBBuffer);
				return FALSE;
			}
		}

		FreeVec(ReadBuffer);
		D(bug("pnm.datatype/OM_NEW: WRITEPIXELARRAY of whole picture done\n"));
		break;
	case 6:
		// PPM BINARY FORMAT

		/* Flush filehandle, so that unbuffered Read() can be used after buffered FGets() */
		Flush(FileHandle);

		/* Copy picture line by line to picture.datatype using WRITEPIXELARRAY method */
		for(i=0; i<Height; i++)
		{
			if(!(Read(FileHandle, RGBBuffer, (Width*3))==(Width*3)))
			{
				D(bug("pnm.datatype/LoadPNM(): Read(RGBBuffer) failed, maybe file too short\n"));
				FreeVec(RGBBuffer);
				SetIoErr(ERROR_OBJECT_WRONG_TYPE);
				return FALSE;
			}
			if(!DoSuperMethod(cl, (Object *) o,
						PDTM_WRITEPIXELARRAY,	/* Method_ID */
						(IPTR) RGBBuffer,	/* PixelData */
						PBPAFMT_RGB,		/* PixelFormat */
						Width*3,		/* PixelArrayMod (number of bytes per row) */
						0,			/* Left edge */
						i,			/* Top edge */
						Width,			/* Width */
						1))			/* Height (here: one line) */
			{
				D(bug("pnm.datatype/LoadPNM(): WRITEPIXELARRAY failed\n"));
				FreeVec(RGBBuffer);
				return FALSE;
			}
		}
		D(bug("pnm.datatype/LoadPNM(): WRITEPIXELARRAY of whole picture done\n"));
		break;
	default:
		D(bug("pnm.datatype/LoadPNM(): WRONG PNMTYPE failed\n"));
		FreeVec(RGBBuffer);
		return FALSE;
	}

	FreeVec(RGBBuffer);

	SetDTAttrs((Object *) o, NULL, NULL, DTA_ObjName,      (IPTR) Title,
	DTA_NominalHoriz, Width,
	DTA_NominalVert,  Height,
	TAG_DONE);

	D(bug("pnm.datatype/OM_NEW: Leaving. (24 bit mode)\n"));
	return TRUE;
}

/**************************************************************************************************/

static BOOL SavePNM(struct IClass *cl, Object *o, struct dtWrite *dtw )
{
	BPTR		    filehandle;
	unsigned int    width, height, numplanes, y;
	UBYTE		    *linebuf;
	struct BitMapHeader     *bmhd;
	long                    *colorregs;

	D(bug("ppm.datatype/SavePNM()\n"));

	/* A NULL file handle is a NOP */
	if( !dtw->dtw_FileHandle )
	{
		D(bug("ppm.datatype/SavePNM() --- empty Filehandle - just testing\n"));
		return TRUE;
	}
	filehandle = dtw->dtw_FileHandle;

	/* Get BitMapHeader and color palette */
	if( GetDTAttrs( o,  PDTA_BitMapHeader, (IPTR) &bmhd,
				PDTA_CRegs,        (IPTR) &colorregs,
				TAG_DONE ) != 2UL ||
			!bmhd || !colorregs )
	{
		D(bug("ppm.datatype/SavePNM() --- missing attributes\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

	width = bmhd->bmh_Width;
	height = bmhd->bmh_Height;
	numplanes = bmhd->bmh_Depth;
	if( numplanes != 24 )
	{
		D(bug("ppm.datatype/SavePNM() --- color depth %d, can save only depths of 24\n", numplanes));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
	D(bug("ppm.datatype/SavePNM() --- Picture size %d x %d (x %d bit)\n", width, height, numplanes));

	/* Write header to file */
	if( FPrintf( filehandle, "P6\n#Created by AROS ppm.datatype aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n%ld %ld\n255\n",
				(long)width, (long)height ) == -1 )
	{
		D(bug("ppm.datatype/SavePNM() --- writing header failed\n"));
		return FALSE;
	}

	/* Now read the picture data line by line and write it to a chunky buffer */
	if( !(linebuf = AllocVec(width*3, MEMF_ANY)) )
	{
		SetIoErr(ERROR_NO_FREE_STORE);
		return FALSE;
	}
	D(bug("ppm.datatype/PPM_Save() --- copying picture with READPIXELARRAY\n"));
	for (y=0; y<height; y++)
	{
		if(!DoSuperMethod(cl, o,
					PDTM_READPIXELARRAY,	/* Method_ID */
					(IPTR)linebuf,		/* PixelData */
					PBPAFMT_RGB,		/* PixelFormat */
					width,			/* PixelArrayMod (number of bytes per row) */
					0,			/* Left edge */
					y,			/* Top edge */
					width,			/* Width */
					1))			/* Height */
		{
			D(bug("ppm.datatype/SavePNM() --- READPIXELARRAY line %d failed !\n", y));
			FreeVec(linebuf);
			SetIoErr(ERROR_OBJECT_WRONG_TYPE);
			return FALSE;
		}
		if( FWrite( filehandle, linebuf, width*3, 1 ) != 1 )
		{
			D(bug("ppm.datatype/SavePNM() --- writing picture data line %d failed !\n", y));
			FreeVec(linebuf);
			return FALSE;
		}
	}

	D(bug("ppm.datatype/SavePNM() --- Normal Exit\n"));
	FreeVec(linebuf);
	SetIoErr(0);
	return TRUE;
}

/**************************************************************************************************/

IPTR PNM__OM_NEW(Class *cl, Object *o, Msg msg)
{
	Object *newobj;
	
	D(bug("pnm.datatype/DT_Dispatcher: Method OM_NEW\n"));
	
	newobj = (Object *)DoSuperMethodA(cl, o, msg);
	if (newobj)
	{
		if (!LoadPNM(cl, newobj))
		{
			CoerceMethod(cl, newobj, OM_DISPOSE);
			newobj = NULL;
		}
	}

	return (IPTR)newobj;
}

/**************************************************************************************************/

IPTR PNM__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
	D(bug("pnm.datatype/DT_Dispatcher: Method DTM_WRITE\n"));
	if( (dtw -> dtw_Mode) == DTWM_RAW )
	{
		/* Local data format requested */
		return SavePNM(cl, o, dtw );
	}
	else
	{
		/* Pass msg to superclass (which writes an IFF ILBM picture)... */
		return DoSuperMethodA( cl, o, (Msg)dtw );
	}
}
