/* MetaMake - A Make extension
   Copyright © 1995-2001, The AROS Development Team. All rights reserved.

This file is part of MetaMake.

MetaMake is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

MetaMake is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */
/* Includes */
#include "config.h"

#ifdef PROTOTYPES
#   define PARAMS(x) x
#else
#   define PARAMS(x) ()
#endif /* PROTOTYPES */

#if defined(HAVE_STDARG_H) && defined(__STDC__) && __STDC__
#   include <stdarg.h>
#   define VA_START(args, lastarg) va_start(args, lastarg)
#else
#   include <varargs.h>
#   define VA_START(args, lastarg) va_start(args)
#endif

#ifndef __DATE__
#   define __DATE__ "No __DATE__"
#endif

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif
#ifdef HAVE_SYS_STAT_H
#   include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#   include <netinet/in.h> /* for htonl/ntohl() */
#endif

#define MAJOR		0L
#define MINOR		6L
#define REVISION	1L
#define ID		((MAJOR << 24) | (MINOR << 16) | REVISION)
#define CHECK_ID(id)    (((id) & 0xFFFF0000) == ((ID) & 0xFFFF0000))

/* Types */
typedef struct _Node Node;

struct _Node
{
    Node * next,
	 * prev;
    char * name;
};

typedef struct
{
    Node * first,
	 * last,
	 * prelast;
}
List;

typedef struct
{
    Node   node;
    char * value;
}
Var;

typedef struct
{
    Node node;

    /* The current target doesn't appear in the Makefile itself; it's
       just a metatarget. Don't try to run make with it. */
    int  virtualtarget : 1;
}
Makefile;

#define FLAG_VIRTUAL	0x0001

typedef struct
{
    Node node;

    char *dir;
    char *src;
    char *dest;
}
Regenerate;

typedef struct
{
    Node node;
    int  updated : 1;

    List makefiles;
    List deps;
}
Target;

typedef struct
{
    Node   node;
    time_t time;
}
Dep;

typedef struct _DirNode DirNode;

struct _DirNode
{
    Node node;
    time_t	time;
    DirNode   * parent;
    List subdirs;
};

typedef struct
{
    Node node;

    char * maketool;
    char * defaultmakefilename;
    char * top;
    char * defaulttarget;
    char * genmakefilescript;
    char * globalvarfile;
    char * genglobalvarfile;

    DirNode * topdir;

    int readvars;
    int buildmflist;
    int buildtargetlist;

    List genmakefiledeps;
    List ignoredirs;
    List vars;
    List makefiles;
    List targets;
}
Project;

/* globals */
List projects;
Project * defaultprj, * project;
Project * firstprj;
char * mflags[64];
int mflagc;
char * targets[64];
int targetc;
int verbose = 0;
int debug = 0;

/* Macros */
#   define NewList(l)       (((List *)l)->prelast = (Node *)(l), \
			    ((List *)l)->last = 0, \
			    ((List *)l)->first = (Node *)&(((List *)l)->last))

#   define AddHead(l,n)     ((void)(\
	((Node *)n)->next        = ((List *)l)->first, \
	((Node *)n)->prev        = (Node *)&((List *)l)->first, \
	((List *)l)->first->prev = ((Node *)n), \
	((List *)l)->first       = ((Node *)n)))

#   define AddTail(l,n)     ((void)(\
	((Node *)n)->next          = (Node *)&((List *)l)->last, \
	((Node *)n)->prev          = ((List *)l)->prelast, \
	((List *)l)->prelast->next = ((Node *)n), \
	((List *)l)->prelast       = ((Node *)n) ))

#   define Remove(n)        ((void)(\
	((Node *)n)->prev->next = ((Node *)n)->next,\
	((Node *)n)->next->prev = ((Node *)n)->prev ))

#   define GetHead(l)       (void *)(((List *)l)->first->next \
				? ((List *)l)->first \
				: (Node *)0)
#   define GetTail(l)       (void *)(((List *)l)->prelast->prev \
				? ((List *)l)->prelast \
				: (Node *)0)
#   define GetNext(n)       (void *)(((Node *)n)->next->next \
				? ((Node *)n)->next \
				: (Node *)0)
#   define GetPrev(n)       (void *)(((Node *)n)->prev->prev \
				? ((Node *)n)->prev \
				: (Node *)0)
#   define ForeachNode(l,n) \
	for (n=(void *)(((List *)(l))->first); \
	    ((Node *)(n))->next; \
	    n=(void *)(((Node *)(n))->next))
#   define ForeachNodeSafe(l,node,nextnode) \
	for (node=(void *)(((List *)(l))->first); \
	    ((nextnode)=(void*)((Node *)(node))->next); \
	    (node)=(void *)(nextnode))

#define cfree(x)        if (x) free (x)
#define SETSTR(str,val) \
    cfree (str); \
    str = val ? xstrdup (val) : NULL

#define xstrdup(str)        _xstrdup(str,__FILE__,__LINE__)
#define xmalloc(size)       _xmalloc(size,__FILE__,__LINE__)
#define xfree(ptr)          _xfree(ptr,__FILE__,__LINE__)
#define new(x)              ((x *) xmalloc (sizeof (x)))

/* Prototypes */
extern int execute PARAMS ((Project * prj, const char * cmd, const char * in,
			    const char * out, const char * args));
extern void setvar PARAMS ((Project *, const char *, const char *));
extern void freecachenodes PARAMS ((DirNode * node));
extern int  checkdeps PARAMS ((Project * prj, time_t desttime));

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

void
_xfree (void * ptr, const char * file, int line)
{
    if (ptr)
	free (ptr);
    else
	fprintf (stderr, "Illegal free(NULL) in %s:%d", file, line);
}

