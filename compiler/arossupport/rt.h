#ifndef _RT_H
#define _RT_H

/*
    Copyright © 1995-96, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common header file for RT
    Lang: english
*/
#define ENABLE_RT 0	/* no RT inside this file */
#define RT_INTERNAL 1
#include <aros/rt.h>

#include <exec/lists.h>
#include <stdarg.h>
/*
#include <aros/system.h>
#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/arossupport.h>
#include <proto/alib.h>
*/
#include "etask.h"

#define RT_VERSION	1	/* Current version of RT */

#define HASH_BITS	4	/* Size of hash in bits and entries */
#define HASH_SIZE	(1L << HASH_BITS)

typedef struct
{
    const char * Function;
    const char * File;
    ULONG	 Line;
}
RTStack;

#define STACKDEPTH  256


/* This is what is behind etask->iet_RT */
typedef struct
{
    ULONG	   rtd_Version; /* RT_VERSION */
    struct MinList rtd_ResHash[RTT_MAX][HASH_SIZE];
    ULONG	   rtd_StackPtr;
    RTStack	   rtd_CallStack[STACKDEPTH];
} RTData;

extern RTData * intRT; /* Global variable in case no ETask is available */

typedef struct
{
    struct MinNode Node;
    const char	 * File;
    ULONG	   Line;
    ULONG	   Flags;
}
RTNode;

/* Private flags of resource nodes */
#define RTNF_DONT_FREE	    0x80000000 /* Resource must not be freed */

typedef struct
{
    RTNode Node;
    APTR   Resource;	/* This should be common to every resource */
}
Resource;

typedef struct __RTDesc RTDesc;

typedef IPTR (* RT_AllocFunc)  (RTData * rtd, RTNode *, va_list, BOOL * success);
typedef IPTR (* RT_FreeFunc)   (RTData * rtd, RTNode *);
typedef IPTR (* RT_SearchFunc) (RTData * rtd, int, RTNode **, va_list);
typedef IPTR (* RT_ShowError)  (RTData * rtd, int, RTNode *, IPTR, int, const char * file, ULONG line, va_list);
typedef IPTR (* RT_CheckFunc)  (RTData * rtd, int, const char * file, ULONG line, ULONG op, va_list);

#define HASH_BASE(rtn)  (((Resource *)rtn)->Resource)

#if HASH_BITS==4
#define CALCHASH(res)   \
    ((((ULONG)res) + (((ULONG)res)>>4) +(((ULONG)res)>>8) + (((ULONG)res)>>12) + \
     (((ULONG)res)>>16) + (((ULONG)res)>>20) +(((ULONG)res)>>24) + (((ULONG)res)>>28)) \
     & 0x0000000FL)
#endif

struct __RTDesc
{
    const ULONG    Size;
    RT_AllocFunc   AllocFunc;
    RT_FreeFunc    FreeFunc;
    RT_SearchFunc  SearchFunc;
    RT_ShowError   ShowError;
    RT_CheckFunc   CheckFunc;
};

#define GetRTData()                                                 \
	({                                                          \
	    struct IntETask * et;				    \
								    \
	    et = (struct IntETask *)GetETask (FindTask(NULL));      \
								    \
	    et							    \
		&& et->iet_RT					    \
		&& ((RTData *)et->iet_RT)->rtd_Version == RT_VERSION \
	    ? et->iet_RT					    \
	    : intRTD;						    \
	})
#define SetRTData(rtd)                                              \
	{							    \
	    struct IntETask * et;				    \
								    \
	    et = (struct IntETask *)GetETask (FindTask(NULL));      \
								    \
	    if (et)                                                 \
		et->iet_RT = rtd;				    \
	    else						    \
		intRTD = rtd;					    \
	}

/* Return values of SearchFunc */
#define RT_SEARCH_FOUND 	    0
#define RT_SEARCH_NOT_FOUND	    1
#define RT_SEARCH_SIZE_MISMATCH     2

#define RT_FREE     0
#define RT_CHECK    1
#define RT_EXIT     2

extern RTDesc const * RT_Resources[RTT_MAX];

#define GetRTDesc(no)       RT_Resources[no]
#define GetRTField(no,f)    (GetRTDesc(no) ? GetRTDesc(no)->f : NULL)
#define GetRTSize(no)       (GetRTDesc(no) ? GetRTDesc(no)->Size : 0)
#define GetRTAllocFunc(no)  (GetRTField(no,AllocFunc))
#define GetRTFreeFunc(no)   (GetRTField(no,FreeFunc))
#define GetRTSearchFunc(no) (GetRTField(no,SearchFunc))
#define GetRTShowError(no)  (GetRTField(no,ShowError))
#define GetRTCheckFunc(no)  (GetRTField(no,CheckFunc))

extern void RT_FreeResource (RTData * rtd, int rtt, RTNode * rtnode);

#define ALIGNED_PTR	0x00000001	/* Must be aligned */
#define NULL_PTR	0x00000002	/* May be NULL */
extern BOOL CheckPtr (APTR ptr, ULONG flags);
extern BOOL CheckArea (APTR ptr, ULONG size, ULONG flags);

extern IPTR RT_Search (RTData * rtd, int rtt, RTNode ** rtptr, va_list args);

#endif /* _RT_H */
