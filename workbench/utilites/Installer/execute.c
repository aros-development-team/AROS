/* execute.c -- Here are all functions used to execute the script */
#include "Installer.h"
#include "execute.h"

/* External variables */
extern InstallerPrefs preferences;

/* External function prototypes */
extern void cleanup();
extern void end_malloc();
extern void *get_variable( char * );
extern char *get_var_arg( char * );
extern int get_var_int( char * );
extern void set_variable( char *, char *, int );
#ifdef DEBUG
extern void dump_varlist();
#endif

/* Internal function prototypes */
int eval_cmd( char * );
void execute_script( ScriptArg *, int );
char *strip_quotes( char * );


void execute_script( ScriptArg *commands, int level )
{
ScriptArg *current, *dummy = NULL;


int cmd_type;
int slen, i, j;
int quiet;
char *clip, **mclip;
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
      case _UNKNOWN	: /* Unknown command */
                          printf( "Unknown command <%s>\n", current->arg );
                          cleanup();
                          exit(-1);
                          break;

      case _ABORT	: /* Output all strings, execute onerrors and exit abnormally */
                          while( current->next != NULL )
                          {
                            current = current->next;
#ifdef DEBUG
                            if( current->arg != NULL )
                              printf( "%s\n", current->arg );
#endif
                          }
                          /* Execute onerrors		*/
#define DOTHIS
#ifdef DOTHIS
#undef DOTHIS
                          dump_varlist();
#endif

                          cleanup();
                          exit(-1);
                          break;

      case _AND		: /* logically AND two arguments */
                          if( current->next != NULL  && current->next->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            if( current->next->cmd != NULL )
                            {
                              execute_script( current->next->cmd, level + 1 );
                            }
                            current->parent->intval = current->intval && current->next->intval;
                          }
                          else
                          {
#warning FIXME: add error message
                            printf( "ERROR!\n" );
                            exit(-1);
                          }
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          break;

      case _COMPLETE	: /* Display how much we have done in percent */
#ifdef DEBUG
                          if( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            if( current->arg != NULL )
                            {
                              if( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE )
                              {
                                /* Strip off quotes */
                                clip = strip_quotes( current->arg );
                                printf( "Done %d%c\n", atoi( clip ), PERCENT );
                                current->parent->intval = atoi( clip );
                                free( clip );
                              }
                              else
                              {
                                clip = get_var_arg( current->arg );
                                if( clip != NULL )
                                {
                                  printf( "Done %d%c\n", atoi( clip ), PERCENT );
                                  current->parent->intval = atoi( clip );
                                }
                                else
                                {
                                  printf( "Done %d%c\n", get_var_int( current->arg ), PERCENT );
                                  current->parent->intval = get_var_int( current->arg );
                                }
                              }
                            }
                            else
                            {
                              printf( "Done %d%c\n", current->intval, PERCENT );
                              current->parent->intval = current->intval;
                            }
                          }
                          else
                          {
                            current->parent->intval = 0;
                          }
                          free( current->parent->arg );
                          current->parent->arg = NULL;
#endif
                          break;

      case _EXIT	: /* Output all strings and exit, print "Done with installation" unless (quiet) is given */
                          while( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
#ifdef DEBUG
                            if( current->arg != NULL )
                              printf( "%s\n", current->arg );
#endif
                            if( current->cmd != NULL )
                              if( eval_cmd( current->cmd->arg ) == _QUIET )
                                quiet = TRUE;
                          }
                          if( !quiet )
                            printf("Done with installation!\n");
#define DOTHIS
#ifdef DOTHIS
#undef DOTHIS
                          dump_varlist();
#endif
                          cleanup();
                          exit(0);
                          break;

      case _PLUS	: /* Sum up all arguments and return that value */
                          current->parent->intval = 0;
                          while( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            if( current->arg != NULL )
                            {
                              if( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE )
                              {
                                /* Strip off quotes */
                                clip = strip_quotes( current->arg );
                                current->parent->intval += atoi( clip );
                                free( clip );
                              }
                              else
                              {
                                clip = get_var_arg( current->arg );
                                if( clip != NULL )
                                {
                                  current->parent->intval += atoi( clip );
                                }
                                else
                                {
                                  current->parent->intval += get_var_int( current->arg );
                                }
                              }
                            }
                            else
                            {
                              current->parent->intval += current->intval;
                            }
                          }
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          break;

      case _SET		: /* assign values to variables */
                          /* take odd args as names and even as values */
                          if( current->next != NULL )
                          {
                            current = current->next;
                            while( current != NULL && current->next != NULL )
                            {
                              if( current->cmd != NULL )
                              {
                                /* There is a command instead of a varname */
#warning FIXME: What to do if cmd but expected varname?
#warning FIXME: Maybe do not allow varnames with quotes, too.
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
                                    clip = strip_quotes( current->next->arg );
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
                            dummy->parent->arg = NULL;
                            if( dummy->next->arg != NULL )
                            {
                              if( (dummy->next->arg)[0] == SQUOTE || (dummy->next->arg)[0] == DQUOTE )
                              {
                                dummy->parent->arg = malloc( strlen(dummy->next->arg) + 1 );
                                if( dummy->parent->arg == NULL)
                                {
                                  end_malloc();
                                }
                                strcpy( dummy->parent->arg, dummy->next->arg );
                              }
                              else
                              {
                                clip = get_var_arg( dummy->next->arg );
                                if( clip )
                                {
                                  /* Add surrounding quotes to string */
                                  slen = strlen( clip );
                                  dummy->parent->arg = malloc(slen+3);
                                  if( dummy->parent->arg == NULL)
                                  {
                                    end_malloc();
                                  }
                                  (dummy->parent->arg)[0] = DQUOTE;
                                  strcpy( (dummy->parent->arg) + 1, get_var_arg(dummy->next->arg) );
                                  (dummy->parent->arg)[slen+1] = DQUOTE;
                                  (dummy->parent->arg)[slen+2] = 0;
                                }
                                dummy->parent->intval = get_var_int( dummy->next->arg );
                              }
                            }
                            else
                            {
                              dummy->parent->intval = dummy->next->intval;
                            }
                          }
                          break;

      case _TRANSCRIPT	: /* concatenate strings into logfile */
                          if( preferences.transcriptfile != NULL )
                          {
                            while( current->next != NULL )
                            {
                              current = current->next;
                              if( current->cmd != NULL )
                              {
                                /* There is a command instead of a value -- execute command */
                                execute_script( current->cmd, level + 1 );
                              }
                              if( current->arg != NULL )
                              {
                                if( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE )
                                {
                                  /* Strip off quotes */
                                  clip = strip_quotes( current->arg );
                                  fprintf( preferences.transcriptstream, "%s ", clip );
                                  free( clip );
                                }
                                else
                                {
                                  clip = get_var_arg( current->arg );
                                  if( clip != NULL )
                                  {
                                    fprintf( preferences.transcriptstream, "%s ", clip );
                                  }
                                  else
                                  {
                                    fprintf( preferences.transcriptstream, "%d ", get_var_int( current->arg ) );
                                  }
                                }
                              }
                              else
                              {
                                fprintf( preferences.transcriptstream, "%d ", current->intval );
                              }
                            }
                            fprintf( preferences.transcriptstream, "\n" );
                          }
                          /* Return last string or all ? */
#warning FIXME: Decide what to do
                          break;

      case _STRING	: /* Call RawDoFmt with string as format and args and return output */

                          /* Prepare base string */
                          /* Strip off quotes */
                          clip = strip_quotes( current->arg );
#ifdef DEBUG
printf( "<%s>", clip );
#endif

                          /* Now get arguments into typeless array (void *params) */
                          params = (void *)malloc(sizeof(IPTR));
                          if( params == NULL)
                          {
                            end_malloc();
                          }
                          ((char **)params)[0] = NULL;
                          mclip = NULL;
                          i = 0;
                          j = 0;
                          while( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              /* There is a command instead of a value -- execute command */
                              execute_script( current->cmd, level + 1 );
                            }
                            if( current->arg != NULL )
                            {
                              if( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE )
                              {
                                /* Strip off quotes */
                                mclip = (char **)realloc( mclip, sizeof(char *) * (j+1) );
                                if( mclip == NULL)
                                {
                                  end_malloc();
                                }
                                mclip[j] = strip_quotes( current->arg );
                                ((char **)params)[i] = mclip[j];
                                j++;
#ifdef DEBUG
printf( ", s=%s", ((char **)params)[i] );
#endif
                              }
                              else
                              {
                                ((char **)params)[i] = (char *)get_variable( current->arg );
#ifdef DEBUG
printf( ", var: s=%s d=%d", ((char **)params)[i], *((int **)params)[i] );
#endif
                              }
                            }
                            else
                            {
                              ((char **)params)[i] = (char *)&(current->intval);
#ifdef DEBUG
printf( ", d=%d", *((int **)params)[i] );
#endif
                            }
                            i++;
                            params = (void *)realloc( params, sizeof(IPTR)*(i+1) );
                            if( params == NULL)
                            {
                              end_malloc();
                            }
                          }
#ifdef DEBUG
printf( "\n" );
#endif
                          /* Call RawDoFmt() with parameter list */
#warning FIXME: Use RawDoFmt() here
                          /* Store that produced string as return value */
                          free( current->parent->arg );
#warning FIXME: Put pointer to string here instead of current->parent->cmd->arg
                          current->parent->arg = malloc( strlen(current->parent->cmd->arg)+1 );
                          if( current->parent->arg == NULL )
                          {
                            end_malloc();
                          }
                          strcpy( current->parent->arg, current->parent->cmd->arg );
                          current->parent->intval = 0;

                          /* Free temporary space */
                          free( clip );
                          if( mclip )
                          {
                            do
                            {
                              free( mclip[--j] );
                            } while ( j != 0 );
                            free( mclip );
                          }
                          break;

      case _WELCOME	: /* Display strings instead of "Welcome to the <APPNAME> App installation utility" */
#ifdef DEBUG
                          while( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              /* There is a command instead of a value -- execute command */
                              execute_script( current->cmd, level + 1 );
                            }
                            if( current->arg != NULL )
                            {
                              if( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE )
                              {
                                /* Strip off quotes */
                                clip = strip_quotes( current->arg );
                                printf( "%s ", clip );
                                free( clip );
                              }
                              else
                              {
                                clip = get_var_arg( current->arg );
                                if( clip != NULL )
                                {
                                  printf( "%s ", clip );
                                }
                                else
                                {
                                  printf( "%d ", get_var_int( current->arg ) );
                                }
                              }
                            }
                            else
                            {
                              printf( "%d ", current->intval );
                            }
                          }
                          printf( "\n" );
                          /* Return last string or all ? */
#warning FIXME: Decide what to do
#endif
                          break;

      case _WORKING	: /* Display strings below "Working on Installation" */
#ifdef DEBUG
                          printf( "Working on Installation\n" );
                          while( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              /* There is a command instead of a value -- execute command */
                              execute_script( current->cmd, level + 1 );
                            }
                            if( current->arg != NULL )
                            {
                              if( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE )
                              {
                                /* Strip off quotes */
                                clip = strip_quotes( current->arg );
                                printf( "%s\n", clip );
                                free( clip );
                              }
                              else
                              {
                                clip = get_var_arg( current->arg );
                                if( clip != NULL )
                                {
                                  printf( "%s\n", clip );
                                }
                                else
                                {
                                  printf( "%d\n", get_var_int( current->arg ) );
                                }
                              }
                            }
                            else
                            {
                              printf( "%d\n", current->intval );
                            }
                          }
                          /* Return last string or all ? */
#warning FIXME: Decide what to do
#endif
                          break;

      default		: /* Unimplemented command */
                          printf( "Unimplemented command <%s>\n", current->arg );
                          break;
    }
  }
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
    for( i = 0 ; i < _MAXCOMMAND && strcasecmp(internal_commands[i].cmdsymbol, argument ) != 0 ; i++ );
    if( i == _MAXCOMMAND )
      return _UNKNOWN;
    else
      return ( internal_commands[i].cmdnumber );
  }
}


char *strip_quotes( char *string )
{
int slen;
char *clip;
  /* Strip off quotes */
  slen = strlen(string);
  clip = (char *)malloc( slen - 1 );
  if( clip == NULL)
  {
    end_malloc();
  }
  strncpy( clip, string+1, slen-2 );
  clip[slen-2] = 0;

return clip;
}


