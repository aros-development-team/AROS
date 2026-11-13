#ifndef _PROTO_PICASSO96API_H
#define _PROTO_PICASSO96API_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_PICASSO96API_PROTOS_H) && !defined(__GNUC__)
#include <clib/Picasso96API_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *P96Base;
#endif

#ifdef __GNUC__
#include <inline/Picasso96API.h>
#elif defined(__VBCC__)
#include <inline/Picasso96API_protos.h>
#else
#include <pragma/Picasso96API_lib.h>
#endif

#endif	/*  _PROTO_PICASSO96API_H  */
