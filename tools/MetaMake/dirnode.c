/* MetaMake - A Make extension
   Copyright © 1995-2004, The AROS Development Team. All rights reserved.

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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
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
#ifndef PATH_MAX
#include <limits.h>
#endif

#include "dirnode.h"
#include "mem.h"
#include "var.h"
#include "mmake.h"
#include "io.h"

#define FLAG_VIRTUAL	0x0001

static MakefileTarget *
newmakefiletarget (char *name, int virtualtarget)
{
    MakefileTarget * mftarget;
    
    mftarget = newnodesize (name, sizeof(MakefileTarget));
    mftarget->virtualtarget = virtualtarget;
    NewList (&mftarget->deps);
    
    return mftarget;
}

void
freemakefiletarget (MakefileTarget * mftarget)
{
    freelist (&mftarget->deps);
    xfree (mftarget);
}

void
freemakefiletargetlist (List * targets)
{
    MakefileTarget * mftarget, * mftarget2;
    
    ForeachNodeSafe (targets, mftarget, mftarget2)
	freemakefiletarget (mftarget);
    
    NewList (targets);
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
printdirnodemftarget (DirNode * node)
{
    Makefile * makefile;
    MakefileTarget * mftarget;
    Node * dep;
    DirNode * subdir;

    ForeachNode (&node->makefiles, makefile)
    {
	printf ("%s/%s:\n", buildpath(node), makefile->node.name);
	ForeachNode (&makefile->targets, mftarget)
	{
	    printf ("    %s:", mftarget->node.name);
	    ForeachNode (&mftarget->deps, dep)
		printf (" %s", dep->name);
	    printf ("\n");
	}
    }
    
    ForeachNode (&node->subdirs, subdir)
	printdirnodemftarget (subdir);
}

void
freedirnode (DirNode * node)
{
    DirNode * subnode, * subnode2;

    ForeachNodeSafe (&node->subdirs, subnode, subnode2)
	freedirnode (subnode);

    xfree (node->node.name);
    xfree (node);
}

void
freemakefile (Makefile * makefile)
{
    freemakefiletargetlist (&makefile->targets);
    xfree (makefile->node.name);
    xfree (makefile);
}

DirNode *
finddirnode (DirNode * topnode, const char * path)
{
    const char * ptr;
    char dirname[256];
    int len;
    DirNode * node = topnode, * subdir;

    ptr = path+2;

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

int
scandirnode (DirNode * node, const char * mfname, List * ignoredirs)
{
    struct stat st;
    DIR * dirh;
    struct dirent * dirent;
    int mfnamelen = strlen(mfname), scanned = 0;
    
    if (stat(".", &st) != 0)
    {
	error("scandirnode(): scanning %s\n",
	      strlen(node->node.name)==0 ? "topdir" : node->node.name
	);
	exit(20);
    }

    if (st.st_mtime > node->time)
    {
	List newdirs, newmakefiles;
	DirNode * subdir = NULL, * subdir2;
	Makefile * makefile;
	
	if (debug)
	    printf("scandirnode(): scanning %s\n",
		   strlen(node->node.name)==0 ? "topdir" : buildpath(node)
	    );

	NewList (&newdirs);
	NewList (&newmakefiles);
	
	node->time = st.st_mtime;

	dirh = opendir (".");
	if (!dirh)
	{
	    error("opendir: could not open current dir");
	    exit(20);
	}
	
	while ((dirent = readdir (dirh)))
	{
	    /* Add makefile if it present or the file with .src is present */
	    if (strcmp(dirent->d_name, mfname) == 0
		|| (strlen(dirent->d_name) == mfnamelen + 4
		    && strncmp(dirent->d_name, mfname, mfnamelen) == 0
		    && strcmp(dirent->d_name + mfnamelen, ".src") == 0
		)
	    )
	    {
		/* Don't add makefile twice */
		makefile = FindNode (&newmakefiles, mfname);
		if (makefile == NULL)
		{
		    makefile = FindNode (&node->makefiles, mfname);
		    if (makefile != NULL)
		    {
			Remove (makefile);
		    }
		    else
		    {
			makefile = newnodesize (mfname, sizeof (Makefile));
			makefile->dir = node;
			makefile->time = (time_t)0;
			NewList (&makefile->targets);
		    }
		    AddTail (&newmakefiles, makefile);
		}
	    }
	    else
	    {
		/* If the file is already in the makefiles from the current dirnode
		 * and it is still present in the directory copy it to the new makefiles
		 * list
		 */
		if (strlen (dirent->d_name) > 4
		    && strcmp (dirent->d_name + strlen(dirent->d_name) - 4, ".src") == 0
		)
		{
		    dirent->d_name[strlen(dirent->d_name) - 4] = 0;
		    makefile = FindNode (&node->makefiles, dirent->d_name);
		    dirent->d_name[strlen(dirent->d_name)] = '.';
		}
		else
		    makefile = FindNode (&node->makefiles, dirent->d_name);
		
		if (makefile != NULL)
		{
		    Remove (makefile);
		    AddTail (&newmakefiles, makefile);
		}


		/* Add file to newsubdirs if it is a directory and it has not to be ignored
		 */
		if (lstat (dirent->d_name, &st) == -1)
		{
		    error ("scandirnode: stat(%s)", dirent->d_name);
		    exit(20);
		}

		if (S_ISDIR (st.st_mode)
		    && strcmp (dirent->d_name, ".") != 0
		    && strcmp (dirent->d_name, "..") != 0
		    && !S_ISLNK (st.st_mode)
		    && !FindNode (ignoredirs, dirent->d_name)
		)
		{
		    subdir = FindNode (&node->subdirs, dirent->d_name);
		    
		    if (subdir != NULL)
		    {
			Remove (subdir);
		    }
		    else
		    {
			subdir = newnodesize (dirent->d_name, sizeof(DirNode));
			subdir->parent = node;
			subdir->time = (time_t)0;
			NewList (&subdir->subdirs);
			NewList (&subdir->makefiles);
		    }
		    AddTail (&newdirs, subdir);
		}
	    }
	}
	closedir (dirh);
	
	ForeachNodeSafe (&node->subdirs, subdir, subdir2)
	    freedirnode  (subdir);
	AssignList (&node->subdirs, &newdirs);

	/* Clear the makefiles that have disappeared */
	ForeachNode (&node->makefiles, makefile)
	{
	    MakefileTarget * mftarget;
	    
	    ForeachNode (&makefile->targets, mftarget)
		freelist (&mftarget->deps);
	    
	    freelist (&makefile->targets);
	}
	freelist (&node->makefiles);
	
	AssignList (&node->makefiles, &newmakefiles);
	
	scanned = 1;
    }
    
