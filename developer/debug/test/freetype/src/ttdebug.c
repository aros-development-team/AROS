/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2018 by                                                  */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ttdebug - a simple TrueType debugger for the console.                   */
/*                                                                          */
/****************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#ifdef UNIX

#ifndef HAVE_POSIX_TERMIOS

#include <sys/ioctl.h>
#include <termio.h>

#else /* HAVE_POSIX_TERMIOS */

#ifndef HAVE_TCGETATTR
#define HAVE_TCGETATTR
#endif

#ifndef HAVE_TCSETATTR
#define HAVE_TCSETATTR
#endif

#include <termios.h>

#endif /* HAVE_POSIX_TERMIOS */

#endif /* UNIX */


  /* Define the `getch()' function.  On Unix systems, it is an alias  */
  /* for `getchar()', and the debugger front end must ensure that the */
  /* `stdin' file descriptor is not in line-by-line input mode.       */
#ifdef _WIN32
#include <conio.h>
#define snprintf  _snprintf
#define getch     _getch
#else
#define getch     getchar
#endif


#include <ft2build.h>
#include FT_FREETYPE_H
#include "common.h"
#include "mlgetopt.h"

#include FT_DRIVER_H

  /* The following header shouldn't be used in normal programs.    */
  /* `freetype2/src/truetype' must be in the current include path. */
#include "ttobjs.h"
#include "ttdriver.h"
#include "ttinterp.h"
#include "tterrors.h"


#define Quit     -1
#define Restart  -2


  static FT_Library    library;    /* root library object */
  static FT_Memory     memory;     /* system object       */
  static FT_Driver     driver;     /* truetype driver     */
  static TT_Face       face;       /* truetype face       */
  static TT_Size       size;       /* truetype size       */
  static TT_GlyphSlot  glyph;      /* truetype glyph slot */

  static unsigned int  tt_interpreter_versions[3];
  static int           num_tt_interpreter_versions;
  static unsigned int  dflt_tt_interpreter_version;

  /* number formats */
  static FT_Bool  use_float = 0; /* for points                   */
  static FT_Bool  use_hex   = 1; /* for integers (except points) */

  static FT_Error  error;


  typedef char  ByteStr[2];
  typedef char  WordStr[4];
  typedef char  LongStr[8];
  typedef char  DebugStr[128];

  static DebugStr  tempStr;


  typedef struct  Storage_
  {
    FT_Bool  initialized;
    FT_Long  value;

  } Storage;


  typedef struct  Breakpoint_
  {
    FT_Long  IP;
    FT_Int   range;

  } Breakpoint;

  /* right now, we support a single breakpoint only */
  static Breakpoint  breakpoint;


