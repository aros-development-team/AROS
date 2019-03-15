/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality font engine         */
/*                                                                          */
/*  Copyright 2005-2018 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*  ftvalid: Validates layout related tables of OpenType and                */
/*           TrueTypeGX/AAT.  This program calls `FT_OpenType_Validate',    */
/*           `FT_TrueTypeGX_Validate' or `FT_ClassicKern_Validate' on a     */
/*           given file, and reports the validation result.                 */
/*                                                                          */
/*                                                                          */
/*  written by YAMATO Masatake and SUZUKI Toshiya.                          */
/*                                                                          */
/****************************************************************************/


#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H

#include FT_OPENTYPE_VALIDATE_H
#include FT_GX_VALIDATE_H

  /* the following four header files shouldn't be used in normal programs */
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_VALIDATE_H
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_OBJECTS_H

#include "common.h"
#include "mlgetopt.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


  static char*  execname;

  typedef enum
  {
    OT_VALIDATE = 0,
    GX_VALIDATE,
    CKERN_VALIDATE,

    LAST_VALIDATE

  } ValidatorType;
  static ValidatorType  validator;


  typedef struct  TableSpecRec_
  {
    FT_UInt  tag;
    FT_UInt  validation_flag;

  } TableSpecRec, *TableSpec;

#define MAKE_TABLE_SPEC( x ) { TTAG_##x, FT_VALIDATE_##x }

  static TableSpecRec  ot_table_spec[] =
  {
    MAKE_TABLE_SPEC( BASE ),
    MAKE_TABLE_SPEC( GDEF ),
    MAKE_TABLE_SPEC( GPOS ),
    MAKE_TABLE_SPEC( GSUB ),
    MAKE_TABLE_SPEC( JSTF ),
    MAKE_TABLE_SPEC( MATH ),
  };
#define N_OT_TABLE_SPEC  ( sizeof ( ot_table_spec ) / sizeof ( TableSpecRec ) )

  static TableSpecRec  gx_table_spec[] =
  {
    MAKE_TABLE_SPEC( feat ),
    MAKE_TABLE_SPEC( mort ),
    MAKE_TABLE_SPEC( morx ),
    MAKE_TABLE_SPEC( bsln ),
    MAKE_TABLE_SPEC( just ),
    MAKE_TABLE_SPEC( kern ),
    MAKE_TABLE_SPEC( opbd ),
    MAKE_TABLE_SPEC( trak ),
    MAKE_TABLE_SPEC( prop ),
    MAKE_TABLE_SPEC( lcar ),
  };
