/* execute.c -- Here are all functions used to execute the script */

#include "Installer.h"
#include "execute.h"
#include "cmdlist.h"

/* External variables */
extern InstallerPrefs preferences;
extern int error;

/* External function prototypes */
extern void cleanup();
extern void end_malloc();
extern void *get_variable( char * );
extern char *get_var_arg( char * );
extern long int get_var_int( char * );
extern void set_variable( char *, char *, long int );
#ifdef DEBUG
extern void dump_varlist();
#endif /* DEBUG */
extern ScriptArg *find_proc( char * );
extern void show_abort( char * );
extern void show_complete( long int );
extern void show_exit( char * );
extern void show_working( char * );
extern void final_report();
extern long int request_bool( struct ParameterList * );
extern long int request_number( struct ParameterList * );
extern char *request_string( struct ParameterList * );
extern long int request_choice( struct ParameterList * );
extern char *request_dir( struct ParameterList * );
extern char *request_disk( struct ParameterList * );
extern char *request_file( struct ParameterList * );
extern long int request_options( struct ParameterList * );
extern void request_userlevel( char * );
extern void traperr( char * );

/* Internal function prototypes */
int eval_cmd( char * );
void execute_script( ScriptArg *, int );
char *strip_quotes( char * );
#ifndef LINUX
static void callback( char, char ** );
#endif /* !LINUX */
long int getint( ScriptArg * );
int database_keyword( char * );
char *collect_strings( ScriptArg *, char, int );
struct ParameterList *get_parameters( ScriptArg *, int );
void collect_stringargs( ScriptArg *, int, struct ParameterList * );

#ifndef LINUX
char * callbackstring = NULL, * globalstring = NULL;
#endif /* !LINUX */


