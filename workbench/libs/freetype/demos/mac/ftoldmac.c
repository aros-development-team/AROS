/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2000, 2003, 2004, 2005, 2006 by                          */
/*  suzuki toshiya, D. Turner, R.Wilhelm, and W. Lemberg                    */
/*                                                                          */
/*                                                                          */
/*  ftoldmac - test program for MacOS specific API in ftmac.c               */
/*                                                                          */
/*                                                                          */
/****************************************************************************/


#if defined(__GNUC__) && defined(__APPLE_CC__)
# include <Carbon/Carbon.h>
# include <ApplicationServices/ApplicationServices.h>
# include <stdint.h>
# include <sys/param.h>
#else
# include <ConditionalMacros.h>
# include <Files.h>
# include <Fonts.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H

  /* the following header shouldn't be used in normal programs */
#include FT_INTERNAL_DEBUG_H

  /* showing driver name */
#include FT_MODULE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DRIVER_H

  /* FSSpec functions are deprecated since Mac OS X 10.4 */
#ifndef HAVE_FSSPEC
#if defined( __LP64__ ) && !TARGET_CPU_68K && !TARGET_CPU_PPC && !TARGET_CPU_PPC64
#define HAVE_FSSPEC  0
typedef void FSSpec;
#else
#define HAVE_FSSPEC  1
#endif
#endif

  /* most FSRef functions were introduced since Mac OS 9 */
#ifndef HAVE_FSREF
#if TARGET_API_MAC_OSX
#define HAVE_FSREF  1
#else
#define HAVE_FSREF  0
typedef void FSRef;
#endif
#endif

  /* QuickDraw is deprecated since Mac OS X 10.4 */
#ifndef HAVE_QUICKDRAW_CARBON
#if defined( __LP64__ ) && !TARGET_CPU_68K && !TARGET_CPU_PPC && !TARGET_CPU_PPC64
#define HAVE_QUICKDRAW_CARBON  0
#define HAVE_QUICKDRAW_TOOLBOX 0
#elif TARGET_API_MAC_CARBON || TARGET_API_MAC_OSX
#define HAVE_QUICKDRAW_CARBON  1
#define HAVE_QUICKDRAW_TOOLBOX 1
#elif TARGET_API_MAC_OS8
#define HAVE_QUICKDRAW_CARBON  0
#define HAVE_QUICKDRAW_TOOLBOX 1
#endif
#endif

  /* AppleTypeService is available since Mac OS X */
#ifndef HAVE_ATS
#if TARGET_API_MAC_OSX
#define HAVE_ATS  1
#else
#define HAVE_ATS  0
#endif
#endif

#include FT_MAC_H

#undef FT_GetFile_From_Mac_Name
#undef FT_GetFile_From_Mac_ATS_Name
#undef FT_New_Face_From_FSSpec

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
  FT_OldMac_Err_Unimplemented,
  FT_OldMac_Err_TooLongFileName,
  FT_OldMac_Err_FileNotFound,
  FT_OldMac_Err_UnresolvableFontName,
  FT_OldMac_Err_PseudoFontName,
  FT_OldMac_Err_UnswappableFontID,
  FT_OldMac_Err_TooLargeFaceIndex,
  FT_OldMac_Err_Ok = 0
} FT_OldMac_Err;


/* statics of font scanning */
int      num_scanned_fonts;
int      num_opened_fonts;
int      num_scanned_faces;
int      num_opened_faces;

/* setting for face scanning */
int      max_face_number;
Boolean  force_scan_face;
char*    font_listing_api;
char*    font_resolve_api;
Boolean  auto_suffix;


void     initParamBlock( CInfoPBRec*, Str255 );
void     dumpPBErr( CInfoPBRec* );
void     crawlDir( CInfoPBRec*, char* );
void     crawlFontFile( char* );
OSErr    ftmac_FSPathMakeSpec( const UInt8*, FSSpec*, Boolean );
OSErr    ftmac_FSpMakePath( const FSSpec*, UInt8*, UInt32 );


