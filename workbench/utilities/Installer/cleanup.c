/* cleanup.c -- here are all functions used before exiting program */

#include "Installer.h"

/* External variables */
extern ScriptArg script;
extern InstallerPrefs preferences;

/* External function prototypes */
extern void free_varlist();
extern void execute_script( ScriptArg *, int );
#ifndef LINUX
extern void deinit_gui();
#endif /* !LINUX */

/* Internal function prototypes */
void free_script( ScriptArg * );
void cleanup();
void end_malloc();


void free_script( ScriptArg *first )
{
  if( first != NULL )
  {
    free_script( first->cmd );
    free_script( first->next );
    free( first->arg );
    free( first );
  }
}

void cleanup( )
{
  free_script( script.cmd );
  free_varlist();
#ifndef LINUX
  deinit_gui();
#endif /* !LINUX */
}

void end_malloc( )
{
#ifdef DEBUG
  printf("Couldn't malloc memory!\n");
#endif /* DEBUG */
#warning FIXME: handle (trap ...) routine
  if( preferences.trap[OUTOFMEMORY - 1].cmd != NULL )
  {
    execute_script( preferences.trap[OUTOFMEMORY - 1].cmd, -99 );
  }
  else
  {
    if( preferences.onerror.cmd != NULL )
    {
      execute_script( preferences.onerror.cmd, -99 );
    }
  }
  cleanup();
  exit(-1);
}

