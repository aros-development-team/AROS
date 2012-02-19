/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tool to convert IFF ILBM images into Amiga icon file.
	      
    Lang: 
    
*/

#define D(x)

/****************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <png.h>

/****************************************************************************************/

#define TRUE    ~0
#define FALSE   0

#define MAKE_ID(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | ((d)))

#define ID_FORM MAKE_ID('F','O','R','M')
#define ID_ILBM MAKE_ID('I','L','B','M')
#define ID_CMAP MAKE_ID('C','M','A','P')
#define ID_BODY MAKE_ID('B','O','D','Y')
#define ID_BMHD MAKE_ID('B','M','H','D')

#define ID_ICON MAKE_ID('I','C','O','N')
#define ID_FACE MAKE_ID('F','A','C','E')
#define ID_IMAG MAKE_ID('I','M','A','G')
#define ID_PNG  MAKE_ID('p','n','g',' ')
#define ID_ARGB MAKE_ID('A','R','G','B')

#define CMP_NONE     0
#define CMP_BYTERUN1 1

#define MSK_HASMASK  1
#define MSK_HASTRANS 2

/****************************************************************************************/

/* For this tool it does not really matter if the following types
   have a bigger sizeof() than on Amiga */

#ifndef __AROS__
typedef void           *APTR;
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

struct ILBMImage
{
    struct BitMapHeader bmh;
    struct BitMapHeader planarbmh;
    unsigned char   	*planarbuffer, *chunkybuffer;
    LONG    	    	cmapentries, bpr, totdepth;
    UBYTE   	    	rgb[256][3];
    UBYTE   	    	remaptable[256];
    APTR                png;
    ULONG               png_size;
    APTR                argb;
    ULONG               argb_size;
};

/****************************************************************************************/

struct Palette
{
    UWORD numentries;
    UBYTE rgb[][3];
};

/****************************************************************************************/

struct Palette std4colpal =
{
    4,
    {
	{0x95, 0x95, 0x95},     /* Gray (and transparent!) */
	{0x00, 0x00, 0x00},     /* Black */
	{0xFF, 0xFF, 0xFF},     /* White */
	{0x3b, 0x67, 0xa2}      /* Blue */
    }
};

/****************************************************************************************/

struct Palette magicwb8colpal =
{
    8,
    {
	{0x95, 0x95, 0x95},     /* Gray (and transparent!) */
	{0x00, 0x00, 0x00},     /* Black */
	{0xFF, 0xFF, 0xFF},     /* White */
	{0x3b, 0x67, 0xa2},     /* Blue */
	{0x7b, 0x7b, 0x7b},     /* Dk. Gray */
	{0xaf, 0xaf, 0xaf},     /* Lt. Gray */
	{0xaa, 0x90, 0x7c},     /* Brown */
	{0xff, 0xa9, 0x97}      /* Pink */
    }
};

/****************************************************************************************/

struct Palette scalos16colpal =
{
    16,
    {   
	{ 0x9c, 0x9c, 0x9c },   /*  0 - Gray */
	{ 0x00, 0x00, 0x00 },   /*  1 - Black */
	{ 0xFF, 0xFF, 0xFF },   /*  2 - White */
	{ 0x3a, 0x3a, 0xd7 },   /*  3 - Blue */
	{ 0x75, 0x75, 0x75 },   /*  4 - Med. Gray */
	{ 0xc4, 0xc4, 0xc4 },   /*  5 - Lt. Gray */
	{ 0xd7, 0xb0, 0x75 },   /*  6 - Peach */
	{ 0xeb, 0x62, 0x9c },   /*  7 - Pink */
	{ 0x13, 0x75, 0x27 },   /*  8 - Dk. Green */
	{ 0x75, 0x3a, 0x00 },   /*  9 - Brown */
	{ 0xff, 0xd7, 0x13 },   /* 10 - Yellow */
	{ 0x3a, 0x3a, 0x3a },   /* 11 - Dk. Gray */
	{ 0xc4, 0x13, 0x27 },   /* 12 - Red */
	{ 0x27, 0xb0, 0x3a },   /* 13 - Lt. Green */
	{ 0x3a, 0x75, 0xff },   /* 14 - Lt. Blue */
	{ 0xd7, 0x75, 0x27 },   /* 15 - Orange */
    }
};

/****************************************************************************************/

static char 	     	    *filename, *outfilename, *infilename;
static unsigned char	    *filebuffer, *body;
static FILE 	     	    *file, *outfile, *infile;
static long 	     	    filesize, bodysize, bodysize_packed;
static long 	     	    filepos;
static struct ILBMImage     img1, img2;
static BOOL 	    	    have_bmhd, have_cmap, have_body, is_png;

static char 	    	    *image1option;
static char 	    	    *image2option;
static char 	    	    *defaulttooloption;
static char 	    	    *drawerdataoption;
static char 	    	    **tooltypesoption;
static LONG 	    	    typeoption = 3; /* WBTOOL */
static LONG 	    	    iconleftoption = 0x80000000; /* NO_ICON_POSITION */
static LONG 	    	    icontopoption = 0x80000000; /* NO_ICON_POSITION */
static LONG 	    	    stackoption = 4096;
static LONG 	    	    drawerleftoption = 50;
static LONG 	    	    drawertopoption = 50;
static LONG 	    	    drawerwidthoption = 400;
static LONG 	    	    drawerheightoption = 100;
static LONG 	    	    drawervleftoption = 0;
static LONG 	    	    drawervtopoption = 0;
static LONG 	    	    drawershowoption = 0;
static LONG 	    	    drawershowasoption = 0;
static LONG 	    	    transparentoption = -1;

static BOOL		    dualpng; /* png file contains second image */
static unsigned char	    *dualpngstart; /* address of 2nd image in filebuffer */
static BOOL		    nosavePNG; /* Don't save the original PNG data */
static BOOL		    nosaveARGB; /* Don't ARGB data */
static BOOL		    nosaveIFF; /* Don't save any IFF data */

/****************************************************************************************/

static void freeimage(struct ILBMImage *img)
{        
    if (img->chunkybuffer)
    {
    	free(img->chunkybuffer);
	img->chunkybuffer = NULL;
    }
    
    if (img->planarbuffer)
    {
    	free(img->planarbuffer);
	img->planarbuffer = NULL;
    }

    if (img->argb)
    {
        free(img->argb);
        img->argb = NULL;
    }
    
    if (filebuffer)
    {
    	free(filebuffer);
	filebuffer = NULL;
    }
    
    if (file)
    {
    	fclose(file);
	file = NULL;
    }

    filepos = 0;
}

