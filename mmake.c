/*
    0.2 - Got rid of external program find
	- Some bugfixes
	- Renamed some variables

    0.1 - First version
*/
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

struct Node
{
    struct Node * next,
		* prev;
    char	* name;
};

struct List
{
    struct Node * first,
		* last,
		* prelast;
};

#   define NEWLIST(l)       (((struct List *)l)->prelast \
				= (struct Node *)(l), \
			    ((struct List *)l)->last = 0, \
			    ((struct List *)l)->first \
				= (struct Node *)\
				    &(((struct List *)l)->last))

#   define ADDHEAD(l,n)     ((void)(\
	((struct Node *)n)->next          = ((struct List *)l)->first, \
	((struct Node *)n)->prev          = (struct Node *)&((struct List *)l)->first, \
	((struct List *)l)->first->prev = ((struct Node *)n), \
	((struct List *)l)->first          = ((struct Node *)n)))

#   define ADDTAIL(l,n)     ((void)(\
	((struct Node *)n)->next              = (struct Node *)&((struct List *)l)->last, \
	((struct Node *)n)->prev              = ((struct List *)l)->prelast, \
	((struct List *)l)->prelast->next = ((struct Node *)n), \
	((struct List *)l)->prelast          = ((struct Node *)n) ))

#   define REMOVE(n)        ((void)(\
	((struct Node *)n)->prev->next = ((struct Node *)n)->next,\
	((struct Node *)n)->next->prev = ((struct Node *)n)->prev ))

#   define GetHead(l)       (void *)(((struct List *)l)->first->next \
				? ((struct List *)l)->first \
				: (struct Node *)0)
#   define GetTail(l)       (void *)(((struct List *)l)->prelast->prev \
				? ((struct List *)l)->prelast \
				: (struct Node *)0)
#   define GetSucc(n)       (void *)(((struct Node *)n)->next->next \
				? ((struct Node *)n)->next \
				: (struct Node *)0)
#   define GetPred(n)       (void *)(((struct Node *)n)->prev->prev \
				? ((struct Node *)n)->prev \
				: (struct Node *)0)
#   define ForeachNode(l,n) \
	for (n=(void *)(((struct List *)(l))->first); \
	    ((struct Node *)(n))->next; \
	    n=(void *)(((struct Node *)(n))->next))

char * xstrdup (const char * str)
{
    assert (str);

    return strdup (str);
}

void * xmalloc (size_t size)
{
    void * ptr;

    ptr = malloc (size);

    if (size && !ptr)
    {
	fprintf (stderr, "Out of memory");
	exit (20);
    }

    return ptr;
}

void xfree (void * ptr)
{
    assert (ptr);

    free (ptr);
}

struct Node * FindNode (const struct List * l, const char * name)
{
    struct Node * n;

    ForeachNode (l, n)
    {
	if (!strcmp (n->name, name))
	    return n;
    }

    return NULL;
}

void error (char * fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    fprintf (stderr, "Error: ");
    vfprintf (stderr, fmt, args);
    fprintf (stderr, ": %s\n", strerror (errno));
    va_end (args);
}

typedef struct
{
    struct Node node;

    char * maketool;
    char * defaultmakefilename;
    char * top;
    char * defaulttarget;
    char * genmakefilescript;
    char * genmakefiledeps;
    char * globalvarfile;

    int readvars;
    int buildmflist;
    int buildtargetlist;

    struct List ignoredirs;
    struct List vars;
    struct List makefiles;
    struct List targets;
}
Project;

void setvar (Project *, const char *, const char *);

typedef struct
{
    struct Node node;
    char      * value;
}
Var;

typedef struct
{
    struct Node node;
    int updated;

    struct List makefiles;
    struct List deps;
}
Target;

void printlist (struct List * l)
{
    struct Node * n;

    ForeachNode (l,n)
    {
	printf ("    %s\n", n->name);
    }
}

void printvarlist (struct List * l)
{
    Var * n;

    ForeachNode (l,n)
    {
	printf ("    %s=%s\n", n->node.name, n->value);
    }
}

void printtargetlist (struct List * l)
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

Var * newnode (const char * name, const char * value)
{
    Var * n;

    assert (name);

    n = malloc (sizeof (Var));

    n->node.name = xstrdup (name);
    n->value = value ? xstrdup (value) : NULL;

    return n;
}

