/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Internal information for aros.library.
    Lang:
*/
#ifndef _AROS_INTERN_H_
#define _AROS_INTERN_H_

/*
    This is the internal version of the ArosBase structure
*/

struct ArosBase
{
    struct Library       aros_LibNode;
    struct ExecBase     *aros_SysBase;
    BPTR                 aros_SegList;

};

/* digulla again... Needed for close() */
#define expunge() \
 AROS_LC0(BPTR, expunge, struct ArosBase *, ArosBase, 3, Aros)

#define SysBase         UtilityBase->aros_SysBase

#endif /* _AROS_INTERN_H */