OSErr
ftmac_FSpMakePath( const FSSpec*  spec_p,
                   UInt8*         path,
                   UInt32         maxPathSize )
{
  OSErr   err;
  FSSpec  spec = *spec_p;
  short   vRefNum;
  long    dirID;
  Str255  parDir_name;


  FT_MEM_SET( path, 0, maxPathSize );
  while ( 1 )
  {
    int             child_namelen = ft_strlen( (char *)path );
    unsigned char   node_namelen  = spec.name[0];
    unsigned char*  node_name     = spec.name + 1;


    if ( node_namelen + child_namelen > maxPathSize )
      return errFSNameTooLong;

    FT_MEM_MOVE( path + node_namelen + 1, path, child_namelen );
    FT_MEM_COPY( path, node_name, node_namelen );
    if ( child_namelen > 0 )
      path[ node_namelen ] = ':';

    vRefNum        = spec.vRefNum;
    dirID          = spec.parID;
    parDir_name[0] = '\0';
    err = FSMakeFSSpec( vRefNum, dirID, parDir_name, &spec );
    if ( noErr != err || dirID == spec.parID )
      break;
  }
  return noErr;
}


void
dump_face_info( FT_Face  face )
{
  printf( "\t\tface_index=%d, face_flags=0x%08x, num_glyphs=%d\n",
              (int)face->face_index,
              (unsigned int)(face->face_flags),
              (int)face->num_glyphs );
  printf( "\t\tnum_fixed_sizes=%d, style_flags=%d%d%d%d%d%d\n",
          face->num_fixed_sizes,
          ( (unsigned int)(face->style_flags) >> 5 ) & 1,
          ( (unsigned int)(face->style_flags) >> 4 ) & 1,
          ( (unsigned int)(face->style_flags) >> 3 ) & 1,
          ( (unsigned int)(face->style_flags) >> 2 ) & 1,
          ( (unsigned int)(face->style_flags) >> 1 ) & 1,
          ( (unsigned int)(face->style_flags)      ) & 1
                                                                );
  printf("\t\tfamily_name=[%s], style_name=[%s]\n",
          face->family_name, face->style_name );
}


void
crawlFontFile( char*  font_file_path )
{
  FT_Library  library;
  FT_Face     face;
  int         i, j;
  FSSpec      spec;


  printf( "*** check font file [%s]\n", font_file_path );

  if ( 0 != FT_Init_FreeType( &library ) )
  {
    printf( "\tError: Could not initialize FreeType\n" );
    return;
  }

  if ( noErr != ftmac_FSPathMakeSpec( (unsigned char const *)font_file_path, &spec, FALSE ) )
  {
    printf( "\tError: Could not open File (MacOS API)\n" );
    return;
  }

  face = NULL;
  num_scanned_fonts ++;
  if ( 0 != FT_New_Face_From_FSSpec( library, &spec, 0, &face ) )
  {
    printf( "\tError: Could not open File (FreeType API)\n" );
    return;
  }

  num_opened_fonts ++;
  printf( "\tFont file has %d face\n", (int)face->num_faces );


  j = face->num_faces + max_face_number;
  for ( i = 0; i < j; i++ )
  {
    num_scanned_faces ++;
    printf( "\tCheck Face #%d...", i );
    if ( 0 == FT_New_Face_From_FSSpec( library, &spec, i, &face ) )
    {
      num_opened_faces ++;
      printf( "Ok\n" );
      dump_face_info( face );
      FT_Done_Face( face );
    }
    else
    {
      printf( "Failed\n" );
      if ( !force_scan_face )
        goto no_more_face;
    }
  }
no_more_face:
  FT_Done_FreeType( library );
}


