/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* parse.c -- Here are all functions used to parse the input file */

#include "Installer.h"
#include "parse.h"
#include "cleanup.h"
#include "procedure.h"
#include "gui.h"

/* External variables */
extern char buffer[MAXARGSIZE];
extern BPTR inputfile;
extern InstallerPrefs preferences;


int line;

#ifdef DEBUG
/*
 * Print the sub-tree of ScriptArg *a
 */
void printcode(ScriptArg *a, int x)
{
int i;
    /* Don't produce a SegFault if pointer is non-NULL */
    if ((long int)a > 1000)
    {
	for ( i = 0 ; i < x ; i++ )
	    printf(" ");
	printf("%ld: %s , cmd=%ld, next=%ld, parent=%ld\n",(long int)a,a->arg,(long int)a->cmd,(long int)a->next,(long int)a->parent);
	printcode(a->cmd, x+1);
	printcode(a->next, x);
    }
}
#endif /* DEBUG */


/*
 * Read in file and generate executable tree
 */
void parse_file(ScriptArg *first)
{
ScriptArg *current;
int count, i, j, ready;
char **mclip;

    ready = FALSE;
    current = first;
    do
    {
	count = Read(inputfile, &buffer[0], 1);
	if (count == 0)
	{
	    PrintFault(IoErr(), INSTALLER_NAME);
	    show_parseerror("End of File", line);
	    cleanup();
	    exit(-1);
	}
	if (!isspace(buffer[0]))
	{
	    switch (buffer[0])
	    {
		case SEMICOLON: /* A comment, ok - Go on with next line */
		    do
		    {
			count = Read(inputfile, &buffer[0], 1);
		    } while (buffer[0] != LINEFEED && count != 0);
		    line++;
		    break;

		case LBRACK: /* Open bracket: recurse into next level */
		    current->cmd = AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
		    if (current->cmd == NULL)
		    {
			end_alloc();
		    }
		    /* Set initial values */
		    current->cmd->parent = current;
		    current->cmd->arg = NULL;
		    current->cmd->cmd = NULL;
		    current->cmd->next = NULL;
		    current->cmd->intval = 0;
		    current->cmd->ignore = 0;
		    parse_file(current->cmd);
		    current->next = AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
		    if (current->next == NULL)
		    {
			end_alloc();
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

		case RBRACK: /* All args collected return to lower level */
		    /* We have allocated one ScriptArg too much */
		    current = current->parent->cmd;
		    if (current->next != NULL)
		    {
			while (current->next->next != NULL)
			{
			    current = current->next;
			}
			FreeVec(current->next);
			current->next = NULL;
		    }
		    else
		    {
			/* This is an empty bracket */
			show_parseerror("There is an empty bracket.", line);
			cleanup();
			exit(-1);
		    }
		    ready = TRUE;
		    break;

		default: /* This is the real string */
		    i = 0;
		    if (buffer[0] == DQUOTE || buffer[0] == SQUOTE)
		    {
		    int masquerade = FALSE;
			do
			{
			    if (masquerade == TRUE)
			    {
				switch (buffer[i])
				{
				    case 'n': /* NEWLINE */
					buffer[i-1] = 0x0a;
						  break;
				    case 'r': /* RETURN */
					buffer[i-1] = 0x0d;
					break;
				    case 't': /* TAB */
					buffer[i-1] = 0x09;
					break;
				    case SQUOTE: /* SQUOTE */
					buffer[i-1] = SQUOTE;
					break;
				    case DQUOTE: /* DQUOTE */
					 buffer[i-1] = DQUOTE;
					 break;
				    case BACKSLASH: /* BACKSLASH */
					 buffer[i-1] = BACKSLASH;
					 break;
#warning TODO: convert missing '\\' masqueraded symbols
				    case 'h': /* H.TAB */
				    case 'v': /* V.TAB */
				    case 'b': /* BACKSPACE */
				    case 'f': /* FORMFEED */
				    case 'x': /* HEX number */
				    case 'o': /* \\ooo OCTAL number ('o' is just to remember) */
				    default:
					i++;
					break;
				}
				masquerade = FALSE;
			    }
			    else
			    {
				if (buffer[i] == BACKSLASH)
				    masquerade = TRUE;
				i++;
			    }
			    if (i == MAXARGSIZE)
			    {
				show_parseerror("Argument length overflow!" ,line);
				cleanup();
				exit(-1);
			    }
			    count = Read(inputfile, &buffer[i], 1);
			} while (masquerade || (buffer[i] != buffer[0] && count != 0));
			current->arg = AllocVec(sizeof(char)*(i+2), MEMF_PUBLIC);
			if (current->arg == NULL)
			{
			    end_alloc();
			}
			buffer[i+1] = 0;
			strncpy (current->arg, buffer, i+2);
		    }
		    else
		    {
			do
			{
			    i++;
			    count = Read(inputfile, &buffer[i], 1);
			} while (!isspace(buffer[i]) && buffer[i]!=LBRACK && buffer[i]!=RBRACK && buffer[i]!=SEMICOLON && count != 0 && i < MAXARGSIZE);
			if (buffer[i] == LINEFEED)
			{
			    line++;
			}
			if (i == MAXARGSIZE)
			{
			    show_parseerror("Argument length overflow!", line);
			    cleanup();
			    exit(-1);
			}
			if (buffer[i] == SEMICOLON)
			{
			    do
			    {
				count = Read(inputfile, &buffer[i], 1);
			    } while (buffer[i] != LINEFEED && count != 0);
			    line++;
			}
			if (buffer[i] == LBRACK || buffer[i] == RBRACK)
			{
			    Seek(inputfile, -1 , OFFSET_CURRENT);
			}
			buffer[i] = 0;
			switch (buffer[0])
			{
			    case DOLLAR: /* HEX number */
				current->intval = strtol(&buffer[1], NULL, 16);
					    break;
			    case PERCENT: /* binary number */
				current->intval = strtol(&buffer[1], NULL, 2);
				break;
			    default: /* number or variable */
				if (isdigit(buffer[0]) || ((buffer[0] == PLUS || buffer[0] == MINUS) && isdigit(buffer[1])))
				{
				    current->intval = atol(buffer);
				}
				else
				{
				    current->arg = AllocVec(sizeof(char)*(i+1), MEMF_PUBLIC);
				    if (current->arg == NULL)
				    {
					end_alloc();
				    }
				    strncpy(current->arg, buffer, i+1);
				}
				if (current == current->parent->cmd && strcasecmp(buffer, "procedure") == 0)
				{
				ScriptArg *proc, *uproc;
				int finish;
				    /* Save procedure in ProcedureList */
				    proc = AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
				    if (proc == NULL)
				    {
					end_alloc();
				    }
				    uproc = proc;
				    proc->parent = NULL;
				    proc->next = NULL;
				    proc->arg = NULL;
				    proc->intval = 0;
				    proc->ignore = 0;
				    proc->cmd = AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
				    if (proc->cmd == NULL)
				    {
					end_alloc();
				    }
				    proc->cmd->parent = proc;
				    proc->cmd->next = NULL;
				    proc->cmd->arg = NULL;
				    proc->cmd->intval = 0;
				    proc->cmd->ignore = 0;
				    proc = proc->cmd;
				    proc->cmd = AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
				    if (proc->cmd == NULL)
				    {
					end_alloc();
				    }
				    proc->cmd->parent = proc;
				    proc->cmd->next = NULL;
				    proc->cmd->arg = NULL;
				    proc->cmd->intval = 0;
				    proc->cmd->ignore = 0;
				    proc->cmd->cmd = NULL;
				    /* parse procedure name and args */
				    mclip = NULL;
				    j = 0;
				    finish = FALSE;
				    do
				    {
					/* goto next argument */
					do
					{
					    count = Read(inputfile, &buffer[0], 1);
					    if (buffer[0] == LINEFEED)
					    {
						line++;
					    }
					    if (buffer[0] == RBRACK)
					    {
						if (j > 0)
						{
						    show_parseerror("Procedure has no body!", line);
						}
						else
						{
						    show_parseerror("Procedure has no name!", line);
						}
						cleanup();
						exit(-1);
					    }
					    if (buffer[0] == SQUOTE ||  buffer[0] == DQUOTE)
					    {
						show_parseerror("Procedure has a quoted argument!", line);
						cleanup();
						exit(-1);
					    }
					    if (buffer[0] == SEMICOLON && count != 0)
					    {
						do
						{
						    count = Read(inputfile, &buffer[0], 1);
						} while (buffer[0] != LINEFEED && count != 0);
						line++;
					    }
					} while (isspace(buffer[0]) && count != 0);

					if (buffer[0] != LBRACK)
					{
					    i = 0;
					    /* read in string */
					    do
					    {
						i++;
						count = Read(inputfile, &buffer[i], 1);
					    } while (!isspace(buffer[i]) && buffer[i]!=LBRACK && buffer[i]!=RBRACK && buffer[i]!=SEMICOLON && count != 0 && i < MAXARGSIZE);
					    if (i == MAXARGSIZE)
					    {
						show_parseerror("Argument length overflow!", line);
						cleanup();
						exit(-1);
					    }
					    if (buffer[i] == LINEFEED)
					    {
						line++;
					    }
					    if (buffer[i] == LBRACK || buffer[i] == RBRACK || buffer[i] == SEMICOLON)
					    {
						Seek(inputfile, -1 , OFFSET_CURRENT);
					    }
					    buffer[i] = 0;
					    j++;
					    mclip = ReAllocVec(mclip, sizeof(char *) * j, MEMF_PUBLIC);
					    if (mclip == NULL)
					    {
						end_alloc();
					    }
					    mclip[j-1] = StrDup(buffer);
					    if (mclip[j-1] == NULL)
					    {
						end_alloc();
					    }
					}
					else
					{
					    /* Exit if procedure has no name or name is string/digit or bracket follows */
					    if (j == 0)
					    {
						show_parseerror("Argument to procedure is a command!\n", line);
						cleanup();
						exit(-1);
					    }
					    /* Next string is body-command */
					    finish = TRUE;
					}
				    } while (!finish);
				    /* Procedure body */
				    parse_file(proc->cmd);
				    finish = FALSE;
				    do
				    {
					do
					{
					    count = Read(inputfile, &buffer[0], 1);
					    if (buffer[0] == LINEFEED)
					    {
						line++;
					    }
					} while (isspace(buffer[0]) && count != 0);
					if (buffer[0] == SEMICOLON)
					{
					    do
					    {
						count = Read(inputfile, &buffer[0], 1);
					    } while (buffer[0] != LINEFEED && count != 0);
					    line++;
					}
					else if (buffer[0] == LBRACK)
					{
					    proc->next = AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
					    if (proc->next == NULL)
					    {
						end_alloc();
					    }
					    proc->next->parent = proc->parent;
					    proc->next->next = NULL;
					    proc->next->arg = NULL;
					    proc->next->intval = 0;
					    proc->next->ignore = 0;
					    proc = proc->next;
					    proc->cmd = AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
					    if (proc->cmd == NULL)
					    {
						end_alloc();
					    }
					    proc->cmd->parent = proc;
					    proc->cmd->next = NULL;
					    proc->cmd->arg = NULL;
					    proc->cmd->intval = 0;
					    proc->cmd->ignore = 0;
					    proc->cmd->cmd = NULL;
					    parse_file(proc->cmd);
					}
					else if (buffer[0] == RBRACK)
					{
					    finish = TRUE;
					}
					else
					{
					    show_parseerror("Procedure has rubbish in its body!", line);
					    cleanup();
					    exit(-1);
					}
				    } while (!finish);
				    current->next = AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
				    if (current->next == NULL)
				    {
					end_alloc();
				    }
				    current->next->parent = current->parent;
				    current = current->next;
				    current->arg = mclip[0];
				    current->cmd = NULL;
				    current->next = NULL;
				    current->intval = set_procedure(mclip, j, uproc);
				    current->ignore = 0;
#ifdef DEBUG
	printf("\n\n");
	printcode(uproc, 0);
#endif /* DEBUG */
				    buffer[0] = 0;
				    ready = TRUE;
				}
				if (current->arg == current->parent->cmd->arg && strcasecmp(buffer, "welcome") == 0)
				{
				    preferences.welcome = TRUE;
				}
				break;
			}
		    }
		    current->next = AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
		    if (current->next == NULL)
		    {
			end_alloc();
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
	    if (buffer[0] == LINEFEED)
	    {
		line++;
	    }
	}
	if (count == 0)
	{
	    PrintFault(IoErr(), INSTALLER_NAME);
	    show_parseerror("End of File", line);
	    cleanup();
	    exit(-1);
	}
    } while (!ready);
}

