#ifndef _PROTO_BZ2_H
#define _PROTO_BZ2_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_BZ2_PROTOS_H) && !defined(__GNUC__)
#include <clib/bz2_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *BZ2Base;
#endif

#ifdef __GNUC__
#include <inline/bz2.h>
#elif defined(__VBCC__)
#include <inline/bz2_protos.h>
#else
#include <pragma/bz2_lib.h>
#endif

#endif	/*  _PROTO_BZ2_H  */
