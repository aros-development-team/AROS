/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_REXXSYSLIB_H
#define _INLINE_REXXSYSLIB_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef REXXSYSLIB_BASE_NAME
#define REXXSYSLIB_BASE_NAME RexxSysBase
#endif

#define ClearRexxMsg(msgptr, count) \
	LP2NR(0x9c, ClearRexxMsg, struct RexxMsg *, msgptr, a0, unsigned long, count, d0, \
	, REXXSYSLIB_BASE_NAME)

#define CreateArgstring(string, length) \
	LP2(0x7e, UBYTE *, CreateArgstring, UBYTE *, string, a0, unsigned long, length, d0, \
	, REXXSYSLIB_BASE_NAME)

#define CreateRexxMsg(port, extension, host) \
	LP3(0x90, struct RexxMsg *, CreateRexxMsg, struct MsgPort *, port, a0, UBYTE *, extension, a1, UBYTE *, host, d0, \
	, REXXSYSLIB_BASE_NAME)

#define DeleteArgstring(argstring) \
	LP1NR(0x84, DeleteArgstring, UBYTE *, argstring, a0, \
	, REXXSYSLIB_BASE_NAME)

#define DeleteRexxMsg(packet) \
	LP1NR(0x96, DeleteRexxMsg, struct RexxMsg *, packet, a0, \
	, REXXSYSLIB_BASE_NAME)

#define FillRexxMsg(msgptr, count, mask) \
	LP3(0xa2, BOOL, FillRexxMsg, struct RexxMsg *, msgptr, a0, unsigned long, count, d0, unsigned long, mask, d1, \
	, REXXSYSLIB_BASE_NAME)

#define IsRexxMsg(msgptr) \
	LP1(0xa8, BOOL, IsRexxMsg, struct RexxMsg *, msgptr, a0, \
	, REXXSYSLIB_BASE_NAME)

#define LengthArgstring(argstring) \
	LP1(0x8a, ULONG, LengthArgstring, UBYTE *, argstring, a0, \
	, REXXSYSLIB_BASE_NAME)

#define LockRexxBase(resource) \
	LP1NR(0x1c2, LockRexxBase, unsigned long, resource, d0, \
	, REXXSYSLIB_BASE_NAME)

#define UnlockRexxBase(resource) \
	LP1NR(0x1c8, UnlockRexxBase, unsigned long, resource, d0, \
	, REXXSYSLIB_BASE_NAME)

#endif /* _INLINE_REXXSYSLIB_H */
