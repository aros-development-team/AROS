/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Amiga bootloader -- config file routines
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>

#include "string.h"

#include <proto/exec.h>
#include <proto/dos.h>

#include "boot.h"
#include "config.h"

char modulestring[] = "MODULE ";

struct FileList *ReadConfig(char *file)
{
    BPTR fh;
    struct FileList *list;
    struct Node *node;
    char *name;
    char *result;
    int i;
    char buffer[80];

    if( (fh = Open(file, MODE_OLDFILE)) )
    {
	if( (list = AllocVec(sizeof(struct FileList), MEMF_CLEAR)) )
	{
	    list->fl_List.lh_Head = (struct Node *)&list->fl_List.lh_Tail;
	    list->fl_List.lh_Tail = 0;
	    list->fl_List.lh_TailPred = (struct Node *)&list->fl_List.lh_Head;
	    list->fl_Num = 0;

            while( (result = FGets(fh, buffer, 79)) )
	    {
		if( !(strnicmp(buffer, modulestring, strlen(modulestring))) )
		{
		    /* found a valid entry */
		    if( (node = AllocVec(sizeof(struct Node), MEMF_CLEAR)) )
		    {
			if( (name = AllocVec(strlen(buffer+strlen(modulestring)), MEMF_CLEAR)) )
			{
			    strcpy(name, buffer+strlen(modulestring));
			    /* remove newline character, if present */
			    for(i = 0; i < strlen(name); i++)
			    {
				if('\n' == name[i]) name[i] = '\0';
			    }
			    node->ln_Name = name;
			    AddTail((struct List *)list, node);
			    list->fl_Num++;
			}
			else
			{
			    FreeVec(node);
			}
		    } /* if(node=AllocVec) */
		} /* if(strnicmp) */
	    } /* while */

	} /* if(list=AllocVec) */
	Close(fh);
    } /* if(Open) */
    else
    {
	return 0;
    } /* if(Open) */

    return list;
}

void FreeConfig(struct FileList *list)
{
    struct Node *node, *next;

    for(node = list->fl_List.lh_Head; node->ln_Succ; node = next)
    {
	/*
	    Get the next node here, because after the Remove() it is undefined.
	*/
	next = node->ln_Succ;
	Remove(node);
	if(node->ln_Name) FreeVec(node->ln_Name);
	FreeVec(node);
    }

    if(list) FreeVec(list);
}
