#ifndef INTUITION_CLASSUSR_H
#define INTUITION_CLASSUSR_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: BOOPSI users
    Lang: english
*/

#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif

typedef ULONG  Object;
typedef UBYTE *ClassID;

typedef struct
{
    ULONG MethodID;
} *Msg;

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
    ULONG               MethodID;
    struct TagItem    * ops_AttrList;
    struct GadgetInfo * ops_GInfo;
};

struct opGet
{
    ULONG   MethodID;
    ULONG   opg_AttrID;
    ULONG * opg_Storage;
};

struct opAddTail
{
    ULONG         MethodID;
    struct List * opat_List;
};

struct opUpdate
{
    ULONG               MethodID;
    struct TagItem    * opu_AttrList;
    struct GadgetInfo * opu_GInfo;
    ULONG               opu_Flags;    /* see below */
};

/* opu_Flags */
#define OPUF_INTERIM (1L<<0)

struct opMember
{
    ULONG    MethodID;
    Object * opam_Object;
};
#define opAddMember opMember

#endif /* INTUITION_CLASSUSR_H */
