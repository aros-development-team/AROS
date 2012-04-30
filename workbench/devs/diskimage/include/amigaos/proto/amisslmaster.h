#ifndef _PROTO_AMISSLMASTER_H
#define _PROTO_AMISSLMASTER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_AMISSLMASTER_PROTOS_H) && !defined(__GNUC__)
#include <clib/amisslmaster_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *AmiSSLMasterBase;
#endif

#ifdef __GNUC__
#include <inline/amisslmaster.h>
#elif defined(__VBCC__)
#include <inline/amisslmaster_protos.h>
#else
#include <pragma/amisslmaster_lib.h>
#endif

#endif	/*  _PROTO_AMISSLMASTER_H  */
