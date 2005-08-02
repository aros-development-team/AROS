#ifndef INTUITION_CLASSUSR_H
#define INTUITION_CLASSUSR_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef __typedef_Object
#   define __typedef_Object
    typedef ULONG  Object;
#endif

#ifndef __typedef_ClassID
#   define __typedef_ClassID
    typedef CONST_STRPTR ClassID;
#endif

#ifndef __typedef_Msg
#   define __typedef_Msg
    typedef struct _struct_Msg
    {
        STACKULONG MethodID;
    } *Msg;
#endif

#define ROOTCLASS     "rootclass"
#define IMAGECLASS    "imageclass"
#define FRAMEICLASS   "frameiclass"
#define SYSICLASS     "sysiclass"
#define FILLRECTCLASS "fillrectclass"
#define GADGETCLASS   "gadgetclass"
#define PROPGCLASS    "propgclass"
#define STRGCLASS     "strgclass"
#define BUTTONGCLASS  "buttongclass"
#define FRBUTTONCLASS "frbuttonclass"
#define GROUPGCLASS   "groupgclass"
#define ICCLASS       "icclass"
#define MODELCLASS    "modelclass"
#define ITEXTICLASS   "itexticlass"
#define POINTERCLASS  "pointerclass"

/* public classes existing only in AROS but not AmigaOS */
#define MENUBARLABELCLASS "menubarlabelclass"
#define WINDECORCLASS	  "windecoreclass"

#define OM_Dummy     0x0100
#define OM_NEW       (OM_Dummy + 1)
#define OM_DISPOSE   (OM_Dummy + 2)
#define OM_SET       (OM_Dummy + 3)
#define OM_GET       (OM_Dummy + 4)
#define OM_ADDTAIL   (OM_Dummy + 5)
#define OM_REMOVE    (OM_Dummy + 6)
#define OM_NOTIFY    (OM_Dummy + 7)
#define OM_UPDATE    (OM_Dummy + 8)
#define OM_ADDMEMBER (OM_Dummy + 9)
#define OM_REMMEMBER (OM_Dummy + 10)

struct opSet
{
    STACKULONG          MethodID;
    struct TagItem    * ops_AttrList;
    struct GadgetInfo * ops_GInfo;
};

struct opGet
{
    STACKULONG  MethodID;
    Tag         opg_AttrID;
    IPTR      * opg_Storage;
};

struct opAddTail
{
    STACKULONG    MethodID;
    struct List * opat_List;
};

struct opUpdate
{
    STACKULONG          MethodID;
    struct TagItem    * opu_AttrList;
    struct GadgetInfo * opu_GInfo;
    STACKULONG          opu_Flags;    /* see below */
};

/* opu_Flags */
#define OPUF_INTERIM (1L<<0)

struct opMember
{
    STACKULONG MethodID;
    Object   * opam_Object;
};
#define opAddMember opMember

#endif /* INTUITION_CLASSUSR_H */
