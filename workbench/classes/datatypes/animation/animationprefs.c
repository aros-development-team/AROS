/*
    Copyright © 2015-2016, The AROS Development	Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

/* amigaos prototypes */
#include <proto/exec.h>
#include <proto/dos.h>

/* ansi includes */
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "animationclass.h"

/*****************************************************************************/
/* local prototypes */
static STRPTR GetPrefsVar(STRPTR);
static BOOL  matchstr(STRPTR, STRPTR);
/*****************************************************************************/


/****** animation.datatype/preferences *****************************************
*
*   NAME
*       preferences
*
*   DESCRIPTION
*       The "ENV:Classes/DataTypes/animation.prefs" file contains global
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
*       MATCHCLASS/K,MATCHPROJECT/K,STACK/K/N,BUFFERTIME/K/N,BUFFERSTEP/K/N
*
*       MATCHCLASS -- settings in this line will only affect matching
*           classes, e.g. if the case-insensitive pattern does not match,
*           this line is ignored.
*           The maximum length of the pattern is 128 chars.
*           Defaults to #?, which matches any class.
*
*       MATCHPROJECT -- settings in this line will only affect matching
*           projects, e.g. if the case-insensitive pattern does not match,
*           this line is ignored.
*           The maximum length of the pattern is 128 chars.
*           Defaults to #?, which matches any project.
*
*       STACK -- The amount of stack memory to use for the playback/buffering
*           child processes.
*
*       BUFFERTIME -- specify the number of seconds of playback to buffer.
*
*       BUFFERSTEP -- specify the number of frames to buffer at one time.
*
*   NOTE
*
*   BUGS
*
*******************************************************************************/

static STRPTR GetPrefsVar(STRPTR name)
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

static BOOL matchstr(STRPTR pat, STRPTR s)
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

void ReadENVPrefs(Object *o, struct Animation_Data *animd)
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
        STRPTR  matchclass;
        STRPTR  matchproject;
        IPTR    *stack;
        IPTR    *bufftime;
        IPTR    *buffstep;
    } animationargs;

    TEXT   varbuff[ 258 ];
    STRPTR var;

    D(bug("[animation.datatype]: %s()\n", __PRETTY_FUNCTION__));

    if ((var = GetPrefsVar("Classes/DataTypes/animation.prefs" ) ) != NULL)
    {
        STRPTR    prefsline      = var,
                  nextprefsline;
        ULONG     linecount      = 1UL;

        /* Be sure that "var" contains at least one break-char */
        strcat( var, "\n" );

        while ((nextprefsline = strpbrk( prefsline, "\n" ) ) != NULL)
        {
            stccpy(varbuff, prefsline, (int)MIN((sizeof(varbuff) - 2UL), (((ULONG)(nextprefsline - prefsline)) + 1UL)));

            /* be sure that this line isn't a comment line or an empty line */
            if ((varbuff[ 0 ] != '#') && (varbuff[ 0 ] != ';') && (varbuff[ 0 ] != '\n') && (strlen( varbuff ) > 2UL))
            {
                /* Prepare ReadArgs processing */
                strcat( varbuff, "\n" );                                        /* Add NEWLINE-char            */
                envvarrda . RDA_Source . CS_Buffer = varbuff;                   /* Buffer                      */
                envvarrda . RDA_Source . CS_Length = strlen( varbuff ) + 1UL;   /* Set up input buffer length  */
                envvarrda . RDA_Source . CS_CurChr = 0L;
                envvarrda . RDA_Buffer = NULL;
                envvarrda . RDA_BufSiz = 0L;
                memset((void *)(&animationargs), 0, sizeof( animationargs ));   /* Clear result array          */

                if (ReadArgs("MATCHCLASS/K,"
                        "MATCHPROJECT/K,"
                        "STACK/K/N,"
                        "BUFFERTIME/K/N,"
                        "BUFFERSTEP/K/N", (IPTR *)(&animationargs), (&envvarrda)))
                {
                    BOOL noignore = TRUE;

                    if (animationargs.matchclass)
                    {
#if (0)
                        noignore = matchstr((animationargs.matchclass), (animd->ad_BaseName));
#endif
                        D(bug("[animation.datatype] %s: matched class\n", __PRETTY_FUNCTION__));
                    }

                    if ((animationargs.matchproject) && (animd->ad_BaseName))
                    {
                        noignore = matchstr((animationargs.matchproject), (animd->ad_BaseName));
                        D(bug("[animation.datatype] %s: matched project\n", __PRETTY_FUNCTION__));
                    }

                    if (noignore)
                    {
                        if (animationargs.stack)
                        {
                            animd->ad_ProcStack = *(animationargs.stack);
                            D(bug("[animation.datatype] %s:   using %d bytes for child process stack(s)\n", __PRETTY_FUNCTION__, animd->ad_ProcStack));
                        }
                        if (animationargs.bufftime)
                        {
                            animd->ad_BufferTime = (ULONG)*(animationargs.bufftime);
                            D(bug("[animation.datatype] %s:   buffering %d seconds of frames\n", __PRETTY_FUNCTION__, animd->ad_BufferTime));
                        }
                        if (animationargs.buffstep)
                        {
                            animd->ad_BufferStep = (ULONG)*(animationargs.buffstep);
                            D(bug("[animation.datatype] %s:   buffering %d frames at a time\n", __PRETTY_FUNCTION__, animd->ad_BufferStep));
                        }
                    }

                    FreeArgs(&envvarrda);
                }
                else
                {
                    LONG ioerr = IoErr();
                    TEXT errbuff[ 256 ];

                    Fault(ioerr, "Classes/DataTypes/animation.prefs", errbuff, (LONG)sizeof(errbuff));

                    bug("[animation.datatype] preferences \"%s\" line %lu\n", errbuff, linecount);
                }
            }
            prefsline = ++nextprefsline;
            linecount++;
        }
        FreeVec( var );
    }
}