#if 0
    printf ("scandirnode()\n");
    printdirnode (node);
#endif
    
    return scanned;
}

int
scanmakefiles (DirNode * node, List * vars)
{
    Makefile * makefile;
    struct stat st;
    static char * line = NULL;
    static int linelen = 512;
    int reread = 0;
    FILE * fh;
    
    assert (node);

    if (line == NULL)
	line = xmalloc(linelen);
    
    ForeachNode (&node->makefiles, makefile)
    {
	if (stat(makefile->node.name, &st) != 0)
	{
	    error("Could not stat %s", makefile->node.name);
	    exit(20);
	}
	
	if (st.st_mtime > makefile->time)
	{
	    int flags = 0;
	    int lineno = 0;
	    MakefileTarget * mftarget = NULL;
	    
	    if (debug)
		printf("scanmakefiles(): scanning makefile in %s/%s\n",
		       strlen(node->node.name)==0 ? "topdir" : buildpath(node),
		       makefile->node.name
		);

#if 0
printf ("Opening %s\n", makefile->name);
#endif

	    fh = fopen (makefile->node.name, "r");
	    if (!fh)
	    {
		error ("buildtargetlist:fopen(): Opening %s for reading",
		       makefile->node.name
		);
	    }

	    /* Free old metatargets when the file is reread */
	    freemakefiletargetlist (&makefile->targets);
	    NewList (&makefile->targets);

	    while (fgets (line, linelen, fh))
	    {
		lineno ++;

		while (line[strlen(line)-1] != '\n' && !feof(fh))
		{
		    char * ptr;
		    
		    linelen += 512;
		    ptr = xmalloc (linelen);
		    strcpy (ptr, line);
		    xfree (line);
		    line = ptr;
		    fgets (line+strlen(line), linelen-strlen(line), fh);
		}

		if (line[strlen(line)-1] == '\n')
		{
		    line[strlen(line)-1] = 0;
		}

		if (strncmp (line, "#MM", 3) == 0)
		{
		    char * ptr;
		    int count, t;
		    
#if 0
printf ("found #MM in %s\n", makefile->name);
#endif

		    /* Read in next lines if there is continuation */
		    while (line[strlen(line)-1] == '\\')
		    {
			ptr = line + strlen(line) - 1;
			
			if (!fgets (ptr, linelen-strlen(line)+1, fh))
			{
			    error("%s/%s:unexpected end of makefile",
				  getcwd(NULL, 0),
				  makefile->node.name
			    );
			    exit(20);
			}

			lineno ++;
			
			while (line[strlen(line)-1] != '\n' && !feof(fh))
			{
			    int pos = ptr - line;
			    linelen += 512;
			    ptr = xmalloc (linelen);
			    strcpy (ptr, line);
			    xfree (line);
			    line = ptr;
			    fgets (line+strlen(line), linelen-strlen(line), fh);
			    ptr = line + pos;
			}

			if (line[strlen(line)-1] == '\n')
			{
			    line[strlen(line)-1] = 0;
			}

			if (strncmp (ptr, "##MM", 3) == 0)
			{
			    *ptr = line[strlen(line)-1];
			    ptr[1] = 0;
			    continue;
			}
			
			if (strncmp (ptr, "#MM", 3) != 0)
			{
			    errno = 0;
			    error("%s/%s:%d:continuation line has to start with #MM",
				  getcwd (NULL, 0),
				  makefile->node.name,
				  lineno
			    );
			    exit(20);
			}
			
			memmove (ptr, ptr+4, strlen(ptr)-4+1);
		    }

		    ptr = line+3;

		    if (*ptr == '-')
		    {
			flags |= FLAG_VIRTUAL;
			ptr ++;
		    }
		    else
			flags &= ~FLAG_VIRTUAL;
		    
		    while (isspace (*ptr))
			ptr ++;

		    if (!*ptr)
		    {
			/* Line with only #MM, metatarget is on next line */
			char ** targets;
			fgets (line, linelen, fh);
			lineno ++;

			ptr = line;
			while (*ptr != ':' && *ptr)
			    ptr ++;

			*ptr = 0;

			targets = getargs (line, &count, NULL);

			if (count != 0)
			{
			    mftarget = FindNode (&makefile->targets, targets[0]);
			    
			    if (mftarget == NULL)
			    {
				mftarget = newmakefiletarget (targets[0], 0);
				AddTail (&makefile->targets, mftarget);
			    }
			    else
				mftarget->virtualtarget = 0;
			}
			else
			    printf ("Warning: Can't find metatarget in %s:%d\n", makefile->node.name, lineno);
		    }
		    else
		    {
			List newtargets;
			char * ptr2 = ptr, ** tptr;
			MakefileTarget * mftarget2, * mftarget3;
			
			NewList (&newtargets);
			
			while (*ptr2 != ':' && *ptr2)
			    ptr2 ++;
			if (*ptr2)
			    *ptr2 ++ = 0;

			tptr = getargs (ptr, &count, NULL);
			
			for (t = 0; t < count; t++)
			{
			    mftarget = newmakefiletarget (tptr[t], (flags & FLAG_VIRTUAL) != 0);
			    AddTail (&newtargets, mftarget);
			}
			
			tptr = getargs (ptr2, &count, NULL);
			for (t = 0; t < count; t++)
			{
			    ForeachNode (&newtargets, mftarget)
				addnodeonce (&mftarget->deps, tptr[t]);
			}
			
			ForeachNodeSafe (&newtargets, mftarget, mftarget2)
			{
			    mftarget3 = FindNode (&makefile->targets, mftarget->node.name);

			    /* mftarget doesn't exists yet add it to targets */
			    if (mftarget3 == NULL)
			    {
				Remove (mftarget);
				AddTail (&makefile->targets, mftarget);
			    }
			    else
			    {
				/* Merge data in mftarget into mftarget3 */
				Node * node;
				
				mftarget3->virtualtarget =  mftarget3->virtualtarget && mftarget->virtualtarget;
				
				ForeachNode (&mftarget->deps, node)
				    addnodeonce (&mftarget3->deps, node->name);
			    }
			    /* Free the targets from which the data was merged in other targets */
			    freemakefiletargetlist (&newtargets);
			}
		    }
		} /* If this is a MetaMake line in the makefile */
	    } /* For all lines in a makefile */
	    
	    reread ++;
	    makefile->time = st.st_mtime;
#if 0
printf ("Read %d lines\n", lineno);
#endif
	    
	    fclose (fh);
	} /* If the makefile needed to be scanned */
    } /* For all makefiles in the project */

    return reread;
}

