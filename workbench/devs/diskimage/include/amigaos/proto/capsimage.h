#ifndef _PROTO_CAPSIMAGE_H
#define _PROTO_CAPSIMAGE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_CAPSIMAGE_PROTOS_H) && !defined(__GNUC__)
#include <clib/capsimage_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Device *CapsImageBase;
#endif

#ifdef __GNUC__
#include <inline/capsimage.h>
#elif defined(__VBCC__)
#include <inline/capsimage_protos.h>
#else
#include <pragma/capsimage_lib.h>
#endif

#endif	/*  _PROTO_CAPSIMAGE_H  */
