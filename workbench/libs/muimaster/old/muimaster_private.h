/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUIMASTER_PRIVATE_H
#define _MUIMASTER_PRIVATE_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

#if defined(_AMIGA) && defined(__GNUC__)
#   ifndef NO_INLINE_STDARG
#	define NO_INLINE_STDARG
#   endif
#   include "muimaster_pinline.h"
#else
#   include "muimaster_pdefs.h"
#endif

/*#ifndef NO_INLINE_STDARG*/
#define MUI_NewObject(a0, tags...) \
        ({ULONG _tags[] = { tags }; MUI_NewObjectA((a0), (struct TagItem *)_tags);})
/*#endif /* !NO_INLINE_STDARG */

/*** GString.c ***/

typedef struct _GString
{
	char *str;
	int   len;
	int   max;
} GString;

AROS_LP1(GString *, g_string_new,
	AROS_LPA(const char *, s, A0),
	struct Library *, MUIMasterBase, 35, MUIMaster)

AROS_LP1(GString *, g_string_sized_new,
	AROS_LPA(int, len, D0),
	struct Library *, MUIMasterBase, 36, MUIMaster)

AROS_LP2(void, g_string_free,
	AROS_LPA(GString *, str,  A0),
	AROS_LPA(int      , flag, D0),
	struct Library *, MUIMasterBase, 37, MUIMaster)

AROS_LP2(GString *, g_string_append,
	AROS_LPA(GString    *, str, A0),
	AROS_LPA(const char *, s,   A1),
	struct Library *, MUIMasterBase, 38, MUIMaster)

AROS_LP2(GString *, g_string_append_c,
	AROS_LPA(GString *, str, A0),
	AROS_LPA(char     , c,   D0),
	struct Library *, MUIMasterBase, 39, MUIMaster)

AROS_LP3(STRPTR *, g_strsplit,
	AROS_LPA(const char *, string,     A0),
	AROS_LPA(const char *, delimiter,  A1),
	AROS_LPA(      int   , max_tokens, D0),
	struct Library *, MUIMasterBase, 50, private)

AROS_LP1(void, g_strfreev,
	AROS_LPA(STRPTR *, str_array, A0),
	struct Library *, MUIMasterBase, 51, private)


/*** GList.c ***/

typedef void *gpointer;

typedef struct _GList
{
    gpointer data;
    struct _GList *next;
    struct _GList *prev;
} GList;

#define g_list_next(n)     ((n)?(n)->next:0)
#define g_list_previous(n) ((n)?(n)->prev:0)

AROS_LP2(GList *, g_list_append,
	AROS_LPA(GList  *, list, A0),
	AROS_LPA(gpointer, data, A1),
	struct Library *, MUIMasterBase, 40, MUIMaster)

AROS_LP2(GList *, g_list_prepend,
	AROS_LPA(GList  *, list, A0),
	AROS_LPA(gpointer, data, A1),
	struct Library *, MUIMasterBase, 41, MUIMaster)

AROS_LP2(GList *, g_list_find,
	AROS_LPA(GList  *, list, A0),
	AROS_LPA(gpointer, data, A1),
	struct Library *, MUIMasterBase, 42, MUIMaster)

AROS_LP2(GList *, g_list_remove,
	AROS_LPA(GList  *, list, A0),
	AROS_LPA(gpointer, data, A1),
	struct Library *, MUIMasterBase, 43, MUIMaster)

AROS_LP1(GList *, g_list_reverse,
	AROS_LPA(GList *, list, A0),
	struct Library *, MUIMasterBase, 44, MUIMaster)

AROS_LP1(int, g_list_length,
	AROS_LPA(GList *, list, A0),
	struct Library *, MUIMasterBase, 45, MUIMaster)

AROS_LP1(GList *, g_list_first,
	AROS_LPA(GList *, list, A0),
	struct Library *, MUIMasterBase, 46, MUIMaster)

AROS_LP1(GList *, g_list_last,
	AROS_LPA(GList *, list, A0),
	struct Library *, MUIMasterBase, 47, MUIMaster)

AROS_LP3(void, g_list_foreach,
	AROS_LPA(GList   *, list,  A0),
	AROS_LPA(VOID_FUNC, func,  A1),
	AROS_LPA(gpointer , param, A2),
	struct Library *, MUIMasterBase, 48, MUIMaster)

AROS_LP1(void, g_list_free,
	AROS_LPA(GList *, list, A0),
	struct Library *, MUIMasterBase, 49, MUIMaster)


#endif /* _MUIMASTER_PRIVATE_H */
