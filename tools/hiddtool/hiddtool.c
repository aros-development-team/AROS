/* Hiddtool
   Copyright © 1995-2001, The AROS Development Team. All rights reserved.

This file is part of Hiddtool

Hiddtool is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

Hiddtool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */
/* Includes */
// #include "config.h"


#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#define TRUE 1
#define FALSE 0

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
    List deps;
    int circularcheck; /* Used when checking for circular dependencies */
    
} Item;

typedef struct {

    List items;
} DepsDB;

typedef struct {
    Node node;
    int opt;
} Conf;

enum {
    OPT_KERNEL,
    OPT_DISK
};

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
				
#define InsertBefore(n, before)		\
	((Node *)before)->prev->next = n;		\
	((Node *)n)->prev = ((Node *)before)->prev;	\
	((Node *)before)->prev = n;			\
	((Node *)n)->next	= before;
	
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

Node *
findnode (const List * l, const char * name)
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
printlist (List * l)
{
    Node * n;

    ForeachNode (l,n)
    {
	printf ("    \"%s\"\n", n->name);
    }
}




void cleanup(int exitcode)
{
     exit(exitcode);
}

void usage()
{
    fprintf(stderr, "Usage: hiddtool -k configfile: get list of all kernel hidds\n");
    fprintf(stderr, "       hiddtool -d configfile: get list of all disk hidds\n"); 
    fprintf(stderr, "       hiddtool -m configfile: generate metamake dependancies\n");
    cleanup(1);
}

/* Returns pointer to next noblank car or NULL if end of buffer is reached */
char *next_not_blank(char *s)
{
    while (*s && isspace(*s)) {
    	s ++;
    }
    if (0 == *s)
    	s = NULL;
    
    return s;
}

/* Returns pointer to end of word or NULL on error */
char *end_of_word(char *word)
{
    char *s = word;
    while (*s && (isalpha(*s) || *s == '-')) {
    	s ++;
    }
    
    if (s == word)	/* Target was of length 0 ? */
    	s = NULL;
    
    return s;
}

void print_deps_db(DepsDB *db)
{
    Item *item;
    
    ForeachNode(&db->items, item) {
    	Node *depnode;
	
	printf("%s :", item->node.name);
	
	ForeachNode(&item->deps, depnode) {
	    printf(" %s", depnode->name);
	}
	
	printf("\n");
    }
}

static inline void freenode(Node *n)
{
    if (NULL != n->name) {
    	xfree(n->name);
	n->name = NULL;
    }

    xfree(n);
}

void free_deps_db(DepsDB *db)
{
    Item *item, *safeitem;
    
    ForeachNodeSafe(&db->items, item, safeitem) {
    	Node *depnode, *safedepnode;
	
	ForeachNodeSafe(&item->deps, depnode, safedepnode) {
	    freenode(depnode);
	}
	freenode((Node *)item);
    }
    
    xfree(db);
}


DepsDB *build_deps_db(char *filename)
{
    DepsDB *db;
    FILE *f;
    int ok = 1;
     
    db = xmalloc(sizeof (*db));
    NewList(&db->items);
     
    f = fopen(filename, "r");
    if (NULL == f) {
	fprintf(stderr, "Could not open dependancy db file %s\n", filename);
	ok = FALSE;
    } else {
    	/* Read in all the dependancies */
	char buf[500];
    	int line;
	
	line = 0;
	while(ok) {
	    char *s;
	    char *target, *target_end;
	    Item *item;
	    int done = FALSE;
	    if (NULL == fgets(buf, sizeof (buf), f))
	    	break;
	    line ++;
	    
	    s = buf;
		
	    /* Parse the contents in the buffer */
	    target = s = next_not_blank(s);
	    if (NULL == target)
	    	continue;
	
	    s = end_of_word(s);
	    if (NULL == s) {
	    	fprintf(stderr, "Malformed target at line %d in %s\n", line, filename);
		ok = FALSE;
		break;
	    }
	    target_end = s;
	    
	    /* Go to next not blanke */
	    s = next_not_blank(s);
	    if (NULL == s) {
	    	fprintf(stderr, "Missing colon at line %d in %s\n", line, filename);
		ok = FALSE;
		break;
	    }
	    
	    if (*s != ':') {
	    	fprintf(stderr, "Rubbish instead of colon at line %d in %s\n", line, filename);
		ok = FALSE;
		break;
	    }
	    
	    s ++;
	    
	    *target_end = 0;
	    /* Now read all the targets dependancies */
	    
	    /* Allocate a db item */
	    item = xmalloc(sizeof (*item));
	    item->node.name = xstrdup(target);
	    item->circularcheck = 0;
	    
	    NewList(&item->deps);
	    
	    /* Add it to the db */
	    AddTail(&db->items, item);
	    
	    
	    /* Read all the dependencies */
	    
	    while(!done && ok) {
	    	Node *depnode;

	        char *dep, *dep_end;
	    	dep = s = next_not_blank(s);
		if (NULL == dep) {
		    break;
		}
		dep_end = s = end_of_word(s);
		if (NULL == dep_end) {
		    fprintf(stderr, "Malformed dependancy at line %d in %s\n", line, filename);
		    ok = 0;
		    break;
		}
		
		
		
		if (*s == 0) {
		    done = 1;
		} else {
		    s ++;
		}
		
		*dep_end = 0;
		
		depnode = xmalloc(sizeof (*depnode));
		depnode->name = xstrdup(dep);
		
		AddTail(&item->deps, depnode);
	    
	    } /* while (parse deps) */
	    
	} /* while (parse lines) */
    
    	fclose(f);
    } /* if (file opened) */
    
    if (!ok) {
    	free_deps_db(db);
	db = NULL;
    }
    return db;
     
}