#define N_GX_TABLE_SPEC  ( sizeof ( gx_table_spec ) / sizeof ( TableSpecRec ) )


  typedef struct  ValidatorRec_
  {
    ValidatorType  type;
    const char*    symbol;

    const char*    unimplemented_message;
    int         (* is_implemented)( FT_Library library );

    FT_Error    (* run)           ( FT_Face     face,
                                    const char* tables,
                                    int         validation_level );
    int         (* list_tables)   ( FT_Face     face );

    TableSpec      table_spec;
    unsigned int   n_table_spec;

  } ValidatorRec, *Validator;

  static int is_ot_validator_implemented    ( FT_Library library );
  static int is_gx_validator_implemented    ( FT_Library library );
  static int is_ckern_validator_implemented ( FT_Library library );

  static FT_Error run_ot_validator          ( FT_Face      face,
                                              const char*  tables,
                                              int          validation_level );
  static FT_Error run_gx_validator          ( FT_Face      face,
                                              const char*  tables,
                                              int          validation_level );
  static FT_Error run_ckern_validator       ( FT_Face      face,
                                              const char*  dialect_request,
                                              int          validation_level );

  static int list_ot_tables                 ( FT_Face  face );
  static int list_gx_tables                 ( FT_Face  face );
  static int list_ckern_tables              ( FT_Face  face );


  static ValidatorRec validators[] =
  {
    {
      OT_VALIDATE,
      "ot",
      ( "FT_OpenType_Validate"
        " is disabled!  Recompile FreeType 2 with "
        "otvalid"
        " module enabled.\n" ),
      is_ot_validator_implemented,
      run_ot_validator,
      list_ot_tables,
      ot_table_spec,
      N_OT_TABLE_SPEC,
    },
    {
      GX_VALIDATE,
      "gx",
      ( "FT_TrueTypeGX_Validate"
        " is disabled!  Recompile FreeType 2 with "
        "gxvalid"
        " module enabled.\n" ),
      is_gx_validator_implemented,
      run_gx_validator,
      list_gx_tables,
      gx_table_spec,
      N_GX_TABLE_SPEC,
    },
    {
      CKERN_VALIDATE,
      "ckern",
      ( "FT_ClassicKern_Validate"
        " is disabled!  Recompile FreeType 2 with "
        "gxvalid"                /* NOTE: classic kern validator is in gxvalid. */
        " module enabled.\n" ),
      is_ckern_validator_implemented,
      run_ckern_validator,
      list_ckern_tables,
      NULL,
      0,
    },
  };


  static void
  panic( int          error,
         const char*  message )
  {
    fprintf( stderr, "%s\n  error = 0x%04x\n", message, error );
    exit( 1 );
  }


  static char*
  make_tag_chararray ( char     chararray[4],
                       FT_UInt  tag )
  {
    chararray[0] = (char)( ( tag >> 24 ) & 0xFF );
    chararray[1] = (char)( ( tag >> 16 ) & 0xFF );
    chararray[2] = (char)( ( tag >> 8  ) & 0xFF );
    chararray[3] = (char)( ( tag >> 0  ) & 0xFF );
    return chararray;
  }


  static void
  print_tag ( FILE*    stream,
              FT_UInt  tag )
  {
    char  buffer[5];


    buffer[4] = '\0';
    fprintf( stream, "%s", make_tag_chararray( buffer, tag ) );
  }


  /* To initialize the internal variable, call this
     function with FT_Library variable. Then call
     with NULL. The print messages is printed if
     call with NULL. */
  static void
  print_usage( FT_Library  library_initializer )
  {
    unsigned int i, j;
    Validator v;
    static FT_Library library;

    if (library_initializer)
    {
      library = library_initializer;
      return ;
    }

    fprintf( stderr,
      "\n"
      "ftvalid: layout table validator -- part of the FreeType project\n"
      "---------------------------------------------------------------\n"
      "\n" );
    fprintf( stderr,
      "Usage: %s [options] fontfile\n"
      "\n",
             execname );

    fprintf( stderr,
      "Options:\n"
      "\n" );

    fprintf( stderr,
      "  -f index      Select font index (default: 0).\n"
      "\n" );

    fprintf( stderr,
      "  -t validator  Select validator.\n"
      "                Available validators:\n"
      "                 " );
    for ( i = 0; i < LAST_VALIDATE; i++ )
    {
      v = &validators[i];
      fprintf( stderr, " %s%s",
               v->symbol,
               v->is_implemented( library ) ? ""
                                            : " (NOT COMPILED IN)" );
    }
    fprintf( stderr,
      "\n"
      "\n" );

    fprintf( stderr,
      "  -T tbls       [ot, gx] Select sfnt table name tags to be validated.\n"
      "                Use `:' to separate tags.\n"
      "\n" );
    for ( i = 0; i < LAST_VALIDATE; i++ )
    {
      v = &validators[i];

      if ( v->n_table_spec == 0 )
        continue;

      fprintf( stderr,
        "                Supported tables in %s validator:\n"
        "                 ",
               v->symbol );
      for ( j = 0; j < v->n_table_spec; j++ )
      {
        fprintf( stderr, " " );
        print_tag( stderr, v->table_spec[j].tag );
      }
      fprintf( stderr,
        "\n"
        "\n" );
    }
    fprintf( stderr,
      "                Example: -T \"feat:morx\"\n"
      "\n" );

    fprintf( stderr,
      "  -T dialect    [ckern] Select classic kern dialect for validation.\n"
      "                Use `:' to separate dialect names.\n"
      "                If more than one dialect is specified,\n"
      "                all dialects are accepted when validating.\n"
      "\n"
      "                Supported dialects in ckern validator:\n"
      "                  ms apple\n"
      "\n" );

    fprintf( stderr,
      "  -l            List the layout-related SFNT tables\n"
      "                available in the font file.\n"
      "                The selected validator (with option `-t')\n"
      "                affects the list.\n"
      "\n"
      "                ckern is applicable to `kern' table only.\n"
      "                Option `-l' lists dialects supported in ckern validator\n"
      "                only if `kern' table is available in the font file.\n"
      "\n" );

    fprintf( stderr,
      "  -V level      Validation level.  Possible values:\n"
      "                  0 (default), 1 (tight), 2 (paranoid)\n"
      "\n" );

    fprintf( stderr,
      "  -v            Show version."
      "\n" );

    fprintf( stderr,
      "-------------------------------------------------------------------\n"
      "\n" );

    fprintf( stderr,
      "`FT2_DEBUG' environment variable:\n"
      "\n"
      "  You can specify `component:level' pairs for tracing.\n"
      "  `level' must be in the range [1,7].\n"
      "  Available components for ot validator:\n"
      "    otvmodule otvcommon otvbase otvgdef otvgpos otvgsub otvjstf\n"
      "  Available components for gx validator:\n"
      "    gxvmodule gxvcommon gxvfeat gxvmort gxvmorx gxvbsln gxvjust\n"
      "    gxvkern gxvopbd gxvtrak gxvprop gxvlcar\n"
      "  Available components for ckern validator:\n"
      "    gxvkern\n"
      "\n"
      "  Example:\n"
      "\n"
      "    FT2_DEBUG=\"otvcommon:5 gxvkern:7\"\n"
      "\n"
      "FT2_DEBUG only works if tracing support is compiled into FreeType 2\n"
      "\n" );

    exit( 1 );
  }


  static FT_Error
  try_load( FT_Face   face,
            FT_ULong  tag )
  {
    FT_ULong  length;


    length = 0;
    return FT_Load_Sfnt_Table( face, tag, 0, NULL, &length );
  }


  static FT_UInt
  find_validation_flag( FT_UInt             tag,
                        const TableSpecRec  spec[],
                        int                 spec_count )
  {
    int i;


    for ( i = 0; i < spec_count; i++ )
    {
      if ( tag == spec[i].tag )
        return spec[i].validation_flag;
    }

    fprintf( stderr, "*** Wrong table name: " );
    print_tag( stderr, tag );
    fprintf( stderr, "\n" );

    print_usage( NULL );

    return 0;
  }


  static FT_UInt
  parse_table_specs( const char*         tables,
                     const TableSpecRec  spec[],
                     int                 spec_count )
  {
    FT_UInt  validation_flags;
    size_t   len;

    unsigned int  i;
    char          tag[4];


    validation_flags = 0;

    len = strlen( tables );
    if (( len % 5 ) != 4 )
    {
      fprintf( stderr, "*** Wrong length of table names\n" );
      print_usage( NULL );
    }

    for ( i = 0; i < len; i++ )
    {
      if ( ( ( i % 5 ) == 4 ) )
      {
        if ( tables[i] != ':' )
        {
          fprintf( stderr, "*** Wrong table separator: %c\n", tables[i] );
          print_usage( NULL );
        }
        i++;
      }

      tag[i % 5] = tables[i];
      if ( ( i % 5 ) == 3 )
        validation_flags |= find_validation_flag( FT_MAKE_TAG( tag[0],
                                                               tag[1],
                                                               tag[2],
                                                               tag[3] ),
                                                  spec,
                                                  spec_count );
    }

    return validation_flags;
  }


  static FT_UInt
  list_face_tables( FT_Face             face,
                    const TableSpecRec  spec[],
                    int                 spec_count )
  {
    FT_Error  error;

    FT_UInt  validation_flags;
    int      i;
    FT_UInt  tag;


    validation_flags = 0;

    for ( i = 0; i < spec_count; i++ )
    {
      tag   = spec[i].tag;
      error = try_load( face, tag );
      if ( error == 0 )
        validation_flags |= spec[i].validation_flag;
    }
    return validation_flags;
  }


  static FT_UInt
  make_table_specs( FT_Face             face,
                    const char*         request,
                    const TableSpecRec  spec[],
                    int                 spec_count )
  {
    if ( request == NULL || request[0] == '\0' )
      return list_face_tables ( face, spec, spec_count );
    else
      return parse_table_specs ( request, spec, spec_count );
  }


  static int
  print_tables( FILE*               stream,
                FT_UInt             validation_flags,
                const TableSpecRec  spec[],
                int                 spec_count )
  {
    int i;
    int n_print;


    for ( i = 0, n_print = 0; i < spec_count; i++ )
    {
      if ( spec[i].validation_flag & validation_flags )
      {
        if ( n_print != 0 )
          fprintf( stream, "%c", ':' );
        print_tag( stream, spec[i].tag );
        n_print++;
      }
    }

    fprintf( stream, "\n" );

    return !n_print;
  }


  static void
  report_header( FT_UInt             validation_flags,
                 const TableSpecRec  spec[],
                 int                 spec_count )
  {
    printf( "[%s:%s] validation targets: ",
            execname, validators[validator].symbol );
    print_tables( stdout, validation_flags, spec, spec_count );
    printf( "-------------------------------------------------------------------\n" );
  }


  static void
  report_result( FT_Bytes            data[],
                 FT_UInt             validation_flags,
                 const TableSpecRec  spec[],
                 int                 spec_count )
  {
    int  i;
    int  n_passes;
    int  n_targets;


    for ( i = 0, n_passes = 0, n_targets = 0; i < spec_count; i++ )
    {
      if ( spec[i].validation_flag & validation_flags )
      {
        n_targets++;

        if ( data[i] != NULL )
        {
          printf( "[%s:%s] ", execname, validators[validator].symbol );
          print_tag( stdout, spec[i].tag );
          printf( "%s", "...pass\n" );
          n_passes++;
        }
      }
    }

    if ( n_passes == 0 && n_targets != 0 )
    {
      printf( "[%s:%s] layout tables are invalid.\n",
              execname, validators[validator].symbol );
      printf( "[%s:%s] set FT2_DEBUG environment variable to\n",
              execname, validators[validator].symbol );
      printf( "[%s:%s] know the validation detail.\n",
              execname, validators[validator].symbol );
    }
  }


  /*
   * OpenType related functions
   */
  static int
  is_ot_validator_implemented( FT_Library library )
  {
    FT_Module mod;

    mod = FT_Get_Module( library, "otvalid" );
    return mod? 1: 0;
  }

  static FT_Error
  run_ot_validator( FT_Face      face,
                    const char*  tables,
                    int          validation_level )
  {
    FT_UInt       validation_flags;
    FT_Error      error;
    FT_Bytes      data[N_OT_TABLE_SPEC];
    unsigned int  i;


    validation_flags  = (FT_UInt)validation_level;
    validation_flags |= make_table_specs( face, tables, ot_table_spec,
                                          N_OT_TABLE_SPEC );

    for ( i = 0; i < N_OT_TABLE_SPEC; i++ )
      data[i] = NULL;

    report_header( validation_flags, ot_table_spec, N_OT_TABLE_SPEC );

    error = FT_OpenType_Validate(
              face,
              validation_flags,
              &data[0], &data[1], &data[2], &data[3], &data[4] );

    report_result( data, validation_flags, ot_table_spec, N_OT_TABLE_SPEC );

    for ( i = 0; i < N_OT_TABLE_SPEC; i++ )
      FT_OpenType_Free( face, data[i] );

    return error;
  }


  static int
  list_ot_tables( FT_Face  face )
  {
    FT_UInt  validation_flags;


    validation_flags = list_face_tables( face, ot_table_spec,
                                         N_OT_TABLE_SPEC );
    return print_tables( stdout, validation_flags, ot_table_spec,
                         N_OT_TABLE_SPEC );
  }


  /*
   * TrueTypeGX related functions
   */
  static int
  is_gx_validator_implemented( FT_Library library )
  {
    FT_Module mod;

    mod = FT_Get_Module( library, "gxvalid" );
    return mod? 1: 0;
  }

  static FT_Error
  run_gx_validator( FT_Face      face,
                    const char*  tables,
                    int          validation_level )
  {
    FT_UInt       validation_flags;
    FT_Error      error;
    FT_Bytes      data[N_GX_TABLE_SPEC];
    unsigned int  i;


    validation_flags  = (FT_UInt)validation_level;
    validation_flags |= make_table_specs( face, tables, gx_table_spec,
                                          N_GX_TABLE_SPEC );

    for ( i = 0; i < N_GX_TABLE_SPEC; i++ )
      data[i] = NULL;

    report_header( validation_flags, gx_table_spec, N_GX_TABLE_SPEC );

    error = FT_TrueTypeGX_Validate(
              face,
              validation_flags,
              data,
              N_GX_TABLE_SPEC );

    report_result( data, validation_flags, gx_table_spec, N_GX_TABLE_SPEC );

    for ( i = 0; i < N_GX_TABLE_SPEC; i++ )
      FT_TrueTypeGX_Free( face, data[i] );

    return error;
  }


  static int
  list_gx_tables ( FT_Face  face )
  {
    FT_UInt  validation_flags;

    validation_flags = list_face_tables( face, gx_table_spec,
                                         N_GX_TABLE_SPEC );
    return print_tables( stdout, validation_flags, gx_table_spec,
                         N_GX_TABLE_SPEC );
  }


  /*
   * Classic kern related functions
   */
  static int
  is_ckern_validator_implemented( FT_Library library )
  {
    FT_Module mod;

    mod = FT_Get_Module( library, "gxvalid" );
    return mod? 1: 0;
  }


  static FT_Error
  run_ckern_validator( FT_Face      face,
                       const char*  dialect_request,
                       int          validation_level )
  {
    FT_UInt    validation_flags;
    FT_Error   error;
    FT_Bytes   data;


    if ( dialect_request == NULL )
      dialect_request = "ms:apple";


    validation_flags  = (FT_UInt)validation_level;

    if ( strcmp( dialect_request, "ms:apple" ) == 0 ||
         strcmp( dialect_request, "apple:ms" ) == 0 )
      validation_flags |= FT_VALIDATE_MS | FT_VALIDATE_APPLE;
    else if ( strcmp( dialect_request, "ms" ) == 0 )
      validation_flags |= FT_VALIDATE_MS;
    else if ( strcmp( dialect_request, "apple" ) == 0 )
      validation_flags |= FT_VALIDATE_APPLE;
    else
    {
      fprintf( stderr, "Wrong classic kern dialect: %s\n", dialect_request );
      print_usage( NULL );
    }

    printf( "[%s:%s] validation targets: %s...",
            execname, validators[validator].symbol, dialect_request );


    error = FT_ClassicKern_Validate(
              face,
              validation_flags,
              &data );


    if ( data )
      printf( "pass\n" );
    else if ( data == NULL && error )
      printf( "fail\n" );
    else
      printf( "no kern\n" );

    FT_ClassicKern_Free( face, data );

    return error;
  }

  static int
  list_ckern_tables ( FT_Face  face )
  {
    FT_Error  error;

    error = try_load( face, TTAG_kern );
    if ( error == 0 )
      printf( "ms:apple\n" );
    return 0;
  }

  /*
   * Main driver
   */

  int
  main( int     argc,
        char**  argv )
  {
    FT_Library  library;
    FT_Error    error;

    char*  fontfile;
    int    option;


    char*  tables;
    int    dump_table_list;

    int  validation_level;

    int  font_index = 0;


    execname = ft_basename( argv[0] );

    error = FT_Init_FreeType( &library );
    if ( error )
      panic ( error, "Could not initialize FreeType library" );

    /* Initialize print_usage internal variable */
    print_usage( library );


    /*
     * Parsing options
     */
    validator        = OT_VALIDATE;
    tables           = NULL;
    dump_table_list  = 0;
    validation_level = FT_VALIDATE_DEFAULT;

    while ( 1 )
    {
      option = getopt( argc, argv, "f:lt:T:vV:" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 't':
        {
          int i;

          validator = LAST_VALIDATE;
          for ( i = 0; i < LAST_VALIDATE; i++ )
          {
            if ( strcmp( optarg, validators[i].symbol ) == 0 )
            {
              validator = (ValidatorType)i;
              break;
            }
          }
          if ( validator == LAST_VALIDATE )
          {
            fprintf( stderr, "*** Unknown validator name: %s\n", optarg );
            print_usage( NULL );
          }
        }
        break;

      case 'T':
        tables = optarg;
        break;

      case 'l':
        dump_table_list = 1;
        break;

      case 'V':
        validation_level = atoi( optarg );
        if ( validation_level < 0 ||
             validation_level > FT_VALIDATE_PARANOID )
        {
          fprintf( stderr, "*** Validation level is out of range: %d\n",
                           validation_level );
          print_usage( NULL );
        }
        break;

      case 'f':
        font_index = atoi( optarg );
        break;

      case 'v':
        {
          FT_Int  major, minor, patch;


          FT_Library_Version( library, &major, &minor, &patch );

          printf( "ftvalid (FreeType) %d.%d", major, minor );
          if ( patch )
            printf( ".%d", patch );
          printf( "\n" );
          exit( 0 );
        }
        /* break; */

      default:
        print_usage( NULL );
        break;
      }
    }

    argc -= optind;
    argv += optind;

    if ( argc == 0 )
    {
      fprintf(stderr, "*** Font file is not specified.\n");
      print_usage( NULL );
    }
    else if ( argc > 1 )
    {
      fprintf(stderr, "*** Too many font files.\n");
      print_usage( NULL );
    }

    fontfile = argv[0];

    /*
     * Run a validator
     */
    {
      FT_Face     face;
      FT_Error    status;

      status = 0;

      if ( !validators[validator].is_implemented( library ) )
        panic( FT_Err_Unimplemented_Feature,
               validators[validator].unimplemented_message );


      /* TODO: Multiple faces in a font file? */
      error = FT_New_Face( library, fontfile, font_index, &face );
      if ( error )
        panic( error, "Could not open face." );

      if ( dump_table_list )
        validators[validator].list_tables( face );
      else
        status = validators[validator].run( face, tables, validation_level );

      FT_Done_Face( face );
      FT_Done_FreeType( library );

      return (int)status;
    }
  }


/* End */
