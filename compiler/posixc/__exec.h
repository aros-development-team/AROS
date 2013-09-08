/*
    Copyright © 2008-2009, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef __EXEC_H
#define __EXEC_H

#include <stdarg.h>

APTR __exec_prepare(const char *filename, int searchpath, char *const argv[], char *const envp[]);
void __exec_do(APTR id);
char *const *__exec_valist2array(const char *arg1, va_list list);
void __exec_cleanup_array();

#endif //__EXEC_H
