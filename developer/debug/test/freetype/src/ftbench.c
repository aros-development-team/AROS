/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 2002-2018 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftbench: bench some common FreeType call paths                          */
/*                                                                          */
/****************************************************************************/


#ifndef  _GNU_SOURCE
#define  _GNU_SOURCE /* we want to use extensions to `time.h' if available */
#endif

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
#include FT_SYNTHESIS_H
#include FT_ADVANCES_H
#include FT_OUTLINE_H
#include FT_BBOX_H
#include FT_MODULE_H
#include FT_DRIVER_H
#include FT_LCD_FILTER_H

#ifdef UNIX
#include <unistd.h>
#else
#include "mlgetopt.h"
#endif

#include "common.h"


  typedef struct  btimer_t_ {
    double  t0;
    double  total;

  } btimer_t;


  typedef int
  (*bcall_t)( btimer_t*  timer,
              FT_Face    face,
              void*      user_data );


  typedef struct  btest_t_ {
    const char*  title;
    bcall_t      bench;
    int          cache_first;
    void*        user_data;

  } btest_t;


  typedef struct  bcharset_t_
  {
    FT_Int     size;
    FT_ULong*  code;

  } bcharset_t;


  static FT_Error
  get_face( FT_Face*  face );


  /*
   * Globals
   */

#define CACHE_SIZE  1024
#define BENCH_TIME  2.0
#define FACE_SIZE   10


  static FT_Library        lib;
  static FTC_Manager       cache_man;
  static FTC_CMapCache     cmap_cache;
  static FTC_ImageCache    image_cache;
  static FTC_SBitCache     sbit_cache;
  static FTC_ImageTypeRec  font_type;


  enum {
    FT_BENCH_LOAD_GLYPH,
    FT_BENCH_LOAD_ADVANCES,
    FT_BENCH_RENDER,
    FT_BENCH_GET_GLYPH,
    FT_BENCH_GET_CBOX,
    FT_BENCH_CMAP,
    FT_BENCH_CMAP_ITER,
    FT_BENCH_NEW_FACE,
    FT_BENCH_EMBOLDEN,
    FT_BENCH_GET_BBOX,
    FT_BENCH_NEW_FACE_AND_LOAD_GLYPH,
    N_FT_BENCH
  };


  static const char*  bench_desc[] =
  {
    "load a glyph        (FT_Load_Glyph)",
    "load advance widths (FT_Get_Advances)",
    "render a glyph      (FT_Render_Glyph)",
    "load a glyph        (FT_Get_Glyph)",
    "get glyph cbox      (FT_Glyph_Get_CBox)",
    "get glyph indices   (FT_Get_Char_Index)",
    "iterate CMap        (FT_Get_{First,Next}_Char)",
    "open a new face     (FT_New_Face)",
    "embolden            (FT_GlyphSlot_Embolden)",
    "get glyph bbox      (FT_Outline_Get_BBox)",

    "open face and load glyph",
    NULL
  };


  static int    preload;
  static char*  filename;

  static unsigned int  first_index = 0U;
  static unsigned int  last_index  = ~0U;

  static FT_Render_Mode  render_mode = FT_RENDER_MODE_NORMAL;
  static FT_Int32        load_flags  = FT_LOAD_DEFAULT;

  static unsigned int  tt_interpreter_versions[3];
  static int           num_tt_interpreter_versions;
  static unsigned int  dflt_tt_interpreter_version;

  static unsigned int  ps_hinting_engines[2];
  static int           num_ps_hinting_engines;
  static unsigned int  dflt_ps_hinting_engine;

  static char  ps_hinting_engine_names[2][10] = { "freetype",
                                                  "adobe" };


  /*
   * Dummy face requester (the face object is already loaded)
   */

  static FT_Error
  face_requester( FTC_FaceID  face_id,
                  FT_Library  library,
                  FT_Pointer  request_data,
                  FT_Face*    aface )
  {
    FT_UNUSED( face_id );
    FT_UNUSED( library );

    *aface = (FT_Face)request_data;

    return FT_Err_Ok;
  }


  /*
   * timer in milliseconds
   */

  static double
  get_time( void )
  {
#if defined _POSIX_TIMERS && _POSIX_TIMERS > 0
    struct timespec  tv;


#ifdef _POSIX_CPUTIME
    clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &tv );