Var * addnodeonce (struct List * l, const char * name, const char * value)
{
    Var * n;

    n = (Var *)FindNode (l, name);

    if (n)
    {
	if (n->value)
	    free (n->value);

	n->value = value ? xstrdup (value) : NULL;
    }
    else
    {
	n = newnode (name, value);
	ADDTAIL(l,n);
    }

    return n;
}

struct List projects;

char * getvar (Project * prj, const char * varname)
{
    static char buffer[256];
    Var * var = (Var *)FindNode (&prj->vars, varname);

    if (var)
	return var->value;

    sprintf (buffer, "?$(%s)", varname);
    return buffer;
}

char * substvars (Project * prj, const char * str)
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

char ** getargs (Project * prj, const char * line, int * argc)
{
    static char * argv[64];
    static char * buffer = NULL;
    char * src;
    int arg;

    if (buffer)
	free (buffer);

    assert (line);

    buffer = xstrdup (substvars (prj, line));

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

    if (*argc)
	*argc = arg;

    return argv;
}

Project * defaultprj, * project;
Project * firstprj;

Project * initproject (char * name)
{
    Project * prj = malloc (sizeof (Project));

    assert (prj);

    if (!defaultprj)
    {
	prj->maketool = xstrdup ("make");
	prj->defaultmakefilename = xstrdup ("Makefile");
	prj->top = getcwd (NULL, 1024);
	prj->defaulttarget = xstrdup ("all");
	prj->genmakefilescript = NULL;
	prj->genmakefiledeps = NULL;
	prj->globalvarfile = NULL;
    }
    else
    {
	prj->maketool = xstrdup (defaultprj->maketool);
	prj->defaultmakefilename = xstrdup (defaultprj->defaultmakefilename);
	prj->top = xstrdup (defaultprj->top);
	prj->defaulttarget = xstrdup (defaultprj->defaulttarget);
	prj->genmakefilescript = defaultprj->genmakefilescript
	    ? xstrdup (defaultprj->genmakefilescript) : NULL;
	prj->genmakefiledeps = defaultprj->genmakefiledeps
	    ? xstrdup (defaultprj->genmakefiledeps) : NULL;
	prj->globalvarfile = defaultprj->globalvarfile
	    ? xstrdup (defaultprj->globalvarfile) : NULL;
    }

    prj->node.name = xstrdup (name);

    prj->readvars = 1;
    prj->buildmflist = 1;
    prj->buildtargetlist = 1;

    NEWLIST(&prj->ignoredirs);
    NEWLIST(&prj->vars);
    NEWLIST(&prj->makefiles);
    NEWLIST(&prj->targets);

    return prj;
}

#define SETSTR(str,val) \
    if (str) free (str); \
    str = val ? xstrdup (val) : NULL