/****************************************************************************************/

static void cleanup(char *msg, int rc)
{
    if (msg) fprintf(stderr, "ilbmtoicon: %s\n", msg);
    
    freeimage(&img1);
    freeimage(&img2);
    
    if (outfile) fclose(outfile);
    if (infile) fclose(infile);
    
    exit(rc);
}

/****************************************************************************************/

static void getarguments(int argc, char **argv)
{
    WORD i;

    nosavePNG = 1;

    for (; argc > 1 && argv[1][0] == '-'; argc--, argv++) {
        if (strcmp(argv[1],"--png") == 0) {
            nosavePNG = 0;
            continue;
        }
        if (strcmp(argv[1],"--no-iff") == 0) {
            nosaveIFF = 1;
            continue;
        }
        if (strcmp(argv[1],"--no-argb") == 0) {
            nosaveIFF = 1;
            continue;
        }
    }

    
    if ((argc != 4) && (argc != 5))
    {
    	fprintf(stderr, "Wrong number of arguments\n");
    	cleanup("Usage: ilbmtoicon [--png] [--no-argb] [--no-iff] icondescription image1 [image2] filename", 1);
    }
    
    if (argc == 4)
    {
    	infilename   = argv[1];
    	image1option = argv[2];
	outfilename  = argv[3];
    }
    else if (argc == 5)
    {
    	infilename   = argv[1];
    	image1option = argv[2];
	image2option = argv[3];
	outfilename  = argv[4];
    }
}

/****************************************************************************************/

static char *skipblanks(char *s)
{
    while ((*s == ' ') || (*s == '\t')) s++;
    
    return s;
}

/****************************************************************************************/

static char *skipword(char *s)
{
    while((*s != ' ') &&
    	  (*s != '\t') &&
	  (*s != '\0') &&
	  (*s != '\n'))
    {
    	s++;
    }
    
    return s;
}

/****************************************************************************************/

static char *checkquotes(char *s)
{
    char *s2;
    static char s3[256];
    BOOL escaped = 0;
    int i = 0;

    if (*s != '"')
    {
    	s2 = skipword(s);
	*s2 = '\0';
	
    	return s;
    }
    
    s++;

    
    while ((i < 256) && (s3[i] = *s))
    {
        if (!escaped)
        {
            if (*s == '\\')
            {
                escaped = !escaped;
                i--;
            }
            else if (*s == '"')
                break;
        }
        else
            escaped = !escaped;
        i++;
        s++;
    }
    s3[i] = '\0';
    s = s3;

    return s;
}

/****************************************************************************************/

#define KEYWORD_STRING      0
#define KEYWORD_INTEGER     1
#define KEYWORD_STRINGARRAY 2
#define KEYWORD_CYCLE	    3

#define MAX_ARRAY_SIZE      200

/****************************************************************************************/

struct cycle
{
    char *keyword;
    LONG value;
};

struct cycle typecycles[] =
{
    {"DISK" 	, 1},
    {"DRAWER"	, 2},
    {"TOOL" 	, 3},
    {"PROJECT"	, 4},
    {"GARBAGE"	, 5},
    {"DEVICE"	, 6},
    {"KICK" 	, 7},
    {"APPICON"	, 8},
};

struct cycle showcycles[] =
{
    {"DEFAULT"	, 0},
    {"ICONS"   	, 1},
    {"ALL"  	, 2},
    {NULL   	, 0}
    
};

struct cycle showascycles[] =
{
    {"DEFAULT"	, 0},
    {"ICON"   	, 1},
    {"TEXT_NAME", 2},
    {"TEXT_DATE", 3},
    {"TEXT_SIZE", 4},
    {"TEXT_TYPE", 5},	
    {NULL   	, 0}
    
};


/****************************************************************************************/

struct keyword
{
    WORD  type;
    char *keyword;
    APTR  store;
    APTR  extra;
}
keywordtable[] =
{
    {KEYWORD_STRING 	, "DEFAULTTOOL"     , &defaulttooloption    , NULL  	    	},
    {KEYWORD_STRING 	, "DRAWERDATA"      , &drawerdataoption     , NULL  	    	},
    {KEYWORD_CYCLE 	, "TYPE"    	    , &typeoption   	    , typecycles    	},
    {KEYWORD_STRINGARRAY, "TOOLTYPE"	    , &tooltypesoption	    , NULL  	    	},
    {KEYWORD_INTEGER	, "STACK"   	    , &stackoption  	    , NULL  	    	},
    {KEYWORD_INTEGER	, "ICONLEFTPOS"     , &iconleftoption	    , NULL  	    	},
    {KEYWORD_INTEGER	, "ICONTOPPOS"	    , &icontopoption	    , NULL  	    	},
    {KEYWORD_INTEGER	, "DRAWERLEFTPOS"   , &drawerleftoption     , NULL  	    	},
    {KEYWORD_INTEGER	, "DRAWERTOPPOS"    , &drawertopoption	    , NULL  	    	},
    {KEYWORD_INTEGER	, "DRAWERWIDTH"     , &drawerwidthoption    , NULL  	    	},
    {KEYWORD_INTEGER	, "DRAWERHEIGHT"    , &drawerheightoption   , NULL  	    	},
    {KEYWORD_INTEGER	, "DRAWERVIEWLEFT"  , &drawervleftoption    , NULL  	    	},
    {KEYWORD_INTEGER	, "DRAWERVIEWTOP"   , &drawervtopoption     , NULL  	    	},
    {KEYWORD_CYCLE	, "DRAWERSHOW"      , &drawershowoption     , showcycles   	},
    {KEYWORD_CYCLE	, "DRAWERSHOWAS"    , &drawershowoption     , showascycles   	},
    {KEYWORD_INTEGER	, "TRANSPARENT"     , &transparentoption    , NULL  	    	},
    {0	    	    	, NULL	    	    , NULL  	    	    	    	    	}
};


/****************************************************************************************/