void *
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

void
error (char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    fprintf (stderr, "Error: ");
    vfprintf (stderr, fmt, args);
    fprintf (stderr, ": %s\n", strerror (errno));
    va_end (args);
}

void
printlist (List * l)
{
    Node * n;

    ForeachNode (l,n)
    {
	printf ("    \"%s\"\n", n->name);
    }
}

void
printvarlist (List * l)
{
    Var * n;

    ForeachNode (l,n)
    {
	printf ("    %s=%s\n", n->node.name, n->value);
    }
}

void
printtargetlist (List * l)
{
    Target * n;

    ForeachNode (l,n)
    {
	printf ("target %s:\n", n->node.name);
	printf ("  updated=%d\n", n->updated);
	printf ("  makefiles=\n");
	printlist (&n->makefiles);
	printf ("  deps=\n");
	printlist (&n->deps);
    }
}

Var *
newnode (const char * name, const char * value)
{
    Var * n;

    assert (name);

    n = new (Var);
    n->value = NULL;

    n->node.name = xstrdup (name);
    SETSTR(n->value, value);

    return n;
}

Dep *
newdepnode (const char * path)
{
    Dep * n;
    struct stat st;

    assert (path);

    n = new (Dep);

    n->node.name = xstrdup (path);
    lstat (path, &st);
    n->time = st.st_mtime;

    return n;
}

Var *
addnodeonce (List * l, const char * name, const char * value)
{
    Var * n;

    n = FindNode (l, name);

    if (n)
    {
	cfree (n->value);
	SETSTR (n->value, value);
    }
    else
    {
	n = newnode (name, value);
	AddTail(l,n);
    }

    return n;
}

char *
getvar (Project * prj, const char * varname)
{
    static char buffer[256];
    char *env_val;
    Var * var = FindNode (&prj->vars, varname);

    if (var)
	return var->value;

    env_val = getenv(varname);
    if(env_val)
    {
	return env_val;
    }
    sprintf (buffer, "?$(%s)", varname);
    return buffer;
}

char *
substvars (Project * prj, const char * str)
{
    static char buffer[4096];
    char varname[256];
    const char * src;
    char * dest, * vptr;

    assert (str);

    src = str;
    dest = buffer;

    while (*src)
    {
	if (*src == '$')
	{
	    src += 2;
	    vptr = varname;

	    while (*src && *src != ')')
	    {
		*vptr ++ = *src ++;
	    }
	    if (*src)
		src ++;

	    *vptr = 0;

	    strcpy (dest, getvar (prj, varname));
	    dest += strlen (dest);
	}
	else
	    *dest ++ = *src ++;

	assert (dest<buffer+1024);
    }

    *dest = 0;

    return buffer;
}

char **
getargs (Project * prj, const char * line, int * argc, int subst)
{
    static char * argv[64];
    static char * buffer = NULL;
    char * src;
    int arg;

    cfree (buffer);

    if (!prj)
	return NULL;

    assert (line);

    if (subst)
	buffer = xstrdup (substvars (prj, line));
    else
	buffer = xstrdup (line);

    assert (buffer);

    src = buffer;
    arg = 0;

    while (*src)
    {
	while (isspace (*src))
	    src ++;

	if (!*src)
	    break;

	assert (arg < 63);
	argv[arg++] = src;

	if (*src == '"')
	{
	    while (*src && *src != '"')
		src ++;
	}
	else
	{
	    while (*src && !isspace (*src))
		src ++;
	}

	if (*src)
	    *src++ = 0;
    }

    argv[arg] = NULL;

    if (argc)
	*argc = arg;

    return argv;
}

Project *
initproject (char * name)
{
    Project * prj = new (Project);

    memset (prj, 0, sizeof(Project));

    if (!defaultprj)
    {
	prj->maketool = xstrdup ("make \"TOP=$(TOP)\" \"CURDIR=$(CURDIR)\"");
	prj->defaultmakefilename = xstrdup ("Makefile");
	prj->top = getcwd (NULL, 1024);
	prj->defaulttarget = xstrdup ("all");
	prj->genmakefilescript = NULL;
	prj->globalvarfile = NULL;
	prj->genglobalvarfile = NULL;
    }
    else
    {
	prj->maketool = xstrdup (defaultprj->maketool);
	prj->defaultmakefilename = xstrdup (defaultprj->defaultmakefilename);
	prj->top = xstrdup (defaultprj->top);
	prj->defaulttarget = xstrdup (defaultprj->defaulttarget);
	SETSTR (prj->genmakefilescript, defaultprj->genmakefilescript);
	SETSTR (prj->globalvarfile, defaultprj->globalvarfile);
	SETSTR (prj->genglobalvarfile, defaultprj->genglobalvarfile);
    }

    prj->node.name = xstrdup (name);

    prj->readvars = 1;
    prj->buildmflist = 1;
    prj->buildtargetlist = 1;

    NewList(&prj->genmakefiledeps);
    NewList(&prj->ignoredirs);
    NewList(&prj->vars);
    NewList(&prj->makefiles);
    NewList(&prj->targets);

    return prj;
}

void
freelist (List * l)
{
    Node * node, * next;

    ForeachNodeSafe(l,node,next)
    {
	Remove (node);

	cfree (node->name);
	free (node);
    }
}

void
freevarlist (List * l)
{
    Var * node, * next;

    ForeachNodeSafe(l,node,next)
    {
	Remove (node);

	xfree (node->node.name);
	cfree (node->value);
	xfree (node);
    }
}