void init (void)
{
    char * optionfile;
    char * home = getenv("HOME");
    char line[256];
    FILE * optfh;

    NEWLIST(&projects);
    defaultprj = project = initproject ("default");
    ADDTAIL(&projects, project);

    if ((optionfile = getenv ("MMAKE_CONFIG")))
    {
	optionfile = strdup (optionfile);
    }
    else
    {
	optionfile = malloc (strlen(home)+sizeof("/.mmake.config")+1);
	sprintf (optionfile, "%s/.mmake.config", home);
    }

    optfh = fopen (optionfile, "r");

    if (!optfh)
    {
	free (optionfile);
	optionfile = strdup (".mmake.config");

	optfh = fopen (optionfile, "r");
    }

    if (!optfh)
    {
	free (optionfile);
	optionfile = strdup ("mmake.config");

	optfh = fopen (optionfile, "r");
    }

    if (!optfh)
    {
	error ("Opening %s for reading", optionfile);
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

	    ADDTAIL(&projects,project);
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
		struct Node * n;
		n = newnode(args,NULL);
		ADDTAIL(&project->makefiles, n);
	    }
	    else if (!strcmp (cmd, "ignoredir"))
	    {
		struct Node * n;
		n = newnode(args,NULL);
		ADDTAIL(&project->ignoredirs, n);
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
		SETSTR(project->genmakefiledeps,args);
	    }
	    else if (!strcmp (cmd, "globalvarfile"))
	    {
		SETSTR(project->globalvarfile,args);
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
    free (optionfile);

#if 0
    printf ("known projects: ");
    printlist (&projects);
#endif
}

char * mflags[64];
int mflagc;
char * targets[64];
int targetc;

void buildmflist (Project * prj)
{
    char * mfn, * mfnsrc;
    struct stat st;
    char path[256];
    int len, offset;
    struct List dirs;
    struct Node * cd;
    DIR * dirh;
    struct dirent * dirent;
    int foundmf;

    if (!prj->buildmflist)
	return;

    prj->buildmflist = 0;

    printf ("Collecting makefiles...\n");

    mfnsrc = malloc (strlen(mfn=prj->defaultmakefilename)+5);
    assert (mfnsrc);
    strcpy (mfnsrc, mfn);
    len = strlen (mfn);
    strcpy (mfnsrc+len, ".src");

    NEWLIST(&dirs);
    cd = (struct Node *)newnode (".", NULL);
    ADDTAIL(&dirs,cd);

    while ((cd = GetHead(&dirs)))
    {
	REMOVE (cd);

	chdir (prj->top);

	strcpy (path, cd->name);
	offset = strlen (path);
	path[offset ++] = '/';
	path[offset] = 0;

/* printf ("Entering \"%s\"\n", path); */

	dirh = opendir (path);
	if (!dirh)
	{
	    error ("opendir(%s)", path);
	    exit (10);
	}

	foundmf = 0;

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
/* printf ("Adding %s for later\n", path); */
	    }

	    path[offset] = 0;
	}

	if (foundmf == 2)
	{
	    strcpy (path+offset, mfnsrc);
	    addnodeonce (&prj->makefiles, path+2, NULL);
	    path[offset] = 0;
	}
	else if (foundmf == 1)
	{
	    mfnsrc[len] = 0;
	    strcpy (path+offset, mfnsrc);
	    addnodeonce (&prj->makefiles, path+2, NULL);
	    path[offset] = 0;
	    mfnsrc[len] = '.';
	}

	closedir (dirh);

	free (cd);
    }

#if 0
    printf ("project %s.makefiles=", prj->node.name);
    printlist (&prj->makefiles);
#endif
}

void progress (int max, int curr, int * data)
{
    int x = curr*10/max;

    if (x != *data)
    {
	*data = x;
	putchar ('.');
	fflush (stdout);
    }
}

void appendtarget (Project * prj, const char * tname, const char * mf, char ** deps)
{
    Target * target;

    assert (tname);
    assert (mf);

    target = (Target *)FindNode (&prj->targets, tname);

    if (!target)
    {
	target = malloc (sizeof(Target));
	target->node.name = strdup (tname);
	ADDTAIL(&prj->targets, target);
	NEWLIST(&target->makefiles);
	NEWLIST(&target->deps);
	target->updated = 0;
    }

    addnodeonce(&target->makefiles, mf, NULL);

#if 1
printf ("add %s.%s mf=%s\n", prj->node.name, target->node.name, mf);
#endif

    if (deps)
    {
	while (*deps)
	{
#if 1
printf ("   add dep %s\n", *deps);
#endif
	    addnodeonce (&target->deps, *deps, NULL);
	    deps ++;
	}
    }
}

void setvar (Project * prj, const char * name, const char * val)
{
    Var * var;

    assert (name);

#if 0
    printf ("assign %s=%s\n", name, val);
#endif

    var = (Var *)FindNode (&prj->vars, name);

    if (!var)
    {
	var = malloc (sizeof (Var));
	var->node.name = xstrdup (name);
	var->value = NULL;
	ADDTAIL(&prj->vars, var);
    }

    SETSTR (var->value, val);

#if 0
    printf ("%s.vars=", prj->node.name);
    printvarlist (&prj->vars);
#endif
}

