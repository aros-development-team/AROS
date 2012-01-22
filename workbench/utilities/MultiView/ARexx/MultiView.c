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

#include <dos/rdargs.h>

#include "MultiView.h"

struct rxs_stemnode
{
        struct rxs_stemnode *succ;
        char *name;
        char *value;
};


extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct RxsLib *RexxSysBase;

void (*ARexxResultHook)( struct RexxHost *, struct RexxMsg * ) = NULL;



void ReplyRexxCommand(
        struct RexxMsg  *rexxmessage,
        long                    primary,
        long                    secondary,
        char                    *result )
{
        if( rexxmessage->rm_Action & RXFF_RESULT )
        {
                if( primary == 0 )
                {
                        secondary = result
                                ? (long) CreateArgstring( result, strlen(result) )
                                : (long) NULL;
                }
                else
                {
                        char buf[16];
                        
                        if( primary > 0 )
                        {
                                sprintf( buf, "%ld", secondary );
                                result = buf;
                        }
                        else
                        {
                                primary = -primary;
                                result = (char *) secondary;
                        }
                        
                        SetRexxVar( (struct Message *) rexxmessage,
                                "RC2", result, strlen(result) );
                        
                        secondary = 0;
                }
        }
        else if( primary < 0 )
                primary = -primary;
        
        rexxmessage->rm_Result1 = primary;
        rexxmessage->rm_Result2 = secondary;
        ReplyMsg( (struct Message *) rexxmessage );
}


void FreeRexxCommand( struct RexxMsg *rexxmessage )
{
        if( !rexxmessage->rm_Result1 && rexxmessage->rm_Result2 )
                DeleteArgstring( (char *) rexxmessage->rm_Result2 );

        if( rexxmessage->rm_Stdin &&
                rexxmessage->rm_Stdin != Input() )
                Close( rexxmessage->rm_Stdin );

        if( rexxmessage->rm_Stdout &&
                rexxmessage->rm_Stdout != rexxmessage->rm_Stdin &&
                rexxmessage->rm_Stdout != Output() )
                Close( rexxmessage->rm_Stdout );

        DeleteArgstring( (char *) ARG0(rexxmessage) );
        DeleteRexxMsg( rexxmessage );
}


struct RexxMsg *CreateRexxCommand( struct RexxHost *host, char *buff, BPTR fh )
{
        struct RexxMsg *rexx_command_message;

        if( (rexx_command_message = CreateRexxMsg( host->port,
                rexx_extension, host->port->mp_Node.ln_Name)) == NULL )
        {
                return( NULL );
        }

        if( (rexx_command_message->rm_Args[0] =
                CreateArgstring(buff,strlen(buff))) == NULL )
        {
                DeleteRexxMsg(rexx_command_message);
                return( NULL );
        }

        rexx_command_message->rm_Action = RXCOMM | RXFF_RESULT;
        rexx_command_message->rm_Stdin  = fh;
        rexx_command_message->rm_Stdout = fh;
        
        return( rexx_command_message );
}


struct RexxMsg *CommandToRexx( struct RexxHost *host, struct RexxMsg *rexx_command_message )
{
        struct MsgPort *rexxport;
        
        Forbid();

        if( (rexxport = FindPort(RXSDIR)) == NULL )
        {
                Permit();
                return( NULL );
        }

        PutMsg( rexxport, &rexx_command_message->rm_Node );
        
        Permit();
        
        ++host->replies;
        return( rexx_command_message );
}


struct RexxMsg *SendRexxCommand( struct RexxHost *host, char *buff, BPTR fh )
{
        struct RexxMsg *rcm;
        
        if( rcm = CreateRexxCommand(host, buff, fh) )
                return CommandToRexx( host, rcm );
        else
                return NULL;
}


