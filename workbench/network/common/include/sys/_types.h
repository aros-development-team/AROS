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
 * Basic types upon which most other types are built.
 *
 * Note: It would be nice to simply use the compiler-provided __FOO_TYPE__
 * macros. However, in order to do so we have to check that those match the
 * previous typedefs exactly (not just that they have the same size) since any
 * change would be an ABI break. For example, changing `long` to `long long`
 * results in different C++ name mangling.
 */
typedef	signed char		__int8_t;
typedef	unsigned char		__uint8_t;
typedef	short			__int16_t;
typedef	unsigned short		__uint16_t;
typedef	int			__int32_t;
typedef	unsigned int		__uint32_t;
#if __SIZEOF_LONG__ == 8
typedef	long			__int64_t;
typedef	unsigned long		__uint64_t;
#elif __SIZEOF_LONG__ == 4
__extension__
typedef	long long		__int64_t;
__extension__
typedef	unsigned long long	__uint64_t;
#else
#error unsupported long size
#endif

typedef	__int8_t	__int_least8_t;
typedef	__int16_t	__int_least16_t;
typedef	__int32_t	__int_least32_t;
typedef	__int64_t	__int_least64_t;
typedef	__int64_t	__intmax_t;
typedef	__uint8_t	__uint_least8_t;
typedef	__uint16_t	__uint_least16_t;
typedef	__uint32_t	__uint_least32_t;
typedef	__uint64_t	__uint_least64_t;
typedef	__uint64_t	__uintmax_t;

#if __WORDSIZE == 64
typedef	__int64_t	__intptr_t;
typedef	__int64_t	__intfptr_t;
typedef	__uint64_t	__uintptr_t;
typedef	__uint64_t	__uintfptr_t;
typedef	__uint64_t	__vm_offset_t;
typedef	__uint64_t	__vm_size_t;
#elif __WORDSIZE == 32
typedef	__int32_t	__intptr_t;
typedef	__int32_t	__intfptr_t;
typedef	__uint32_t	__uintptr_t;
typedef	__uint32_t	__uintfptr_t;
typedef	__uint32_t	__vm_offset_t;
typedef	__uint32_t	__vm_size_t;
#else
#error unsupported pointer size
#endif

#if __SIZEOF_SIZE_T__ == 8
typedef	__uint64_t	__size_t;	/* sizeof() */
typedef	__int64_t	__ssize_t;	/* byte count or error */
#elif __SIZEOF_SIZE_T__ == 4
typedef	__uint32_t	__size_t;	/* sizeof() */
typedef	__int32_t	__ssize_t;	/* byte count or error */
#else
#error unsupported size_t size
#endif

/*
 * Standard type definitions.
 */
typedef	__uint8_t	__sa_family_t;
typedef	__uint32_t	__socklen_t;

#endif /* !_SYS__TYPES_H_ */