#else
    clock_gettime( CLOCK_REALTIME, &tv );
#endif /* _POSIX_CPUTIME */

    return 1E6 * (double)tv.tv_sec + 1E-3 * (double)tv.tv_nsec;
#else
    /* clock() accuracy has improved since glibc 2.18 */
    return 1E6 * (double)clock() / (double)CLOCKS_PER_SEC;
#endif /* _POSIX_TIMERS */
  }

#define TIMER_START( timer )  ( timer )->t0 = get_time()
#define TIMER_STOP( timer )   ( timer )->total += get_time() - ( timer )->t0
#define TIMER_GET( timer )    ( timer )->total
#define TIMER_RESET( timer )  ( timer )->total = 0


  /*
   * Bench code
   */

  static void
  benchmark( FT_Face   face,
             btest_t*  test,
             int       max_iter,
             double    max_time )
  {
    int       n, done;
    btimer_t  timer, elapsed;


    if ( test->cache_first )
    {
      if ( !cache_man )
      {
        printf( "  %-25s no cache manager\n", test->title );

        return;
      }

      TIMER_RESET( &timer );
      test->bench( &timer, face, test->user_data );
    }

    printf( "  %-25s ", test->title );
    fflush( stdout );

    TIMER_RESET( &timer );
    TIMER_RESET( &elapsed );

    for ( n = 0, done = 0; !max_iter || n < max_iter; n++ )
    {
      TIMER_START( &elapsed );

      done += test->bench( &timer, face, test->user_data );

      TIMER_STOP( &elapsed );

      if ( TIMER_GET( &elapsed ) > 1E6 * max_time )
        break;
    }

    if ( done )
      printf( "%5.3f us/op\n", TIMER_GET( &timer ) / (double)done );
    else
      printf( "no error-free calls\n" );
  }


  /*
   * Various tests
   */

  static int
  test_load( btimer_t*  timer,
             FT_Face    face,
             void*      user_data )
  {
    unsigned int  i;
    int           done = 0;

    FT_UNUSED( user_data );


    TIMER_START( timer );

    for ( i = first_index; i <= last_index; i++ )
    {
      if ( !FT_Load_Glyph( face, i, load_flags ) )
        done++;
    }

    TIMER_STOP( timer );

    return done;
  }


  static int
  test_load_advances( btimer_t*  timer,
                      FT_Face    face,
                      void*      user_data )
  {
    int        done = 0;
    FT_Fixed*  advances;
    FT_ULong   flags = *((FT_ULong*)user_data);


    advances = (FT_Fixed *)calloc( sizeof ( FT_Fixed ),
                                   (size_t)face->num_glyphs );

    TIMER_START( timer );

    FT_Get_Advances( face,
                     first_index,
                     last_index - first_index + 1,
                     (FT_Int32)flags,
                     advances );
    done += (int)( last_index - first_index ) + 1;

    TIMER_STOP( timer );

    free( advances );

    return done;
  }


  static int
  test_render( btimer_t*  timer,
               FT_Face    face,
               void*      user_data )
  {
    unsigned int  i;
    int           done = 0;

    FT_UNUSED( user_data );


    for ( i = first_index; i <= last_index; i++ )
    {
      if ( FT_Load_Glyph( face, i, load_flags ) )
        continue;

      TIMER_START( timer );
      if ( !FT_Render_Glyph( face->glyph, render_mode ) )
        done++;
      TIMER_STOP( timer );
    }

    return done;
  }


  static int
  test_embolden( btimer_t*  timer,
                 FT_Face    face,
                 void*      user_data )
  {
    unsigned int  i;
    int           done = 0;

    FT_UNUSED( user_data );


    for ( i = first_index; i <= last_index; i++ )
    {
      if ( FT_Load_Glyph( face, i, load_flags ) )
        continue;

      TIMER_START( timer );
      FT_GlyphSlot_Embolden( face->glyph );
      done++;
      TIMER_STOP( timer );
    }

    return done;
  }


  static int
  test_get_glyph( btimer_t*  timer,
                  FT_Face    face,
                  void*      user_data )
  {
    FT_Glyph      glyph;
    unsigned int  i;
    int           done = 0;

    FT_UNUSED( user_data );


    for ( i = first_index; i <= last_index; i++ )
    {
      if ( FT_Load_Glyph( face, i, load_flags ) )
        continue;

      TIMER_START( timer );
      if ( !FT_Get_Glyph( face->glyph, &glyph ) )
      {
        FT_Done_Glyph( glyph );
        done++;
      }
      TIMER_STOP( timer );
    }

    return done;
  }


  static int
  test_get_cbox( btimer_t*  timer,
                 FT_Face    face,
                 void*      user_data )
  {
    FT_Glyph      glyph;
    FT_BBox       bbox;
    unsigned int  i;
    int           done = 0;

    FT_UNUSED( user_data );


    for ( i = first_index; i <= last_index; i++ )
    {
      if ( FT_Load_Glyph( face, i, load_flags ) )
        continue;

      if ( FT_Get_Glyph( face->glyph, &glyph ) )
        continue;

      TIMER_START( timer );
      FT_Glyph_Get_CBox( glyph, FT_GLYPH_BBOX_PIXELS, &bbox );
      TIMER_STOP( timer );

      FT_Done_Glyph( glyph );
      done++;
    }

    return done;
  }


  static int
  test_get_bbox( btimer_t*  timer,
                 FT_Face    face,
                 void*      user_data )
  {
    FT_BBox       bbox;
    unsigned int  i;
    int           done  = 0;
    FT_Matrix     rot30 = { 0xDDB4, -0x8000, 0x8000, 0xDDB4 };

    FT_UNUSED( user_data );


    for ( i = first_index; i <= last_index; i++ )
    {
      FT_Outline*  outline;


      if ( FT_Load_Glyph( face, i, load_flags ) )
        continue;

      outline = &face->glyph->outline;

      /* rotate outline by 30 degrees */
      FT_Outline_Transform( outline, &rot30 );

      TIMER_START( timer );
      FT_Outline_Get_BBox( outline, &bbox );
      TIMER_STOP( timer );

      done++;
    }

    return done;
  }


  static int
  test_get_char_index( btimer_t*  timer,
                       FT_Face    face,
                       void*      user_data )
  {
    bcharset_t*  charset = (bcharset_t*)user_data;
    int          i, done = 0;


    TIMER_START( timer );

    for ( i = 0; i < charset->size; i++ )
    {
      if ( FT_Get_Char_Index(face, charset->code[i]) )
        done++;
    }

    TIMER_STOP( timer );

    return done;
  }


  static int
  test_cmap_cache( btimer_t*  timer,
                   FT_Face    face,
                   void*      user_data )
  {
    bcharset_t*  charset = (bcharset_t*)user_data;
    int          i, done = 0;

    FT_UNUSED( face );


    if ( !cmap_cache )
    {
      if ( FTC_CMapCache_New( cache_man, &cmap_cache ) )
        return 0;
    }

    TIMER_START( timer );

    for ( i = 0; i < charset->size; i++ )
    {
      if ( FTC_CMapCache_Lookup( cmap_cache,
                                 font_type.face_id,
                                 0,
                                 charset->code[i] ) )
        done++;
    }

    TIMER_STOP( timer );

    return done;
  }


  static int
  test_image_cache( btimer_t*  timer,
                    FT_Face    face,
                    void*      user_data )
  {
    FT_Glyph      glyph;
    unsigned int  i;
    int           done = 0;

    FT_UNUSED( face );
    FT_UNUSED( user_data );


    if ( !image_cache )
    {
      if ( FTC_ImageCache_New( cache_man, &image_cache ) )
        return 0;
    }

    TIMER_START( timer );

    for ( i = first_index; i <= last_index; i++ )
    {
      if ( !FTC_ImageCache_Lookup( image_cache,
                                   &font_type,
                                   i,
                                   &glyph,
                                   NULL ) )
        done++;
    }

    TIMER_STOP( timer );

    return done;
  }


  static int
  test_sbit_cache( btimer_t*  timer,
                   FT_Face    face,
                   void*      user_data )
  {
    FTC_SBit      glyph;
    unsigned int  i;
    int           done = 0;

    FT_UNUSED( face );
    FT_UNUSED( user_data );


    if ( !sbit_cache )
    {
      if ( FTC_SBitCache_New( cache_man, &sbit_cache ) )
        return 0;
    }

    TIMER_START( timer );

    for ( i = first_index; i <= last_index; i++ )
    {
      if ( !FTC_SBitCache_Lookup( sbit_cache,
                                  &font_type,
                                  i,
                                  &glyph,
                                  NULL ) )
        done++;
    }

    TIMER_STOP( timer );

    return done;
  }


  static int
  test_cmap_iter( btimer_t*  timer,
                  FT_Face    face,
                  void*      user_data )
  {
    FT_UInt   idx;
    FT_ULong  charcode;

    FT_UNUSED( user_data );


    TIMER_START( timer );

    charcode = FT_Get_First_Char( face, &idx );
    while ( idx != 0 )
      charcode = FT_Get_Next_Char( face, charcode, &idx );

    TIMER_STOP( timer );

    return 1;
  }


  static int
  test_new_face( btimer_t*  timer,
                 FT_Face    face,
                 void*      user_data )
  {
    FT_Face  bench_face;

    FT_UNUSED( face );
    FT_UNUSED( user_data );


    TIMER_START( timer );

    if ( !get_face( &bench_face ) )
      FT_Done_Face( bench_face );

    TIMER_STOP( timer );

    return 1;
  }


  static int
  test_new_face_and_load_glyph( btimer_t*  timer,
                                FT_Face    face,
                                void*      user_data )
  {
    FT_Face  bench_face;

    unsigned int  i;
    int           done = 0;

    FT_UNUSED( face );
    FT_UNUSED( user_data );


    TIMER_START( timer );

    if ( !get_face( &bench_face ) )
    {
      for ( i = first_index; i <= last_index; i++ )
      {
        if ( !FT_Load_Glyph( bench_face, i, load_flags ) )
          done++;
      }

      FT_Done_Face( bench_face );
    }

    TIMER_STOP( timer );

    return done;
  }


  /*
   * main
   */

  static void
  get_charset( FT_Face      face,
               bcharset_t*  charset )
  {
    FT_ULong  charcode;
    FT_UInt   gindex;
    int       i;


    charset->code = (FT_ULong*)calloc( (size_t)face->num_glyphs,
                                       sizeof ( FT_ULong ) );
    if ( !charset->code )
      return;

    if ( face->charmap )
    {
      i        = 0;
      charcode = FT_Get_First_Char( face, &gindex );

      /* certain fonts contain a broken charmap that will map character */
      /* codes to out-of-bounds glyph indices.  Take care of that here. */
      /*                                                                */
      while ( gindex && i < face->num_glyphs )
      {
        if ( gindex >= first_index && gindex <= last_index )
          charset->code[i++] = charcode;
        charcode = FT_Get_Next_Char( face, charcode, &gindex );
      }
    }
    else
    {
      unsigned int  j;


      /* no charmap, do an identity mapping */
      for ( i = 0, j = first_index; j <= last_index; i++, j++ )
        charset->code[i] = j;
    }

    charset->size = i;
  }


  static FT_Error
  get_face( FT_Face*  face )
  {
    static unsigned char*  memory_file = NULL;
    static size_t          memory_size;
    int                    face_index = 0;
    FT_Error               error;


    if ( preload )
    {
      if ( !memory_file )
      {
        FILE*  file = fopen( filename, "rb" );


        if ( file == NULL )
        {
          fprintf( stderr, "couldn't find or open `%s'\n", filename );

          return 1;
        }

        fseek( file, 0, SEEK_END );
        memory_size = (size_t)ftell( file );
        fseek( file, 0, SEEK_SET );

        memory_file = (FT_Byte*)malloc( memory_size );
        if ( memory_file == NULL )
        {
          fprintf( stderr,
                   "couldn't allocate memory to pre-load font file\n" );

          return 1;
        }

        if ( !fread( memory_file, memory_size, 1, file ) )
        {
          fprintf( stderr, "read error\n" );
          free( memory_file );
          memory_file = NULL;

          return 1;
        }
      }

      error = FT_New_Memory_Face( lib,
                                  memory_file,
                                  (FT_Long)memory_size,
                                  face_index,
                                  face );
    }
    else
      error = FT_New_Face( lib, filename, face_index, face );

    if ( error )
      fprintf( stderr, "couldn't load font resource\n");

    return error;
  }


  static void
  usage( void )
  {
    int   i;
    char  interpreter_versions[32];
    char  hinting_engines[32];


    /* we expect that at least one interpreter version is available */
    if ( num_tt_interpreter_versions == 2 )
      sprintf(interpreter_versions,
              "%d and %d",
              tt_interpreter_versions[0],
              tt_interpreter_versions[1] );
    else
      sprintf(interpreter_versions,
              "%d, %d, and %d",
              tt_interpreter_versions[0],
              tt_interpreter_versions[1],
              tt_interpreter_versions[2] );

    /* we expect that at least one hinting engine is available */
    if ( num_ps_hinting_engines == 1 )
      sprintf(hinting_engines,
              "`%s'",
              ps_hinting_engine_names[ps_hinting_engines[0]] );
    else
      sprintf(hinting_engines,
              "`%s' and `%s'",
              ps_hinting_engine_names[ps_hinting_engines[0]],
              ps_hinting_engine_names[ps_hinting_engines[1]] );


    fprintf( stderr,
      "\n"
      "ftbench: run FreeType benchmarks\n"
      "--------------------------------\n"
      "\n"
      "Usage: ftbench [options] fontname\n"
      "\n"
      "  -C        Compare with cached version (if available).\n"
      "  -c N      Use at most N iterations for each test\n"
      "            (0 means time limited).\n"
      "  -f L      Use hex number L as load flags (see `FT_LOAD_XXX').\n"
      "  -H NAME   Use PS hinting engine NAME.\n"
      "            Available versions are %s; default is `%s'.\n"
      "  -I VER    Use TT interpreter version VER.\n"
      "            Available versions are %s; default is version %d.\n"
      "  -i IDX    Start with index IDX (default is 0).\n"
      "  -j IDX    End with index IDX (default is number of glyphs minus one).\n"
      "  -l N      Set LCD filter to N\n"
      "              0: none, 1: default, 2: light, 16: legacy\n"
      "  -m M      Set maximum cache size to M KiByte (default is %d).\n",
             hinting_engines,
             ps_hinting_engine_names[dflt_ps_hinting_engine],
             interpreter_versions,
             dflt_tt_interpreter_version,
             CACHE_SIZE );
    fprintf( stderr,
      "  -p        Preload font file in memory.\n"
      "  -r N      Set render mode to N\n"
      "              0: normal, 1: light, 2: mono, 3: LCD, 4: LCD vertical\n"
      "            (default is 0).\n"
      "  -s S      Use S ppem as face size (default is %dppem).\n"
      "            If set to zero, don't call FT_Set_Pixel_Sizes.\n"
      "            Use value 0 with option `-f 1' or something similar to\n"
      "            load the glyphs unscaled, otherwise errors will show up.\n",
             FACE_SIZE );
    fprintf( stderr,
      "  -t T      Use at most T seconds per bench (default is %.0f).\n"
      "\n"
      "  -b tests  Perform chosen tests (default is all):\n",
             BENCH_TIME );

    for ( i = 0; i < N_FT_BENCH; i++ )
    {
      if ( !bench_desc[i] )
        break;

      fprintf( stderr,
      "              %c  %s\n", 'a' + i, bench_desc[i] );
    }

    fprintf( stderr,
      "\n"
      "  -v        Show version.\n"
      "\n" );

    exit( 1 );
  }