void CloseDownARexxHost( struct RexxHost *host )
{
        struct RexxMsg *rexxmsg;
        
        if( host->port )
        {
                /* Port abmelden */
                RemPort( host->port );
                
                /* Auf noch ausstehende Replies warten */
                while( host->replies > 0 )
                {
                        WaitPort( host->port );
                        
                        while( rexxmsg = (struct RexxMsg *) GetMsg(host->port) )
                        {
                                if( rexxmsg->rm_Node.mn_Node.ln_Type == NT_REPLYMSG )
                                {
                                        if( !rexxmsg->rm_Args[15] )
                                        {
                                                /* Reply zu einem SendRexxCommand()-Call */
                                                if( ARexxResultHook )
                                                        ARexxResultHook( host, rexxmsg );
                                        }
                                        
                                        FreeRexxCommand( rexxmsg );
                                        --host->replies;
                                }
                                else
                                        ReplyRexxCommand( rexxmsg, -20, (long) "Host closing down", NULL );
                        }
                }
                
                /* MsgPort leeren */
                while( rexxmsg = (struct RexxMsg *) GetMsg(host->port) )
                        ReplyRexxCommand( rexxmsg, -20, (long) "Host closing down", NULL );
                
                if( !(host->flags & ARB_HF_USRMSGPORT) )
                        DeleteMsgPort( host->port );
        }
        
        if( host->rdargs ) FreeDosObject( DOS_RDARGS, host->rdargs );
        FreeVec( host );
}


struct RexxHost *SetupARexxHost( char *basename, struct MsgPort *usrport )
{
        struct RexxHost *host;
        int ext = 0;
        
        if( !basename )
                basename = RexxPortBaseName;
        else if( !*basename )
                basename = RexxPortBaseName;
        
        if( !(host = AllocVec(sizeof *host, MEMF_CLEAR)) )
                return NULL;
        
        strcpy( host->portname, basename );
        
        if( usrport )
        {
                host->port   = usrport;
                host->flags |= ARB_HF_USRMSGPORT;
        }
        else if( !(host->port = CreateMsgPort()) )
        {
                FreeVec( host );
                return NULL;
        }
        else
        {
                host->port->mp_Node.ln_Pri = 0;
        }
        
        Forbid();
        
        while( FindPort(host->portname) )
                sprintf( host->portname, "%s.%d", basename, ++ext );
        
        host->port->mp_Node.ln_Name = host->portname;
        AddPort( host->port );
        
        Permit();
        
        if( !(host->rdargs = AllocDosObject(DOS_RDARGS, NULL)) )
        {
                RemPort( host->port );
                if( !usrport ) DeleteMsgPort( host->port );
                FreeVec( host );
                return NULL;
        }
        
        host->rdargs->RDA_Flags = RDAF_NOPROMPT;
        
        return( host );
}


/* StateMachine für FindRXCommand() */

static inline char *scmp( char *inp, char *str )
{
        while( *str && *inp )
                if( *inp++ != *str++ )
                        return NULL;
        
        /* Reststring zurückgeben */
        return inp;
}

static int find( char *input )
{
        struct arb_p_state *st = arb_p_state;
        struct arb_p_link *ad;
        char *ni, tmp[36], *s;
        
        ni = tmp;
        while( *input && ni-tmp < 32 )
                *ni++ = toupper( *input++ );
        *ni = 0;
        input = tmp;
        
        while( *input )
        {
                /* Terminalzustand erreicht? */
                if( !st->pa )
                {
                        if( *input )
                                return -1;
                        else
                                return st->cmd;
                }
                
                /* Wo geht's weiter? */
                ni = 0;
                for( ad = st->pa; s = ad->str; ad++ )
                {
                        /* die Links sind absteigend sortiert */
                        if( *input > *s )
                                break;
                        
                        if( *input == *s )
                                if( ni = scmp(input+1, s+1) )
                                        break;
                }
                
                /* Nirgends... */
                if( !ni )
                        return -1;
                
                /* Zustandsüberführung */
                st = arb_p_state + ad->dst;
                input = ni;
        }
        
        return st->cmd;
}

struct rxs_command *FindRXCommand( char *com )
{
        int cmd;
        
        cmd = find( com );
        
        if( cmd == -1 )
                return NULL;
        else
                return( rxs_commandlist + cmd );
}


static struct rxs_command *ParseRXCommand( char **arg )
{
        char com[256], *s, *t;
        
        s = *arg;
        t = com;
        
        while( *s && *s != ' ' && *s != '\n' )
                *t++ = *s++;
        
        *t = '\0';
        while( *s == ' ' ) ++s;
        *arg = s;
        
        return( FindRXCommand( com ) );
}


