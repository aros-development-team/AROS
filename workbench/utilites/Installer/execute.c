/* execute.c -- Here are all functions used to execute the script */
#define SDEBUG 1
#include "Installer.h"
#include "execute.h"

int eval_cmd( char * );

void execute_script( ScriptArg *commands, int level )
{
ScriptArg *current;

#warning FIXME: implement execution of script
#ifndef SDEBUG

int cmd_type;
int slen;

  current = commands;
/* Assume commands->cmd/arg to be first cmd/arg in parentheses */
/* If first one is a (...)-function first execute it */
  if( current->cmd !=NULL )
  {
    execute_script( current->cmd, level+1 );
  }
  else
  {
    cmd_type = eval_cmd( current->arg );
    switch( cmd_type )
    {
      case CMD_UNKNOWN	: /* */
                          printf( "Unknown command <%s>!", current->arg );
                          exit(-1);
                          break;

      case CMD_STRING	: /* */
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

      default		: /* */
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

return 0;
}
