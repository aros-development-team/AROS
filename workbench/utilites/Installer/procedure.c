/* procedure.c -- Here are all functions related to user-defined procedures */

#include "Installer.h"

/* External variables */

/* External function prototypes */
extern void end_malloc();

/* Internal function prototypes */
struct ScriptArg *find_proc( char * );
void set_procedure( char *, ScriptArg * );
void free_proclist();


int numusrprocs = 0;
struct ProcedureList *usrprocs = NULL;

struct ScriptArg *find_proc( char *name )
{
int i;

  /* Check if procedure is in list */
  for( i = 0 ; i < numusrprocs && strcmp( name, usrprocs[i].procname ) != 0 ; i++ );
  if( i == numusrprocs )
  {
    /* Not in list */
    printf( "<%s> - Procedure not found!\n", name );
    return NULL;
  }

return (usrprocs[i].procbody);
}

void set_procedure( char *name, ScriptArg *cmd )
{
int i;

  /* Check if name is in preset list */
  for( i = 0 ; i < numusrprocs && strcmp( name, usrprocs[i].procname ) != 0 ; i++ );
  if( i != numusrprocs )
  {
    printf( "Procedure <%s> already defined!\n", name );
    exit(-1);
  }
  else
  {
    /* Enlarge list for one additional element */
    numusrprocs++;
    usrprocs = realloc( usrprocs, sizeof(struct ProcedureList) * numusrprocs );
    if( usrprocs == NULL )
    {
      end_malloc();
    }
    usrprocs[i].procbody = cmd;
    usrprocs[i].procname = malloc( strlen( name ) + 1 );
    if( usrprocs[i].procname == NULL )
    {
      end_malloc();
    }
    strcpy( usrprocs[i].procname, name );
  }
}

void free_proclist( )
{
int i; 

  for( i = 0 ; i < numusrprocs ; i++ )
  {
    free( usrprocs[i].procname );
  }
  free( usrprocs );
}