void
freetarget (Target * target)
{
    xfree (target->node.name);

    freelist (&target->makefiles);
    freelist (&target->deps);

    xfree (target);
}

void
freetargetlist (List * l)
{
    Node * node, * next;

    ForeachNodeSafe(l,node,next)
    {
	Remove (node);
	freetarget ((Target *)node);
    }
}

void
freeproject (Project * prj)
{
    assert (prj);

    cfree (prj->node.name);
    cfree (prj->maketool);
    cfree (prj->defaultmakefilename);
    cfree (prj->top);
    cfree (prj->defaulttarget);
    cfree (prj->genmakefilescript);
    cfree (prj->globalvarfile);
    cfree (prj->genglobalvarfile);

    freelist (&prj->genmakefiledeps);
    freelist (&prj->ignoredirs);
    freevarlist (&prj->vars);
    freelist (&prj->makefiles);
    freetargetlist (&prj->targets);
    if (prj->topdir)
	freecachenodes (prj->topdir);

    xfree (prj);
}

void
init (void)
{
    char * optionfile;
    char * home;
    char line[256];
    FILE * optfh = NULL;

    NewList(&projects);
    defaultprj = project = initproject ("default");
    AddTail(&projects, project);


    /* Try "$MMAKE_CONFIG" */
    if ((optionfile = getenv ("MMAKE_CONFIG")))
	optfh = fopen (optionfile, "r");

    /* Try "$HOME/.mmake.config" */
    if (!optfh)
    {
	if ((home = getenv("HOME")))
	{
		optionfile = xmalloc (strlen(home) + sizeof("/.mmake.config") + 1);
		sprintf (optionfile, "%s/.mmake.config", home);
		optfh = fopen (optionfile, "r");
		free (optionfile);
    	}
    }

    /* Try with $CWD/.mmake.config" */
    if (!optfh)
	optfh = fopen (".mmake.config", "r");

    /* Try with "$CWD/mmake.config */
    if (!optfh)
	optfh = fopen ("mmake.config", "r");

    /* Give up */
    if (!optfh)
    {
	fprintf (stderr,
	    "Please set the HOME or MMAKE_CONFIG env var (with setenv or export)\n"
	);
	error ("Opening mmake.config for reading");
	exit (10);
    }

    while (fgets (line, sizeof(line), optfh))
    {
	if (*line == '\n' || *line == '#') continue;
	line[strlen(line)-1] = 0;

	if (*line == '[')
	{
	    char * name, * ptr;

	    name = ptr = line+1;
	    while (*ptr && *ptr != ']')
		ptr ++;

	    *ptr = 0;

#if 0
printf ("name=%s\n", name);
#endif

	    project = initproject (name);

	    if (!firstprj)
		firstprj = project;

	    AddTail(&projects,project);
	}
	else
	{
	    char * cmd, * args, * ptr;

	    cmd = line;
	    while (isspace (*cmd))
		cmd ++;

	    args = cmd;
	    while (*args && !isspace(*args))
	    {
		*args = tolower (*args);
		args ++;
	    }
	    if (*args)
		*args++ = 0;
	    while (isspace (*args))
		args ++;

	    ptr = args;

	    while (*ptr && *ptr != '\n')
		ptr ++;

	    *ptr = 0;

	    if (!strcmp (cmd, "add"))
	    {
		Node * n;
		n = (Node *)newnode(args,NULL);
		AddTail(&project->makefiles, n);
	    }
	    else if (!strcmp (cmd, "ignoredir"))
	    {
		Node * n;
		n = (Node *)newnode(args,NULL);
		AddTail(&project->ignoredirs, n);
	    }
	    else if (!strcmp (cmd, "defaultmakefilename"))
	    {
		SETSTR(project->defaultmakefilename,args);
	    }
	    else if (!strcmp (cmd, "top"))
	    {
		SETSTR(project->top,args);
	    }
	    else if (!strcmp (cmd, "defaulttarget"))
	    {
		SETSTR(project->defaulttarget,args);
	    }
	    else if (!strcmp (cmd, "genmakefilescript"))
	    {
		SETSTR(project->genmakefilescript,args);
	    }
	    else if (!strcmp (cmd, "genmakefiledeps"))
	    {
		Var * dep;
		int depc, t;
		char ** deps = getargs (project, args, &depc, 0);

		for (t=0; t<depc; t++)
		{
		    dep = (Var *)addnodeonce (&project->genmakefiledeps,
			deps[t], NULL
		    );
		}
	    }
	    else if (!strcmp (cmd, "globalvarfile"))
	    {
		SETSTR(project->globalvarfile,args);
	    }
	    else if (!strcmp (cmd, "genglobalvarfile"))
	    {
		SETSTR(project->genglobalvarfile,args);
	    }
	    else if (!strcmp (cmd, "maketool"))
	    {
		SETSTR(project->maketool,args);
	    }
	    else
	    {
		setvar(project, cmd, args);
	    }
	}
    }

    fclose (optfh);

    if (debug)
    {
	printf ("known projects: ");
	printlist (&projects);
    }
}

void
printdirnode (DirNode * node, int level)
{
    DirNode * subdir;
    int t;

    for (t=0; t<level; t++)
	printf ("    ");

    printf ("%s\n", node->node.name);

    level ++;

    ForeachNode (&node->subdirs, subdir)
	printdirnode (subdir, level);
}

void
printdirtree (Project * prj)
{
    printf ("top=%s\n", prj->top);
    printdirnode (prj->topdir, 1);
}

void
freecachenodes (DirNode * node)
{
    DirNode * subnode;

    while ((subnode = GetHead (&node->subdirs)))
    {
	Remove (subnode);
	freecachenodes (subnode);
    }

    xfree (node->node.name);
    xfree (node);
}

