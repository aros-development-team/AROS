/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Installer V43.3
    Lang: english
*/

#include "Installer.h"
#include "main.h"

static const char version[] = "$VER: Installer 43.3 (21.02.1999)\n";


/* External variables */
extern int line;

/* External function prototypes */
extern void parse_file( ScriptArg * );
extern void execute_script( ScriptArg * , int );
extern void cleanup();
extern void set_preset_variables();
extern void *get_variable( char *name );
extern long int get_var_int( char *name );
extern void set_variable( char *name, char *text, long int intval );
extern void end_malloc();
#ifdef DEBUG
extern void dump_varlist();
#endif /* DEBUG */
extern void show_parseerror( char *, int );
extern void final_report();
extern void init_gui();

/* Internal function prototypes */
int main( int, char ** );


char *filename = NULL;
BPTR inputfile;
char buffer[MAXARGSIZE];
int error = 0, grace_exit = 0;

InstallerPrefs preferences;
ScriptArg script;

IPTR * args[TOTAL_ARGS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/*
 * MAIN
 */
int main( int argc, char *argv[] )
{
struct RDArgs *rda;

ScriptArg *currentarg, *dummy;
int nextarg, endoffile, count;

  /* evaluate args with RDArgs(); */
  rda = ReadArgs( ARG_TEMPLATE, (LONG *)args, NULL );
  if( rda == NULL )
  {
    PrintFault( IoErr(), "Installer" );
    exit(-1);
  }

  /* open script file */
#ifdef DEBUG
  if( args[ARG_SCRIPT] )
  {
    printf( "Using script %s.\n", (STRPTR)args[ARG_SCRIPT] );
    filename = strdup( (STRPTR)args[ARG_SCRIPT] );
  }
  else
  {
    printf( "Using default script.\n" );
    filename = strdup( "test.script" );
  }
#else /* DEBUG */
  filename = strdup( (STRPTR)args[ARG_SCRIPT] );
#endif /* DEBUG */

  inputfile = Open( filename, MODE_OLDFILE );
  if( inputfile == NULL )
  {
    PrintFault( IoErr(), "Installer" );
    exit(-1);
  }

  preferences.welcome = FALSE;
  if( args[ARG_NOLOG] )
  {
    preferences.novicelog = FALSE;
  }
  else
  {
    preferences.novicelog = TRUE;
  }
  preferences.transcriptfile = strdup( ( args[ARG_LOGFILE] ) ? (char *)args[ARG_LOGFILE] : "install_log_file" );
  preferences.transcriptstream = NULL;
  preferences.nopretend = (int)args[ARG_NOPRETEND];
  preferences.pretend = 0;

#ifdef DEBUG
  preferences.debug = TRUE;
#else /* DEBUG */
  preferences.debug = FALSE;
#endif /* DEBUG */

  if( args[ARG_DEFUSER] )
  {
    preferences.defusrlevel = _NOVICE;
    if( strcasecmp( "average", (char *)args[ARG_DEFUSER] ) == 0 )
    {
      preferences.defusrlevel = _AVERAGE;
    }
    else if( strcasecmp( "expert", (char *)args[ARG_DEFUSER] ) == 0 )
    {
      preferences.defusrlevel = _EXPERT;
    }
    else
    {
      preferences.defusrlevel = _NOVICE;
    }
  }
  else
  {
    preferences.defusrlevel = _NOVICE;
  }
  preferences.copyfail = COPY_FAIL;
  preferences.copyflags = 0;

  preferences.onerror.cmd = NULL;
  preferences.onerror.next = NULL;
  preferences.onerror.parent = NULL;
  preferences.onerrorparent = NULL;
  for( count = 0 ; count < NUMERRORS ; count++ )
  {
    dummy = &(preferences.trap[count]);
    dummy->cmd = NULL;
    dummy->next = NULL;
    dummy->parent = NULL;
    preferences.trapparent[count] = NULL;
  }

#warning FIXME: distinguish between cli/workbench invocation

  /* Init GUI -- i.e open empty window */
  init_gui();

  line = 1;

  endoffile = FALSE;
  script.arg = NULL;
  script.cmd = NULL;
  script.next = NULL;
  script.parent = NULL;
  script.intval = 0;
  script.ignore = 0;
  currentarg = script.cmd;
  /* parse script file */
  do
  {
    /* Allocate space for script cmd and save first one to scriptroot */
    if( script.cmd == NULL )
    {
      script.cmd = (ScriptArg *)malloc( sizeof(ScriptArg) );
      if( script.cmd == NULL )
      {
        end_malloc();
      }
      currentarg = script.cmd;
      currentarg->parent = &script;
    }
    else
    {
      currentarg->next = (ScriptArg *)malloc( sizeof(ScriptArg) );
      if( currentarg->next == NULL )
      {
        end_malloc();
      }
      currentarg->next->parent = currentarg->parent;
      currentarg = currentarg->next;
    }
    /* Set initial values */
    currentarg->arg = NULL;
    currentarg->cmd = NULL;
    currentarg->next = NULL;
    currentarg->intval = 0;
    currentarg->ignore = 0;

    nextarg = FALSE;
    do
    {
      count = Read( inputfile, &buffer[0], 1 );
      if( count == 0 )
        endoffile = TRUE;

      if( !isspace( buffer[0] ) && !endoffile )
      {
        /* This is text, is it valid ? */
        switch( buffer[0] )
        {
          case SEMICOLON  : /* A comment, ok - Go on with next line */
                            do
                            {
                              count = Read( inputfile, &buffer[0], 1 );
                            } while( buffer[0] != LINEFEED && count != 0 );
                            line++;
                            if( count == 0 )
                              endoffile = TRUE;
                            break;

          case LBRACK	  : /* A command (...) , parse the content of braces */
                            currentarg->cmd = (ScriptArg *)malloc( sizeof(ScriptArg) );
                            if( currentarg->cmd == NULL )
                            {
                              end_malloc();
                            }
                            dummy = currentarg->cmd;
                            dummy->parent = currentarg;
                            dummy->arg = NULL;
                            dummy->ignore = 0;
                            dummy->intval = 0;
                            dummy->cmd = NULL;
                            dummy->next = NULL;
                            parse_file( currentarg->cmd );
                            nextarg = TRUE;
                            break;

          default	  : /* Plain text or closing bracket is not allowed */
                            Close( inputfile );
                            show_parseerror( "Too many closing brackets!", line );
                            cleanup();
                            exit(-1);
                            break;
        }
      }
      else
      {
        if( buffer[0] == LINEFEED )
        {
          line++;
        }
      }
    } while( nextarg != TRUE && !endoffile );
  } while( !endoffile );

  /* Okay, we (again) have allocated one ScriptArg too much, so get rid of it */
  currentarg = script.cmd;
  if( currentarg->next != NULL )
  {
    while( currentarg->next->next != NULL )
    {
      currentarg = currentarg->next;
    }
    free( currentarg->next );
    currentarg->next = NULL;
  }

  free( filename );
  Close( inputfile );

  if( preferences.transcriptfile != NULL )
  {
    /* open transcript file */
    preferences.transcriptstream = Open( preferences.transcriptfile, MODE_NEWFILE );
    if( preferences.transcriptstream == NULL )
    {
      PrintFault( IoErr(), "Installer" );
      cleanup();
      exit(-1);
    }
  }

  /* Set variables which are not constant */
  set_preset_variables();

  /* Finally free ReadArgs struct (set_preset_variables() needed them) */
  FreeArgs(rda);

  if( get_var_int( "@user-level" ) == _NOVICE )
  {
    preferences.copyflags &= ~(COPY_ASKUSER & preferences.copyflags);
  }
  else
  {
    preferences.copyflags |= COPY_ASKUSER;
  }

  /* execute parsed script */
  execute_script( script.cmd, 0 );

#ifdef DEBUG
  dump_varlist();
#endif /* DEBUG */

  final_report();
  cleanup();

return error;
}