static char *CreateVAR( struct rxs_stemnode *stem )
{
        char *var;
        struct rxs_stemnode *s;
        long size = 0;
        
        if( !stem || stem == (struct rxs_stemnode *) -1L )
                return( (char *) stem );
        
        for( s = stem; s; s = s->succ )
                size += strlen( s->value ) + 1;
        
        if( !(var = AllocVec( size + 1, MEMF_ANY )) )
                return( (char *) -1 );
        
        *var = '\0';
        
        for( s = stem; s; s = s->succ )
        {
                strcat( var, s->value );
                if( s->succ )
                        strcat( var, " " );
        }
        
        return( var );
}


static struct rxs_stemnode *new_stemnode( struct rxs_stemnode **first, struct rxs_stemnode **old )
{
        struct rxs_stemnode *new;
        
        if( !(new = AllocVec(sizeof(struct rxs_stemnode), MEMF_CLEAR)) )
        {
                return( NULL );
        }
        else
        {
                if( *old )
                {
                        (*old)->succ = new;
                        (*old) = new;
                }
                else
                {
                        *first = *old = new;
                }
        }
        
        return( new );
}


static void free_stemlist( struct rxs_stemnode *first )
{
        struct rxs_stemnode *next;
        
        if( (long) first == -1 )
                return;
        
        for( ; first; first = next )
        {
                next = first->succ;
                if( first->name  ) FreeVec( first->name );
                if( first->value ) FreeVec( first->value );
                FreeVec( first );
        }
}


char *StrDup( char *s )
{
        char *t = AllocVec( strlen(s)+1, MEMF_ANY );
        if( t ) strcpy( t, s );
        return t;
}


static struct rxs_stemnode *CreateSTEM( struct rxs_command *rxc, LONG *resarray, char *stembase )
{
        struct rxs_stemnode *first = NULL, *old = NULL, *new;
        char resb[512], *rs, *rb;
        char longbuff[16];
        
        rb = resb;
        if( stembase )
        {
                while( *stembase )
                        *rb++ = toupper( *stembase++ );
        }
        *rb = '\0';
        
        rb = resb + strlen(resb);
        rs = rxc->results;
        
        while( *rs )
        {
                char *t = rb;
                BOOL optn = FALSE, optm = FALSE;
                
                while( *rs && *rs != ',' )
                {
                        if( *rs == '/' )
                        {
                                ++rs;
                                if( *rs == 'N' ) optn = TRUE;
                                else if( *rs == 'M' ) optm = TRUE;
                        }
                        else
                                *t++ = *rs;
                        
                        ++rs;
                }
                
                if( *rs == ',' ) ++rs;
                *t = '\0';
                
                /*
                 * Resultat(e) erzeugen
                 */
                
                if( !*resarray )
                {
                        ++resarray;
                        continue;
                }
                
                if( optm )
                {
                        long *r, index = 0;
                        LONG **subarray = (LONG **) *resarray++;
                        struct rxs_stemnode *countnd;
                        
                        /* Anzahl der Elemente */
                        
                        if( !(new = new_stemnode(&first, &old)) )
                        {
                                free_stemlist( first );
                                return( (struct rxs_stemnode *) -1L );
                        }
                        countnd = new;
                        
                        /* Die Elemente selbst */
                        
                        while( r = *subarray++ )
                        {
                                if( !(new = new_stemnode(&first, &old)) )
                                {
                                        free_stemlist( first );
                                        return( (struct rxs_stemnode *) -1L );
                                }
                                
                                sprintf( t, ".%ld", index++ );
                                new->name = StrDup( resb );
                                
                                if( optn )
                                {
                                        sprintf( longbuff, "%ld", *r );
                                        new->value = StrDup( longbuff );
                                }
                                else
                                {
                                        new->value = StrDup( (char *) r );
                                }
                        }
                        
                        /* Die Count-Node */
                        
                        strcpy( t, ".COUNT" );
                        countnd->name = StrDup( resb );
                        
                        sprintf( longbuff, "%ld", index );
                        countnd->value = StrDup( longbuff );
                }
                else
                {
                        /* Neue Node anlegen */
                        if( !(new = new_stemnode(&first, &old)) )
                        {
                                free_stemlist( first );
                                return( (struct rxs_stemnode *) -1L );
                        }
                        
                        new->name = StrDup( resb );
                        
                        if( optn )
                        {
                                sprintf( longbuff, "%ld", *((long *) *resarray) );
                                new->value = StrDup( longbuff );
                                ++resarray;
                        }
                        else
                        {
                                new->value = StrDup( (char *) (*resarray++) );
                        }
                }
        }
        