void buildtargetlist (Project * prj)
{
    int max, pos, data;
    struct Node * mfnode;
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
	pos++;
	progress (max,pos,&data);

	fh = fopen (mfnode->name, "r");
	lineno = 0;

	while (fgets (line, sizeof(line), fh))
	{
	    lineno ++;

	    if (!strncmp (line, "#MM", 3))
	    {
		char * ptr;
		char * depptr, ** deps;
		int count, depc, t;

		ptr = substvars (prj, line+3);
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

		    targets = getargs (prj, line, &count);

		    if (count != 0)
			appendtarget (prj, targets[0], mfnode->name, NULL);
		    else
			printf ("Warning: Can't find metatarget in %s:%d\n", mfnode->name, lineno);
		}
		else
		{
		    char * targets[64], ** tptr;
		    char * lptr = ptr;

		    while (*ptr != ':' && *ptr)
			ptr ++;
		    if (*ptr)
			*ptr ++ = 0;
		    depptr = ptr;

		    tptr = getargs (prj, lptr, &count);
		    for (t=0; t<count; t++)
			targets[t] = xstrdup (tptr[t]);
		    deps = getargs (prj, depptr, &depc);

		    for (t=0; t<count; t++)
		    {
			appendtarget (prj, targets[t], mfnode->name, deps);
		    }

		    for (t=0; t<count; t++)
			xfree (targets[t]);
		}
	    }
	}

	fclose (fh);
    }

    putchar ('\n');

#if 0
    printtargetlist (&prj->targets);
#endif
}

void readvars (Project * prj)
{
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

	fn = substvars (prj, prj->globalvarfile);
	fh = fopen (fn, "r");

	if (!fh)
	{
	    error ("Opening %s for reading", fn);
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

	    setvar (prj, name, substvars (prj, value));
	}

	fclose (fh);
    }

#if 0
    printf ("project %s.vars=", prj->node.name);
    printvarlist (&prj->vars);
#endif
}

int execute (Project * prj, const char * cmd, const char * in,
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

#if 0
    printf ("Executing %s...\n", cmdstr);
#endif

    rc = system (cmdstr);

    return !rc;
}

void callmake (Project * prj, const char * tname, const char * mforig)
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
    if (*dir)
    {
	file[-1] = 0;
	chdir (dir);
    }

    if (ext && !strcmp (ext, "src"))
    {
	char * src, * dest;
	char * depfile;
	struct stat sst, dst, depst;

	ext[-1] = 0;
	dest = xstrdup (file);
	ext[-1] = '.';

	src = file;
	stat (src, &sst);

	if (prj->genmakefiledeps && *prj->genmakefiledeps)
	{
	    depfile = xstrdup (substvars (prj, prj->genmakefiledeps));
	    stat (depfile, &depst);
	}
	else
	    depfile = NULL;

	if (stat (dest, &dst) == -1
	    || sst.st_mtime > dst.st_mtime
	    || (depfile && depst.st_mtime > dst.st_mtime)
	)
	{
	    if (!execute (prj, prj->genmakefilescript,"-",dest,src))
	    {
		fprintf (stderr, "Error while regenerating makefile %s\n", dest);
		exit (10);
	    }
	}

	if (depfile)
	    free (depfile);
    }

    setvar (prj, "CURDIR", dir);

    if (ext)
	ext[-1] = 0;

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
	fprintf (stderr, "Error while running make in %s\n", dir);
	exit (10);
    }

    free (mf);
}

void maketarget (char * metatarget)
{
    char * pname, * tname, * ptr;
    Project * prj;
    Target * target, * subtarget;
    struct Node * node;

    pname = ptr = metatarget;
    while (*ptr && *ptr != '.')
	ptr ++;
    if (*ptr)
	*ptr ++ = 0;
    tname = ptr;

    prj = (Project *)FindNode (&projects, pname);

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
    buildmflist (prj);
    buildtargetlist (prj);

    target = (Target *)FindNode (&prj->targets, tname);

    if (!target)
    {
	printf ("Nothing known about target %s in project %s\n", tname, pname);
	return;
    }

    target->updated = 1;

    ForeachNode (&target->deps, node)
    {
	subtarget = (Target *)FindNode (&prj->targets, node->name);

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

    ForeachNode (&target->makefiles, node)
    {
	callmake (prj, tname, node->name);
    }
}

int main (int argc, char ** argv)
{
    char * currdir;
    int t;

    currdir = getcwd (NULL, 1024);

    init ();

    mflagc = targetc = 0;

    for (t=1; t<argc; t++)
    {
	if (argv[t][0] == '-')
	{
	    mflags[mflagc++] = argv[t];
	}
	else
	{
	    targets[targetc++] = argv[t];
	}
    }

    if (!targetc)
    {
	assert (firstprj);

	targets[targetc++] = firstprj->node.name;
    }

    for (t=0; t<targetc; t++)
    {
	maketarget (targets[t]);
    }

    chdir (currdir);

    free (currdir);

    return 0;
}