void free_list(List *conf)
{
    Conf *cnode, *safe;
    
    ForeachNodeSafe(conf, cnode, safe) {
    	freenode((Node *)cnode);
    }
    xfree(conf);
}

void print_conf(List *conf)
{
    Conf *cnode;
    
    ForeachNode(conf, cnode) {
    	char *optstr;
    	switch (cnode->opt) {
	    case OPT_KERNEL:
	    	optstr = "kernel";
		break;
	    case OPT_DISK:
	    	optstr = "disk";
		break;
		
	    default:
	    	fprintf(stderr, "print_conf: unknown option %d\n", cnode->opt);
		cleanup(1);
		break;
	}
    	printf("%s => %s\n", cnode->node.name, optstr);
    }
}

List *get_conf(char *filename)
{
    FILE *f;
    List *conf;
    int ok = TRUE;
    int line = 0;
    
    conf = xmalloc(sizeof (*conf));
    NewList(conf);
    
    f = fopen(filename, "r");
    if (NULL == f) {
    	fprintf(stderr, "Could not open config file %s\n", filename);
	ok = FALSE;
    } else {
    	while (ok) {
	    char buf[500];
	    char *s, *target, *target_end, *opt, *opt_end;
	    Conf *cnode;
	    
	    if (NULL == fgets(buf, sizeof(buf), f))
	    	break;
		
	    s = buf;
	    line ++;
	    
	    target = s = next_not_blank(s);
	    if (NULL == target)
	    	continue; /* Handle blank lines */
	
	    target_end = s = end_of_word(s);
	    if (NULL == s) {
	    	fprintf(stderr, "Malformed target at line %d in %s\n", line, filename);
		ok = FALSE;
		break;
	    }
	    
	    opt = s = next_not_blank(s);
	    if (NULL == opt) {
	    	fprintf(stderr, "Missing option at line %d in %s\n", line, filename);
		ok = FALSE;
		break;
	    }
	    
	    if (opt == target_end) {
	    	fprintf(stderr, "Malformed option at line %d in %s\n", line, filename);
		ok = FALSE;
		break;
	    }
	    *target_end = 0;
	    
	    opt_end = s = end_of_word(s);
	    if (NULL == opt_end) {
	    	fprintf(stderr, "Malformed option at line %d in %s\n", line, filename);
		ok = FALSE;
		break;
	    }
	    
	    if (NULL != next_not_blank(s)) {
	    	fprintf(stderr, "Rubbish at end of line %d in %s\n", line, filename);
		ok = FALSE;
		break;
	    }
	    
	    *opt_end = 0;
	    
	    cnode = xmalloc(sizeof (*cnode));
	    AddTail(conf, cnode);
	    cnode->node.name = xstrdup(target);

	    if (0 == strcmp(opt, "kernel")) {
	    	cnode->opt = OPT_KERNEL;
	    } else if (0 == strcmp(opt, "disk")) {
	    	cnode->opt = OPT_DISK;
	    } else {
	    	fprintf(stderr, "Unknown option %s at line %d in %s\n", opt, line, filename);
		ok = FALSE;
		break;
	    }
	}
	fclose(f);
    }
    
    if (!ok) {
    	free_list(conf);
	conf = NULL;
    }
    return conf;
}