void
crawlDir( CInfoPBRec*  ci_pb_dir,
          char*        dir_path  )
{
  CInfoPBRec  ci_pb;
  char        file_full_path[1024];
  int         dirname_len;
  int         i;


  printf( "ioVRefNum = 0x%04x, ioDrDirID = 0x%08x, ioDrParID= 0x%08x\n",
          ci_pb_dir->dirInfo.ioVRefNum,
          (unsigned int) ci_pb_dir->dirInfo.ioDrParID,
          (unsigned int) ci_pb_dir->dirInfo.ioDrDirID );
  printf( "files in directory: %d\n", ci_pb_dir->dirInfo.ioDrNmFls );


  dirname_len = strlen( dir_path );
  strcpy( file_full_path, dir_path );
  if ( 0 < dirname_len && ':' != file_full_path[ dirname_len - 1 ] )
    dirname_len ++;

  file_full_path[ dirname_len - 1 ] = ':';


  for ( i = 0; i <= ci_pb_dir->dirInfo.ioDrNmFls; i ++ )
  {
    Str255  fileName;


    memset( &ci_pb, 0, sizeof( CInfoPBRec ) );
    fileName[0] = 0;
    ci_pb.hFileInfo.ioVRefNum   = ci_pb_dir->dirInfo.ioVRefNum;
    ci_pb.hFileInfo.ioDirID     = ci_pb_dir->dirInfo.ioDrDirID;
    ci_pb.hFileInfo.ioNamePtr   = fileName;
    ci_pb.hFileInfo.ioFDirIndex = i;
    if ( noErr == PBGetCatInfoSync( &ci_pb ) )
    {
      if ( NULL != ci_pb.hFileInfo.ioNamePtr )
      {
        char  file_name[256];


        strncpy( file_name, (char *)ci_pb.hFileInfo.ioNamePtr + 1, ci_pb.hFileInfo.ioNamePtr[0] );
        file_name[ ci_pb.hFileInfo.ioNamePtr[0] ] = '\0';
        if ( 0 == strcmp( ".DS_Store", file_name ) )
          printf( "*** known non-font filename [%s]\n", file_name );
        else if ( 0 == ( ci_pb.hFileInfo.ioFlAttrib & ioDirMask ) )
        {
          file_full_path[ dirname_len ] = '\0';
          strncat( file_full_path, file_name, sizeof( file_full_path ) );
          crawlFontFile( file_full_path );
        }
      }
    }
  }
}


void
initParamBlock( CInfoPBRec*  paramBlock,
                 Str255      fileName    )
{
  paramBlock->hFileInfo.ioCompletion = 0; /* synch calls */
  paramBlock->hFileInfo.ioNamePtr    = fileName;
  paramBlock->hFileInfo.ioVRefNum    = 0; /* alias for default */
  paramBlock->hFileInfo.ioFDirIndex  = 0; /* XXX */
  paramBlock->hFileInfo.ioDirID      = 0; /* alias for default */
}

void
dumpPBErr( CInfoPBRec* paramBlock )
{
  printf( "[PB access returned after " );
  switch ( paramBlock->hFileInfo.ioResult )
  {
    case ioErr:
      printf( "I/O Error" );
      break;
    case fnOpnErr:
      printf( "File not Open Error" );
      break;
    case nsvErr:
      printf( "No such volume Error" );
      break;
    case fnfErr:
      printf( "File not found Error" );
      break;
    case rfNumErr:
      printf( "Bad reference number Error" );
      break;
    default:
      printf( "unexpected err=%d", paramBlock->hFileInfo.ioResult );
      break;
  }
  printf( "]\n" );
}


