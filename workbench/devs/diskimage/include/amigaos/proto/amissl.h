#ifndef _PROTO_AMISSL_H
#define _PROTO_AMISSL_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_AMISSL_PROTOS_H) && !defined(__GNUC__)
#include <clib/amissl_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *AmiSSLBase;
#endif

#ifdef __GNUC__
#include <inline/amissl.h>
#elif defined(__VBCC__)
#include <inline/amissl_protos.h>
#else
#include <pragma/amissl_lib.h>
#endif

#endif	/*  _PROTO_AMISSL_H  */