int checkcircular(Item *cur, DepsDB *ddb, Item **circ_found)
{
    Item *depnode;
    
    if (cur->circularcheck) {
    	fprintf(stderr, "Circular dependency detected:\n");
	fprintf(stderr, "%s depends on ", cur->node.name);
	*circ_found = cur;
    	return FALSE;
    }
    cur->circularcheck = 1;
    
    ForeachNode(&cur->deps, depnode) {
    	Item *dep;
	
	dep = (Item *)findnode(&ddb->items, depnode->node.name);
	if (NULL != dep) {
/*fprintf(stderr, "checking cirular on dependcy of %s: %s\n", cur->node.name, dep->node.name);	
*/	    if (!checkcircular(dep, ddb, circ_found)) {
	    	if (NULL != *circ_found) {
		    if (0 == strcmp(cur->node.name, (*circ_found)->node.name)) {
			fprintf(stderr, "%s\n", cur->node.name);
			*circ_found = NULL;
		    } else {
			fprintf(stderr, "%s depends on ", cur->node.name);
		    }
		}
		return FALSE;
	    }
	    
	}
    }
    
    cur->circularcheck = 0;
    
    return TRUE;
}

int add_deps(List *l, Node *dependee, Item *item, DepsDB *ddb)
{
    Node *n;
    Item *circ_found;
/*fprintf(stderr, "Looking at dep %s\n", item->node.name);
*/
    if (!checkcircular(item, ddb, &circ_found))
    	return FALSE;
	
    /* Check if this item allredy exists in the list */
    n = findnode(l, item->node.name);
    if (NULL != n) {
/*fprintf(stderr, "Allready existed in list\n");
*/  } else{
    	Item *depnode;
	
	n = xmalloc(sizeof (*n));
	n->name = xstrdup(item->node.name);
	
	if (NULL != dependee) {
	
	    /* Insert before the dependee */
/*fprintf(stderr, "Inserted before dependee %s\n", dependee->name);
*/	    InsertBefore(n, dependee);
	} else {
	
/*fprintf(stderr, "Added at end of list\n");
*/	    AddTail(l, n);
	}
	
	/* Go through everything we depend on and add it to the list */
	ForeachNode(&item->deps, depnode) {
	    Item *dep;
/*fprintf(stderr, "Looking at dep in list: %s\n", depnode->node.name);
*/	    /* Find the item in the deps DB */
	    dep = (Item *)findnode(&ddb->items, depnode->node.name);
	    if (NULL != dep) {
/*fprintf(stderr, "Found dep for %s: %s\n", depnode->node.name, dep->node.name);
*/	    	/* This dependany has more under-dependancies. Check it out */
		if (!add_deps(l, n, dep, ddb))
		    return FALSE;
	    } else {
	    	fprintf(stderr, "Non existing dependancy: %s\n", depnode->node.name);
		return FALSE;
	    }
	}
	
    }
    return TRUE;
    
}

List *do_deps(List *conf, DepsDB *ddb, int opt)
{
    Conf *cnode;
    List *deps;
    int ok = TRUE;
    
    deps = xmalloc(sizeof (*deps));
    NewList(deps);
    
    /* Go through all entries in the conf list */
    ForeachNode(conf, cnode) {
    	Item *item;
    	if (opt != cnode->opt)
	    continue;
	    
	/* Find in the deps db */
	item = (Item *)findnode(&ddb->items, cnode->node.name);
	if (NULL == item) {
	    fprintf(stderr, "Target %s could not be found in dependancy db\n", cnode->node.name);
	    ok = FALSE;
	    break;
	}
	
	if (!add_deps(deps, NULL, item, ddb)) {
	    ok = FALSE;
	    break;
	}
    }
    
    if (!ok) {
    	free_list(deps);
	deps = NULL;
    }
    
    return deps;
}

void print_deps(List *deps, const char *pre)
{
    Node *dnode;
    
    ForeachNode(deps, dnode) {
    	printf("%s%s ", pre, dnode->name);
    }
}

