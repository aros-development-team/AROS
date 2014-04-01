/*
 * $Id$
 *
 * :ts=4
 *
 * SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
 *
 * Copyright (C) 2000-2009 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _SMBFS_H
#define _SMBFS_H 1

/****************************************************************************/

#ifndef _SYSTEM_HEADERS_H
#include "system_headers.h"
#endif /* _SYSTEM_HEADERS_H */

#ifndef _ASSERT_H
#include "assert.h"
#endif /* _ASSERT_H */

/****************************************************************************/

#define SAME (0)
#define OK (0)
#define NOT !

/****************************************************************************/

#ifndef ZERO
#define ZERO ((BPTR)NULL)
#endif /* ZERO */

/****************************************************************************/

#ifndef AMIGA_COMPILER_H

/****************************************************************************/

#if defined(__SASC)
#define FAR __far
#define ASM __asm
#define REG(r,p) register __##r p
#define INLINE __inline
#endif /* __SASC */

#if defined(__GNUC__)
#define FAR
#define ASM
#define REG(r,p) p __asm(#r)
#define INLINE __inline__
#endif /* __GNUC__ */

/****************************************************************************/

#ifndef VARARGS68K
#define VARARGS68K
#endif /* VARARGS68K */

/*****************************************************************************/

#endif /* AMIGA_COMPILER_H */

/*****************************************************************************/

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif /* min */

/****************************************************************************/

#if defined(__SASC)
extern struct Library * FAR AbsExecBase;
#else
#ifndef AbsExecBase
#define AbsExecBase (*(struct Library **)4)
#endif /* AbsExecBase */
#endif /* __SASC */

/****************************************************************************/

extern struct Library * SocketBase;
extern struct ExecBase * SysBase;
extern struct Library * DOSBase;

/****************************************************************************/

#if defined(__amigaos4__)

/****************************************************************************/

extern struct ExecIFace *	IExec;
extern struct DOSIFace *	IDOS;
extern struct SocketIFace *	ISocket;

/****************************************************************************/

#endif /* __amigaos4__ */

/****************************************************************************/

#ifndef h_errno
extern int h_errno;
#endif

/****************************************************************************/

extern int BroadcastNameQuery(char *name, char *scope, UBYTE *address);
extern LONG CompareNames(STRPTR a,STRPTR b);
extern LONG GetTimeZoneDelta(VOID);
extern STRPTR amitcp_strerror(int error);
extern STRPTR host_strerror(int error);
extern time_t MakeTime(const struct tm * const tm);
extern ULONG GetCurrentTime(VOID);
extern VOID GMTime(time_t seconds,struct tm * tm);
#ifdef __AROS__
extern VOID VReportError(STRPTR fmt, IPTR *args);
#define ReportError(fmt, ...) do { \
    IPTR vargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    VReportError(fmt, vargs); } while (0)
#define SPrintf(buf, fmt, ...) do { \
    IPTR vargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    VSPrintf(buf, fmt, vargs); } while (0)
#else
extern VOID VARARGS68K ReportError(STRPTR fmt,...);
extern VOID VARARGS68K SPrintf(STRPTR buffer, STRPTR formatString,...);
#endif
extern VOID StringToUpper(STRPTR s);

/****************************************************************************/

size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);

/****************************************************************************/

extern void smb_encrypt(unsigned char *passwd, unsigned char *c8, unsigned char *p24);
extern void smb_nt_encrypt(unsigned char *passwd, unsigned char *c8, unsigned char *p24);

/****************************************************************************/

extern VOID FreeMemory(APTR address);
extern APTR AllocateMemory(ULONG size);

#define malloc(s) AllocateMemory(s)
#define free(m) FreeMemory(m)

/****************************************************************************/

#undef memcpy
#define memcpy(to,from,size) ((void)CopyMem((APTR)(from),(APTR)(to),(ULONG)(size)))

/****************************************************************************/

#endif /* _SMBFS_H */