#undef  PACK
#define PACK( x, y )  ( ( x << 4 ) | y )

  static const FT_Byte  Pop_Push_Count[256] =
  {
    /* Opcodes are gathered in groups of 16. */
    /* Please keep the spaces as they are.   */

    /*  SVTCA  y  */  PACK( 0, 0 ),
    /*  SVTCA  x  */  PACK( 0, 0 ),
    /*  SPvTCA y  */  PACK( 0, 0 ),
    /*  SPvTCA x  */  PACK( 0, 0 ),
    /*  SFvTCA y  */  PACK( 0, 0 ),
    /*  SFvTCA x  */  PACK( 0, 0 ),
    /*  SPvTL //  */  PACK( 2, 0 ),
    /*  SPvTL +   */  PACK( 2, 0 ),
    /*  SFvTL //  */  PACK( 2, 0 ),
    /*  SFvTL +   */  PACK( 2, 0 ),
    /*  SPvFS     */  PACK( 2, 0 ),
    /*  SFvFS     */  PACK( 2, 0 ),
    /*  GPV       */  PACK( 0, 2 ),
    /*  GFV       */  PACK( 0, 2 ),
    /*  SFvTPv    */  PACK( 0, 0 ),
    /*  ISECT     */  PACK( 5, 0 ),

    /*  SRP0      */  PACK( 1, 0 ),
    /*  SRP1      */  PACK( 1, 0 ),
    /*  SRP2      */  PACK( 1, 0 ),
    /*  SZP0      */  PACK( 1, 0 ),
    /*  SZP1      */  PACK( 1, 0 ),
    /*  SZP2      */  PACK( 1, 0 ),
    /*  SZPS      */  PACK( 1, 0 ),
    /*  SLOOP     */  PACK( 1, 0 ),
    /*  RTG       */  PACK( 0, 0 ),
    /*  RTHG      */  PACK( 0, 0 ),
    /*  SMD       */  PACK( 1, 0 ),
    /*  ELSE      */  PACK( 0, 0 ),
    /*  JMPR      */  PACK( 1, 0 ),
    /*  SCvTCi    */  PACK( 1, 0 ),
    /*  SSwCi     */  PACK( 1, 0 ),
    /*  SSW       */  PACK( 1, 0 ),

    /*  DUP       */  PACK( 1, 2 ),
    /*  POP       */  PACK( 1, 0 ),
    /*  CLEAR     */  PACK( 0, 0 ),
    /*  SWAP      */  PACK( 2, 2 ),
    /*  DEPTH     */  PACK( 0, 1 ),
    /*  CINDEX    */  PACK( 1, 1 ),
    /*  MINDEX    */  PACK( 1, 0 ),
    /*  AlignPTS  */  PACK( 2, 0 ),
    /*  INS_$28   */  PACK( 0, 0 ),
    /*  UTP       */  PACK( 1, 0 ),
    /*  LOOPCALL  */  PACK( 2, 0 ),
    /*  CALL      */  PACK( 1, 0 ),
    /*  FDEF      */  PACK( 1, 0 ),
    /*  ENDF      */  PACK( 0, 0 ),
    /*  MDAP[0]   */  PACK( 1, 0 ),
    /*  MDAP[1]   */  PACK( 1, 0 ),

    /*  IUP[0]    */  PACK( 0, 0 ),
    /*  IUP[1]    */  PACK( 0, 0 ),
    /*  SHP[0]    */  PACK( 0, 0 ),
    /*  SHP[1]    */  PACK( 0, 0 ),
    /*  SHC[0]    */  PACK( 1, 0 ),
    /*  SHC[1]    */  PACK( 1, 0 ),
    /*  SHZ[0]    */  PACK( 1, 0 ),
    /*  SHZ[1]    */  PACK( 1, 0 ),
    /*  SHPIX     */  PACK( 1, 0 ),
    /*  IP        */  PACK( 0, 0 ),
    /*  MSIRP[0]  */  PACK( 2, 0 ),
    /*  MSIRP[1]  */  PACK( 2, 0 ),
    /*  AlignRP   */  PACK( 0, 0 ),
    /*  RTDG      */  PACK( 0, 0 ),
    /*  MIAP[0]   */  PACK( 2, 0 ),
    /*  MIAP[1]   */  PACK( 2, 0 ),

    /*  NPushB    */  PACK( 0, 0 ),
    /*  NPushW    */  PACK( 0, 0 ),
    /*  WS        */  PACK( 2, 0 ),
    /*  RS        */  PACK( 1, 1 ),
    /*  WCvtP     */  PACK( 2, 0 ),
    /*  RCvt      */  PACK( 1, 1 ),
    /*  GC[0]     */  PACK( 1, 1 ),
    /*  GC[1]     */  PACK( 1, 1 ),
    /*  SCFS      */  PACK( 2, 0 ),
    /*  MD[0]     */  PACK( 2, 1 ),
    /*  MD[1]     */  PACK( 2, 1 ),
    /*  MPPEM     */  PACK( 0, 1 ),
    /*  MPS       */  PACK( 0, 1 ),
    /*  FlipON    */  PACK( 0, 0 ),
    /*  FlipOFF   */  PACK( 0, 0 ),
    /*  DEBUG     */  PACK( 1, 0 ),

    /*  LT        */  PACK( 2, 1 ),
    /*  LTEQ      */  PACK( 2, 1 ),
    /*  GT        */  PACK( 2, 1 ),
    /*  GTEQ      */  PACK( 2, 1 ),
    /*  EQ        */  PACK( 2, 1 ),
    /*  NEQ       */  PACK( 2, 1 ),
    /*  ODD       */  PACK( 1, 1 ),
    /*  EVEN      */  PACK( 1, 1 ),
    /*  IF        */  PACK( 1, 0 ),
    /*  EIF       */  PACK( 0, 0 ),
    /*  AND       */  PACK( 2, 1 ),
    /*  OR        */  PACK( 2, 1 ),
    /*  NOT       */  PACK( 1, 1 ),
    /*  DeltaP1   */  PACK( 1, 0 ),
    /*  SDB       */  PACK( 1, 0 ),
    /*  SDS       */  PACK( 1, 0 ),

    /*  ADD       */  PACK( 2, 1 ),
    /*  SUB       */  PACK( 2, 1 ),
    /*  DIV       */  PACK( 2, 1 ),
    /*  MUL       */  PACK( 2, 1 ),
    /*  ABS       */  PACK( 1, 1 ),
    /*  NEG       */  PACK( 1, 1 ),
    /*  FLOOR     */  PACK( 1, 1 ),
    /*  CEILING   */  PACK( 1, 1 ),
    /*  ROUND[0]  */  PACK( 1, 1 ),
    /*  ROUND[1]  */  PACK( 1, 1 ),
    /*  ROUND[2]  */  PACK( 1, 1 ),
    /*  ROUND[3]  */  PACK( 1, 1 ),
    /*  NROUND[0] */  PACK( 1, 1 ),
    /*  NROUND[1] */  PACK( 1, 1 ),
    /*  NROUND[2] */  PACK( 1, 1 ),
    /*  NROUND[3] */  PACK( 1, 1 ),

    /*  WCvtF     */  PACK( 2, 0 ),
    /*  DeltaP2   */  PACK( 1, 0 ),
    /*  DeltaP3   */  PACK( 1, 0 ),
    /*  DeltaCn[0] */ PACK( 1, 0 ),
    /*  DeltaCn[1] */ PACK( 1, 0 ),
    /*  DeltaCn[2] */ PACK( 1, 0 ),
    /*  SROUND    */  PACK( 1, 0 ),
    /*  S45Round  */  PACK( 1, 0 ),
    /*  JROT      */  PACK( 2, 0 ),
    /*  JROF      */  PACK( 2, 0 ),
    /*  ROFF      */  PACK( 0, 0 ),
    /*  INS_$7B   */  PACK( 0, 0 ),
    /*  RUTG      */  PACK( 0, 0 ),
    /*  RDTG      */  PACK( 0, 0 ),
    /*  SANGW     */  PACK( 1, 0 ),
    /*  AA        */  PACK( 1, 0 ),

    /*  FlipPT    */  PACK( 0, 0 ),
    /*  FlipRgON  */  PACK( 2, 0 ),
    /*  FlipRgOFF */  PACK( 2, 0 ),
    /*  INS_$83   */  PACK( 0, 0 ),
    /*  INS_$84   */  PACK( 0, 0 ),
    /*  ScanCTRL  */  PACK( 1, 0 ),
    /*  SDVPTL[0] */  PACK( 2, 0 ),
    /*  SDVPTL[1] */  PACK( 2, 0 ),
    /*  GetINFO   */  PACK( 1, 1 ),
    /*  IDEF      */  PACK( 1, 0 ),
    /*  ROLL      */  PACK( 3, 3 ),
    /*  MAX       */  PACK( 2, 1 ),
    /*  MIN       */  PACK( 2, 1 ),
    /*  ScanTYPE  */  PACK( 1, 0 ),
    /*  InstCTRL  */  PACK( 2, 0 ),
    /*  INS_$8F   */  PACK( 0, 0 ),

    /*  INS_$90  */   PACK( 0, 0 ),
    /*  INS_$91  */   PACK( 0, 0 ),
    /*  INS_$92  */   PACK( 0, 0 ),
    /*  INS_$93  */   PACK( 0, 0 ),
    /*  INS_$94  */   PACK( 0, 0 ),
    /*  INS_$95  */   PACK( 0, 0 ),
    /*  INS_$96  */   PACK( 0, 0 ),
    /*  INS_$97  */   PACK( 0, 0 ),
    /*  INS_$98  */   PACK( 0, 0 ),
    /*  INS_$99  */   PACK( 0, 0 ),
    /*  INS_$9A  */   PACK( 0, 0 ),
    /*  INS_$9B  */   PACK( 0, 0 ),
    /*  INS_$9C  */   PACK( 0, 0 ),
    /*  INS_$9D  */   PACK( 0, 0 ),
    /*  INS_$9E  */   PACK( 0, 0 ),
    /*  INS_$9F  */   PACK( 0, 0 ),

    /*  INS_$A0  */   PACK( 0, 0 ),
    /*  INS_$A1  */   PACK( 0, 0 ),
    /*  INS_$A2  */   PACK( 0, 0 ),
    /*  INS_$A3  */   PACK( 0, 0 ),
    /*  INS_$A4  */   PACK( 0, 0 ),
    /*  INS_$A5  */   PACK( 0, 0 ),
    /*  INS_$A6  */   PACK( 0, 0 ),
    /*  INS_$A7  */   PACK( 0, 0 ),
    /*  INS_$A8  */   PACK( 0, 0 ),
    /*  INS_$A9  */   PACK( 0, 0 ),
    /*  INS_$AA  */   PACK( 0, 0 ),
    /*  INS_$AB  */   PACK( 0, 0 ),
    /*  INS_$AC  */   PACK( 0, 0 ),
    /*  INS_$AD  */   PACK( 0, 0 ),
    /*  INS_$AE  */   PACK( 0, 0 ),
    /*  INS_$AF  */   PACK( 0, 0 ),

    /*  PushB[0]  */  PACK( 0, 1 ),
    /*  PushB[1]  */  PACK( 0, 2 ),
    /*  PushB[2]  */  PACK( 0, 3 ),
    /*  PushB[3]  */  PACK( 0, 4 ),
    /*  PushB[4]  */  PACK( 0, 5 ),
    /*  PushB[5]  */  PACK( 0, 6 ),
    /*  PushB[6]  */  PACK( 0, 7 ),
    /*  PushB[7]  */  PACK( 0, 8 ),
    /*  PushW[0]  */  PACK( 0, 1 ),
    /*  PushW[1]  */  PACK( 0, 2 ),
    /*  PushW[2]  */  PACK( 0, 3 ),
    /*  PushW[3]  */  PACK( 0, 4 ),
    /*  PushW[4]  */  PACK( 0, 5 ),
    /*  PushW[5]  */  PACK( 0, 6 ),
    /*  PushW[6]  */  PACK( 0, 7 ),
    /*  PushW[7]  */  PACK( 0, 8 ),

    /*  MDRP[00]  */  PACK( 1, 0 ),
    /*  MDRP[01]  */  PACK( 1, 0 ),
    /*  MDRP[02]  */  PACK( 1, 0 ),
    /*  MDRP[03]  */  PACK( 1, 0 ),
    /*  MDRP[04]  */  PACK( 1, 0 ),
    /*  MDRP[05]  */  PACK( 1, 0 ),
    /*  MDRP[06]  */  PACK( 1, 0 ),
    /*  MDRP[07]  */  PACK( 1, 0 ),
    /*  MDRP[08]  */  PACK( 1, 0 ),
    /*  MDRP[09]  */  PACK( 1, 0 ),
    /*  MDRP[10]  */  PACK( 1, 0 ),
    /*  MDRP[11]  */  PACK( 1, 0 ),
    /*  MDRP[12]  */  PACK( 1, 0 ),
    /*  MDRP[13]  */  PACK( 1, 0 ),
    /*  MDRP[14]  */  PACK( 1, 0 ),
    /*  MDRP[15]  */  PACK( 1, 0 ),

    /*  MDRP[16]  */  PACK( 1, 0 ),
    /*  MDRP[17]  */  PACK( 1, 0 ),
    /*  MDRP[18]  */  PACK( 1, 0 ),
    /*  MDRP[19]  */  PACK( 1, 0 ),
    /*  MDRP[20]  */  PACK( 1, 0 ),
    /*  MDRP[21]  */  PACK( 1, 0 ),
    /*  MDRP[22]  */  PACK( 1, 0 ),
    /*  MDRP[23]  */  PACK( 1, 0 ),
    /*  MDRP[24]  */  PACK( 1, 0 ),
    /*  MDRP[25]  */  PACK( 1, 0 ),
    /*  MDRP[26]  */  PACK( 1, 0 ),
    /*  MDRP[27]  */  PACK( 1, 0 ),
    /*  MDRP[28]  */  PACK( 1, 0 ),
    /*  MDRP[29]  */  PACK( 1, 0 ),
    /*  MDRP[30]  */  PACK( 1, 0 ),
    /*  MDRP[31]  */  PACK( 1, 0 ),

    /*  MIRP[00]  */  PACK( 2, 0 ),
    /*  MIRP[01]  */  PACK( 2, 0 ),
    /*  MIRP[02]  */  PACK( 2, 0 ),
    /*  MIRP[03]  */  PACK( 2, 0 ),
    /*  MIRP[04]  */  PACK( 2, 0 ),
    /*  MIRP[05]  */  PACK( 2, 0 ),
    /*  MIRP[06]  */  PACK( 2, 0 ),
    /*  MIRP[07]  */  PACK( 2, 0 ),
    /*  MIRP[08]  */  PACK( 2, 0 ),
    /*  MIRP[09]  */  PACK( 2, 0 ),
    /*  MIRP[10]  */  PACK( 2, 0 ),
    /*  MIRP[11]  */  PACK( 2, 0 ),
    /*  MIRP[12]  */  PACK( 2, 0 ),
    /*  MIRP[13]  */  PACK( 2, 0 ),
    /*  MIRP[14]  */  PACK( 2, 0 ),
    /*  MIRP[15]  */  PACK( 2, 0 ),

    /*  MIRP[16]  */  PACK( 2, 0 ),
    /*  MIRP[17]  */  PACK( 2, 0 ),
    /*  MIRP[18]  */  PACK( 2, 0 ),
    /*  MIRP[19]  */  PACK( 2, 0 ),
    /*  MIRP[20]  */  PACK( 2, 0 ),
    /*  MIRP[21]  */  PACK( 2, 0 ),
    /*  MIRP[22]  */  PACK( 2, 0 ),
    /*  MIRP[23]  */  PACK( 2, 0 ),
    /*  MIRP[24]  */  PACK( 2, 0 ),
    /*  MIRP[25]  */  PACK( 2, 0 ),
    /*  MIRP[26]  */  PACK( 2, 0 ),
    /*  MIRP[27]  */  PACK( 2, 0 ),
    /*  MIRP[28]  */  PACK( 2, 0 ),
    /*  MIRP[29]  */  PACK( 2, 0 ),
    /*  MIRP[30]  */  PACK( 2, 0 ),
    /*  MIRP[31]  */  PACK( 2, 0 )
  };


  static const FT_String*  OpStr[256] =
  {
    "SVTCA y",       /* set vectors to coordinate axis y    */
    "SVTCA x",       /* set vectors to coordinate axis x    */
    "SPVTCA y",      /* set proj. vec. to coord. axis y     */
    "SPVTCA x",      /* set proj. vec. to coord. axis x     */
    "SFVTCA y",      /* set free. vec. to coord. axis y     */
    "SFVTCA x",      /* set free. vec. to coord. axis x     */
    "SPVTL ||",      /* set proj. vec. parallel to segment  */
    "SPVTL +",       /* set proj. vec. normal to segment    */
    "SFVTL ||",      /* set free. vec. parallel to segment  */
    "SFVTL +",       /* set free. vec. normal to segment    */
    "SPVFS",         /* set proj. vec. from stack           */
    "SFVFS",         /* set free. vec. from stack           */
    "GPV",           /* get projection vector               */
    "GFV",           /* get freedom vector                  */
    "SFVTPV",        /* set free. vec. to proj. vec.        */
    "ISECT",         /* compute intersection                */

    "SRP0",          /* set reference point 0               */
    "SRP1",          /* set reference point 1               */
    "SRP2",          /* set reference point 2               */
    "SZP0",          /* set zone pointer 0                  */
    "SZP1",          /* set zone pointer 1                  */
    "SZP2",          /* set zone pointer 2                  */
    "SZPS",          /* set all zone pointers               */
    "SLOOP",         /* set loop counter                    */
    "RTG",           /* round to grid                       */
    "RTHG",          /* round to half-grid                  */
    "SMD",           /* set minimum distance                */
    "ELSE",          /* else                                */
    "JMPR",          /* jump relative                       */
    "SCVTCI",        /* set CVT cut-in                      */
    "SSWCI",         /* set single width cut-in             */
    "SSW",           /* set single width                    */

    "DUP",           /*                                     */
    "POP",           /*                                     */
    "CLEAR",         /*                                     */
    "SWAP",          /*                                     */
    "DEPTH",         /*                                     */
    "CINDEX",        /*                                     */
    "MINDEX",        /*                                     */
    "AlignPTS",      /*                                     */
    "INS_$28",
    "UTP",           /*                                     */
    "LOOPCALL",      /*                                     */
    "CALL",          /*                                     */
    "FDEF",          /*                                     */
    "ENDF",          /*                                     */
    "MDAP[0]",       /*                                     */
    "MDAP[1]",       /*                                     */

    "IUP[0]",        /*                                     */
    "IUP[1]",        /*                                     */
    "SHP[0]",        /*                                     */
    "SHP[1]",        /*                                     */
    "SHC[0]",        /*                                     */
    "SHC[1]",        /*                                     */
    "SHZ[0]",        /*                                     */
    "SHZ[1]",        /*                                     */
    "SHPIX",         /*                                     */
    "IP",            /*                                     */
    "MSIRP[0]",      /*                                     */
    "MSIRP[1]",      /*                                     */
    "AlignRP",       /*                                     */
    "RTDG",          /*                                     */
    "MIAP[0]",       /*                                     */
    "MIAP[1]",       /*                                     */

    "NPushB",        /*                                     */
    "NPushW",        /*                                     */
    "WS",            /*                                     */
    "RS",            /*                                     */
    "WCvtP",         /*                                     */
    "RCvt",          /*                                     */
    "GC[0]",         /*                                     */
    "GC[1]",         /*                                     */
    "SCFS",          /*                                     */
    "MD[0]",         /*                                     */
    "MD[1]",         /*                                     */
    "MPPEM",         /*                                     */
    "MPS",           /*                                     */
    "FlipON",        /*                                     */
    "FlipOFF",       /*                                     */
    "DEBUG",         /*                                     */

    "LT",            /*                                     */
    "LTEQ",          /*                                     */
    "GT",            /*                                     */
    "GTEQ",          /*                                     */
    "EQ",            /*                                     */
    "NEQ",           /*                                     */
    "ODD",           /*                                     */
    "EVEN",          /*                                     */
    "IF",            /*                                     */
    "EIF",           /*                                     */
    "AND",           /*                                     */
    "OR",            /*                                     */
    "NOT",           /*                                     */
    "DeltaP1",       /*                                     */
    "SDB",           /*                                     */
    "SDS",           /*                                     */

    "ADD",           /*                                     */
    "SUB",           /*                                     */
    "DIV",           /*                                     */
    "MUL",           /*                                     */
    "ABS",           /*                                     */
    "NEG",           /*                                     */
    "FLOOR",         /*                                     */
    "CEILING",       /*                                     */
    "ROUND[0]",      /*                                     */
    "ROUND[1]",      /*                                     */
    "ROUND[2]",      /*                                     */
    "ROUND[3]",      /*                                     */
    "NROUND[0]",     /*                                     */
    "NROUND[1]",     /*                                     */
    "NROUND[2]",     /*                                     */
    "NROUND[3]",     /*                                     */

    "WCvtF",         /*                                     */
    "DeltaP2",       /*                                     */
    "DeltaP3",       /*                                     */
    "DeltaC1",       /*                                     */
    "DeltaC2",       /*                                     */
    "DeltaC3",       /*                                     */
    "SROUND",        /*                                     */
    "S45Round",      /*                                     */
    "JROT",          /*                                     */
    "JROF",          /*                                     */
    "ROFF",          /*                                     */
    "INS_$7B",
    "RUTG",          /*                                     */
    "RDTG",          /*                                     */
    "SANGW",         /*                                     */
    "AA",            /*                                     */

    "FlipPT",        /*                                     */
    "FlipRgON",      /*                                     */
    "FlipRgOFF",     /*                                     */
    "INS_$83",
    "INS_$84",
    "ScanCTRL",      /*                                     */
    "SDPVTL[0]",     /*                                     */
    "SDPVTL[1]",     /*                                     */
    "GetINFO",       /*                                     */
    "IDEF",          /*                                     */
    "ROLL",          /*                                     */
    "MAX",           /*                                     */
    "MIN",           /*                                     */
    "ScanTYPE",      /*                                     */
    "InstCTRL",      /*                                     */
    "INS_$8F",

    "INS_$90",
    "INS_$91",
    "INS_$92",
    "INS_$93",
    "INS_$94",
    "INS_$95",
    "INS_$96",
    "INS_$97",
    "INS_$98",
    "INS_$99",
    "INS_$9A",
    "INS_$9B",
    "INS_$9C",
    "INS_$9D",
    "INS_$9E",
    "INS_$9F",

    "INS_$A0",
    "INS_$A1",
    "INS_$A2",
    "INS_$A3",
    "INS_$A4",
    "INS_$A5",
    "INS_$A6",
    "INS_$A7",
    "INS_$A8",
    "INS_$A9",
    "INS_$AA",
    "INS_$AB",
    "INS_$AC",
    "INS_$AD",
    "INS_$AE",
    "INS_$AF",

    "PushB[0]",      /*                                     */
    "PushB[1]",      /*                                     */
    "PushB[2]",      /*                                     */
    "PushB[3]",      /*                                     */
    "PushB[4]",      /*                                     */
    "PushB[5]",      /*                                     */
    "PushB[6]",      /*                                     */
    "PushB[7]",      /*                                     */
    "PushW[0]",      /*                                     */
    "PushW[1]",      /*                                     */
    "PushW[2]",      /*                                     */
    "PushW[3]",      /*                                     */
    "PushW[4]",      /*                                     */
    "PushW[5]",      /*                                     */
    "PushW[6]",      /*                                     */
    "PushW[7]",      /*                                     */

    "MDRP[G]",       /*                                     */
    "MDRP[B]",       /*                                     */
    "MDRP[W]",       /*                                     */
    "MDRP[?]",       /*                                     */
    "MDRP[rG]",      /*                                     */
    "MDRP[rB]",      /*                                     */
    "MDRP[rW]",      /*                                     */
    "MDRP[r?]",      /*                                     */
    "MDRP[mG]",      /*                                     */
    "MDRP[mB]",      /*                                     */
    "MDRP[mW]",      /*                                     */
    "MDRP[m?]",      /*                                     */
    "MDRP[mrG]",     /*                                     */
    "MDRP[mrB]",     /*                                     */
    "MDRP[mrW]",     /*                                     */
    "MDRP[mr?]",     /*                                     */
    "MDRP[pG]",      /*                                     */
    "MDRP[pB]",      /*                                     */

    "MDRP[pW]",      /*                                     */
    "MDRP[p?]",      /*                                     */
    "MDRP[prG]",     /*                                     */
    "MDRP[prB]",     /*                                     */
    "MDRP[prW]",     /*                                     */
    "MDRP[pr?]",     /*                                     */
    "MDRP[pmG]",     /*                                     */
    "MDRP[pmB]",     /*                                     */
    "MDRP[pmW]",     /*                                     */
    "MDRP[pm?]",     /*                                     */
    "MDRP[pmrG]",    /*                                     */
    "MDRP[pmrB]",    /*                                     */
    "MDRP[pmrW]",    /*                                     */
    "MDRP[pmr?]",    /*                                     */

    "MIRP[G]",       /*                                     */
    "MIRP[B]",       /*                                     */
    "MIRP[W]",       /*                                     */
    "MIRP[?]",       /*                                     */
    "MIRP[rG]",      /*                                     */
    "MIRP[rB]",      /*                                     */
    "MIRP[rW]",      /*                                     */
    "MIRP[r?]",      /*                                     */
    "MIRP[mG]",      /*                                     */
    "MIRP[mB]",      /*                                     */
    "MIRP[mW]",      /*                                     */
    "MIRP[m?]",      /*                                     */
    "MIRP[mrG]",     /*                                     */
    "MIRP[mrB]",     /*                                     */
    "MIRP[mrW]",     /*                                     */
    "MIRP[mr?]",     /*                                     */
    "MIRP[pG]",      /*                                     */
    "MIRP[pB]",      /*                                     */

    "MIRP[pW]",      /*                                     */
    "MIRP[p?]",      /*                                     */
    "MIRP[prG]",     /*                                     */
    "MIRP[prB]",     /*                                     */
    "MIRP[prW]",     /*                                     */
    "MIRP[pr?]",     /*                                     */
    "MIRP[pmG]",     /*                                     */
    "MIRP[pmB]",     /*                                     */
    "MIRP[pmW]",     /*                                     */
    "MIRP[pm?]",     /*                                     */
    "MIRP[pmrG]",    /*                                     */
    "MIRP[pmrB]",    /*                                     */
    "MIRP[pmrW]",    /*                                     */
    "MIRP[pmr?]"     /*                                     */
  };


  /*********************************************************************
   *
   * Init_Keyboard: Set the input file descriptor to char-by-char
   *                mode on Unix.
   *
   *********************************************************************/

