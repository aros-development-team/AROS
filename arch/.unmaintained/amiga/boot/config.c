/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amiga bootloader -- config file routines
    Lang: C
*/

#define AROS_ALMOST_COMPATIBLE /* For GetHead/GetSucc macros */
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>

#include <ctype.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>

#include "boot.h"
#include "main.h"
#include "config.h"

#define D(x) if (debug) x
#define bug Printf

/*
 * Maximum config file length. Is actually 100, but we add 2 for the newline
 * character and the \0 character. Add another character for the V36/V37 FGets
 * bug. All of this still implies a maximum line length of 100 to the user.
 */
#define MAX_LINE_LEN 103

char txt_module[] = "MODULE";
char args_module[] = "MODULE/K/A";
#define ARG_MOD_MODULE 0
#define ARG_MOD_END    1
LONG vec_module[ARG_MOD_END];

char txt_function[] = "FUNCTION";
char args_function[] = "FUNCTION/N/K/A,MODULE/A,ON/S,OFF/S";
#define ARG_FUNC_FUNCTION   0
#define ARG_FUNC_MODULE     1
#define ARG_FUNC_ON         2
#define ARG_FUNC_OFF        3
#define ARG_FUNC_END        4
LONG vec_function[ARG_FUNC_END];

BOOL isconfigline(char *, char *);
struct Node *FindNameNC(struct List *, UBYTE *);

struct BootConfig *ReadConfig(char *file)
{
    BPTR fh;
    LONG ioerr = 0;
    BOOL running = TRUE;
    struct RDArgs *rdargs;
    struct BootConfig *config;
    struct ModNode *modnode;
    struct FuncNode *funcnode;
    UBYTE *linebuffer;

    D(bug("Processing config file\n"));