static void handleoption(char *keyword, char *keyvalue)
{
    struct keyword *kw;
    struct cycle *cy;

    D(printf("Keyword %s, value %s\n", keyword, keyvalue));

    for(kw = keywordtable; kw->keyword; kw++)
    {
    	if (strcasecmp(kw->keyword, keyword) == 0)
	{	
    	    switch(kw->type)
	    {
		case KEYWORD_STRING:
	    	    *(char **)kw->store = strdup(keyvalue);
		    if (!(*(char **)kw->store)) cleanup("Out of memory!", 1);
		    break;

		case KEYWORD_INTEGER:
    	    	    *(LONG *)kw->store = strtol(keyvalue, 0, 0);
		    break;

		case KEYWORD_CYCLE:
	    	    for(cy = (struct cycle *)kw->extra; cy->keyword; cy++)
		    {
			if (strcasecmp(keyvalue, cy->keyword) == 0)
			{
			    *(LONG *)kw->store = cy->value;
			    break;
			}
		    }
		    break;

    	    	case KEYWORD_STRINGARRAY:
		    if (!(*(char ***)kw->store))
		    {
		    	*(char ***)kw->store = (char **)malloc(MAX_ARRAY_SIZE * sizeof(char *));
			if (!(*(char ***)kw->store)) cleanup("Out of memory!", 1);
			
			memset(*(char ***)kw->store, 0, MAX_ARRAY_SIZE * sizeof(char *));
		    }

		    {
		    	char *dupvalue;
			char **strarray = *(char ***)kw->store;
			WORD i = 0;
			
			dupvalue = strdup(keyvalue);
			if (!dupvalue) cleanup("Out of memory!", 1);
			
			while(*strarray)
			{
			    strarray++;
			    i++;
			}
			
			if (i >= MAX_ARRAY_SIZE - 1) cleanup("Array overflow!", 1);
			
			*strarray = dupvalue;
		    }
		    
	    } /* switch(kw->type) */

	    break;
	    
	} /* if (strcasecmp(kw->keyword, keyword) == 0) */
	
    } /* for(kw = keywordtable; kw->keyword; kw++) */
    
}

/****************************************************************************************/

static void parseline(char *s)
{
    char *keyword;
    char *keyvalue = NULL;
    
    s = skipblanks(s);
    
    if (*s == '#') return;
    if (*s == ';') return;
    
    keyword = s;
    
    s = skipword(s);
    if (*s == '\0') return;
    
    *s = '\0';
    s = skipblanks(s + 1);
    
    if (*s == '=') s = skipblanks(s + 1);
    if (*s == '\0') return;

    keyvalue = checkquotes(s);
    
    handleoption(keyword, keyvalue);
}

/****************************************************************************************/

static void parseiconsource(void)
{
    char s[256];
    
    infile = fopen(infilename, "r");
    if (infile)
    {
	while(fgets(s, sizeof(s), infile))
	{
	    D(printf("Read line: %s\n", s));
    	    parseline(s);
	}

	fclose(infile);
	infile = 0;
    }
}

/****************************************************************************************/

static void showoptions(void)
{
    char **strarray;
    
    printf("image1: %s\n", image1option ? image1option : "(NULL)");
    printf("image2: %s\n", image2option ? image2option : "(NULL)");
    printf("type: %ld\n", typeoption);
    
    strarray = tooltypesoption;
    if (strarray)
    {
    	printf("tooltypes:\n");
	while(*strarray)
	{
	    printf(" %s\n", *strarray++);
	}
    }
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

static void openimage(struct ILBMImage *img)
{    
    file = fopen(filename, "rb");
    if (!file) cleanup("Can't open file!", 1);
    
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    
    if (filesize < 12) cleanup("Bad file size!", 1);
    
    //fprintf(stderr, "Filesize is %d\n", filesize);
    
    fseek(file, 0, SEEK_SET);
    
    filebuffer = malloc(filesize + 10);
    if (!filebuffer) cleanup("Memory allocation for file buffer failed!", 1);
    
    if (fread(filebuffer, 1, filesize, file) != filesize)
    	cleanup("Error reading file!", 1);
    
    fclose(file); file = NULL;    
}

/****************************************************************************************/

static void checkimage(struct ILBMImage *img)
{
    static UBYTE pngsig[8] = {137, 80, 78, 71, 13, 10, 26, 10};

    ULONG id;
    ULONG size;
   
    if (dualpng) {
        img->png = dualpngstart;
        img->png_size = filesize - (dualpngstart - filebuffer);
        return;
    }

    if (memcmp(filebuffer, pngsig, 8) == 0)
    {
    	is_png = 1;
    	img->png = filebuffer;
	
	/* search for second image */
	for
	(
	    dualpngstart = filebuffer + 8 ;
	    dualpngstart < filebuffer + filesize - 8 ;
	    dualpngstart++
	)
	{
	    if (memcmp(dualpngstart, pngsig, 8) == 0)
	    {
		dualpng = 1;
		break;
	    }
	}
	if (dualpng)
	    img->png_size = dualpngstart - filebuffer;
        else
            img->png_size = filesize;
    }
    else if (is_png == 0)
    {   
	id = getlong();
	if (id != ID_FORM) cleanup("File is not an IFF file!", 1);

	size = getlong();
	if (size != filesize - 8) cleanup("File is IFF, but has bad size in IFF header!", 1);

	id = getlong();
	if (id != ID_ILBM) cleanup("File is IFF, but not of type ILBM!", 1);
    }
    else if (is_png == 1)
    {
    	cleanup("Second image must be a PNG image, too!", 1);
    }

}

/****************************************************************************************/

static void my_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    png_bytep *pdata = png_get_io_ptr(png_ptr);

    if (*pdata-filebuffer >= filesize)
        png_error(png_ptr, "Read past end of file");

    memcpy(data, *pdata, length);
    *pdata += length;
}

static UBYTE findcolor(struct Palette *pal, ULONG r, ULONG g, ULONG b, BOOL notrans);

