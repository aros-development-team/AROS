#ifndef _MUIMASTER_PINLINE_H
#define _MUIMASTER_PINLINE_H

/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Private m68k inlines for muimaster.library
    Lang: english
*/

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef MUIMASTER_BASE_NAME
#define MUIMASTER_BASE_NAME MUIMasterBase
#endif


/*** GString.c ***/

#if 0
#define g_string_new(s) \
	LP1(0xd2, GString *, g_string_new, const char *, s, a0, \
	, MUIMASTER_BASE_NAME)

#define g_string_sized_new(len) \
	LP1(0xd8, GString *, g_string_sized_new, int, len, d0, \
	, MUIMASTER_BASE_NAME)

#define g_string_free(str, flag) \
	LP2NR(0xde, g_string_free, GString *, str, a0, int, flag, d0, \
	, MUIMASTER_BASE_NAME)

#define g_string_append(str, s) \
	LP2(0xe4, GString *, g_string_append, GString *, str, a0, const char *, s, a1, \
	, MUIMASTER_BASE_NAME)

#define g_string_append_c(str, c) \
	LP2(0xea, GString *, g_string_append_c, GString *, str, a0, char, c, d0, \
	, MUIMASTER_BASE_NAME)

#define g_strsplit(s, d, mt) \
	LP3(0x12c, STRPTR *, g_strsplit, const char *, s, a0, const char *, d, a1, int, mt, d0, \
	, MUIMASTER_BASE_NAME)

#define g_strfreev(sa) \
	LP1NR(0x132, g_strfreev, STRPTR *, sa, a0, \
	, MUIMASTER_BASE_NAME)

#endif

/*** GList.c ***/

#if 0
#define g_list_append(list, data) \
	LP2(0xf0, GList *, g_list_append, GList *, list, a0, gpointer, data, a1, \
	, MUIMASTER_BASE_NAME)

#define g_list_prepend(list, data) \
	LP2(0xf6, GList *, g_list_prepend, GList *, list, a0, gpointer, data, a1, \
	, MUIMASTER_BASE_NAME)

#define g_list_find(list, data) \
	LP2(0xfc, GList *, g_list_find, GList *, list, a0, gpointer, data, a1, \
	, MUIMASTER_BASE_NAME)

#define g_list_remove(list, data) \
	LP2(0x102, GList *, g_list_remove, GList *, list, a0, gpointer, data, a1, \
	, MUIMASTER_BASE_NAME)

#define g_list_reverse(list) \
	LP1(0x108, GList *, g_list_reverse, GList *, list, a0, \
	, MUIMASTER_BASE_NAME)

#define g_list_length(list) \
	LP1(0x10e, int, g_list_length, GList *, list, a0, \
	, MUIMASTER_BASE_NAME)

#define g_list_first(list) \
	LP1(0x114, GList *, g_list_first, GList *, list, a0, \
	, MUIMASTER_BASE_NAME)

#define g_list_last(list) \
	LP1(0x11a, GList *, g_list_last, GList *, list, a0, \
	, MUIMASTER_BASE_NAME)

#define g_list_foreach(list, func, param) \
	LP3NR(0x120, g_list_foreach, GList *, list, a0, VOID_FUNC, func, a1, gpointer, param, a2, \
	, MUIMASTER_BASE_NAME)

#define g_list_free(list) \
	LP1NR(0x126, g_list_free, GList *, list, a0, \
	, MUIMASTER_BASE_NAME)

#endif

#endif /* _MUIMASTER_PINLINE_H */