Makefile *
addmakefile (DirNode * node, const char * filename)
{
    static char curdir[PATH_MAX];
    const char * ptr = filename;
    char * name;
    int len = 0;
    DirNode * subnode;
    Makefile * makefile = NULL;

    getcwd(curdir, PATH_MAX);
    
    while (ptr != NULL)
    {
	len = 0;
	while (ptr[len] != '/' && ptr[len] != 0)
	    len++;

	name = xmalloc (len+4+1); /* Make room for possibly adding .src at the end */
	strncpy (name, ptr, len);
	name[len] = 0;
	
	if (ptr[len] == '/')
	{
	    subnode = FindNode (&node->subdirs, name);
	    if (subnode == NULL)
	    {
		xfree(name);
		chdir (curdir);
		return NULL;
	    }
	    chdir (name);
	    node = subnode;
	    ptr = ptr + len + 1;
	}
	else
	{
	    if (len >= 4 && strcmp (name+len-4, ".src") == 0)
		name[len-4] = 0;

	    makefile = FindNode (&node->makefiles, name);
	    
	    if (makefile == NULL)
	    {
		struct stat st;

		printf ("Trying to stat %s\n", name);
		
		if (stat(name, &st) != 0)
		{
		    len = strlen(name);
		    strcat (name, ".src");
		    if (stat (name, &st) != 0)
		    {
			xfree (name);
			chdir (curdir);
			return NULL;
		    }
		    name[len]=0;
		}

		makefile = newnodesize (name, sizeof (Makefile));
		makefile->dir = node;
		makefile->time = (time_t)0;
		NewList (&makefile->targets);
		AddTail (&node->makefiles, makefile);
	    }
	    
	    ptr = NULL;
	}
	
	xfree (name);
    }

    chdir (curdir);
    
    return makefile;
}

