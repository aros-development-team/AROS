/* execute.c -- Here are all functions used to execute the script */
/* #define SDEBUG 1 */
#include "Installer.h"
#include "execute.h"

extern InstallerPrefs preferences;
extern ScriptArg script;

int eval_cmd( char * );

void execute_script( ScriptArg *commands, int level )
{
ScriptArg *current;

#ifndef SDEBUG

int cmd_type;
int slen;
int quiet;

  current = commands;
/* Assume commands->cmd/arg to be first cmd/arg in parentheses */

/* If first one is a (...)-function execute it */
  if( current->cmd != NULL )
  {
    execute_script( current->cmd, level+1 );
/* So next ones are (...)-functions, too: execute them */
    while( current->next != NULL )
    {
      current = current->next;
      if( current->cmd != NULL )
      {
        execute_script( current->cmd, level );
      }
      else
      {
        if( current->arg != NULL )
	{
          printf( "Argument in list of commands!\n" );
	  exit(-1);
	}
      }
    }
  }
  else
  {
    quiet = FALSE;
    cmd_type = eval_cmd( current->arg );
#ifdef DEBUG
    printf( "<%s>\n", current->arg );
#endif
    switch( cmd_type )
    {
      case 0		: /* Unknown command */
                          printf( "Unknown command <%s>\n", current->arg );
			  /* free_script( script.cmd );	*/
                          exit(-1);
                          break;

      case _ABORT	: /* Output all strings, execute onerrors and exit abnormally */
#ifdef DEBUG
                          while( current->next != NULL )
			  {
			    current = current->next;
			    if( current->arg != NULL )
                              printf( "%s\n", current->arg );
			  }
			  /* Execute onerrors		*/
			  /* free_script( script.cmd );	*/
			  exit(-1);
#endif			  
                          break;

      case _EXIT	: /* Output all strings and exit, print "Done with installation" unless (quiet) is given */
#ifdef DEBUG
                          while( current->next != NULL )
			  {
			    current = current->next;
			    if( current->arg != NULL )
                              printf( "%s\n", current->arg );
                            if( current->cmd != NULL )
                              if( eval_cmd(current->cmd->arg) == _QUIET )
                                quiet = TRUE;
			  }
			  if( !quiet )
			    printf("Done with installation!\n");
			  /* free_script( script.cmd );	*/
			  exit(0);
#endif			  
                          break;

      case _TRANSCRIPT	: /* */
#ifdef DEBUG
			  while( current->next != NULL )
			  {
			    current = current->next;
			    if( current->arg != NULL )
                              printf( "%s\n", current->arg );
			  }
#endif			  
                          if( preferences.transcriptfile != NULL )
			  {
			  }
			  break;

      case _STRING	: /* */
#ifdef DEBUG
                          printf( "string <%s>\n", current->arg );
#endif			  
                          slen = strlen(current->arg);
                          current->value = malloc( slen - 1 );
			  if( current->value == NULL)
			  {
			    printf( "Couldn't malloc memory!\n" );
			    exit(-1);
		          }
			  strncpy( current->value, current->arg+1, slen-2 );
			  current->value[slen-2] = 0;
                          break;

      default		: /* Unimplemented command */
                          printf( "Unimplemented command <%s>\n", current->arg );
                          break;
    }
  }

#else /* print the structure of the script */

int i;
current = commands;

  if( current->arg != NULL )
  {
    for( i = 0 ; i < level ; i++ )
      printf(" ");
    switch( current->arg[0] )
    {
      case SQUOTE :
      case DQUOTE :
                    printf("String:%s\n", current->arg);
		    break;
      default :
                    printf("Var/Cmd:%s\n", current->arg);
		    break;
    }
  }
  else
  {
    if( current->cmd != NULL )
    {
      execute_script( current->cmd , level+1 );
    }
    else
    {
      if( current->next != NULL )
      {
        for( i = 0 ; i < level ; i++ )
          printf(" ");
        printf( "int = %d\n", current->intval );
      }
    }
  }
  if( current->next != NULL )
    execute_script( current->next , level );
  else
  printf("\n");

#endif /* !SDEBUG */

}


int eval_cmd( char * argument )
{
int i;
  for( i = 0 ; i < _MAXCOMMAND && strcmp(internal_commands[i].cmdsymbol, argument ) != 0 ; i++ );
  if( i == _MAXCOMMAND )
    return 0;
  else
    return ( internal_commands[i].cmdnumber );
}