    D(bug("Allocating Config..."));
    if( (config = AllocVec(sizeof(*config), MEMF_CLEAR)))
    {
	D(bug(" ok\n"));
	NewList(&config->bc_Modules);
	config->bc_Num = 0;

	D(bug("Allocating RDArgs..."));
	if( (rdargs = AllocDosObject(DOS_RDARGS, NULL)))
	{
	    D(bug(" ok\n"));
	    D(bug("Allocating linebuffer..."));
	    if( (linebuffer = AllocMem(MAX_LINE_LEN, MEMF_CLEAR)))
	    {
		D(bug(" ok\n"));

		D(bug("Opening config file \"%s\"", (ULONG)file));
		if( (fh = Open(file, MODE_OLDFILE)))
		{
		    D(bug(" ok\n"));

		    while(running)
		    {
			/* MAX_LINE_LEN-1 because of V36/V37 bug */
			if( (FGets(fh, linebuffer, MAX_LINE_LEN-1)))
			{
			    rdargs->RDA_Source.CS_Buffer = linebuffer;
			    rdargs->RDA_Source.CS_Length = MAX_LINE_LEN;
			    rdargs->RDA_Source.CS_CurChr = 0;
			    rdargs->RDA_Buffer = NULL;

			    if( (isconfigline(linebuffer, txt_module)))
			    {
				vec_module[0] = NULL;

				D(bug("About to ReadArgs()..."));
				if( (ReadArgs(args_module, vec_module, rdargs)))
				{
				    D(bug(" ok\n"));

				    D(bug("Found module: \"%s\"\n", vec_module[ARG_MOD_MODULE]));

				    D(bug("Looking if module already included..."));
				    if(!(FindNameNC(&config->bc_Modules,
					(STRPTR)vec_module[ARG_MOD_MODULE])))
				    {
					D(bug(" no\n"));

					D(bug("Allocating module node..."));
					if( (modnode = AllocVec(sizeof(*modnode), MEMF_CLEAR)))
					{
					    D(bug(" ok\n"));

					    NewList(&modnode->mn_FuncList);

					    D(bug("Allocating space for module name..."));
					    if( (modnode->mn_Node.ln_Name =
						AllocVec(strlen((char *)vec_module[ARG_MOD_MODULE])+1,
						MEMF_CLEAR)))
					    {
						D(bug(" ok\n"));

						strcpy(modnode->mn_Node.ln_Name, (char *)vec_module[ARG_MOD_MODULE]);
						D(bug("Copied name \"%s\"\n", (ULONG)modnode->mn_Node.ln_Name));

						D(bug("Adding node to list\n"));
						AddTail(&config->bc_Modules, &modnode->mn_Node);
						config->bc_Num++;
					    }
					    else
					    {
						FreeVec(modnode);
						ioerr = ERROR_NO_FREE_STORE;
						D(bug(" fail\n"));
					    }
					}
					else
					{
					    ioerr = ERROR_NO_FREE_STORE;
					    D(bug(" fail\n"));
					}
				    }
				    else
				    {
					D(bug(" yes\n"));
					if(!quiet)
					    Printf("Module \"%s\" declared twice. Ignored second occurence.\n",
						(ULONG)vec_module[ARG_MOD_MODULE]
					    );
				    }

				    FreeArgs(rdargs);
				}
				else
				{
				    ioerr = IoErr();
				    D(bug(" fail or incomplete parameters\n"));
				}
			    }
			    else if( (isconfigline(linebuffer, txt_function)))
			    {
				vec_module[ARG_FUNC_FUNCTION] =
				vec_module[ARG_FUNC_MODULE]   =
				vec_module[ARG_FUNC_OFF]      = 0;
				vec_module[ARG_FUNC_ON]       = 1;

				D(bug("About to ReadArgs()..."));
				if( (ReadArgs(args_function, vec_function, rdargs)))
				{
				    D(bug(" ok\n"));

				    /* ON has precedence */
				    if(vec_function[ARG_FUNC_ON] & vec_function[ARG_FUNC_OFF])
					vec_function[ARG_FUNC_OFF] = 0;

				    /* Both flags off: ON is default */
				    if(!vec_function[ARG_FUNC_ON] & !vec_function[ARG_FUNC_OFF])
					vec_function[ARG_FUNC_ON] = 1;

				    D(bug("Found function: \"%ld\" in \"%s\" status \"%s\"\n",
					*(LONG *)vec_function[ARG_FUNC_FUNCTION],
					vec_function[ARG_FUNC_MODULE],
					vec_function[ARG_FUNC_ON] ? (ULONG)"on" : (ULONG)"off"
				    ));

				    if( *(LONG *)vec_function[ARG_FUNC_FUNCTION] % 6 )
				    {
					if(!quiet)
					    Printf("Function offset %ld is not a multiple of -6! Ignoring this function line.\n",
						*(LONG *)vec_function[ARG_FUNC_FUNCTION]
					    );
				    }
				    else
				    {
					D(bug("Finding correct module..."));
					if( (modnode = (struct ModNode *)FindNameNC(&config->bc_Modules,
					    (STRPTR)vec_function[ARG_FUNC_MODULE])))
					{
					    D(bug(" ok\n"));

					    D(bug("Allocating function node..."));
					    if( (funcnode = AllocVec(sizeof(*funcnode), MEMF_CLEAR)))
					    {
						D(bug(" ok\n"));

						D(bug("Setting function status in function node..."));
						funcnode->fn_Slot = *(LONG *)vec_function[ARG_FUNC_FUNCTION] / -6;
						funcnode->fn_Status =
						vec_function[ARG_FUNC_ON] ? 1 : 0;
						D(bug(" ok\n"));

						D(bug("Adding function node to list..."));
						AddTail(&modnode->mn_FuncList, &funcnode->fn_Node);
						D(bug(" ok\n"));
					    }
					    else
					    {
						ioerr = ERROR_NO_FREE_STORE;
						D(bug(" fail\n"));
					    }
					}
					else
					{
					    if(!quiet)
						Printf("Function %ld \"%s\" not in any known module. Ignored.\n",
						    *(LONG *)vec_function[ARG_FUNC_FUNCTION],
						    vec_function[ARG_FUNC_MODULE]
						);
					}
				    }

				    FreeArgs(rdargs);
				}
				else
				{
				    ioerr = IoErr();
				    D(bug(" fail or incomplete parameters\n"));
				}
			    }
			}
			else
			{
			    ioerr = IoErr(); /* will be 0 in case of EOF */
			    running = FALSE;
			}
		    }

		    D(bug("Closing file \"%s\"...", (ULONG)file));
		    if(!(Close(fh)))
		    {
			ioerr = IoErr();
			D(bug(" fail\n"));
		    }
		    else
		    {
			D(bug(" ok\n"));
		    }
		}
		else
		{
		    ioerr = IoErr();
		    D(bug(" fail\n"));
		}

		D(bug("Freeing linebuffer..."));
		FreeMem(linebuffer, MAX_LINE_LEN);
		D(bug(" ok\n"));
	    }
	    else
	    {
		ioerr = ERROR_NO_FREE_STORE;
		D(bug(" fail\n"));
	    }

	    D(bug("Freeing RDArgs..."));
	    FreeDosObject(DOS_RDARGS, rdargs);
	    D(bug(" ok\n"));
	}
	else
	{
	    D(bug(" fail\n"));
	    ioerr = ERROR_NO_FREE_STORE;
	}
    }
    else
    {
	D(bug(" fail\n"));
	ioerr = ERROR_NO_FREE_STORE;
    }