/*
  identify first arg with command and next ones as parameters to it
  command has to be keyword or quoted string
  parameters are converted as needed, <cmd> executed
*/
void execute_script( ScriptArg *commands, int level )
{
ScriptArg *current, *dummy = NULL;
struct ParameterList *parameter;
int cmd_type, slen;
long int i, j;
char *clip, **mclip, *string;
void *params;

  current = commands;
  /* Assume commands->cmd/arg to be first cmd/arg in parentheses */

  /* If first one is a (...)-function execute it */
  if( current->cmd != NULL )
  {
    execute_script( current->cmd, level + 1 );
    /* So next ones are (...)-functions, too: execute them */
    while( current->next != NULL )
    {
      current = current->next;
      if( current->cmd != NULL )
      {
        execute_script( current->cmd, level+ 1  );
      }
      else
      {
        error = SCRIPTERROR;
        traperr( "Argument in list of commands!\n" );
        cleanup();
        exit(-1);
      }
    }
    free( current->parent->arg );
    current->parent->arg = NULL;
    current->parent->intval = current->intval;
    if( current->arg != NULL )
    {
      current->parent->arg = strdup( current->arg );
      if( current->parent->arg == NULL )
      {
        end_malloc();
      }
    }
  }
  else
  {
    cmd_type = eval_cmd( current->arg );
#ifdef DEBUG
    printf( "%d - <%s>\n", level, current->arg );
#endif /* DEBUG */
    free( current->parent->arg );
    current->parent->arg = NULL;
    current->parent->intval = 0;
    switch( cmd_type )
    {
      case _UNKNOWN	: /* Unknown command */
                          error = SCRIPTERROR;
                          traperr( "Unknown command !\n" );
                          cleanup();
                          exit(-1);
                          break;

      case _ABORT	: /* Output all strings, execute onerrors and exit abnormally */
                          string = collect_strings( current->next, LINEFEED, level );
                          show_abort( string );
                          free( string );
                          /* Execute onerrors */
                          if( preferences.onerror.cmd != NULL )
                          {
                            execute_script( preferences.onerror.cmd, -99 );
                          }
#ifdef DEBUG
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
                                  error = BADPARAMETER;
                                  traperr( "Division by zero!\n" );
                                  cleanup();
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
                            error = SCRIPTERROR;
                            traperr( "Two arguments required!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      case _CAT		: /* Return concatenated strings */
                          string = collect_strings( current->next, '#', level );
                          /* Add surrounding quotes to string */
                          slen = strlen( string );
                          current->parent->arg = malloc( slen + 3 );
                          if( current->parent->arg == NULL )
                          {
                            end_malloc();
                          }
                          (current->parent->arg)[0] = DQUOTE;
                          strcpy( (current->parent->arg) + 1, string );
                          (current->parent->arg)[slen+1] = DQUOTE;
                          (current->parent->arg)[slen+2] = 0;
                          free( string );
                          break;

      case _COMPLETE	: /* Display how much we have done in percent */
                          if( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            i = getint( current );
                          }
                          else
                          {
                            i = 0;
                          }
#warning FIXME: Do we have to check for percentage in [0,100] ?
                          current->parent->intval = i;
                          show_complete( i );
                          break;

      case _DEBUG	: /* printf() all strings to shell */
                          string = collect_strings( current->next, SPACE, level );
                          if( preferences.debug == TRUE )
                          {
                            printf( "%s\n", string );
                          }
                          /* Set return value */
                          /* Add surrounding quotes to string */
                          slen = ( string == NULL ) ? 0 : strlen( string );
                          clip = malloc( slen + 3 );
                          if( clip == NULL )
                          {
                            end_malloc();
                          }
                          clip[0] = DQUOTE;
                          strcpy( clip + 1, string );
                          clip[slen+1] = DQUOTE;
                          clip[slen+2] = 0;
                          current->parent->arg = clip;
                          free( string );
                          break;

      case _EXIT	: /* Output all strings and exit */
                          /* print summary where app has been installed unless (quiet) is given */
                          parameter = get_parameters( current->next, level );
                          string = collect_strings( current->next, LINEFEED, level );
                          show_exit( string );
                          if( GetPL( parameter, _QUIET ).intval == 0 )
                          {
                            final_report();
                          }
                          free( string );
                          free( parameter );
#ifdef DEBUG
                          dump_varlist();
#endif /* DEBUG */
                          cleanup();
                          exit(0);
                          break;

      case _IF		: /* if 1st arg != 0 execute 2nd cmd else execute optional 3rd cmd */
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
                                current->parent->arg = strdup( current->arg );
                                if( current->parent->arg == NULL )
                                {
                                  end_malloc();
                                }
                              }
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            error = SCRIPTERROR;
                            traperr( "No arguments given!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      case _IN		: /* Return (arg1) bitwise-and with bit numbers given as following args */
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
                            error = SCRIPTERROR;
                            traperr( "No argument given!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      case _PLUS	: /* Sum up all arguments and return that value */
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
                          break;

      case _PROCEDURE	: /* Ignore this keyword, it was parsed in parse.c */
                          break;

      case _SELECT	: /* Return the nth item of arguments, NULL|0 if 0 */
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
                                  current->parent->arg = strdup( current->arg );
                                  if( current->parent->arg == NULL )
                                  {
                                    end_malloc();
                                  }
                                }
                              }
                            }
                          }
                          else
                          {
                            error = SCRIPTERROR;
                            traperr( "Two arguments required!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      case _SYMBOLSET	: /* assign values to variables -- allow strings and commands as variablenames */
                          /* take odd args as names and even as values */
                          if( current->next != NULL )
                          {
                            current = current->next;
                            while( current != NULL && current->next != NULL )
                            {
                              i = current->next->intval;
                              string = NULL;
                              clip = NULL;
                              if( current->cmd != NULL )
                              {
                                execute_script( current->cmd, level + 1 );
                              }
                              if( current->arg == NULL )
                              {
                                /* There is no varname */
                                error = BADPARAMETER;
                                traperr( "Variable name is not a string!\n" );
                                cleanup();
                                exit(-1);
                              }
                              if( current->arg != NULL && ( current->arg[0] == SQUOTE || current->arg[0] == DQUOTE ) )
                              {
                                /* There is a quoted varname */
                                /* Strip off quotes */
                                string = strip_quotes( current->arg );
                              }
                              else
                              {
                                string = strdup( current->arg );
                                if( string == NULL )
                                {
                                  end_malloc();
                                }
                              }
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
                                }
                                else
                                {
                                  /* value is a variable */
                                  clip = strdup( get_var_arg( current->next->arg ) );
                                  if( clip == NULL )
                                  {
                                    end_malloc();
                                  }
                                  i = get_var_int( current->next->arg );
                                }
                              }
                              set_variable( string, clip, i );
                              free( string );
                              free( clip );
                              dummy = current;
                              current = current->next->next;
                            }
                          }
                          /* SET returns the value of the of the last assignment */
                          if( dummy->next->arg != NULL )
                          {
                            if( (dummy->next->arg)[0] == SQUOTE || (dummy->next->arg)[0] == DQUOTE )
                            {
                              dummy->parent->arg = strdup(dummy->next->arg);
                              if( dummy->parent->arg == NULL)
                              {
                                end_malloc();
                              }
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
                                strcpy( (dummy->parent->arg) + 1, clip );
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
                          break;

      case _SYMBOLVAL	: /* return values of variables -- allow strings and commands as variablenames */
                          if( current->next != NULL )
                          {
                            current = current->next;
                            string = NULL;
                            clip = NULL;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            if( current->arg == NULL )
                            {
                              /* There is no varname */
                              error = BADPARAMETER;
                              traperr( "Variable name is not a string!\n" );
                              cleanup();
                              exit(-1);
                            }
                            if( current->arg != NULL && ( current->arg[0] == SQUOTE || current->arg[0] == DQUOTE ) )
                            {
                              /* There is a quoted varname */
                              /* Strip off quotes */
                              string = strip_quotes( current->arg );
                              current->parent->arg = get_var_arg( string );
                              current->parent->intval = get_var_int( string );
                              free( string );
                            }
                            else
                            {
                              current->parent->arg = get_var_arg( current->arg );
                              current->parent->intval = get_var_int( current->arg );
                            }
                          }
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
                                error = BADPARAMETER;
                                traperr( "Expected variablename, found function instead!\n" );
                                cleanup();
                                exit(-1);
                              }
                              if( current->arg == NULL )
                              {
                                /* There is no varname */
                                error = BADPARAMETER;
                                traperr( "Variable name is not a string!\n" );
                                cleanup();
                                exit(-1);
                              }
                              if( current->arg != NULL && ( current->arg[0] == SQUOTE || current->arg[0] == DQUOTE ) )
                              {
                                /* There is a quoted varname */
                                error = BADPARAMETER;
                                traperr( "Expected symbol, found quoted string instead!\n" );
                                cleanup();
                                exit(-1);
                              }
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
                                  free( clip );
                                }
                                else
                                {
                                  /* value is a variable */
                                  set_variable( current->arg, get_var_arg( current->next->arg ), get_var_int( current->next->arg ) );
                                }
                              }
                              else
                              {
                                  set_variable( current->arg, current->next->arg, current->next->intval );
                              }
                              dummy = current;
                              current = current->next->next;
                            }
                          }
                          /* SET returns the value of the of the last assignment */
                          if( dummy->next->arg != NULL )
                          {
                            if( (dummy->next->arg)[0] == SQUOTE || (dummy->next->arg)[0] == DQUOTE )
                            {
                              dummy->parent->arg = strdup(dummy->next->arg);
                              if( dummy->parent->arg == NULL)
                              {
                                end_malloc();
                              }
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
                                strcpy( (dummy->parent->arg) + 1, clip );
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
                          break;

      case _STRLEN	: /* Return the length of the string, 0 for integer argument */
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
                              }
                              else
                              {
                                ((char **)params)[i] = (char *)get_variable( current->arg );
                              }
                            }
                            else
                            {
                              ((char **)params)[i] = (char *)(current->intval);
                            }
                            i++;
                            params = (void *)realloc( params, sizeof(IPTR)*(i+1) );
                            if( params == NULL)
                            {
                              end_malloc();
                            }
                          }
                          /* Call RawDoFmt() with parameter list */
                          /* Store that produced string as return value */