OSErr
ftmac_FSPathMakeSpec( const UInt8*   pathname,
                      FSSpec*        spec_p,
                      Boolean        isDirectory )
{
  const char  *p, *q;
  short       vRefNum;
  long        dirID;
  Str255      nodeName;
  OSErr       err;


  FT_UNUSED( isDirectory );
  p = q = (const char *)pathname;
  dirID   = 0;
  vRefNum = 0;

  while ( 1 )
  {
    q = p + FT_MIN( 255, ft_strlen( (char const *)p ) );

    if ( q == p )
      return 0;

    if ( 255 < ft_strlen( (char const *)pathname ) )
    {
      while ( p < q && *q != ':' )
        q--;
    }

    if ( p < q )
      nodeName[0] = q - p;
    else if ( ft_strlen( (char const *)p ) < 256 )
      nodeName[0] = ft_strlen( p );
    else
      return errFSNameTooLong;

    strncpy( (char *)nodeName + 1, (char *)p, nodeName[0] );
    nodeName[ nodeName[0] + 1 ] = '\0';
    err = FSMakeFSSpec( vRefNum, dirID, nodeName, spec_p );
    if ( err || '\0' == *q )
      return err;

    vRefNum = spec_p->vRefNum;
    dirID   = spec_p->parID;

    p = q + 1;
  }
}


void
test_font_files( int     argc,
                 char**  argv )
{
  int i;


  for ( i = 1; i < argc; i++ )
  {
    OSErr       status;
    CInfoPBRec  paramBlock;
    Str255      fileName;


    /* XXX: should be skipped by better argument handler */
    if ( '-' == argv[i][0] && '-' == argv[i][1] )
      continue;


    /* XXX: restrict pathname to legacy HFS limit for simplicity */
    if ( 254 < strlen( argv[i] ) )
      continue;

    fileName[0] = strlen( argv[i] );
    memcpy( fileName + 1, argv[i], fileName[0] );
    initParamBlock( &paramBlock, fileName );
    status = PBGetCatInfoSync( &paramBlock );
    if ( 0 > status )
      printf( "[PB access failed] error = %d\n", status );


    {
      FSSpec  spec;
      Str255  volName;
      int     i;


      strncpy( (char *)volName, (char *)fileName, fileName[0] );
      printf( "given file name [%s]", fileName + 1 );
      for ( i = 1; i < fileName[0] && ':' != fileName[i + 1]; i ++ )
        ;
      volName[i + 1] = ':';
      volName[i + 2] = '\0';
      volName[0] = i + 1;
      printf( "-> volume name [%s]", volName + 1 );
      status = FSMakeFSSpec( 0, 0, volName, &spec);
      if ( noErr != status )
        printf( "FSMakeFSSpec(%s) error %d\n", volName, status );
      else
      {
        printf( "FSMakeFSSpec(%s) return volID = 0x%08x\n", volName + 1, spec.vRefNum );
        paramBlock.hFileInfo.ioVRefNum = spec.vRefNum;
      }
    }


    if ( 0 != paramBlock.hFileInfo.ioResult )
      dumpPBErr( &paramBlock );
    else if ( 0 != ( paramBlock.hFileInfo.ioFlAttrib & ioDirMask ) )
      crawlDir( &paramBlock, argv[i] );
    else
      crawlFontFile( argv[i] );
  }
}


void
print_help_and_exit()
{
  printf("\n" );
  printf(" ftoldmac  [pathname in HFS syntax]\n" );
  printf("\n" );
  printf("           e.g. \"Macintosh HD:System Folder:Fonts:\"\n" );
  printf("           quotation is required to avoid shell expansion\n" );
  printf("           scan directory and open all font files in there\n" );
  printf("\n" );
  printf("           --max-face-number=N\n" );
  printf("           scan face until N instead of face->num_faces\n" );
  printf("\n" );
  printf("           --force-scan-face\n" );
  printf("           ignore the error to open face and continue to max face number\n" );
  printf("\n" );
  printf(" ftoldmac  --font-listing-api=XXX --font-resolve-api=YYY\n" );
  printf("\n" );
  printf("           --font-listing-api={quickdraw_old|quickdraw|ats}\n" );
  printf("           specify API to list system font\n" );
  printf("\n" );
  printf("           --font-resolve-api={quickdraw|ats}\n" );
  printf("           specify API to find fontfile by fontname\n" );
  printf("\n" );
  printf("           --auto-suffix\n" );
  printf("           old QuickDraw API cannot list available style suffixes,\n" );
  printf("           this option adds Bold and Italic suffixes automatically.\n" );
  printf("\n" );
  printf("           available API:" );
#if HAVE_QUICKDRAW_TOOLBOX
  printf(" quickdraw_old" );
#endif
#if HAVE_QUICKDRAW_CARBON
  printf(" quickdraw" );
#endif
#if HAVE_ATS
  printf(" ats" );
#endif
  printf("\n" );
  printf("\n" );
  exit( 0 );
}