DirNode *
readcachedir (FILE * fh)
{
    int len, ret;
    DirNode * node, * subnode;
    time_t tt;

    ret = fread (&len, sizeof(len), 1, fh);
    if (ferror (fh))
    {
	error ("readcachedir:fread():%d",
	    __LINE__
	);
	return NULL;
    }
    len = ntohl (len);

    if (len < 0)
	return NULL;

    node = new (DirNode);
    NewList(&node->subdirs);
    node->node.name = xmalloc (len+1);
    node->parent = NULL;

    if (len)
    {
	ret = fread (node->node.name, len, 1, fh);
	if (ferror (fh))
	{
	    error ("readcachedir:fread():%d",
		__LINE__
	    );
	    free (node);
	    return NULL;
	}
    }
    node->node.name[len] = 0;
    ret = fread (&tt, sizeof (tt), 1, fh);
    if (ferror (fh))
    {
	error ("readcachedir:fread():%d",
	    __LINE__
	);
	free (node);
	return NULL;
    }
    node->time = ntohl (tt);

    while ((subnode = readcachedir (fh)))
    {
	subnode->parent = node;
	AddTail (&node->subdirs, subnode);
    }

    return node;
}

void
readcache (Project * prj)
{
    char path[256];
    FILE * fh;
    long id;

    strcpy (path, prj->top);
    strcat (path, "/mmake.cache");
    assert (strlen(path) < sizeof(path));

    fh = fopen (path, "r");

    if (fh)
    {
	fread (&id, sizeof(id), 1, fh);
	if (!CHECK_ID(id))
	{
	    fclose (fh);
	    fh = NULL;
	}
    }

    if (fh)
    {
	prj->topdir = readcachedir (fh);

	if (!prj->topdir)
	{
	    fclose (fh);
	    fh = NULL;
	}
    }

    if (!fh)
    {
	prj->topdir = new (DirNode);
	NewList(&prj->topdir->subdirs);
	prj->topdir->node.name = xstrdup ("");
	prj->topdir->parent = NULL;

	/* Force a check the first time */
	prj->topdir->time = 0;
    }

    if (fh)
	fclose (fh);

#if 0
    printf ("readcache()\n");
    printdirtree (prj);
#endif
}

DirNode *
finddirnode (Project * prj, const char * path)
{
    const char * ptr;
    char dirname[256];
    int len;
    DirNode * node, * subdir;

    ptr = path+2;
    node = prj->topdir;

    if (!*ptr)
	return node;

    subdir = NULL;

    while (*ptr)
    {
	for (len=0; ptr[len] && ptr[len] != '/'; len++);

	strncpy (dirname, ptr, len);
	dirname[len] = 0;
	ptr += len;
	while (*ptr == '/')
	    ptr ++;

	subdir = FindNode (&node->subdirs, dirname);

	if (!subdir)
	    break;

	node = subdir;
    }

    return subdir;
}

DirNode *
adddirnode (Project * prj, const char * path)
{
    char pathcopy[256], * pathptr;
    const char * ptr;
    char dirname[256];
    int len;
    DirNode * node, * subdir;
    struct stat st;

    ptr = path+2;
    node = prj->topdir;
    pathptr = pathcopy;

    if (!*ptr)
	return node;

    printf ("adddirnode(): adding %s\n", path);

    subdir = NULL;

    while (*ptr)
    {
	for (len=0; ptr[len] && ptr[len] != '/'; len++);

	strncpy (dirname, ptr, len);
	dirname[len] = 0;
	if (pathptr != pathcopy)
	    *pathptr ++ = '/';
	strncpy (pathptr, ptr, len);
	pathptr[len] = 0;
	ptr += len;
	while (*ptr == '/')
	    ptr ++;

	subdir = FindNode (&node->subdirs, dirname);

	if (!subdir)
	{
	    subdir = new (DirNode);
	    NewList (&subdir->subdirs);
	    subdir->node.name = xstrdup (dirname);
	    subdir->parent = node;
	    stat (pathcopy, &st);
	    subdir->time = st.st_mtime;

	    AddTail(&node->subdirs, subdir);

	    node = subdir;
	}

	node = subdir;
    }

#if 0
    printf ("adddirnode()\n");
    printdirtree (prj);
#endif

    return subdir;
}

int
writecachedir (FILE * fh, DirNode * node)
{
    int len, ret, out;
    DirNode * subnode;

    len = strlen (node->node.name);
    out = htonl (len);
    ret = fwrite (&out, sizeof(out), 1, fh);
    if (ret <= 0)
    {
	error ("writecachedir/fwrite():%d",
	    __LINE__
	);
	return ret;
    }

    if (len)
    {
	ret = fwrite (node->node.name, len, 1, fh);
	if (ret <= 0)
	{
	    error ("writecachedir/fwrite():%d",
		__LINE__
	    );
	    return ret;
	}
    }
    out = htonl (node->time);
    ret = fwrite (&out, sizeof (out), 1, fh);
    if (ret <= 0)
    {
	error ("writecachedir/fwrite():%d",
	    __LINE__
	);
	return ret;
    }

    ForeachNode (&node->subdirs, subnode)
    {
	ret = writecachedir (fh, subnode);
	if (ret <= 0)
	    return ret;
    }

    out = htonl (-1);
    ret = fwrite (&out, sizeof (out), 1, fh);
    if (ret <= 0)
    {
	error ("writecachedir/fwrite():%d",
	    __LINE__
	);
	return ret;
    }

    return ret;
}