Makefile *
findmakefile (DirNode * node, const char *filename)
{
    const char * ptr = filename;
    char * name;
    int len;
    DirNode * subnode;
    Makefile * makefile = NULL;

    while (ptr != NULL)
    {
	len = 0;
	while (ptr[len] != '/' && ptr[len] != 0)
	    len++;

	name = xstrndup (ptr, len);
	name[len] = 0;
	
	if (ptr[len] == '/')
	{
	    subnode = FindNode (&node->subdirs, name);
	    if (subnode == NULL)
	    {
		xfree(name);
		return NULL;
	    }
	    node = subnode;
	    ptr = ptr + len + 1;
	}
	else
	{
	    if (len >= 4 && strcmp (name+len-4, ".src") == 0)
		name[len-4] = 0;

	    makefile = FindNode (&node->makefiles, name);
	    ptr = NULL;
	}
	
	xfree (name);
    }

    return makefile;
}
    
typedef struct {
    Node node;
    DirNode * dirnode;
}
DirNodeRef;

const char *
buildpath (DirNode * node)
{
    static char path[PATH_MAX];
    List tree;
    DirNodeRef * ref = NULL;
    
    NewList (&tree);

    do
    {
	if (strlen (node->node.name) > 0)
	{
	    ref = newnodesize ("", sizeof (DirNodeRef));
	    ref->dirnode = node;
	    AddHead (&tree, ref);
	}
	node = node->parent;
    } while (node != NULL);
    
    strcpy (path, "");
    ForeachNode (&tree, ref)
    {
	if (path[0] != 0)
	    strcat (path, "/");
	strcat (path, ref->dirnode->node.name);
    }

    freelist (&tree);
    
    return path;
}