#ifndef LINUX
                          string = malloc( MAXARGSIZE );
                          if( string == NULL )
                          {
                            end_malloc();
                          }
                          callbackstring = string;
                          globalstring = callbackstring;
                          RawDoFmt( clip, params, (VOID_FUNC)&callback, &(globalstring) );
                          string = callbackstring;
#ifdef DEBUG
                          printf( "String = <%s>\n", string );
#endif /* DEBUG */
#else /* !LINUX */
                          string = strdup( clip );
                          if( string == NULL )
                          {
                            end_malloc();
                          }
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
                          slen = strlen( string );
                          clip = malloc( slen + 3 );
                          if( clip == NULL)
                          {
                            end_malloc();
                          }
                          clip[0] = DQUOTE;
                          strcpy( clip + 1, string );
                          clip[slen+1] = DQUOTE;
                          clip[slen+2] = 0;
                          free( string );
                          current->parent->arg = clip;
                          break;

      case _SUBSTR	: /* Return the substring of arg1 starting with arg2+1 character up to arg3 or end if !arg3 */
                          if( current->next != NULL && current->next->next != NULL )
                          {
                            current = current->next;
                            /* Get string */
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            if( current->arg != NULL )
                            {
                              if( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE )
                              {
                                /* Strip off quotes */
                                string = strip_quotes( current->arg );
                              }
                              else
                              {
                                clip = get_var_arg( current->arg );
                                if( clip != NULL )
                                {
                                  string = strdup( clip );
                                  if( string == NULL )
                                  {
                                    end_malloc();
                                  }
                                }
                                else
                                {
                                  string = malloc( MAXARGSIZE );
                                  if( string == NULL )
                                  {
                                    end_malloc();
                                  }
                                  sprintf( string, "%ld", get_var_int( current->arg ) );
                                }
                              }
                            }
                            else
                            {
                              string = malloc( MAXARGSIZE );
                              if( string == NULL )
                              {
                                end_malloc();
                              }
                              sprintf( string, "%ld", current->intval );
                            }
                            current = current->next;
                            /* Get offset */
                            i = getint( current );
                            if( i > 0 )
                            {
                              slen = strlen( string ) - i;
                            }
                            else
                            {
                              free( string );
                              error = BADPARAMETER;
                              traperr( "Negative argument to (substr)!\n" );
                              cleanup();
                              exit(-1);
                            }
                            /* Get number of chars to copy */
                            if( current->next != NULL )
                            {
                              current = current->next;
                              if( current->cmd != NULL )
                              {
                                execute_script( current->cmd, level + 1 );
                              }
                              j = getint( current );
                              if( j < 0 )
                              {
                                j = 0;
                              }
                              slen = j;
                            }
                            else
                            {
                              j = slen;
                            }
                            clip = malloc( slen + 1 );
                            if( clip == NULL )
                            {
                              end_malloc();
                            }
                            strncpy( clip, ( string + i ), j );
                            clip[j] = 0;
                            free( string );
                            current->parent->arg = clip;
                          }
                          else
                          {
                            error = SCRIPTERROR;
                            traperr( "Wrong number of arguments to (substr)!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      case _TIMES	: /* Multiply all arguments and return that value */
                          if( current->next == NULL )
                          {
                            error = SCRIPTERROR;
                            traperr( "No arguments to \"*\" operator!\n" );
                            cleanup();
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
                          break;

      case _TRANSCRIPT	: /* concatenate strings into logfile */
                          string = collect_strings( current->next, 0, level );
                          if( preferences.transcriptfile != NULL )
                          {
                            fprintf( preferences.transcriptstream, "%s\n", string );
                          }
                          /* Add surrounding quotes to string */
                          slen = ( string == NULL ) ? 0 : strlen( string );
                          clip = malloc( slen + 3 );
                          if( clip == NULL )
                          {
                            end_malloc();
                          }
                          clip[0] = DQUOTE;
                          strcpy( clip + 1, string );
                          clip[slen+1] = DQUOTE;
                          clip[slen+2] = 0;
                          current->parent->arg = clip;
                          free( string );
                          break;

      case _UNTIL	: /* execute 2nd cmd until 1st arg != 0 */
                          if( current->next != NULL && current->next->next != NULL )
                          {
                            current = current->next;
                            if( current->next->cmd == NULL )
                            {
                              /* We don't have a block, so what can we execute ??? */
                              traperr( "Until has no command-block!\n" );
                              cleanup();
                              exit(-1);
                            }
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
                                  current->parent->arg = strdup( current->next->arg );
                                  if( current->parent->arg == NULL )
                                  {
                                    end_malloc();
                                  }
                                }
                              }
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            error = SCRIPTERROR;
                            traperr( "Wrong number of arguments to (until)!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      case _USER	: /* Change the current user-level -- Use only do debug scripts */
                          if( current->next != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            i = getint( current );
                            if( i < _NOVICE || i > _EXPERT )
                            {
#warning FIXME: add error message
                              error = BADPARAMETER;
                              traperr( "New user-level not in [Novice|Average|Expert] !\n" );
                              cleanup();
                              exit(-1);
                            }
                            else
                            {
                              set_variable( "@user-level", NULL, i );
                              current->parent->intval = i;
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            error = SCRIPTERROR;
                            traperr( "No argument given!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      case _WELCOME	: /* Display strings instead of "Welcome to the <APPNAME> App installation utility" */
                          string = collect_strings( current->next, SPACE, level );
                          request_userlevel( string );

                          /* Set return value */
                          /* Add surrounding quotes to string */
                          slen = ( string == NULL ) ? 0 : strlen( string );
                          clip = malloc( slen + 3 );
                          if( clip == NULL )
                          {
                            end_malloc();
                          }
                          clip[0] = DQUOTE;
                          strcpy( clip + 1, string );
                          clip[slen+1] = DQUOTE;
                          clip[slen+2] = 0;
                          current->parent->arg = clip;
                          free( string );
                          break;

      case _WHILE	: /* while 1st arg != 0 execute 2nd cmd */
                          if( current->next != NULL && current->next->next != NULL )
                          {
                            current = current->next;
                            if( current->next->cmd == NULL )
                            {
                              /* We don't have a block, so what can we execute ??? */
                              traperr( "While has no command-block!\n" );
                              cleanup();
                              exit(-1);
                            }
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
                                  current->parent->arg = strdup( current->next->arg );
                                  if( current->parent->arg == NULL )
                                  {
                                    end_malloc();
                                  }
                                }
                              }
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            error = SCRIPTERROR;
                            traperr( "No arguments given!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      case _WORKING	: /* Display strings below "Working on Installation" */
                          string = collect_strings( current->next, LINEFEED, level );
                          show_working( string );

                          /* Set return value */
                          /* Add surrounding quotes to string */
                          slen = ( string == NULL ) ? 0 : strlen( string );
                          clip = malloc( slen + 3 );
                          if( clip == NULL )
                          {
                            end_malloc();
                          }
                          clip[0] = DQUOTE;
                          strcpy( clip + 1, string );
                          clip[slen+1] = DQUOTE;
                          clip[slen+2] = 0;
                          current->parent->arg = clip;
                          free( string );
                          break;

      case _DATABASE	: /* Return information on the hardware Installer is running on */
                          if( current->next != NULL )
                          {
                            current = current->next;
                            clip = strip_quotes( current->arg );
                            i = database_keyword( clip );
                            free( clip );
#warning FIXME: compute return values for "database"
                            switch( i )
                            {
                              case _VBLANK :
                                clip = malloc( MAXARGSIZE );
                                if( clip == NULL )
                                {
                                  end_malloc();
                                }
#ifndef LINUX
                                sprintf( clip, "%c%d%c", DQUOTE, SysBase->VBlankFrequency, DQUOTE );
#else /* !LINUX */
                                sprintf( clip, "%c%d%c", DQUOTE, 50, DQUOTE );
#endif /* !LINUX */
#ifdef DEBUG
                                printf( "VBlank = %s\n", clip );
#endif /* DEBUG */
                                current->parent->arg = strdup( clip );
                                if( current->parent->arg == NULL )
                                {
                                  end_malloc();
                                }
                                free( clip );
                                break;

                              case _CPU:
                                break;

                              case _GRAPHICS_MEM:
                                break;

                              case _TOTAL_MEM:
                                break;

                              case _FPU:
                                break;

                              case _CHIPREV:
                                break;

                              default :
                                break;
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            error = SCRIPTERROR;
                            traperr( "No arguments given!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      case _ASKBOOL	: /* Ask user for a boolean */
                          parameter = get_parameters( current->next, level );
                          current->parent->intval = request_bool( parameter );
                          free( parameter );
                          break;

      case _ASKNUMBER	: /* Ask user for a number */
                          parameter = get_parameters( current->next, level );
                          current->parent->intval = request_number( parameter );
                          free( parameter );
                          break;

      case _ASKSTRING	: /* Ask user for a string */
                          parameter = get_parameters( current->next, level );
                          current->parent->arg = request_string( parameter );
                          free( parameter );
                          break;

      case _ASKCHOICE	: /* Ask user to choose one item */
                          parameter = get_parameters( current->next, level );
                          current->parent->intval = request_choice( parameter );
                          free( parameter );
                          break;

      case _ASKDIR	: /* Ask user for a directory */
                          parameter = get_parameters( current->next, level );
                          current->parent->arg = request_dir( parameter );
                          free( parameter );
                          break;

      case _ASKDISK	: /* Ask user to insert a disk */
                          parameter = get_parameters( current->next, level );
                          current->parent->arg = request_disk( parameter );
                          free( parameter );
                          break;

      case _ASKFILE	: /* Ask user for a filename */
                          parameter = get_parameters( current->next, level );
                          current->parent->arg = request_file( parameter );
                          free( parameter );
                          break;

      case _ASKOPTIONS	: /* Ask user to choose multiple items */
                          parameter = get_parameters( current->next, level );
                          current->parent->intval = request_options( parameter );
                          free( parameter );
                          break;

      case _ONERROR	: /* link onerror to preferences */
                          current = current->next;
                          /* reset parent of old onerror statement */
                          dummy = preferences.onerror.cmd;
                          while( dummy != NULL )
                          {
                            dummy->parent = preferences.onerrorparent;
                            dummy = dummy->next;
                          }
                          /* set new onerror statement */
                          preferences.onerror.cmd = current->cmd;
                          preferences.onerrorparent = current;
                          /* set new parent of new onerror statement */
                          dummy = current->cmd;
                          while( dummy != NULL )
                          {
                            dummy->parent = &(preferences.onerror);
                            dummy = dummy->next;
                          }
                          break;

      case _TRAP	: /* link trap to preferences */
                          if( current->next != NULL  && current->next->cmd != NULL )
                          {
                            current = current->next;
                            if( current->cmd != NULL )
                            {
                              execute_script( current->cmd, level + 1 );
                            }
                            i = getint( current ) - 1;
                            current = current->next;
                            /* reset parent of old trap statement */
                            dummy = preferences.trap[i].cmd;
                            while( dummy != NULL )
                            {
                              dummy->parent = preferences.trapparent[i];
                              dummy = dummy->next;
                            }
                            /* set new onerror statement */
                            preferences.trap[i].cmd = current->cmd;
                            preferences.trapparent[i] = current;
                            /* set new parent of new onerror statement */
                            dummy = current->cmd;
                            while( dummy != NULL )
                            {
                              dummy->parent = &(preferences.trap[i]);
                              dummy = dummy->next;
                            }
                          }
                          else
                          {
#warning FIXME: add error message
                            error = SCRIPTERROR;
                            traperr( "Two arguments required!\n" );
                            cleanup();
                            exit(-1);
                          }
                          break;

      /* Here are all unimplemented commands */
      case _COPYFILES	:
      case _COPYLIB	:
      case _DELETE	:
      case _EARLIER	:
      case _EXECUTE	:
      case _EXISTS	:
      case _EXPANDPATH	:
      case _FILEONLY	:
      case _FOREACH	:
      case _GETASSIGN	:
      case _GETDEVICE	:
      case _GETDISKSPACE	:
      case _GETENV	:
      case _GETSIZE	:
      case _GETSUM	:
      case _GETVERSION	:
      case _MAKEASSIGN	:
      case _MAKEDIR	:
      case _MESSAGE	:
      case _PATHONLY	:
      case _PATMATCH	:
      case _PROTECT	:
      case _RENAME	:
      case _REXX	:
      case _RUN		:
      case _STARTUP	:
      case _TACKON	:
      case _TEXTFILE	:
      case _TOOLTYPE	:
                          printf( "Unimplemented command <%s>\n", current->arg );
                          break;

      case _USERDEF	: /* User defined routine */
                          dummy = find_proc( current->arg );
                          execute_script( dummy->cmd, level + 1 );
                          current->parent->intval = dummy->intval;
                          if( dummy->arg != NULL )
                          {
                            current->parent->arg = strdup( dummy->arg );
                            if( current->parent->arg == NULL )
                            {
                              end_malloc();
                            }
                          }
                          break;

      /* Here are all tags, first the ones which have to be executed */
      case _DELOPTS	: /* unset copying/deleting options if we are called global */
                          /* as parameter to a function we have got an ignore=1 before */
                          if( current->parent->ignore == 0)
                          {
                            /* Read in strings */
                            parameter = malloc( sizeof( struct ParameterList ) );
                            if( parameter == NULL )
                            {
                              end_malloc();
                            }
                            collect_stringargs( current, level, parameter );
                            /* Store data in preferences */
                            for( i = 0 ; i < parameter->intval ; i++ )
                            {
                              /* These are mutually exclusive */
                    #warning FIXME: How are (fail-)strings interpreted in "delopts" ?
                              if( strcasecmp( parameter->arg[i], "fail" ) == 0 )
                              {
                              }
                              if( strcasecmp( parameter->arg[i], "nofail" ) == 0 )
                              {
                              }
                              if( strcasecmp( parameter->arg[i], "oknodelete" ) == 0 )
                              {
                              }

                              /* These may be combined in any way */
                              if( strcasecmp( parameter->arg[i], "force" ) == 0 )
                              {
                                preferences.copyflags &= ~COPY_ASKUSER;
                              }
                              if( strcasecmp( parameter->arg[i], "askuser" ) == 0 )
                              {
                                preferences.copyflags &= ~COPY_ASKUSER;
                              }
                            }

                            free( parameter );
                          }
                          break;

      case _OPTIONAL	: /* set copying/deleting options if we are called global */
                          /* as parameter to a function we have got an ignore=1 before */
                          if( current->parent->ignore == 0)
                          {
                            /* Read in strings */
                            parameter = malloc( sizeof( struct ParameterList ) );
                            if( parameter == NULL )
                            {
                              end_malloc();
                            }
                            collect_stringargs( current, level, parameter );
                            /* Store data in preferences */
                            for( i = 0 ; i < parameter->intval ; i++ )
                            {
                              /* These are mutually exclusive */
                              if( strcasecmp( parameter->arg[i], "fail" ) == 0 )
                              {
                                preferences.copyfail &= ~(COPY_FAIL | COPY_NOFAIL | COPY_OKNODELETE );
                                preferences.copyfail |= COPY_FAIL;
                              }
                              if( strcasecmp( parameter->arg[i], "nofail" ) == 0 )
                              {
                                preferences.copyfail &= ~(COPY_FAIL | COPY_NOFAIL | COPY_OKNODELETE );
                                preferences.copyfail |= COPY_NOFAIL;
                              }
                              if( strcasecmp( parameter->arg[i], "oknodelete" ) == 0 )
                              {
                                preferences.copyfail &= ~(COPY_FAIL | COPY_NOFAIL | COPY_OKNODELETE );
                                preferences.copyfail |= COPY_OKNODELETE;
                              }

                              /* These may be combined in any way */
                              if( strcasecmp( parameter->arg[i], "force" ) == 0 )
                              {
                                preferences.copyflags |= COPY_ASKUSER;
                              }
                              if( strcasecmp( parameter->arg[i], "askuser" ) == 0 )
                              {
                                preferences.copyflags |= COPY_ASKUSER;
                              }
                            }

                            free( parameter );
                          }
                          break;

#ifdef DEBUG
      case _ALL		:
      case _APPEND	:
      case _ASSIGNS	:
      case _CHOICES	:
      case _COMMAND	:
      case _CONFIRM	:
      case _DEFAULT	:
      case _DEST	:
      case _DISK	:
      case _FILES	:
      case _FONTS	:
      case _HELP	:
      case _INCLUDE	:
      case _INFOS	:
      case _NEWNAME	:
      case _NEWPATH	:
      case _NOGAUGE	:
      case _NOPOSITION	:
      case _PATTERN	:
      case _PROMPT	:
      case _RANGE	:
      case _SAFE	:
      case _SETDEFAULTTOOL	:
      case _SETSTACK	:
      case _SETTOOLTYPE	:
      case _SOURCE	:
      case _SWAPCOLORS	:
      case _QUIET	:
                          /* We are tags -- we don't want to be executed */
                          current->parent->ignore = 1;
                          break;
#endif /* DEBUG */

      default		:
#ifdef DEBUG
                          /* Hey! Where did you get this number from??? It's invalid -- must be a bug. */
                          printf( "Unknown command ID %d called <%s>!\n", cmd_type, current->arg );
                          cleanup();
                          exit(-1);
#else /* DEBUG */
                          /* We are tags -- we don't want to be executed */
                          current->parent->ignore = 1;
#endif /* DEBUG */
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
    {
      if( find_proc( argument ) != NULL )
      {
        return _USERDEF;
      }
      else
      {
        return _UNKNOWN;
      }
    }
    else
    {
      return internal_commands[i].cmdnumber;
    }
  }
}


#ifndef LINUX
static void callback( char chr, char ** data )
{
static int i = 0, j = 1;
static char * string = NULL;

  if( callbackstring != string )
  {
    string = callbackstring;
    i = 0;
    j = 1;
  }
  i++;
  if( i > MAXARGSIZE )
  {
    j++;
    i = 1;
    callbackstring = realloc( callbackstring, MAXARGSIZE * j );
    if( callbackstring == NULL )
    {
      end_malloc();
    }
    globalstring += ( callbackstring - string );
    string = callbackstring;
  }
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

/*
  Convert data entry to <int>
  <string>s are atol()'d, <cmd>s are *not* executed
*/
long int getint( ScriptArg *argument )
{
long int i;
char * clip;

  if( argument->arg != NULL )
  {
    if( (argument->arg)[0] == SQUOTE || (argument->arg)[0] == DQUOTE )
    {
      /* Strip off quotes */
      clip = strip_quotes( argument->arg );
      i = atol( clip );
      free( clip );
    }
    else
    {
      clip = get_var_arg( argument->arg );
      if( clip != NULL )
      {
        i = atol( clip );
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

/*
  Find out information on hardware
*/
int database_keyword( char *name )
{
  if( strcasecmp( name, "vblank" ) == 0 )
    return _VBLANK;
  if( strcasecmp( name, "cpu" ) == 0 )
    return _CPU;
  if( strcasecmp( name, "graphics-mem" ) == 0 )
    return _GRAPHICS_MEM;
  if( strcasecmp( name, "total-mem" ) == 0 )
    return _TOTAL_MEM;
  if( strcasecmp( name, "fpu" ) == 0 )
    return _FPU;
  if( strcasecmp( name, "chiprev" ) == 0 )
    return _CHIPREV;

return _UNKNOWN;
}

/*
  Concatenate all arguments as a string with separating character
  if character is 0 strings are concatenated without separator
  <int>s are converted to strings, <cmd>s are executed,
  <parameter>s are not considered
*/
char *collect_strings( ScriptArg *current, char separator, int level )
{
char *string = NULL, *clip, *dummy;
int i;

  while( current != NULL )
  {
    if( current->cmd != NULL )
    {
      /* There is a command instead of a value -- execute command */
      execute_script( current->cmd, level + 1 );
    }
    /* Concatenate string unless it was a parameter which will be ignored */
    if( current->ignore == 0 )
    {
      if( current->arg != NULL )
      {
        if( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE )
        {
          /* Strip off quotes */
          clip = strip_quotes( current->arg );
        }
        else
        {
          dummy = get_var_arg( current->arg );
          if( dummy != NULL )
          {
            clip = strdup( dummy );
            if( clip == NULL )
            {
              end_malloc();
            }
          }
          else
          {
            clip = malloc( MAXARGSIZE );
            if( clip == NULL )
            {
              end_malloc();
            }
            sprintf( clip, "%ld", get_var_int( current->arg ) );
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
        sprintf( clip, "%ld", current->intval );
      }
      i = ( string == NULL ) ? 0 : strlen( string );
      string = realloc( string, i + strlen( clip ) + 2 );
      if( string == NULL )
      {
        end_malloc();
      }
      if( i == 0 )
      {
        string[0] = 0;
      }
      else
      {
        string[i] = separator;
        string[i+1] = 0;
      }
      strcat( string, clip );
      free( clip );
    }
    current = current->next;
  }

return string;
}

/*
  args are scanned for known parameters
  the used entry will be set and <int>s and <string>s read in ParameterList
  intval contains number of <string>s
*/
struct ParameterList *get_parameters( ScriptArg *script, int level )
{
struct ParameterList *pl;
ScriptArg *current;
long int i;
int cmd;
char *string, *clip;

  pl = calloc( NUMPARAMS, sizeof( struct ParameterList ) );
  if( pl == NULL )
  {
    end_malloc();
  }
  while( script != NULL )
  {
    /* Check if we have a command as argument */
    if( script->cmd != NULL )
    {
      current = script->cmd->next;
      /* Check if we don't have a block as argument */
      if( script->cmd->arg != NULL )
      {
        /* Check if we have a parameter as command */
        cmd = eval_cmd( script->cmd->arg );
        if( cmd > _PARAMETER && cmd <= ( _PARAMETER + NUMPARAMS ) )
        {
          /* This is a parameter */
          GetPL( pl, cmd ).used = 1;
          if( cmd > ( _PARAMETER + NUMARGPARAMS ) )
          {
            /* This is a boolean parameter */
            GetPL( pl, cmd ).intval = 1;
          }
          else
          {
            /* This parameter may have arguments */
            switch( cmd )
            {
              /* Parameters with args */
              case _APPEND	: /* $ */
              case _CHOICES	: /* $... */
              case _COMMAND	: /* $... */
              case _DELOPTS	: /* $... */
              case _DEST	: /* $ */
              case _HELP	: /* $... */
              case _INCLUDE	: /* $ */
              case _NEWNAME	: /* $ */
              case _OPTIONAL	: /* $... */
              case _PATTERN	: /* $ */
              case _PROMPT	: /* $... */
              case _SETDEFAULTTOOL: /* $ */
              case _SETTOOLTYPE	: /* $ [$] */
              case _SOURCE	: /* $ */
                                  collect_stringargs( current, level, &(GetPL( pl, cmd )) );
                                  break;

              case _CONFIRM	: /* ($->)# */
                                  i = _EXPERT;
                                  if( current != NULL )
                                  {
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
                                        string = strip_quotes( current->arg );
                                        i = atol( string );
                                        free( string );
                                      }
                                      else
                                      {
                                        clip = get_var_arg( current->arg );
                                        if( clip != NULL )
                                        {
                                          i = atol( clip );
                                          if( strcasecmp( clip, "novice" ) == 0 )
                                            i = _NOVICE;
                                          if( strcasecmp( clip, "average" ) == 0 )
                                            i = _AVERAGE;
                                          if( strcasecmp( clip, "expert" ) == 0 )
                                            i = _EXPERT;
                                        }
                                        else
                                        {
                                          i = get_var_int( current->arg );
                                        }
                                      }
                                    }
                                    else
                                    {
                                      i = current->intval;
                                    }
                                  }
                                  if( i < _NOVICE || i > _EXPERT )
                                  {
                                    error = BADPARAMETER;
                                    traperr( "Userlevel out of range!\n" );
                                    cleanup();
                                    exit(-1);
                                  }
                                  GetPL( pl, cmd ).intval = i;
                                  break;

              case _DEFAULT	: /* * */
                                  i = 0;
                                  string = NULL;
                                  if( current != NULL )
                                  {
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
                                        string = strip_quotes( current->arg );
                                      }
                                      else
                                      {
                                        clip = get_var_arg( current->arg );
                                        if( clip != NULL )
                                        {
                                          string = strdup( clip );
                                          if( string == NULL )
                                          {
                                            end_malloc();
                                          }
                                        }
                                        else
                                        {
                                          i = get_var_int( current->arg );
                                        }
                                      }
                                    }
                                    else
                                    {
                                      i = current->intval;
                                    }
                                    GetPL( pl, cmd ).intval = i;
                                    if( string != NULL )
                                    {
                                      GetPL( pl, cmd ).arg = malloc( sizeof( char * ) );
                                      if( GetPL( pl, cmd ).arg == NULL )
                                      {
                                        end_malloc();
                                      }
                                      GetPL( pl, cmd ).arg[0] = string;
                                    }
                                  }
                                  else
                                  {
                                    error = SCRIPTERROR;
                                    traperr( "No argument to (default)!\n" );
                                    cleanup();
                                    exit(-1);
                                  }
                                  break;

              case _RANGE	: /* # # */
                                  i = 0;
                                  if( current != NULL && current->next != NULL )
                                  {
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
                                        string = strip_quotes( current->arg );
                                        i = atol( string );
                                        free( string );
                                      }
                                      else
                                      {
                                        clip = get_var_arg( current->arg );
                                        if( clip != NULL )
                                        {
                                          i = atol( clip );
                                        }
                                        else
                                        {
                                          i = get_var_int( current->arg );
                                        }
                                      }
                                    }
                                    else
                                    {
                                      i = current->intval;
                                    }
                                    GetPL( pl, cmd ).intval = i;
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
                                        string = strip_quotes( current->arg );
                                        i = atol( string );
                                        free( string );
                                      }
                                      else
                                      {
                                        clip = get_var_arg( current->arg );
                                        if( clip != NULL )
                                        {
                                          i = atol( clip );
                                        }
                                        else
                                        {
                                          i = get_var_int( current->arg );
                                        }
                                      }
                                    }
                                    else
                                    {
                                      i = current->intval;
                                    }
                                    GetPL( pl, cmd ).intval2 = i;
                                  }
                                  else
                                  {
                                    error = SCRIPTERROR;
                                    traperr( "Not enough arguments to (range)!\n" );
                                    cleanup();
                                    exit(-1);
                                  }
                                  break;

              case _SETSTACK	: /* # */
                                  i = 0;
                                  if( current != NULL )
                                  {
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
                                        string = strip_quotes( current->arg );
                                        i = atol( string );
                                        free( string );
                                      }
                                      else
                                      {
                                        clip = get_var_arg( current->arg );
                                        if( clip != NULL )
                                        {
                                          i = atol( clip );
                                        }
                                        else
                                        {
                                          i = get_var_int( current->arg );
                                        }
                                      }
                                    }
                                    else
                                    {
                                      i = current->intval;
                                    }
                                    GetPL( pl, cmd ).intval = i;
                                  }
                                  else
                                  {
                                    error = SCRIPTERROR;
                                    traperr( "No argument to (setstack)!\n" );
                                    cleanup();
                                    exit(-1);
                                  }
                                  break;


              default	: /* We do only collect tags -- this is a command */
                          break;
            }
          }
        }
      }
    }
    script = script->next;
  }

return pl;
}

/*
  read <string>s in ParameterList
  <int>s are converted, <cmd>s executed
*/
void collect_stringargs( ScriptArg *current, int level, struct ParameterList *pl )
{
char *string, *clip, **mclip = NULL;
int j = 0;

  while( current != NULL )
  {
    if( current->cmd != NULL )
    {
      /* There is a command instead of a value -- execute command */
      execute_script( current->cmd, level + 1 );
    }
    mclip = (char **)realloc( mclip, sizeof(char *) * (j+1) );
    if( mclip == NULL)
    {
      end_malloc();
    }
    if( current->arg != NULL )
    {
      if( (current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE )
      {
        /* Strip off quotes */
        string = strip_quotes( current->arg );
      }
      else
      {
        clip = get_var_arg( current->arg );
        if( clip != NULL )
        {
          string = strdup( clip );
          if( string == NULL )
          {
            end_malloc();
          }
        }
        else
        {
          clip = malloc( MAXARGSIZE );
          if( clip == NULL )
          {
            end_malloc();
          }
          sprintf( clip, "%ld", get_var_int( current->arg ) );
          string = strdup( clip );
          if( string == NULL)
          {
            end_malloc();
          }
          free( clip );
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
      sprintf( clip, "%ld", current->intval );
      string = strdup( clip );
      if( string == NULL)
      {
        end_malloc();
      }
      free( clip );
    }
    mclip[j] = string;
    j++;
    current = current->next;
  }
  pl->arg = mclip;
  pl->intval = j;
}

