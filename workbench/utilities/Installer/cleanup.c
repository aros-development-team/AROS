/* cleanup.c -- here are all functions used before exiting program */
#include "Installer.h"

void free_script( ScriptArg *first )
{
  if( first != NULL )
  {
    free_script( first->cmd );
    free_script( first->next );
    free( first->arg );
    free( first->value );
    free( first );
  }
}
