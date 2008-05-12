/*
    Copyright C 2003, The AROS Development Team. All rights reserved
    $Id: macros.h 17527 2003-05-09 07:03:26Z schulz $

    Desc: Some usefull macros
    Lang: English
*/

#ifndef ASM_MACROS_H
#define ASM_MACROS_H

#define STDCALL /* eps */
#define SYSCALL /* eps */

#define NORETURN __attribute__((noreturn))

#define PACKED __attribute__((packed))

#define STACKPARM __attribute__((stackparm))

#define SECTION_CODE __attribute__((section(".text")))

#define SECTION_DATA __attribute__((section(".data")))

#endif /* ASM_MACROS_H */