static void loadpng(struct ILBMImage *img, struct Palette *pal)
{
    png_structp png_ptr;
    png_infop info_ptr, end_info;
    png_bytep fpos, *row_pointers;
    UBYTE *chunkrow;
    UWORD width, height;
    int x, y;

    if (png_sig_cmp(img->png, 0, img->png_size) != 0)
        cleanup("I thought it was a PNG, but I was wrong.", 1);

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                        0,      /* error ptr */
                        0,
                        0);
    if (!png_ptr)
        cleanup("png_create_read_struct() failed", 1);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        cleanup("png_create_info_struct() failed", 1);
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        cleanup("png_create_info_struct() failed", 1);
    }

    fpos = img->png;
    png_set_read_fn(png_ptr, &fpos, my_read_fn);

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        cleanup("png read failed", 1);
    }

    /* Read the PNG as RGBA */
    png_set_add_alpha(png_ptr, 255, PNG_FILLER_AFTER); 
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_STRIP_16, NULL);

    row_pointers = png_get_rows(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);

    img->bmh.bmh_Width  = width;
    img->bmh.bmh_Height = height;
    img->bmh.bmh_Left   = 0;
    img->bmh.bmh_Top    = 0;
    img->bmh.bmh_Depth  = 8;
    img->bmh.bmh_Masking = MSK_HASTRANS;
    img->bmh.bmh_Compression = CMP_NONE;
    img->bmh.bmh_Pad         = 0;
    img->bmh.bmh_XAspect     = 1;
    img->bmh.bmh_YAspect     = 1;
    img->bmh.bmh_PageWidth   = 320;
    img->bmh.bmh_PageHeight  = 200;

    img->planarbmh;
    img->bpr = ((img->bmh.bmh_Width + 15) & ~15) / 8;
    img->totdepth = 0;

    /* Transform the RGBA data into chunky */
    img->cmapentries = pal->numentries;
    memcpy(img->rgb, pal->rgb, sizeof(img->rgb[0])*pal->numentries);

    img->chunkybuffer = malloc(width * height * sizeof(UBYTE));
    if (!img->chunkybuffer)
        cleanup("Can't allocate the chunky buffer", 1);

    chunkrow = img->chunkybuffer;
    img->argb_size = width * height * sizeof(UBYTE) * 4;
    img->argb = malloc(img->argb_size);
    if (!img->argb)
        cleanup("Can't allocate the ARGB buffer", 1);

    for (y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        UBYTE *ap = img->argb + (width * sizeof(UBYTE) * 4) * y;
        for (x = 0; x < width; x++, row += 4, ap += 4, chunkrow++) {
            UBYTE r,g,b;

            ap[0] = row[3];
            ap[1] = row[0];
            ap[2] = row[1];
            ap[3] = row[2];

            /* Opacity of 0? Use the transparency color */
            if (row[3] == 0) {
                if (transparentoption < 0)
                        transparentoption = 0;
                *chunkrow = transparentoption;
            } else {
                r = row[0];
                g = row[1];
                b = row[2];

                *chunkrow = findcolor(pal, r, g, b, TRUE);
            }
        }
    }

    img->bmh.bmh_Transparent = transparentoption;

    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
}


/****************************************************************************************/

