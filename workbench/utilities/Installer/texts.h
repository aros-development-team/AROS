#ifndef _TEXTS_H
#define _TEXTS_H

/* Preset @variables */

struct VariableList variables[] =
{
  {"abort-button",	"Abort",	0},
  {"app-name",		NULL,	0},
  {"askchoice-help",	NULL,	0},
  {"askdir-help",	NULL,	0},
  {"askdisk-help",	NULL,	0},
  {"askfile-help",	NULL,	0},
  {"asknumber-help",	NULL,	0},
  {"askoptions-help",	NULL,	0},
  {"askstring-help",	NULL,	0},
  {"copyfiles-help",	NULL,	0},
  {"copylib-help",	NULL,	0},
  {"default-dest",	NULL,	0},
  {"each-name",		NULL,	0},
  {"each-type",		NULL,	0},
  {"error-msg",		NULL,	0},
  {"execute-dir",	NULL,	0},
  {"icon",		NULL,	0},
  {"ioerr",		NULL,	0},
  {"language",		NULL,	0},
  {"makedir-help",	NULL,	0},
  {"pretend",		NULL,	0},
  {"special-msg",	NULL,	0},
  {"startup-help",	NULL,	0},
  {"user-level",	NULL,	0}
};

#define NUMDEFVAR 24

int numlocalvars = 0;
struct VariableList *localvars = NULL;

void *get_variable( char *name )
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

  /* Return Pointer to value */
  if( list[i].vartext == NULL )
    return &(list[i].varinteger);
  else
    return list[i].vartext;
}

char *get_var_arg( char *name )
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

return list[i].vartext;
}

int get_var_int( char *name )
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

return list[i].varinteger;
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
      if( localvars != NULL )
      {
        localvars = realloc( localvars, sizeof(struct VariableList) * numlocalvars );
      }
      else
      {
        localvars = malloc( sizeof(struct VariableList) );
      }
      if( localvars == NULL )
      {
        printf( "Couldn't malloc memory!\n" );
        exit(-1);
      }
      localvars[i].varsymbol = NULL;
      localvars[i].vartext = NULL;
    }
    else
    {
      /* Free space for strings to be replaced in dynamic list */
      free( localvars[i].vartext );
    }
    list = localvars;
  }
  else
  {
    /* Free space for strings to be replaced in preset list */
    free( variables[i].vartext );
  }

  /* Change values in list */

  /* Allocate memory and copy variable name if it does not exist yet */
  
  if( list[i].varsymbol == NULL )
  {
    list[i].varsymbol = malloc( strlen(name) + 1 );
    if( list[i].varsymbol == NULL )
    {
      printf( "Couldn't malloc memory!\n" );
      exit(-1);
    }
    strcpy( list[i].varsymbol, name );
  }

  if( text != NULL )
  {
    /* Allocate memory and copy variable text */
    list[i].vartext = malloc( strlen(text) + 1 );
    if( list[i].vartext == NULL )
    {
      printf( "Couldn't malloc memory!\n" );
      exit(-1);
    }
    strcpy( list[i].vartext, text );
  }

  /* Set integer value */
  list[i].varinteger = intval;

}

void set_preset_variables()
{
#warning FIXME: Use real APPNAME from RDArgs()
  set_variable( "app-name", "DemoApp", 0 );
}

#ifdef DEBUG
void dump_varlist()
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

#endif /* _TEXTS_H */

