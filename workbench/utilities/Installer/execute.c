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
#endif /* DEBUG */

/* Internal function prototypes */
int eval_cmd( char * );
void execute_script( ScriptArg *, int );
char *strip_quotes( char * );
#ifndef LINUX
static void callback( char, char ** );
#endif /* !LINUX */
int getint( ScriptArg * );

void execute_script( ScriptArg *commands, int level )
{
ScriptArg *current, *dummy = NULL;

int cmd_type;
int slen, i, j;
int quiet;
char *clip, **mclip, *string;
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
          cleanup();
          printf( "Argument in list of commands!\n" );
          exit(-1);
        }
      }
    }
    free( current->parent->arg );
    current->parent->arg = NULL;
    current->parent->intval = current->intval;
    if( current->arg != NULL )
    {
      current->parent->arg = malloc( sizeof( current->arg ) + 1 );
      if( current->parent->arg == NULL )
      {
        end_malloc();
      }
      strcpy( current->parent->arg, current->arg );
    }
  }
  else
  {
    quiet = FALSE;
    cmd_type = eval_cmd( current->arg );
#ifdef DEBUG
    printf( "%d - <%s>\n", level, current->arg );
#endif /* DEBUG */
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
#endif /* DEBUG */
                          }
                          /* Execute onerrors		*/
#define DOTHIS
#ifdef DOTHIS
#undef DOTHIS
                          dump_varlist();
