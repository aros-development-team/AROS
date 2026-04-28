/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction library internal definitions
*/

#ifndef REACTION_INTERN_H
#define REACTION_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif
#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#include LC_LIBDEFS_FILE

struct ReactionBase
{
    struct Library          aui_Lib;
    struct SignalSemaphore  aui_Lock;
    APTR                    aui_SysBase;
    APTR                    aui_IntuitionBase;
    APTR                    aui_UtilityBase;
};

#define ARB(b) ((struct ReactionBase *)(b))

#define GM_SYSBASE_FIELD(b) (ARB(b)->aui_SysBase)
#define SysBase         ((struct ExecBase *)ARB(ReactionBase)->aui_SysBase)
#define IntuitionBase   (ARB(ReactionBase)->aui_IntuitionBase)
#define UtilityBase     (ARB(ReactionBase)->aui_UtilityBase)

#endif /* REACTION_INTERN_H */