void
writecache (Project * prj)
{
    char path[256];
    FILE * fh;
    int ret;
    long id;

    if (!prj->topdir)
	return;

    strcpy (path, prj->top);
    strcat (path, "/mmake.cache");
    assert (strlen(path) < sizeof(path));

    fh = fopen (path, "w");

    if (!fh)
	return;

    id = ID;
    fwrite (&id, sizeof (id), 1, fh);

    ret = writecachedir (fh, prj->topdir);

    fclose (fh);

    if (ret <= 0)
    {
	unlink (path);

	printf ("Warning: Creating the cache failed\n");
    }
}

void
updatemakefile (Project * prj, const char * path, List * regeneratefiles)
{
    char * mf = xmalloc (strlen (path) + 4);
    char * ptr, * dir, * file, * ext;
    char * dest, * src;
    int len;
    struct stat sst, dst;

    strcpy (mf, path);
    len = strlen (mf) - 4;
    if (len > 0 && strcmp (mf+len, ".src"))
	strcat (mf, ".src");

    ptr = dir = mf;
    file = NULL;
    while (*ptr)
    {
	if (*ptr == '/')
	    file = ptr+1;

	ptr ++;
    }
    if (!file)
    {
	dir = ".";
	file = mf;
    }
    ptr = file;
    ext = NULL;
    while (*ptr)
    {
	if (*ptr == '.')
	    ext = ptr+1;

	ptr ++;
    }

    chdir (prj->top);
    if (file != mf)
    {
	file[-1] = 0;
	chdir (dir);
    }

    ext[-1] = 0;
    dest = xstrdup (file);
    ext[-1] = '.';

    src = xstrdup (file);
    if (stat (src, &sst) == -1)
    {
	xfree (dest);
	xfree (mf);
	xfree (src);
	return;
    }

    if (stat (dest, &dst) == -1
	|| sst.st_mtime > dst.st_mtime
	|| checkdeps (prj, dst.st_mtime)
    )
    {
	Regenerate *reg = new (Regenerate);

	reg->dir = xstrdup (dir);
	reg->src = src;
	reg->dest = dest;

	AddTail (regeneratefiles, reg);
    }
    else
    {
	xfree (dest);
	xfree (src);
    }

    xfree (mf);
}

void
updatemflist (Project * prj, List * regeneratefiles)
{
    char mfnsrc[256];
    Node * makefile;
    struct stat st;
  
    ForeachNode(&prj->makefiles, makefile)
    {
        strcpy(mfnsrc, makefile->name);
        strcat(mfnsrc, ".src");
        assert(strlen(mfnsrc)<256);
      
        if (!stat(mfnsrc, &st))
	    updatemakefile(prj, mfnsrc, regeneratefiles);
    }
}

void
buildmflist (Project * prj, List * regeneratefiles)
{
    char * mfn, * mfnsrc;
    struct stat st;
    char path[256];
    int len, offset;
    List dirs;
    Node * cd;
    DIR * dirh;
    struct dirent * dirent;
    int foundmf;
    DirNode * dnode;
    int done, todo, nummfs, reread;
    Node * tmpnode;
    time_t tt, now;

    if (!prj->buildmflist)
	return;

    prj->buildmflist = 0;

    printf ("Collecting makefiles...\n");

    readcache (prj);

    mfnsrc = xmalloc (strlen(mfn=prj->defaultmakefilename)+5);
    strcpy (mfnsrc, mfn);
    len = strlen (mfn);
    strcpy (mfnsrc+len, ".src");

    NewList(&dirs);
    cd = (Node *)newnode (".", NULL);
    AddTail(&dirs,cd);

    done = nummfs = reread = 0;

    time (&tt);
    now = 0;

    while ((cd = GetHead(&dirs)))
    {
	todo = 0;
	ForeachNode (&dirs, tmpnode)
	    todo ++;

	time (&now);
	if (now != tt)
	{
	    printf ("Done: %4d   Todo: %4d\r", done, todo);
	    fflush (stdout);
	    tt = now;
	}

	Remove (cd);

	chdir (prj->top);

	strcpy (path, cd->name);
	offset = strlen (path);
	path[offset ++] = '/';
	path[offset] = 0;

#if 0
    printf ("Entering \"%s\"\n", path);
#endif

	dnode = finddirnode (prj, path);

	if (dnode)
	{
	    stat (path, &st);
	}

	foundmf = 0;

	if (!dnode || st.st_mtime > dnode->time)
	{
#if 0
    printf ("Updating cache in %s\n", path);
#endif
	    if (!dnode)
		dnode = adddirnode (prj, path);

	    dnode->time = st.st_mtime;

	    reread ++;

	    dirh = opendir (path);
	    if (!dirh)
	    {
		error ("opendir(%s)", path);
		exit (10);
	    }

	    while ((dirent = readdir (dirh)))
	    {
		if (!strcmp (dirent->d_name, mfnsrc))
		{
		    foundmf = 2;
		    continue;
		}

		if (!foundmf)
		{
		    mfnsrc[len] = 0;

		    if (!strcmp (dirent->d_name, mfnsrc))
		    {
			foundmf = 1;
			mfnsrc[len] = '.';
			continue;
		    }

		    mfnsrc[len] = '.';
		}

		strcpy (path+offset, dirent->d_name);

		if (lstat (path, &st) == -1)
		{
		    error ("stat(%s)", path);
		    exit (10);
		}

		if (S_ISDIR (st.st_mode)
		    && strcmp (dirent->d_name, ".")
		    && strcmp (dirent->d_name, "..")
		    && !S_ISLNK (st.st_mode)
		    && !FindNode (&prj->ignoredirs, dirent->d_name)
		)
		{
		    addnodeonce (&dirs, path, NULL);
#if 0
    printf ("Adding %s for later\n", path);
#endif
		}

		path[offset] = 0;
	    }

	    closedir (dirh);
	}
	else
	{
	    DirNode * subdir, * nextsubdir;

	    chdir (path);

	    if (stat (mfnsrc, &st) != -1)
	    {
		foundmf = 2;
	    }
	    else
	    {
		mfnsrc[len] = 0;

		if (stat (mfnsrc, &st) != -1)
		    foundmf = 1;

		mfnsrc[len] = '.';
	    }

	    chdir (prj->top);

	    ForeachNodeSafe (&dnode->subdirs, subdir, nextsubdir)
	    {
		strcpy (path+offset, subdir->node.name);

		if (stat (path, &st) == -1 && errno == ENOENT)
		{
#if 0
    printf ("Removing %s from cache (%s)\n", path, subdir->node.name);
    printdirtree (prj);
#endif
		    Remove (subdir);
		    freecachenodes (subdir);
		}
		else
		{
#if 0
    printf ("Adding %s for later\n", path);
#endif
		    addnodeonce (&dirs, path, NULL);
		}
	    }

	    path[offset] = 0;
	}

	if (foundmf == 2)
	{
	    strcpy (path+offset, mfnsrc);
	    updatemakefile (prj, path, regeneratefiles);
	    foundmf --;
	}

	if (foundmf == 1)
	{
	    mfnsrc[len] = 0;
	    strcpy (path+offset, mfnsrc);
	    addnodeonce (&prj->makefiles, path+2, NULL);
	    path[offset] = 0;
	    mfnsrc[len] = '.';
	    nummfs ++;
	}

	free (cd->name);
	free (cd);
	done ++;
    }

    todo = 0;
    printf ("Done: %4d   Todo: %4d\n", done, todo);
    printf ("Found %d makefiles, reread %d dirs\n", nummfs, reread);

    chdir (prj->top);

    xfree (mfnsrc);

    if (debug)
    {
	printf ("project %s.makefiles=\n", prj->node.name);
	printlist (&prj->makefiles);
    }
    writecache (prj);
}