#define TEST( x ) ( !test_string || strchr( test_string, (x) ) )


  int
  main( int     argc,
        char**  argv )
  {
    FT_Face   face;
    FT_Error  error;

    unsigned long  max_bytes      = CACHE_SIZE * 1024;
    char*          test_string    = NULL;
    unsigned int   size           = FACE_SIZE;
    int            max_iter       = 0;
    double         max_time       = BENCH_TIME;
    int            compare_cached = 0;
    size_t         i;
    int            j;

    unsigned int  versions[3] = { TT_INTERPRETER_VERSION_35,
                                  TT_INTERPRETER_VERSION_38,
                                  TT_INTERPRETER_VERSION_40 };
    unsigned int  engines[2]  = { FT_HINTING_FREETYPE,
                                  FT_HINTING_ADOBE };
    int           version;
    char         *engine;


    if ( FT_Init_FreeType( &lib ) )
    {
      fprintf( stderr, "could not initialize font library\n" );

      return 1;
    }


    /* collect all available versions, then set again the default */
    FT_Property_Get( lib,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );
    for ( j = 0; j < 3; j++ )
    {
      error = FT_Property_Set( lib,
                               "truetype",
                               "interpreter-version", &versions[j] );
      if ( !error )
        tt_interpreter_versions[num_tt_interpreter_versions++] = versions[j];
    }
    FT_Property_Set( lib,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );

    FT_Property_Get( lib,
                     "cff",
                     "hinting-engine", &dflt_ps_hinting_engine );
    for ( j = 0; j < 2; j++ )
    {
      error = FT_Property_Set( lib,
                               "cff",
                               "hinting-engine", &engines[j] );
      if ( !error )
        ps_hinting_engines[num_ps_hinting_engines++] = engines[j];
    }
    FT_Property_Set( lib,
                     "cff",
                     "hinting-engine", &dflt_ps_hinting_engine );
    FT_Property_Set( lib,
                     "type1",
                     "hinting-engine", &dflt_ps_hinting_engine );
    FT_Property_Set( lib,
                     "t1cid",
                     "hinting-engine", &dflt_ps_hinting_engine );


    version = (int)dflt_tt_interpreter_version;
    engine  = ps_hinting_engine_names[dflt_ps_hinting_engine];

    while ( 1 )
    {
      int  opt;


      opt = getopt( argc, argv, "b:Cc:f:H:I:i:j:l:m:pr:s:t:v" );

      if ( opt == -1 )
        break;

      switch ( opt )
      {
      case 'b':
        test_string = optarg;
        break;

      case 'C':
        compare_cached = 1;
        break;

      case 'c':
        max_iter = atoi( optarg );
        if ( max_iter < 0 )
          max_iter = -max_iter;
        break;

      case 'f':
        load_flags = strtol( optarg, NULL, 16 );
        break;

      case 'H':
        engine = optarg;

        for ( j = 0; j < num_ps_hinting_engines; j++ )
        {
          if ( !strcmp( engine, ps_hinting_engine_names[j] ) )
          {
            FT_Property_Set( lib,
                             "cff",
                             "hinting-engine", &j );
            FT_Property_Set( lib,
                             "type1",
                             "hinting-engine", &j );
            FT_Property_Set( lib,
                             "t1cid",
                             "hinting-engine", &j );
            break;
          }
        }

        if ( j == num_ps_hinting_engines )
          fprintf( stderr,
                   "warning: couldn't set hinting engine\n" );
        break;

      case 'I':
        version = atoi( optarg );

        for ( j = 0; j < num_tt_interpreter_versions; j++ )
        {
          if ( version == (int)tt_interpreter_versions[j] )
          {
            FT_Property_Set( lib,
                             "truetype",
                             "interpreter-version", &version );
            break;
          }
        }

        if ( j == num_tt_interpreter_versions )
          fprintf( stderr,
                   "warning: couldn't set TT interpreter version\n" );
        break;

      case 'i':
        {
          int  fi = atoi( optarg );


          if ( fi > 0 )
            first_index = (unsigned int)fi;
        }
        break;

      case 'j':
        {
          int  li = atoi( optarg );


          if ( li > 0 )
            last_index = (unsigned int)li;
        }
        break;

      case 'l':
        {
          int  filter = atoi( optarg );


          switch ( filter )
          {
          case FT_LCD_FILTER_NONE:
          case FT_LCD_FILTER_DEFAULT:
          case FT_LCD_FILTER_LIGHT:
          case FT_LCD_FILTER_LEGACY1:
          case FT_LCD_FILTER_LEGACY:
            FT_Library_SetLcdFilter( lib, (FT_LcdFilter)filter );
          }
        }
        break;

      case 'm':
        {
          int  mb = atoi( optarg );


          if ( mb > 0 )
            max_bytes = (unsigned int)mb * 1024;
        }
        break;

      case 'p':
        preload = 1;
        break;

      case 'r':
        {
          int  rm = atoi( optarg );


          if ( rm < 0 || rm >= FT_RENDER_MODE_MAX )
            render_mode = FT_RENDER_MODE_NORMAL;
          else
            render_mode = (FT_Render_Mode)rm;
        }
        break;

      case 's':
        {
          int  sz = atoi( optarg );


          /* value 0 is special */
          if ( sz < 0 )
            size = 1;
          else
            size = (unsigned int)sz;
        }
        break;

      case 't':
        max_time = atof( optarg );
        if ( max_time < 0 )
          max_time = -max_time;
        break;

      case 'v':
        {
          FT_Int  major, minor, patch;


          FT_Library_Version( lib, &major, &minor, &patch );

          printf( "ftbench (FreeType) %d.%d", major, minor );
          if ( patch )
            printf( ".%d", patch );
          printf( "\n" );
          exit( 0 );
        }
        /* break; */

      default:
        usage();
        break;
      }
    }

    argc -= optind;
    argv += optind;

    if ( argc != 1 )
      usage();

    filename = *argv;

    if ( get_face( &face ) )
      goto Exit;

    if ( last_index >= (unsigned int)face->num_glyphs )
      last_index = (unsigned int)face->num_glyphs - 1;
    if ( last_index < first_index )
      last_index = first_index;

    if ( size )
    {
      if ( FT_IS_SCALABLE( face ) )
      {
        if ( FT_Set_Pixel_Sizes( face, size, size ) )
        {
          fprintf( stderr, "failed to set pixel size to %d\n", size );

          return 1;
        }
      }
      else
      {
        size = (unsigned int)face->available_sizes[0].size >> 6;
        fprintf( stderr,
                 "using size of first bitmap strike (%dpx)\n", size );
        FT_Select_Size( face, 0 );
      }
    }

    FTC_Manager_New( lib,
                     0,
                     0,
                     max_bytes,
                     face_requester,
                     face,
                     &cache_man );

    font_type.face_id = (FTC_FaceID)1;
    font_type.width   = size;
    font_type.height  = size;
    font_type.flags   = load_flags;

    printf( "\n"
            "ftbench results for font `%s'\n"
            "---------------------------",
            filename );
    for ( i = 0; i < strlen( filename ); i++ )
      putchar( '-' );
    putchar( '\n' );

    printf( "\n"
            "family: %s\n"
            " style: %s\n"
            "\n",
            face->family_name,
            face->style_name );

    if ( max_iter )
      printf( "number of iterations for each test: at most %d\n",
              max_iter );
    printf( "number of seconds for each test: %s%f\n",
             max_iter ? "at most " : "",
             max_time );

    printf( "\n"
            "first glyph index: %d\n"
            "last glyph index: %d\n"
            "face size: %dppem\n"
            "font preloading into memory: %s\n",
            first_index,
            last_index,
            size,
            preload ? "yes" : "no" );

    printf( "\n"
            "load flags: 0x%X\n"
            "render mode: %d\n",
            load_flags,
            render_mode );
    printf( "\n"
            "CFF hinting engine set to `%s'\n"
            "TrueType interpreter set to version %d\n"
            "maximum cache size: %ldKiByte\n",
            engine,
            version,
            max_bytes / 1024 );

    printf( "\n"
            "executing tests:\n" );

    for ( j = 0; j < N_FT_BENCH; j++ )
    {
      btest_t   test;
      FT_ULong  flags;


      if ( !TEST( 'a' + j ) )
        continue;

      test.title       = NULL;
      test.bench       = NULL;
      test.cache_first = 0;
      test.user_data   = NULL;

      switch ( j )
      {
      case FT_BENCH_LOAD_GLYPH:
        test.title = "Load";
        test.bench = test_load;
        benchmark( face, &test, max_iter, max_time );

        if ( compare_cached )
        {
          test.cache_first = 1;

          test.title = "Load (image cached)";
          test.bench = test_image_cache;
          benchmark( face, &test, max_iter, max_time );

          test.title = "Load (sbit cached)";
          test.bench = test_sbit_cache;
          if ( size )
            benchmark( face, &test, max_iter, max_time );
          else
            printf( "  %-25s disabled (size = 0)\n", test.title );
        }
        break;

      case FT_BENCH_LOAD_ADVANCES:
        test.user_data = &flags;

        test.title = "Load_Advances (Normal)";
        test.bench = test_load_advances;
        flags      = FT_LOAD_DEFAULT;
        benchmark( face, &test, max_iter, max_time );

        test.title  = "Load_Advances (Fast)";
        test.bench  = test_load_advances;
        flags       = FT_LOAD_TARGET_LIGHT;
        benchmark( face, &test, max_iter, max_time );

        test.title  = "Load_Advances (Unscaled)";
        test.bench  = test_load_advances;
        flags       = FT_LOAD_NO_SCALE;
        benchmark( face, &test, max_iter, max_time );
        break;

      case FT_BENCH_RENDER:
        test.title = "Render";
        test.bench = test_render;
        if ( size )
          benchmark( face, &test, max_iter, max_time );
        else
          printf( "  %-25s disabled (size = 0)\n", test.title );
        break;

      case FT_BENCH_GET_GLYPH:
        test.title = "Get_Glyph";
        test.bench = test_get_glyph;
        benchmark( face, &test, max_iter, max_time );
        break;

      case FT_BENCH_GET_CBOX:
        test.title = "Get_CBox";
        test.bench = test_get_cbox;
        benchmark( face, &test, max_iter, max_time );
        break;

      case FT_BENCH_GET_BBOX:
        test.title = "Get_BBox";
        test.bench = test_get_bbox;
        benchmark( face, &test, max_iter, max_time );
        break;

      case FT_BENCH_CMAP:
        {
          bcharset_t  charset;


          get_charset( face, &charset );
          if ( charset.code )
          {
            test.user_data = (void*)&charset;


            test.title = "Get_Char_Index";
            test.bench = test_get_char_index;

            benchmark( face, &test, max_iter, max_time );

            if ( compare_cached )
            {
              test.cache_first = 1;

              test.title = "Get_Char_Index (cached)";
              test.bench = test_cmap_cache;
              benchmark( face, &test, max_iter, max_time );
            }

            free( charset.code );
          }
        }
        break;

      case FT_BENCH_CMAP_ITER:
        test.title = "Iterate CMap";
        test.bench = test_cmap_iter;
        benchmark( face, &test, max_iter, max_time );
        break;

      case FT_BENCH_NEW_FACE:
        test.title = "New_Face";
        test.bench = test_new_face;
        benchmark( face, &test, max_iter, max_time );
        break;

      case FT_BENCH_EMBOLDEN:
        test.title = "Embolden";
        test.bench = test_embolden;
        if ( size )
          benchmark( face, &test, max_iter, max_time );
        else
          printf( "  %-25s disabled (size = 0)\n", test.title );
        break;

      case FT_BENCH_NEW_FACE_AND_LOAD_GLYPH:
        test.title = "Create face & load glyph(s)";
        test.bench = test_new_face_and_load_glyph;
        benchmark( face, &test, max_iter, max_time );
        break;
      }
    }

  Exit:
    /* The following is a bit subtle: When we call FTC_Manager_Done, this
     * normally destroys all FT_Face objects that the cache might have
     * created by calling the face requester.
     *
     * However, this little benchmark uses a tricky face requester that
     * doesn't create a new FT_Face through FT_New_Face but simply passes a
     * pointer to the one that was previously created.
     *
     * If the cache manager has been used before, the call to
     * FTC_Manager_Done discards our single FT_Face.
     *
     * In the case where no cache manager is in place, or if no test was
     * run, the call to FT_Done_FreeType releases any remaining FT_Face
     * object anyway.
     */
    if ( cache_man )
      FTC_Manager_Done( cache_man );

    FT_Done_FreeType( lib );

    return 0;
  }


/* End */