#endif /* DEBUG */

                          cleanup();
                          exit(-1);
                          break;

      case _AND		: /* logically AND two arguments	*/
      case _BITAND	: /* bitwise AND two arguments		*/
      case _BITOR	: /* bitwise OR two arguments		*/
      case _BITXOR	: /* bitwise XOR two arguments		*/
      case _DIFF	: /* returns 1 if 1st != 2nd else 0	*/
      case _DIV		: /* divide 1st by 2nd intval		*/
      case _EQUAL	: /* returns 1 if 1st = 2nd else 0	*/
      case _LESS	: /* returns 1 if 1st < 2nd else 0	*/
      case _LESSEQ	: /* returns 1 if 1st <= 2nd else 0	*/
      case _MINUS	: /* subtract 2nd from 1st intval	*/
      case _MORE	: /* returns 1 if 1st > 2nd else 0	*/
      case _MOREEQ	: /* returns 1 if 1st >= 2nd else 0	*/
      case _OR		: /* logically OR two arguments		*/
      case _SHIFTLEFT	: /* shift 1st left by 2nd arg bits	*/
      case _SHIFTRGHT	: /* shift 1st right by 2nd arg bits	*/
      case _XOR		: /* logically XOR two arguments	*/
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
                            i = getint( current );
                            current = current->next;
                            j = getint( current );
                            switch( cmd_type )
                            {
                              case _AND :
                                current->parent->intval = i && j;
                                break;
                              case _BITAND :
                                current->parent->intval = i & j;
                                break;
                              case _BITOR :
                                current->parent->intval = i | j;
                                break;
                              case _BITXOR :
                                current->parent->intval = i ^ j;
                                break;
                              case _DIFF :
                                current->parent->intval = ( i != j ) ? 1 : 0;
                                break;
                              case _DIV :
                                if( j == 0 )
                                {
                                  cleanup();
                                  printf( "Division by zero!\n" );
                                  exit(-1);
                                }
                                current->parent->intval = (int)( i / j );
                                break;
                              case _EQUAL :
                                current->parent->intval = ( i == j ) ? 1 : 0;
                                break;
                              case _LESS :
                                current->parent->intval = (i < j ) ? 1 : 0;
                                break;
                              case _LESSEQ :
                                current->parent->intval = ( i <= j ) ? 1 : 0;
                                break;
                              case _MINUS :
                                current->parent->intval = i - j;
                                break;
                              case _MORE :
                                current->parent->intval = ( i > j ) ? 1 : 0;
                                break;
                              case _MOREEQ :
                                current->parent->intval = ( i >= j ) ? 1 : 0;
                                break;
                              case _OR :
                                current->parent->intval = i || j;
                                break;
                              case _SHIFTLEFT :
                                current->parent->intval = i << j;
                                break;
                              case _SHIFTRGHT :
                                current->parent->intval = i >> j;
                                break;
                              case _XOR :
                                current->parent->intval = ( i && !j ) || ( j && !i );
                                break;
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            cleanup();
                            printf( "ERROR!\n" );
                            exit(-1);
                          }
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          break;

      case _CAT		: /* Return concatenated strings */
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          current->parent->intval = 0;
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
                              }
                              else
                              {
                                string = get_var_arg( current->arg );
                                if( string != NULL )
                                {
                                  clip = malloc( strlen( string ) + 1 );
                                  if( clip == NULL )
                                  {
                                    end_malloc();
                                  }
                                  strcpy( clip, string );
                                }
                                else
                                {
                                  clip = malloc( MAXARGSIZE );
                                  if( clip == NULL )
                                  {
                                    end_malloc();
                                  }
                                  sprintf( clip, "%d", get_var_int( current->arg ));
                                }
                              }
                            }
                            else
                            {
                              clip = malloc( MAXARGSIZE );
                              if( clip == NULL )
                              {
                                end_malloc();
                              }
                              sprintf( clip, "%d", current->intval );
                            }
                            slen = current->parent->arg == NULL ?
                                   0 :
                                   strlen( current->parent->arg );
                            current->parent->arg = realloc( current->parent->arg, slen + strlen( clip ) + 1 );
                            if( current->parent->arg == NULL )
                            {
                              end_malloc();
                            }
                            sprintf( &(current->parent->arg[slen]), "%s", clip );
                            free( clip );
                          }
                          /* Add surrounding quotes to string */
                          slen = strlen( current->parent->arg );
                          clip = malloc( slen + 1 );
                          if( clip == NULL )
                          {
                            end_malloc();
                          }
                          strcpy( clip, current->parent->arg );
                          current->parent->arg = realloc( current->parent->arg, slen + 3 );
                          if( current->parent->arg == NULL)
                          {
                            end_malloc();
                          }
                          (current->parent->arg)[0] = DQUOTE;
                          strcpy( (current->parent->arg) + 1, clip );
                          (current->parent->arg)[slen+1] = DQUOTE;
                          (current->parent->arg)[slen+2] = 0;
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
#endif /* DEBUG */
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
#endif /* DEBUG */
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
#endif /* DOTHIS */
                          cleanup();
                          exit(0);
                          break;

      case _IF		: /* if 1st arg != 0 execute 2nd cmd else execute optional 3rd cmd */
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          current->parent->intval = 0;
                          if( current->next != NULL && current->next->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            i = getint( current );
                            if( i == 0 )
                            {
                              current = current->next;
                            }
                            if( current->next != NULL )
                            {
                              current = current->next;
                              if( current->cmd != NULL )
                              {
                                execute_script( current->cmd, level + 1 );
                              }
                              current->parent->intval = current->intval;
                              if( current->arg != NULL )
                              {
                                current->parent->arg = malloc( sizeof( current->arg ) + 1 );
                                if( current->parent->arg == NULL )
                                {
                                  end_malloc();
                                }
                                strcpy( current->parent->arg, current->arg );
                              }
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            cleanup();
                            printf( "No arguments given!\n" );
                            exit(-1);
                          }
                          break;

      case _IN		: /* Sum up all arguments and return that value */
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          current->parent->intval = 0;
                          /* Get base integer into i */
                          if( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            i = getint( current );
                          }
                          /* Write the corresponding bits of i into parent */
                          while( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            j = getint( current );
                            current->parent->intval |= i & ( 1 << j );
                          }
                          break;

      case _BITNOT	: /* bitwise invert argument */
      case _NOT		: /* logically invert argument */
                          if( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            i = getint( current );
                            current->parent->intval = ( cmd_type == _NOT ) ? !i : ~i;
                          }
                          else
                          {
#warning FIXME: add error message
                            cleanup();
                            printf( "No argument given!\n" );
                            exit(-1);
                          }
                          free( current->parent->arg );
                          current->parent->arg = NULL;
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
                            i = getint( current );
                            current->parent->intval += i;
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
                                cleanup();
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
#endif /* DEBUG */
                                    free( clip );
                                  }
                                  else
                                  {
                                    /* value is a variable */
                                    set_variable( current->arg, get_var_arg( current->next->arg ), get_var_int( current->next->arg ) );
#ifdef DEBUG
printf( "%s = %s | %d\n", current->arg, get_var_arg( current->next->arg ), get_var_int( current->next->arg ) );
#endif /* DEBUG */
                                  }
                                }
                                else
                                {
                                    set_variable( current->arg, current->next->arg, current->next->intval );
#ifdef DEBUG
printf( "%s = %s | %d\n", current->arg, current->next->arg, current->next->intval );
#endif /* DEBUG */
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
                                  dummy->parent->arg = malloc( slen + 3 );
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

      case _SELECT	: /* Return the nth item of arguments, NULL|0 if 0 */
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          current->parent->intval = 0;
                          if( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            i = getint( current );
                            if( i > 0 )
                            {
                              j = 0;
                              for( ; i > 0 ; i-- )
                              {
                                if( current->next != NULL )
                                {
                                  current = current->next;
                                }
                                else
                                {
                                  j = 1;
                                }
                              }
                              if( j == 0 )
                              {
                                current->parent->intval = current->intval;
                                if( current->arg != NULL )
                                {
                                  current->parent->arg = malloc( strlen( current->arg ) + 1 );
                                  if( current->parent->arg == NULL )
                                  {
                                    end_malloc();
                                  }
                                  strcpy( current->parent->arg, current->arg );
                                }
                              }
                            }
                          }
                          else
                          {
                            cleanup();
                            printf( "ERROR!\n" );
                            exit(-1);
                          }
                          break;

      case _STRLEN	: /* Return the length of the string */
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          current->parent->intval = 0;
                          if( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            if( current->arg != NULL )
                            {
                              current->parent->intval = ( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE ) ?
                                                        strlen( current->arg ) - 2 :
                                                        strlen( get_var_arg( current->arg ) );
                            }
                          }
                          break;

      case _STRING	: /* Call RawDoFmt with string as format and args and return output */

                          /* Prepare base string */
                          /* Strip off quotes */
                          clip = strip_quotes( current->arg );
#ifdef DEBUG
printf( "<%s>", clip );
#endif /* DEBUG */

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
#endif /* DEBUG */
                              }
                              else
                              {
                                ((char **)params)[i] = (char *)get_variable( current->arg );
#ifdef DEBUG
printf( ", var: s=%s d=%d", ((char **)params)[i], *((int **)params)[i] );
#endif /* DEBUG */
                              }
                            }
                            else
                            {
                              ((char **)params)[i] = (char *)&(current->intval);
#ifdef DEBUG
printf( ", d=%d", *((int **)params)[i] );
#endif /* DEBUG */
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
#endif /* DEBUG */
                          /* Call RawDoFmt() with parameter list */
                          /* Store that produced string as return value */
                          free( current->parent->arg );
#ifndef LINUX
                          current->parent->arg = malloc( MAXARGSIZE );
                          if( current->parent->arg == NULL )
                          {
                            end_malloc();
                          }
                          string = current->parent->arg;
                          RawDoFmt( clip, params, (VOID_FUNC)&callback, &(current->parent->arg) );
                          current->parent->arg = string;
#ifdef DEBUG
                          printf( "String = <%s>\n", current->parent->arg );
#endif
#else /* !LINUX */
                          current->parent->arg = malloc( strlen( clip ) );
                          if( current->parent->arg == NULL )
                          {
                            end_malloc();
                          }
                          strcpy( current->parent->arg, clip );
#endif /* !LINUX */
                          /* Free temporary space */
                          free( clip );
                          if( mclip )
                          {
                            while( j > 0 )
                            {
                              free( mclip[--j] );
                            }
                            free( mclip );
                          }

                          /* Add surrounding quotes to string */
                          slen = strlen( current->parent->arg );
                          clip = malloc(slen + 3);
                          if( clip == NULL)
                          {
                            end_malloc();
                          }
                          clip[0] = DQUOTE;
                          strcpy( (clip) + 1, (current->parent->arg) );
                          clip[slen+1] = DQUOTE;
                          clip[slen+2] = 0;
                          free( current->parent->arg );
                          current->parent->arg = clip;
                          current->parent->intval = 0;

                          break;

      case _TIMES	: /* Multiply all arguments and return that value */
                          if( current->next == NULL )
                          {
                            cleanup();
                            printf( "No arguments to \"*\" operator!\n" );
                            exit(-1);
                          }
                          current->parent->intval = 1;
                          while( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            i = getint( current );
                            current->parent->intval *= i;
                          }
                          free( current->parent->arg );
                          current->parent->arg = NULL;
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

      case _UNTIL	: /* execute 2nd cmd until 1st arg != 0 */
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          current->parent->intval = 0;
                          if( current->next != NULL && current->next->next != NULL )
                          {
                            current = current->next;
                            i = 0;
                            while( i == 0 )
                            {
                              /* Execute command */
                              if( current->next->cmd != NULL )
                              {
                                execute_script( current->next->cmd, level + 1 );
                              }

                              /* Now check condition */
                              if( current->cmd != NULL )
                              {
                                execute_script( current->cmd, level + 1 );
                              }
                              i = getint( current );

                              /* condition is true -> return values and exit */
                              if( i != 0 )
                              {
                                current->parent->intval = current->next->intval;
                                if( current->next->arg != NULL )
                                {
                                  current->parent->arg = malloc( sizeof( current->next->arg ) + 1 );
                                  if( current->parent->arg == NULL )
                                  {
                                    end_malloc();
                                  }
                                  strcpy( current->parent->arg, current->next->arg );
                                }
                              }
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            cleanup();
                            printf( "No arguments given!\n" );
                            exit(-1);
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
#endif /* DEBUG */
                          break;

      case _WHILE	: /* while 1st arg != 0 execute 2nd cmd */
                          free( current->parent->arg );
                          current->parent->arg = NULL;
                          current->parent->intval = 0;
                          if( current->next != NULL && current->next->next != NULL )
                          {
                            current = current->next;
                            i = 1;
                            while( i != 0 )
                            {
                              if( current->cmd != NULL )
                              {
                                execute_script( current->cmd, level + 1 );
                              }

                              /* Now check condition */
                              i = getint( current );
                              if( i != 0 )
                              {
                                if( current->next->cmd != NULL )
                                {
                                  execute_script( current->next->cmd, level + 1 );
                                }
                              }
                              else
                              {
                                current->parent->intval = current->next->intval;
                                if( current->next->arg != NULL )
                                {
                                  current->parent->arg = malloc( sizeof( current->next->arg ) + 1 );
                                  if( current->parent->arg == NULL )
                                  {
                                    end_malloc();
                                  }
                                  strcpy( current->parent->arg, current->next->arg );
                                }
                              }
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            cleanup();
                            printf( "No arguments given!\n" );
                            exit(-1);
                          }
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
#endif /* DEBUG */
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
      return ( i == _MAXCOMMAND ) ?
             _UNKNOWN :
             internal_commands[i].cmdnumber;
  }
}


#ifndef LINUX
static void callback( char chr, char ** data )
{
  *(*data)++ = chr;
}
#endif /* !LINUX */


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


int getint( ScriptArg *argument )
{
int i;
char * clip;

  if( argument->arg != NULL )
  {
    if( (argument->arg)[0] == SQUOTE || (argument->arg)[0] == DQUOTE )
    {
      /* Strip off quotes */
      clip = strip_quotes( argument->arg );
      i = atoi( clip );
      free( clip );
    }
    else
    {
      clip = get_var_arg( argument->arg );
      if( clip != NULL )
      {
        i = atoi( clip );
      }
      else
      {
        i = get_var_int( argument->arg );
      }
    }
  }
  else
  {
    i = argument->intval;
  }

return i;
}


