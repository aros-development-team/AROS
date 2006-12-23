/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tool to convert IFF ILBM images into C source.
	      
    Lang: 
    
*/

/****************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>

/****************************************************************************************/

#define MAKE_ID(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | ((d)))

#define ID_FORM MAKE_ID('F','O','R','M')
#define ID_ILBM MAKE_ID('I','L','B','M')
#define ID_CMAP MAKE_ID('C','M','A','P')
#define ID_BODY MAKE_ID('B','O','D','Y')
#define ID_BMHD MAKE_ID('B','M','H','D')

#define CMP_NONE     0
#define CMP_BYTERUN1 1

#define MSK_HASMASK  1

/****************************************************************************************/

/* For this tool it does not really matter if the following types
   have a bigger sizeof() than on Amiga */

#ifndef __AROS__
typedef unsigned long 	ULONG;
typedef long	    	LONG;
typedef unsigned short  UWORD;
typedef short 	    	WORD;
typedef short	    	BOOL;
typedef unsigned char 	UBYTE;
#else
#include <exec/types.h>
#endif

/****************************************************************************************/

struct BitMapHeader
{
    UWORD        bmh_Width;
    UWORD        bmh_Height;
    WORD         bmh_Left;
    WORD         bmh_Top;
    UBYTE        bmh_Depth;
    UBYTE        bmh_Masking;
    UBYTE        bmh_Compression;
    UBYTE        bmh_Pad;
    UWORD        bmh_Transparent;
    UBYTE        bmh_XAspect;
    UBYTE        bmh_YAspect;
    WORD         bmh_PageWidth;
    WORD         bmh_PageHeight;
};

/****************************************************************************************/

static char 	     	    *filename;
static FILE 	     	    *file;
static unsigned char 	    *filebuffer, *body, *planarbuffer, *chunkybuffer;
static unsigned char	    *planarbuffer_packed, *chunkybuffer_packed;
static long 	     	    filesize, bodysize, bodysize_packed;
static long 	     	    filepos;
static struct BitMapHeader  bmh;
static LONG 	    	    cmapentries, totdepth, bpr;
static BOOL 	    	    have_bmhd, have_cmap, have_body;
static UBYTE	    	    red[256], green[256], blue[256];
static char 	    	    imagename[1000];
static char 	    	    bigimagename[1000];

/****************************************************************************************/

static void cleanup(char *msg, int rc)
{
    if (msg) fprintf(stderr, "ilbmtoc: %s\n", msg);
    
    if (chunkybuffer_packed) free(chunkybuffer_packed);
    if (planarbuffer_packed) free(planarbuffer_packed);
    if (chunkybuffer) free(chunkybuffer);
    if (planarbuffer) free(planarbuffer);    
    if (filebuffer) free(filebuffer);
    
    if (file) fclose(file);
    
    exit(rc);
}

/****************************************************************************************/

static void getarguments(int argc, char **argv)
{
    char *imagenamestart, *sp;
    WORD i;
    
    if (argc != 2)
    {
    	fprintf(stderr, "Wrong number of arguments\n");
    }
    
    if (argc < 2)
    {
    	cleanup("Usage: ilbmtoc filename", 1);
    }
    
    filename = argv[1];
    
    if (strlen(filename) >= sizeof(imagename)) cleanup("Filename too long!", 1);
    
    imagenamestart = filename;
    for(;;)
    {
    	sp = strchr(imagenamestart + 1, '/');
	if (!sp) sp = strchr(imagenamestart + 1, '\\');
	if (!sp) sp = strchr(imagenamestart + 1, ':');
	if (!sp) break;
	
	imagenamestart = sp + 1;
    }
    
    strcpy(imagename, imagenamestart);
    if ((sp = strchr(imagename, '.'))) *sp = 0;
    
    for(i = 0; i < strlen(imagename); i++) bigimagename[i] = toupper(imagename[i]);
}

/****************************************************************************************/

static ULONG getlong(void)
{
    ULONG ret;
    
    if (filepos > filesize - 4) cleanup("Tried to read over file end!", 1);

    ret =  filebuffer[filepos++] * 0x1000000;
    ret += filebuffer[filepos++] * 0x10000;
    ret += filebuffer[filepos++] * 0x100;
    ret += filebuffer[filepos++];
    
    return ret;
}

/****************************************************************************************/

static UWORD getword(void)
{
    UWORD ret;
    
    if (filepos > filesize - 2) cleanup("Tried to read over file end!", 1);

    ret =  filebuffer[filepos++] * 0x100;
    ret += filebuffer[filepos++];
    
    return ret;
}