void
verifyFMOutput( FMOutput*  fmout )
{
  OSErr    err;
  Handle   font_handle;
  short    rsrcID;
  ResType  rsrcType;
  Str255   rsrcName;



#ifdef VERBOSE
  printf( "FMOutput.boldPixels   0x%02x\n", fmout->boldPixels );
  printf( "FMOutput.italicPixels 0x%02x\n", fmout->italicPixels );
  printf( "FMOutput.ulOffset     0x%02x\n", fmout->ulOffset );
  printf( "FMOutput.ulShadow     0x%02x\n", fmout->ulShadow );
  printf( "FMOutput.ulThick      0x%02x\n", fmout->ulThick );
  printf( "FMOutput.shadowPixels 0x%02x\n", fmout->shadowPixels );
  printf( "FMOutput.extra        0x%02x\n", fmout->extra );
  printf( "FMOutput.curStyle     0x%02x\n", fmout->curStyle );
#else
  printf( "FMOutput.widMax:%d\n", fmout->widMax );
#endif

  font_handle = fmout->fontHandle,
  GetResInfo( font_handle, &rsrcID, &rsrcType, rsrcName );
  err = ResError();
  if ( 0 != err )
    printf( "could not get resource info for handle 0x%08x, err=%d\n",
            (int)(fmout->fontHandle), err );
  else
    printf( "resource: ID=0x%04x, Type=%c%c%c%c, name=%s\n",
             rsrcID,
             (int)MAX( (rsrcType >> 24) & 0xFF, 0x20 ),
             (int)MAX( (rsrcType >> 16) & 0xFF, 0x20 ),
             (int)MAX( (rsrcType >>  8) & 0xFF, 0x20 ),
             (int)MAX( (rsrcType >>  0) & 0xFF, 0x20 ),
             rsrcName[0] > 0 ? rsrcName + 1 : '\0' );
}


FT_OldMac_Err
resolveToolBoxQuickDrawFontName( const char*  font_name )
{
#if !HAVE_QUICKDRAW_TOOLBOX
  printf( "cannot check [%s] by Toolbox QuickDraw\n", font_name );
  return FT_OldMac_Err_Unimplemented;
#else
  Str255     familyName;
  SInt16     familyID;
  FMInput    lookup_inst;
  FMOutput*  lookup_rslt;


  ft_strncpy( (char*)familyName + 1, (char*)font_name, 254 );
  familyName[0] = strlen( (char *)familyName + 1 );
  GetFNum( familyName, &familyID );
  if ( 0 > familyID )
  {
    printf( "familyName %s is unresolvable\n", familyName + 1 );
    return FT_OldMac_Err_UnresolvableFontName;
  }
  else if ( 0 == familyID )
    return FT_OldMac_Err_PseudoFontName;

  printf( "familyName %s:%d -> familyID 0x%04x: -> ",
           familyName + 1, familyName[0],
           familyID );

  bzero( &lookup_inst, sizeof( FMInput ) );
  lookup_inst.family   = familyID;
  lookup_inst.size     = 14;
  lookup_inst.face     = 0;
  lookup_inst.needBits = FALSE;
  lookup_inst.device   = 0;
  lookup_inst.numer.v  = 1;
  lookup_inst.numer.h  = 1;
  lookup_inst.denom.v  = 1;
  lookup_inst.denom.h  = 1;

  lookup_rslt = FMSwapFont( &lookup_inst );
  if ( NULL == lookup_rslt )
  {
    printf( "FMSwapFont returns NULL (unresolved)\n" );
    return FT_OldMac_Err_UnswappableFontID;
  }
  else
    verifyFMOutput( lookup_rslt );
  return FT_OldMac_Err_Ok;
#endif
}


