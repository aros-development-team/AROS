/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993, David Koblas.  (koblas@netcom.com)    | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */


#include       "pnm.h"

#define        MAXCOLORMAPSIZE         256

#define        TRUE    1
#define        FALSE   0

#define CM_RED         0
#define CM_GREEN       1
#define CM_BLUE                2

#define        MAX_LWZ_BITS            12

#define INTERLACE              0x40
#define LOCALCOLORMAP  0x80
#define BitSet(byte, bit)      (((byte) & (bit)) == (bit))

#define        ReadOK(file,buffer,len) (fread(buffer, len, 1, file) != 0)

#define LM_to_uint(a,b)                        (((b)<<8)|(a))

static struct {
       unsigned int    Width;
       unsigned int    Height;
       unsigned char   ColorMap[3][MAXCOLORMAPSIZE];
       unsigned int    BitPixel;
       unsigned int    ColorResolution;
       unsigned int    Background;
       unsigned int    AspectRatio;
       /*
       **
       */
       int             GrayScale;
} GifScreen;

static struct {
       int     transparent;
       int     delayTime;
       int     inputFlag;
       int     disposal;
} Gif89 = { -1, -1, -1, 0 };

pixel  *Image = NULL;
int    verbose;
int    showComment;

static char    usage[] = "[-verbose] [-comments] [-image N] [GIFfile]";

static void ReadGIF ARGS(( FILE        *fd, int imageNumber ));
static int ReadColorMap ARGS(( FILE *fd, int number, unsigned char buffer[3][MAXCOLORMAPSIZE], int *flag ));
static int DoExtension ARGS(( FILE *fd, int label ));
static int GetDataBlock ARGS(( FILE *fd, unsigned char  *buf ));
static int GetCode ARGS(( FILE *fd, int code_size, int flag ));
static int LWZReadByte ARGS(( FILE *fd, int flag, int input_code_size ));
static void ReadImage ARGS(( FILE *fd, int len, int height, unsigned char cmap[3][MAXCOLORMAPSIZE], int gray, int interlace, int ignore ));

int
main(argc,argv)
int    argc;
char   **argv;
{
       int             argn;
       FILE            *in;
       int             imageNumber;

       pnm_init(&argc, argv);

       argn = 1;
       imageNumber = 1;
       verbose = FALSE;
       showComment = FALSE;

       while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0') {
               if (pm_keymatch(argv[argn], "-verbose", 2)) {
                       verbose = TRUE;
                       showComment = TRUE;
               } else if (pm_keymatch(argv[argn], "-comments", 2)) {
                       showComment = TRUE;
               } else if (pm_keymatch(argv[argn], "-image", 2)) {
                       ++argn;
                       if (argn == argc || sscanf(argv[argn], "%d", &imageNumber) != 1)
                               pm_usage(usage);
               } else {
                       pm_usage(usage);
               }
               ++argn;
       }
       if (argn != argc) {
               in = pm_openr(argv[argn]);
               ++argn;
       } else {
               in = stdin;
       }

       if (argn != argc)
               pm_usage(usage);

       ReadGIF(in, imageNumber);
       pm_close(in);
       pm_close(stdout);
       exit(0);
}

