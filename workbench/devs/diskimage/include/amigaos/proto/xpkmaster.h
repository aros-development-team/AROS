#ifndef _PROTO_XPKMASTER_H
#define _PROTO_XPKMASTER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_XPKMASTER_PROTOS_H) && !defined(__GNUC__)
#include <clib/xpkmaster_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *XpkBase;
#endif

#ifdef __GNUC__
#include <inline/xpkmaster.h>
#elif defined(__VBCC__)
#include <inline/xpkmaster_protos.h>
#else
#include <pragma/xpkmaster_lib.h>
#endif

#endif	/*  _PROTO_XPKMASTER_H  */
