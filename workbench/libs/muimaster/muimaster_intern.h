#ifndef MUIMASTER_INTERN_H
#define MUIMASTER_INTERN_H

/* Include files */
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef LIBCORE_BASE_H
#include <libcore/base.h>
#endif

struct MUIMasterBase_intern
{
    struct Library   mmb_LibNode;
    BPTR             mmb_SegList;
    struct ExecBase *mmb_SysBase;
};

BOOL destroy_classes();

#endif /* MUIMASTER_INTERN_H */
