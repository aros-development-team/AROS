/* parse.c -- Here are all functions used to parse the input file */

#include "Installer.h"

/* External variables */
extern char buffer[MAXARGSIZE];
extern FILE *inputfile;
extern int error, line;
extern InstallerPrefs preferences;

/* External function prototypes */
extern void end_malloc();
extern void set_procedure( char *, ScriptArg * );
extern void show_parseerror( int );
extern void cleanup();
extern void parseerror( char *, int );

/* Internal function prototypes */
void parse_file( ScriptArg * );


int line;

void parse_file( ScriptArg *first )
{
ScriptArg *current;
int count, i, ready;
char *clip;

  ready = FALSE;
  current = first;
  do
  {
    count = fread( &buffer[0], 1, 1, inputfile );
    if( count == 0 )
    {
      PrintFault( IoErr(), "Installer" );
      show_parseerror( line );
      cleanup();
      exit(-1);
    }
    if( !isspace( buffer[0] ) )
    {
      switch( buffer[0] )
      {
        case SEMICOLON : /* A comment, ok - Go on with next line */
                         do
                         {
                           count = fread( &buffer[0], 1, 1, inputfile );
                         } while( buffer[0] != LINEFEED && count != 0 );
                         line++;
                         break;

        case LBRACK    : /* Open bracket: recurse into next level */
                         current->cmd = (ScriptArg *)malloc( sizeof(ScriptArg) );
                         if( current->cmd == NULL )
                         {
                           end_malloc();
                         }
                         /* Set initial values */
                         current->cmd->parent = current;
                         current->cmd->arg = NULL;
                         current->cmd->cmd = NULL;
                         current->cmd->next = NULL;
                         current->cmd->intval = 0;
                         current->cmd->ignore = 0;
                         parse_file( current->cmd );
                         current->next = (ScriptArg *)malloc( sizeof(ScriptArg) );
                         if( current->next == NULL )
                         {
                           end_malloc();
                         }
                         current->next->parent = current->parent;
                         /* Set initial values */
                         current = current->next;
                         current->arg = NULL;
                         current->cmd = NULL;
                         current->next = NULL;
                         current->intval = 0;
                         current->ignore = 0;
                         break;

        case RBRACK    : /* All args collected return to lower level */
                         /* We have allocated one ScriptArg too much */
                         current = current->parent->cmd;
                         if( current->next != NULL )
                         {
                           while( current->next->next != NULL )
                           {
                             current = current->next;
                           }
                           free( current->next );
                           current->next = NULL;
                         }
                         else
                         {
                           /* This is an empty bracket */
                           parseerror( "There is an empty bracket in line %d.\n", line );
                           cleanup();
                           exit(-1);
                         }
                         ready = TRUE;
                         break;

        default        : /* This is the real string */
                         i = 0;
                         if( buffer[0] == DQUOTE || buffer[0] == SQUOTE )
                         {
                         int masquerade = FALSE;
                           do
                           {
                             if( masquerade )
                             {
#warning TODO: convert "\n" to 0x0a, etc.
                             }
                             if( buffer[i] == BACKSLASH && !masquerade )
                               masquerade = TRUE;
                             else
                               masquerade = FALSE;
                             i++;
                             if( i == MAXARGSIZE )
                             {
                               parseerror( "Argument length overflow in line %d!\n" ,line );
                               cleanup();
                               exit(-1);
                             }
                             count = fread( &buffer[i], 1, 1, inputfile );
                           } while( masquerade || ( buffer[i] != buffer[0] && count != 0 ) );
                           current->arg = (char *)malloc( sizeof(char)*(i+2) );
                           if( current->arg == NULL )
                           {
                             end_malloc();
                           }
                           buffer[i+1] = 0;
                           strncpy( current->arg, buffer, i+2 );
                         }
                         else
                         {
                           do
                           {
                             i++;
                             count = fread( &buffer[i], 1, 1, inputfile );
                           } while( !isspace( buffer[i] ) && buffer[i]!=LBRACK && buffer[i]!=RBRACK && buffer[i]!=SEMICOLON && count != 0 && i < MAXARGSIZE );
                           if( buffer[i] == LINEFEED )
                           {
                             line++;
                           }
                           if( i == MAXARGSIZE )
                           {
                             parseerror( "Argument length overflow in line %d!\n", line );
                             cleanup();
                             exit(-1);
                           }
                           if( buffer[i] == SEMICOLON )
                           {
                             do
                             {
                               count = fread( &buffer[i], 1, 1, inputfile );
                             } while( buffer[i] != LINEFEED && count != 0 );
                             line++;
                           }
                           if( buffer[i] == LBRACK || buffer[i] == RBRACK )
                           {
#warning FIXME: fseek() does not work!
                             fseek(inputfile, -1 , SEEK_CUR );
                           }
                           buffer[i] = 0;
                           switch( buffer[0] )
                           {
                             case DOLLAR  : /* HEX number */
                                            current->intval = strtol( &buffer[1], NULL, 16 );
                                            break;
                             case PERCENT : /* binary number */
                                            current->intval = strtol( &buffer[1], NULL, 2 );
                                            break;
                             default : /* number or variable */
                                            if( isdigit( buffer[0] ) || ( ( buffer[0] == PLUS || buffer[0] == MINUS ) && isdigit( buffer[1] ) ) )
                                            {
                                              current->intval = atol( buffer );
                                            }
                                            else
                                            {
                                              current->arg = (char *)malloc( sizeof(char)*(i+1) );
                                              if( current->arg == NULL )
                                              {
                                                end_malloc();
                                              }
                                              strncpy( current->arg, buffer, i+1 );
                                            }
                                            if( current->arg == current->parent->cmd->arg && strcasecmp( buffer, "procedure" ) == 0 )
                                            {
                                            ScriptArg *proc;
                                              /* Save procedure in ProcedureList */
                                              proc = malloc( sizeof( ScriptArg ) );
                                              if( proc == NULL )
                                              {
                                                end_malloc();
                                              }
                                              proc->parent = NULL;
                                              proc->next = NULL;
                                              proc->arg = NULL;
                                              proc->intval = 0;
                                              proc->ignore = 0;
                                              proc->cmd = malloc( sizeof( ScriptArg ) );
                                              if( proc->cmd == NULL )
                                              {
                                                end_malloc();
                                              }
                                              proc->cmd->parent = proc;
                                              proc->cmd->next = NULL;
                                              proc->cmd->arg = NULL;
                                              proc->cmd->intval = 0;
                                              proc->cmd->ignore = 0;
                                              /* Procedure name */
                                              i = 0;
                                              /* goto 1st argument after keyword "procedure" */
                                              do
                                              {
                                                do
                                                {
                                                  count = fread( &buffer[0], 1, 1, inputfile );
                                                  if( buffer[0] == LINEFEED )
                                                  {
                                                    line++;
                                                  }
                                                  if( buffer[0] == RBRACK )
                                                  {
                                                    parseerror( "Procedure has no name in line %d!\n", line );
                                                    cleanup();
                                                    exit(-1);
                                                  }
                                                } while( isspace( buffer[0] ) && count != 0 );
                                                if( buffer[0] == SEMICOLON && count != 0 )
                                                {
                                                  do
                                                  {
                                                    count = fread( &buffer[0], 1, 1, inputfile );
                                                  } while( buffer[0] != LINEFEED && count != 0 );
                                                  line++;
                                                }
                                                else
                                                {
                                                  i++;
                                                }
                                              } while( i == 0 );
                                              i = 0;
                                              /* read in name */
                                              do
                                              {
                                                i++;
                                                count = fread( &buffer[i], 1, 1, inputfile );
                                              } while( !isspace( buffer[i] ) && buffer[i]!=LBRACK && buffer[i]!=RBRACK && buffer[i]!=SEMICOLON && count != 0 && i < MAXARGSIZE );
                                              if( i == MAXARGSIZE )
                                              {
                                                parseerror( "Argument length overflow in line %d!\n", line );
                                                cleanup();
                                                exit(-1);
                                              }
                                              if( buffer[i] == LINEFEED )
                                              {
                                                line++;
                                              }
                                              if( buffer[i] == SEMICOLON )
                                              {
                                                do
                                                {
                                                  count = fread( &buffer[i], 1, 1, inputfile );
                                                } while( buffer[i] != LINEFEED && count != 0 );
                                                line++;
                                              }
                                              if( buffer[i] == LBRACK || buffer[i] == RBRACK )
                                              {
#warning FIXME: fseek() does not work!
                                                fseek(inputfile, -1 , SEEK_CUR );
                                              }
                                              buffer[i] = 0;
                                              /* Exit if procedure has no name or name is string/digit or bracket follows */
                                              if( buffer[0] == LBRACK || buffer[0] == RBRACK || buffer[0] == SQUOTE || buffer[0] == DQUOTE )
                                              {
                                                clip = malloc( MAXARGSIZE );
                                                if( clip == NULL )
                                                {
                                                  end_malloc();
                                                }
                                                sprintf( clip, "Invalid procedure name <%s> in line %cd!\n", buffer, '%' );
                                                parseerror( clip, line );
                                                free( clip );
                                                cleanup();
                                                exit(-1);
                                              }
                                              clip = strdup( buffer );
                                              if( clip == NULL )
                                              {
                                                end_malloc();
                                              }
                                              /* Procedure body */
                                              parse_file( proc->cmd );
                                              set_procedure( clip, proc->cmd );
                                              buffer[0] = 0;
                                              ready = TRUE;
                                            }
                                            if( current->arg == current->parent->cmd->arg && strcasecmp( buffer, "welcome" ) == 0 )
                                            {
                                              preferences.welcome = TRUE;
                                            }
                                            break;
                           }
                         }
                         current->next = (ScriptArg *)malloc( sizeof(ScriptArg) );
                         if( current->next == NULL )
                         {
                           end_malloc();
                         }
                         current->next->parent = current->parent;
                         current = current->next;
                         /* Set initial values */
                         current->arg = NULL;
                         current->cmd = NULL;
                         current->next = NULL;
                         current->intval = 0;
                         current->ignore = 0;
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
    if( count == 0 )
    {
      PrintFault( IoErr(), "Installer" );
      show_parseerror( line );
      cleanup();
      exit(-1);
    }
  } while( !ready );
}

