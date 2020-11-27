#ifndef AROS_AROSSUPPORTBASE_H
#define AROS_AROSSUPPORTBASE_H

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdarg.h>

#if defined(__GNUC__) 
#define ATTRIB_FMT(a,b)  __attribute__ ((format (printf, a, b)))
#else
#define ATTRIB_FMT(a,b)
#endif

struct AROSSupportBase
{
    void    	    * StdOut;
    int     	    (*kprintf)(const char *, ...) ATTRIB_FMT(1, 2);
    int     	    (*rkprintf)(const char *, const char *, int, const char *, ...) ATTRIB_FMT(4, 5);
    int     	    (*vkprintf)(const char *, va_list);
    void    	    * DebugConfig;    
};

#endif /* AROS_AROSSUPPORTBASE_H */
