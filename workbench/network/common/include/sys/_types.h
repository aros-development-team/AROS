/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2002 Mike Barcroft <mike@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS__TYPES_H_
#define _SYS__TYPES_H_

#include <aros/cpu.h>

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
typedef unsigned AROS_INTPTR_TYPE   __size_t;
typedef signed AROS_INTPTR_TYPE     __ssize_t;

/*
 * Standard type definitions.
 */
typedef unsigned AROS_8BIT_TYPE     __sa_family_t;
typedef unsigned AROS_32BIT_TYPE    __socklen_t;

#endif /* !_SYS__TYPES_H_ */
