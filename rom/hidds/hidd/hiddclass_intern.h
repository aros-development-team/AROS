#ifndef HIDD_CLASS_INTERN_H
#define HIDD_CLASS_INTERN_H

/* Include files */

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif


/*
 * Object relations.
 *
 * A relation is a single edge between two objects. It has two ends, and each
 * end is linked into the relation list of the object it belongs to. An end
 * holds the relation as seen from its own side, so enumerating the relations
 * of an object is a plain walk of one list.
 *
 * A relation declared by device path whose target does not (or no longer)
 * exist keeps its target end on class_static_data::cs_Unresolved.
 */
struct RelationEnd
{
    struct MinNode  rn_Node;
    OOP_Object     *rn_Other;   /* Object at the other end, NULL if unresolved    */
    ULONG           rn_Type;    /* vHidd_Relation_#? as seen from this end        */
    ULONG           rn_Flags;
};

#define RNF_TARGET  (1 << 0)    /* This is the end at the target of the relation  */

struct RelationEdge
{
    struct RelationEnd re_Source;   /* End at the object that declared the relation */
    struct RelationEnd re_Target;
    STRPTR             re_Path;     /* Device path the target was named by, or NULL */
    STRPTR             re_Label;    /* Description, or NULL                         */
    /* The strings follow */
};

/* An object registered under a device path. ln_Name is the path, stored behind the node. */
struct PathNode
{
    struct Node  pn_Node;
    OOP_Object  *pn_Object;
};

struct HIDDData
{
    UWORD       hd_Type;
    UWORD       hd_SubType;
    ULONG       hd_Producer;
    ULONG	hd_Product;
    STRPTR      hd_Name;
    STRPTR      hd_HWName;
    STRPTR	hd_ProducerName;
    BOOL        hd_Active;
    UWORD       hd_Locking;
    ULONG       hd_Status;
    ULONG       hd_ErrorCode;
    struct MinList   hd_Relations;  /* RelationEnds of this object */
    struct PathNode *hd_Path;       /* Our device path, if we have one */
};

struct DriverNode
{
    struct MinNode node;
    OOP_Object     *driverObject;  /* Driver object */
};

struct HWData
{
    const char            *name;
    struct MinList         drivers;
    struct SignalSemaphore driver_lock;
};

/* Static Data for the hiddclass. */
struct class_static_data
{
    OOP_AttrBase                hiddAttrBase;  // keep lower case so it does not clash with define.
    OOP_AttrBase                hwAttrBase;
    OOP_MethodID                hwMethodBase;

    OOP_Class                   *hiddclass;
    OOP_Class                   *hwclass;
    OOP_Class                   *rootclass;

    OOP_Object                  *hwroot;
    APTR                        MemPool;

    struct Library              *cs_OOPBase;
    struct Library              *cs_UtilityBase;

    struct SignalSemaphore      cs_RelLock;     /* Protects everything below, and all hd_Relations */
    struct MinList              cs_Unresolved;  /* Target ends waiting for their device path       */
    struct List                 cs_Paths;       /* PathNodes                                       */
};

/* Shared between the classes of this module */
struct HIDDData;

BOOL relation_Add(struct class_static_data *csd, OOP_Object *o, ULONG type,
                  OOP_Object *target, CONST_STRPTR path, CONST_STRPTR label);
OOP_Object *relation_Find(struct class_static_data *csd, struct HIDDData *hd, ULONG type,
                          CONST_STRPTR classID);
void relation_SetPath(struct class_static_data *csd, OOP_Object *o, struct HIDDData *hd,
                      CONST_STRPTR path);
void relation_Dispose(struct class_static_data *csd, OOP_Object *o, struct HIDDData *hd);


/* Library base */

struct IntHIDDClassBase
{
    struct Library            hd_LibNode;

    struct class_static_data  hd_csd;
};


#define CSD(cl) (&((struct IntHIDDClassBase *)cl->UserData)->hd_csd)

#undef HiddAttrBase
#undef HWAttrBase
#undef HWBase
#define HiddAttrBase (CSD(cl)->hiddAttrBase)
#define HWAttrBase   (CSD(cl)->hwAttrBase)
#define HWBase       (CSD(cl)->hwMethodBase)

#endif /* HIDD_CLASS_INTERN_H */
