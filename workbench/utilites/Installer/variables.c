/* variables.c -- Here are all functions related to variables */

#include "Installer.h"
#include "texts.h"

/* External variables */
extern InstallerPrefs preferences;

/* External function prototypes */
extern void request_userlevel( char * );
extern void outofmem( void * );

/* Internal function prototypes */
void *get_variable( char * );
char *get_var_arg( char * );
long int get_var_int( char * );
void set_variable( char *, char *, long int );
void set_preset_variables();
#ifdef DEBUG
void dump_varlist();
#endif /* DEBUG */
void free_varlist();
struct VariableList *find_var( char * );


int numvariables = 0;
struct VariableList *variables = NULL;

struct VariableList *find_var( char *name )
{
int i;

  /* Check if variable is in list */
  for( i = 0 ; i < numvariables && strcmp( name, variables[i].varsymbol ) != 0 ; i++ );
  return ( i == numvariables ) ?
         NULL:
         &(variables[i]);
}

void *get_variable( char *name )
{
struct VariableList *entry;

  /* Find variable in lists */
  entry = find_var( name );

  /* Return Pointer to value */
  if( entry != NULL )
  {
    return ( entry->vartext == NULL ) ?
           (void *)(entry->varinteger) :
           (void *)entry->vartext;
  }
  else
  {
    return NULL;
  }
}

char *get_var_arg( char *name )
{
struct VariableList *entry;

  /* Find variable in lists */
  entry = find_var( name );

return ( entry != NULL ) ?
       entry->vartext :
       NULL;
}

long int get_var_int( char *name )
{
struct VariableList *entry;

  /* Find variable in lists */
  entry = find_var( name );

return ( entry != NULL ) ?
       entry->varinteger :
       0;
}

void set_variable( char *name, char *text, long int intval )
{
int i;

  /* Check if variable is in list */
  for( i = 0 ; i < numvariables && strcmp( name, variables[i].varsymbol ) != 0 ; i++ );
  if( i == numvariables )
  {
    /* Enlarge list for one additional element */
    numvariables++;
    variables = realloc( variables, sizeof(struct VariableList) * numvariables );
    outofmem( variables );
    variables[i].varsymbol = NULL;
    variables[i].vartext = NULL;
  }
  else
  {
    /* Free space for strings to be replaced in dynamic list */
    free( variables[i].vartext );
    variables[i].vartext = NULL;
  }

  /* Change values in list */

  /* Duplicate variable name if it does not exist yet */
  
  if( variables[i].varsymbol == NULL )
  {
    variables[i].varsymbol = strdup( name );
    outofmem( variables[i].varsymbol );
  }

  /* Duplicate variable text if existent */
  if( text != NULL )
  {
    variables[i].vartext = strdup( text );
    outofmem( variables[i].vartext );
  }

  /* Set integer value */
  variables[i].varinteger = intval;

}

void set_preset_variables( )
{
#warning FIXME: Use real APPNAME, LANGUAGE from RDArgs()
  set_variable( "@app-name", "DemoApp", 0 );
  set_variable( "@language", "english", 0 );

  set_variable( "@abort-button", ABORT_BUTTON, 0 );
  set_variable( "@default-dest", DEFAULT_DEST, 0 );
  set_variable( "@installer-version", NULL, ( INSTALLER_VERSION << 16 ) + INSTALLER_REVISION );
  set_variable( "@user-level", NULL, preferences.defusrlevel );
  if( preferences.welcome == FALSE )
  {
    request_userlevel( NULL );
  }

  /* Set other variables to (NULL|0) */
  set_variable( "@askchoice-help",	NULL,	0 );
  set_variable( "@askdir-help",		NULL,	0 );
  set_variable( "@askdisk-help",	NULL,	0 );
  set_variable( "@askfile-help",	NULL,	0 );
  set_variable( "@asknumber-help",	NULL,	0 );
  set_variable( "@askoptions-help",	NULL,	0 );
  set_variable( "@askstring-help",	NULL,	0 );
  set_variable( "@copyfiles-help",	NULL,	0 );
  set_variable( "@copylib-help",	NULL,	0 );
  set_variable( "@each-name",		NULL,	0 );
  set_variable( "@each-type",		NULL,	0 );
  set_variable( "@error-msg",		NULL,	0 );
  set_variable( "@execute-dir",		NULL,	0 );
  set_variable( "@icon",		NULL,	0 );
  set_variable( "@ioerr",		NULL,	0 );
  set_variable( "@makedir-help",	NULL,	0 );
  set_variable( "@pretend",		NULL,	0 );
  set_variable( "@special-msg",		NULL,	0 );
  set_variable( "@startup-help",	NULL,	0 );
}

#ifdef DEBUG
void dump_varlist( )
{
int i;

  printf( "DUMP of all variables:\n" );
  for( i = 0 ; i < numvariables ; i++ )
    printf( "%s = %s | %ld\n", variables[i].varsymbol, variables[i].vartext, variables[i].varinteger );

}
#endif /* DEBUG */

void free_varlist( )
{
int i; 

  for( i = 0 ; i < numvariables ; i++ )
  {
    free( variables[i].varsymbol );
    free( variables[i].vartext );
  }
  free( variables );
}

