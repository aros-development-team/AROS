
/*
**
**  $VER: prefs.c 2.3 (24.5.98)
**  gifanim.datatype 2.3
**
**  Preferences
**
**  Written 1997/1998 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

struct MyStackSwapStruct;
struct GIFAnimInstData;
struct GIFEncoder;

/* main includes */
#include "classbase.h"
#include "classdata.h"
#include "encoder.h"

/* ansi includes */
#include <limits.h>
#include <stdio.h>

/*****************************************************************************/

/* local prototypes */
static STRPTR GetPrefsVar( struct ClassBase *, STRPTR );
static BOOL   matchstr( struct ClassBase *, STRPTR, STRPTR );

/*****************************************************************************/

/****** gifanim.datatype/preferences *****************************************
*
*   NAME
*       preferences
*
*   DESCRIPTION
*       The "ENV:Classes/DataTypes/gifanim.prefs" file contains global
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
*       MATCHPROJECT/K,VERBOSE/S,NOVERBOSE/S,STRICTSYNTAX/S,NOSTRICTSYNTAX/S,
*       MODEID/K/N,16BITCHUNKY=24BITCHUNKY=TRUECOLOR/S,
*       NO16BITCHUNKY=NO24BITCHUNKY=NOTRUECOLOR/S,FPS/K/N,
*       SAMPLE/K,SAMPLESPERFRAME=SPF/K/N,VOLUME/K/N,LOADALL/S,
*       NOLOADALL/S,ENC_INTERLACE/S,ENC_NO_INTERLACE/S,
*       ENC_BACKGROUNDPEN=ENC_BG/K/N,ENC_TRANSPARENTPEN=ENC_TRANSPARENT/K/N
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
*       NOVERBOSE -- Turns verbose output and error messages OFF.
*          Be carefull, you won't see any error messages any more !!!
*
*       STRICTSYNTAX -- Prompt syntax errors in the gif streams.
*
*       NOSTRICTSYNTAX -- Turns STRICTSYNTAX off
*
*       MODEID -- Select screen mode id of datatype (will be stored in
*           ADTA_ModeID). Note that the DOS ReadArgs function used for parsing
*           fetches a SIGNED long. The bit 31 will be represented by minus
*           '-'. (example: "MODEID=266240" sets the mode to the A2024 screen
*           mode id)
*           Defaults to -1, which means: Use the best screenmode available for
*           the given width, height and depth.
*
*       16BITCHUNKY
*       24BITCHUNKY
*       TRUECOLOR -- Create 24 bit chunky bitmaps, if possible.
*           Note that the 16BITCHUNKY and the 24BITCHUNKY options will be
*           seperated in the future. The TRUECOLOR option selects the
*           best truecolor depth in this case...
*
*       NO16BITCHUNKY
*       NO24BITCHUNKY
*       NOTRUECOLOR -- Turns 24BITCHUNKY option off. (Default)
*           Note that the 16BITCHUNKY and the 24BITCHUNKY options will be
*           seperated in the future. The TRUECOLOR option selects the
*           best truecolor depth in this case...
*
*       FPS -- Frames Per Second
*           A value of 0 here means: Use default FPS.
*
*       SAMPLE -- Attach the given sample to the animation. The sample will
*           be loaded using datatypes (GID_SOUND).
*           Only one sample can be attached to one animation stream, any
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
*       LOADALL -- Load all frames into memory.
*
*       NOLOADALL -- Turns off the LOADALL flag, which may be set in a prefs-
*           line before. This switch is set per default, and can be turned off
*           by the LOADALL option, later it can be turned on again by this
*           option.
*
*       Encoder related options:
*       ENC_INTERLACE    - create interlaced gif animation
*
*       ENC_NO_INTERLACE - create non-interlaced gif animation 
*           (set per default).
*
*       ENC_BACKGROUNDPEN
*       ENC_BG           - background pen number
*           Defaults to 0 (e.g. default bg pen)
*
*       ENC_TRANSPARENTPEN
*       ENC_TRANSPARENT - transparent pen number
*           Defaults to -1 (means: no transparent pen).
*
*
*   NOTE
*       - An invalid prefs file line will be ignored and forces the VERBOSE
*         output.
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

void ReadENVPrefs( struct ClassBase *cb, struct GIFAnimInstData *gaid, struct GIFEncoder *genc )
{
    struct RDArgs envvarrda =
    {
        { NULL, 256L, 0L},
        0L,
        NULL,
        0L,
        NULL,
        RDAF_NOPROMPT
    };

    struct
    {
      STRPTR  matchproject;
      IPTR    verbose;
      IPTR    noverbose;
      IPTR    strictsyntax;
      IPTR    nostrictsyntax;
      IPTR    *modeid;
      IPTR    use24bitchunky;
      IPTR    nouse24bitchunky;
      IPTR    *fps;
      STRPTR  sample;
      IPTR    *samplesperframe;
      IPTR    *volume;
      IPTR    loadall;
      IPTR    noloadall;
      IPTR    enc_interlace;
      IPTR    enc_no_interlace;
      IPTR    *enc_backgroundpen;
      IPTR    *enc_transparentpen;
    } gifanimargs;

    TEXT   varbuff[ 258 ];
    STRPTR var;

    if ((var = GetPrefsVar( cb, "Classes/DataTypes/gifanim.prefs" ) ) != NULL)
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
          memset( (void *)(&gifanimargs), 0, sizeof( gifanimargs ) );          /* Clear result array          */

          if( ReadArgs( "MATCHPROJECT/K,"
                        "VERBOSE/S,"
                        "NOVERBOSE/S,"
                        "STRICTSYNTAX/S,"
                        "NOSTRICTSYNTAX/S,"
                        "MODEID/K/N,"
                        "16BITCHUNKY=24BITCHUNKY=TRUECOLOR/S,"
                        "NO16BITCHUNKY=NO24BITCHUNKY=NOTRUECOLOR/S,"
                        "FPS/K/N,"
                        "SAMPLE/K,"
                        "SAMPLESPERFRAME=SPF/K/N,"
                        "VOLUME/K/N,"
                        "LOADALL/S,"
                        "NOLOADALL/S,"
                        "ENC_INTERLACE/S,"
                        "ENC_NO_INTERLACE/S,"
                        "ENC_BACKGROUNDPEN=ENC_BG/K/N,"
                        "ENC_TRANSPARENTPEN=ENC_TRANSPARENT/K/N", (IPTR *)(&gifanimargs), (&envvarrda) ) )
          {
            BOOL noignore = TRUE;

            if( (gifanimargs . matchproject) && (gaid -> gaid_ProjectName) )
            {
              noignore = matchstr( cb, (gifanimargs . matchproject), (gaid -> gaid_ProjectName) );
            }

            if( noignore )
            {
              /* Read encoder prefs ? */
              if( genc )
              {
                if( gifanimargs . enc_interlace )
                {
                  genc -> interlace = TRUE;
                }

                if( gifanimargs . enc_no_interlace )
                {
                  genc -> interlace = FALSE;
                }

                if( gifanimargs . enc_backgroundpen )
                {
                  genc -> backgroundpen = (WORD)(*(gifanimargs . enc_backgroundpen));
                }

                if( gifanimargs . enc_transparentpen )
                {
                  genc -> transparentpen = (WORD)(*(gifanimargs . enc_transparentpen));
                }
              }
              else
              {
                if( gifanimargs . verbose )
                {
                  OpenLogfile( cb, gaid );
                }

                if( gifanimargs . noverbose )
                {
                  if( (gaid -> gaid_VerboseOutput) && ((gaid -> gaid_VerboseOutput) != (BPTR)-1L) )
                  {
                    Close( (gaid -> gaid_VerboseOutput) );
                  }

                  gaid -> gaid_VerboseOutput = (BPTR)-1L;
                }

                if( gifanimargs . strictsyntax )
                {
                  gaid -> gaid_StrictSyntax = TRUE;
                }

                if( gifanimargs . nostrictsyntax )
                {
                  gaid -> gaid_StrictSyntax = FALSE;
                }

                if( gifanimargs . modeid )
                {
                  gaid -> gaid_ModeID = *(gifanimargs . modeid);
                }

                if( gifanimargs . use24bitchunky )
                {
#if !defined(__AROS__)
                  /* Check if we have animation.datatype V41 (or higher) as superclass */
                  if( (cb -> cb_SuperClassBase -> lib_Version) >= 41U )
                  {
                    /* Check here if we opened the cybergraphics.library. After this point, I'll assume
                     * that (gaid_UseChunkyMap == TRUE) implies a opened CyberGfxBase !!
                     */
                    if( CyberGfxBase )
                    {
#endif
                      gaid -> gaid_UseChunkyMap = TRUE;
#if !defined(__AROS__)
                    }
                    else
                    {
                      error_printf( cb, gaid, "no cybergraphics.library available, can't output a 24 bit chunky map\n" );
                    }
                  }
                  else
                  {
                    error_printf( cb, gaid, "Requires at least animation.datatype V41 for non-planar bitmap support\n" );
                  }
#endif
                }
                if( gifanimargs . nouse24bitchunky )
                {
                  gaid -> gaid_UseChunkyMap = FALSE;
                }

                if( gifanimargs . fps )
                {
                  gaid -> gaid_FPS = *(gifanimargs . fps);
                }

                if( gifanimargs . loadall )
                {
                  gaid -> gaid_LoadAll = TRUE;
                }

                if( gifanimargs . noloadall )
                {
                  gaid -> gaid_LoadAll = FALSE;
                }

                if( (gifanimargs . sample) && ((gaid -> gaid_Sample) == NULL) )
                {
                  Object *so;
                  LONG    ioerr = 0L;

                  verbose_printf( cb, gaid, "loading sample \"%s\"...\n", (gifanimargs . sample) );

                  if ((so = NewDTObject( (gifanimargs . sample), DTA_GroupID, GID_SOUND, TAG_DONE ) ) != NULL)
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
                      if ((gaid -> gaid_Sample = (STRPTR)AllocPooled( (gaid -> gaid_Pool), (length + 1UL) ) ) != NULL)
                      {
                        /* Copy sample and context */
                        CopyMem( (APTR)sample, (APTR)(gaid -> gaid_Sample), length );
                        gaid -> gaid_SampleLength = length;
                        gaid -> gaid_Period       = period;
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

                  if( (gaid -> gaid_Sample) == NULL )
                  {
                    TEXT errbuff[ 256 ];

                    if( ioerr >= DTERROR_UNKNOWN_DATATYPE )
                    {
                      mysprintf( cb, errbuff, GetDTString( ioerr ), (gifanimargs . sample) );
                    }
                    else
                    {
                      Fault( ioerr, (gifanimargs . sample), errbuff, sizeof( errbuff ) );
                    }

                    error_printf( cb, gaid, "can't load sample: \"%s\" line %lu\n", errbuff, linecount );
                  }
                }

                if( gifanimargs . samplesperframe )
                {
                  gaid -> gaid_SamplesPerFrame = *(gifanimargs . samplesperframe);
                }

                if( gifanimargs . volume )
                {
                  gaid -> gaid_Volume = *(gifanimargs . volume);

                  if( (gaid -> gaid_Volume) > 64UL )
                  {
                    gaid -> gaid_Volume = 64UL;
                  }
                }
              }
            }
            else
            {
              verbose_printf( cb, gaid, "prefs line %lu ignored\n", linecount );
            }

            FreeArgs( (&envvarrda) );
          }
          else
          {
            LONG ioerr = IoErr();
            TEXT errbuff[ 256 ];

            Fault( ioerr, "Classes/DataTypes/gifanim.prefs", errbuff, (LONG)sizeof( errbuff ) );

            error_printf( cb, gaid, "preferences \"%s\" line %lu\n", errbuff, linecount );
          }
        }

        prefsline = ++nextprefsline;
        linecount++;
      }

      FreeVec( var );
    }
}




