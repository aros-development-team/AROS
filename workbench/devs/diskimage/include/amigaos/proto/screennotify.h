#ifndef _PROTO_SCREENNOTIFY_H
#define _PROTO_SCREENNOTIFY_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#if !defined(CLIB_SCREENNOTIFY_PROTOS_H) && !defined(__GNUC__)
#include <clib/screennotify_protos.h>
#endif

#ifndef __NOLIBBASE__
extern struct Library *ScreenNotifyBase;
#endif

#ifdef __GNUC__
#include <inline/screennotify.h>
#elif defined(__VBCC__)
#include <inline/screennotify_protos.h>
#else
#include <pragma/screennotify_lib.h>
#endif

#endif	/*  _PROTO_SCREENNOTIFY_H  */