/****************************************************************************************/

static UBYTE getbyte(void)
{
    ULONG ret;
    
    if (filepos > filesize - 1) cleanup("Tried to read over file end!", 1);
    ret = filebuffer[filepos++];
    
    return ret;
}

/****************************************************************************************/

static void skipbytes(ULONG howmany)
{
    filepos += howmany;
}

/****************************************************************************************/

static void openfile(void)
{
    file = fopen(filename, "rb");
    if (!file) cleanup("Can't open file!", 1);
    
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    
    if (filesize < 12) cleanup("Bad file size!", 1);
    
    fprintf(stderr, "Filesize is %d\n", filesize);
    
    fseek(file, 0, SEEK_SET);
    
    filebuffer = malloc(filesize + 10);
    if (!filebuffer) cleanup("Memory allocation for file buffer failed!", 1);
    
    if (fread(filebuffer, 1, filesize, file) != filesize)
    	cleanup("Error reading file!", 1);
    
    fclose(file); file = NULL;
}

/****************************************************************************************/

static void checkfile(void)
{
    ULONG id;
    ULONG size;
    
    id = getlong();
    if (id != ID_FORM) cleanup("File is not an IFF file!", 1);
    
    size = getlong();
    if (size != filesize - 8) cleanup("File is IFF, but has bad size in IFF header!", 1);
    
    id = getlong();
    if (id != ID_ILBM) cleanup("File is IFF, but not of type ILBM!", 1);
}

/****************************************************************************************/

static void scanfile(void)
{
    WORD i;
    
    for(;;)
    {
    	ULONG id;
	ULONG size;
	
	id   = getlong();
	size = getlong();

	fprintf(stderr, "Chunk: %c%c%c%c  Size: %d\n", id >> 24, id >> 16, id >> 8, id, size);
		
	switch(id)
	{
	    case ID_BMHD:
	    	if (size != 20) cleanup("Bad BMHD chunk size!", 1);
		
	    	bmh.bmh_Width 	    = getword();
		bmh.bmh_Height      = getword();
		bmh.bmh_Left 	    = (WORD)getword();
		bmh.bmh_Top 	    = (WORD)getword();
		bmh.bmh_Depth 	    = getbyte();
		bmh.bmh_Masking     = getbyte();
		bmh.bmh_Compression = getbyte();
		bmh.bmh_Pad 	    = getbyte();
		bmh.bmh_Transparent = getword();
		bmh.bmh_XAspect     = getbyte();
		bmh.bmh_YAspect     = getbyte();
		bmh.bmh_PageWidth   = (WORD)getword();
		bmh.bmh_PageHeight  = (WORD)getword();
		
		if (bmh.bmh_Depth > 8) cleanup("ILBM file has too many colors!", 1);
		if ((bmh.bmh_Compression != CMP_NONE) && (bmh.bmh_Compression != CMP_BYTERUN1)) cleanup("Compression method unsupported!", 1);
		
		have_bmhd = 1;
		
		totdepth = bmh.bmh_Depth + ((bmh.bmh_Masking == MSK_HASMASK) ? 1 : 0);
		
		bpr = ((bmh.bmh_Width + 15) & ~15) / 8;
		
		fprintf(stderr, "BMHD: %d x %d x %d (%d)\n", bmh.bmh_Width,
		    	    	    	    	    	     bmh.bmh_Height,
							     bmh.bmh_Depth,
							     totdepth);
		break;
	
	    case ID_CMAP:   
	    	if (!have_bmhd) cleanup("CMAP chunk before BMHD chunk (or no BMHD chunk at all!", 1);
		
		cmapentries = size / 3;
		if (size & 1) size++;
		
		if ((cmapentries < 2) || (cmapentries > 256)) cleanup("CMAP chunk has bad number of entries!", 1);
		
		for(i = 0; i < cmapentries; i++)
		{
		    red[i]   = getbyte();
		    green[i] = getbyte();
		    blue[i]  = getbyte();
		    size -= 3;
		}
	    	
		skipbytes(size);
		
		have_cmap = 1;
		
		break;
	
	    case ID_BODY:
	    	if (!have_bmhd) cleanup("BODY chunk before BMHD chunk (or no BMHD chunk at all!", 1);
		body = &filebuffer[filepos];
		bodysize = size;
		
		if (bmh.bmh_Compression == CMP_NONE)
		{
		    LONG shouldbesize = totdepth * bpr * bmh.bmh_Height;
		    if (bodysize != shouldbesize) cleanup("BODY chunk size seems to be wrong!", 1);
		}
		
		have_body = 1;
		/* Fall through */
		
	    default:
	    	if (size & 1) size++;
	    	skipbytes(size);
		break;
	}
	
	if (filepos == filesize) break;
	if (have_bmhd && have_body && have_cmap) break;
    }
    
    if (!have_bmhd) cleanup("BMHD chunk missing!", 1);
    if (!have_body) cleanup("BODY chunk missing!", 1);
}

