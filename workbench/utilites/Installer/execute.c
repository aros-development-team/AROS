/* execute.c -- Here are all functions used to execute the script */
/* #define SDEBUG 1 */
#include "Installer.h"
#include "execute.h"
#include "texts.h"

extern InstallerPrefs preferences;
extern ScriptArg script;

int eval_cmd( char * );

void execute_script( ScriptArg *commands, int level )
{
ScriptArg *current, *dummy;

#ifndef SDEBUG

int cmd_type;
int slen;
int quiet;
char *clip;
void *params;

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
    printf( "%d - <%s>\n", level, current->arg );
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

      case _SET		: /* */
                          /* take odd args as names and even as values */
                          if( current->next != NULL )
                          {
                            current = current->next;
                            while( current != NULL && current->next != NULL )
                            {
                              if( current->cmd != NULL )
                              {
                                /* There is a command instead of a varname */
#warning FIXME: What to do if cmd but expected arg?
#warning FIXME: Maybe do not allow varnames with quotes.
                                printf( "Expected symbol, found function instead!\n" );
                                exit(-1);
                              }
                              else
                              {
                                if( current->next->cmd != NULL )
                                {
                                  /* There is a command instead of a value -- execute command */
                                  execute_script( current->next->cmd, level + 1 );
                                }
                                if( current->next->arg != NULL )
                                {
                                  if( (current->next->arg)[0] == SQUOTE || (current->next->arg)[0] == DQUOTE )
                                  {
                                    /* Strip off quotes */
                                    slen = strlen(current->next->arg);
                                    clip = malloc( slen - 1 );
                                    if( clip == NULL)
                                    {
                                      printf( "Couldn't malloc memory!\n" );
                                      exit(-1);
                                    }
                                    strncpy( clip, (current->next->arg)+1, slen-2 );
                                    clip[slen-2] = 0;
                                    set_variable( current->arg, clip, current->next->intval );
#ifdef DEBUG
printf( "%s = %s | %d\n", current->arg, clip, current->next->intval );
#endif
                                    free( clip );
                                  }
                                  else
                                  {
                                    /* value is a variable */
                                    set_variable( current->arg, get_var_arg( current->next->arg ), get_var_int( current->next->arg ) );
#ifdef DEBUG
printf( "%s = %s | %d\n", current->arg, get_var_arg( current->next->arg ), get_var_int( current->next->arg ) );
#endif
                                  }
                                }
                                else
                                {
                                    set_variable( current->arg, current->next->arg, current->next->intval );
#ifdef DEBUG
printf( "%s = %s | %d\n", current->arg, current->next->arg, current->next->intval );
#endif
                                }
                                dummy = current;
                                current = current->next->next;
                              }
                            }
                            /* SET returns the value of the of the last assignment */
                            free( dummy->parent->arg );
                            if( dummy->next->arg != NULL )
                            {
                              if( (dummy->next->arg)[0] == SQUOTE || (dummy->next->arg)[0] == DQUOTE )
                              {
                                dummy->parent->arg = malloc( strlen(dummy->next->arg) + 1 );
                                if( dummy->parent->arg == NULL)
                                {
                                  printf( "Couldn't malloc memory!\n" );
                                  exit(-1);
                                }
                                strcpy( dummy->parent->arg, dummy->next->arg );
                              }
                              else
                              {
                                clip = get_var_arg( dummy->next->arg );
                                if( clip )
                                {
                                  slen = strlen( clip );
                                  dummy->parent->arg = malloc(slen+3);
                                  if( dummy->parent->arg == NULL)
                                  {
                                    printf( "Couldn't malloc memory!\n" );
                                    exit(-1);
                                  }
                                  (dummy->parent->arg)[0] = DQUOTE;
                                  strcpy( (dummy->parent->arg) + 1, get_var_arg(dummy->next->arg) );
                                  (dummy->parent->arg)[slen+1] = DQUOTE;
                                  (dummy->parent->arg)[slen+2] = 0;
                                }
                                else
                                {
                                  dummy->parent->arg = NULL;
                                }
                                dummy->parent->intval = get_var_int( dummy->next->arg );
                              }
                            }
                            else
                            {
                              dummy->parent->arg = NULL;
                              dummy->parent->intval = dummy->next->intval;
                            }
                          }
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

      case _STRING	: /* Call RawDoFmt with string as format and args */
#ifdef DEBUG
                          printf( "string <%s>\n", current->arg );
#endif			  
                          /* Strip off quotes */
                          slen = strlen(current->arg);
                          clip = malloc( slen - 1 );
                          if( clip == NULL)
                          {
                            printf( "Couldn't malloc memory!\n" );
                            exit(-1);
                          }
                          strncpy( clip, current->arg+1, slen-2 );
                          clip[slen-2] = 0;
                          /* Now get arguments into typeless array (void *params) */

                          /* Call RawDoFmt() */

                          /* Free temporary space */
                          free( clip );
                          break;

      default		: /* Unimplemented command */
                          printf( "Unimplemented command <%s>\n", current->arg );
                          break;
    }
  }


#else /* Test things ;-) */

#warning TESTMODE!

#ifdef DOTHIS
/* print the structure of the script */

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
#endif

#define DOTHIS
#ifdef DOTHIS
#undef DOTHIS
/* test variable list */

printf("APPNAME is %s\n", (char *)get_variable( "app-name" ) );

#endif

#endif /* !SDEBUG */

}


int eval_cmd( char * argument )
{
int i;

  if( argument[0] == SQUOTE || argument[0] == DQUOTE )
  {
    return _STRING;
  }
  else
  {
    for( i = 0 ; i < _MAXCOMMAND && strcmp(internal_commands[i].cmdsymbol, argument ) != 0 ; i++ );
    if( i == _MAXCOMMAND )
      return 0;
    else
      return ( internal_commands[i].cmdnumber );
  }
}

