/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2002 by                                                       */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftbench: bench some common FreeType call paths                          */
/*                                                                          */
/****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H
#include FT_CACHE_CHARMAP_H
#include FT_CACHE_IMAGE_H
#include FT_CACHE_SMALL_BITMAPS_H


#ifdef UNIX
#include <sys/time.h>
#endif

typedef int
(*bench_t)(FT_UInt idx,
           FT_UInt charcode);

typedef struct charmap_t_
{
  FT_UInt charcode;
  FT_UInt index;
} charmap_t;


/*
 * Globals
 */

#define CACHE_SIZE 1024
#define BENCH_TIME 2.0f

FT_Library        lib;
FT_Face           face;
FTC_Manager       cache_man;
FTC_CMapCache     cmap_cache;
FTC_CMapDescRec   cmap_desc;
FTC_Image_Cache   image_cache;
FTC_SBitCache     sbit_cache;
FTC_ImageTypeRec  font_type;
charmap_t*        cmap = NULL;
double            bench_time = BENCH_TIME;


/*
 * Dummy face requester (the face object is already loaded)
 */

FT_Error
face_requester (FTC_FaceID face_id,
                FT_Library library,
                FT_Pointer request_data,\
                FT_Face* aface)
{
  FT_UNUSED( face_id );
  FT_UNUSED( library );
  FT_UNUSED( request_data );

  *aface = face;
  return 0;
}


/*
 * Bench code
 */

double
get_time(void)
{
#ifdef UNIX
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec + (double)tv.tv_usec / 1E6;
#else
  /* clock() has an awful precision (~10ms) under Linux 2.4 + glibc 2.2 */
  return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}


void
get_cmap(void)
{
  FT_ULong charcode;
  FT_UInt  gindex;
  int i;

  if (cmap)
    return; /* Already available */

  cmap = (charmap_t*)calloc(face->num_glyphs, sizeof(charmap_t));

  if (face->charmap)
  {
    i = 0;
    charcode = FT_Get_First_Char(face, &gindex);

    /* certain fonts contain a broken charmap that will map character codes */
    /* to out-of-bounds glyph indices. Take care of that here !!            */
    /*                                                                      */
    while ( gindex && i < face->num_glyphs )
    {
      cmap[i].index = gindex;
      cmap[i].charcode = charcode;
      i++;
      charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }
  }
  else
    /* no charmap, do an identity mapping */
    for (i = 0; i < face->num_glyphs; i++)
    {
      cmap[i].index = i;
      cmap[i].charcode = i;
      i++;
    }
}


void
bench(bench_t bench_func,
      const char* title,
      int max)
{
  int      i, n, done;
  double   t0, delta;

  printf("%-30s : ", title);
  fflush(stdout);

  get_cmap();

  n = 0;
  done = 0;
  t0 = get_time();
  do
  {
    for (i = 0; i < face->num_glyphs; i++)
      if (!(*bench_func)(cmap[i].index, cmap[i].charcode))
        done++;
    n++;
    delta = get_time() - t0;
  }
  while ((!max || n < max) && delta < bench_time);

  printf("%5.3f us/op\n", delta * 1E6 / (double)done);
}


/*
 * Various tests
 */

int
load_test(FT_UInt idx,
          FT_UInt charcode)
{
  FT_UNUSED( charcode );
  return FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT);
}


int
fetch_test(FT_UInt idx,
           FT_UInt charcode)
{
  FT_Glyph glyph;

  FT_UNUSED( charcode );

  return
    FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT) ||
    FT_Get_Glyph(face->glyph, &glyph);
}


int
cbox_test(FT_UInt idx,
          FT_UInt charcode)
{
  FT_BBox  bbox;
  FT_Glyph glyph;

  FT_UNUSED( charcode );

  if (FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT) ||
      FT_Get_Glyph(face->glyph, &glyph))
    return 1;
  FT_Glyph_Get_CBox(glyph, ft_glyph_bbox_pixels, &bbox);
  return 0;
}


int
cmap_test(FT_UInt idx,
          FT_UInt charcode)
{
  FT_UNUSED( idx );

  return !FT_Get_Char_Index(face, charcode);
}


int
cmap_cache_test(FT_UInt idx,
                FT_UInt charcode)
{
  FT_UNUSED( idx );

  return !FTC_CMapCache_Lookup(cmap_cache, &cmap_desc, charcode);
}


int
image_cache_test(FT_UInt idx,
                 FT_UInt charcode)
{
  FT_Glyph glyph;

  FT_UNUSED( charcode );

  return FTC_ImageCache_Lookup(image_cache, &font_type, idx, &glyph, NULL);
}


int
sbit_cache_test(FT_UInt idx,
                FT_UInt charcode)
{
  FTC_SBit glyph;

  FT_UNUSED( charcode );

  return FTC_SBitCache_Lookup(sbit_cache, &font_type, idx, &glyph, NULL);
}


/*
 * main
 */