/****************************************************************************************/

static unsigned char *unpack_byterun1(unsigned char *source, unsigned char *dest, LONG unpackedsize)
{
    unsigned char r;
    signed char c;
    
    for(;;)
    {
	c = (signed char)(*source++);
	if (c >= 0)
	{
    	    while(c-- >= 0)
	    {
		*dest++ = *source++;
		if (--unpackedsize <= 0) return source;
	    }
	}
	else if (c != -128)
	{
    	    c = -c;
	    r = *source++;

	    while(c-- >= 0)
	    {
		*dest++ = r;
		if (--unpackedsize <= 0) return source;
	    }
	}
    }
    
}

/****************************************************************************************/

static BOOL norm1(LONG count, unsigned char **source_backup,
    	    	  unsigned char **dest, LONG *checksize)
{
    //if (count >= 0) fprintf(stderr, "XX: non packable %d\n",count);
    
    while(count >= 0)
    {
	LONG step = count;

	if (step > 127) step = 127;

	*checksize -= step;
	*checksize -= 2;

	if (*checksize <= 0) return 0;

   	count -= step;

	*(*dest)++ = step;


	while(step-- >= 0)
	{
	    *(*dest)++ = *(*source_backup)++;
	}
	
	count--;

    }
    
    return 1;
}

static BOOL copy1(unsigned char r, LONG count, unsigned char **dest, LONG *checksize)
{
    //if (count >= 1) fprintf(stderr, "XX: repeat %02x x %d\n", r, count);
    
    while(--count >= 0)
    {
    	LONG step = count;
	
	if (step > 127) step = 127;
	
	count -= step;
	step = -step;
	*checksize -= 2;
	if (*checksize <= 0) return 0;
	
	*(*dest)++ = (unsigned char)step;
	*(*dest)++ = r;
    }
    
    return 1;
}

static BOOL pack_byterun1(unsigned char *source, unsigned char *dest,
    	    	      LONG size, LONG check_size, LONG *packsize)
{
    unsigned char *source_backup, *dest_backup;
    LONG samebytes_counter, samebytes, count;
    LONG checksize = check_size;
    unsigned char oldbyte, actbyte;
    
    if (checksize < 0) checksize = 0x7FFFFFFF;
    
    oldbyte = *source;
    samebytes_counter = 0;
    source_backup = source;
    dest_backup = dest;
    
    for(;;)
    {
    	//fprintf(stderr, "size = %d. checksize = %d\n", size, checksize);
    	if (--size < 0) break;
	actbyte = *source++;
	if (actbyte == oldbyte)
	{
	    samebytes_counter++;
	    continue;
	}
	
	oldbyte = actbyte;
	
	samebytes = samebytes_counter;
	samebytes_counter = 1;
	
	if (samebytes < 3) continue;
	
	count = (LONG)(source - source_backup - samebytes - 2);
	if (!norm1(count, &source_backup, &dest, &checksize)) return 0;
	
	if (!copy1(source[-2], samebytes, &dest, &checksize)) return 0;

    	source_backup = source - 1;
    }
    //fprintf(stderr, "done\n");    
    
    if (samebytes_counter >= 3)
    {
    	samebytes = samebytes_counter;
	count = (LONG)(source - source_backup - samebytes - 1);
	if (!norm1(count, &source_backup, &dest, &checksize)) return 0;
	if (!copy1(source[-2], samebytes, &dest, &checksize)) return 0;	
    }
    else
    {
    	count = (LONG)(source - source_backup - 1);
	if (!norm1(count, &source_backup, &dest, &checksize)) return 0;
    }
    //fprintf(stderr, "realdone\n");
    
    if (packsize) *packsize = (LONG)(dest - dest_backup);
    
    return 1;
}

/****************************************************************************************/

