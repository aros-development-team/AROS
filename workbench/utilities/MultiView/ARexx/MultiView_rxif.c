/*
 * Source generated with ARexxBox 1.12 (May 18 1993)
 * which is Copyright (c) 1992,1993 Michael Balzer
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>

#ifdef __GNUC__
/* GCC needs all struct defs */
#include <dos/exall.h>
#include <graphics/graphint.h>
#include <intuition/classes.h>
#include <devices/keymap.h>
#include <exec/semaphores.h>
#endif

#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/rexxsyslib_protos.h>

#ifndef __NO_PRAGMAS

#ifdef AZTEC_C
#include <pragmas/exec_lib.h>
#include <pragmas/dos_lib.h>
#include <pragmas/rexxsyslib_lib.h>
#endif

#ifdef LATTICE
#include <pragmas/exec_pragmas.h>
#include <pragmas/dos_pragmas.h>
#include <pragmas/rexxsyslib_pragmas.h>
#endif

#endif /* __NO_PRAGMAS */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef LATTICE
#undef toupper
#define inline __inline
#endif

#ifdef __GNUC__
#undef toupper
static inline int toupper( int c )
{
        return( islower(c) ? c - 'a' + 'A' : c );
}
#endif

#ifdef AZTEC_C
#define inline
#endif

#include "MultiView.h"


extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct RxsLib *RexxSysBase;


/* $ARB: I 1191273255 */


/* $ARB: B 1 OPEN */
void rx_open( struct RexxHost *host, struct rxd_open **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_open *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        if( rd = *rxd )
                        {
                                /* set your DEFAULTS here */
                        }
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 1 OPEN */

/* $ARB: B 2 RELOAD */
void rx_reload( struct RexxHost *host, struct rxd_reload **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_reload *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 2 RELOAD */

/* $ARB: B 3 SAVEAS */
void rx_saveas( struct RexxHost *host, struct rxd_saveas **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_saveas *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        if( rd = *rxd )
                        {
                                /* set your DEFAULTS here */
                        }
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 3 SAVEAS */

/* $ARB: B 4 PRINT */
void rx_print( struct RexxHost *host, struct rxd_print **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_print *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 4 PRINT */

/* $ARB: B 5 ABOUT */
void rx_about( struct RexxHost *host, struct rxd_about **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_about *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 5 ABOUT */

/* $ARB: B 6 QUIT */
void rx_quit( struct RexxHost *host, struct rxd_quit **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_quit *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 6 QUIT */

/* $ARB: B 7 MARK */
void rx_mark( struct RexxHost *host, struct rxd_mark **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_mark *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 7 MARK */

/* $ARB: B 8 COPY */
void rx_copy( struct RexxHost *host, struct rxd_copy **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_copy *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 8 COPY */

/* $ARB: B 9 CLEARSELECTED */
void rx_clearselected( struct RexxHost *host, struct rxd_clearselected **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_clearselected *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 9 CLEARSELECTED */

/* $ARB: B 10 GETTRIGGERINFO */
void rx_gettriggerinfo( struct RexxHost *host, struct rxd_gettriggerinfo **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_gettriggerinfo *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        if( rd = *rxd )
                        {
                                /* set your DEFAULTS here */
                        }
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 10 GETTRIGGERINFO */

/* $ARB: B 11 DOTRIGGERMETHOD */
void rx_dotriggermethod( struct RexxHost *host, struct rxd_dotriggermethod **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_dotriggermethod *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        if( rd = *rxd )
                        {
                                /* set your DEFAULTS here */
                        }
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 11 DOTRIGGERMETHOD */

/* $ARB: B 12 SCREEN */
void rx_screen( struct RexxHost *host, struct rxd_screen **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_screen *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        if( rd = *rxd )
                        {
                                /* set your DEFAULTS here */
                        }
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 12 SCREEN */

/* $ARB: B 13 PUBSCREEN */
void rx_pubscreen( struct RexxHost *host, struct rxd_pubscreen **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_pubscreen *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        if( rd = *rxd )
                        {
                                /* set your DEFAULTS here */
                        }
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 13 PUBSCREEN */

/* $ARB: B 14 SNAPSHOT */
void rx_snapshot( struct RexxHost *host, struct rxd_snapshot **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_snapshot *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 14 SNAPSHOT */

/* $ARB: B 15 GETCURRENTDIR */
void rx_getcurrentdir( struct RexxHost *host, struct rxd_getcurrentdir **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_getcurrentdir *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 15 GETCURRENTDIR */

/* $ARB: B 16 GETFILEINFO */
void rx_getfileinfo( struct RexxHost *host, struct rxd_getfileinfo **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_getfileinfo *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 16 GETFILEINFO */

/* $ARB: B 17 GETOBJECTINFO */
void rx_getobjectinfo( struct RexxHost *host, struct rxd_getobjectinfo **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_getobjectinfo *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        if( rd = *rxd )
                        {
                                /* set your DEFAULTS here */
                        }
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 17 GETOBJECTINFO */

/* $ARB: B 18 MINIMUMSIZE */
void rx_minimumsize( struct RexxHost *host, struct rxd_minimumsize **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_minimumsize *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 18 MINIMUMSIZE */

/* $ARB: B 19 NORMALSIZE */
void rx_normalsize( struct RexxHost *host, struct rxd_normalsize **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_normalsize *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 19 NORMALSIZE */

/* $ARB: B 20 MAXIMUMSIZE */
void rx_maximumsize( struct RexxHost *host, struct rxd_maximumsize **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_maximumsize *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 20 MAXIMUMSIZE */

/* $ARB: B 21 WINDOWTOFRONT */
void rx_windowtofront( struct RexxHost *host, struct rxd_windowtofront **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_windowtofront *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 21 WINDOWTOFRONT */

/* $ARB: B 22 WINDOWTOBACK */
void rx_windowtoback( struct RexxHost *host, struct rxd_windowtoback **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_windowtoback *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 22 WINDOWTOBACK */

/* $ARB: B 23 ACTIVATEWINDOW */
void rx_activatewindow( struct RexxHost *host, struct rxd_activatewindow **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_activatewindow *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 23 ACTIVATEWINDOW */

/* $ARB: B 24 SCREENTOFRONT */
void rx_screentofront( struct RexxHost *host, struct rxd_screentofront **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_screentofront *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 24 SCREENTOFRONT */

/* $ARB: B 25 SCREENTOBACK */
void rx_screentoback( struct RexxHost *host, struct rxd_screentoback **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_screentoback *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 25 SCREENTOBACK */

/* $ARB: B 26 BEEPSCREEN */
void rx_beepscreen( struct RexxHost *host, struct rxd_beepscreen **rxd, long action, struct RexxMsg *rexxmsg )
{
        struct rxd_beepscreen *rd = *rxd;

        switch( action )
        {
                case RXIF_INIT:
                        *rxd = AllocVec( sizeof *rd, MEMF_CLEAR );
                        break;
                        
                case RXIF_ACTION:
                        /* Insert your CODE here */
                        rd->rc = 0;
                        break;
                
                case RXIF_FREE:
                        /* FREE your local data here */
                        FreeVec( rd );
                        break;
        }
        return;
}
/* $ARB: E 26 BEEPSCREEN */


#ifndef RX_ALIAS_C
char *ExpandRXCommand( struct RexxHost *host, char *command )
{
        /* Insert your ALIAS-HANDLER here */
        return( NULL );
}
#endif

