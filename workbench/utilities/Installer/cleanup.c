/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/* cleanup.c -- here are all functions used before exiting program */

#include "Installer.h"

/* External variables */
extern ScriptArg script;
extern InstallerPrefs preferences;
extern int error;
extern struct IconBase *IconBase;

/* External function prototypes */
extern void free_varlist();
extern void execute_script( ScriptArg *, int );
extern void deinit_gui();
extern void traperr( char *, char * );

/* Internal function prototypes */
void free_script( ScriptArg * );
void cleanup();
void end_malloc();
void outofmem( void * );

void free_script( ScriptArg *first )
{
  if ( first != NULL )
  {
    free_script( first->cmd );
    free_script( first->next );
    free( first->arg );
    free( first );
  }
}

void cleanup( )
{
  if ( preferences.transcriptstream != NULL )
  {
    Close( preferences.transcriptstream );
  }

  free_script( script.cmd );
  free_varlist();
  deinit_gui();
  CloseLibrary( (struct Library*)IconBase );
}

void end_malloc( )
{
#ifdef DEBUG
  fprintf( stderr, "Couldn't malloc memory!\n");
#endif /* DEBUG */
  cleanup();
  exit(-1);
}

void outofmem( void * ptr )
{
  if ( ptr == NULL )
  {
    error = OUTOFMEMORY;
    traperr( "Out of memory!\n", NULL );
  }
}

