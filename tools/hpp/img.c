#include <stdio.h>
#include <string.h>
#include <toollib/error.h>
#include "img.h"

/*
// a great deal of this code was ripped off wholesale from the Independent
// JPEG Group's sample source code, obtainable from any SimTel mirror under
// msdos/graphics/jpegsrc6.zip
*/

/*
// JPEG markers
*/
#define M_SOF0	0xC0	    /* Start Of Frame N */
#define M_SOF1	0xC1		/* N indicates which compression process */
#define M_SOF2	0xC2		/* Only SOF0-SOF2 are now in common use */
#define M_SOF3	0xC3
#define M_SOF5	0xC5		/* NB: codes C4 and CC are NOT SOF markers */
#define M_SOF6	0xC6
#define M_SOF7	0xC7
#define M_SOF9	0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI	0xD8		/* Start Of Image (beginning of datastream) */
#define M_EOI	0xD9		/* End Of Image (end of datastream) */
#define M_SOS	0xDA		/* Start Of Scan (begins compressed data) */
#define M_APP0	0xE0
#define M_COM	0xFE		/* COMment */

typedef unsigned char  BYTE;
typedef unsigned short WORD;
#ifndef TRUE
#   define TRUE 1
#endif
#ifndef FALSE
#   define FALSE 0
#endif

int JpegReadByte(FILE *file, BYTE *b)
{
    int i;

    if ((i = fgetc(file)) == EOF)
    {
	PushStdError ("Unable to read byte from JFIF file");
	return -1;
    }

    *b = (BYTE) i;

    return TRUE;
}

int JpegReadWord(FILE *file, WORD *w)
{
    int hi;
    int lo;

    /* words are kept in MSB format */
    if((hi = fgetc(file)) == EOF)
    {
	PushStdError ("unable to read high byte from JFIF file");
	return -1;
    }

    if((lo = fgetc(file)) == EOF)
    {
	PushStdError ("unable to read low byte from JFIF file");
	return -1;
    }

    *w = lo + 256*hi;

    return TRUE;
}

int JpegFirstMarker(FILE *file)
{
    BYTE flag;
    BYTE marker;

    /* move to the beginning of the file */
    if(fseek(file, 0, SEEK_SET) != 0)
    {
	PushStdError ("unable to seek to start of JFIF file");
	return -1;
    }

    /* look for the start of image marker */
    if(JpegReadByte(file, &flag) < 0)
    {
	return -1;
    }

    if(JpegReadByte(file, &marker) < 0)
    {
	return -1;
    }

    /* start of image? */
    if((flag != 0xFF) || (marker != M_SOI))
    {
	return FALSE;
    }

    return TRUE;
}

int JpegNextMarker(FILE *file, BYTE *marker)
{
    BYTE flag;
    int ok;

    /* move file pointer to next 0xFF flag */
    while((ok = JpegReadByte(file, &flag)) == TRUE)
    {
	if(flag == 0xFF)
	{
	    break;
	}
    }

    if (ok <= 0)
    {
	return ok;
    }

    /* extra 0xFF flags are legal as padding, so move past them */
    while((ok = JpegReadByte(file, marker)) == TRUE)
    {
	if(*marker != 0xFF)
	{
	    break;
	}
    }

    /* exit condition really depends if a good marker was found */
    return ok;
}

int JpegFormatFound(FILE *file)
{
    BYTE marker;
    char signature[8];
    int  rc;

    if((rc = JpegFirstMarker(file)) <= 0)
    {
	return rc;
    }

    if((rc = JpegNextMarker(file, &marker)) <= 0)
    {
	return rc;
    }

    /* should see an APP0 marker */
    if(marker != M_APP0)
    {
	return FALSE;
    }

    /* file format is now pointing to JFIF header ... skip two bytes and */
    /* look for the signature */
    if(fseek(file, 2, SEEK_CUR) != 0)
    {
	PushStdError ("unable to seek to start of JFIF file");
	return -1;
    }

    if(fread(signature, 1, 5, file) != 5)
    {
	PushStdError ("unable to read JFIF signature from file");
	return -1;
    }

    /* it all comes down to the signature being present */
    return (strcmp(signature, "JFIF") == 0) ? TRUE : FALSE;
}

int JpegReadDimensions(FILE *file, WORD *height, WORD *width)
{
    BYTE marker;
    int  rc;

    /* make sure we can find the first marker */
    if((rc = JpegFirstMarker(file)) <= 0)
    {
	return rc;
    }

    /* read file looking for SOF (start of frame) ... when it or */
    /* or SOS (start of scan, the compressed data) is reached, stop */
    while(JpegNextMarker(file, &marker) == TRUE)
    {
	/* if SOS, stop */
	if(marker == M_SOS)
	{
	    PushStdError ("JFIF SOS marker found before SOF marker");
	    break;
	}

	/* if not SOF, continue */
	if((marker < M_SOF0) || (marker > M_SOF15))
	{
	    continue;
	}

	/* start of frame found ... process the dimension information */
	/* seek past the next three bytes, useless for this application */
	if(fseek(file, 3, SEEK_CUR) != 0)
	{
	    PushStdError ("unable to seek in frame");
	    return -1;
	}

	/* read the height and width and get outta here */
	if((rc = JpegReadWord(file, height)) <= 0)
	{
	    return rc;
	}

	if((rc = JpegReadWord(file, width)) <= 0)
	{
	    return rc;
	}

	return TRUE;
    }

    PushStdError ("JFIF SOF marker not found");

    /* didn't find the SOF or found the SOS */
    return FALSE;
}


static int
GIF_GetSize (FILE * file, int * width, int * height)
{
    unsigned char buffer[10];

    if (fseek (file, 0, SEEK_SET) != 0)
    {
	PushStdError ("Can't seek to start of GIF file");
	return -1;
    }

    if (fread (buffer, 10, 1, file) < 0)
    {
	PushStdError ("Can't read header of GIF file");
	return -1;
    }

    if (strncmp (buffer, "GIF87a", 6)
	&& strncmp (buffer, "GIF89a", 6)
    )
	return 0;

    *width  = buffer[6] + 256*buffer[7];
    *height = buffer[8] + 256*buffer[9];

    return 1;
}

int
JPEG_GetSize (FILE * fh, int * width, int * height)
{
    int  rc;
    WORD w, h;

    if ((rc = JpegFormatFound (fh)) <= 0)
	return rc;

    if ((rc = JpegReadDimensions (fh, &h, &w)) <= 0)
	return rc;

    *width  = w;
    *height = h;

    return 1;
}

int
IMG_GetSize (const char * filename, int * width, int * height)
{
    int    rc;
    FILE * fh;

    fh = fopen (filename, "r");

    if (!fh)
	return 0;

    rc = GIF_GetSize (fh, width, height);

    if (!rc)
	rc = JPEG_GetSize (fh, width, height);

    if (rc < 0)
    {
	PushError ("Error reading file %s\n", filename);
    }

    fclose (fh);

    return rc;
}