#ifdef UNIX

  static struct termios  old_termio;


  static void
  Init_Keyboard( void )
  {
    struct termios  termio;


#ifndef HAVE_TCGETATTR
    ioctl( 0, TCGETS, &old_termio );
#else
    tcgetattr( 0, &old_termio );
#endif

    termio = old_termio;

#if 0
    termio.c_lflag &= (tcflag_t)~( ICANON + ECHO + ECHOE + ECHOK + ECHONL + ECHOKE );
#else
    termio.c_lflag &= (tcflag_t)~( ICANON + ECHO + ECHOE + ECHOK + ECHONL );
#endif

#ifndef HAVE_TCSETATTR
    ioctl( 0, TCSETS, &termio );
#else
    tcsetattr( 0, TCSANOW, &termio );
#endif
  }


  static void
  Reset_Keyboard( void )
  {
#ifndef HAVE_TCSETATTR
    ioctl( 0, TCSETS, &old_termio );
#else
    tcsetattr( 0, TCSANOW, &old_termio );
#endif
  }

#else /* !UNIX */

  static void
  Init_Keyboard( void )
  {
  }

  static void
  Reset_Keyboard( void )
  {
  }

#endif /* !UNIX */


  static void
  Abort( const char*  message )
  {
    fprintf( stderr, "%s\n  error code = 0x%04x.\n", message, error );
    Reset_Keyboard();
    exit( 1 );
  }


  /******************************************************************
   *
   *  Function:    Calc_Length
   *
   *  Description: Compute the length in bytes of current opcode.
   *
   *****************************************************************/