#define get_kernel_deps(conf, ddb)	\
	do_deps(conf, ddb, OPT_KERNEL)

List *get_disk_deps(List *conf, DepsDB *ddb, List *supkdeps)
{
    List *ddeps;

    /* First get the disk deps */
    ddeps = do_deps(conf, ddb, OPT_DISK);
    if (NULL != ddeps) {
    	List *kdeps = NULL;
	if (NULL == supkdeps) {
	    kdeps = do_deps(conf, ddb, OPT_KERNEL);
	} else {
	    kdeps = supkdeps;
	}
	if (NULL != kdeps) {
	    Node *ddep, *ddepsafe;
	    /* Go through all disk deps and see if it exists among the kernel deps. If so, remove it */
	    ForeachNodeSafe(ddeps, ddep, ddepsafe) {
	    	Node *kdep;
		
		kdep = findnode(kdeps, ddep->name);
		if (NULL != kdep) {
		    /* Allready in kernel. Remove from disk */
		    Remove(ddep);
		    freenode(ddep);
		}
	    
	    }
	
	    if (NULL == supkdeps)
	    	free_list(kdeps);	/* Dependancies were gotten incide here */
		
	    return ddeps;
	}
    	free_list(ddeps);
    }
    
    
    return NULL;
}

void print_meta_targets(List *deps, const char *text)
{
    printf("#MM hidd-%s : ", text);
    print_deps(deps, "hidd-");
    
    printf("\n");
    printf("hidd-%s : hidd-%s-local\n",text, text);
    printf("\t@(NOP)\n");
}

int do_metamake(List *conf, DepsDB *ddb)
{
    List *kdeps;
    int ret = FALSE;
    
    kdeps = get_kernel_deps(conf, ddb);
    if (NULL != kdeps) {
        List *ddeps;
	
    	ddeps = get_disk_deps(conf, ddb, kdeps);
	if (NULL != ddeps) {
	    
	    print_meta_targets(kdeps, "kernel");
	    printf("\n");
	    print_meta_targets(ddeps, "disk");
	    
	    ret = TRUE;
	    
	    free_list(ddeps);
	}
	free_list(kdeps);
    }
    return ret;
}



#define DEPSFILE "./hidds.dep"

#define DO_DISK		0x01
#define DO_KERNEL	0x02
#define DO_METAMAKE	0x04



int main(int argc, char *argv[])
{
    DepsDB *ddb;
    int exitcode = 0;
    int action = 0;
    
    
    char *configfile;
    
    for (;;) {
    	int c;
	c = getopt(argc, argv, "kdm");
	if (-1 == c)
	    break;
	switch (c) {
	    case 'k':
	    	action |= DO_KERNEL;
		break;
	
	    case 'd':
	    	action |= DO_DISK;
		break;
		
	    case 'm':
	    	action |= DO_METAMAKE;
		break;
	    
	    default:
	    	usage();
	}
    	
    }

    if (optind + 1 != argc)
    	usage();

    configfile = argv[optind];
    
	    
	
/* We used flags to get the options. This way we easily catch the case where
  the user has passed more than one option.
*/
    if (action != DO_KERNEL && action != DO_DISK && action != DO_METAMAKE)
    	usage();

    
    ddb = build_deps_db(DEPSFILE);
    if (NULL != ddb) {
	List *conf;
    	
//    	print_deps_db(ddb);
	
	conf = get_conf(configfile);
	if (NULL != conf) {
	    List *deps = NULL;
	    switch (action) {
	    	case DO_DISK:
		    deps = get_disk_deps(conf, ddb, NULL);
		    break;
		    
		case DO_KERNEL: {
		    deps = get_kernel_deps(conf, ddb);
		    break;
		}
		
		case DO_METAMAKE:
		if (!do_metamake(conf, ddb)) {
		    exitcode = 1;
		}
		    break;
	    
	    }
	    
	    
	    /* Print and free stuff from DO_DISK and DO_KERNEL */
	    if (action == DO_DISK || action == DO_KERNEL) {
		if (NULL != deps) {
		    print_deps(deps, "");
		    free_list(deps);
	    	} else {
		    exitcode = 1;
		}
	    }
	    
	    free_list(conf);
	    
	} else {
	    exitcode = 1;
	}
	
	free_deps_db(ddb);
	
    } else {
    	exitcode = 1;
    }
    
    return exitcode;
}