void
progress (int max, int curr, int * data)
{
    int x = curr*10/max;

    if (x != *data)
    {
	*data = x;
	putchar ('.');
	fflush (stdout);
    }
}

Target *
appendtarget (Project * prj, const char * tname, const char * mf, char ** deps, int flags)
{
    Target   * target;
    Makefile * makefile;

    assert (tname);
    assert (mf);

    target = FindNode (&prj->targets, tname);

    if (!target)
    {
	target = new (Target);
	target->node.name = strdup (tname);
	AddTail(&prj->targets, target);
	NewList(&target->makefiles);
	NewList(&target->deps);
	target->updated = 0;
    }

    if (!(makefile = FindNode (&target->makefiles, mf)) )
    {
	makefile = new (Makefile);
	makefile->node.name = xstrdup (mf);
	makefile->virtualtarget = ((flags & FLAG_VIRTUAL) != 0);

	AddTail (&target->makefiles, makefile);
    }
    else
    {
	makefile->virtualtarget = ((flags & FLAG_VIRTUAL) != 0);
    }

#if 0
printf ("add %s.%s mf=%s\n", prj->node.name, target->node.name, mf);
#endif

    if (deps)
    {
	while (*deps)
	{
#if 0
printf ("   add dep %s\n", *deps);
#endif
	    addnodeonce (&target->deps, *deps, NULL);
	    deps ++;
	}
    }

    return target;
}

void
setvar (Project * prj, const char * name, const char * val)
{
    Var * var;

    assert (name);

#if 0
    printf ("assign %s=%s\n", name, val);
#endif

    var = FindNode (&prj->vars, name);

    if (!var)
    {
	var = new (Var);
	var->node.name = xstrdup (name);
	var->value = NULL;
	AddTail(&prj->vars, var);
    }

    SETSTR (var->value, val);

#if 0
    printf ("project %s.vars=", prj->node.name);
    printvarlist (&prj->vars);
#endif
}

