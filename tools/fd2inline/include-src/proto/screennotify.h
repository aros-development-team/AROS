#ifndef PROTO_SCREENNOTIFY_H
#define PROTO_SCREENNOTIFY_H

#ifndef INTUITION_SCREENS_H
#include <intuition/screens.h>
#endif /* !INTUITION_SCREENS_H */

#include <clib/screennotify_protos.h>

#ifdef __GNUC__
#include <inline/screennotify.h>
#endif /* __GNUC__ */

#ifndef __NOLIBBASE__
extern struct Library *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
ScreenNotifyBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_SCREENNOTIFY_H */