    if(ioerr)
    {
	if(!quiet) PrintFault(ioerr, "arosboot");
	FreeConfig(config);
	return 0;
    }

    if(config->bc_Modules.lh_TailPred == (struct Node *)&config->bc_Modules)
    {
	/* empty list: no modules */
	if(!quiet) Printf("No modules found in config file\n");
	FreeConfig(config);
	return 0;
    }

    D(bug("End of config file processing\n"));
    return config;
}

void FreeConfig(struct BootConfig *config)
{
    struct ModNode *modnode, *modnext;
    struct FuncNode *funcnode, *funcnext;

    D(bug("FreeConfig...\n"));

    if(config)
    {
	for(modnode = (struct ModNode *)config->bc_Modules.lh_Head;
	    modnode->mn_Node.ln_Succ;
	    modnode = modnext)
	{
	    /* Get the next node here, because after the Remove() it is undefined. */
	    modnext = (struct ModNode *)modnode->mn_Node.ln_Succ;

	    for(funcnode = (struct FuncNode *)modnode->mn_FuncList.lh_Head;
		funcnode->fn_Node.ln_Succ;
		funcnode = funcnext)
	    {
		funcnext = (struct FuncNode *)funcnode->fn_Node.ln_Succ;
		D(bug("  Free funcnode\n"));
		FreeVec(funcnode);
	    }

	    Remove(&modnode->mn_Node);
	    if(modnode->mn_Node.ln_Name)
	    {
		D(bug("  Free modname\n"));
		FreeVec(modnode->mn_Node.ln_Name);
	    }
	    D(bug("  Free modnode\n"));
	    FreeVec(modnode);
	}
	D(bug("  Free config\n"));
	FreeVec(config);
    }
}

BOOL isconfigline(char *buffer, char *keyword)
{
    /* account for indented lines */
    while(isspace(*buffer) && *buffer != '\n') buffer++;

    /* comment? */
    if(*buffer == ';') return FALSE;

    while(*keyword && *buffer)
    {
	if(*keyword != toupper(*buffer)) return FALSE;
	keyword++;
	buffer++;
    }

    return TRUE;
}

/* FindName case-insensitive */
struct Node *FindNameNC(struct List *list, UBYTE *name)
{
    struct Node * node;

    /* Look through the list */
    for (node=GetHead(list); node; node=GetSucc(node))
    {
	/* Only compare the names if this node has one. */
	if(node->ln_Name)
	{
	    /* Check the node. If we found it, stop. */
	    if (!strcasecmp (node->ln_Name, name))
		break;
	}
    }
    /*
     * If we found a node, this will contain the pointer to it. If we
     * didn't, this will be NULL (either because the list was
     * empty or because we tried all nodes in the list)
     */
    return node;
}
