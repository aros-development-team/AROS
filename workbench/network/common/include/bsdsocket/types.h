#ifndef BSDSOCKET_TYPES_H
#define BSDSOCKET_TYPES_H
/*
 * $Id$
 *
 * Common types previously defined in multiple headers.
 *
 * Copyright (c) 1994 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
  */

#include <aros/cpu.h>

#include <sys/types.h>

#if __BSD_VISIBLE
#ifndef _GID_T_DECLARED
#include <aros/types/gid_t.h>
#define _GID_T_DECLARED
#endif

#ifndef _OFF_T_DECLARED
#include <aros/types/off_t.h>
#define _OFF_T_DECLARED
#endif

#ifndef _PID_T_DECLARED
#include <aros/types/pid_t.h>
#define _PID_T_DECLARED
#endif
#endif

#ifndef _SA_FAMILY_T_DECLARED
#include <aros/types/sa_family_t.h>
#define _SA_FAMILY_T_DECLARED
#endif

#ifndef _SOCKLEN_T_DECLARED
#include <aros/types/socklen_t.h>
#define _SOCKLEN_T_DECLARED
#endif

#ifndef _SSIZE_T_DECLARED
#include <aros/types/ssize_t.h>
#define _SSIZE_T_DECLARED
#endif

#if __BSD_VISIBLE 
#ifndef _UID_T_DECLARED
#include <aros/types/uid_t.h>
#define _UID_T_DECLARED
#endif
#endif

#ifndef _UINT32_T_DECLARED
#include <aros/types/int_t.h>
#define _UINT32_T_DECLARED
#endif

#ifndef _UINTPTR_T_DECLARED
#include <aros/types/uintptr_t.h>
#define _UINTPTR_T_DECLARED
#endif

#ifndef _MODE_T_DECLARED
#include <aros/types/mode_t.h>
#define _MODE_T_DECLARED
#endif

#ifndef _TIME_T_DECLARED
#include <aros/types/time_t.h>
#define _TIME_T_DECLARED
#endif

#ifndef NULL
#define	NULL 0L
#endif
/*
 * Basic types upon which other BSD types are built.
 * Use AROS provided definitions.
 */
typedef signed AROS_8BIT_TYPE       __int8_t;
typedef unsigned AROS_8BIT_TYPE     __uint8_t;
typedef signed AROS_16BIT_TYPE      __int16_t;
typedef unsigned AROS_16BIT_TYPE    __uint16_t;
typedef signed AROS_32BIT_TYPE      __int32_t;
typedef unsigned AROS_32BIT_TYPE    __uint32_t;
typedef signed AROS_64BIT_TYPE      __int64_t;
typedef unsigned AROS_64BIT_TYPE    __uint64_t;

typedef __int8_t                    __int_least8_t;
typedef __int16_t                   __int_least16_t;
typedef __int32_t                   __int_least32_t;
typedef __int64_t                   __int_least64_t;
typedef __int64_t                   __intmax_t;
typedef __uint8_t                   __uint_least8_t;
typedef __uint16_t                  __uint_least16_t;
typedef __uint32_t                  __uint_least32_t;
typedef __uint64_t                  __uint_least64_t;
typedef __uint64_t                  __uintmax_t;

typedef signed AROS_INTPTR_TYPE     __intptr_t;
typedef signed AROS_INTPTR_TYPE     __intfptr_t;
typedef unsigned AROS_INTPTR_TYPE   __uintptr_t;
typedef unsigned AROS_INTPTR_TYPE   __uintfptr_t;
typedef unsigned AROS_INTPTR_TYPE   __vm_offset_t;
typedef unsigned AROS_INTPTR_TYPE   __vm_size_t;
#ifdef __size_t
#undef __size_t
#endif
typedef unsigned AROS_INTPTR_TYPE   __size_t;
typedef signed AROS_INTPTR_TYPE     __ssize_t;

/*
 * Standard type definitions.
 */
typedef unsigned AROS_8BIT_TYPE     __sa_family_t;
typedef unsigned AROS_32BIT_TYPE    __socklen_t;

#include <endian.h>

#ifndef _TIME_ /*  XXX fast fix for SNMP, going away soon */
#include <sys/time.h>
#endif

#define _ALIGN AROS_ALIGN

#endif /* !BSDSOCKET_TYPES_H */