static void p2c(unsigned char *source, unsigned char *dest, LONG width, LONG height,
    	    	LONG totplanes, LONG wantplanes, LONG chunkybpr)
{
    LONG alignedwidth, x, y, p, bpr, bpl;
    
    alignedwidth = (width + 15) & ~15;
    bpr = alignedwidth / 8;
    bpl = bpr * totplanes;
    
    for(y = 0; y < height; y++)
    {
    	for(x = 0; x < width; x++)
	{
	    LONG mask   = 1 << (7 - (x & 7));
	    LONG offset = x / 8;
	    unsigned char chunkypix = 0;
	    
	    for(p = 0; p < wantplanes; p++)
	    {
	    	if (source[p * bpr + offset] & mask) chunkypix |= (1 << p);
	    }
	    dest[x] = chunkypix;
	}
	
	source += bpl;
	dest += chunkybpr;
    }
    
    
}

/****************************************************************************************/

static void convertbody(void)
{
    LONG unpackedsize = bpr * bmh.bmh_Height * totdepth;
    
    planarbuffer = malloc(unpackedsize);
    if (!planarbuffer) cleanup("Memory allocation for planar buffer failed!", 1);
    
    if (bmh.bmh_Compression == CMP_NONE)
    {
    	memcpy(planarbuffer, body, unpackedsize);
    }
    else
    {
    	unpack_byterun1(body, planarbuffer, unpackedsize);
    }
    
    chunkybuffer = malloc(bmh.bmh_Width * bmh.bmh_Height);
    if (!chunkybuffer) cleanup("Memory allocation for chunky buffer failed!", 1);
    
    p2c(planarbuffer,
    	chunkybuffer,
	bmh.bmh_Width,
	bmh.bmh_Height,
	totdepth,
	bmh.bmh_Depth,
	bmh.bmh_Width);
}

/****************************************************************************************/

static void packdata(void)
{
    LONG chunkysize = (LONG)bmh.bmh_Width * (LONG)bmh.bmh_Height;
    BOOL success;
    
    chunkybuffer_packed = malloc(chunkysize);
    
    if (!chunkybuffer_packed) cleanup("Memory allocation for packed chunky buffer failed!", 1);

    fprintf(stderr, "Starting packing\n"); 
       
    success = pack_byterun1(chunkybuffer, chunkybuffer_packed,
    	    	    	    chunkysize, chunkysize, &bodysize_packed);
			    
    fprintf(stderr, "Done packing. Success = %d\n", success);    
    if (!success)
    {
    	free(chunkybuffer_packed); chunkybuffer_packed = 0;
    }	  
}

/****************************************************************************************/

static void gensource(void)
{
    unsigned char *buffer;
    LONG i, x, y, buffersize;
    
    printf("#include <exec/types.h>\n");
    printf("\n");
    printf("#define %s_WIDTH  %d\n", bigimagename, bmh.bmh_Width);
    printf("#define %s_HEIGHT %d\n", bigimagename, bmh.bmh_Height);
    printf("#define %s_PACKED %d\n", bigimagename, (chunkybuffer_packed || planarbuffer_packed) ? 1 : 0);
    printf("#define %s_PLANES %d\n", bigimagename, bmh.bmh_Depth);
    if (have_cmap)
    	printf("#define %s_COLORS %d\n", bigimagename, cmapentries);
    
    printf("\n");
    
    if (have_cmap)
    {
    	printf("ULONG %s_pal[%d] =\n", imagename, cmapentries);
	printf("{\n");
	for(i = 0; i < cmapentries; i++)
	{
	    ULONG col = (((ULONG)red[i]) << 16) +
	    	    	(((ULONG)green[i]) << 8) +
			((ULONG)blue[i]);
			
	    printf("    0x%06x", col);
	    if (i == cmapentries - 1)
	    	printf("\n");
	    else
	    	printf(",\n");
	}
	printf("};\n\n");
    }
    
    
    if (chunkybuffer_packed)
    {
    	buffer = chunkybuffer_packed;
	buffersize = bodysize_packed;
    }
    else
    {
    	buffer = chunkybuffer;
	buffersize = bodysize;
    }

    printf("UBYTE %s_data[%d] =\n", imagename, buffersize);
    printf("{");
    
    i = 0;
        
    for(x = 0; x < buffersize; x++)
    {
	if ((i++ % 20) == 0) printf("\n    ");
	printf("0x%02x", buffer[x]);
	if (!(x == buffersize - 1)) printf(","); 
    }
    
    printf("\n");
    printf("};\n");
}

/****************************************************************************************/

int main(int argc, char **argv)
{
    getarguments(argc, argv);
    openfile();
    checkfile();
    scanfile();
    convertbody();
    packdata();
    gensource();
    
    cleanup(0, 0);
}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
