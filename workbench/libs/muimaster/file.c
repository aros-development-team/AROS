/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <exec/types.h>

#ifdef _AROS
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h> /* snprintf */
#endif

#include <zunepriv.h>
#include <file.h>
#ifndef _AROS
#include <sys/stat.h>
#endif
#include <unistd.h>
#include <string.h>


/*
 * find the image file corresponding to the given relative path,
 * by searching first into $(HOME)/.zune/images/, then in
 * ZUNEDATADIR/images/
 */
/* TODO: let user add search paths in Zune prefs */ 
STRPTR
__zune_file_find_image(STRPTR relpath)
{
    static char buf[1000]; /* static buffer to return the path */
    STRPTR path;

/*  g_print("searching %s\n", relpath);     */

    path = g_strconcat(__zune_file_get_user_image_dir(), relpath, NULL);
    if (__zune_file_exists_regular(path))
    {
	strncpy(buf, path, 999);
	buf[999] = 0;
	g_free(path);
/*  g_print("found %s\n", buf);     */
	return buf;
    }
    g_free(path);

    path = g_strconcat(__zune_file_get_sys_image_dir(), relpath, NULL);
    if (__zune_file_exists_regular(path))
    {
	strncpy(buf, path, 999);
	buf[999] = 0;
	g_free(path);
/*  g_print("found %s\n", buf);     */
	return buf;
    }
    g_free(path);
/*  g_print("not found\n", buf);     */
    return relpath;
}

BOOL
__zune_file_exists_regular(STRPTR path)
{
#ifndef _AROS

    struct stat st;

    if (stat(path, &st) == 0)
    {
	if (S_ISREG(st.st_mode))
	{
/*  	    g_print("path %s exists\n", path);     */
	    return TRUE;
	}
    }

#else

    BPTR lock;

    lock = Lock(path, ACCESS_READ);
    if (lock != 0)
    {
/*      g_print("path %s exists\n", path);     */
        UnLock(lock);
        return TRUE;
    }

#endif

/*  	    g_print("path %s doesnt exist\n", path);     */
    return FALSE;
}

#ifndef _AROS

STRPTR
__zune_file_get_data_dir(void)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	strncpy(buf, ZUNEDATADIR G_DIR_SEPARATOR_S, 999);
	buf[999] = 0;
    }
    return buf;
}

STRPTR
__zune_file_get_lib_dir(void)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	strncpy(buf, ZUNELIBDIR G_DIR_SEPARATOR_S, 999);
	buf[999] = 0;
    }
    return buf;
}

STRPTR
__zune_file_get_user_dir(void)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	g_snprintf(buf, 1000, "%s" G_DIR_SEPARATOR_S ".zune" G_DIR_SEPARATOR_S,
		   g_get_home_dir());
    }
    return buf;
}

STRPTR
__zune_file_get_user_image_dir(void)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	g_snprintf(buf, 1000, "%s" "images" G_DIR_SEPARATOR_S,
		   __zune_file_get_user_dir());
    }
    return buf;
}

STRPTR
__zune_file_get_user_libs_dir(void)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	g_snprintf(buf, 1000, "%s" "libs" G_DIR_SEPARATOR_S,
		   __zune_file_get_user_dir());
    }
    return buf;
}

STRPTR
__zune_file_get_sys_image_dir(void)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	g_snprintf(buf, 1000, "%s" "images" G_DIR_SEPARATOR_S,
		   __zune_file_get_data_dir());
    }
    return buf;
}

STRPTR
__zune_file_get_sys_prefs_global_file(void)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	g_snprintf(buf, 1000, "%s" "global.prefs",
		   __zune_file_get_data_dir());
    }
    return buf;
}

#endif

/*
 * usually you have only one apptitle per program :)
 */
STRPTR
__zune_file_get_sys_prefs_app_file(STRPTR app_title)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	buf[0] = 0;
	if (!app_title)
	    return "";
	g_snprintf(buf, 1000, "%s" "apps" G_DIR_SEPARATOR_S "%s.prefs",
		   __zune_file_get_data_dir(), app_title);
    }
    return buf;
}

STRPTR
__zune_file_get_user_prefs_global_file(void)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	g_snprintf(buf, 1000, "%s" "global.prefs",
		   __zune_file_get_user_dir());
    }
    return buf;
}

STRPTR
__zune_file_get_user_prefs_app_file(STRPTR app_title)
{
    static char buf[1000];
    static int init = 0;

    if (!init)
    {
	init = 1;
	buf[0] = 0;
	if (!app_title)
	    return "";
	g_snprintf(buf, 1000, "%s" "apps" G_DIR_SEPARATOR_S "%s.prefs",
		   __zune_file_get_user_dir(), app_title);
    }
    return buf;
}

