/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUIMASTER_PDEFS_H
#define _MUIMASTER_PDEFS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif


/*** GString.c ***/

#if 0
#define g_string_new(s) \
AROS_LC1(GString *, g_string_new, \
    AROS_LCA(const char *, s, A0), \
    struct Library *, MUIMasterBase, 35, private)

#define g_string_sized_new(len) \
AROS_LC1(GString *, g_string_sized_new, \
    AROS_LCA(int, len, D0), \
    struct Library *, MUIMasterBase, 36, private)

#define g_string_free(str, flag) \
AROS_LC2(void, g_string_free, \
    AROS_LCA(GString *, str,  A0), \
    AROS_LCA(int      , flag, D0), \
    struct Library *, MUIMasterBase, 37, private)

#define g_string_append(str, s) \
AROS_LC2(GString *, g_string_append, \
    AROS_LCA(GString    *, str, A0), \
    AROS_LCA(const char *, s,   A1), \
    struct Library *, MUIMasterBase, 38, private)

#define g_string_append_c(str, c) \
AROS_LC2(GString *, g_string_append_c, \
    AROS_LCA(GString *, str, A0), \
    AROS_LCA(char     , c,   D0), \
    struct Library *, MUIMasterBase, 39, private)

#define g_strsplit(s, d, mt) \
AROS_LC3(STRPTR *, g_strsplit, \
    AROS_LCA(const char *, s,  A0), \
    AROS_LCA(const char *, d,  A1), \
    AROS_LCA(      int   , mt, D0), \
    struct Library *, MUIMasterBase, 50, private)

#define g_strfreev(sa) \
AROS_LC1(void, g_strfreev, \
    AROS_LCA(STRPTR *, sa, A0), \
    struct Library *, MUIMasterBase, 51, private)

#endif

/*** GList.c ***/

#if 0
#define g_list_append(list, data) \
AROS_LC2(GList *, g_list_append, \
    AROS_LCA(GList  *, list, A0), \
    AROS_LCA(gpointer, data, A1), \
    struct Library *, MUIMasterBase, 40, private)

#define g_list_prepend(list, data) \
AROS_LC2(GList *, g_list_prepend, \
    AROS_LCA(GList  *, list, A0), \
    AROS_LCA(gpointer, data, A1), \
    struct Library *, MUIMasterBase, 41, private)

#define g_list_find(list, data) \
AROS_LC2(GList *, g_list_find, \
    AROS_LCA(GList  *, list, A0), \
    AROS_LCA(gpointer, data, A1), \
    struct Library *, MUIMasterBase, 42, private)

#define g_list_remove(list, data) \
AROS_LC2(GList *, g_list_remove, \
    AROS_LCA(GList  *, list, A0), \
    AROS_LCA(gpointer, data, A1), \
    struct Library *, MUIMasterBase, 43, private)

#define g_list_reverse(list) \
AROS_LC1(GList *, g_list_reverse, \
    AROS_LCA(GList *, list, A0), \
    struct Library *, MUIMasterBase, 44, private)

#define g_list_length(list) \
AROS_LC1(int, g_list_length, \
    AROS_LCA(GList *, list, A0), \
    struct Library *, MUIMasterBase, 45, private)

#define g_list_first(list) \
AROS_LC1(GList *, g_list_first, \
    AROS_LCA(GList *, list, A0), \
    struct Library *, MUIMasterBase, 46, private)

#define g_list_last(list) \
AROS_LC1(GList *, g_list_last, \
    AROS_LCA(GList *, list, A0), \
    struct Library *, MUIMasterBase, 47, private)

#define g_list_foreach(list, func, param) \
AROS_LC3(void, g_list_foreach, \
    AROS_LCA(GList   *, list,  A0), \
    AROS_LCA(VOID_FUNC, func,  A1), \
    AROS_LCA(gpointer , param, A2), \
    struct Library *, MUIMasterBase, 48, private)

#define g_list_free(list) \
AROS_LC1(void, g_list_free, \
    AROS_LCA(GList *, list, A0), \
    struct Library *, MUIMasterBase, 49, private)

#endif

#endif /* _MUIMASTER_PDEFS_H */