        return( first );
}


static void DoRXCommand( struct RexxHost *host, struct RexxMsg *rexxmsg )
{
        struct rxs_command *rxc;
        char *argb, *arg;
        
        LONG *array = NULL;
        LONG *argarray;
        LONG *resarray;
        
        char *cargstr = NULL;
        long rc=20, rc2;
        char *result = NULL;
        
        if( !(argb = AllocVec(strlen((char *) ARG0(rexxmsg)) + 2, MEMF_ANY)) )
        {
                rc2 = ERROR_NO_FREE_STORE;
                goto drc_cleanup;
        }
        
        /* welches Kommando? */
        
        strcpy( argb, (char *) ARG0(rexxmsg) );
        strcat( argb, "\n" );
        arg = argb;
        
        if( !( rxc = ParseRXCommand( &arg ) ) )
        {
                if( arg = ExpandRXCommand( host, (char *) ARG0(rexxmsg) ) )
                {
                        FreeVec( argb );
                        if( !(argb = AllocVec( strlen(arg) + 2, MEMF_ANY )) )
                        {
                                rc2 = ERROR_NO_FREE_STORE;
                                goto drc_cleanup;
                        }
                        
                        strcpy( argb, arg );
                        strcat( argb, "\n" );
                        FreeVec( arg );
                        arg = argb;
                        
                        rxc = ParseRXCommand( &arg );
                }
        }
        
        if( !rxc )
        {
                /* Msg an ARexx schicken, vielleicht existiert ein Skript */
                struct RexxMsg *rm;
                
                if( rm = CreateRexxCommand(host, (char *) ARG0(rexxmsg), NULL) )
                {
                        /* Original-Msg merken */
                        rm->rm_Args[15] = (STRPTR) rexxmsg;
                        
                        if( CommandToRexx(host, rm) )
                        {
                                /* Reply wird später vom Dispatcher gemacht */
                                if( argb ) FreeVec( argb );
                                return;
                        }
                        else
                                rc2 = ERROR_NOT_IMPLEMENTED;
                }
                else
                        rc2 = ERROR_NO_FREE_STORE;
                
                goto drc_cleanup;
        }
        
        if( !(rxc->flags & ARB_CF_ENABLED) )
        {
                rc = -10;
                rc2 = (long) "Command disabled";
                goto drc_cleanup;
        }
        
        /* Speicher für Argumente etc. holen */
        
        (rxc->function)( host, (void **) &array, RXIF_INIT, rexxmsg );
        cargstr = AllocVec( rxc->args ? 15+strlen(rxc->args) : 15, MEMF_ANY );
        
        if( !array || !cargstr )
        {
                rc2 = ERROR_NO_FREE_STORE;
                goto drc_cleanup;
        }
        
        argarray = array + 2;
        resarray = array + rxc->resindex;
        
        /* Argumente parsen */
        
        if( rxc->results )
                strcpy( cargstr, "VAR/K,STEM/K" );
        else
                *cargstr = '\0';
        
        if( rxc->args )
        {
                if( *cargstr )
                        strcat( cargstr, "," );
                strcat( cargstr, rxc->args );
        }
        
        if( *cargstr )
        {
                host->rdargs->RDA_Source.CS_Buffer = arg;
                host->rdargs->RDA_Source.CS_Length = strlen(arg);
                host->rdargs->RDA_Source.CS_CurChr = 0;
                host->rdargs->RDA_DAList = NULL;
                host->rdargs->RDA_Buffer = NULL;
                
                if( !ReadArgs(cargstr, argarray, host->rdargs) )
                {
                        rc = 10;
                        rc2 = IoErr();
                        goto drc_cleanup;
                }
        }
        
        /* Funktion aufrufen */
        
        (rxc->function)( host, (void **) &array, RXIF_ACTION, rexxmsg );
        
        rc = array[0];
        rc2 = array[1];
        
        /* Resultat(e) auswerten */
        
        if( rxc->results && rc==0 &&
                (rexxmsg->rm_Action & RXFF_RESULT) )
        {
                struct rxs_stemnode *stem, *s;
                
                stem = CreateSTEM( rxc, resarray, (char *)argarray[1] );
                result = CreateVAR( stem );
                
                if( result )
                {
                        if( argarray[0] )
                        {
                                /* VAR */
                                if( (long) result == -1 )
                                {
                                        rc = 20;
                                        rc2 = ERROR_NO_FREE_STORE;
                                }
                                else
                                {
                                        char *rb;
                                        
                                        for( rb = (char *) argarray[0]; *rb; ++rb )
                                                *rb = toupper( *rb );
                                        
                                        if( SetRexxVar( (struct Message *) rexxmsg,
                                                *((char *)argarray[0]) ? (char *)argarray[0] : "RESULT",
                                                result, strlen(result) ) )
                                        {
                                                rc = -10;
                                                rc2 = (long) "Unable to set Rexx variable";
                                        }
                                        
                                        FreeVec( result );
                                }
                                
                                result = NULL;
                        }
                        
                        if( !rc && argarray[1] )
                        {
                                /* STEM */
                                if( (long) stem == -1 )
                                {
                                        rc = 20;
                                        rc2 = ERROR_NO_FREE_STORE;
                                }
                                else
                                {
                                        for( s = stem; s; s = s->succ )
                                                rc |= SetRexxVar( (struct Message *) rexxmsg, s->name, s->value, strlen(s->value) );
                                        
                                        if( rc )
                                        {
                                                rc = -10;
                                                rc2 = (long) "Unable to set Rexx variable";
                                        }
                                        
                                        if( result && (long) result != -1 )
                                                FreeVec( result );
                                }
                                
                                result = NULL;
                        }
                        
                        /* Normales Resultat: Möglich? */
                        
                        if( (long) result == -1 )
                        {
                                /* Nein */
                                rc = 20;
                                rc2 = ERROR_NO_FREE_STORE;
                                result = NULL;
                        }
                }
                
                free_stemlist( stem );
        }
        
drc_cleanup:

        /* Nur RESULT, wenn weder VAR noch STEM */
        
        ReplyRexxCommand( rexxmsg, rc, rc2, result );
        
        /* benutzten Speicher freigeben */
        
        if( result ) FreeVec( result );
        FreeArgs( host->rdargs );
        if( cargstr ) FreeVec( cargstr );
        if( array ) (rxc->function)( host, (void **) &array, RXIF_FREE, rexxmsg );
        if( argb ) FreeVec( argb );
}