static void scanimage(struct ILBMImage *img)
{
    WORD i;
    
    have_bmhd = 0;
    have_cmap = 0;
    have_body = 0;
    
    if (img->png) {
        /* Load the PNG, using the Scalos 16-color pallette,
         * into the Chunky buffer.
         */
        loadpng(img, &scalos16colpal);
        return;
    }

    for(;;)
    {
    	ULONG id;
	ULONG size;
	
	id   = getlong();
	size = getlong();

	//fprintf(stderr, "Chunk: %c%c%c%c  Size: %d\n", id >> 24, id >> 16, id >> 8, id, size);
		
	switch(id)
	{
	    case ID_BMHD:
	    	if (size != 20) cleanup("Bad BMHD chunk size!", 1);
		
	    	img->bmh.bmh_Width 	    = getword();
		img->bmh.bmh_Height      = getword();
		img->bmh.bmh_Left 	    = (WORD)getword();
		img->bmh.bmh_Top 	    = (WORD)getword();
		img->bmh.bmh_Depth 	    = getbyte();
		img->bmh.bmh_Masking     = getbyte();
		img->bmh.bmh_Compression = getbyte();
		img->bmh.bmh_Pad 	    = getbyte();
		img->bmh.bmh_Transparent = getword();
		img->bmh.bmh_XAspect     = getbyte();
		img->bmh.bmh_YAspect     = getbyte();
		img->bmh.bmh_PageWidth   = (WORD)getword();
		img->bmh.bmh_PageHeight  = (WORD)getword();
		
		if (img->bmh.bmh_Depth > 8) cleanup("ILBM file has too many colors!", 1);
		if ((img->bmh.bmh_Compression != CMP_NONE) && (img->bmh.bmh_Compression != CMP_BYTERUN1)) cleanup("Compression method unsupported!", 1);
		
		have_bmhd = 1;
		
		img->totdepth = img->bmh.bmh_Depth + ((img->bmh.bmh_Masking == MSK_HASMASK) ? 1 : 0);
		
		img->bpr = ((img->bmh.bmh_Width + 15) & ~15) / 8;
		
		/*fprintf(stderr, "BMHD: %d x %d x %d (%d)\n", img->bmh.bmh_Width,
		    	    	    	    	    	     img->bmh.bmh_Height,
							     img->bmh.bmh_Depth,
							     img->totdepth);*/
		img->planarbmh = img->bmh;
		break;
	
	    case ID_CMAP:   
	    	if (!have_bmhd) cleanup("CMAP chunk before BMHD chunk (or no BMHD chunk at all!", 1);
		
		img->cmapentries = size / 3;
		if (size & 1) size++;
		
		if ((img->cmapentries < 2) || (img->cmapentries > 256)) cleanup("CMAP chunk has bad number of entries!", 1);
		
		for(i = 0; i < img->cmapentries; i++)
		{
		    img->rgb[i][0] = getbyte();
		    img->rgb[i][1] = getbyte();
		    img->rgb[i][2] = getbyte();
		    size -= 3;
		}
	    	
		skipbytes(size);
		
		have_cmap = 1;
		
		break;
	
	    case ID_BODY:
	    	if (!have_bmhd) cleanup("BODY chunk before BMHD chunk (or no BMHD chunk at all!", 1);
		body = &filebuffer[filepos];
		bodysize = size;
		
		if (img->bmh.bmh_Compression == CMP_NONE)
		{
		    LONG shouldbesize = img->totdepth * img->bpr * img->bmh.bmh_Height;
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
	    LONG mask   = 0x80 >> (x & 7);
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

static void c2p(unsigned char *source, unsigned char *dest, LONG width, LONG height, LONG planes)
{
    LONG alignedwidth, x, y, p, bpr, bpl;
    
    alignedwidth = (width + 15) & ~15;
    bpr = alignedwidth / 8;
    bpl = bpr * planes;
    
    for(y = 0; y < height; y++)
    {
	for(x = 0; x < width; x++)
	{
	    LONG mask   = 0x80 >> (x & 7);
	    LONG offset = x / 8;
	    unsigned char chunkypix = source[x];

	    for(p = 0; p < planes; p++)
	    {
		if (chunkypix & (1 << p))
	    	    dest[p * bpr + offset] |= mask;
		else
		    dest[p * bpr + offset] &= ~mask;
	    }
	}

    	source += width;
    	dest += bpl;
    }
 
}

/****************************************************************************************/

static void convertbody(struct ILBMImage *img)
{
    LONG unpackedsize = img->bpr * img->bmh.bmh_Height * img->totdepth;
    
    if (is_png)
        return;

    img->planarbuffer = malloc(unpackedsize);
    if (!img->planarbuffer) cleanup("Memory allocation for planar buffer failed!", 1);
    
    if (img->bmh.bmh_Compression == CMP_NONE)
    {
    	memcpy(img->planarbuffer, body, unpackedsize);
    }
    else
    {
    	unpack_byterun1(body, img->planarbuffer, unpackedsize);
    }
    
    img->chunkybuffer = malloc(img->bmh.bmh_Width * img->bmh.bmh_Height);
    if (!img->chunkybuffer) cleanup("Memory allocation for chunky buffer failed!", 1);
    
    p2c(img->planarbuffer,
    	img->chunkybuffer,
	img->bmh.bmh_Width,
	img->bmh.bmh_Height,
	img->totdepth,
	img->bmh.bmh_Depth,
	img->bmh.bmh_Width);
}

/****************************************************************************************/

static UBYTE findcolor(struct Palette *pal, ULONG r, ULONG g, ULONG b, BOOL notrans)
{
    ULONG dist, bestdist = 0xFFFFFFFF;
    UBYTE i, besti = 0;
    
    for(i = 0; i < pal->numentries; i++)
    {
    	LONG r1, g1, b1, r2, g2, b2, dr, dg, db;

    	if (notrans && i == transparentoption)
    	    continue;
	
	r1 = (LONG)r;
	g1 = (LONG)g;
	b1 = (LONG)b;
	
	r2 = (LONG)pal->rgb[i][0];
	g2 = (LONG)pal->rgb[i][1];
    	b2 = (LONG)pal->rgb[i][2];
	
	dr = r1 - r2;
	dg = g1 - g2;
	db = b1 - b2;
	
	dist = (dr * dr) + (dg * dg) + (db * db);
	if (dist < bestdist)
	{
	    bestdist = dist;
	    besti = i;
	}
	
    }
    
    return besti;
}

/****************************************************************************************/

static void remapplanar(struct ILBMImage *img, struct Palette *pal)
{
    UBYTE *remapbuffer;
    LONG i, x, y, highestcol = 0, newdepth = 0;
    
    remapbuffer = malloc(img->bmh.bmh_Width * img->bmh.bmh_Height);
    if (!remapbuffer) cleanup("Error allocating remap buffer!", 1);
    
    for(i = 0; i < img->cmapentries; i++)
    {
    	img->remaptable[i] = findcolor(pal, img->rgb[i][0], img->rgb[i][1], img->rgb[i][2], FALSE);
    }
    
    for(i = 0; i < img->bmh.bmh_Width * img->bmh.bmh_Height; i++)
    {
    	remapbuffer[i] = img->remaptable[img->chunkybuffer[i]];

    	if (remapbuffer[i] > highestcol)
	    highestcol = remapbuffer[i];
    }
    
    for(i = highestcol; i; i >>= 1) newdepth++;
    if (newdepth == 0) newdepth = 1;
    
    if (newdepth > img->totdepth)
    {
        if (img->planarbuffer)
            free(img->planarbuffer);
	
	img->planarbuffer = malloc(img->bpr * img->bmh.bmh_Height * newdepth);
	if (!img->planarbuffer)
	{
	    free(remapbuffer);
	    cleanup("Error re-allocating planar buffer!", 1);	
    	}
    }
  
    img->planarbmh.bmh_Width = img->bmh.bmh_Width;
    img->planarbmh.bmh_Height= img->bmh.bmh_Height;
    img->planarbmh.bmh_Depth = newdepth;
    
    memset(img->planarbuffer, 0, img->bpr * img->bmh.bmh_Height * newdepth);
    
    c2p(remapbuffer, img->planarbuffer, img->bmh.bmh_Width, img->bmh.bmh_Height, newdepth);
    
    free(remapbuffer);
}

/****************************************************************************************/

static void loadimage(char *name, struct ILBMImage *img)
{
    freeimage(img);
    
    filename = name;
    
    openimage(img);
    checkimage(img);
    scanimage(img);
    convertbody(img);
}

/****************************************************************************************/

struct diskobject
{
    UBYTE do_magic[2];
    UBYTE do_version[2];
    UBYTE do_gadget_nextgadget[4];
    UBYTE do_gadget_leftedge[2];
    UBYTE do_gadget_topedge[2];
    UBYTE do_gadget_width[2];
    UBYTE do_gadget_height[2];
    UBYTE do_gadget_flags[2];
    UBYTE do_gadget_activation[2];
    UBYTE do_gadget_gadgettype[2];
    UBYTE do_gadget_gadgetrender[4];
    UBYTE do_gadget_selectrender[4];
    UBYTE do_gadget_gadgettext[4];
    UBYTE do_gadget_mutualexclude[4];
    UBYTE do_gadget_specialinfo[4];
    UBYTE do_gadget_gadgetid[2];
    UBYTE do_gadget_userdata[4];
    UBYTE do_type;
    UBYTE do_pad;
    UBYTE do_defaulttool[4];
    UBYTE do_tooltypes[4];
    UBYTE do_currentx[4];
    UBYTE do_currenty[4];
    UBYTE do_drawerdata[4];
    UBYTE do_toolwindow[4];
    UBYTE do_stacksize[4];
};

/****************************************************************************************/

struct olddrawerdata
{
    UBYTE dd_newwindow_leftedge[2];
    UBYTE dd_newwindow_topedge[2];
    UBYTE dd_newwindow_width[2];
    UBYTE dd_newwindow_height[2];
    UBYTE dd_newwindow_detailpen;
    UBYTE dd_newwindow_blockpen;
    UBYTE dd_newwindow_idcmpflags[4];
    UBYTE dd_newwindow_flags[4];
    UBYTE dd_newwindow_firstgadget[4];
    UBYTE dd_newwindow_checkmark[4];
    UBYTE dd_newwindow_title[4];
    UBYTE dd_newwindow_screen[4];
    UBYTE dd_newwindow_bitmap[4];
    UBYTE dd_newwindow_minwidth[2];
    UBYTE dd_newwindow_minheight[2];
    UBYTE dd_newwindow_maxwidth[2];
    UBYTE dd_newwindow_maxheight[2];
    UBYTE dd_newwindow_type[2];
    UBYTE dd_currentx[4];
    UBYTE dd_currenty[4];
};

/****************************************************************************************/

struct newdrawerdata
{
    UBYTE dd_flags[4];
    UBYTE dd_viewmodes[2];
};

/****************************************************************************************/

struct image
{
    UBYTE leftedge[2];
    UBYTE topedge[2];
    UBYTE width[2];
    UBYTE height[2];
    UBYTE depth[2];
    UBYTE imagedata[4];
    UBYTE planepick;
    UBYTE planeonoff;
    UBYTE nextimage[4];
};

/****************************************************************************************/

#define SET_BYTE(field,value) \
    ACT_STRUCT.field = value
    
#define SET_WORD(field, value) \
    ACT_STRUCT.field[0] = ((value) >> 8) & 0xFF; \
    ACT_STRUCT.field[1] = (value) & 0xFF;

#define SET_LONG(field,value) \
    ACT_STRUCT.field[0] = ((value) >> 24) & 0xFF; \
    ACT_STRUCT.field[1] = ((value) >> 16) & 0xFF; \
    ACT_STRUCT.field[2] = ((value) >> 8) & 0xFF; \
    ACT_STRUCT.field[3] = (value) & 0xFF;

#define BOOL_YES 0x2A2A2A2A
#define BOOL_NO  0x00000000

static void writediskobject(void)
{
    struct diskobject dobj;

    if (typeoption == 2)        /* DRAWER */
    {
        drawerdataoption = "YES";
    }
   
#define ACT_STRUCT dobj
    
    SET_WORD(do_magic, 0xE310);
    SET_WORD(do_version, 1);
    SET_LONG(do_gadget_nextgadget, 0);
    SET_WORD(do_gadget_leftedge, 0);
    SET_WORD(do_gadget_topedge, 0);
    SET_WORD(do_gadget_width, img1.bmh.bmh_Width);
    SET_WORD(do_gadget_height, img1.bmh.bmh_Height);
    
    if (image2option)
    {
    	/* GFLG_GADGHIMAGE + GFLG_GADGIMAGE */
    	SET_WORD(do_gadget_flags, 4 + 2);
    }
    else
    {
    	/* GFLG_GADGIMAGE */
    	SET_WORD(do_gadget_flags, 4);
    }
    
    SET_WORD(do_gadget_activation, 1);
    SET_WORD(do_gadget_gadgettype, 1);
    SET_LONG(do_gadget_gadgetrender, BOOL_YES);
    
    if (image2option)
    {
    	SET_LONG(do_gadget_selectrender, BOOL_YES);
    }
    else
    {
    	SET_LONG(do_gadget_selectrender, BOOL_NO);
    }
    
    SET_LONG(do_gadget_gadgettext, 0);
    SET_LONG(do_gadget_mutualexclude, 0);
    SET_LONG(do_gadget_specialinfo, 0);
    SET_WORD(do_gadget_gadgetid, 0);
    SET_LONG(do_gadget_userdata, 1); /* full drawer data */
    
    SET_BYTE(do_type, typeoption);
    SET_BYTE(do_pad, 0);
    
    if (defaulttooloption)
    {
    	SET_LONG(do_defaulttool, BOOL_YES);
    }
    else
    {
    	SET_LONG(do_defaulttool, BOOL_NO);
    }
    
    if (tooltypesoption)
    {
    	SET_LONG(do_tooltypes, BOOL_YES);
    }
    else
    {
    	SET_LONG(do_tooltypes, BOOL_NO);
    }
    
    SET_LONG(do_currentx, iconleftoption);
    SET_LONG(do_currenty, icontopoption);

    if (drawerdataoption)
    {
    	SET_LONG(do_drawerdata, BOOL_YES);
    }
    else
    {
    	SET_LONG(do_drawerdata, BOOL_NO);
    }
    
    
    SET_LONG(do_toolwindow, 0);
    SET_LONG(do_stacksize, stackoption);
    
    if (fwrite(&dobj, 1, sizeof(dobj), outfile) != sizeof(dobj))
    {
    	cleanup("Error writing diskobject structure to outfile!", 1);
    }
}

/****************************************************************************************/

static void writeolddrawerdata(void)
{
    struct olddrawerdata dd;
    
    if (!drawerdataoption) return;
    
#undef ACT_STRUCT
#define ACT_STRUCT dd

    SET_WORD(dd_newwindow_leftedge, drawerleftoption);
    SET_WORD(dd_newwindow_topedge, drawertopoption);
    SET_WORD(dd_newwindow_width, drawerwidthoption);
    SET_WORD(dd_newwindow_height, drawerheightoption);
    SET_BYTE(dd_newwindow_detailpen, 255);
    SET_BYTE(dd_newwindow_blockpen, 255);
    SET_LONG(dd_newwindow_idcmpflags, 0);
    SET_LONG(dd_newwindow_flags, 0x240027f);
    SET_LONG(dd_newwindow_firstgadget, 0);
    SET_LONG(dd_newwindow_checkmark, 0);
    SET_LONG(dd_newwindow_title, 0);
    SET_LONG(dd_newwindow_screen, 0);
    SET_LONG(dd_newwindow_bitmap, 0);
    SET_WORD(dd_newwindow_minwidth, 90);
    SET_WORD(dd_newwindow_minheight, 40);
    SET_WORD(dd_newwindow_maxwidth, 65535);
    SET_WORD(dd_newwindow_maxheight, 65535);
    SET_WORD(dd_newwindow_type, 1);
    SET_LONG(dd_currentx, drawervleftoption);
    SET_LONG(dd_currenty, drawervtopoption);

    if (fwrite(&dd, 1, sizeof(dd), outfile) != sizeof(dd))
    {
    	cleanup("Error writing olddrawerdata structure to outfile!", 1);
    }

}


/****************************************************************************************/

static void writenewdrawerdata(void)
{
    struct newdrawerdata dd;
    
    if (!drawerdataoption) return;
    
#undef ACT_STRUCT
#define ACT_STRUCT dd

    SET_LONG(dd_flags, drawershowoption);
    SET_WORD(dd_viewmodes, drawershowasoption);

    if (fwrite(&dd, 1, sizeof(dd), outfile) != sizeof(dd))
    {
    	cleanup("Error writing newdrawerdata structure to outfile!", 1);
    }

}

/****************************************************************************************/

static void writeword(WORD l)
{
    UBYTE f[2];

    f[1] = (l >> 8) & 0xFF;
    f[2] = l & 0xFF;
 
    if (fwrite(f, 1, 2, outfile) != 2)
    {
    	cleanup("Error writing word value!", 1);
    }
    
}

/****************************************************************************************/

static void writelong(LONG l)
{
    UBYTE f[4];

    f[0] = (l >> 24) & 0xFF;
    f[1] = (l >> 16) & 0xFF;
    f[2] = (l >> 8) & 0xFF;
    f[3] = l & 0xFF;
 
    if (fwrite(f, 1, 4, outfile) != 4)
    {
    	cleanup("Error writing long value!", 1);
    }
    
}

/****************************************************************************************/

static void writenormalstring(char *s)
{
    int len = strlen(s) + 1;

    if (fwrite(s, 1, len, outfile) != len)
    {
    	cleanup("Error writing string!", 1);
    }
    
}

/****************************************************************************************/

static void writestring(char *s)
{
    int len = strlen(s) + 1;

    D(printf("String: \"%s\", length %d\n", s, len));

    writelong(len);
        
    if (fwrite(s, 1, len, outfile) != len)
    {
    	cleanup("Error writing string!", 1);
    }
    
}

/****************************************************************************************/

static void writeimage(struct ILBMImage *img)
{
    struct image i;
    LONG d, y;
    
#undef ACT_STRUCT
#define ACT_STRUCT i

    SET_WORD(leftedge, 0);
    SET_WORD(topedge, 0);
    SET_WORD(width, img->planarbmh.bmh_Width);
    SET_WORD(height, img->planarbmh.bmh_Height);
    SET_WORD(depth, img->planarbmh.bmh_Depth);
    SET_LONG(imagedata, BOOL_YES);
    SET_BYTE(planepick, (1 << img->planarbmh.bmh_Depth) - 1);
    SET_BYTE(planeonoff, 0);
    SET_LONG(nextimage, 0);

    if (fwrite(&i, 1, sizeof(i), outfile) != sizeof(i))
    {
    	cleanup("Error writing image structure to outfile!", 1);
    }
 
    for(d = 0; d < img->planarbmh.bmh_Depth; d++)
    {
    	UBYTE *dat = img->planarbuffer + img->bpr * d;
	
	for(y = 0; y < img->planarbmh.bmh_Height; y++)
	{
	    if(fwrite(dat, 1, img->bpr, outfile) != img->bpr)
	    {
     	    	cleanup("Error writing image data to outfile!", 1);
	    }
	    dat += (img->planarbmh.bmh_Depth * img->bpr);
	}
    }
          
}

/****************************************************************************************/

struct facechunk
{
    UBYTE fc_width;
    UBYTE fc_height;
    UBYTE fc_flags;
    UBYTE fc_aspect;
    UBYTE fc_maxpalettebytes[2];
};

/****************************************************************************************/

struct imagchunk
{
    UBYTE ic_transparentcolour;
    UBYTE ic_numcolours;
    UBYTE ic_flags;
    UBYTE ic_imageformat;
    UBYTE ic_paletteformat;
    UBYTE ic_depth;
    UBYTE ic_numimagebytes[2];
    UBYTE ic_numpalettebytes[2];
};

/****************************************************************************************/

static LONG writefacechunk(void)
{
    struct facechunk fc;
    LONG palbytes;
    
#undef ACT_STRUCT
#define ACT_STRUCT fc

    writelong(ID_FACE);
    writelong(sizeof(struct facechunk));
    
    SET_BYTE(fc_width, img1.bmh.bmh_Width - 1);
    SET_BYTE(fc_height, img1.bmh.bmh_Height - 1);
    SET_BYTE(fc_flags, 0);
    SET_BYTE(fc_aspect, 0); // 0x11);
    
    palbytes = (img1.cmapentries > img2.cmapentries) ? img1.cmapentries : img2.cmapentries;
    palbytes = palbytes * 3;
    
    SET_WORD(fc_maxpalettebytes, palbytes - 1);
    
    if (fwrite(&fc, 1, sizeof(fc), outfile) != sizeof(fc))
    {
    	cleanup("Error writing face chunk!", 1);
    }
    
    return sizeof(struct facechunk) + 8;
}

/****************************************************************************************/

/* createrle() based on ModifyIcon source by Dirk Stöcker */

/****************************************************************************************/

static char * createrle(unsigned long depth, unsigned char *dtype, LONG *dsize, unsigned long size,
    	    	        unsigned char *src)
{
  int i, j, k;
  unsigned long bitbuf, numbits;
  unsigned char *buf;
  long ressize, numcopy, numequal;

  buf = malloc(size * 2);
  if (!buf) return NULL;
  
  numcopy = 0;
  numequal = 1;
  bitbuf = 0;
  numbits = 0;
  ressize = 0;
  k = 0; /* the really output pointer */
  for(i = 1; numequal || numcopy;)
  {
    if(i < size && numequal && (src[i-1] == src[i]))
    {
      ++numequal; ++i;
    }
    else if(i < size && numequal*depth <= 16)
    {
      numcopy += numequal; numequal = 1; ++i;
    }
    else
    {
      /* care for end case, where it maybe better to join the two */
      if(i == size && numcopy + numequal <= 128 && (numequal-1)*depth <= 8)
      {
        numcopy += numequal; numequal = 0;
      }
      if(numcopy)
      {
        if((j = numcopy) > 128) j = 128;
        bitbuf = (bitbuf<<8) | (j-1);
        numcopy -= j;
      }
      else
      {
        if((j = numequal) > 128) j = 128;
        bitbuf = (bitbuf<<8) | (256-(j-1));
        numequal -= j;
        k += j-1;
        j = 1;
      }
      buf[ressize++] = (bitbuf >> numbits);
      while(j--)
      {
        numbits += depth;
        bitbuf = (bitbuf<<depth) | src[k++];
        if(numbits >= 8)
        {
          numbits -= 8;
          buf[ressize++] = (bitbuf >> numbits);
        }
      }
      if(i < size && !numcopy && !numequal)
      {
        numequal = 1; ++i;
      }
    }
  }
  if(numbits)
    buf[ressize++] = bitbuf << (8-numbits);

  if(ressize > size) /* no RLE */
  {
    ressize = size;
    *dtype = 0;
    for(i = 0; i < size; ++i)
      buf[i]= src[i];
  }
  else
    *dtype = 1;
    
  *dsize = ressize;
  
  return buf;
}

/****************************************************************************************/

static LONG writeimagchunk(struct ILBMImage *img)
{
    struct imagchunk ic;  
    LONG imagsize;
    UBYTE skippalette = 0;
    UBYTE *pal, *gfx;
    LONG palsize, gfxsize;
    UBYTE palpacked, gfxpacked;
    
    imagsize = sizeof(struct imagchunk);
    
    /* if this is second image check whether palette is identical to
       the one of first image */
       
    if (img == &img2)
    {
    	if (img1.cmapentries == img2.cmapentries)
	{
	    WORD i;
	    
	    for (i = 0; i < img1.cmapentries; i++)
	    {
	    	if (img1.rgb[i][0] != img2.rgb[i][0]) break;
	    	if (img1.rgb[i][1] != img2.rgb[i][1]) break;
	    	if (img1.rgb[i][2] != img2.rgb[i][2]) break;		
	    }
	    
	    if (i == img1.cmapentries) skippalette = 1;
	}
    }
    
    if (!skippalette)
    {
    	pal = createrle(8,
	    	    	&palpacked,
			&palsize,
			img->cmapentries * 3,
			(unsigned char *)img->rgb);
			
    	imagsize += palsize;
    }
    
    gfx = createrle(img->bmh.bmh_Depth,
    	    	    &gfxpacked,
		    &gfxsize,
		    img->bmh.bmh_Width * img->bmh.bmh_Height,
		    img->chunkybuffer);
		
    imagsize += gfxsize;
    
#undef ACT_STRUCT
#define ACT_STRUCT ic

    SET_BYTE(ic_transparentcolour, transparentoption);
    if (skippalette)
    {
    	SET_BYTE(ic_numcolours, 0);
	SET_BYTE(ic_flags, (transparentoption != -1) ? 1 : 0); /* 1 = HasTransparentColour */
	SET_BYTE(ic_paletteformat, 0);
	SET_WORD(ic_numpalettebytes, 0);
    }
    else
    {
    	SET_BYTE(ic_numcolours, img->cmapentries - 1);
	SET_BYTE(ic_flags, (transparentoption != -1) ? 3 : 2); /* 2 = HasPalette */
	SET_BYTE(ic_paletteformat, palpacked);
    	SET_WORD(ic_numpalettebytes, palsize - 1);
    }
    
    SET_BYTE(ic_imageformat, gfxpacked);
    SET_BYTE(ic_depth, img->bmh.bmh_Depth);
    SET_WORD(ic_numimagebytes, gfxsize - 1);
    
    writelong(ID_IMAG);
    writelong(imagsize);
    
    if (fwrite(&ic, 1, sizeof(ic), outfile) != sizeof(ic))
    {
    	cleanup("Error writing imag chunk!", 1);
    }

    if (fwrite(gfx, 1, gfxsize, outfile) != gfxsize)
    {
    	cleanup("Error write gfx data in imag chunk!", 1);
    }
    
    if (!skippalette)
    {
	if (fwrite(pal, 1, palsize, outfile) != palsize)
	{
    	    cleanup("Error write palette data in imag chunk!", 1);
	}    	
    }
    
    if (imagsize & 1)
    {
    	UBYTE dummy = 0;
	
	if (fwrite(&dummy, 1, 1, outfile) != 1)
	{
	    cleanup("Error writing imag chunk!", 1);
	}
	
	imagsize++;
    }
    
    return imagsize + 8;
}

/****************************************************************************************/
static LONG writeargb(APTR argb, ULONG argb_size)
{
    LONG formsize = 10;
    struct ARGB35_Header {
        ULONG ztype;    /* Always 1 */
        ULONG zsize;    /* Compressed size, or -1 */
        UWORD resv;     /* Always 0 */
    } ahdr;
    Bytef *zdest;
    uLongf zsize, size;
    int err;
    
    zsize = size = argb_size;

    zdest = malloc(zsize);
    if (!zdest)
        return 0;

    err = compress(zdest, &zsize, argb, size);
    if (err != Z_OK) {
        free(zdest);
        return 0;
    }

    writelong(ID_ARGB);
    formsize = 10 + zsize;
    if (formsize & 1)
        formsize++;
    writelong(formsize);
    writelong(1);
    writelong(zsize);
    writeword(0);
    fwrite(zdest, 1, zsize, outfile);
    if (zsize & 1) {
        char c = 0;
        fwrite(&c, 1, 1, outfile);
    }

    free(zdest);

    return 8 + formsize;
}

/****************************************************************************************/

static void write35data(void)
{
    LONG formsize = 4;
    LONG formsizeseek;

    if (nosaveIFF)
        return;

    writelong(ID_FORM);
    formsizeseek = ftell(outfile);
    writelong(0x12345678);
    writelong(ID_ICON);
    
    formsize += writefacechunk();
    formsize += writeimagchunk(&img1);
    if (image2option) formsize += writeimagchunk(&img2);

    if (!nosaveARGB && img1.argb) {
        formsize += writeargb(img1.argb, img1.argb_size);
    }

    if (!nosaveARGB && img2.argb) {
        formsize += writeargb(img2.argb, img2.argb_size);
    }

    if (!nosavePNG && img1.png) {
        writelong(ID_PNG);
        writelong(img1.png_size);
        fwrite(img1.png, 1, img1.png_size, outfile);
        if (img1.png_size & 1) {
            char c = 0;
            fwrite(&c, 1, 1, outfile);
            img1.png_size++;
        }
        formsize += 8 + img1.png_size;
    }

    if (!nosavePNG && img2.png) {
        writelong(ID_PNG);
        writelong(img2.png_size);
        fwrite(img2.png, 1, img2.png_size, outfile);
        if (img2.png_size & 1) {
            char c = 0;
            fwrite(&c, 1, 1, outfile);
            img2.png_size++;
        }
        formsize += 8 + img2.png_size;
    }
    
    fseek(outfile, formsizeseek, SEEK_SET);
    writelong(formsize);
}

/****************************************************************************************/

static void writeicon(void)
{
    struct diskobject dobj;

    outfile = fopen(outfilename, "wb");
    if (!outfile) cleanup("Can't open output file for writing!", 1);
    
    writediskobject();
    writeolddrawerdata();
    writeimage(&img1);
    
    if (image2option) writeimage(&img2);
    
    if (defaulttooloption) writestring(defaulttooloption);
    
    if (tooltypesoption)
    {
    	char **strarray;	
    	LONG numtooltypes = 0;
	
	for(strarray = tooltypesoption; *strarray; strarray++, numtooltypes++);
	
	writelong((numtooltypes + 1) * 4);

	for(strarray = tooltypesoption; *strarray; strarray++)
	{
	    writestring(*strarray);
	}
	
    }
    
    /* toolwindow would have to be saved in between here if there is any */
    
    writenewdrawerdata();
    
    write35data();
    
}

/****************************************************************************************/

static void remapicon(void)
{
    remapplanar(&img1, &std4colpal);
    if (image2option) remapplanar(&img2, &std4colpal);       
}

/****************************************************************************************/

int main(int argc, char **argv)
{
    getarguments(argc, argv);
    parseiconsource();
    loadimage(image1option, &img1);
    if (image2option) loadimage(image2option, &img2);
    remapicon();
    writeicon();
    
    cleanup(0, 0);
}

/****************************************************************************************/
