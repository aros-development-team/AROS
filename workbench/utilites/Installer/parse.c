/* parse.c -- Here are all functions used to parse the input file */

#include "Installer.h"

/* External variables */
extern char buffer[MAXARGSIZE];
extern FILE *inputfile;
extern int error;
extern InstallerPrefs preferences;

/* External function prototypes */
extern void end_malloc();

/* Internal function prototypes */
void parse_file( ScriptArg * );


void parse_file( ScriptArg *first )
{
ScriptArg *current;
int count, i, ready;

  ready = FALSE;
  current = first;
  do
  {
    count = fread( &buffer[0], 1, 1, inputfile );
    if( count == 0 )
    {
      PrintFault( IoErr(), "Installer" );
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
                         parse_file( current->cmd );
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
#warning FIXME: What to do if bracket is empty ?
                         }
                         ready = TRUE;
                         break;

        default        : /* This is the real string */
                         current->cmd = NULL;
                         i = 0;
                         if( buffer[0] == DQUOTE || buffer[0] == SQUOTE )
                         {
                         int masquerade = FALSE;
                           do
                           {
                             if( buffer[i] == BACKSLASH && !masquerade )
                               masquerade = TRUE;
                             else
                               masquerade = FALSE;
                             i++;
                             if( i == MAXARGSIZE )
                             {
                               printf("Argument length overflow!\n");
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
                           if( i == MAXARGSIZE )
                           {
                             printf("Argument length overflow!\n");
                             exit(-1);
                           }
                           if( buffer[i] == SEMICOLON )
                           {
                             do
                             {
                               count = fread( &buffer[i], 1, 1, inputfile );
                             } while( buffer[i] != LINEFEED && count != 0 );
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
                                              current->intval = atoi( buffer );
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
                                            if( strcasecmp( buffer, "welcome" ) == 0 )
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
                         break;
      }
    }
    if( count == 0 )
    {
      PrintFault( IoErr(), "Installer" );
      exit(-1);
    }
  } while( !ready );
}