void
test_face_quickdraw( char*       face_name,
                     FT_Library  library    )
{
#if !HAVE_QUICKDRAW_CARBON
  printf( "cannot check [%s] by Carbon QuickDraw\n", face_name );
#else
  FSSpec   spec;
  UInt8    font_file_path[1024];
  FT_Long  face_index;
  FT_Face  face;


  printf( "Lookup [%s]...", face_name );
  if ( 0 != FT_GetFile_From_Mac_Name( face_name, &spec, &face_index ) )
  {
    printf( "FreeType could not find font file\n" );
    return;
  }

  ftmac_FSpMakePath( &spec, font_file_path, 1024 );
  printf( "Font file found [%s], face #%d...", font_file_path, (int)face_index );
  if ( 0 != FT_New_Face_From_FSSpec( library, &spec, face_index, &face ) )
  {
    printf( "FreeType could not load font file\n" );
    return;
  }
  printf( "Ok\n" );

  num_opened_faces ++;
  dump_face_info( face );
  FT_Done_Face( face );
#endif
}


void
test_face_ats( char*       face_name,
               FT_Library  library    )
{
#if !HAVE_ATS
  FT_UNUSED( library );
  printf( "cannot check [%s] by ATS\n", face_name );
#else
  UInt8    font_file_path[1024];
  FT_Long  face_index;
  FT_Face  face;
  FSSpec   spec;


  printf( "Lookup [%s]...", face_name );
  if ( 0 != FT_GetFile_From_Mac_ATS_Name( face_name, &spec, &face_index ) )
  {
    printf( "FreeType could not find font file\n" );
    return;
  }

  ftmac_FSpMakePath( &spec, font_file_path, 1024 );

  printf( "Font file found [%s], face #%d...", font_file_path, (int)face_index );

  if ( 0 != FT_New_Face_From_FSSpec( library, &spec, face_index, &face ) )
  {
    printf( "FreeType could not load font file\n" );
    return;
  }
  printf( "Ok\n" );

  num_opened_faces ++;
  dump_face_info( face );
  FT_Done_Face( face );
#endif
}


void
test_face( char*       face_name,
           FT_Library  library    )
{
  num_scanned_faces ++;
  if ( 0 == ft_strcmp( font_resolve_api, "quickdraw" ) )
    test_face_quickdraw( face_name, library );
  else if ( 0 == ft_strcmp( font_resolve_api, "ats" ) )
    test_face_ats( face_name, library );
  else
  {
    printf( "invalid api name to resolve [%s]\n", font_resolve_api );
    exit( -1 );
  }
}


void
test_font_list_quickdraw_old( FT_Library  library )
{
#if !HAVE_QUICKDRAW_TOOLBOX
  FT_UNUSED( library );
  printf( "FreeType2 is configured without quickdraw_old (Toolbox QuickDraw)\n" );
#else
  Str255  fmo_family_name;
  char    fmo_face_name[1024];
  int     i;


  for ( i = 0; i < 0x7FFF; i++ )
  {
    GetFontName( i, fmo_family_name );
    if ( 0 < fmo_family_name[0] )
    {
      fmo_family_name[ fmo_family_name[0] + 1 ] = '\0';

      ft_strncpy( (char *)fmo_face_name, (char *)fmo_family_name + 1, 1024 );
      test_face( fmo_face_name, library );

      if ( !auto_suffix )
        continue;

      ft_strncpy( (char *)fmo_face_name, (char *)fmo_family_name + 1, 1024 );
      strncat( (char *)fmo_face_name, " Bold", 1024 );
      printf( "+ " );
      test_face( fmo_face_name, library );

      ft_strncpy( (char *)fmo_face_name, (char *)fmo_family_name + 1, 1024 );
      strncat( (char *)fmo_face_name, " Italic", 1024 );
      printf( "+ " );
      test_face( fmo_face_name, library );

      ft_strncpy( (char *)fmo_face_name, (char *)fmo_family_name + 1, 1024 );
      strncat( (char *)fmo_face_name, " Bold Italic", 1024 );
      printf( "+ " );
      test_face( fmo_face_name, library );
    }
  }
#endif
}


