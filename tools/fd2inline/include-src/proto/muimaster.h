#ifndef PROTO_MUIMASTER_H
#define PROTO_MUIMASTER_H

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif /* !LIBRARIES_MUI_H */

#include <clib/muimaster_protos.h>

#ifdef __GNUC__
#include <inline/muimaster.h>
#endif /* __GNUC__ */

#ifndef __NOLIBBASE__
extern struct Library *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
MUIMasterBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_MUIMASTER_H */
