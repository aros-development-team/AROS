#ifndef _PROTO_MPEGA_H
#define _PROTO_MPEGA_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_MPEGA_PROTOS_H) && !defined(__GNUC__)
#include <clib/mpega_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *MPEGABase;
#endif

#ifdef __GNUC__
#include <inline/mpega.h>
#elif defined(__VBCC__)
#include <inline/mpega_protos.h>
#else
#include <pragma/mpega_lib.h>
#endif

#endif	/*  _PROTO_MPEGA_H  */