void
buildtargetlist (Project * prj)
{
    int max, pos, data;
    Node * mfnode;
    FILE * fh;
    char line[256];
    int lineno;

    if (!prj->buildtargetlist)
	return;

    prj->buildtargetlist = 0;

    printf ("Collecting metatargets...");

    max=0;
    ForeachNode(&prj->makefiles,mfnode) max++;
    pos=data=0;

    ForeachNode(&prj->makefiles,mfnode)
    {
	int contline = 0;
	int flags = 0;

	pos++;
	progress (max,pos,&data);

#if 0
printf ("Opening %s\n", mfnode->name);
#endif

	fh = fopen (mfnode->name, "r");

	if (!fh)
	{
	    error ("buildtargetlist:fopen():%d: Opening %s for reading",
		__LINE__, mfnode->name
	    );
	}

	lineno = 0;

	while (fgets (line, sizeof(line), fh))
	{
	    char * targets[64], ** tptr;

	    lineno ++;

	    if (!strncmp (line, "#MM", 3))
	    {
		char * ptr;
		char * depptr, ** deps;
		int count, depc, t;
		int cl;

#if 0
printf ("found #MM in %s\n", mfnode->name);
#endif

		ptr = line+3;

		if (!contline)
		{
		    if (*ptr == '-')
		    {
			flags |= FLAG_VIRTUAL;
			ptr ++;
		    }
		    else
			flags &= ~FLAG_VIRTUAL;
		}

		depptr = ptr + strlen (ptr) - 2;

		cl = (*depptr == '\\');

		if (cl)
		    *depptr = 0;

		ptr = substvars (prj, ptr);

		/* Must be *after* substvars() or empty target lines
		   will cause problems. */
		while (isspace (*ptr))
		    ptr ++;

		if (!*ptr)
		{
		    char ** targets;
		    fgets (line, sizeof(line), fh);
		    ptr = substvars (prj, line);

		    while (*ptr != ':' && *ptr)
			ptr ++;

		    *ptr = 0;

		    targets = getargs (prj, line, &count, 1);

		    if (count != 0)
			appendtarget (prj, targets[0], mfnode->name, NULL, flags);
		    else
			printf ("Warning: Can't find metatarget in %s:%d\n", mfnode->name, lineno);
		}
		else
		{
		    char * lptr = ptr;

		    if (!contline)
		    {
			while (*ptr != ':' && *ptr)
			    ptr ++;
			if (*ptr)
			    *ptr ++ = 0;
			depptr = ptr;

			tptr = getargs (prj, lptr, &count, 1);
			for (t=0; t<count; t++)
			    targets[t] = xstrdup (tptr[t]);
		    }
		    else
			depptr = ptr;

		    deps = getargs (prj, depptr, &depc, 1);

		    for (t=0; t<count; t++)
		    {
			appendtarget (prj, targets[t], mfnode->name, deps, flags);
		    }

		    contline = cl;

		    if (!contline)
		    {
			for (t=0; t<count; t++)
			    xfree (targets[t]);
		    }
		}
	    } /* If this is a MetaMake line in the makefile */
	} /* For all lines in a makefile */

#if 0
printf ("Read %d lines\n", lineno);
#endif

	fclose (fh);
    } /* For all makefiles in the project */

    putchar ('\n');

    if (debug)
    {
	printf ("%s.targets=\n", prj->node.name);
	printtargetlist (&prj->targets);
    }
}

void
readvars (Project * prj)
{
    List deps;
    Node * node, * next;
    Dep * dep;

    if (!prj->readvars)
	return;

    prj->readvars = 0;

    printf ("Read vars...\n");

    setvar (prj, "TOP", prj->top);
    setvar (prj, "CURDIR", "");

    if (prj->globalvarfile)
    {
	char * fn;
	FILE * fh;
	char line[256];
	char * name, * value, * ptr;

	fn = xstrdup (substvars (prj, prj->globalvarfile));
	fh = fopen (fn, "r");

	if (!fh && prj->genglobalvarfile)
	{
	    char * gen = xstrdup (substvars (prj, prj->genglobalvarfile));

	    printf ("Generating %s...\n", fn);

	    if (!execute (prj, gen, "-", "-", ""))
	    {
		error ("Error while creating \"%s\" with \"%s\"", fn, gen);
		exit (10);
	    }
	    else
		fh = fopen (fn, "r");

	    xfree (gen);
	}

	xfree (fn);

	if (!fh)
	{
	    error ("readvars():fopen(): Opening \"%s\" for reading", fn);
	    return;
	}

	while (fgets (line, sizeof(line), fh))
	{
	    if (*line == '\n' || *line == '#') continue;
	    line[strlen(line)-1] = 0;

	    ptr = line;
	    while (isspace (*ptr)) ptr++;
	    name = ptr;
	    while (*ptr && !isspace(*ptr) && *ptr != ':' && *ptr != '=')
		ptr ++;

	    if (*ptr)
		*ptr++ = 0;

	    while (isspace(*ptr) || *ptr == ':' || *ptr == '=')
		ptr ++;

	    value = ptr;

	    while (*ptr && *ptr != '#')
		ptr ++;

	    *ptr = 0;

#if 0
    printf ("%s=%s\n", name, substvars (prj, value));
#endif

	    setvar (prj, name, substvars (prj, value));
	}

	fclose (fh);
    }

    NewList(&deps);
    ForeachNodeSafe (&project->genmakefiledeps, node, next)
    {
	Remove (node);
	AddTail (&deps, node);
    }

    ForeachNodeSafe (&deps, node, next)
    {
	Remove (node);
	dep = newdepnode (substvars (project, node->name));
	AddTail (&project->genmakefiledeps, dep);
	xfree (node->name);
	xfree (node);
    }

    if (debug)
    {
	printf ("project %s.genmfdeps=\n", prj->node.name);
	printlist (&project->genmakefiledeps);
    }

    if (debug)
    {
	printf ("project %s.vars=", prj->node.name);
	printvarlist (&prj->vars);
    }
}

int
execute (Project * prj, const char * cmd, const char * in,
	 const char * out, const char * args)
{
    char buffer[4096];
    char * cmdstr;
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

    cmdstr = substvars (prj, buffer);

    if (verbose)
	printf ("Executing %s...\n", cmdstr);

    rc = system (cmdstr);

    if (rc)
    {
	printf ("%s failed: %d\n", cmdstr, rc);
    }

    return !rc;
}

int
checkdeps (Project * prj, time_t desttime)
{
    Dep * dep;
    int newer = 0;

    ForeachNode (&prj->genmakefiledeps, dep)
    {
	if (dep->time > desttime)
	{
/*printf ("%s is newer\n", dep->node.name);*/
	    newer = 1;
	    break;
	}
    }

    return newer;
}