Makefile *
readmakefile (FILE * fh)
{
    Makefile * makefile;
    MakefileTarget * mftarget;
    Node * n;
    uint32_t intt;
    char * s;

    if (!readstring(fh, &s))
    {
	error ("readmakefile:readstring():%d", __LINE__);
	exit (20);
    }

    if (s == NULL)
	return NULL;

    makefile = newnodesize(s, sizeof(Makefile));
    xfree(s);
    NewList(&makefile->targets);

    if (!readuint32 (fh, &intt)) 
    {
	error ("readmakefile:fread():%d", __LINE__);
	exit (20);
    }
    makefile->time = intt;

    for (;;)
    {
	int32_t in;

	if (!readstring(fh, &s))
	{
	    error ("readmakefile:readstring():%d", __LINE__);
	    exit (20);
	}

	if (s == NULL)
	    break;

	mftarget = newnodesize(s, sizeof(MakefileTarget));
	xfree(s);
	AddTail (&makefile->targets, mftarget);
	NewList (&mftarget->deps);

	if (!readint32 (fh, &in))
	{
	    error ("readmakefile:fread():%d", __LINE__);
	    exit (20);
	}
	mftarget->virtualtarget = in;
	
	for (;;)
	{
	    if (!readstring(fh, &s))
	    {
		error ("readmakefile:readstring():%d", __LINE__);
		exit (20);
	    }
	    
	    if (s == NULL)
		break;

	    n = newnode(s);
	    xfree(s);
	    AddTail (&mftarget->deps, n);
	}
    }
    
    return makefile;
}

int
writemakefile (FILE * fh, Makefile * makefile)
{
    MakefileTarget * mftarget;
    Node * n;

    if (makefile == NULL)
    {
	if (!writestring (fh, NULL))
	{
	    error ("writemakefile/writestring():%d", __LINE__);
	    return 0;
	}
	    
	return 1;
    }
    
    if (!writestring (fh, makefile->node.name))
    {
	error ("writemakefile/writestring():%d", __LINE__);
	return 0;
    }

    if (!writeuint32 (fh, makefile->time))
    {
	error ("writemakefile/fwrite():%d", __LINE__);
	return 0;
    }

    ForeachNode (&makefile->targets, mftarget)
    {
	if (!writestring (fh, mftarget->node.name))
	{
	    error ("writemakefile/writestring():%d", __LINE__);
	    return 0;
	}

	if (!writeint32 (fh, mftarget->virtualtarget))
	{
	    error ("writemakefile/fwrite():%d", __LINE__);
	    return 0;
	}

	ForeachNode (&mftarget->deps, n)
	{
	    if (!writestring (fh, n->name))
	    {
		error ("writemakefile/writestring():%d", __LINE__);
		return 0;
	    }
	}

	if (!writestring (fh, NULL))
	{
	    error ("writemakefile/writestring():%d", __LINE__);
	    return 0;
	}
    }

    if (!writestring(fh, NULL))
    {
	error ("writemakefile/writestring():%d", __LINE__);
	return 0;
    }
    
    return 1;
}

DirNode *
readcachedir (FILE * fh)
{
    DirNode * node, * subnode;
    Makefile * makefile;
    uint32_t intt;
    char * s;

    if (!readstring(fh, &s))
    {
	error ("readcachedir:readstring():%d", __LINE__);
	return NULL;
    }

    if (s == NULL)
	return NULL;

    node = newnodesize(s, sizeof(DirNode));
    xfree (s);
    NewList(&node->makefiles);
    NewList(&node->subdirs);

    if (!readuint32 (fh, &intt))
    {
	error ("readcachedir:fread():%d", __LINE__);
	free (node);
	return NULL;
    }
    node->time = intt;

    while ((makefile = readmakefile (fh)))
    {
	makefile->dir = node;
	AddTail (&node->makefiles, makefile);
    }
    
    while ((subnode = readcachedir (fh)))
    {
	subnode->parent = node;
	AddTail (&node->subdirs, subnode);
    }

    return node;
}

int
writecachedir (FILE * fh, DirNode * node)
{
    int out;
    DirNode * subnode;
    Makefile * makefile;

    if (node == NULL)
    {
	if (!writestring(fh, NULL))
	{
	    error ("writecachedir/writestring():%d", __LINE__);
	    return 0;
	}

	return 1;
    }
    
    if (!writestring(fh, node->node.name))
    {
	error ("writecachedir/writestring():%d", __LINE__);
	return 0;
    }

    if (!writeuint32 (fh, node->time))
    {
	error ("writecachedir/fwrite():%d", __LINE__);
	return 0;
    }

    ForeachNode (&node->makefiles, makefile)
	writemakefile (fh, makefile);

    if (!writemakefile (fh, NULL))
	return 0;

    ForeachNode (&node->subdirs, subnode)
    {
	if (!writecachedir (fh, subnode))
	    return 0;
    }

    return writecachedir (fh, NULL);
}
