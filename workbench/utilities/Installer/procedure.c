/* procedure.c -- Here are all functions related to user-defined procedures */

#include "Installer.h"
#include "execute.h"

/* External variables */
extern struct CommandList internal_commands[];

/* External function prototypes */
extern void end_malloc();
extern void cleanup();

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
  for( i = 0 ; i < _MAXCOMMAND && strcmp( name, internal_commands[i].cmdsymbol ) != 0 ; i++ );
  if( i < _MAXCOMMAND )
  {
    printf( "Procedure name <%s> already defined for internal function!\n", name );
    cleanup();
    exit(-1);
  }

  /* Check if name is in list */
  for( i = 0 ; i < numusrprocs && strcmp( name, usrprocs[i].procname ) != 0 ; i++ );
  if( i != numusrprocs )
  {
    printf( "Procedure <%s> already defined!\n", name );
    cleanup();
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
    usrprocs[i].procname = name;
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