static void
ReadGIF(fd, imageNumber)
FILE   *fd;
int    imageNumber;
{
       unsigned char   buf[16];
       unsigned char   c;
       unsigned char   localColorMap[3][MAXCOLORMAPSIZE];
       int             grayScale;
       int             useGlobalColormap;
       int             bitPixel;
       int             imageCount = 0;
       char            version[4];

       if (! ReadOK(fd,buf,6))
               pm_error("error reading magic number" );

       if (strncmp((char *)buf,"GIF",3) != 0)
               pm_error("not a GIF file" );

       strncpy(version, (char *)buf + 3, 3);
       version[3] = '\0';

       if ((strcmp(version, "87a") != 0) && (strcmp(version, "89a") != 0))
               pm_error("bad version number, not '87a' or '89a'" );

       if (! ReadOK(fd,buf,7))
               pm_error("failed to read screen descriptor" );

       GifScreen.Width           = LM_to_uint(buf[0],buf[1]);
       GifScreen.Height          = LM_to_uint(buf[2],buf[3]);
       GifScreen.BitPixel        = 2<<(buf[4]&0x07);
       GifScreen.ColorResolution = (((buf[4]&0x70)>>3)+1);
       GifScreen.Background      = buf[5];
       GifScreen.AspectRatio     = buf[6];

       if (BitSet(buf[4], LOCALCOLORMAP)) {    /* Global Colormap */
               if (ReadColorMap(fd,GifScreen.BitPixel,GifScreen.ColorMap,
                                       &GifScreen.GrayScale))
                       pm_error("error reading global colormap" );
       }

       if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49) {
               float   r;
               r = ( (float) GifScreen.AspectRatio + 15.0 ) / 64.0;
               pm_message("warning - non-square pixels; to fix do a 'pnmscale -%cscale %g'",
                   r < 1.0 ? 'x' : 'y',
                   r < 1.0 ? 1.0 / r : r );
       }

       for (;;) {
               if (! ReadOK(fd,&c,1))
                       pm_error("EOF / read error on image data" );

               if (c == ';') {         /* GIF terminator */
                       if (imageCount < imageNumber)
                               pm_error("only %d image%s found in file",
                                        imageCount, imageCount>1?"s":"" );
                       return;
               }

               if (c == '!') {         /* Extension */
                       if (! ReadOK(fd,&c,1))
                               pm_error("OF / read error on extention function code");
                       DoExtension(fd, c);
                       continue;
               }

               if (c != ',') {         /* Not a valid start character */
                       pm_message("bogus character 0x%02x, ignoring", (int) c );
                       continue;
               }

               ++imageCount;

               if (! ReadOK(fd,buf,9))
                       pm_error("couldn't read left/top/width/height");

               useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

               bitPixel = 1<<((buf[8]&0x07)+1);

               if (! useGlobalColormap) {
                       if (ReadColorMap(fd, bitPixel, localColorMap, &grayScale))
                               pm_error("error reading local colormap" );
                       ReadImage(fd, LM_to_uint(buf[4],buf[5]),
                                 LM_to_uint(buf[6],buf[7]), 
                                 localColorMap, grayScale,
                                 BitSet(buf[8], INTERLACE), imageCount != imageNumber);
               } else {
                       ReadImage(fd, LM_to_uint(buf[4],buf[5]),
                                 LM_to_uint(buf[6],buf[7]), 
                                 GifScreen.ColorMap, GifScreen.GrayScale,
                                 BitSet(buf[8], INTERLACE), imageCount != imageNumber);
               }

       }
}

static int
ReadColorMap(fd,number,buffer,pbm_format)
FILE           *fd;
int            number;
unsigned char  buffer[3][MAXCOLORMAPSIZE];
int            *pbm_format;
{
       int             i;
       unsigned char   rgb[3];
       int             flag;

       flag = TRUE;

       for (i = 0; i < number; ++i) {
               if (! ReadOK(fd, rgb, sizeof(rgb)))
                       pm_error("bad colormap" );

               buffer[CM_RED][i] = rgb[0] ;
               buffer[CM_GREEN][i] = rgb[1] ;
               buffer[CM_BLUE][i] = rgb[2] ;

               flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
       }

       if (flag)
               *pbm_format = (number == 2) ? PBM_TYPE : PGM_TYPE;
       else
               *pbm_format = PPM_TYPE;

       return FALSE;
}

