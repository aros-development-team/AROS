#ifndef INTUITION_INTERN_H
#define INTUITION_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/29 13:33:31  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.2  1996/08/28 17:55:37  digulla
    Proportional gadgets
    BOOPSI

    Revision 1.1  1996/08/13 15:37:26  digulla
    First function for intuition.library

    Desc:
    Lang:
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif

struct IntIntuitionBase
{
    struct IntuitionBase IBase;
    /* Put local shit here, invisible for the user */
    struct GfxBase  * GfxBase;
    struct ExecBase * SysBase;
    struct MinList    ClassList;
    struct Screen   * WorkBench;
};

extern struct IntuitionBase * IntuitionBase;

#define GetPubIBase(ib)   ((struct IntuitionBase *)ib)
#define GetPrivIBase(ib)  ((struct IntIntuitionBase *)ib)

#ifdef GfxBase
#undef GfxBase
#endif
#define GfxBase     (GetPrivIBase(IntuitionBase)->GfxBase)
#ifdef SysBase
#undef SysBase
#endif
#define SysBase     (GetPrivIBase(IntuitionBase)->SysBase)

#define PublicClassList ((struct List *)&(GetPrivIBase(IntuitionBase)->ClassList))

/* Needed for close() */
#define expunge() \
__AROS_LC0(BPTR, expunge, struct IntuitionBase *, IntuitionBase, 3, Intuition)

#endif /* INTUITION_INTERN_H */
