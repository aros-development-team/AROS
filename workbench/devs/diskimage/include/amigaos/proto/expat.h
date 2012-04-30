#ifndef _PROTO_EXPAT_H
#define _PROTO_EXPAT_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_EXPAT_PROTOS_H) && !defined(__GNUC__)
#include <clib/expat_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *ExpatBase;
#endif

#ifdef __GNUC__
#include <inline/expat.h>
#elif defined(__VBCC__)
#include <inline/expat_protos.h>
#else
#include <pragma/expat_lib.h>
#endif

#endif	/*  _PROTO_EXPAT_H  */
