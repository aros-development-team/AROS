#ifndef PROTO_EXEC_H
#define PROTO_EXEC_H

#ifndef EXEC_IO_H
#include <exec/io.h>
#endif /* !EXEC_IO_H */
#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif /* !EXEC_MEMORY_H */
#ifndef EXEC_DEVICES_H
#include <exec/devices.h>
#endif /* !EXEC_DEVICES_H */
#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif /* !EXEC_PORTS_H */
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif /* !EXEC_SEMAPHORES_H */

#include <clib/exec_protos.h>

#ifdef __GNUC__
#ifdef _USEOLDEXEC_
#define EXEC_BASE_NAME (*(struct ExecBase **)4)
#endif /* _USEOLDEXEC_ */
#include <inline/exec.h>
#endif /* __GNUC__ */

#if !defined(__NOLIBBASE__) && !defined(_USEOLDEXEC_)
extern struct ExecBase *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
SysBase;
#endif /* !__NOLIBBASE__ && !_USEOLDEXEC_ */

#ifndef PROTO_ALIB_H
#include <proto/alib.h>
#endif /* !PROTO_ALIB_H */

#endif /* !PROTO_EXEC_H */
