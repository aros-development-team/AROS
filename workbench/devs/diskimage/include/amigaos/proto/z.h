#ifndef _PROTO_Z_H
#define _PROTO_Z_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_Z_PROTOS_H) && !defined(__GNUC__)
#include <clib/z_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *ZBase;
#endif

#ifdef __GNUC__
#include <inline/z.h>
#elif defined(__VBCC__)
#include <inline/z_protos.h>
#else
#include <pragma/z_lib.h>
#endif

#endif	/*  _PROTO_Z_H  */
