/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$

    Desc: Installer
    Lang: english
*/

#include "Installer.h"
#include "main.h"

static const char version[] = "$VER: Installer 0.1 (26.6.1998)\n";


FILE *inputfile;
char buffer[MAXARGSIZE];
int error = 0;


/* Prototypes: */
extern void parse_file( ScriptArg * );
extern void execute_script( ScriptArg * , int );
extern void free_script(ScriptArg * );

InstallerPrefs preferences;
ScriptArg script;

int main( int argc, char *argv[] )
{
ScriptArg *currentarg, *dummy;
int nextarg, endoffile, count;

char *filename;

  /* evaluate args with RDArgs(); */
#warning FIXME: evaluate args with RDArgs()

#warning FIXME: distinguish between cli/workbench invocation

#warning FIXME: get real script name instead of static "test.script"
  filename = malloc( 12 * sizeof(char) );
  strcpy( filename, "test.script" );


  /* open script file */
  inputfile = fopen( filename, "r" );
  if( inputfile == NULL )
  {
    PrintFault( IoErr(), "Installer" );
    exit(-1);
  }


  preferences.transcriptfile = NULL;

  endoffile = FALSE;
  script.arg = NULL;
  script.cmd = NULL;
  script.next = NULL;
  script.value = NULL;
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
        printf("Couldn't malloc memory!\n");
        exit(-1);
      }
      currentarg = script.cmd;
      currentarg->parent = &script;
    }
    else
    {
      currentarg->next = (ScriptArg *)malloc( sizeof(ScriptArg) );
      if( currentarg->next == NULL )
      {
        printf("Couldn't malloc memory!\n");
        exit(-1);
      }
      currentarg->next->parent = currentarg->parent;
      currentarg = currentarg->next;
    }
    /* Set initial values */
    currentarg->arg = NULL;
    currentarg->cmd = NULL;
    currentarg->next = NULL;
    currentarg->value = NULL;

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
                              printf("Couldn't malloc memory!\n");
                              exit(-1);
                            }
			    dummy = currentarg->cmd;
                            dummy->parent = currentarg;
			    dummy->arg = NULL;
			    dummy->cmd = NULL;
			    dummy->next = NULL;
			    dummy->value = NULL;
	                    parse_file( currentarg->cmd );
	                    nextarg = TRUE;
	                    break;

	  default	  : /* Plain text is not allowed */
	                    printf( "Script syntax error!\n" );
	                    exit(-1);
	                    break;
        }
      }
    } while( nextarg != TRUE && !endoffile );
  } while( !endoffile );

  fclose( inputfile );

  /* execute parsed script */
  execute_script( &script, 0 );

/*  free_script( script.cmd ); */

return error;
}