char*
make_style_suffix( char*        fm_style_name,
                   FMFontStyle  fm_style      )
{
  fm_style_name[0] = ' ';
  fm_style_name[1] = '\0';
  if ( fm_style & bold      ) strcat( fm_style_name, "Bold " );
  if ( fm_style & italic    ) strcat( fm_style_name, "Italic " );
  if ( fm_style & underline ) strcat( fm_style_name, "Underline " );
  if ( fm_style & outline   ) strcat( fm_style_name, "Outline " );
  if ( fm_style & shadow    ) strcat( fm_style_name, "Shadow " );
  if ( fm_style & condense  ) strcat( fm_style_name, "Condense " );
  if ( fm_style & extend    ) strcat( fm_style_name, "Extend " );
  if ( ft_strlen( fm_style_name ) > 0 )
    fm_style_name[ strlen( fm_style_name) - 1 ] = '\0';

  return fm_style_name;
}


void
test_font_list_quickdraw( FT_Library  library )
{
#if !HAVE_QUICKDRAW_CARBON
  FT_UNUSED( library );
  printf( "FreeType2 is configured without quickdraw (Carbon QuickDraw)\n" );
#else
  FMFontFamilyIterator          fm_family_iter;
  FMFontFamily                  fm_family;
  Str255                        fm_family_namestr;
  char                          fm_family_name[1024];

  FMFontFamilyInstanceIterator  fm_font_iter;
  FMFont                        fm_font;
  FMFontStyle                   fm_style;
  FMFontSize                    fm_size;
  char                          fm_style_name[1024];



  if ( 0 != FMCreateFontFamilyIterator( NULL, NULL,
                                        kFMUseGlobalScopeOption,
                                        &fm_family_iter ) )
    return;

get_quickdraw_font_family:
  if ( 0 == FMGetNextFontFamily( &fm_family_iter, &fm_family ) )
  {
    if ( 0 == FMCreateFontFamilyInstanceIterator( fm_family, &fm_font_iter ) )
    {
get_quickdraw_font_instance:
      if ( 0 == FMGetNextFontFamilyInstance( &fm_font_iter, &fm_font, &fm_style, &fm_size ) )
      {
        if ( 0 == fm_size )
        {
          FMGetFontFamilyName( fm_family, fm_family_namestr );
          CopyPascalStringToC( fm_family_namestr, fm_family_name );
          strcat( fm_family_name, make_style_suffix( fm_style_name, fm_style ) );
          test_face( fm_family_name, library );
        }
        goto get_quickdraw_font_instance;
      }
      else
      {
        FMDisposeFontFamilyInstanceIterator( &fm_font_iter );
        goto get_quickdraw_font_family;
      }
    }
    else
      goto get_quickdraw_font_family;
  }

  FMDisposeFontFamilyIterator( &fm_family_iter );
  return;
#endif
}