void usage(void)
{
  fprintf( stderr,
    "ftbench: bench some common FreeType paths\n"
    "-----------------------------------------\n\n"
    "Usage: ftbench [options] fontname\n\n"
    "options:\n" );
  fprintf( stderr,
  "   -m : max cache size in kByte (default is %d)\n", CACHE_SIZE );
  fprintf( stderr,
  "   -t : max time per bench in seconds (default is %.0f)\n", BENCH_TIME );
  fprintf( stderr,
  "   -p : preload font file in memory\n"
  "   -b tests : perform choosen tests (default is all)\n"
  "      a : Load\n"
  "      b : Load + Get_Glyph\n"
  "      c : Load + Get_Glyph + Get_CBox\n"
  "      d : Get_Char_Index\n"
  "      e : CMap cache\n"
  "      f : Outline cache\n"
  "      g : Bitmap cache\n"
  "      h : SBit cache\n" );

  exit( 1 );
}


#define TEST(x) (!tests || strchr(tests, x))


int
main(int argc,
     char** argv)
{
  FT_ULong max_bytes = CACHE_SIZE * 1024;
  char* tests = NULL;
  int size;
  int preload = 0;
  FT_Byte*  memory_file = NULL;
  long      memory_size;

  while (argc > 1 && argv[1][0] == '-')
  {
    switch (argv[1][1])
    {
    case 'm':
      argc--;
      argv++;
      if (argc < 2 ||
          sscanf(argv[1], "%ld", &max_bytes) != 1)
        usage();
      max_bytes *= 1024;
      break;

    case 'p':
      preload = 1;
      break;

    case 't':
      argc--;
      argv++;
      if (argc < 2 ||
          sscanf(argv[1], "%lf", &bench_time) != 1)
        usage();
      break;

    case 'b':
      argc--;
      argv++;
      if (argc < 2)
        usage();
      tests = argv[1];
      break;

    default:
      fprintf(stderr, "Unknown argument `%s'\n\n", argv[1]);
      usage();
      break;
    }

    argc--;
    argv++;
  }

  if ( argc != 2 )
    usage();

  if (FT_Init_FreeType(&lib))
  {
    fprintf( stderr, "could not initialize font library\n" );
    return 1;
  }

  if ( preload )
  {
    FILE*  file = fopen( argv[1], "rb" );
    if ( file == NULL )
    {
      fprintf( stderr, "couldn't find or open `%s'\n", argv[1] );
      return 1;
    }
    fseek( file, 0, SEEK_END );
    memory_size = ftell( file );
    fseek( file, 0, SEEK_SET );

    memory_file = malloc( memory_size );
    if ( memory_file == NULL )
    {
      fprintf( stderr, "couldn't allocate memory to pre-load font file\n" );
      return 1;
    }

    fread( memory_file, 1, memory_size, file );
    if ( FT_New_Memory_Face( lib, memory_file, memory_size, 0, &face ) )
    {
      fprintf( stderr, "couldn't load font resource\n" );
      return 1;
    }
  }
  else if ( FT_New_Face(lib, argv[1], 0, &face) )
  {
    fprintf( stderr, "couldn't load font resource\n");
    return 1;
  }

  if (FT_IS_SCALABLE(face))
  {
    /* We're not benchmarking the scanline renderer, pick any size */
    size = 10;
    FT_Set_Pixel_Sizes(face, size, size);
  }
  else
    size = face->available_sizes[0].width;

  if (TEST('a')) bench( load_test,  "Load", 0);
  if (TEST('b')) bench( fetch_test, "Load + Get_Glyph", 0);
  if (TEST('c')) bench( cbox_test,  "Load + Get_Glyph + Get_CBox", 0);

  cmap_desc.face_id    = (void*)1;
  cmap_desc.type       = FTC_CMAP_BY_INDEX;
  cmap_desc.u.encoding = 0;
  if (TEST('d') &&
      face->charmap) /* some fonts (eg. windings) don't have a charmap */
  {
    bench( cmap_test,  "Get_Char_Index", 0);
  }

  if (!FTC_Manager_New(lib, 0, 0, max_bytes, face_requester, NULL, &cache_man))
  {
    if (TEST('e') &&
        face->charmap && !FTC_CMapCache_New(cache_man, &cmap_cache))
    {
      bench( cmap_cache_test,  "CMap cache (1st run)", 1);
      bench( cmap_cache_test,  "CMap cache", 0);
    }

    font_type.font.face_id    = (void*)1;
    font_type.font.pix_width  = (short) size;
    font_type.font.pix_height = (short) size;

    if (!FTC_ImageCache_New(cache_man, &image_cache))
    {
      if (TEST('f'))
      {
        font_type.flags = FT_LOAD_NO_BITMAP;
        bench( image_cache_test,  "Outline cache (1st run)", 1);
        bench( image_cache_test,  "Outline cache", 0);
      }

      if (TEST('g'))
      {
        font_type.flags = FT_LOAD_RENDER;
        bench( image_cache_test,  "Bitmap cache (1st run)", 1);
        bench( image_cache_test,  "Bitmap cache", 0);
      }
    }

    if (TEST('h') &&
        !FTC_SBitCache_New(cache_man, &sbit_cache))
    {
      font_type.flags = FT_LOAD_DEFAULT;
      bench( sbit_cache_test,  "SBit cache (1st run)", 1);
      bench( sbit_cache_test,  "SBit cache", 0);
    }
  }

  if (cmap)
    free (cmap);

  if (cache_man)
    FTC_Manager_Done(cache_man);

  FT_Done_FreeType(lib);

  return 0;
}


/* End */
