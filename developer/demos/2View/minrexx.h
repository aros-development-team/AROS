#ifndef _MINREXX_H
#define _MINREXX_H

/*
 *   Includes for minrexx.c; please refer to that file for
 *   further documentation.
 */
/* #include <rexx/rxslib.h> */

/*
 *   This is the list of functions we can access.  (Cheap forward
 *   declarations, too.)
 */
long upRexxPort() ;
void dnRexxPort() ;
void dispRexxPort() ;
struct RexxMsg *sendRexxCmd() ;
struct RexxMsg *syncRexxCmd() ;
struct RexxMsg *asyncRexxCmd() ;
void replyRexxCmd() ;
/*
 *   Maximum messages that can be pending, and the return codes
 *   for two bad situations.
 */
#define MAXRXOUTSTANDING (300)
#define RXERRORIMGONE (100)
#define RXERRORNOCMD (30)
/*
 *   This is the association list you build up (statically or
 *   dynamically) that should be terminated with an entry with
 *   NULL for the name . . .
 */
struct rexxCommandList {
   char *name ;
   APTR userdata ;
} ;

#endif /* _MINREXX_H */