static int
DoExtension(fd, label)
FILE   *fd;
int    label;
{
       static char     buf[256];
       char            *str;

       switch (label) {
       case 0x01:              /* Plain Text Extension */
               str = "Plain Text Extension";
#ifdef notdef
               if (GetDataBlock(fd, (unsigned char*) buf) == 0)
                       ;

               lpos   = LM_to_uint(buf[0], buf[1]);
               tpos   = LM_to_uint(buf[2], buf[3]);
               width  = LM_to_uint(buf[4], buf[5]);
               height = LM_to_uint(buf[6], buf[7]);
               cellw  = buf[8];
               cellh  = buf[9];
               foreground = buf[10];
               background = buf[11];

               while (GetDataBlock(fd, (unsigned char*) buf) != 0) {
                       PPM_ASSIGN(image[ypos][xpos],
                                       cmap[CM_RED][v],
                                       cmap[CM_GREEN][v],
                                       cmap[CM_BLUE][v]);
                       ++index;
               }

               return FALSE;
#else
               break;
#endif
       case 0xff:              /* Application Extension */
               str = "Application Extension";
               break;
       case 0xfe:              /* Comment Extension */
               str = "Comment Extension";
               while (GetDataBlock(fd, (unsigned char*) buf) != 0) {
                       if (showComment)
                               pm_message("gif comment: %s", buf );
               }
               return FALSE;
       case 0xf9:              /* Graphic Control Extension */
               str = "Graphic Control Extension";
               (void) GetDataBlock(fd, (unsigned char*) buf);
               Gif89.disposal    = (buf[0] >> 2) & 0x7;
               Gif89.inputFlag   = (buf[0] >> 1) & 0x1;
               Gif89.delayTime   = LM_to_uint(buf[1],buf[2]);
               if ((buf[0] & 0x1) != 0)
                       Gif89.transparent = buf[3];

               while (GetDataBlock(fd, (unsigned char*) buf) != 0)
                       ;
               return FALSE;
       default:
               str = buf;
               sprintf(buf, "UNKNOWN (0x%02x)", label);
               break;
       }

       pm_message("got a '%s' extension", str );

       while (GetDataBlock(fd, (unsigned char*) buf) != 0)
               ;

       return FALSE;
}

int    ZeroDataBlock = FALSE;

static int
GetDataBlock(fd, buf)
FILE           *fd;
unsigned char  *buf;
{
       unsigned char   count;

       if (! ReadOK(fd,&count,1)) {
               pm_message("error in getting DataBlock size" );
               return -1;
       }

       ZeroDataBlock = count == 0;

       if ((count != 0) && (! ReadOK(fd, buf, count))) {
               pm_message("error in reading DataBlock" );
               return -1;
       }

       return count;
}

