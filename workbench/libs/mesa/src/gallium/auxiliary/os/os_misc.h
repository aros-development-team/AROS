/**************************************************************************
 *
 * Copyright 2010 Vmware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/*
 * Miscellaneous OS services.
 */


#ifndef _OS_MISC_H_
#define _OS_MISC_H_


#include "pipe/p_compiler.h"


#if defined(PIPE_OS_UNIX)
#  include <signal.h> /* for kill() */
#  include <unistd.h> /* getpid() */
#endif


#ifdef  __cplusplus
extern "C" {
#endif


/*
 * Trap into the debugger.
 */
#if (defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)) && defined(PIPE_CC_GCC)
#  define os_break() __asm("int3")
#elif defined(PIPE_CC_MSVC)
#  define os_break()  __debugbreak()
#elif defined(PIPE_OS_UNIX)
#  define os_break() kill(getpid(), SIGTRAP)
#else
#  define os_break() abort()
#endif


/*
 * Abort the program.
 */
#if defined(DEBUG) || defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY) || defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
#  define os_abort() os_break()
#else
#  define os_abort() abort()
#endif


/*
 * Output a message. Message should preferably end in a newline.
 */
void
os_log_message(const char *message);


/*
 * Get an option. Should return NULL if specified option is not set.
 */
const char *
os_get_option(const char *name);


#ifdef	__cplusplus
}
#endif


#endif /* _OS_MISC_H_ */