void
callmake (Project * prj, const char * tname, const char * mforig)
{
    char * mf = xstrdup (substvars (prj, mforig));
    char * dir, * file, * ext, * ptr;
    int t;
    char buffer[4096];

    ptr = dir = mf;
    file = NULL;
    while (*ptr)
    {
	if (*ptr == '/')
	    file = ptr+1;

	ptr ++;
    }
    if (!file)
    {
	dir = ".";
	file = mf;
    }
    ptr = file;
    ext = NULL;
    while (*ptr)
    {
	if (*ptr == '.')
	    ext = ptr+1;

	ptr ++;
    }

    chdir (prj->top);
    if (file != mf)
    {
	file[-1] = 0;
	chdir (dir);
    }

    setvar (prj, "CURDIR", dir);
    setvar (prj, "TARGET", tname);

    *buffer = 0;

    for (t=0; t<mflagc; t++)
    {
	strcat (buffer, mflags[t]);
	strcat (buffer, " ");
    }

    if (strcmp (file, "Makefile") && strcmp (file, "makefile"));
    {
	strcat (buffer, "--file=");
	strcat (buffer, file);
	strcat (buffer, " ");
    }

    strcat (buffer, tname);

    printf ("Making %s in %s\n", tname, dir);

    if (!execute (prj, prj->maketool, "-", "-", buffer))
    {
	error ("Error while running make in %s", dir);
	exit (10);
    }

    free (mf);
}

void
regeneratemf (Project * prj, List * regeneratefiles)
{
    Regenerate * reg,* reg2;
    char tmpname[20];
    int fd;
    FILE *f;

    if (GetHead (regeneratefiles) == NULL)
	return;
    
    strcpy (tmpname, "/tmp/genmfXXXXXX");
    fd = mkstemp (tmpname);
    if (fd < 0)
    {
	error ("Could not create temporary file %s", tmpname);
	exit (10);
    }
    else
    {
	f = fdopen (fd, "w");
	if (f == NULL)
	{
	    error ("Could not open temporary file %s", tmpname);
	    exit (10);
	}
    }
    
    ForeachNodeSafe (regeneratefiles, reg, reg2)
    {
	fprintf (f, "%s/%s %s/%s\n", reg->dir, reg->src, reg->dir, reg->dest);
	Remove (reg);
	xfree (reg->dir);
	xfree (reg->src);
	xfree (reg->dest);
	xfree (reg);
    }

    fclose (f);

    setvar (prj, "MMLIST", tmpname);
    if (!execute (prj, prj->genmakefilescript,"-","-",""))
    {
	error ("Error regenerating makefile");
	exit (10);
    }

    unlink (tmpname);
}


void
maketarget (char * metatarget)
{
    char * pname, * tname, * ptr;
    Project * prj;
    Target * target, * subtarget;
    Node * node;
    Makefile * makefile;
    List regeneratefiles;
    
    pname = ptr = metatarget;
    while (*ptr && *ptr != '.')
	ptr ++;
    if (*ptr)
	*ptr ++ = 0;
    tname = ptr;

    if (!*pname)
    {
	prj = GetHead (&projects);
	if (prj)
	    prj = GetNext (prj);

	pname = prj->node.name;
    }

    prj = FindNode (&projects, pname);

    if (!prj)
    {
	printf ("Nothing known about project %s\n", pname);
	return;
    }

    if (!*tname)
	tname = prj->defaulttarget;

    printf ("Building %s.%s\n", pname, tname);

    chdir (prj->top);

    readvars (prj);
    NewList (&regeneratefiles);
    updatemflist (prj, &regeneratefiles);
    buildmflist (prj, &regeneratefiles);
    regeneratemf (prj, &regeneratefiles);
    buildtargetlist (prj);

    target = FindNode (&prj->targets, tname);

    if (!target)
    {
	printf ("Nothing known about target %s in project %s\n", tname, pname);
	return;
    }

    target->updated = 1;

    ForeachNode (&target->deps, node)
    {
	subtarget = FindNode (&prj->targets, node->name);

	if (!subtarget)
	{
	    printf ("Nothing known about target %s in project %s\n", node->name, pname);
	}
	else if (!subtarget->updated)
	{
	    char buffer[256];
	    strcpy (buffer, pname);
	    strcat (buffer, ".");
	    strcat (buffer, node->name);
	    maketarget (buffer);
	}
    }

    ForeachNode (&target->makefiles, makefile)
    {
	if (!makefile->virtualtarget)
	{
	    callmake (prj, tname, makefile->node.name);
	}
    }
}

int
main (int argc, char ** argv)
{
    Project * prj, * next;
    char * currdir;
    int t;

    currdir = getcwd (NULL, 1024);

    mflagc = targetc = 0;

    for (t=1; t<argc; t++)
    {
	if (argv[t][0] == '-')
	{
	    if (!strcmp (argv[t], "--version"))
	    {
		printf ("MetaMake %s (%s)\n", VERSION, __DATE__);
		if (argc == 2)
		    exit (0);
	    }
	    else if (!strcmp (argv[t], "--verbose") || !strcmp (argv[t], "-v"))
	    {
		verbose = 1;
	    }
	    else if (!strcmp (argv[t], "--debug"))
	    {
		debug = 1;
	    }
	    else if (!strcmp (argv[t], "--help"))
	    {
		printf ("%s [--version] [-v,--verbose] [--debug] [--help]\n", argv[0]);
		return 0;
	    }
	    else
	    {
		mflags[mflagc++] = argv[t];
	    }
	}
	else
	{
	    targets[targetc++] = argv[t];
	}
    }

    init ();

    if (!targetc)
    {
	assert (firstprj);

	targets[targetc++] = firstprj->node.name;
    }

    for (t=0; t<targetc; t++)
    {
	maketarget (targets[t]);
    }

    ForeachNodeSafe (&projects, prj, next)
    {
	Remove (prj);
	freeproject (prj);
    }

    chdir (currdir);

    free (currdir);

    /* Free internal memory */
    getargs (NULL, NULL, NULL, 0);

    return 0;
}

