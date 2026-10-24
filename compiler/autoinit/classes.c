/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: autoinit library - class creation for genmodule generated modules
*/

#ifndef __OOP_NOLIBBASE__
#define __OOP_NOLIBBASE__
#endif

#include <aros/symbolsets.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <utility/tagitem.h>
#include <proto/oop.h>

/* The class creation genmodule used to emit inline into every OOP class's
   startup function. The tags, their order and the order of the steps
   after the class is created must stay exactly as the generated code had
   them. */
int _set_oop_addclass(const struct __aros_oopclass *c, OOP_Class *superptr,
                      APTR libbase, OOP_Class **clptr, struct Library *OOPBase)
{
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
    OOP_Class *cl;
    struct TagItem tags[5], *t = tags;

    if (MetaAttrBase == 0)
        return FALSE;

    if (c->superid)
    {
        t->ti_Tag = aMeta_SuperID;
        t->ti_Data = (IPTR)c->superid;
    }
    else
    {
        t->ti_Tag = aMeta_SuperPtr;
        t->ti_Data = (IPTR)superptr;
    }
    t++;
    t->ti_Tag = aMeta_InterfaceDescr;
    t->ti_Data = (IPTR)c->ifdescr;
    t++;
    t->ti_Tag = aMeta_InstSize;
    t->ti_Data = c->instsize;
    t++;
    if (c->classid)
    {
        t->ti_Tag = aMeta_ID;
        t->ti_Data = (IPTR)c->classid;
        t++;
    }
    t->ti_Tag = TAG_DONE;
    t->ti_Data = 0;

    cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    if (cl != NULL)
    {
        cl->UserData = libbase;
        *clptr = cl;
        OOP_AddClass(cl);
    }

    OOP_ReleaseAttrBase(IID_Meta);
    return cl != NULL;
}

void _set_oop_removeclass(OOP_Class *cl, struct Library *OOPBase)
{
    if (cl != NULL)
    {
        OOP_RemoveClass(cl);
        OOP_DisposeObject((OOP_Object *)cl);
    }
}