void ARexxDispatch( struct RexxHost *host )
{
        struct RexxMsg *rexxmsg;

        while( rexxmsg = (struct RexxMsg *) GetMsg(host->port) )
        {
                if( (rexxmsg->rm_Action & RXCODEMASK) != RXCOMM )
                {
                        /* Keine Rexx-Message */
                        ReplyMsg( (struct Message *) rexxmsg );
                }
                else if( rexxmsg->rm_Node.mn_Node.ln_Type == NT_REPLYMSG )
                {
                        struct RexxMsg *org = (struct RexxMsg *) rexxmsg->rm_Args[15];
                        
                        if( org )
                        {
                                /* Reply zu durchgereichter Msg */
                                if( rexxmsg->rm_Result1 != 0 )
                                {
                                        /* Befehl unbekannt */
                                        ReplyRexxCommand( org, 20, ERROR_NOT_IMPLEMENTED, NULL );
                                }
                                else
                                {
                                        ReplyRexxCommand( org, 0, 0, (char *) rexxmsg->rm_Result2 );
                                }
                        }
                        else
                        {
                                /* Reply zu einem SendRexxCommand()-Call */
                                if( ARexxResultHook )
                                        ARexxResultHook( host, rexxmsg );
                        }
                        
                        FreeRexxCommand( rexxmsg );
                        --host->replies;
                }
                else if( ARG0(rexxmsg) )
                {
                        DoRXCommand( host, rexxmsg );
                }
                else
                {
                        ReplyMsg( (struct Message *) rexxmsg );
                }
        }
}

