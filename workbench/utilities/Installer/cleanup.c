/* cleanup.c -- here are all functions used before exiting program */
#include "Installer.h"

/* External variables */
extern ScriptArg script;

/* External function prototypes */
extern void free_varlist();

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
}

void end_malloc( )
{
  cleanup();
  printf("Couldn't malloc memory!\n");
  exit(-1);
}

