#ifndef TOOLLIB_TOOLLIB_H
#define TOOLLIB_TOOLLIB_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>

/* Do we have <stdarg.h> ? */
#ifndef HAVE_STDARG_H
#   define HAVE_STDARG_H
#endif

#ifndef PROTOTYPES
#   ifdef __STDC__
#	define PROTOTYPES
#   endif
#endif

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

#include <stdlib.h>

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

/* Macros */
#   define IsListEmpty(l)   (((List *)l)->prelast == (Node *)(l))
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

#define cfree(x)            if (x) free (x)
#define cstrdup(x)          ((x) ? xstrdup (x) : NULL)
#define setstr(str,val)     cfree (str); \
			    str = cstrdup (val)
#define xstrdup(str)        _xstrdup(str,__FILE__,__LINE__)
#define xmalloc(size)       _xmalloc(size,__FILE__,__LINE__)
#define xrealloc(ptr,size)  _xrealloc(ptr,size,__FILE__,__LINE__)
#define xfree(ptr)          _xfree(ptr,__FILE__,__LINE__)
#define new(type)           (type *)xmalloc(sizeof(type))
#define ALIGN(x,y)          ((x + (y-1)) & (-(y)))
#define hexval(c)           (((c) > '9') ? (c) - 'a' + 10 : (c) - '0')
#define xcalloc(cnt,size)   _xcalloc(cnt,size,__FILE__,__LINE__)

/* Prototypes */
extern int execute PARAMS ((const char * cmd, const char * args,
			    const char * in, const char * out));
extern char * _xstrdup PARAMS ((const char * str, const char * file, int line));
extern void * _xmalloc PARAMS ((size_t size, const char * file, int line));
extern void * _xrealloc PARAMS ((void * ptr, size_t size, const char * file, int line));
extern void _xfree PARAMS ((void * ptr, const char * file, int line));
extern void * _xcalloc PARAMS ((size_t cnt, size_t size, const char * file, int line));
extern Node * FindNode PARAMS ((const List * l, const char * name));
extern Node * FindNodeNC PARAMS ((const List * l, const char * name));
extern void printlist PARAMS ((const List * l));
extern Node * RemHead PARAMS ((List * l));

#endif /* TOOLLIB_TOOLLIB_H */
