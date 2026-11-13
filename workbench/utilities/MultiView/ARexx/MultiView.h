/*
 * Source generated with ARexxBox 1.12 (May 18 1993)
 * which is Copyright (c) 1992,1993 Michael Balzer
 */

#ifndef _MultiView_H
#define _MultiView_H

#define RXIF_INIT   1
#define RXIF_ACTION 2
#define RXIF_FREE   3

#define ARB_CF_ENABLED     (1L << 0)

#define ARB_HF_CMDSHELL    (1L << 0)
#define ARB_HF_USRMSGPORT  (1L << 1)

struct RexxHost
{
        struct MsgPort *port;
        char portname[ 80 ];
        long replies;
        struct RDArgs *rdargs;
        long flags;
        APTR userdata;
};

struct rxs_command
{
        char *command, *args, *results;
        long resindex;
        void (*function)( struct RexxHost *, void **, long, struct RexxMsg * );
        long flags;
};

struct arb_p_link
{
        char    *str;
        int             dst;
};

struct arb_p_state
{
        int             cmd;
        struct arb_p_link *pa;
};

#ifndef NO_GLOBALS
extern char RexxPortBaseName[80];
extern struct rxs_command rxs_commandlist[];
extern struct arb_p_state arb_p_state[];
extern int command_cnt;
extern char *rexx_extension;
#endif

void ReplyRexxCommand( struct RexxMsg *rxmsg, long prim, long sec, char *res );
void FreeRexxCommand( struct RexxMsg *rxmsg );
struct RexxMsg *CreateRexxCommand( struct RexxHost *host, char *buff, BPTR fh );
struct RexxMsg *CommandToRexx( struct RexxHost *host, struct RexxMsg *rexx_command_message );
struct RexxMsg *SendRexxCommand( struct RexxHost *host, char *buff, BPTR fh );

void CloseDownARexxHost( struct RexxHost *host );
struct RexxHost *SetupARexxHost( char *basename, struct MsgPort *usrport );
struct rxs_command *FindRXCommand( char *com );
char *ExpandRXCommand( struct RexxHost *host, char *command );
char *StrDup( char *s );
void ARexxDispatch( struct RexxHost *host );

/* rxd-Strukturen dürfen nur AM ENDE um lokale Variablen erweitert werden! */

struct rxd_about
{
        long rc, rc2;
};

void rx_about( struct RexxHost *, struct rxd_about **, long, struct RexxMsg * );

struct rxd_activatewindow
{
        long rc, rc2;
};

void rx_activatewindow( struct RexxHost *, struct rxd_activatewindow **, long, struct RexxMsg * );

struct rxd_beepscreen
{
        long rc, rc2;
};

void rx_beepscreen( struct RexxHost *, struct rxd_beepscreen **, long, struct RexxMsg * );

struct rxd_clearselected
{
        long rc, rc2;
};

void rx_clearselected( struct RexxHost *, struct rxd_clearselected **, long, struct RexxMsg * );

struct rxd_copy
{
        long rc, rc2;
};

void rx_copy( struct RexxHost *, struct rxd_copy **, long, struct RexxMsg * );

struct rxd_dotriggermethod
{
        long rc, rc2;
        struct {
                char *method;
        } arg;
};

void rx_dotriggermethod( struct RexxHost *, struct rxd_dotriggermethod **, long, struct RexxMsg * );

struct rxd_getcurrentdir
{
        long rc, rc2;
        struct {
                char *var, *stem;
        } arg;
        struct {
                char *currentdir;
        } res;
};

void rx_getcurrentdir( struct RexxHost *, struct rxd_getcurrentdir **, long, struct RexxMsg * );

struct rxd_getfileinfo
{
        long rc, rc2;
        struct {
                char *var, *stem;
        } arg;
        struct {
                char *fileinfo;
        } res;
};

void rx_getfileinfo( struct RexxHost *, struct rxd_getfileinfo **, long, struct RexxMsg * );

struct rxd_getobjectinfo
{
        long rc, rc2;
        struct {
                char *var, *stem;
                long myvar;
                char *mystem;
        } arg;
        struct {
                char *result;
        } res;
};

void rx_getobjectinfo( struct RexxHost *, struct rxd_getobjectinfo **, long, struct RexxMsg * );

struct rxd_gettriggerinfo
{
        long rc, rc2;
        struct {
                char *var, *stem;
                long myvar;
                char *mystem;
        } arg;
        struct {
                char *result;
        } res;
};

void rx_gettriggerinfo( struct RexxHost *, struct rxd_gettriggerinfo **, long, struct RexxMsg * );

struct rxd_mark
{
        long rc, rc2;
};

void rx_mark( struct RexxHost *, struct rxd_mark **, long, struct RexxMsg * );

struct rxd_maximumsize
{
        long rc, rc2;
};

void rx_maximumsize( struct RexxHost *, struct rxd_maximumsize **, long, struct RexxMsg * );

struct rxd_minimumsize
{
        long rc, rc2;
};

void rx_minimumsize( struct RexxHost *, struct rxd_minimumsize **, long, struct RexxMsg * );

struct rxd_normalsize
{
        long rc, rc2;
};

void rx_normalsize( struct RexxHost *, struct rxd_normalsize **, long, struct RexxMsg * );

struct rxd_open
{
        long rc, rc2;
        struct {
                char *name;
                long clipboard;
                long *clipunit;
        } arg;
};

void rx_open( struct RexxHost *, struct rxd_open **, long, struct RexxMsg * );

struct rxd_print
{
        long rc, rc2;
};

void rx_print( struct RexxHost *, struct rxd_print **, long, struct RexxMsg * );

struct rxd_pubscreen
{
        long rc, rc2;
        struct {
                char *name;
        } arg;
};

void rx_pubscreen( struct RexxHost *, struct rxd_pubscreen **, long, struct RexxMsg * );

struct rxd_quit
{
        long rc, rc2;
};

void rx_quit( struct RexxHost *, struct rxd_quit **, long, struct RexxMsg * );

struct rxd_reload
{
        long rc, rc2;
};

void rx_reload( struct RexxHost *, struct rxd_reload **, long, struct RexxMsg * );

struct rxd_saveas
{
        long rc, rc2;
        struct {
                char *name;
                long iff;
        } arg;
};

void rx_saveas( struct RexxHost *, struct rxd_saveas **, long, struct RexxMsg * );

struct rxd_screen
{
        long rc, rc2;
        struct {
                long true;
                long false;
        } arg;
};

void rx_screen( struct RexxHost *, struct rxd_screen **, long, struct RexxMsg * );

struct rxd_screentoback
{
        long rc, rc2;
};

void rx_screentoback( struct RexxHost *, struct rxd_screentoback **, long, struct RexxMsg * );

struct rxd_screentofront
{
        long rc, rc2;
};

void rx_screentofront( struct RexxHost *, struct rxd_screentofront **, long, struct RexxMsg * );

struct rxd_snapshot
{
        long rc, rc2;
};

void rx_snapshot( struct RexxHost *, struct rxd_snapshot **, long, struct RexxMsg * );

struct rxd_windowtoback
{
        long rc, rc2;
};

void rx_windowtoback( struct RexxHost *, struct rxd_windowtoback **, long, struct RexxMsg * );

struct rxd_windowtofront
{
        long rc, rc2;
};

void rx_windowtofront( struct RexxHost *, struct rxd_windowtofront **, long, struct RexxMsg * );

#endif
