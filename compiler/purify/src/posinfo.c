/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include "posinfo.h"
#include "util.h"
#include "error.h"

int Purify_Lineno;
const char * Purify_Filename;
const char * Purify_Functionname;

CallStackNode  * Purify_CallStack;
CallStackEntry * Purify_CurrentFrame;

static int usedEntries;
static int callLevels;

static void _Purify_MakeRoom (void)
{
    if (!Purify_CallStack)
    {
	Purify_CallStack    = xmalloc (sizeof (CallStackNode));
	Purify_CurrentFrame = &Purify_CallStack->entries[0];

	Purify_CallStack->next = NULL;

	usedEntries = 1;
    }
    else
    {
	if (usedEntries == PURIFY_CSNE)
	{
	    CallStackNode * node = xmalloc (sizeof (CallStackNode));

	    node->next		= Purify_CallStack;
	    Purify_CallStack	= node;
	    Purify_CurrentFrame = &Purify_CallStack->entries[0];

	    usedEntries = 1;
	}
	else
	{
	    usedEntries ++;
	    Purify_CurrentFrame ++;
	}
    }
}

void Purify_EnterFunction (const char * filename, const char * functionname,
    int lineno, const void * fp)
{
    _Purify_MakeRoom ();

    callLevels ++;

    SETPOS(&Purify_CurrentFrame->caller);

    Purify_Filename	= filename;
    Purify_Functionname = functionname;
    Purify_Lineno	= lineno;

    SETPOS(&Purify_CurrentFrame->called);

    Purify_CurrentFrame->fp = fp;

#if 0
    printf ("EnterFunction() usedEntries=%d  Purify_CurrentFrame=%p\n",
	usedEntries, Purify_CurrentFrame
    );
    printf ("at " POSINFO_FORMAT " fp=%p\n",
	POSINFO_ARG(&Purify_CurrentFrame->called),
	fp
    );
    printf ("called from " POSINFO_FORMAT "\n",
	POSINFO_ARG(&Purify_CurrentFrame->caller)
    );
#endif
}

void Purify_LeaveFunction (void)
{
    Purify_Filename	= Purify_CurrentFrame->caller.filename;
    Purify_Functionname = Purify_CurrentFrame->caller.functionname;
    Purify_Lineno	= Purify_CurrentFrame->caller.lineno;

#if 0
    printf ("LeaveFunction() usedEntries=%d  Purify_CurrentFrame=%p\n",
	usedEntries, Purify_CurrentFrame
    );
    printf ("to " POSINFO_FORMAT "\n",
	POSINFO_ARG(&Purify_CurrentFrame->caller)
    );
#endif

    usedEntries --;
    callLevels --;

    if (usedEntries > 0)
	Purify_CurrentFrame --;
    else if (callLevels)
    {
	CallStackNode * node = Purify_CallStack;

	Purify_CallStack = node->next;

	xfree (node);

	if (Purify_CallStack)
	{
	    usedEntries = PURIFY_CSNE;
	    Purify_CurrentFrame = &Purify_CallStack->entries[PURIFY_CSNE-1];
	}
	else
	{
	    usedEntries = 0;
	    Purify_CurrentFrame = NULL;
	}
    }
}

void Purify_RememberCallers (RememberData * rd)
{
    int n = 0;
    int entry = usedEntries;
    CallStackEntry * cse;

    cse = Purify_CurrentFrame;

    SETPOS(&rd->current);

    while (n < PURIFY_RememberDepth && n < callLevels)
    {
	rd->stack[n] = cse->caller;

	entry --;
	n ++;

	if (entry > 0)
	    cse --;
	else
	{
	    if (!Purify_CallStack->next)
		break;

	    entry = PURIFY_CSNE;
	    cse = &Purify_CallStack->next->entries[PURIFY_CSNE-1];
	}
    }

    rd->nstack = n;
}

void Purify_PrintCallers (RememberData * rd)
{
    int t;

    if (rd->nstack == -1)
	return;

    fprintf (stderr, "at " POSINFO_FORMAT "\n",
	POSINFO_ARG(&rd->current)
    );

    for (t=0; t<rd->nstack; t++)
    {
	fprintf (stderr, "called by " POSINFO_FORMAT "\n",
	    POSINFO_ARG(&rd->stack[t])
	);
    }
}
