/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <toollib/toollib.h>

int verbose;

/* Functions */
char *
_xstrdup (const char * str, const char * file, int line)
{
    char * nstr;

    assert (str);

    nstr = strdup (str);

    if (!nstr)
    {
	fprintf (stderr, "Out of memory in %s:%d", file, line);
	exit (20);
    }

    return nstr;
}

void *
_xmalloc (size_t size, const char * file, int line)
{
    void * ptr;

    ptr = malloc (size);

    if (size && !ptr)
    {
	fprintf (stderr, "Out of memory in %s:%d", file, line);
	exit (20);
    }

    return ptr;
}

void *
_xrealloc (void * optr, size_t size, const char * file, int line)
{
    void * ptr;

    ptr = realloc (optr, size);

    if (size && !ptr)
    {
	fprintf (stderr, "Out of memory in %s:%d", file, line);
	exit (20);
    }

    return ptr;
}

void
_xfree (void * ptr, const char * file, int line)
{
    if (ptr)
	free (ptr);
    else
	fprintf (stderr, "Illegal free(NULL) in %s:%d", file, line);
}

void *
_xcalloc (size_t number, size_t size, const char * file, int line)
{
    void * ptr;

    ptr = calloc (number, size);

    if (size && !ptr)
    {
	fprintf (stderr, "Out of memory in %s:%d", file, line);
	exit (20);
    }

    return ptr;
}

Node *
FindNode (const List * l, const char * name)
{
    Node * n;

    ForeachNode (l, n)
    {
	if (!strcmp (n->name, name))
	    return n;
    }

    return NULL;
}

Node *
FindNodeNC (const List * l, const char * name)
{
    Node * n;

    ForeachNode (l, n)
    {
	if (!strcasecmp (n->name, name))
	    return n;
    }

    return NULL;
}

void
printlist (const List * l)
{
    Node * n;

    ForeachNode (l,n)
    {
	printf ("    \"%s\"\n", n->name);
    }
}

int
execute (const char * cmd, const char * args,
	 const char * in, const char * out)
{
    char buffer[4096];
    int rc;

    strcpy (buffer, cmd);
    strcat (buffer, " ");

    if (strcmp (in, "-"))
    {
	strcat (buffer, "<");
	strcat (buffer, in);
	strcat (buffer, " ");
    }

    if (strcmp (out, "-"))
    {
	strcat (buffer, ">");
	strcat (buffer, out);
	strcat (buffer, " ");
    }

    strcat (buffer, args);

    if (verbose)
	printf ("Executing %s...\n", buffer);

    rc = system (buffer);

    if (rc)
    {
	printf ("%s failed: %d\n", buffer, rc);
    }

    return !rc;
}

Node *
RemHead (List * l)
{
    Node * node = GetHead (l);

    Remove (node);

    return node;
}
