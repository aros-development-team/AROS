/* texts.c -- Here are all functions related to variables */

#include "Installer.h"
#include "texts.h"

/* External variables */

/* External function prototypes */
extern void end_malloc();

/* Internal function prototypes */
void *get_variable( char * );
char *get_var_arg( char * );
int get_var_int( char * );
void set_variable( char *, char *, int );
void set_preset_variables();
#ifdef DEBUG
void dump_varlist();
#endif
void free_varlist();
struct VariableList *find_var( char * );

int numlocalvars = 0;
struct VariableList *localvars = NULL;

struct VariableList *find_var( char *name )
{
int i;
struct VariableList *list;

  /* Check if variable is in preset list */
  list = variables;
  for( i = 0 ; i < NUMDEFVAR && strcmp( name, variables[i].varsymbol ) != 0 ; i++ );
  if( i == NUMDEFVAR )
  {
    list = localvars;
    /* Check if variable is in dynamic list */
    for( i = 0 ; i < numlocalvars && strcmp( name, localvars[i].varsymbol ) != 0 ; i++ );
    if( i == numlocalvars )
    {
      /* Not in any list */
      printf( "<%s> - Variable not found!\n", name );
      exit(-1);
    }
  }

return &(list[i]);
}

void *get_variable( char *name )
{
struct VariableList *entry;

  /* Find variable in lists */
  entry = find_var( name );

  /* Return Pointer to value */
  return ( entry->vartext == NULL ) ?
         (void *)&(entry->varinteger) :
         (void *)entry->vartext;
}

char *get_var_arg( char *name )
{
struct VariableList *entry;

  /* Find variable in lists */
  entry = find_var( name );

return entry->vartext;
}

int get_var_int( char *name )
{
struct VariableList *entry;

  /* Find variable in lists */
  entry = find_var( name );

return entry->varinteger;
}

void set_variable( char *name, char *text, int intval )
{
int i;
struct VariableList *list;

  /* Check if variable is in preset list */
  list = variables;
  for( i = 0 ; i < NUMDEFVAR && strcmp( name, variables[i].varsymbol ) != 0 ; i++ );
  if( i == NUMDEFVAR )
  {
    /* Check if variable is in dynamic list */
    for( i = 0 ; i < numlocalvars && strcmp( name, localvars[i].varsymbol ) != 0 ; i++ );
    if( i == numlocalvars )
    {
      /* Enlarge list for one additional element */
      numlocalvars++;
      localvars = realloc( localvars, sizeof(struct VariableList) * numlocalvars );
      if( localvars == NULL )
      {
        end_malloc();
      }
      localvars[i].varsymbol = NULL;
      localvars[i].vartext = NULL;
    }
    else
    {
      /* Free space for strings to be replaced in dynamic list */
      free( localvars[i].vartext );
      localvars[i].vartext = NULL;
    }
    list = localvars;
  }
  else
  {
#warning FIXME: Who can change values of preset variables?
    /* Free space for strings to be replaced in preset list */
    free( variables[i].vartext );
    variables[i].vartext = NULL;
  }

  /* Change values in list */

  /* Allocate memory and copy variable name if it does not exist yet */
  
  if( list[i].varsymbol == NULL )
  {
    list[i].varsymbol = malloc( strlen(name) + 1 );
    if( list[i].varsymbol == NULL )
    {
      end_malloc();
    }
    strcpy( list[i].varsymbol, name );
  }

  if( text != NULL )
  {
    /* Allocate memory and copy variable text */
    list[i].vartext = malloc( strlen(text) + 1 );
    if( list[i].vartext == NULL )
    {
      end_malloc();
    }
    strcpy( list[i].vartext, text );
  }

  /* Set integer value */
  list[i].varinteger = intval;

}

void set_preset_variables( )
{
#warning FIXME: Use real APPNAME from RDArgs()
  set_variable( "@app-name", "DemoApp", 0 );
}

#ifdef DEBUG
void dump_varlist( )
{
int i;

  printf( "DUMP of all variables:\n" );

  printf( "preset:\n" );
  /* Dump variables in preset list */
  for( i = 0 ; i < NUMDEFVAR ; i++ )
    printf( "%s = %s | %d\n", variables[i].varsymbol, variables[i].vartext, variables[i].varinteger );

  printf( "dynamic:\n" );
  /* Dump variables in dynamic list */
  for( i = 0 ; i < numlocalvars ; i++ )
    printf( "%s = %s | %d\n", localvars[i].varsymbol, localvars[i].vartext, localvars[i].varinteger );

}
#endif

void free_varlist( )
{
int i; 

  for( i = 0 ; i < numlocalvars ; i++ )
  {
    free( localvars[i].varsymbol );
    free( localvars[i].vartext );
  }
}