static int
GetCode(fd, code_size, flag)
FILE   *fd;
int    code_size;
int    flag;
{
       static unsigned char    buf[280];
       static int              curbit, lastbit, done, last_byte;
       int                     i, j, ret;
       unsigned char           count;

       if (flag) {
               curbit = 0;
               lastbit = 0;
               done = FALSE;
               return 0;
       }

       if ( (curbit+code_size) >= lastbit) {
               if (done) {
                       if (curbit >= lastbit)
                               pm_error("ran off the end of my bits" );
                       return -1;
               }
               buf[0] = buf[last_byte-2];
               buf[1] = buf[last_byte-1];

               if ((count = GetDataBlock(fd, &buf[2])) == 0)
                       done = TRUE;

               last_byte = 2 + count;
               curbit = (curbit - lastbit) + 16;
               lastbit = (2+count)*8 ;
       }

       ret = 0;
       for (i = curbit, j = 0; j < code_size; ++i, ++j)
               ret |= ((buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;

       curbit += code_size;

       return ret;
}

static int
LWZReadByte(fd, flag, input_code_size)
FILE   *fd;
int    flag;
int    input_code_size;
{
       static int      fresh = FALSE;
       int             code, incode;
       static int      code_size, set_code_size;
       static int      max_code, max_code_size;
       static int      firstcode, oldcode;
       static int      clear_code, end_code;
       static int      table[2][(1<< MAX_LWZ_BITS)];
       static int      stack[(1<<(MAX_LWZ_BITS))*2], *sp;
       register int    i;

       if (flag) {
               set_code_size = input_code_size;
               code_size = set_code_size+1;
               clear_code = 1 << set_code_size ;
               end_code = clear_code + 1;
               max_code_size = 2*clear_code;
               max_code = clear_code+2;

               GetCode(fd, 0, TRUE);
               
               fresh = TRUE;

               for (i = 0; i < clear_code; ++i) {
                       table[0][i] = 0;
                       table[1][i] = i;
               }
               for (; i < (1<<MAX_LWZ_BITS); ++i)
                       table[0][i] = table[1][0] = 0;

               sp = stack;

               return 0;
       } else if (fresh) {
               fresh = FALSE;
               do {
                       firstcode = oldcode =
                               GetCode(fd, code_size, FALSE);
               } while (firstcode == clear_code);
               return firstcode;
       }

       if (sp > stack)
               return *--sp;

       while ((code = GetCode(fd, code_size, FALSE)) >= 0) {
               if (code == clear_code) {
                       for (i = 0; i < clear_code; ++i) {
                               table[0][i] = 0;
                               table[1][i] = i;
                       }
                       for (; i < (1<<MAX_LWZ_BITS); ++i)
                               table[0][i] = table[1][i] = 0;
                       code_size = set_code_size+1;
                       max_code_size = 2*clear_code;
                       max_code = clear_code+2;
                       sp = stack;
                       firstcode = oldcode =
                                       GetCode(fd, code_size, FALSE);
                       return firstcode;
               } else if (code == end_code) {
                       int             count;
                       unsigned char   buf[260];

                       if (ZeroDataBlock)
                               return -2;

                       while ((count = GetDataBlock(fd, buf)) > 0)
                               ;

                       if (count != 0)
                               pm_message("missing EOD in data stream (common occurence)");
                       return -2;
               }

               incode = code;

               if (code >= max_code) {
                       *sp++ = firstcode;
                       code = oldcode;
               }

               while (code >= clear_code) {
                       *sp++ = table[1][code];
                       if (code == table[0][code])
                               pm_error("circular table entry BIG ERROR");
                       code = table[0][code];
               }

               *sp++ = firstcode = table[1][code];

               if ((code = max_code) <(1<<MAX_LWZ_BITS)) {
                       table[0][code] = oldcode;
                       table[1][code] = firstcode;
                       ++max_code;
                       if ((max_code >= max_code_size) &&
                               (max_code_size < (1<<MAX_LWZ_BITS))) {
                               max_code_size *= 2;
                               ++code_size;
                       }
               }

               oldcode = incode;

               if (sp > stack)
                       return *--sp;
       }
       return code;
}

static void
ReadImage(fd, len, height, cmap, pbm_format, interlace, ignore)
FILE   *fd;
int    len, height;
unsigned char  cmap[3][MAXCOLORMAPSIZE];
int    pbm_format, interlace, ignore;
{
       unsigned char   c;      
       int             v;
       int             xpos = 0, ypos = 0, pass = 0;
       pixel           **image;

       /*
       **  Initialize the Compression routines
       */
       if (! ReadOK(fd,&c,1))
               pm_error("EOF / read error on image data" );

       if (LWZReadByte(fd, TRUE, c) < 0)
               pm_error("error reading image" );

       /*
       **  If this is an "uninteresting picture" ignore it.
       */
       if (ignore) {
               if (verbose)
                       pm_message("skipping image..." );

               while (LWZReadByte(fd, FALSE, c) >= 0)
                       ;
               return;
       }

       if ((image = pnm_allocarray(len, height)) == NULL)
               pm_error("couldn't alloc space for image" );

       if (verbose)
               pm_message("reading %d by %d%s GIF image",
                       len, height, interlace ? " interlaced" : "" );

       while ((v = LWZReadByte(fd,FALSE,c)) >= 0 ) {
               PPM_ASSIGN(image[ypos][xpos], cmap[CM_RED][v],
                                       cmap[CM_GREEN][v], cmap[CM_BLUE][v]);

               ++xpos;
               if (xpos == len) {
                       xpos = 0;
                       if (interlace) {
                               switch (pass) {
                               case 0:
                               case 1:
                                       ypos += 8; break;
                               case 2:
                                       ypos += 4; break;
                               case 3:
                                       ypos += 2; break;
                               }

                               if (ypos >= height) {
                                       ++pass;
                                       switch (pass) {
                                       case 1:
                                               ypos = 4; break;
                                       case 2:
                                               ypos = 2; break;
                                       case 3:
                                               ypos = 1; break;
                                       default:
                                               goto fini;
                                       }
                               }
                       } else {
                               ++ypos;
                       }
               }
               if (ypos >= height)
                       break;
       }

fini:
       if (LWZReadByte(fd,FALSE,c)>=0)
               pm_message("too much input data, ignoring extra...");

       if (verbose)
               pm_message("writing a %s file",
                               pbm_format == PBM_TYPE ? "PBM" :
                               pbm_format == PGM_TYPE ? "PGM" :
                               pbm_format == PPM_TYPE ? "PPM" :
                               "UNKNOWN!");

       pnm_writepnm(stdout, image, len, height, (pixval) 255, pbm_format, 0 );
}
