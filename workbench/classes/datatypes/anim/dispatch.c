
/*
**
** $Id$
**  anim.datatype 1.12
**
**  Dispatch routine for a DataTypes class
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

struct ClassBase;
struct AnimInstData;
struct FrameNode;

/* main includes */
#include "classbase.h"
#include "classdata.h"
#if defined(DOASYNCIO)
#include "asyncio.h"
#endif
#include <stdio.h>

/*****************************************************************************/

/* IFF errors to DOS errors */
const
LONG ifferr2doserr[] =
{
  0L,                         /* End of file (not an error).                  */
  0L,                         /* End of context (not an error).               */
  DTERROR_INVALID_DATA,       /* No lexical scope.                            */
  ERROR_NO_FREE_STORE,        /* Insufficient memory.                         */
  ERROR_SEEK_ERROR,           /* Stream read error.                           */
  ERROR_SEEK_ERROR,           /* Stream write error.                          */
  ERROR_SEEK_ERROR,           /* Stream seek error.                           */
  DTERROR_INVALID_DATA,       /* File is corrupt.                             */
  DTERROR_INVALID_DATA,       /* IFF syntax error.                            */
  ERROR_OBJECT_WRONG_TYPE,    /* Not an IFF file.                             */
  ERROR_REQUIRED_ARG_MISSING, /* Required call-back hook missing.             */
  (LONG)(0xDEADDEADUL)        /* Return to client.  You should never see this */
};

/*****************************************************************************/

/* local prototypes */
static                 STRPTR               GetPrefsVar( struct ClassBase *, STRPTR );
#if !defined(__AROS__)
static                 void                 YouShouldRegister( struct ClassBase *, struct AnimInstData * );
#endif
static                 BOOL                 matchstr( struct ClassBase *, STRPTR, STRPTR );
static                 struct FrameNode    *AllocFrameNode( struct ClassBase *, APTR );
static                 void                 XCopyMem( struct ClassBase *, APTR, APTR, ULONG );
static                 void                 DumpAnimHeader( struct ClassBase *, struct AnimInstData *, ULONG, struct AnimHeader * );
static                 struct FrameNode    *GetPrevFrameNode( struct FrameNode *, ULONG );
static                 void                 AttachSample( struct ClassBase *, struct AnimInstData * );

static                 struct IFFHandle    *CreateDOSIFFHandle( struct ClassBase *, BPTR );
static                 LONG                 StartIFFAnim3( struct ClassBase *, struct AnimInstData *, struct IFFHandle *iff, struct AnimContext *, struct BitMapHeader *, ULONG, ULONG *, ULONG, ULONG, ULONG, struct BitMap * );
static                 void                 EndIFFAnim3( struct ClassBase *, struct AnimInstData *, struct IFFHandle * );
static                 LONG                 WriteIFFAnim3( struct ClassBase *, struct IFFHandle *, struct AnimContext *, ULONG, ULONG, struct BitMapHeader *, ULONG *, ULONG, struct BitMap * );
static                 LONG                 PutAnim3Delta( struct ClassBase *, struct IFFHandle *, struct AnimContext *, struct BitMap *, struct BitMap * );
static                 LONG                 PutILBMCMAP( struct ClassBase *, struct IFFHandle *, ULONG *, ULONG );
static                 LONG                 PutILBMBody( struct ClassBase *, struct IFFHandle *, struct BitMap *, struct BitMapHeader * );
static                 struct AnimContext  *CreateAnimContext( struct ClassBase *, ULONG, ULONG, ULONG );
#if 0
static                 struct BitMap       *PrevFrame( struct ClassBase *, struct AnimContext * );
#endif
static                 void                 SwapFrames( struct ClassBase *, struct AnimContext * );
static                 struct BitMap       *CurrFrame( struct ClassBase *, struct AnimContext * );
static                 void                 DeleteAnimContext( struct ClassBase *, struct AnimContext * );


/*****************************************************************************/

#if !defined (__AROS__)
/* Create "anim.datatype" BOOPSI class */
struct IClass *initClass( struct ClassBase *cb )
{
    struct IClass *cl;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Create our class... */
    if( cl = MakeClass( ANIMDTCLASS, ANIMATIONDTCLASS, NULL, (ULONG)sizeof( struct AnimInstData ), 0UL ) )
    {
#define DTSTACKSIZE (16384UL)
      cl -> cl_Dispatcher . h_Entry    = (HOOKFUNC)StackSwapDispatch; /* see stackswap.c */
      cl -> cl_Dispatcher . h_SubEntry = (HOOKFUNC)Dispatch;          /* see stackswap.c */
      cl -> cl_Dispatcher . h_Data     = (APTR)DTSTACKSIZE;           /* see stackswap.c */
      cl -> cl_UserData                = (IPTR)cb;                   /* class library base as expected by datatypes.library */

      AddClass( cl );
    }

    return( cl );
}

#include "methods.h"

/*****************************************************************************/

/* class dispatcher */
DISPATCHERFLAGS
IPTR Dispatch( REGA0 struct IClass *cl, REGA2 Object *o, REGA1 Msg msg )
{
    struct ClassBase     *cb = (struct ClassBase *)(cl -> cl_UserData);
    struct AnimInstData  *aid;
    IPTR                 retval = 0UL;

    D(bug("[anim.datatype] %s()\n", __func__));

    switch( msg -> MethodID )
    {

      case OM_NEW:
          retval = DT_NewMethod( cl, o, msg );
          break;

      case OM_DISPOSE:
          retval = DT_DisposeMethod( cl, o, msg );
          break;

      case OM_UPDATE:
      case OM_SET:
          retval = DT_SetMethod( cl, o, msg );
          break;

      case DTM_WRITE:
          retval = DT_WriteMethod( cl, o, msg );
          break;

      case ADTM_START:
          retval = DT_Start( cl, o, msg );
          break;

      case ADTM_PAUSE:
          retval = DT_Pause( cl, o, msg );
          break;

      case ADTM_STOP:
          retval = DT_Stop( cl, o, msg );
          break;

      case ADTM_LOADFRAME:
          retval = DT_LoadFrameMethod( cl, o, msg );
          break;

      case ADTM_UNLOADFRAME:
          retval = DT_UnLoadFrameMethod( cl, o, msg );
          break;

      default:
          retval = DoSuperMethodA( cl, o, msg );
          break;
    }

    return( retval );
}
#endif

BOOL FreeAbleFrame( struct AnimInstData *aid, struct FrameNode *fn )
{
    struct FrameNode *currfn = aid -> aid_CurrFN;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Don't free the current nor the previous nor the next bitmap (to avoid problems with delta frames) */
    if( (fn == currfn) ||
        (fn == (struct FrameNode *)(currfn -> fn_Node . mln_Succ)) ||
        (fn == (struct FrameNode *)(currfn -> fn_Node . mln_Pred)) )
    {
      return( FALSE );
    }

    if( ABS( ((LONG)(fn -> fn_TimeStamp)) - ((LONG)(currfn -> fn_TimeStamp)) ) < 5UL )
    {
      return( FALSE );
    }

    return( TRUE );
}


/****** anim.datatype/preferences ********************************************
*
*   NAME
*       preferences
*
*   DESCRIPTION
*       The "ENV:Classes/DataTypes/anim.prefs" file contains global
*       settings for the datatype.
*       The preferences file is an ASCII file containing one line where the
*       preferences can be set.
*       It can be superset by a local variable with the same name.
*
*       Each line can contain settings, special settings for some projects
*       can be set using the MATCHPROJECT option.
*       Lines beginning with a '#' or ';' chars are treated as comments.
*       Lines are limitted to 256 chars.
*
*   TEMPLATE
*       MATCHPROJECT/K,VERBOSE/S,MODEID/K/N,CMAPS/S,NOCMAPS/S,
*       DPAINTBRUSHPATCH/S,NODPAINTBRUSHPATCH/S,FPS/K/N,DYNAMICTIMING/S,
*       NODYNAMICTIMING/S,SAMPLE/K,SAMPLESPERFRAME=SPF/K/N,VOLUME/K/N,
*       LOADALL/S,NOLOADALL/S
*
*       MATCHPROJECT -- The settings in this line belongs only to this
*           project(s), e.g. if the case-insensitive pattern does not match,
*           this line is ignored.
*           The maximum length of the pattern is 128 chars.
*           Defaults to #?, which matches any project.
*
*       VERBOSE -- Print information about the animation. Currently
*          the frame numbers and the used compression are printed, after all
*          number of scanned/loaded frames, set FPS rate, dimensions (width/
*          height/depth), sample information etc.
*
*       MODEID -- Select screen mode id of datatype (will be stored in
*           ADTA_ModeID). Note that the DOS ReadArgs function used for parsing
*           fetches a SIGNED long. The bit 31 will be represented by minus
*           '-'. (example: "MODEID=266240" sets the mode to the A2024 screen
*           mode id)
*           Defaults to 0, which means: Use the screen mode from CAMG chunk,
*           if missing use the best screenmode available for the given width,
*           height and depth.
*
*       CMAPS -- Use colormaps per frame. This switch is set per default,
*           and can be turned off by the NOCMAPS option, later it can be
*           turned on again by this option.
*
*       NOCMAPS -- Don't load/use colormaps per frame. Only the initial
*           colormap will be used.
*           The current version of animation.datatype (V40.7 (28.09.93)) does
*           not implement per frame colormaps, it's output may look trashed.
*           Custom players like "DBufDTAnim" does support
*           "per frame colormaps",
*           animation.datatype V41 will implement "per frame colormaps".
*
*       DPAINTBRUSHPATCH -- If frames of ANIM-5 with an interleave of 1
*           occurs, the XOR mode is forced, even if the XOR bit is not set.
*           This fixes problems with some DPaint brush animations.
*           This option is ON per default and can be turned off by the 
*           NODPAINTBRUSHPATCH option.
*
*       NODPAINTBRUSHPATCH -- Turns off the DPaint brush patch. See 
*           DPAINTBRUSHPATCH option for details.
*
*       FPS -- Frames Per Second
*           Defaults to FPS set by DPAN chunk. If the DPAN chunk is missing
*           a fixed 5 fps rate is used.
*           A value of 0 here means: Use default FPS.
*
*       DYNAMICTIMING -- Turns dynamic timing on. Default if superclass
*           is animation.datatype V41, otherwise this option must
*           be explicitly set.
*
*       NODYNAMICTIMING -- Turn dynamic timing off. Default if superclass
*           is animation.datatype < V41 (e.g. V40.6).
*
*       SAMPLE -- Attach the given sample to the animation. The sample will
*           be loaded using datatypes (GID_SOUND).
*           Only one sample can be attached to one animationstream, any
*           following attempt to attach a sample will be ignored.
*
*       SAMPLESPERFRAME -- Set samples per frame rate for sound. This
*           overrides the own internal calculations to get rid of rounding
*           errors.
*
*       VOLUME -- Volume of the sound when playing.
*           Defaults to 64, which is the maximum. A value greater than 64 will
*           be set to 64.
*
*       LOADALL -- Load all frames into memory. If the source input is a
*           clipboard, this option is always set.
*
*       NOLOADALL -- Turns off the LOADALL flag, which may be set in a prefs-
*           line before. This switch is set per default, and can be turned off
*           by the LOADALL option, later it can be turned on again by this
*           option.
*
*       REGISTERED -- Turns off the shareware notice requester.
*
*   NOTE
*       An invalid prefs file line will be ignored and forces the VERBOSE
*       output.
*
*   BUGS
*       - Low memory may cause that the prefs file won't be parsed.
*
*       - Lines are limitted to 256 chars
*
*       - An invalid prefs file line will be ignored.
*
*       - The sample path length is limitted to 200 chars. A larger
*         value may crash the machine if an error occurs.
*
******************************************************************************
*
*/


static
STRPTR GetPrefsVar( struct ClassBase *cb, STRPTR name )
{
          STRPTR buff;
    const ULONG  buffsize = 16UL;

    if ((buff = (STRPTR)AllocVec( (buffsize + 2UL), (MEMF_PUBLIC | MEMF_CLEAR) ) ) != NULL)
    {
      if( GetVar( name, buff, buffsize, GVF_BINARY_VAR ) != (-1L) )
      {
        ULONG varsize = IoErr();

        varsize += 2UL;

        if( varsize > buffsize )
        {
          FreeVec( buff );

          if ((buff = (STRPTR)AllocVec( (varsize + 2UL), (MEMF_PUBLIC | MEMF_CLEAR) ) ) != NULL)
          {
            if( GetVar( name, buff, varsize, GVF_BINARY_VAR ) != (-1L) )
            {
              return( buff );
            }
          }
        }
        else
        {
          return( buff );
        }
      }

      FreeVec( buff );
    }

    return( NULL );
}


static
BOOL matchstr( struct ClassBase *cb, STRPTR pat, STRPTR s )
{
    TEXT buff[ 512 ];

    if( pat && s )
    {
      if( ParsePatternNoCase( pat, buff, (sizeof( buff ) - 1) ) != (-1L) )
      {
        if( MatchPatternNoCase( buff, s ) )
        {
          return( TRUE );
        }
      }
    }

    return( FALSE );
}