#define CUR  (*exc)


  static void
  Calc_Length( TT_ExecContext  exc )
  {
    CUR.opcode = CUR.code[CUR.IP];

    switch ( CUR.opcode )
    {
    case 0x40:
      if ( CUR.IP + 1 >= CUR.codeSize )
        Abort( "code range overflow!" );

      CUR.length = CUR.code[CUR.IP + 1] + 2;
      break;

    case 0x41:
      if ( CUR.IP + 1 >= CUR.codeSize )
        Abort( "code range overflow!" );

      CUR.length = CUR.code[CUR.IP + 1] * 2 + 2;
      break;

    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0xB6:
    case 0xB7:
      CUR.length = CUR.opcode - 0xB0 + 2;
      break;

    case 0xB8:
    case 0xB9:
    case 0xBA:
    case 0xBB:
    case 0xBC:
    case 0xBD:
    case 0xBE:
    case 0xBF:
      CUR.length = ( CUR.opcode - 0xB8 ) * 2 + 3;
      break;

    default:
      CUR.length = 1;
      break;
    }

    /* make sure result is in range */

    if ( CUR.IP + CUR.length > CUR.codeSize )
      Abort( "code range overflow!" );
  }


  /* Disassemble the current line. */
  /*                               */
  static const FT_String*
  Cur_U_Line( TT_ExecContext  exc )
  {
    FT_String  s[32];
    FT_Int     op, i, n;


    op = CUR.code[CUR.IP];

    sprintf( tempStr, "%s", OpStr[op] );

    if ( op == 0x40 )
    {
      n = CUR.code[CUR.IP + 1];
      sprintf( s, "(%d)", n );
      strncat( tempStr, s, 8 );

      /* limit output */
      if ( n > 20 )
        n = 20;

      for ( i = 0; i < n; i++ )
      {
        const FT_String*  temp;


        temp = use_hex ? " $%02x" : " %d";
        sprintf( s, temp, CUR.code[CUR.IP + i + 2] );
        strncat( tempStr, s, 8 );
      }
    }
    else if ( op == 0x41 )
    {
      n = CUR.code[CUR.IP + 1];
      sprintf( s, "(%d)", n );
      strncat( tempStr, s, 8 );

      /* limit output */
      if ( n > 20 )
        n = 20;

      for ( i = 0; i < n; i++ )
      {
        if ( use_hex )
          sprintf( s, " $%02x%02x",
                      CUR.code[CUR.IP + i * 2 + 2],
                      CUR.code[CUR.IP + i * 2 + 3] );
        else
        {
          unsigned short  temp;


          temp = (unsigned short)( ( CUR.code[CUR.IP + i * 2 + 2] << 8 ) +
                                     CUR.code[CUR.IP + i * 2 + 3]        );
          sprintf( s, " %d",
                      (signed short)temp );
        }
        strncat( tempStr, s, 8 );
      }
    }
    else if ( ( op & 0xF8 ) == 0xB0 )
    {
      n = op - 0xB0;

      for ( i = 0; i <= n; i++ )
      {
        const FT_String*  temp;


        temp = use_hex ? " $%02x" : " %d";
        sprintf( s, temp, CUR.code[CUR.IP + i + 1] );
        strncat( tempStr, s, 8 );
      }
    }
    else if ( ( op & 0xF8 ) == 0xB8 )
    {
      n = op - 0xB8;

      for ( i = 0; i <= n; i++ )
      {
        if ( use_hex )
          sprintf( s, " $%02x%02x",
                      CUR.code[CUR.IP + i * 2 + 1],
                      CUR.code[CUR.IP + i * 2 + 2] );
        else
        {
          unsigned short  temp;


          temp = (unsigned short)( ( CUR.code[CUR.IP + i * 2 + 1] << 8 ) +
                                     CUR.code[CUR.IP + i * 2 + 2]        );
          sprintf( s, " %d",
                      (signed short)temp );
        }
        strncat( tempStr, s, 8 );
      }
    }
    else if ( op == 0x39 )  /* IP */
    {
      sprintf( s, " rp1=%d, rp2=%d", CUR.GS.rp1, CUR.GS.rp2 );
      strncat( tempStr, s, 31 );
    }

    return (FT_String*)tempStr;
  }


  /* we have to track the `WS' opcode specially so that we are able */
  /* to properly handle uninitialized storage area values           */
  static void
  handle_WS( TT_ExecContext  exc,
             Storage*        storage )
  {
    if ( CUR.opcode == 0x42 && CUR.top >= 2 )
    {
      FT_ULong  idx   = (FT_ULong)CUR.stack[CUR.top - 2];
      FT_Long   value = (FT_Long) CUR.stack[CUR.top - 1];


      if ( idx < CUR.storeSize )
      {
        storage[idx].initialized = 1;
        storage[idx].value       = value;
      }
    }
  }


  static void
  display_changed_points( TT_GlyphZoneRec*  prev,
                          TT_GlyphZoneRec*  curr,
                          FT_Bool           is_twilight )
  {
    FT_Int  A;


    for ( A = 0; A < curr->n_points; A++ )
    {
      FT_Int  diff = 0;


      if ( prev->org[A].x != curr->org[A].x )
        diff |= 1;
      if ( prev->org[A].y != curr->org[A].y )
        diff |= 2;
      if ( prev->cur[A].x != curr->cur[A].x )
        diff |= 4;
      if ( prev->cur[A].y != curr->cur[A].y )
        diff |= 8;
      if ( prev->tags[A] != curr->tags[A] )
        diff |= 16;

      if ( diff )
      {
        const FT_String*  temp;


        printf( "%3d%s ", A, is_twilight ? "T" : " " );
        printf( "%6ld,%6ld  ", curr->orus[A].x, curr->orus[A].y );

        if ( diff & 16 )
          temp = "(%c%c%c)";
        else
          temp = " %c%c%c ";
        printf( temp,
                prev->tags[A] & FT_CURVE_TAG_ON ? 'P' : 'C',
                prev->tags[A] & FT_CURVE_TAG_TOUCH_X ? 'X' : ' ',
                prev->tags[A] & FT_CURVE_TAG_TOUCH_Y ? 'Y' : ' ' );

        if ( diff & 1 )
          temp = use_float ? "(%8.2f)" : "(%8ld)";
        else
          temp = use_float ? " %8.2f " : " %8ld ";
        if ( use_float )
          printf( temp, prev->org[A].x / 64.0 );
        else
          printf( temp, prev->org[A].x );

        if ( diff & 2 )
          temp = use_float ? "(%8.2f)" : "(%8ld)";
        else
          temp = use_float ? " %8.2f " : " %8ld ";
        if ( use_float )
          printf( temp, prev->org[A].y / 64.0 );
        else
          printf( temp, prev->org[A].y );

        if ( diff & 4 )
          temp = use_float ? "(%8.2f)" : "(%8ld)";
        else
          temp = use_float ? " %8.2f " : " %8ld ";
        if ( use_float )
          printf( temp, prev->cur[A].x / 64.0 );
        else
          printf( temp, prev->cur[A].x );

        if ( diff & 8 )
          temp = use_float ? "(%8.2f)" : "(%8ld)";
        else
          temp = use_float ? " %8.2f " : " %8ld ";
        if ( use_float )
          printf( temp, prev->cur[A].y / 64.0 );
        else
          printf( temp, prev->cur[A].y );

        printf( "\n" );

        printf( "                    " );

        if ( diff & 16 )
          temp = "(%c%c%c)";
        else
          temp = "     ";
        printf( temp,
                curr->tags[A] & FT_CURVE_TAG_ON ? 'P' : 'C',
                curr->tags[A] & FT_CURVE_TAG_TOUCH_X ? 'X' : ' ',
                curr->tags[A] & FT_CURVE_TAG_TOUCH_Y ? 'Y' : ' ' );

        if ( diff & 1 )
          temp = use_float ? "[%8.2f]" : "[%8ld]";
        else
          temp = "          ";
        if ( use_float )
          printf( temp, curr->org[A].x / 64.0 );
        else
          printf( temp, curr->org[A].x );

        if ( diff & 2 )
          temp = use_float ? "[%8.2f]" : "[%8ld]";
        else
          temp = "          ";
        if ( use_float )
          printf( temp, curr->org[A].y / 64.0 );
        else
          printf( temp, curr->org[A].y );

        if ( diff & 4 )
          temp = use_float ? "[%8.2f]" : "[%8ld]";
        else
          temp = "          ";
        if ( use_float )
          printf( temp, curr->cur[A].x / 64.0 );
        else
          printf( temp, curr->cur[A].x );

        if ( diff & 8 )
          temp = use_float ? "[%8.2f]" : "[%8ld]";
        else
          temp = "          ";
        if ( use_float )
          printf( temp, curr->cur[A].y / 64.0 );
        else
          printf( temp, curr->cur[A].y );

        printf( "\n" );
      }
    }
  }


  static void
  show_points_table( TT_GlyphZoneRec*  zone,
                     const FT_String*  code_range,
                     int               n_points,
                     FT_Bool           is_twilight )
  {
    int  A;


    if ( code_range[0] == 'g' )
    {
      printf( "%s points\n"
              "\n",
              is_twilight ? "twilight" : "glyph" );
      printf( " idx "
              "orig. unscaled  "
              "   orig. scaled     "
              " current scaled     "
              "tags\n" );
      printf( "-----"
              "----------------"
              "--------------------"
              "--------------------"
              "----\n" );
    }
    else
      printf( "Not yet in `glyf' program.\n" );

    for ( A = 0; A < n_points; A++ )
    {
      printf( "%3d%s ",
              A,
              is_twilight
                ? "T"
                : ( A >= n_points - 4 )
                    ? "F"
                    : " " );
      printf( "(%5ld,%5ld) - ",
              zone->orus[A].x, zone->orus[A].y );
      if ( use_float )
      {
        printf( "(%7.2f,%7.2f) - ",
                zone->org[A].x / 64.0, zone->org[A].y / 64.0 );
        printf( "(%7.2f,%7.2f) - ",
                zone->cur[A].x / 64.0, zone->cur[A].y / 64.0 );
      }
      else
      {
        printf( "(%7ld,%7ld) - ",
                zone->org[A].x, zone->org[A].y );
        printf( "(%7ld,%7ld) - ",
                zone->cur[A].x, zone->cur[A].y );
      }
      printf( "%c%c%c\n",
              zone->tags[A] & FT_CURVE_TAG_ON ? 'P' : 'C',
              zone->tags[A] & FT_CURVE_TAG_TOUCH_X ? 'X' : ' ',
              zone->tags[A] & FT_CURVE_TAG_TOUCH_Y ? 'Y' : ' ' );
    }
    printf( "\n" );
  }


  static FT_Error
  RunIns( TT_ExecContext  exc )
  {
    FT_Int  key;

    FT_Bool  really_leave;

    FT_String  ch, oldch = '\0';

    FT_Long  last_IP    = 0;
    FT_Int   last_range = 0;

    TT_GlyphZoneRec  pts;
    TT_GlyphZoneRec  twilight;
    TT_GlyphZoneRec  save_pts;
    TT_GlyphZoneRec  save_twilight;

    FT_Long*  save_cvt;

    Storage*  storage;
    Storage*  save_storage;

    const FT_String*  code_range;

    const FT_String*  round_str[8] =
    {
      "to half-grid",
      "to grid",
      "to double grid",
      "down to grid",
      "up to grid",
      "off",
      "super",
      "super 45"
    };


    error = FT_Err_Ok;

    pts      = CUR.pts;
    twilight = CUR.twilight;

    save_pts.n_points   = pts.n_points;
    save_pts.n_contours = pts.n_contours;

    save_pts.org  = (FT_Vector*)malloc( 2 * sizeof( FT_F26Dot6 ) *
                                        save_pts.n_points );
    save_pts.cur  = (FT_Vector*)malloc( 2 * sizeof( FT_F26Dot6 ) *
                                        save_pts.n_points );
    save_pts.tags = (FT_Byte*)malloc( save_pts.n_points );

    save_twilight.n_points   = twilight.n_points;
    save_twilight.n_contours = twilight.n_contours;

    save_twilight.org  = (FT_Vector*)malloc( 2 * sizeof( FT_F26Dot6 ) *
                                             save_twilight.n_points );
    save_twilight.cur  = (FT_Vector*)malloc( 2 * sizeof( FT_F26Dot6 ) *
                                             save_twilight.n_points );
    save_twilight.tags = (FT_Byte*)malloc( save_twilight.n_points );

    save_cvt = (FT_Long*)malloc( sizeof ( FT_Long ) * CUR.cvtSize );

    /* set everything to zero in Storage Area */
    storage      = (Storage*)calloc( CUR.storeSize, sizeof ( Storage ) );
    save_storage = (Storage*)calloc( CUR.storeSize, sizeof ( Storage ) );

    CUR.instruction_trap = 1;

    switch ( CUR.curRange )
    {
    case tt_coderange_glyph:
      code_range = "glyf";
      break;

    case tt_coderange_cvt:
      code_range = "prep";
      break;

    default:
      code_range = "fpgm";
    }

    printf( "Entering `%s' table.\n"
            "\n", code_range );

    really_leave = 0;

    do
    {
      if ( CUR.IP < CUR.codeSize )
      {
        Calc_Length( exc );

        CUR.args = CUR.top - ( Pop_Push_Count[CUR.opcode] >> 4 );

        /* `args' is the top of the stack once arguments have been popped. */
        /* One can also interpret it as the index of the last argument.    */

        /* Print the current line.  We use an 80-columns console with the  */
        /* following formatting:                                           */
        /*                                                                 */
        /* [loc]:[addr] [opcode]  [disassembly]         [a][b]|[c][d]      */

        {
          char  temp[90];
          int   n, col, pop;
          int   args;


          sprintf( temp, "%78c\n", ' ' );

          /* first letter of location */
          switch ( CUR.curRange )
          {
          case tt_coderange_glyph:
            temp[0] = 'g';
            break;

          case tt_coderange_cvt:
            temp[0] = 'c';
            break;

          default:
            temp[0] = 'f';
          }

          /* current IP */
          sprintf( temp + 1, "%04lx: %02x  %-36.36s",
                             CUR.IP,
                             CUR.opcode,
                             Cur_U_Line( &CUR ) );

          strncpy( temp + 46, " (", 2 );

          args = CUR.top - 1;
          pop  = Pop_Push_Count[CUR.opcode] >> 4;
          col  = 48;

          /* special case for IP */
          if ( CUR.opcode == 0x39 )
            pop = CUR.GS.loop;

          for ( n = 6; n > 0; n-- )
          {
            int  num_chars;


            if ( pop == 0 )
              temp[col - 1] = temp[col - 1] == '(' ? ' ' : ')';

            if ( args >= 0 )
            {
              long  val = (signed long)CUR.stack[args];


              if ( use_hex )
              {
                /* we display signed hexadecimal numbers, which */
                /* is easier to read and needs less space       */
                num_chars = sprintf( temp + col, "%s%04lx",
                                                 val < 0 ? "-" : "",
                                                 val < 0 ? -val : val );
              }
              else
                num_chars = sprintf( temp + col, "%ld",
                                                 val );

              if ( col + num_chars >= 78 )
                break;
            }
            else
              num_chars = 0;

            temp[col + num_chars] = ' ';
            col                  += num_chars + 1;

            pop--;
            args--;
          }

          for ( n = col; n < 78; n++ )
            temp[n] = ' ';

          temp[78] = '\n';
          temp[79] = '\0';
          printf( "%s", temp );
        }

        /* First, check for empty stack and overflow */
        if ( CUR.args < 0 )
        {
          printf( "ERROR: Too Few Arguments.\n" );
          error = TT_Err_Too_Few_Arguments;
          goto LErrorLabel_;
        }

        CUR.new_top = CUR.args + ( Pop_Push_Count[CUR.opcode] & 15 );

        /* `new_top' is the new top of the stack, after the instruction's */
        /* execution.  `top' will be set to `new_top' after the 'case'.   */

        if ( CUR.new_top > CUR.stackSize )
        {
          printf( "ERROR: Stack overflow.\n" );
          error = TT_Err_Stack_Overflow;
          goto LErrorLabel_;
        }
      }
      else
      {
        if ( CUR.curRange == tt_coderange_glyph )
        {
          if ( !really_leave )
          {
            printf( "End of `glyf' program reached.\n" );
            really_leave = 1;
          }
          else
          {
            really_leave = 0;
            goto LErrorLabel_;
          }
        }
        else
        {
          printf( "\n" );
          goto LErrorLabel_;
        }
      }

      if ( breakpoint.IP == CUR.IP          &&
           breakpoint.range == CUR.curRange )
        printf( "Hit breakpoint.\n" );

      key = 0;
      do
      {
        /* read keyboard */
        ch = (FT_String)getch();

        switch ( ch )
        {
        /* Help - show keybindings */
        case '?':
        case 'h':
          printf(
            "ttdebug Help\n"
            "\n"
            "Q   quit debugger                       V   show vector info\n"
            "R   restart debugger                    G   show graphics state\n"
            "c   continue to next code range         P   show points zone\n"
            "n   skip to next instruction            T   show twilight zone\n"
            "s   step into                           S   show storage area\n"
            "f   finish current function             C   show CVT data\n"
            "l   show last bytecode instruction      K   show full stack\n"
            "b   toggle breakpoint at curr. pos.     F   toggle floating/fixed\n"
            "p   toggle breakpoint at prev. pos.         point format\n"
            "                                        I   toggle hexadecimal/\n"
            "                                            decimal int. format\n"
            "                                        B   show backtrace\n"
            "\n"
            "\n"
            "  Format of point changes:\n"
            "\n"
            "    idx   orus.x  orus.y  tags  org.x  org.y  cur.x  cur.y\n"
            "\n"
            "  The first line gives the values before the instruction,\n"
            "  the second line the changes after the instruction,\n"
            "  indicated by parentheses and brackets for emphasis.\n"
            "\n"
            "  `T', `F', `S', `s', or `C' appended to the index indicates\n"
            "  a twilight point, a phantom point, a storage location,\n"
            "  a stack value, or data from the Control Value Table (CVT),\n"
            "  respectively.\n"
            "\n"
            "  Possible tag values are `P' (on curve), `C' (control point),\n"
            "  `X' (touched horizontally), and `Y' (touched vertically).\n"
            "\n" );
          break;

        /* Toggle between floating and fixed point format */
        case 'F':
          use_float = !use_float;
          printf( "Use %s point format for displaying non-integer values.\n",
                  use_float ? "floating" : "fixed" );
          printf( "\n" );
          break;

        /* Toggle between decimal and hexadimal integer format */
        case 'I':
          use_hex = !use_hex;
          printf( "Use %s format for displaying integers.\n",
                  use_hex ? "hexadecimal" : "decimal" );
          printf( "\n" );
          break;

        /* Show vectors */
        case 'V':
          if ( use_float )
          {
            /* 2.14 numbers */
            printf( "freedom    (%.5f, %.5f)\n",
                    CUR.GS.freeVector.x / 16384.0,
                    CUR.GS.freeVector.y / 16384.0 );
            printf( "projection (%.5f, %.5f)\n",
                    CUR.GS.projVector.x / 16384.0,
                    CUR.GS.projVector.y / 16384.0 );
            printf( "dual       (%.5f, %.5f)\n",
                    CUR.GS.dualVector.x / 16384.0,
                    CUR.GS.dualVector.y / 16384.0 );
            printf( "\n" );
          }
          else
          {
            printf( "freedom    ($%04hx, $%04hx)\n",
                    CUR.GS.freeVector.x,
                    CUR.GS.freeVector.y );
            printf( "projection ($%04hx, $%04hx)\n",
                    CUR.GS.projVector.x,
                    CUR.GS.projVector.y );
            printf( "dual       ($%04hx, $%04hx)\n",
                    CUR.GS.dualVector.x,
                    CUR.GS.dualVector.y );
            printf( "\n" );
          }
          break;

        /* Show graphics state */
        case 'G':
          {
            int  version;


            /* this doesn't really belong to the graphics state, */
            /* but I consider it a good place to show            */
            FT_Property_Get( library,
                             "truetype",
                             "interpreter-version", &version );
            printf( "hinting engine version: %d\n"
                    "\n",
                    version );
          }

          printf( "rounding state      %s\n",
                  round_str[CUR.GS.round_state] );
          if ( use_float )
          {
            /* 26.6 numbers */
            printf( "minimum distance    %.2f\n",
                    CUR.GS.minimum_distance / 64.0 );
            printf( "CVT cut-in          %.2f\n",
                    CUR.GS.control_value_cutin / 64.0 );
          }
          else
          {
            printf( "minimum distance    $%04lx\n",
                    CUR.GS.minimum_distance );
            printf( "CVT cut-in          $%04lx\n",
                    CUR.GS.control_value_cutin );
          }
          printf( "ref. points 0,1,2   %d, %d, %d\n",
                  CUR.GS.rp0, CUR.GS.rp1, CUR.GS.rp2 );
          printf( "\n" );
          break;

        /* Show CVT */
        case 'C':
          {
            if ( code_range[0] == 'f' )
              printf( "Not yet in `prep' or `glyf' program.\n" );
            else
            {
              FT_ULong  i;


              printf( "Control Value Table (CVT) data\n"
                      "\n" );
              printf( " idx         value       \n"
                      "-------------------------\n" );

              for ( i = 0; i < CUR.cvtSize; i++ )
                printf( "%3ldC  %8ld (%8.2f)\n",
                        i, CUR.cvt[i], CUR.cvt[i] / 64.0 );
              printf( "\n" );
            }
          }
          break;

        /* Show Storage Area */
        case 'S':
          {
            if ( code_range[0] == 'f' )
              printf( "Not yet in `prep' or `glyf' program.\n" );
            else
            {
              FT_ULong  i;


              printf( "Storage Area\n"
                      "\n" );
              printf( " idx         value       \n"
                      "-------------------------\n" );

              for ( i = 0; i < CUR.storeSize; i++ )
              {
                if ( storage[i].initialized )
                  printf( "%3ldS  %8ld (%8.2f)\n",
                          i,
                          storage[i].value,
                          storage[i].value / 64.0 );
                else
                  printf( "%3ldS  <uninitialized>\n",
                          i );
              }
              printf( "\n" );
            }
          }
          break;

        /* Show full stack */
        case 'K':
          {
            int  args = CUR.top - 1;


            if ( args >= 0 )
            {
              printf( "Stack\n"
                      "\n" );
              printf( " idx         value       \n"
                      "-------------------------\n" );

              for ( ; args >= 0; args-- )
              {
                long  val = (signed long)CUR.stack[args];


                printf( "%3lds  %8ld (%8.2f)\n",
                        CUR.top - args,
                        val,
                        val / 64.0 );
              }
              printf( "\n" );
            }
            else
              printf( "Stack empty\n" );
          }
          break;

        /* Show glyph points table */
        case 'P':
          show_points_table( &pts, code_range, pts.n_points, 0 );
          break;

        /* Show twilight points table */
        case 'T':
          show_points_table( &twilight, code_range, twilight.n_points, 1 );
          break;

        /* Show backtrace */
        case 'B':
          if ( CUR.callTop <= 0 )
            printf( "At top level.\n" );
          else
          {
            FT_Int  i;


            printf( "Function call backtrace\n"
                    "\n" );
            printf( " idx   loopcount   start    end   caller\n"
                    "----------------------------------------\n" );

            for ( i = CUR.callTop; i > 0; i-- )
            {
              TT_CallRec  *rec = &CUR.callStack[i - 1];


              printf( " %3d      %4ld     f%04lx   f%04lx   %c%04lx\n",
                      rec->Def->opc,
                      rec->Cur_Count,
                      rec->Def->start,
                      rec->Def->end,
                      rec->Caller_Range == tt_coderange_font
                        ? 'f'
                        : ( rec->Caller_Range == tt_coderange_cvt
                            ? 'c'
                            : 'g' ),
                      rec->Caller_IP - 1 );
            }
            printf( "\n" );
          }
          break;

        default:
          key = 1;
        }
      } while ( !key );

      if ( pts.n_points )
      {
        FT_MEM_COPY( save_pts.org,
                     pts.org,
                     pts.n_points * sizeof ( FT_Vector ) );
        FT_MEM_COPY( save_pts.cur,
                     pts.cur,
                     pts.n_points * sizeof ( FT_Vector ) );
        FT_MEM_COPY( save_pts.tags,
                     pts.tags,
                     pts.n_points );
      }

      if ( twilight.n_points )
      {
        FT_MEM_COPY( save_twilight.org,
                     twilight.org,
                     twilight.n_points * sizeof ( FT_Vector ) );
        FT_MEM_COPY( save_twilight.cur,
                     twilight.cur,
                     twilight.n_points * sizeof ( FT_Vector ) );
        FT_MEM_COPY( save_twilight.tags,
                     twilight.tags,
                     twilight.n_points );
      }

      if ( CUR.cvtSize )
        FT_MEM_COPY( save_cvt,
                     CUR.cvt,
                     CUR.cvtSize * sizeof ( FT_Long ) );

      if ( CUR.storeSize )
        FT_MEM_COPY( save_storage,
                     storage,
                     CUR.storeSize * sizeof ( Storage ) );

      /* a return indicates the last command */
      if ( ch == '\r' || ch == '\n' )
        ch = oldch;

      switch ( ch )
      {
      /* quit debugger */
      case 'Q':
        /* without the pedantic hinting flag,                   */
        /* FreeType ignores bytecode errors in `glyf' programs  */
        CUR.pedantic_hinting = 1;
        error = Quit;
        goto LErrorLabel_;

      /* restart debugger */
      case 'R':
        /* without the pedantic hinting flag,                   */
        /* FreeType ignores bytecode errors in `glyf' programs  */
        CUR.pedantic_hinting = 1;
        error = Restart;
        goto LErrorLabel_;

      /* continue */
      case 'c':
        if ( CUR.IP < CUR.codeSize )
        {
          last_IP    = CUR.IP;
          last_range = CUR.curRange;

          /* loop execution until we reach end of current code range */
          /* or hit the breakpoint's position                        */
          while ( CUR.IP < CUR.codeSize )
          {
            handle_WS( exc, storage );
            if ( ( error = TT_RunIns( exc ) ) != 0 )
              goto LErrorLabel_;

            if ( CUR.IP == breakpoint.IP          &&
                 CUR.curRange == breakpoint.range )
              break;
          }
        }
        break;

      /* finish current function or hit breakpoint */
      case 'f':
        oldch = ch;

        if ( CUR.IP < CUR.codeSize )
        {
          if ( code_range[0] == 'f' )
          {
            printf( "Not yet in `prep' or `glyf' program.\n" );
            break;
          }

          if ( CUR.curRange != tt_coderange_font )
          {
            printf( "Not in a function.\n" );
            break;
          }

          last_IP    = CUR.IP;
          last_range = CUR.curRange;

          while ( 1 )
          {
            Calc_Length( exc ); /* this updates CUR.opcode also */

            /* we are done if we see the current function's ENDF opcode */
            if ( CUR.opcode == 0x2d )
              goto Step_into;

            if ( CUR.opcode == 0x2a || CUR.opcode == 0x2b )
            {
              FT_Long  next_IP;


              /* loop execution until we reach the next opcode */
              next_IP = CUR.IP + CUR.length;
              while ( CUR.IP != next_IP )
              {
                handle_WS( exc, storage );
                if ( ( error = TT_RunIns( exc ) ) != 0 )
                  goto LErrorLabel_;

                if ( CUR.IP == breakpoint.IP          &&
                     CUR.curRange == breakpoint.range )
                  break;
              }

              printf( "\n" );
            }
            else
            {
              handle_WS( exc, storage );
              if ( ( error = TT_RunIns( exc ) ) != 0 )
                goto LErrorLabel_;
            }

            if ( CUR.IP == breakpoint.IP          &&
                 CUR.curRange == breakpoint.range )
              break;
          }
        }
        break;

      /* step over or hit breakpoint */
      case 'n':
        if ( CUR.IP < CUR.codeSize )
        {
          FT_Long  next_IP;
          FT_Int   saved_range;


          /* `step over' is equivalent to `step into' except if */
          /* the current opcode is a CALL or LOOPCALL           */
          if ( CUR.opcode != 0x2a && CUR.opcode != 0x2b )
            goto Step_into;

          last_IP    = CUR.IP;
          last_range = CUR.curRange;

          /* otherwise, loop execution until we reach the next opcode */
          saved_range = CUR.curRange;
          next_IP     = CUR.IP + CUR.length;
          while ( !( CUR.IP == next_IP && CUR.curRange == saved_range ) )
          {
            handle_WS( exc, storage );
            if ( ( error = TT_RunIns( exc ) ) != 0 )
              goto LErrorLabel_;

            if ( CUR.IP == breakpoint.IP          &&
                 CUR.curRange == breakpoint.range )
              break;
          }
        }

        oldch = ch;
        break;

      /* step into */
      case 's':
        if ( CUR.IP < CUR.codeSize )
        {
        Step_into:
          last_IP    = CUR.IP;
          last_range = CUR.curRange;

          handle_WS( exc, storage );
          if ( ( error = TT_RunIns( exc ) ) != 0 )
            goto LErrorLabel_;
        }

        oldch = ch;
        break;

      /* toggle breakpoint at current position */
      case 'b':
        if ( breakpoint.IP == CUR.IP          &&
             breakpoint.range == CUR.curRange )
        {
          breakpoint.IP    = 0;
          breakpoint.range = 0;

          printf( "Breakpoint removed.\n" );
        }
        else
        {
          breakpoint.IP    = CUR.IP;
          breakpoint.range = CUR.curRange;

          printf( "Breakpoint set.\n" );
        }

        oldch = ch;
        break;

      /* toggle breakpoint at previous position */
      case 'p':
        if ( last_IP == 0 && last_range == 0 )
          printf( "No previous position yet to set breakpoint.\n" );
        else
        {
          if ( breakpoint.IP == last_IP       &&
               breakpoint.range == last_range )
          {
            breakpoint.IP    = 0;
            breakpoint.range = 0;

            printf( "Breakpoint removed from previous position.\n" );
          }
          else
          {
            breakpoint.IP    = last_IP;
            breakpoint.range = last_range;

            printf( "Breakpoint set to previous position (%c%04lx).\n",
                    last_range == tt_coderange_font
                      ? 'f'
                      : ( last_range == tt_coderange_cvt
                            ? 'c'
                            : 'g' ),
                    last_IP );
          }
        }

        oldch = ch;
        break;

      /* show last bytecode instruction */
      case 'l':
        oldch = ch;
        break;

      default:
        printf( "Unknown command.  Press ? or h for help.\n" );
        oldch = '\0';
        break;
      }

      display_changed_points(&save_pts, &pts, 0);
      display_changed_points(&save_twilight, &twilight, 1);

      {
        FT_ULong  i;


        for ( i = 0; i < CUR.cvtSize; i++ )
          if ( save_cvt[i] != CUR.cvt[i] )
          {
            printf( "%3ldC %8ld (%8.2f)\n",
                    i, save_cvt[i], save_cvt[i] / 64.0 );
            printf( "     %8ld (%8.2f)\n",
                    CUR.cvt[i], CUR.cvt[i] / 64.0 );
          }

        for ( i = 0; i < CUR.storeSize; i++ )
          if ( save_storage[i].initialized != storage[i].initialized ||
               save_storage[i].value != storage[i].value             )
          {
            printf( "%3ldS %8ld (%8.2f)\n",
                    i, save_storage[i].value, save_storage[i].value / 64.0 );
            printf( "     %8ld (%8.2f)\n",
                    storage[i].value, storage[i].value / 64.0 );
          }
      }

    } while ( 1 );

  LErrorLabel_:
    free( save_pts.org );
    free( save_pts.cur );
    free( save_pts.tags );

    free( save_twilight.org );
    free( save_twilight.cur );
    free( save_twilight.tags );

    free( save_cvt );

    free( storage );
    free( save_storage );

    if ( error && error != Quit && error != Restart )
      Abort( "error during execution" );
    return error;
  }


  static void
  Usage( char*  execname )
  {
    char  versions[32];


    /* we expect that at least one interpreter version is available */
    if ( num_tt_interpreter_versions == 2 )
      sprintf(versions, "%d and %d",
                        tt_interpreter_versions[0],
                        tt_interpreter_versions[1] );
    else
      sprintf(versions, "%d, %d, and %d",
                        tt_interpreter_versions[0],
                        tt_interpreter_versions[1],
                        tt_interpreter_versions[2] );

    fprintf( stderr,
      "\n"
      "ttdebug: simple TTF debugger -- part of the FreeType project\n"
      "------------------------------------------------------------\n"
      "\n" );
    fprintf( stderr,
      "Usage: %s [options] idx size font\n"
      "\n", execname );
    fprintf( stderr,
      "  idx       The index of the glyph to debug.\n"
      "  size      The size of the glyph in pixels (ppem).\n"
      "  font      The TrueType font file to debug.\n"
      "\n"
      "  -I ver    Use TT interpreter version VER.\n"
      "            Available versions are %s; default is version %d.\n"
      "  -v        Show version.\n"
      "\n"
      "While running, press the `?' key for help.\n"
      "\n",
      versions,
      dflt_tt_interpreter_version );

    exit( 1 );
  }


  static char*         file_name;
  static unsigned int  glyph_index;
  static int           glyph_size;


  int
  main( int     argc,
        char**  argv )
  {
    char*  execname;
    int    option;
    char   version_string[64];

    int           i;
    unsigned int  versions[3] = { TT_INTERPRETER_VERSION_35,
                                  TT_INTERPRETER_VERSION_38,
                                  TT_INTERPRETER_VERSION_40 };
    int           version;

    int  tmp;


    /* init library, read face object, get driver, create size */
    error = FT_Init_FreeType( &library );
    if ( error )
      Abort( "could not initialize FreeType library" );

    memory = library->memory;
    driver = (FT_Driver)FT_Get_Module( library, "truetype" );
    if ( !driver )
      Abort( "could not find the TrueType driver in FreeType 2\n" );

    {
      FT_Int  major, minor, patch;
      int     offset;


      FT_Library_Version( library, &major, &minor, &patch );

      offset = snprintf( version_string, 64,
                         "ttdebug (FreeType) %d.%d",
                         major, minor );
      if ( patch )
        offset = snprintf( version_string + offset, (size_t)( 64 - offset ),
                           ".%d",
                           patch );
    }

    /* collect all available versions, then set again the default */
    FT_Property_Get( library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );
    for ( i = 0; i < 3; i++ )
    {
      error = FT_Property_Set( library,
                               "truetype",
                               "interpreter-version", &versions[i] );
      if ( !error )
        tt_interpreter_versions[num_tt_interpreter_versions++] = versions[i];
    }
    FT_Property_Set( library,
                     "truetype",
                     "interpreter-version", &dflt_tt_interpreter_version );

    execname = ft_basename( argv[0] );

    while ( 1 )
    {
      option = getopt( argc, argv, "I:v" );

      if ( option == -1 )
        break;

      switch ( option )
      {
      case 'I':
        version = atoi( optarg );

        if ( version < 0 )
        {
          printf( "invalid TrueType interpreter version = %d\n", version );
          Usage( execname );
        }

        for ( i = 0; i < num_tt_interpreter_versions; i++ )
        {
          if ( (unsigned int)version == tt_interpreter_versions[i] )
          {
            FT_Property_Set( library,
                             "truetype",
                             "interpreter-version", &version );
            break;
          }
        }

        if ( i == num_tt_interpreter_versions )
        {
          printf( "invalid TrueType interpreter version = %d\n", version );
          Usage( execname );
        }
        break;

      case 'v':
        printf( "%s\n", version_string );
        exit( 0 );
        /* break; */

      default:
        Usage( execname );
        break;
      }
    }

    argc -= optind;
    argv += optind;

    if ( argc < 3 )
      Usage( execname );

    /* get glyph index */
    if ( sscanf( argv[0], "%d", &tmp ) != 1 || tmp < 0 )
    {
      printf( "invalid glyph index = %s\n", argv[1] );
      Usage( execname );
    }
    glyph_index = (unsigned int)tmp;

    /* get glyph size */
    if ( sscanf( argv[1], "%d", &glyph_size ) != 1 || glyph_size < 0 )
    {
      printf( "invalid glyph size = %s\n", argv[1] );
      Usage( execname );
    }

    /* get file name */
    file_name = argv[2];

    Init_Keyboard();

    FT_Set_Debug_Hook( library,
                       FT_DEBUG_HOOK_TRUETYPE,
                       (FT_DebugHook_Func)RunIns );

    printf( "%s\n"
            "press key `h' or `?' for help\n"
            "\n", version_string );

    while ( !error )
    {
      error = FT_New_Face( library, file_name, 0, (FT_Face*)&face );
      if ( error )
        Abort( "could not open input font file" );

      /* find driver and check format */
      if ( face->root.driver != driver )
      {
        error = FT_Err_Invalid_File_Format;
        Abort( "this is not a TrueType font" );
      }

      size = (TT_Size)face->root.size;

      error = FT_Set_Char_Size( (FT_Face)face,
                                glyph_size << 6,
                                glyph_size << 6,
                                72,
                                72 );
      if ( error )
        Abort( "could not set character size" );

      glyph = (TT_GlyphSlot)face->root.glyph;

      /* now load glyph */
      error = FT_Load_Glyph( (FT_Face)face,
                             (FT_UInt)glyph_index,
                             FT_LOAD_NO_BITMAP );
      if ( error && error != Quit && error != Restart )
        Abort( "could not load glyph" );
      if ( error == Restart )
        error = FT_Err_Ok;

      FT_Done_Face( (FT_Face)face );
    }

    Reset_Keyboard();

    FT_Done_FreeType( library );

    return 0;
  }


/* End */
