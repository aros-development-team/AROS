/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __ZUNE_PRIV_H__
#define __ZUNE_PRIV_H__

#ifdef _AROS


#include <libdefs.h>
#include <exec/memory.h>
#include <aros/debug.h>
#include <muimaster_private.h>

#define GPOINTER_TO_UINT(p) (IPTR)p
#define _U(p) (IPTR)p

#define SLONG_FMT "ld"
#define ULONG_FMT "lu"

#ifndef DoSuperNew
#define DoSuperNew(cl, obj, tags...) \
({ STACKIPTR _tags[] = { tags };\
   STACKIPTR _args[] = { OM_NEW, (STACKIPTR)_tags };\
   DoSuperMethodA(cl, obj, (Msg)_args); })
#endif /* !DoSuperNew */

/*** all of this should really go into glib.h ***/

#define g_malloc(x)               AllocVec((x),MEMF_ANY)
#define g_malloc0(x)              AllocVec((x),MEMF_CLEAR)
#define g_free(x)                 FreeVec(x)
#define g_strdup(x)               strcpy(AllocVec(strlen(x)+1,MEMF_ANY),(x))
#define g_snprintf(s,n,f,args...) snprintf(s,n,f,args)

#define GMemChunk void /* this makes for an APTR */

#define g_mem_chunk_create(type,x,y) CreatePool(MEMF_CLEAR,16384,8192)
#define g_mem_chunk_destroy(pool)    DeletePool(pool)

#define g_chunk_new(type,pool)  (type *)AllocPooled((pool),sizeof(type))
#define g_chunk_new0(type,pool) (type *)AllocPooled((pool),sizeof(type))
#define g_chunk_free(data,pool) FreePooled((pool),(data),sizeof(*(data)))

#else

#include <zune/boopsi.h>
#include <zune/preprotos.h>

/* should use atomic_t, no ? */
extern int __zune_signals;

#define ASSERT(x) g_assert(x)

#endif

#include <gdk/gdktypes.h>
#include <zune/zune_common.h>
#include <shortcuts.h>

#endif