void ReadENVPrefs( struct ClassBase *cb, struct AnimInstData *aid )
{
    struct RDArgs envvarrda =
    {
        {NULL, 256L, 0L},
        0L,
        NULL,
        0L,
        NULL,
        RDAF_NOPROMPT
    };

    struct
    {
      STRPTR  matchproject;
      IPTR  verbose;
      IPTR  *modeid;
      IPTR  cmaps;
      IPTR  nocmaps;
      IPTR  dpaintbrushpatch;
      IPTR  nodpaintbrushpatch;
      IPTR  *fps;
      IPTR  dynamictiming;
      IPTR  nodynamictiming;
      STRPTR  sample;
      IPTR  *samplesperframe;
      IPTR  *volume;
      IPTR  loadall;
      IPTR  noloadall;
      IPTR  registered;
    } animargs;

    TEXT   varbuff[ 258 ];
    STRPTR var;

    D(bug("[anim.datatype] %s()\n", __func__));

    if ((var = GetPrefsVar( cb, "Classes/DataTypes/anim.prefs" ) ) != NULL)
    {
      STRPTR prefsline      = var,
             nextprefsline;
      ULONG  linecount      = 1UL;

      /* Be sure that "var" contains at least one break-char */
      strcat( var, "\n" );

      while ((nextprefsline = strpbrk( prefsline, "\n" ) ) != NULL)
      {
        stccpy( varbuff, prefsline, (int)MIN( (sizeof( varbuff ) - 2UL), (((ULONG)(nextprefsline - prefsline)) + 1UL) ) );

        /* be sure that this line isn't a comment line or an empty line */
        if( (varbuff[ 0 ] != '#') && (varbuff[ 0 ] != ';') && (varbuff[ 0 ] != '\n') && (strlen( varbuff ) > 2UL) )
        {
          /* Prepare ReadArgs processing */
          strcat( varbuff, "\n" );                                       /* Add NEWLINE-char            */
          envvarrda . RDA_Source . CS_Buffer = varbuff;                  /* Buffer                      */
          envvarrda . RDA_Source . CS_Length = strlen( varbuff ) + 1UL;  /* Set up input buffer length  */
          envvarrda . RDA_Source . CS_CurChr = 0L;
          envvarrda . RDA_Buffer = NULL;
          envvarrda . RDA_BufSiz = 0L;
          memset( (void *)(&animargs), 0, sizeof( animargs ) );          /* Clear result array          */

          if( ReadArgs( "MATCHPROJECT/K,"
                        "VERBOSE/S,"
                        "MODEID/K/N,"
                        "CMAPS/S,"
                        "NOCMAPS/S,"
                        "DPAINTBRUSHPATCH/S,"
                        "NODPAINTBRUSHPATCH/S,"
                        "FPS/K/N,"
                        "DYNAMICTIMING/S,"
                        "NODYNAMICTIMING/S,"
                        "SAMPLE/K,"
                        "SAMPLESPERFRAME=SPF/K/N,"
                        "VOLUME/K/N,"
                        "LOADALL/S,"
                        "NOLOADALL/S,"
                        "REGISTERED/S", (IPTR *)(&animargs), (&envvarrda) ) )
          {
            BOOL noignore = TRUE;

            if( (animargs . matchproject) && (aid -> aid_ProjectName) )
            {
              noignore = matchstr( cb, (animargs . matchproject), (aid -> aid_ProjectName) );
            }

            if( noignore )
            {
              if( animargs . verbose )
              {
                OpenLogfile( cb, aid );
              }

              if( animargs . modeid )
              {
                aid -> aid_ModeID = *(animargs . modeid);
              }

              if( animargs . cmaps )
              {
                aid -> aid_NoCMAPs = FALSE;
              }

              if( animargs . nocmaps )
              {
                aid -> aid_NoCMAPs = TRUE;
              }

              if( animargs . dpaintbrushpatch )
              {
                aid -> aid_NoDPaintBrushPatch = FALSE;
              }

              if( animargs . nodpaintbrushpatch )
              {
                aid -> aid_NoDPaintBrushPatch = TRUE;
              }

              if( animargs . fps )
              {
                aid -> aid_FPS = *(animargs . fps);
              }

              if( animargs . dynamictiming )
              {
                aid -> aid_NoDynamicTiming = FALSE;
              }

              if( animargs . nodynamictiming )
              {
                aid -> aid_NoDynamicTiming = TRUE;
              }

              if( (animargs . sample) && ((aid -> aid_Sample) == NULL) )
              {
                Object *so;
                LONG    ioerr = 0L;

                verbose_printf( cb, aid, "loading sample \"%s\"...\n", (animargs . sample) );

                if( (so = NewDTObject( (animargs . sample), DTA_GroupID, GID_SOUND, TAG_DONE ) ) )
                {
                  BYTE  *sample = NULL;
                  IPTR  length = 0;
                  IPTR  period = 0;

                  /* Get sample data from object */
                  if( GetDTAttrs( so, SDTA_Sample,       (&sample),
                                      SDTA_SampleLength, (&length),
                                      SDTA_Period,       (&period),
                                      TAG_DONE ) == 3UL )
                  {
                    if ((aid -> aid_Sample = (STRPTR)AllocPooled( (aid -> aid_Pool), (length + 1UL) ) ) != NULL)
                    {
                      /* Copy sample and context */
                      XCopyMem( cb, (APTR)sample, (APTR)(aid -> aid_Sample), length );
                      aid -> aid_SampleLength = length;
                      aid -> aid_Period       = period;
                    }
                    else
                    {
                      /* Can't alloc sample */
                      ioerr = ERROR_NO_FREE_STORE;
                    }
                  }
                  else
                  {
                    /* Object does not support the requested attributes */
                    ioerr = ERROR_OBJECT_WRONG_TYPE;
                  }

                  DisposeDTObject( so );
                }
                else
                {
                  /* NewDTObjectA failed, cannot load sample... */
                  ioerr = IoErr();
                }

                if( (aid -> aid_Sample) == NULL )
                {
                  TEXT errbuff[ 256 ];

                  if( ioerr >= DTERROR_UNKNOWN_DATATYPE )
                  {
                    mysprintf( cb, errbuff, (STRPTR)GetDTString( ioerr ), (animargs . sample) );
                  }
                  else
                  {
                    Fault( ioerr, (animargs . sample), errbuff, sizeof( errbuff ) );
                  }

                  error_printf( cb, aid, "can't load sample: \"%s\" line %lu\n", errbuff, linecount );
                }
              }

              if( animargs . samplesperframe )
              {
                aid -> aid_SamplesPerFrame = (ULONG)(*(animargs . samplesperframe));
              }

              if( animargs . volume )
              {
                aid -> aid_Volume = *(animargs . volume);

                if( (aid -> aid_Volume) > 64UL )
                {
                  aid -> aid_Volume = 64UL;
                }
              }

              if( animargs . loadall )
              {
                aid -> aid_LoadAll = TRUE;
              }

              if( animargs . noloadall )
              {
                aid -> aid_LoadAll = FALSE;
              }

              if( animargs . registered )
              {
                aid -> aid_Registered = TRUE;
              }
            }
            else
            {
              verbose_printf( cb, aid, "prefs line %lu ignored\n", linecount );
            }

            FreeArgs( (&envvarrda) );
          }
          else
          {
            LONG ioerr = IoErr();
            TEXT errbuff[ 256 ];

            Fault( ioerr, "Classes/DataTypes/anim.prefs", errbuff, (LONG)sizeof( errbuff ) );

            error_printf( cb, aid, "preferences \"%s\" line %lu\n", errbuff, linecount );
          }
        }

        prefsline = ++nextprefsline;
        linecount++;
      }

      FreeVec( var );
    }

#if !defined(__AROS__)
    /* Notify the user that she/he is using shareware... */
    if( !(aid -> aid_Registered) )
    {
      YouShouldRegister( cb, aid );
    }
#endif
}

#if !defined(__AROS__)
/* The shareware notify requester */
static
void YouShouldRegister( struct ClassBase *cb, struct AnimInstData *aid )
{
    struct EasyStruct SharewareES;
    ULONG             xa,
                      xb,
                      xc,
                      xd;
    ULONG             result;
    LONG              reqresult;

    TEXT              buffer[ 256 ],
                      cbuffer[ 256 ];
#define NUMCHOICES (6)
    ULONG             choices[ NUMCHOICES ];

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Create random values */
    CurrentTime( (&xb), (&xa) );
    xc = FastRand( xa );
    xd = FastRand( xb );

    /* Strip down to human-friendly values */
    xa %=  5;
    xb %=  7;
    xc %= 11;
    xd %= 13;

    /* calc */
    result = xa + xb * xc + xd;

    /* Build term */
    mysprintf( cb, buffer, "%lu + %lu * %lu + %lu = ???", xa, xb, xc, xd );

    /* Our choices */
    choices[ 0 ] = result;
    choices[ 1 ] = result + xb;
    choices[ 2 ] = result + xb + 1;
    choices[ 3 ] = result + xa / 2;
    choices[ 4 ] = result + 2 + xc;
    choices[ 5 ] = xa + xb + result / 2;

    mysprintf( cb, cbuffer, "%lu|%lu|%lu|%lu|%lu|%lu",
               choices[ (0 + xc) % NUMCHOICES ],
               choices[ (1 + xc) % NUMCHOICES ],
               choices[ (2 + xc) % NUMCHOICES ],
               choices[ (3 + xc) % NUMCHOICES ],
               choices[ (4 + xc) % NUMCHOICES ],
               choices[ (5 + xc) % NUMCHOICES ] );

    /* Prepare requester */
    SharewareES . es_StructSize   = sizeof( struct EasyStruct );
    SharewareES . es_Flags        = 0UL;
    SharewareES . es_Title        = "IFF ANIM DataType Shareware notice";
    SharewareES . es_TextFormat   = "Please register this DataType if you're using it more than 30 days\n"
                                    "See docs how to pay the shareware fee.\n"
                                    "To get rid of this requester forever, you must set the REGISTERED switch\n"
                                    "in the prefs-file \"ENVARC:Classes/DataTypes/anim.prefs\"\n"
                                    "then reboot the computer.\n"
                                    "To close the requester successfully, you must answer the following term:\n"
                                    "%s";
    SharewareES . es_GadgetFormat = cbuffer;

    /* The trial and error loop... */
    do
    {
      reqresult = EasyRequest( NULL, (&SharewareES), NULL, buffer );

      if( reqresult == 0L )
      {
        reqresult = NUMCHOICES;
      }

      reqresult--;

    } while( choices[ (reqresult + xc) % NUMCHOICES ] != result );
}
#endif

