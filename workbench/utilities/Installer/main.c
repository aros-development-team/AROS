/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$

    Desc: Installer
    Lang: english
*/

#include "Installer.h"
#include "main.h"

static const char version[] = "$VER: Installer 0.1 (18.07.1998)\n";


/* External variables */

/* External function prototypes */
extern void parse_file( ScriptArg * );
extern void execute_script( ScriptArg * , int );
extern void cleanup();
extern void set_preset_variables();
extern void *get_variable( char *name );
extern void set_variable( char *name, char *text, int intval );
extern void end_malloc();
#ifdef DEBUG
extern void dump_varlist();
#endif

/* Internal function prototypes */
int main( int, char ** );


FILE *inputfile;
char buffer[MAXARGSIZE];
int error = 0;

InstallerPrefs preferences;
ScriptArg script;

int main( int argc, char *argv[] )
{
#ifndef LINUX
IPTR args[TOTAL_ARGS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
struct RDArgs *rda;
#endif /* !LINUX */

ScriptArg *currentarg, *dummy;
int nextarg, endoffile, count;

char *filename;

#ifndef LINUX
  /* evaluate args with RDArgs(); */
  rda = ReadArgs( ARG_TEMPLATE, args, NULL );
  if( rda == NULL )
  {
    PrintFault( IoErr(), "Installer" );
    exit(-1);
  }
#warning FIXME: evaluate args with RDArgs()
#endif /* !LINUX */

  preferences.welcome = FALSE;
  preferences.transcriptstream = NULL;
#ifdef DEBUG
  preferences.transcriptfile = "test.transcript";
#else
  preferences.transcriptfile = NULL;
#endif

#warning FIXME: distinguish between cli/workbench invocation

#warning FIXME: get real script name instead of static "test.script"
  filename = malloc( 12 * sizeof(char) );
  strcpy( filename, "test.script" );

  /* Set variables which are not constant */
  set_preset_variables();

  /* open script file */
  inputfile = fopen( filename, "r" );
  if( inputfile == NULL )
  {
    PrintFault( IoErr(), "Installer" );
    exit(-1);
  }

  endoffile = FALSE;
  script.arg = NULL;
  script.cmd = NULL;
  script.next = NULL;
  script.parent = NULL;
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

    nextarg = FALSE;
    do
    {
      count = fread( &buffer[0], 1, 1, inputfile );
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
                              count = fread( &buffer[0], 1, 1, inputfile );
                            } while( buffer[0] != LINEFEED && count != 0 );
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
                            dummy->intval = 0;
                            dummy->cmd = NULL;
                            dummy->next = NULL;
                            parse_file( currentarg->cmd );
                            nextarg = TRUE;
                            break;

          default	  : /* Plain text or closing bracket is not allowed */
                            printf( "Script syntax error!\n" );
                            exit(-1);
                            break;
        }
      }
    } while( nextarg != TRUE && !endoffile );
  } while( !endoffile );

  fclose( inputfile );

  if( preferences.transcriptfile != NULL )
  {
    /* open transcript file */
    preferences.transcriptstream = fopen( preferences.transcriptfile, "w" );
    if( preferences.transcriptstream == NULL )
    {
      PrintFault( IoErr(), "Installer" );
      exit(-1);
    }
  }

  /* execute parsed script */
  if( preferences.welcome == FALSE )
  {
#ifdef DEBUG
    printf( "Welcome to %s App installation utility.\n", (char *)get_variable( "@app-name" ) );
#endif
  }
  execute_script( script.cmd, 0 );

  if( preferences.transcriptstream != NULL )
  {
    fclose( preferences.transcriptstream );
  }

#define DOTHIS
#ifdef DOTHIS
#undef DOTHIS
  dump_varlist();
#endif

 cleanup();

return error;
}