void
test_font_list_ats( FT_Library  library )
{
#if !HAVE_ATS
  FT_UNUSED( library );
  printf( "FreeType2 is configured without ats (AppleTypeService)\n" );
#else
  ATSFontIterator  ats_font_iter;
  ATSFontRef       ats_font_ref;
  CFStringRef      ats_font_name;
  char             face_name[1024];
#ifndef kATSOptionFlagsUnRestrictedScope
#define kATSOptionFlagsUnRestrictedScope kATSOptionFlagsDefault
#endif


  if ( noErr != ATSFontIteratorCreate( kATSFontContextGlobal,
                                       NULL, NULL,
                                       kATSOptionFlagsUnRestrictedScope,
                                       &ats_font_iter ) )
    return;

  while ( noErr == ATSFontIteratorNext( ats_font_iter, &ats_font_ref ) )
  {
    if ( 0 != ATSFontGetName( ats_font_ref, kATSOptionFlagsUnRestrictedScope, &ats_font_name ) )
      continue;

    if ( NULL == ats_font_name )
      continue;

    if ( CFStringGetCString( ats_font_name, face_name, 1024, kCFStringEncodingNonLossyASCII ) )
      test_face( (char *)face_name, library );
  }
  return;
#endif
}


void
test_system_font_list()
{
  FT_Library  library;


  printf( "fontlist is generated by [%s]\n", font_listing_api );
  printf( "fontname is resolved by  [%s]\n", font_resolve_api );

  if ( 0 != FT_Init_FreeType( &library ) )
    return;

  if ( 0 == ft_strcmp( font_listing_api, "quickdraw_old" ) )
    test_font_list_quickdraw_old( library );
  else if ( 0 == ft_strcmp( font_listing_api, "quickdraw" ) )
    test_font_list_quickdraw( library );
  else if ( 0 == ft_strcmp( font_listing_api, "ats" ) )
    test_font_list_ats( library );
  else
  {
    printf( "invalid api name to list [%s]\n", font_listing_api );
    exit( -1 );
  }

  FT_Done_FreeType( library );
}


int
main( int     argc,
      char**  argv )
{
  int  i;


  num_scanned_fonts = 0;
  num_opened_fonts  = 0;
  num_scanned_faces = 0;
  num_opened_faces  = 0;

  font_listing_api = NULL;
  font_resolve_api = NULL;
  auto_suffix      = FALSE;
  max_face_number  = 0;
  force_scan_face  = FALSE;

  if ( 1 == argc )
    print_help_and_exit();
  else
  {
    for ( i = 1; i < argc; i++ )
    {
      if ( 0 == ft_strcmp( "--help", argv[i] ) )
        print_help_and_exit();
      else if ( 0 == ft_strncmp( "--font-listing-api=", argv[i], 19 ) )
        font_listing_api = argv[i] + 19;
      else if ( 0 == ft_strncmp( "--font-resolve-api=", argv[i], 19 ) )
        font_resolve_api = argv[i] + 19;
      else if ( 0 == ft_strcmp( "--auto-suffix", argv[i] ) )
        auto_suffix = TRUE;
      else if ( 0 == ft_strncmp( "--max-face-number=", argv[i], 18 ) )
        max_face_number = atoi( argv[i] + 18 );
      else if ( 0 == ft_strcmp( "--force-scan-face", argv[i] ) )
        force_scan_face = TRUE;
    }
  }

  if ( NULL == font_listing_api && NULL == font_resolve_api )
    test_font_files( argc, argv );
  else if ( NULL != font_listing_api && NULL != font_resolve_api )
    test_system_font_list( argc, argv );
  else if ( NULL == font_listing_api )
    printf( "require --font-listing-api to specify how to list system font\n" );
  else if ( NULL == font_resolve_api )
    printf( "require --font-resolve-api to specify how to find file by fontname\n" );

  printf( "\n" );
  printf( "---------------------------------\n" );
  printf( "Summary\n" );
  printf( "Font File opened/scanned = %d/%d\n", num_opened_fonts, num_scanned_fonts );
  printf( "Font Face opened/scanned = %d/%d\n", num_opened_faces, num_scanned_faces );
  exit( 0 );
}
