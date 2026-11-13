#ifndef _PROTO_DISKIMAGE_H
#define _PROTO_DISKIMAGE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_DISKIMAGE_PROTOS_H) && !defined(__GNUC__)
#include <clib/diskimage_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *DiskImageBase;
#endif

#ifdef __GNUC__
#include <inline/diskimage.h>
#elif defined(__VBCC__)
#include <inline/diskimage_protos.h>
#else
#include <pragma/diskimage_lib.h>
#endif

#endif	/*  _PROTO_DISKIMAGE_H  */