LONG LoadFrames( struct ClassBase *cb, Object *o )
{
    struct AnimInstData *aid   = (struct AnimInstData *)INST_DATA( (cb -> cb_Lib . cl_Class), o );
    LONG                 error = 0L;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Init */
    InitSemaphore( (&(aid -> aid_SigSem)) );
    NewList( (struct List *)(&(aid -> aid_FrameList)) );
    NewList( (struct List *)(&(aid -> aid_PostedFreeList)) );

    /* Create a memory pool for frame nodes and delta buffers */
    if ((aid -> aid_Pool = CreatePool( MEMF_PUBLIC, 16384UL, 16384UL ) ) != NULL)
    {
      BPTR                 fh = BNULL;                              /* handle (IFF stream handle)      */
      IPTR                sourcetype;                      /* type of stream (either DTST_FILE or DTST_CLIPBOARD */
      struct BitMapHeader *bmh = NULL;                             /* obj's bitmapheader              */
      ULONG                modeid     = (ULONG)INVALID_ID;  /* anim view mode                  */
      ULONG                animwidth  = 0UL,                /* anim width                      */
                           animheight = 0UL,                /* anim height                     */
                           animdepth  = 0UL;                /* anim depth                      */
      ULONG                timestamp  = 0UL;                /* timestamp                       */
      ULONG                minreltime = 1UL,                /* Maximum ah_RelTime value        */
                           maxreltime = 0UL;                /* Minimum ah_RelTime              */
      struct tPoint       *grabpoint  = NULL;               /* Grabbing point of animation     */

        D(bug("[anim.datatype] %s: pool @ 0x%p\n", __func__, aid -> aid_Pool));

      /* Prefs defaults */
      aid -> aid_Volume          = 64UL;
#if !defined(__AROS__)
      aid -> aid_NoDynamicTiming = MAKEBOOL( ((cb -> cb_SuperClassBase -> lib_Version) < 41U) );
#else
    aid -> aid_NoDynamicTiming = FALSE;
#endif
      aid -> aid_ModeID          = (ULONG)INVALID_ID;

      /* Read prefs */
      ReadENVPrefs( cb, aid );

      /* Get file handle, handle type and BitMapHeader */
      if( GetDTAttrs( o, DTA_SourceType,    (&sourcetype),
                         DTA_Handle,        (&fh),
                         DTA_Name,          (&(aid -> aid_ProjectName)),
                         ADTA_BitMapHeader, (&bmh),
                         ADTA_Grab,         (&grabpoint), /* animation.datatype V41 */
                         TAG_DONE ) >= 4UL ) /* ADTA_Grab not supported in V40, e.g. 4 == V40, 5 == V41 */
      {
        struct IFFHandle *iff = NULL;

        D(bug("[anim.datatype] %s: handle @ 0x%p\n", __func__, fh));
        D(bug("[anim.datatype] %s: name @ 0x%p '%s'\n", __func__, aid -> aid_ProjectName, aid -> aid_ProjectName));
        D(bug("[anim.datatype] %s: bmh @ 0x%p\n", __func__, bmh));
        D(bug("[anim.datatype] %s: grab @ 0x%p\n", __func__, grabpoint));

        aid -> aid_BMH = bmh; /* Store BitMapHeader */

        switch( sourcetype )
        {
          case DTST_CLIPBOARD:
          {
                D(bug("[anim.datatype] %s: DTST_CLIPBOARD\n", __func__));

              aid -> aid_LoadAll = TRUE;

              iff = (struct IFFHandle *)fh;
          }
              break;

          case DTST_FILE:
          {
              BPTR iff_file_fh;
              BPTR cloned_fh    = BNULL;

            D(bug("[anim.datatype] %s: DTST_FILE\n", __func__));

              iff = (struct IFFHandle *)fh;

              /* Attempt to open file from given stream (allows usage of virtual fs when using datatypes.library V45) */
              iff_file_fh = (BPTR)(iff -> iff_Stream); /* see iffparse.library/InitIFFasDOS autodoc */

            D(bug("[anim.datatype] %s: iff file handle @ 0x%p\n", __func__, iff_file_fh));

              if( iff_file_fh )
              {
                BPTR lock;

                if ((lock = DupLockFromFH( iff_file_fh ) ) != BNULL)
                {
                  /* Set up a filehandle for disk-based loading (random loading) */
                  if( !(cloned_fh = (BPTR)OpenFromLock( lock )) )
                  {
                    /* failure */
                    UnLock( lock );
                  }
                }
              }

              /* OpenFromLock failed ? - Then open by name :-( */
              if( cloned_fh == BNULL )
              {
                /* Set up a filehandle for disk-based loading (random loading) */
                if (!(cloned_fh = (BPTR)Open( (aid -> aid_ProjectName), MODE_OLDFILE )))
                {
                  /* Can't open file */
                  error = IoErr();
                }
              }

              if( cloned_fh )
              {
#ifdef DOASYNCIO
                if( !(aid -> aid_FH = OpenAsync( cb, cloned_fh, 8192L )) )
                {
                  /* Can't get async access */
                  error = IoErr();
                }
#else
                aid -> aid_FH = cloned_fh;
#endif /* DOASYNCIO */
              }
          }
              break;

          case DTST_RAM:
          {
                D(bug("[anim.datatype] %s: DTST_RAM\n", __func__));

              /* do nothing */
          }
              break;

          default:
          {
              /* unsupported source type */
              error = ERROR_NOT_IMPLEMENTED;
          }
              break;
        }

        /* Any error ? */
        if( error == 0L )
        {
          if( iff )
          {
            struct StoredProperty *svxprop       = NULL, /* 8SVX VHDR*/
                                  *bmhdprop       = NULL, /* ILBM BMHD (struct BitMapHeader)        */
                                  *camgprop       = NULL, /* ILBM CAMG (amiga view mode id)         */
                                  *grabprop       = NULL, /* ILBM GRAB (grabbing point)             */
                                  *dpanprop       = NULL, /* DPaint DPAN chunk                      */
                                  *annoprop       = NULL, /* Generic IFF ANNO (annotation) chunk    */
                                  *authprop       = NULL, /* Generic IFF AUTH (author) chunk        */
                                  *copyrightprop  = NULL, /* Generic IFF (C)  (copyright) chunk     */
                                  *fverprop       = NULL, /* Generic IFF FVER (version) chunk       */
                                  *nameprop       = NULL; /* Generic IFF NAME (name) chunk          */

#define NUM_PROPCHUNKS (10L)
            const
            LONG propchunks[ (NUM_PROPCHUNKS * 2) ] =
            {
              ID_8SVX, ID_VHDR,
              ID_ILBM, ID_BMHD,
              ID_ILBM, ID_CAMG,
              ID_ILBM, ID_GRAB,
              ID_ILBM, ID_DPAN,
              ID_ILBM, ID_ANNO,
              ID_ILBM, ID_AUTH,
              ID_ILBM, ID_Copyright,
              ID_ILBM, ID_FVER,
              ID_ILBM, ID_NAME
            };

            if( !(error = PropChunks( iff, (LONG *)propchunks, NUM_PROPCHUNKS )) )
            {
#define NUM_STOPCHUNKS (6L)
              const
              LONG stopchunks[ (NUM_STOPCHUNKS * 2) ] =
              {
                ID_ILBM, ID_FORM,
                ID_ILBM, ID_ANHD,
                ID_ILBM, ID_CMAP,
                ID_ILBM, ID_BODY,
                ID_ILBM, ID_DLTA,
                ID_ILBM, ID_SBDY
              };

              if( !(error = StopChunks( iff, (LONG *)stopchunks, NUM_STOPCHUNKS )) )
              {
                struct FrameNode *fn         = NULL;
                ULONG             numcmaps   = 0UL; /* number of created cmaps  */

                /* Scan IFF stream until an error or an EOF occurs */
                while(TRUE)
                {
                  struct ContextNode *cn;

                    D(bug("[anim.datatype] %s: Parsing IFF ... \n", __func__));

                  if ((error = ParseIFF( iff, IFFPARSE_SCAN ) ) != 0)
                  {
                    /* EOF (End Of File) is no error here... */
                    if( error == IFFERR_EOF )
                    {
                        D(bug("[anim.datatype] %s: EOF Reached\n", __func__, error));
                      error = 0L;
                    }
                    D(
                        else
                        {
                            bug("[anim.datatype] %s: Error! (%08x)\n", __func__, error);
                        }
                    )
                    break;
                  }

                  if (svxprop == NULL)
                  {
                      if ((svxprop = FindProp( iff, ID_8SVX, ID_VHDR ) ) != NULL)
                      {
                          bug("[anim.datatype] %s: 8svx header\n", __func__);
                      }
                  }

                  /* bmhd header loaded ? */
                  if( bmhdprop == NULL )
                  {
                    if ((bmhdprop = FindProp( iff, ID_ILBM, ID_BMHD ) ) != NULL)
                    {
                      ULONG poolsize,
                            availmem;

                        D(bug("[anim.datatype] %s: BitMapHeader = %d bytes (struct %d)\n", __func__, bmhdprop-> sp_Size, sizeof(struct BitMapHeader)));
#if !defined(__AROS__)
                      *bmh = *((struct BitMapHeader *)(bmhdprop -> sp_Data));
#else
                    bmh->bmh_Width = AROS_BE2WORD(((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_Width);
                    bmh->bmh_Height = AROS_BE2WORD(((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_Height);
                    bmh->bmh_Left = AROS_BE2WORD(((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_Left);
                    bmh->bmh_Top = AROS_BE2WORD(((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_Top);
                    bmh->bmh_Depth = ((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_Depth;
                    bmh->bmh_Masking = ((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_Masking;
                    bmh->bmh_Compression = ((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_Compression;
//                    bmh->bmh_Pad;
                    bmh->bmh_Transparent = AROS_BE2WORD(((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_Transparent);
                    bmh->bmh_XAspect = ((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_XAspect;
                    bmh->bmh_YAspect = ((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_YAspect;
                    bmh->bmh_PageWidth = AROS_BE2WORD(((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_PageWidth);
                    bmh->bmh_PageHeight = AROS_BE2WORD(((struct BitMapHeader *)(bmhdprop -> sp_Data))->bmh_PageHeight);
#endif

                      animwidth  = bmh -> bmh_Width;
                      animheight = bmh -> bmh_Height;
                      animdepth  = bmh -> bmh_Depth;

                      D(bug("[anim.datatype] %s: anim dimension %dx%dx%d\n", __func__, animwidth, animheight, animdepth));
                      availmem = AvailMem( MEMF_PUBLIC );

                      /* Create a seperate pool for frames:
                       * (((width + 7) / 8) * height * depth + struct BitMapHeader + Padding) * 4 frames
                       */
                      poolsize = (((animwidth * animheight * animdepth) / 8UL) + 256UL) * 4UL;

                      /* Shrink pool to a fitting size */
                      while( (poolsize * 4) > availmem )
                      {
                        poolsize /= 2UL;
                      }

                      D(bug("[anim.datatype] %s: pool size = %d\n", __func__, poolsize));

                      
                      /* Create a memory pool for frame bitmaps */
                      if( !(aid -> aid_FramePool = CreatePool( MEMF_PUBLIC, poolsize, poolsize )) )
                      {
                        error = ERROR_NO_FREE_STORE;
                      }
                      D(bug("[anim.datatype] %s: Frame pool @ 0x%p\n", __func__, aid -> aid_FramePool));
                    }
                  }

                  /* camg loaded ? */
                  if( camgprop == NULL )
                  {
                    if ((camgprop = FindProp( iff, ID_ILBM, ID_CAMG ) ) != NULL)
                    {
                      modeid = AROS_BE2LONG(*(ULONG *)(camgprop -> sp_Data));

                      /* Check for invalid flags */
                      if( (!(modeid & MONITOR_ID_MASK)) ||
                          ((modeid & EXTENDED_MODE) &&
                          (!(modeid & 0xFFFF0000UL))) )
                      {
                        /* Remove invalid flags (see include31:graphics/view.h) */
                        modeid &= ~(GENLOCK_VIDEO | PFBA | GENLOCK_AUDIO | DUALPF | EXTENDED_MODE | VP_HIDE | SPRITES );
                      }

                      /* Be safe ! */
                      if( (modeid & 0xFFFF0000UL) && (!(modeid & 0x00001000UL)) )
                      {
                        modeid = (ULONG)INVALID_ID;
                      }
                    }
                  }

                  /* grab loaded ? */
                  if( grabprop == NULL )
                  {
                    if ((grabprop = FindProp( iff, ID_ILBM, ID_GRAB ) ) != NULL)
                    {
                      /* Grab point only available in animation.datatype V41 */
                      if( grabpoint )
                      {
                        *grabpoint = *((struct tPoint *)(grabprop -> sp_Data));
                      }

                      verbose_printf( cb, aid, "animation GRAB point x=%ld, y=%ld\n",
                                      (long)(((struct tPoint *)(grabprop -> sp_Data)) -> x),
                                      (long)(((struct tPoint *)(grabprop -> sp_Data)) -> y) );
                    }
                  }

                  /* dpan loaded ? */
                  if( dpanprop == NULL )
                  {
                    if ((dpanprop = FindProp( iff, ID_ILBM, ID_DPAN ) ) != NULL)
                    {
                      if( (aid -> aid_FPS) == 0UL )
                      {
                        struct DPAnimChunk *dpan = (struct DPAnimChunk *)(dpanprop -> sp_Data);

                        if (dpan -> dpan_FPS <= 60 )
                        {
                          aid -> aid_FPS = dpan -> dpan_FPS;

                          verbose_printf( cb, aid, "DPAN found, FPS set to %lu\n", (aid -> aid_FPS) );
                        }
                        else
                        {
                          verbose_printf( cb, aid, "DPAN found, ignoring invalid FPS value %lu\n", (ULONG)(dpan -> dpan_FPS) );
                        }
                      }
                    }
                  }

                  if( annoprop == NULL )
                  {
                    /* IFF ANNO found ? */
                    if ((annoprop = FindProp( iff, ID_ILBM, ID_ANNO ) ) != NULL)
                    {
                      STRPTR buff;

                      /* Allocate a temp buffer so that stccpy can add a '\0'-terminator */
                      if ((buff = (STRPTR)AllocVec( ((annoprop -> sp_Size) + 2UL), MEMF_ANY ) ) != NULL)
                      {
                        stccpy( buff, (annoprop -> sp_Data), (int)(annoprop -> sp_Size) );

                        verbose_printf( cb, aid, "ANNO annotation: \"%s\"\n", buff );

                        SetDTAttrs( o, NULL, NULL, DTA_ObjAnnotation, buff, TAG_DONE );

                        FreeVec( buff );
                      }
                      else
                      {
                        /* no temp. buffer */
                        error = ERROR_NO_FREE_STORE;
                      }
                    }
                  }

                  if( authprop == NULL )
                  {
                    /* IFF AUTH found ? */
                    if ((authprop = FindProp( iff, ID_ILBM, ID_AUTH ) ) != NULL)
                    {
                      STRPTR buff;

                      /* Allocate a temp buffer so that stccpy can add a '\0'-terminator */
                      if ((buff = (STRPTR)AllocVec( ((authprop -> sp_Size) + 2UL), MEMF_ANY ) ) != NULL)
                      {
                        stccpy( buff, (authprop -> sp_Data), (int)(authprop -> sp_Size) );

                        verbose_printf( cb, aid, "AUTH author: \"%s\"\n", buff );

                        SetDTAttrs( o, NULL, NULL, DTA_ObjAuthor, buff, TAG_DONE );

                        FreeVec( buff );
                      }
                      else
                      {
                        /* no temp. buffer */
                        error = ERROR_NO_FREE_STORE;
                      }
                    }
                  }

                  if( copyrightprop == NULL )
                  {
                    /* IFF (C) found ? */
                    if ((copyrightprop = FindProp( iff, ID_ILBM, ID_Copyright ) ) != NULL)
                    {
                      STRPTR buff;

                      /* Allocate a temp buffer so that stccpy can add a '\0'-terminator */
                      if ((buff = (STRPTR)AllocVec( ((copyrightprop -> sp_Size) + 2UL), MEMF_ANY ) ) != NULL)
                      {
                        stccpy( buff, (copyrightprop -> sp_Data), (int)(copyrightprop -> sp_Size) );

                        verbose_printf( cb, aid, "(C) copyright: \"%s\"\n", buff );

                        SetDTAttrs( o, NULL, NULL, DTA_ObjCopyright, buff, TAG_DONE );

                        FreeVec( buff );
                      }
                      else
                      {
                        /* no temp. buffer */
                        error = ERROR_NO_FREE_STORE;
                      }
                    }
                  }

                  if( fverprop == NULL )
                  {
                    /* IFF FVER found ? */
                    if ((fverprop = FindProp( iff, ID_ILBM, ID_FVER ) ) != NULL)
                    {
                      STRPTR buff;

                      /* Allocate a temp buffer so that stccpy can add a '\0'-terminator */
                      if ((buff = (STRPTR)AllocVec( ((fverprop -> sp_Size) + 2UL), MEMF_ANY ) ) != NULL)
                      {
                        stccpy( buff, (fverprop -> sp_Data), (int)(fverprop -> sp_Size) );

                        verbose_printf( cb, aid, "FVER version: \"%s\"\n", buff );

                        SetDTAttrs( o, NULL, NULL, DTA_ObjVersion, buff, TAG_DONE );

                        FreeVec( buff );
                      }
                      else
                      {
                        /* no temp. buffer */
                        error = ERROR_NO_FREE_STORE;
                      }
                    }
                  }

                  if( nameprop == NULL )
                  {
                    /* IFF NAME found ? */
                    if ((nameprop = FindProp( iff, ID_ILBM, ID_NAME ) ) != NULL)
                    {
                      STRPTR buff;

                      D(bug("[anim.datatype] %s: Name '%s'\n", __func__, nameprop -> sp_Data));
                      /* Allocate a temp buffer so that stccpy can add a '\0'-terminator */
                      if ((buff = (STRPTR)AllocVec( ((nameprop -> sp_Size) + 2UL), MEMF_ANY ) ) != NULL)
                      {
                        stccpy( buff, (nameprop -> sp_Data), (int)(nameprop -> sp_Size) );

                        verbose_printf( cb, aid, "NAME name: \"%s\"\n", buff );

                        SetDTAttrs( o, NULL, NULL, DTA_ObjName, buff, TAG_DONE );

                        FreeVec( buff );
                      }
                      else
                      {
                        /* no temp. buffer */
                        error = ERROR_NO_FREE_STORE;
                      }
                    }
                  }

                  if ((cn = CurrentChunk( iff ) ) != NULL)
                  {
                    switch( (cn -> cn_Type) )
                    {
                      case ID_ILBM:
                      {
                            D(bug("[anim.datatype] %s: ID_ILBM\n", __func__));
                          switch( (cn -> cn_ID) )
                          {
                            case ID_FORM:
                            {
                                D(bug("[anim.datatype] %s: ID_FORM\n", __func__));
                                /* Create and prepare a new frame node */
                                if ((fn = AllocFrameNode( cb, (aid -> aid_Pool) ) ) != NULL)
                                {
                                    AddTail( (struct List *)(&(aid -> aid_FrameList)), (struct Node *)(&(fn -> fn_Node)) );

                                    fn -> fn_TimeStamp = timestamp++;
                                    fn -> fn_Frame     = fn -> fn_TimeStamp;

                                    D(bug("[anim.datatype] %s: FrameNode #%d @ 0x%p (%d)\n", __func__, fn -> fn_Frame, fn, fn -> fn_TimeStamp));

                                    fn -> fn_PrevFrame = fn;
                                }
                                else
                                {
                                  /* can't alloc frame node */
                                  error = ERROR_NO_FREE_STORE;
                                }
                            }
                                break;

                            case ID_ANHD:
                            {
                                D(bug("[anim.datatype] %s: ID_ANHD\n", __func__));
                                if( fn )
                                {
                                  ULONG interleave;

                                  /* Read struct AnimHeader */
                                  error = ReadChunkBytes( iff, (&(fn -> fn_AH)), (LONG)sizeof( struct AnimHeader ) );
                                  if( error == (LONG)sizeof( struct AnimHeader ) ) error = 0L;
                                  /* Info */

#if defined(__AROS__)
                                fn->fn_AH.ah_Width = AROS_BE2WORD(fn->fn_AH.ah_Width);
                                fn->fn_AH.ah_Height = AROS_BE2WORD(fn->fn_AH.ah_Height);
                                fn->fn_AH.ah_Left = AROS_BE2WORD(fn->fn_AH.ah_Left);
                                fn->fn_AH.ah_Top = AROS_BE2WORD(fn->fn_AH.ah_Top);
                                fn->fn_AH.ah_AbsTime = AROS_BE2LONG(fn->fn_AH.ah_AbsTime);
                                fn->fn_AH.ah_RelTime = AROS_BE2LONG(fn->fn_AH.ah_RelTime);
                                fn->fn_AH.ah_Flags = AROS_BE2LONG(fn->fn_AH.ah_Flags);
                                if (aid->aid_AnimMode == 0)
                                    aid->aid_AnimMode = fn->fn_AH.ah_Operation;
#endif
                                  DumpAnimHeader( cb, aid, (fn -> fn_TimeStamp), (&(fn -> fn_AH)) );

                                  /* Check if we have dynamic timing */
                                  maxreltime = MAX( maxreltime, (fn -> fn_AH . ah_RelTime) );
                                  minreltime = MIN( minreltime, (fn -> fn_AH . ah_RelTime) );

                                  interleave = (ULONG)(fn -> fn_AH . ah_Interleave);

                                  /* An interleave of 0 means two frames back */
                                  if( interleave == 0 )
                                  {
                                    interleave = 2;
                                  }

                                  D(bug("[anim.datatype] %s: interleave = %d\n", __func__, interleave));
                                  /* Get previous frame */
                                  fn -> fn_PrevFrame = GetPrevFrameNode( fn, interleave );
                                  D(bug("[anim.datatype] %s: PrevFrame @ 0x%p\n", __func__, fn -> fn_PrevFrame));
                                }
                            }
                                break;

                            case ID_CMAP:
                            {
                                D(bug("[anim.datatype] %s: ID_CMAP\n", __func__));
                                if( fn )
                                {
                                  UBYTE *buff;

                                  /* Allocate buffer */
                                  if ((buff = (UBYTE *)AllocPooledVec( cb, (aid -> aid_Pool), ((cn -> cn_Size) + 16) ) ) != NULL)
                                  {
                                    D(bug("[anim.datatype] %s: buffer @ 0x%p\n", __func__, buff));
                                    /* Load CMAP data */
                                    error = ReadChunkBytes( iff, buff, (cn -> cn_Size) );

                                      D(bug("[anim.datatype] %s: read %d bytes\n", __func__, error));
                                    /* All read ? */
                                    if( error == (cn -> cn_Size) )
                                    {
                                      error = 0L; /* Success ! */

                                      if( timestamp == 1UL )
                                      {
                                        if( !CMAP2Object( cb, o, buff, (cn -> cn_Size) ) )
                                        {
                                            D(bug("[anim.datatype] %s: failed to map colors to object\n", __func__));
                                          /* can't alloc object's color table */
                                          error = ERROR_NO_FREE_STORE;
                                        }
                                      }

                                      /* Any failure ? */
                                      if( error == 0L )
                                      {
                                        if( aid -> aid_NoCMAPs )
                                        {
                                          error = 0L; /* Success ! */
                                        }
                                        else
                                        {
                                          if ((fn -> fn_CMap = CMAP2ColorMap( cb, aid, buff, (cn -> cn_Size) ) ) != NULL)
                                          {
                                            D(bug("[anim.datatype] %s: frame colormap @ 0x%p\n", __func__, fn -> fn_CMap));
                                            error = 0L; /* Success ! */
                                            numcmaps++;
                                          }
                                          else
                                          {
                                            /* no colormap */
                                            error = ERROR_NO_FREE_STORE;
                                          }
                                        }
                                      }
                                    }
                                    D(
                                    else
                                    {
                                       bug("[anim.datatype] %s: != %d bytes !!!!\n", __func__, cn -> cn_Size);
                                    })

                                    FreePooledVec( cb, (aid -> aid_Pool), buff );
                                  }
                                  else
                                  {
                                    /* no load buff */
                                    error = ERROR_NO_FREE_STORE;
                                  }
                                }
                            }
                                break;

                            case ID_SBDY:
                                bug("[anim.datatype] %s: ID_SBDY\n", __func__);
                                if( fn )
                                {
                                    if ((fn->fn_Sample = (UBYTE *)AllocPooledVec( cb, (aid -> aid_Pool), ((cn -> cn_Size)) ) ) != NULL)
                                    {
                                        error = ReadChunkBytes( iff, fn->fn_Sample, (cn -> cn_Size) );
                                        fn->fn_SampleLength = cn -> cn_Size;
                                        D(bug("[anim.datatype] %s: read %d bytes into buffer @ 0x%p\n", __func__, fn->fn_SampleLength, fn->fn_Sample);)
                                    }
                                }
                                break;

                            case ID_BODY:
                            case ID_DLTA:
                            {
                                D(
                                    bug("[anim.datatype] %s: %s\n", __func__, (cn -> cn_ID == ID_BODY) ? "ID_BODY" : "ID_DLTA");
                                )
                                if( fn )
                                {
                                  /* Store position of DLTA */
                                  fn -> fn_BMOffset = Seek((BPTR)iff->iff_Stream, 0, OFFSET_CURRENT);
                                  fn -> fn_BMSize   = cn -> cn_Size;
                                   
                                  if( (fn -> fn_BitMap) == NULL )
                                  {
                                    /* Preload frames only if requested or if this is the key frame (first frame of anim) */
                                    if( (aid -> aid_LoadAll) || ((fn -> fn_TimeStamp) == 0) )
                                    {
                                      if( animwidth && animheight && animdepth )
                                      {
                                        if ((fn -> fn_BitMap = AllocBitMapPooled( cb, animwidth, animheight, animdepth, (aid -> aid_FramePool) )) != NULL)
                                        {
                                          UBYTE *buff;

                                            D(bug("[anim.datatype] %s: bitmap @ 0x%p\n", __func__, fn -> fn_BitMap));
                                          /* Allocate buffer */
                                          if ((buff = (UBYTE *)AllocPooledVec( cb, (aid -> aid_Pool), ((cn -> cn_Size) + 32) ) ) != NULL)
                                          {
                                            struct FrameNode *prevfn;

                                              D(bug("[anim.datatype] %s: buffer @ 0x%p\n", __func__, buff));

                                            /* Clear buffer to get rid of some problems with corrupted DLTAs */
                                            memset( (void *)buff, 0, (size_t)((cn -> cn_Size) + 31) );

                                              if ((fn -> fn_TimeStamp == 0) && (fn -> fn_AH . ah_Operation != acmpILBM))
                                                  ClearBitMap( fn -> fn_BitMap );

                                            /* Get previous frame */
                                            prevfn = fn -> fn_PrevFrame;
                                            D(bug("[anim.datatype] %s: prevfn @ 0x%p, prev->bm @ 0x%p\n", __func__, prevfn, prevfn -> fn_BitMap));

                                            /* Load delta data */
                                            error = ReadChunkBytes( iff, buff, (cn -> cn_Size) );

                                            /* All bytes read ? */
                                            if( error == (cn -> cn_Size) )
                                            {
                                              error = DrawDLTA( cb, aid, (prevfn -> fn_BitMap), (fn -> fn_BitMap), (&(fn -> fn_AH)), buff, (cn -> cn_Size) );

                                              if( error )
                                              {
                                                error_printf( cb, aid, "scan/load: dlta unpacking error %lu\n", error );
                                              }
                                            }

                                            FreePooledVec( cb, (aid -> aid_Pool), buff );
                                          }
                                          else
                                          {
                                            /* no load buff */
                                            error = ERROR_NO_FREE_STORE;
                                          }
                                        }
                                        else
                                        {
                                          /* no bitmap */
                                          error = ERROR_NO_FREE_STORE;
                                        }
                                      }
                                      else
                                      {
                                        /* no dimensions for bitmap (possibly a missing bmhd) */
                                        error = DTERROR_NOT_ENOUGH_DATA;
                                      }
                                    }
                                  }
                                  else
                                  {
                                    error_printf( cb, aid, "scan/load: bitmap already loaded @ 0x%p\n", fn->fn_BitMap);
                                  }
                                }
                            }
                                break;
                          }
                      }
                          break;
                    }
                  }

                  /* on error: leave for-loop */
                  if( error )
                  {
                    D(bug("[anim.datatype] %s: error occured\n", __func__));
                    break;
                  }
                }

                /* Any frames ? */
                if( timestamp && (error == 0L) && numcmaps )
                {
                  if( numcmaps == 1UL )
                  {
                    /* We only have a global colormap and no colormap changes,
                     * delete first colormap (a colormap in the first frames indicates following colormap
                     * changes)
                     */
                    struct FrameNode *firstnode = (struct FrameNode *)(aid -> aid_FrameList . mlh_Head);

                    if( firstnode -> fn_CMap )
                    {
                      FreeColorMap( (firstnode -> fn_CMap) );
                      firstnode -> fn_CMap = NULL;
                    }
                  }
                  else
                  {
                    /* All frames must have a colormap, therefore we replicate the colormap
                     * from the previous colormap if one is missing
                     */
                    struct FrameNode *worknode,
                                     *nextnode;
                    struct ColorMap  *currcm = NULL;

                    verbose_printf( cb, aid, "Animation has palette changes per frame\n" );

                    worknode = (struct FrameNode *)(aid -> aid_FrameList . mlh_Head);

                    while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
                    {
                      if( worknode -> fn_CMap )
                      {
                        /* Current node contains colormap, this are the colors for the following frames... */
                        currcm = worknode -> fn_CMap;
                      }
                      else
                      {
                        if( currcm )
                        {
                          /* Copy colormap from previous one... */
                          if( !(worknode -> fn_CMap = CopyColorMap( cb, currcm )) )
                          {
                            /* Can't copy/alloc colormap */
                            error = ERROR_NO_FREE_STORE;
                          }
                        }
                        else
                        {
                          verbose_printf( cb, aid, "scan/load: no colormap, can't copy it\n" );
                        }
                      }

                      worknode = nextnode;
                    }
                  }
                }
              }
            }

            /* Check for required information */
            if( error == 0L )
            {
              /* bmh information available  ? */
              if( bmhdprop == NULL )
              {
                verbose_printf( cb, aid, "scan: no bmhd found\n" );

                /* BMHD missing */
                error = DTERROR_INVALID_DATA;
              }
              else
              {
                /* Any frames loaded ? */
                if( timestamp == 0UL )
                {
                  /* not enougth frames (at least one required) */
                  error = DTERROR_NOT_ENOUGH_DATA;
                }
              }
            }

            /* Dynamic timing ? */
            if( (minreltime != maxreltime) && ((aid -> aid_NoDynamicTiming) == FALSE) )
            {
              struct FrameNode *worknode,
                               *nextnode;
              ULONG             shift = 0UL;    
              
              if( minreltime == 0UL )
              {
                shift = 1UL;
              }

              verbose_printf( cb, aid, "using dynamic timing\n" );

              /* Renumber timestamps */
              timestamp = 0UL; /* again we count from 0 */

              worknode = (struct FrameNode *)(aid -> aid_FrameList . mlh_Head);

              while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
              {
                ULONG duration = (worknode -> fn_AH . ah_RelTime) + shift - 1UL;

                D(bug("[anim.datatype] %s: setting node @ 0x%p to %d:%d:%d\n", __func__, worknode, timestamp, timestamp, duration));
                worknode -> fn_TimeStamp = timestamp;
                worknode -> fn_Frame     = timestamp;
                worknode -> fn_Duration  = duration;

                timestamp += (duration + 1UL);

                worknode = nextnode;
              }
            }

            /* Any error ? */
            if( error == 0L )
            {
              /* Alloc bitmap as key bitmap  */
              if ((aid -> aid_KeyBitMap = AllocBitMap( animwidth, animheight, animdepth, (BMF_CLEAR | BMF_MINPLANES), NULL ) ) != NULL)
              {
                struct FrameNode *firstfn = (struct FrameNode *)(aid -> aid_FrameList . mlh_Head); /* short cut to the first FrameNode */
                
                aid -> aid_CurrFN = firstfn;

                if( (firstfn -> fn_BitMap) == NULL )
                {
                  if( !(firstfn -> fn_BitMap = AllocBitMapPooled( cb, (ULONG)(aid -> aid_BMH -> bmh_Width), (ULONG)(aid -> aid_BMH -> bmh_Height), (ULONG)(aid -> aid_BMH -> bmh_Depth), (aid -> aid_FramePool) )) )
                  {
                    /* can't alloc first bitmap */
                    error = ERROR_NO_FREE_STORE;
                  }
                }

                if( error == 0L )
                {
                  /* Copy first frame into key bitmap */
                  CopyBitMap( cb, (firstfn -> fn_BitMap), (aid -> aid_KeyBitMap) );

                  /* No name chunk ? */
                  if( nameprop == NULL )
                  {
                    STRPTR name;

                    GetDTAttrs( o, DTA_Name, (&name), TAG_DONE );
                    SetDTAttrs( o, NULL, NULL, DTA_ObjName, name, TAG_DONE );
                  }

                  /* ModeID... */
                  if( (aid -> aid_ModeID) != INVALID_ID )
                  {
                    modeid = aid -> aid_ModeID;
                  }
                  else
                  {
                    /* No mode id ? */
                    if( modeid == INVALID_ID )
                    {
                      /* BUG: Does currently not support SUPERHIRES modes */
                      if( animwidth >= 640UL )
                      {
                        if( animheight >= 400 )
                        {
                          modeid = HIRESLACE_KEY;
                        }
                        else
                        {
                          modeid = HIRES_KEY;
                        }
                      }
                      else
                      {
                        if( animheight >= 400 )
                        {
                          modeid = LORESLACE_KEY;
                        }
                        else
                        {
                          modeid = LORES_KEY;
                        }
                      }
                    }
                  }

                  /* No FPS rate found ? */
                  if( (aid -> aid_FPS) == 0UL )
                  {
                    ULONG adaptive;

                    aid -> aid_FPS = 15UL;

                    if( GetDTAttrs( o, ADTA_AdaptiveFPS, (&adaptive), TAG_DONE ) == 1UL )
                    {
                      if( adaptive )
                      {
                        verbose_printf( cb, aid, "animation.datatype uses adaptive playback rate selection\n" );

                        aid -> aid_FPS = 60UL; /* 1/60 sec per frame */
                      }
                    }
                  }

                  /* Attach external sound */
                  AttachSample( cb, aid );

                  /* Infos */
                  verbose_printf( cb, aid, "width %lu height %lu depth %lu frames %lu fps %lu\n",
                                animwidth,
                                animheight,
                                animdepth,
                                timestamp,
                                (aid -> aid_FPS) );

                  /* Set misc attributes */
                  SetDTAttrs( o, NULL, NULL,
                              DTA_TotalHoriz,       animwidth,
                              DTA_TotalVert,        animheight,
                              ADTA_Width,           animwidth,
                              ADTA_Height,          animheight,
                              ADTA_Depth,           animdepth,
                              ADTA_ModeID,          modeid,
                              ADTA_Frames,          timestamp,
                              ADTA_FramesPerSecond, (aid -> aid_FPS),
                              ADTA_KeyFrame,        (aid -> aid_KeyBitMap),
                              ADTA_Sample,          (firstfn -> fn_Sample),
                              ADTA_SampleLength,    (firstfn -> fn_SampleLength),
                              ADTA_Period,          (firstfn -> fn_Period),
                              ADTA_Volume,          (aid -> aid_Volume),
                              ADTA_Cycles,          1UL,
                              TAG_DONE );
                }
              }
              else
              {
                /* can't alloc key bitmap */
                error = ERROR_NO_FREE_STORE;
              }
            }
          }
          else
          {
            /* No iff handle ? - Be sure we got a DTST_RAM sourcetype */
            if( sourcetype != DTST_RAM )
            {
              /* No handle ! */
              error = ERROR_REQUIRED_ARG_MISSING;
            }
          }
        }
      }
      else
      {
        /* can't get required attributes from superclass */
        error = ERROR_OBJECT_WRONG_TYPE;
      }
    }
    else
    {
      /* no memory pool */
      error = ERROR_NO_FREE_STORE;
    }

    /* Error codes below 0 are related to the IFFParse.library functions */
    if( error < 0L )
    {
      verbose_printf( cb, aid, "iff error %ld\n", (long)error );

      /* convert IFFParse error to DOS error */
      error = ifferr2doserr[ (-error - 1) ];
    }

    return( error );
}


static
struct FrameNode *AllocFrameNode( struct ClassBase *cb, APTR pool )
{
    struct FrameNode *fn;

    D(bug("[anim.datatype] %s()\n", __func__));

    if ((fn = (struct FrameNode *)AllocPooled( pool, (ULONG)sizeof( struct FrameNode ) ) ) != NULL)
    {
      memset( fn, 0, sizeof( struct FrameNode ) );
    }

    return( fn );
}


struct FrameNode *FindFrameNode( struct MinList *fnl, ULONG timestamp )
{
    D(bug("[anim.datatype] %s(0x%p : %d)\n", __func__, fnl, timestamp));

    if( fnl )
    {
        struct FrameNode *worknode,
                       *nextnode,
                       *prevnode;

        prevnode = worknode = (struct FrameNode *)(fnl -> mlh_Head);

        while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
        {
            D(bug("[anim.datatype] %s: worknode 0x%p, prevnode 0x%p\n", __func__, worknode, prevnode));
            D(bug("[anim.datatype] %s: worknode frame #%d, ts = %d\n", __func__, worknode -> fn_Frame, worknode -> fn_TimeStamp));
            if( (worknode -> fn_TimeStamp) > timestamp )
            {
                return( prevnode );
            }

            prevnode = worknode;
            worknode = nextnode;
        }

        if( !IsListEmpty( ((struct List *)fnl) ) )
        {
            return( prevnode );
        }  
    }

    return( NULL );
}


void FreeFrameNodeResources( struct ClassBase *cb, struct MinList *fnl )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( fnl )
    {
      struct FrameNode *worknode,
                       *nextnode;

      worknode = (struct FrameNode *)(fnl -> mlh_Head);

      while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
      {
        if( worknode -> fn_CMap )
        {
          FreeColorMap( (worknode -> fn_CMap) );
          worknode -> fn_CMap = NULL;
        }

        worknode = nextnode;
      }
    }
}


/* Copy bm1 to bm2 */
void CopyBitMap( struct ClassBase *cb, struct BitMap *bm1, struct BitMap *bm2 )
{
    ULONG  bpr1 = bm1 -> BytesPerRow;
    ULONG  bpr2 = bm2 -> BytesPerRow;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Same bitmap layout ? */
    if( bpr1 == bpr2 )
    {
      /* Interleaved BitMap ? */
      if( ((bm1 -> Planes[ 1 ]) - (bm1 -> Planes[ 0 ])) == (bpr1 / (ULONG)(bm1 -> Depth)) )
      {
        ULONG planesize = bpr2 * (ULONG)(bm2 -> Rows);

        XCopyMem( cb, (bm1 -> Planes[ 0 ]), (bm2 -> Planes[ 0 ]), planesize );
      }
      else
      {
        ULONG planesize = bpr2 * (ULONG)(bm2 -> Rows);
        UWORD i;

        for( i = 0U ; i < (bm2 -> Depth) ; i++ )
        {
          XCopyMem( cb, (bm1 -> Planes[ i ]), (bm2 -> Planes[ i ]), planesize );
        }
      }
    }
    else
    {
      register UBYTE *src;
      register UBYTE *dst;
      register LONG   r;
      register LONG   p;
               ULONG  width = bm1 -> BytesPerRow;

      /* Interleaved BitMap ? */
      if( ((bm1 -> Planes[ 1 ]) - (bm1 -> Planes[ 0 ])) == (bpr1 / (ULONG)(bm1 -> Depth)) )
      {
        width /= (bm1 -> Depth);
      }

      for( p = bm1 -> Depth - 1 ; p >= 0 ; p-- )
      {
        src = (BYTE *)bm1 -> Planes[ p ];
        dst = (BYTE *)bm2 -> Planes[ p ];

        for( r = bm1 -> Rows - 1 ; r >= 0 ; r-- )
        {
          CopyMem( src, dst, width );
          src += bpr1;
          dst += bpr2;
        }
      }
    }
}


static
void XCopyMem( struct ClassBase *cb, APTR src, APTR dest, ULONG size )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    /* Check if we can use the optimized CopyMemQuick */
    if( (ALIGN_LONG( src ) == src) && (ALIGN_LONG( dest ) == dest) )
    {
      register ULONG lsize = size & ~3UL,
                     cut   = size - lsize; /* remaining bytes (0-3) */

      CopyMemQuick( src, dest, lsize );

      if( cut )
      {
        src  = ((UBYTE *)src)  + lsize;
        dest = ((UBYTE *)dest) + lsize;

        CopyMem( src, dest, cut );
      }
    }
    else
    {
      CopyMem( src, dest, size );
    }
}


void ClearBitMap( struct BitMap *bm )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( bm )
    {
      ULONG planesize = (ULONG)(bm -> BytesPerRow) * (ULONG)(bm -> Rows);
      UWORD i;

      for( i = 0 ; i < (bm -> Depth) ; i++ )
      {
        memset( (bm -> Planes[ i ]), 0, (size_t)planesize );
      }
    }
}


struct BitMap *AllocBitMapPooled( struct ClassBase *cb, ULONG width, ULONG height, ULONG depth, APTR pool)
{
    struct BitMap *bm;
    ULONG          planesize,
                   moredepthsize,
                   size;

    D(bug("[anim.datatype] %s()\n", __func__));

    planesize       = (ULONG)RASSIZE( ((width + 63UL) & ~63UL), height );
    moredepthsize   = (depth > 8UL)?((depth - 8UL) * sizeof( PLANEPTR )):(0UL);
    size            = ((ULONG)sizeof( struct BitMap )) + moredepthsize + (planesize * depth) + 31UL;

    if ((bm = (struct BitMap *)AllocPooledVec( cb, pool, size ) ) != NULL)
    {
      UWORD    pl;
      PLANEPTR plane;

      InitBitMap( bm, depth, width, height );

      plane = (PLANEPTR)ALIGN_QUADLONG( ((IPTR)bm + sizeof( struct BitMap )) ); /* First plane follows struct BitMap */

      /* Set up plane data */
      pl = 0U;

      /* Set up plane ptrs */
      while( pl < depth )
      {
        bm -> Planes[ pl ] = plane;

        plane = (PLANEPTR)ALIGN_QUADLONG( ((IPTR)plane + planesize) );
        pl++;
      }

      /* Clear the remaining plane ptrs (up to 8 planes) */
      while( pl < 8U )
      {
        bm -> Planes[ pl ] = NULL;

        pl++;
      }
    }

    return( bm );
}


BOOL CMAP2Object( struct ClassBase *cb, Object *o, UBYTE *rgb, ULONG rgbsize )
{
    struct ColorRegister *acm;
    ULONG                *acregs;
    IPTR                 nc;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* file has this many colors (e.g. each color has one byte per R,B,G-gun) */
    nc = rgbsize / 3UL;

    SetDTAttrs( o, NULL, NULL, ADTA_NumColors, nc, TAG_DONE );

    /* Get color context */
    if( GetDTAttrs( o,
                    ADTA_ColorRegisters, (&acm),
                    ADTA_CRegs,          (&acregs),
                    ADTA_NumColors,      (&nc),
                    TAG_DONE ) == 3 )
    {
      /* All valid ? */
      if( acm && acregs && nc )
      {
        ULONG i;

        for( i = 0UL ; i < nc ; i++, acm++ )
        {
          acm -> red   =  *rgb++;
          acm -> green =  *rgb++;
          acm -> blue  =  *rgb++;

          /* Replicate the color information.
           * This surrounds an OS bug which uses the low-oreder bytes of the 32-bit colors
           * instead of the high order ones
           */
          acregs[ ((i * 3) + 0) ] = AROS_BE2LONG(((ULONG)(acm -> red))   * 0x01010101UL);
          acregs[ ((i * 3) + 1) ] = AROS_BE2LONG(((ULONG)(acm -> green)) * 0x01010101UL);
          acregs[ ((i * 3) + 2) ] = AROS_BE2LONG(((ULONG)(acm -> blue))  * 0x01010101UL);
        }

        return( TRUE );
      }
    }

    return( FALSE );
}


struct ColorMap *CMAP2ColorMap( struct ClassBase *cb, struct AnimInstData *aid, UBYTE *rgb, ULONG rgbsize )
{
    struct ColorMap *cm;
    ULONG            a_nc   = (1UL << (ULONG)(aid -> aid_BMH -> bmh_Depth)); /* Number of colors in animation */
    ULONG            rgb_nc = rgbsize / 3UL;                                 /* Number of colors in CMAP      */

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Get a colormap which hold all colors */
    if ((cm = GetColorMap( (long)MAX( a_nc, rgb_nc ) ) ) != NULL)
    {
      ULONG i,
            r, g, b;

      for( i = 0UL ; i < rgb_nc ; i++ )
      {
        r = *rgb++;
        g = *rgb++;
        b = *rgb++;

        /* Replicate color information (see CMAP2Object for details) and store them into colormap */
        SetRGB32CM( cm, i, AROS_BE2LONG(r * 0x01010101UL), AROS_BE2LONG(g * 0x01010101UL), AROS_BE2LONG(b * 0x01010101UL) );
      }

      /* BUG: the remaining entries should be filled with colors from the last colormap */
      for( ; i < a_nc ; i++ )
      {
        SetRGB32CM( cm, i, 0UL, 0UL, 0UL ); /* fill remaining entries with black */
      }
    }

    return( cm );
}


struct ColorMap *CopyColorMap( struct ClassBase *cb, struct ColorMap *src )
{
    struct ColorMap *dest = NULL;

    D(bug("[anim.datatype] %s()\n", __func__));

    if( src )
    {
      ULONG *ctable;

      if ((ctable = (ULONG *)AllocVec( ((ULONG)(src -> Count) * sizeof( ULONG ) * 3UL), MEMF_PUBLIC ) ) != NULL)
      {
        if ((dest = GetColorMap( (long)(src -> Count) ) ) != NULL)
        {
          ULONG i;

          GetRGB32( src, 0UL, (ULONG)(src -> Count), ctable );

          for( i = 0UL ; i < (src -> Count) ; i++ )
          {
            SetRGB32CM( dest, i, ctable[ ((i * 3) + 0) ], ctable[ ((i * 3) + 1) ], ctable[ ((i * 3) + 2) ] );
          }
        }

        FreeVec( ctable );
      }
    }

    return( dest );
}


/*****************************************************************************/

#if !defined(__AROS__)
#define DEBUG_POOLS 1

#ifndef DEBUG_POOLS
static
APTR AllocPooledVec( struct ClassBase *cb, APTR pool, ULONG memsize )
{
    IPTR *memory = NULL;

    if( pool && memsize )
    {
      memsize += (IPTR)sizeof( IPTR );

      if( memory = (IPTR *)AllocPooled( pool, memsize ) )
      {
        (*memory) = (IPTR)memsize;

        memory++;
      }
    }

    return( (APTR)memory );
}


static
void FreePooledVec( struct ClassBase *cb, APTR pool, APTR mem )
{
    if( pool && mem )
    {
      IPTR *memory;

      memory = (IPTR *)mem;

      memory--;

      FreePooled( pool, memory, (*memory) );
    }
}
#else
#define MY_MAGIC_MEMID (0xD0ADEAD0UL)

APTR AllocPooledVec( struct ClassBase *cb, APTR pool, ULONG memsize )
{
    IPTR *memory = NULL;

    if( pool && memsize )
    {
      memsize += (ULONG)sizeof( IPTR );
      memsize += (ULONG)sizeof( APTR );

      if ((memory = (IPTR *)AllocPooled( pool, memsize ) ) != NULL)
      {
        (*memory) = (IPTR)memsize;
        memory++;

        (*memory) = (IPTR)pool;
        memory++;

        (*memory) = MY_MAGIC_MEMID;
        memory++;
      }
    }

    return( (APTR)memory );
}


void FreePooledVec( struct ClassBase *cb, APTR pool, APTR mem )
{
    if( pool && mem )
    {
      IPTR *memory;

      memory = (IPTR *)mem;

      memory--;

      if( *memory != MY_MAGIC_MEMID )
      {
        D( kprintf( "wrong mem id\n" ) );
        return;
      }

      memory--;

      if( *memory != (IPTR)pool )
      {
        D( kprintf( "wrong pool %lx %lx\n", pool, *memory ) );
        return;
      }

      memory--;

      FreePooled( pool, memory, (*memory) );
    }
}
#endif /* DEBUG_POOLS */
#endif

LONG DrawDLTA( struct ClassBase *cb, struct AnimInstData *aid, struct BitMap *prevbm, struct BitMap *bm, struct AnimHeader *ah, UBYTE *dlta, ULONG dltasize )
{
    LONG error = 0L;

    D(bug("[anim.datatype] %s()\n", __func__));

    if( bm && ah && dlta && dltasize )
    {
        struct BitMap       *unpackbm = bm,
                          *tempbm   = NULL;
        struct BitMapHeader *bmh      = aid -> aid_BMH;
        BOOL                 DoXOR;

        /* Handle acmpILBM, acmpXORILBM and acmpAnimJ explicitly */
        switch( ah -> ah_Operation )
        {
            case acmpILBM:    /*  0  */
                {
                    /* unpack ILBM BODY */
                    return( cb->unpackilbmbody( cb, unpackbm, bmh, dlta, dltasize ) );
                }

            case acmpXORILBM: /*  1  */
                {
                    LONG retval;
                    /* unpack ILBM BODY */
                    if ((retval = cb->unpackilbmbody( cb, unpackbm, bmh, dlta, dltasize )) == 0)
                    {
                        /* XOR against previous frame */
                        retval = cb->xorbm( ah, unpackbm, prevbm );
                    }
                    return ( retval );
                }

            case acmpAnimJ:   /* 'J' */
            {
                /* unpack ANIM-J  */
                return( cb->unpackanimjdelta(ah, cb, dlta, dltasize, prevbm, bm ) );
            }

            case acmpAnimI:   /* 'I' */
            {
                /* unpack ANIM-I  */
                return( cb->unpackanimidelta(ah, cb, dlta, dltasize, prevbm, bm ) );
            }
        }

        /* XOR ? */
        DoXOR = ((ah -> ah_Flags) & ahfXOR);

        if( !DoXOR )
        {
            if( (aid -> aid_NoDPaintBrushPatch) == FALSE )
            {
                /* DPaint anim brush (compatibility hack) */
                if( ((ah -> ah_Operation) == acmpByteDelta) && ((ah -> ah_Interleave) == 1U) )
                {
                    DoXOR = TRUE;
                }
            }
        }

        /* Prepare XOR (see below) */
        if( DoXOR && prevbm )
        {
            if( prevbm == bm )
            {
                if( !(tempbm = AllocBitMapPooled( cb, (ULONG)(aid -> aid_BMH -> bmh_Width), (ULONG)(aid -> aid_BMH -> bmh_Height), (ULONG)(aid -> aid_BMH -> bmh_Depth), (aid -> aid_FramePool) )) )
                {
                    return( ERROR_NO_FREE_STORE );
                }

                unpackbm = prevbm = tempbm;
            }

            ClearBitMap( unpackbm );
        }
        else
        {
            if( prevbm )
            {
                if( prevbm != bm )
                {
                    CopyBitMap( cb, prevbm, bm );
                }
            }
            else
            {
                ClearBitMap( bm );
            }
        }

        /* dispatch compression type, second attempt */
        switch( ah -> ah_Operation )
        {
            /* acmpILBM, acmpXORILBM and acmpAnimJ have been processed above */

            case acmpLongDelta:         /* 2 */
                {
                    error = cb->unpacklongdelta(ah, unpackbm, dlta, dltasize );
                }
                break;

            case acmpShortDelta:        /* 3 */
                {
                    error = cb->unpackshortdelta(ah, unpackbm, dlta, dltasize );
                }
                break;

            case acmpDelta:             /*  4 */
                {
                    if( (ah -> ah_Flags) & ahfLongData )
                    {
                        error = cb->unpackanim4longdelta(ah, unpackbm, dlta, dltasize, (ah -> ah_Flags) );
                    }
                    else
                    {
                        error = cb->unpackanim4worddelta(ah, unpackbm, dlta, dltasize, (ah -> ah_Flags) );
                    }
                }
                break;

            case acmpByteDelta:         /* 5 */
            case acmpStereoByteDelta:   /* 6 */
                {
                    error = cb->unpackbytedelta(ah, unpackbm, dlta, dltasize );
                }
                break;

            case acmpAnim7:             /* 7 */
                {
                    if( (ah -> ah_Flags) & ahfLongData )
                    {
                        error = cb->unpackanim7longdelta(ah, unpackbm, dlta, dltasize );
                    }
                    else
                    {
                        error = cb->unpackanim7worddelta(ah, unpackbm, dlta, dltasize );
                    }
                }
                break;

            case acmpAnim8:             /* 8 */
                {
                    if( (ah -> ah_Flags) & ahfLongData )
                    {
                        error = cb->unpackanim8longdelta(ah, unpackbm, dlta, dltasize );
                    }
                    else
                    {
                        error = cb->unpackanim8worddelta(ah, unpackbm, dlta, dltasize );
                    }
                }
                break;

            default:                    /* unknown.. */
                {
                    error_printf( cb, aid, "\adlta: anim compression %ld not implemented yet\n", (long)(ah -> ah_Operation) );
                    error = ERROR_NOT_IMPLEMENTED;
                }
                break;
        }

        /* Handle XOR (see above) */
        if( DoXOR && prevbm )
        {
            cb->xorbm( ah, bm, prevbm );
        }

        if( tempbm )
        {
            FreePooledVec( cb, (aid -> aid_FramePool), tempbm );
        }
    }

    return( error );
}


static
void DumpAnimHeader( struct ClassBase *cb, struct AnimInstData *aid, ULONG ti, struct AnimHeader *anhd )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( anhd )
    {
      verbose_printf( cb, aid, "%4lu: ", ti );

      switch( anhd -> ah_Operation )
      {
        case acmpILBM:              verbose_printf( cb, aid, "Operation ILBM" );                  break;
        case acmpXORILBM:           verbose_printf( cb, aid, "Operation XORILBM" );               break;
        case acmpLongDelta:         verbose_printf( cb, aid, "Operation LongDelta" );             break;
        case acmpShortDelta:        verbose_printf( cb, aid, "Operation ShortDelta" );            break;
        case acmpDelta:             verbose_printf( cb, aid, "Operation Delta" );                 break;
        case acmpByteDelta:         verbose_printf( cb, aid, "Operation ByteDelta" );             break;
        case acmpStereoByteDelta:   verbose_printf( cb, aid, "Operation StereoByteDelta" );       break;
        case acmpAnim7:             verbose_printf( cb, aid, "Operation Anim7" );                 break;
        case acmpAnim8:             verbose_printf( cb, aid, "Operation Anim8" );                 break;
        case acmpAnimI:             verbose_printf( cb, aid, "Operation AnimI" );                 break;
        case acmpAnimJ:             verbose_printf( cb, aid, "Operation AnimJ" );                 break;
        default:                    verbose_printf( cb, aid, "Operation %02x <unknown compression>", anhd->ah_Operation); break;
      }

      verbose_printf( cb, aid, " AbsTime %3lu RelTime %3lu Interleave %3lu", (anhd -> ah_AbsTime), (anhd -> ah_RelTime), (ULONG)(anhd -> ah_Interleave) );

      if( (anhd -> ah_Flags) & ahfLongData          ) verbose_printf( cb, aid, " LongData"          );
      if( (anhd -> ah_Flags) & ahfXOR               ) verbose_printf( cb, aid, " XOR"               );
      if( (anhd -> ah_Flags) & ahfOneInfoList       ) verbose_printf( cb, aid, " OneInfoList"       );
      if( (anhd -> ah_Flags) & ahfRLC               ) verbose_printf( cb, aid, " RLC"               );
      if( (anhd -> ah_Flags) & ahfVertical          ) verbose_printf( cb, aid, " Vertical"          );
      if( (anhd -> ah_Flags) & ahfLongInfoOffsets   ) verbose_printf( cb, aid, " LongInfoOffsets"   );

      verbose_printf( cb, aid, "\n" );

      if( (aid -> aid_NoDPaintBrushPatch) == FALSE )
      {
        /* DPaint anim brush (compatibility hack) */
        if( (anhd -> ah_Operation == acmpByteDelta) && ((anhd -> ah_Interleave) == 1U) && (!((anhd -> ah_Flags) & ahfXOR)) )
        {
          verbose_printf( cb, aid, "Assuming XOR mode for this frame (DPaint brush patch, see docs)\n" );
        }
      }
    }
}


static
struct FrameNode *GetPrevFrameNode( struct FrameNode *currfn, ULONG interleave )
{
    struct FrameNode *worknode,
                     *prevnode;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Get previous frame */
    worknode = currfn;

    while ((prevnode = (struct FrameNode *)(worknode -> fn_Node . mln_Pred) ) != NULL)
    {
      if( (interleave-- == 0U) || ((prevnode -> fn_Node . mln_Pred) == NULL) )
      {
        break;
      }

      worknode = prevnode;
    }

    return( worknode );
}

void OpenLogfile( struct ClassBase *cb, struct AnimInstData *aid )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( (aid -> aid_VerboseOutput) == BNULL )
    {
      STRPTR confile;

      if ((confile = (STRPTR)AllocVec( (((aid -> aid_ProjectName)?(strlen( (aid -> aid_ProjectName) )):(0UL)) + 100UL), MEMF_PUBLIC ) ) != NULL)
      {
        mysprintf( cb, confile, "CON:////Anim DataType %s/auto/wait/close/inactive",
                   ((aid -> aid_ProjectName)?(FilePart( (aid -> aid_ProjectName) )):(NULL)) );

        aid -> aid_VerboseOutput = Open( confile, MODE_READWRITE );

        FreeVec( confile );
      }
    }
}

#if !defined(__AROS__)
void mysprintf( struct ClassBase *cb, STRPTR buffer, STRPTR fmt, ... )
{
    APTR args;

    args = (APTR)((&fmt) + 1);

    RawDoFmt( fmt, args, (void (*))"\x16\xc0\x4e\x75", buffer );
}


void error_printf( struct ClassBase *cb, struct AnimInstData *aid, STRPTR format, ... )
{
    va_list args;

    OpenLogfile( cb, aid );

    if (aid -> aid_VerboseOutput)
    {
        va_start (args, format);
        VFPrintf( (aid -> aid_VerboseOutput), format, (const IPTR *)args);
        va_end (args);
    }
}


void verbose_printf( struct ClassBase *cb, struct AnimInstData *aid, STRPTR format, ... )
{
    va_list args;

    if (aid -> aid_VerboseOutput)
    {
        va_start (args, format);
        VFPrintf( (aid -> aid_VerboseOutput), format, (const IPTR *)args);
        va_end (args);
    }
    D(
        else
        {
            vkprintf(format, args);
        }
    )
}
#endif

static
void AttachSample( struct ClassBase *cb, struct AnimInstData *aid )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( aid -> aid_Sample )
    {
      struct FrameNode *worknode,
                       *nextnode;

      ULONG             period          = aid -> aid_Period;
      ULONG             samplesperframe;
      BYTE             *sample          = aid -> aid_Sample;

      samplesperframe = (((SysBase -> ex_EClockFrequency) * 10UL) / (period * (aid -> aid_FPS) * 2UL));

      if( aid -> aid_SamplesPerFrame )
      {
        period = (period * samplesperframe) / (aid -> aid_SamplesPerFrame);

        samplesperframe = aid -> aid_SamplesPerFrame;

        verbose_printf( cb, aid, "period corrected from %lu to %lu to match spf=%lu with fps=%lu\n",
                        (aid -> aid_Period), period, samplesperframe, (aid -> aid_FPS) );
      }

      verbose_printf( cb, aid, "Attching samples (sysclock %lu period %lu fps %lu length %lu samplesperframe %lu)...\n",
                      (SysBase -> ex_EClockFrequency), period, (aid -> aid_FPS), (aid -> aid_SampleLength), samplesperframe );

      worknode = (struct FrameNode *)(aid -> aid_FrameList . mlh_Head);

      while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
      {
        worknode -> fn_Sample       = sample;
        worknode -> fn_SampleLength = samplesperframe * ((worknode -> fn_Duration) + 1UL);
        worknode -> fn_Period       = period;

        sample += (samplesperframe * ((worknode -> fn_Duration) + 1UL));

        /* End of sample reached ? */
        if( (ULONG)(sample - (aid -> aid_Sample)) > (aid -> aid_SampleLength) )
        {
          /* Cut last size of sample to fit */
          worknode -> fn_SampleLength -= (ULONG)(sample - (aid -> aid_Sample));

          break;
        }

        worknode = nextnode;
      }
    }
}


ULONG SaveIFFAnim( struct ClassBase *cb, struct IClass *cl, Object *o, struct dtWrite *dtw )
{
    ULONG retval = 0UL;
    LONG  error  = 0L;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* A NULL file handle is a nop (GMultiView uses this to test if a datatype supports RAW writing) */
    if( dtw -> dtw_FileHandle )
    {
      struct AnimInstData *aid = (struct AnimInstData *)INST_DATA( cl, o );

      struct BitMapHeader *bmh = NULL;
      IPTR                modeid = 0;
      ULONG               *cregs = NULL;
      IPTR                numcolors = 0;
      IPTR                startframe = 0UL,
                           numframes  = 0UL,
                           framestep  = 1UL;
      IPTR                fps = 0UL;
      struct BitMap       *keyframe = NULL;
      IPTR                animwidth = 0,
                           animheight = 0,
                           animdepth = 0;

      if( GetDTAttrs( o, ADTA_BitMapHeader,     (&bmh),
                         ADTA_ModeID,           (&modeid),
                         ADTA_CRegs,            (&cregs),
                         ADTA_NumColors,        (&numcolors),
                         ADTA_Width,            (&animwidth),
                         ADTA_Height,           (&animheight),
                         ADTA_Depth,            (&animdepth),
                         ADTA_Frame,            (&startframe),
                         ADTA_Frames,           (&numframes),
                         ADTA_FramesPerSecond,  (&fps),
                         ADTA_KeyFrame,         (&keyframe),
                         TAG_DONE ) == 11UL )
      {
        struct TagItem     *tstate,
                           *ti;
        struct AnimContext *ac;
        struct IFFHandle   *iff;
        struct BitMapHeader xbmh;
        BOOL                planar       = MAKEBOOL( GetBitMapAttr( keyframe, BMA_FLAGS ) & BMF_STANDARD );
        BOOL                interleaved  = MAKEBOOL( GetBitMapAttr( keyframe, BMA_FLAGS ) & BMF_INTERLEAVED );

#if !defined(__AROS__)
        xbmh   = *bmh;
#else
        CopyMem(bmh, &xbmh, sizeof(struct BitMapHeader));
#endif

        if( planar && (!interleaved) )
        {
          xbmh . bmh_Compression = cmpNone; /* Currently we only write an uncompressed key frame */

          numframes -= startframe;

          tstate = dtw -> dtw_AttrList;

          while ((ti = NextTagItem( (&tstate) ) ) != NULL)
          {
            switch( ti -> ti_Tag )
            {
              case ADTA_Frame:            startframe = ti -> ti_Data; break;
              case ADTA_Frames:           numframes  = ti -> ti_Data; break;
              case ADTA_FrameIncrement:   framestep  = ti -> ti_Data; break;
            }
          }

          if( framestep == 0UL ) framestep = 1UL;

          verbose_printf( cb, aid, "saving iff anim %lu %lu %lu\n", startframe, numframes, framestep );

          if( numframes )
          {
            if ((ac = CreateAnimContext( cb, animwidth, animheight, animdepth ) ) != NULL)
            {
              if ((iff = CreateDOSIFFHandle( cb, (dtw -> dtw_FileHandle) ) ) != NULL)
              {
                if( !(error = StartIFFAnim3( cb, aid, iff, ac, (&xbmh), modeid, cregs, numcolors, numframes, fps, keyframe )) )
                {
                  struct adtFrame  alf;
                  ULONG            timestamp,
                                  *cmap_cregs = NULL;

                  /* Start scan through animation */
                  for( timestamp = startframe ; numframes > 0UL ; timestamp += framestep, numframes-- )
                  {
                    /* On error break */
                    if( error )
                    {
                      break;
                    }

                    /* Check for CTRL_D signal... */
                    if( SetSignal( 0UL, 0UL ) & SIGBREAKF_CTRL_D )
                    {
                      error = ERROR_BREAK;

                      break;
                    }

                    /* reset method msg */
                    memset( (void *)(&alf), 0, sizeof( struct adtFrame ) );

                    /* load frame */
                    alf . MethodID = ADTM_LOADFRAME;
                    alf . alf_TimeStamp = timestamp;
                    alf . alf_Frame     = timestamp;

                    if( DoMethodA( o, (Msg)(&alf) ) == 0UL )
                    {
                      error = IoErr();

                      if( error != ERROR_OBJECT_NOT_FOUND )
                      {
                        break;
                      }
                    }
                    else
                    {
                      /* print frame contents */
                      verbose_printf( cb, aid, "frame: timestamp %lu frame %lu duration %lu bitmap %lx cmap %lx sample %lx len %lu period %lu\n",
                              timestamp,
                              (alf . alf_Frame),
                              (alf . alf_Duration),
                              (alf . alf_BitMap),
                              (alf . alf_CMap),
                              (alf . alf_Sample),
                              (alf . alf_SampleLength),
                              (alf . alf_Period) );

                      if( alf . alf_CMap )
                      {
                        if ((cmap_cregs = (ULONG *)AllocVec( (((sizeof( ULONG ) * 3UL) + 1UL) * numcolors), MEMF_PUBLIC ) ) != NULL)
                        {
                          GetRGB32( (alf . alf_CMap), 0UL, numcolors, cmap_cregs );
                        }
                        else
                        {
                          error_printf( cb, aid, "can't alloc dynamic palette buffer for %d colors\n", numcolors);
                          error = ERROR_NO_FREE_STORE;
                        }
                      }

                      if( alf . alf_BitMap )
                      {
                        if ((error = WriteIFFAnim3( cb, iff, ac, ((timestamp * 60UL) / fps), (60UL / fps), (&xbmh), cmap_cregs, numcolors, (alf . alf_BitMap) ) ) != 0)
                        {
                          error_printf( cb, aid, "error %08x while writing IFF ANIM-3, aborted\n", error);
                        }
                      }

                      if( cmap_cregs )
                      {
                        FreeVec( cmap_cregs );
                        cmap_cregs = NULL;
                      }
                    }

                    alf . MethodID = ADTM_UNLOADFRAME;
                    DoMethodA( o, (Msg)(&alf) );
                  }

                  EndIFFAnim3( cb, aid, iff );

                  if( error == 0L )
                  {
                    retval = 1UL; /* success ! */
                  }
                }

                FreeIFF( iff );
              }
              else
              {
                error = IoErr();
              }

              DeleteAnimContext( cb, ac );
            }
            else
            {
              /* Can't alloc animcontext */
              error = ERROR_NO_FREE_STORE;
            }
          }
        }  
        else
        {
          /* Does not support non-planar or planar interleaved bitmaps */
          error = ERROR_NOT_IMPLEMENTED;
        }
      }
      else
      {
        error_printf( cb, aid, "failed to get %d dt attributes\n", 11);
      }
    }

    /* Error codes below 0 are related to the IFFParse.library functions */
    if( error < 0L )
    {
      /* convert IFFParse error to DOS error */
      error = ifferr2doserr[ (-error - 1) ];
    }

    SetIoErr( error );

    return( retval );
}


static
struct IFFHandle *CreateDOSIFFHandle( struct ClassBase *cb, BPTR fh )
{
    struct IFFHandle *iff;

    D(bug("[anim.datatype] %s()\n", __func__));

    if ((iff = AllocIFF() ) != NULL)
    {
      iff -> iff_Stream = (IPTR)fh;

      InitIFFasDOS( iff );
    }

    return( iff );
}


static
LONG StartIFFAnim3( struct ClassBase *cb, struct AnimInstData *aid, struct IFFHandle *iff, struct AnimContext *ac, struct BitMapHeader *bmh, ULONG modeid, ULONG *cregs, ULONG numcolors, ULONG numframes, ULONG fps, struct BitMap *bm )
{
    LONG error;
    
    D(bug("[anim.datatype] %s()\n", __func__));

    if( !(error = OpenIFF( iff, IFFF_WRITE )) )
    {
      for( ;; ) /* not a loop, used as a jump table */
      {
        if ((error = PushChunk( iff, ID_ANIM, ID_FORM, IFFSIZE_UNKNOWN ) ) != 0)
          break;

        /* write initial FORM ILBM */
        {
          if ((error = PushChunk( iff, ID_ILBM, ID_FORM, IFFSIZE_UNKNOWN ) ) != 0)
            break;

          /* write ILBM BMHD (BitMapHeader) */
          {
            if ((error = PushChunk( iff, 0UL, ID_BMHD, IFFSIZE_UNKNOWN ) ) != 0)
              break;

            if( WriteChunkBytes( iff, (APTR)bmh, sizeof( struct BitMapHeader ) ) != sizeof( struct BitMapHeader ) )
            {
              error = IFFERR_WRITE;
              break;
            }

            if ((error = PopChunk( iff ) ) != 0)
              break;
          }

          /* write ILBM CMAP (global color map) */
          if ((error = PutILBMCMAP( cb, iff, cregs, numcolors ) ) != 0)
            break;

          /* write ILBM DPAN chunk */
          {
            struct DPAnimChunk dpan;

            dpan . dpan_Version = 0U; /* ??? */
            dpan . dpan_nframes = numframes;
            dpan . dpan_FPS     = fps;

            if ((error = PushChunk( iff, 0UL, ID_DPAN, IFFSIZE_UNKNOWN ) ) != 0)
              break;

            if( WriteChunkBytes( iff, (APTR)(&dpan), sizeof( struct DPAnimChunk ) ) != sizeof( struct DPAnimChunk ) )
            {
              error = IFFERR_WRITE;
              break;
            }

            if ((error = PopChunk( iff ) ) != 0)
              break;
          }

          /* write ILBM CAMG (amiga view mode) */
          {
            if ((error = PushChunk( iff, 0UL, ID_CAMG, IFFSIZE_UNKNOWN ) ) != 0)
              break;

            if( WriteChunkBytes( iff, (APTR)(&modeid), sizeof( ULONG ) ) != sizeof( ULONG ) )
            {
              error = IFFERR_WRITE;
              break;
            }

            if ((error = PopChunk( iff ) ) != 0)
              break;
          }

          /* Write ILBM BODY (if there is one...) */
          if( bm )
          {
            if ((error = PutILBMBody( cb, iff, bm, bmh ) ) != 0)
              break;

            /* Copy current bitmap into both buffers */
            CopyBitMap( cb, bm, CurrFrame( cb, ac ) );
            SwapFrames( cb, ac );
            CopyBitMap( cb, bm, CurrFrame( cb, ac ) );
          }

          if ((error = PopChunk( iff ) ) != 0)
            break;
        }

        break; /* end of jump table */
      }

      /* All headers written successfully ? */
      if( error == 0L )
      {
        /* Success ! */
        return( 0L );
      }

      CloseIFF( iff );
    }

    return( error );
}


static
void EndIFFAnim3( struct ClassBase *cb, struct AnimInstData *aid, struct IFFHandle *iff )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( iff )
    {
      LONG error;

      if ((error = PopChunk( iff ) ) != 0)
      {
        error_printf( cb, aid, "error while popping IFF ANIM-3 FORM %ld\n", error );
      }

      CloseIFF( iff );

      verbose_printf( cb, aid, "IFF ANIM-3 sucessfully created\n" );
    }
}


static
LONG WriteIFFAnim3( struct ClassBase *cb, struct IFFHandle *iff, struct AnimContext *ac, ULONG abstime, ULONG reltime, struct BitMapHeader *bmh, ULONG *cregs, ULONG numcolors, struct BitMap *bm )
{
    LONG error = 0L;

    D(bug("[anim.datatype] %s()\n", __func__));

    for( ;; ) /* not a loop, used as a jump-table */
    {
      /* write FORM ILBM */
      {
        if ((error = PushChunk( iff, ID_ILBM, ID_FORM, IFFSIZE_UNKNOWN ) ) != 0)
          break;

        /* write ILBM ANHD (AnimHeader) */
        {
          struct AnimHeader animheader = { 0 };

          animheader . ah_Operation  = acmpShortDelta;
          animheader . ah_AbsTime    = abstime;
          animheader . ah_RelTime    = reltime;
          animheader . ah_Interleave = 2U;

          if ((error = PushChunk( iff, 0UL, ID_ANHD, IFFSIZE_UNKNOWN ) ) != 0)
            break;

          if( WriteChunkBytes( iff, (APTR)(&animheader), sizeof( struct AnimHeader ) ) != sizeof( struct AnimHeader ) )
          {
            error = IFFERR_WRITE;
            break;
          }

          if ((error = PopChunk( iff ) ) != 0)
            break;
        }

        /* Palette change ? */
        if( cregs && numcolors )
        {
          if ((error = PutILBMCMAP( cb, iff, cregs, numcolors ) ) != 0)
            break;
        }

        /* Write ANIM-3 delta */
        if ((error = PutAnim3Delta( cb, iff, ac, CurrFrame( cb, ac ), bm ) ) != 0)
          break;

        /* Copy current bitmap into buffer, then swap to next frame */
        CopyBitMap( cb, bm, CurrFrame( cb, ac ) );
        SwapFrames( cb, ac );

        if ((error = PopChunk( iff ) ) != 0)
          break;
      }

      break; /* end of jump-table */
    }

    return( error );
}


static
LONG PutAnim3Delta( struct ClassBase *cb, struct IFFHandle *iff, struct AnimContext *ac, struct BitMap *prevbm, struct BitMap *bm )
{
    LONG    error;
    ULONG   len;

    IPTR  *ptrs = (IPTR *)(ac -> ac_WorkBuffer);
    WORD   *data = (WORD *)(ptrs + 8UL);  /* data space starts after the 8 byte pointers... */
    UBYTE   plane;
    ULONG   planesize = ((bm -> BytesPerRow) * (bm -> Rows)) / sizeof( WORD ); /* size of plane,
                                                                                * in compression units (WORD).
                                                                                */

    D(bug("[anim.datatype] %s()\n", __func__));

    memset( (ac -> ac_WorkBuffer), 0, (size_t)(ac -> ac_WorkBufferSize) );

    for( plane = 0U ; plane < (bm -> Depth) ; plane++ )
    {
      WORD   distance = 0,
             i;
      WORD  *planedata     = (WORD *)(bm -> Planes[ plane ]),
            *prevplanedata = (WORD *)(prevbm -> Planes[ plane ]);

      ptrs[ plane ] = (IPTR)((UBYTE *)data - (UBYTE *)(ac -> ac_WorkBuffer));

      for( i = 0 ; i < planesize ; i++ )
      {
        if( (planedata[ i ] != prevplanedata[ i ]) || (distance > 30000) )
        {
          if( FALSE /*planedata[ (i + 1) ] != prevplanedata[ (i + 1) ]*/ )
          {
            WORD *len;

            *data++ = -distance - 1;
            len = data++;

            *len = 0;

            while( (planedata[ i ] != prevplanedata[ i ]) && (i < planesize) && (*len < 30000))
            {
              *data++ = planedata[ i ];
              i++;
              (*len)++;
            }

            i--;
          }
          else
          {
            *data++ = distance;
            *data++ = planedata[ i ];
          }

          distance = 1;
        }
        else
        {
          distance++;
        }
      }

      /* No changes ? */
      if( ptrs[ plane ] == (IPTR)((UBYTE *)data - (UBYTE *)(ac -> ac_WorkBuffer)) )
      {
        ptrs[ plane ] = 0UL;
      }
      else
      {
        *data++ = (WORD)0xFFFF; /* terminator */
      }
    }

    if ((error = PushChunk( iff, 0UL, ID_DLTA, IFFSIZE_UNKNOWN ) ) != 0)
      return( error );

    len = (ULONG)((UBYTE *)data - (UBYTE *)(ac -> ac_WorkBuffer));

    if( WriteChunkBytes( iff, (APTR)(ac -> ac_WorkBuffer), len ) != len )
    {
      return( IFFERR_WRITE );
    }

    error = PopChunk( iff );

    return( error );
}


/*****************************************************************************/

/* write ILBM CMAP  */
static
LONG PutILBMCMAP( struct ClassBase *cb, struct IFFHandle *iff, ULONG *cregs, ULONG numcolors )
{
    long                 error;
    ULONG                i;
    struct ColorRegister cm;

    D(bug("[anim.datatype] %s()\n", __func__));

    if ((error = PushChunk( iff, 0UL, ID_CMAP, IFFSIZE_UNKNOWN ) ) != 0)
      return( error );

    for( i = 0UL ; i < numcolors ; i++ )
    {
      /* reduce colors from 32 bits to cmap's 8 bit-per-gun */
      cm . red   = (UBYTE)(cregs[ ((i * 3) + 0) ] >> 24UL);
      cm . green = (UBYTE)(cregs[ ((i * 3) + 1) ] >> 24UL);
      cm . blue  = (UBYTE)(cregs[ ((i * 3) + 2) ] >> 24UL);

      /* Write R, B, G bytes */
      if( WriteChunkBytes( iff, (APTR)(&cm), 3UL ) != 3UL )
      {
        return( IFFERR_WRITE );
      }
    }

    return( PopChunk( iff ) );
}


/*****************************************************************************/

/* from IFF example code ("iffp/ilbm.h") */
#define RowBytes( w )     ((((w) + 15) >> 4) << 1)
#define RowBits( w )      ((((w) + 15) >> 4) << 4)


static
LONG PutILBMBody( struct ClassBase *cb, struct IFFHandle *iff, struct BitMap *bitmap, struct BitMapHeader *bmh )
{
    LONG     error;
    LONG     rowBytes        = bitmap -> BytesPerRow;           /* for source modulo only  */
    LONG     FileRowBytes    = RowBytes( (bmh -> bmh_Width) );  /* width to write in bytes */
    ULONG    planeCnt        = bmh -> bmh_Depth;                /* number of bit planes including mask */
    ULONG    iPlane,
             iRow;
    BYTE    *planes[ 24 ]; /* array of ptrs to planes & mask */

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Copy the ptrs to bit & mask planes into local array "planes" */
    for( iPlane = 0 ; iPlane < planeCnt; iPlane++ )
       planes[ iPlane ] = (BYTE *)(bitmap -> Planes[ iPlane ]);

    /* Write out a BODY chunk header */
    if ((error = PushChunk( iff, 0L, ID_BODY, IFFSIZE_UNKNOWN ) ) != 0)
      return( error );

    /* Write out the BODY contents */
    for( iRow = bmh -> bmh_Height ; iRow > 0 ; iRow-- )
    {
      for( iPlane = 0 ; iPlane < planeCnt ; iPlane++ )
      {
        /* Write next row.*/
        if( WriteChunkBytes( iff, planes[ iPlane ], FileRowBytes ) != FileRowBytes )
          return( IFFERR_WRITE );

        planes[ iPlane ] += rowBytes; /* Possibly skipping unused bytes */
      }
    }

    /* Finish the chunk */
    error = PopChunk( iff );

    return( error );
}


static
struct AnimContext *CreateAnimContext( struct ClassBase *cb, ULONG width, ULONG height, ULONG depth )
{
    APTR pool;

    D(bug("[anim.datatype] %s()\n", __func__));

    if ((pool = CreatePool( (MEMF_PUBLIC | MEMF_CLEAR), 1024UL, 1024UL ) ) != (APTR)NULL)
    {
      struct AnimContext *ac;

      if ((ac = (struct AnimContext *)AllocPooledVec( cb, pool, sizeof( struct AnimContext ) ) ) != NULL)
      {
        /* Alloc two bitmaps, used for interleaving */
        ac -> ac_BitMap[ 0 ] = AllocBitMapPooled( cb, width, height, depth, pool );
        ac -> ac_BitMap[ 1 ] = AllocBitMapPooled( cb, width, height, depth, pool );

        /* Alloc work buffer large enougth to hold fancy things... */

        ac -> ac_WorkBufferSize = (width * height * depth * 2UL) + 1024UL;
        ac -> ac_WorkBuffer     = (UBYTE *)AllocPooledVec( cb, pool, (ac -> ac_WorkBufferSize) );

        if( (ac -> ac_BitMap[ 0 ]) && (ac -> ac_BitMap[ 1 ]) && (ac -> ac_WorkBuffer) )
        {
          return( ac );
        }
      }

      /* Something goes wrong here, get rid of the resources allocated */
      DeletePool( pool );
    }

    return( NULL );
}


#if 0
static
struct BitMap *PrevFrame( struct ClassBase *cb, struct AnimContext *ac )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( ac )
    {
      return( (ac -> ac_BitMap[ ((ac -> ac_WhichBitMap) ^ 1) ]) );
    }

    return( NULL );
}
#endif


static
void SwapFrames( struct ClassBase *cb, struct AnimContext *ac )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( ac )
    {
      ac -> ac_WhichBitMap ^= 1; /* Toggle buffer */
    }
}


static
struct BitMap *CurrFrame( struct ClassBase *cb, struct AnimContext *ac )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( ac )
    {
      return( (ac -> ac_BitMap[ (ac -> ac_WhichBitMap) ]) );
    }

    return( NULL );
}


static
void DeleteAnimContext( struct ClassBase *cb, struct AnimContext *ac )
{
    D(bug("[anim.datatype] %s()\n", __func__));

    if( ac )
    {
      DeletePool( (ac -> ac_Pool) );
    }
}



