/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Relations between HIDD objects.
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <oop/oop.h>
#include <hidd/hidd.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <aros/symbolsets.h>

#include <stddef.h>
#include <string.h>

#include "hiddclass_intern.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************************

    NAME
        --background_relations--

    LOCATION
        hiddclass

    NOTES
        The subsystem tree, as walked with moHW_EnumDrivers, only tells which
        subsystem created which driver object. It does not tell that a display
        driver runs a particular PCI device, that it owns an I2C bus, or that a
        temperature sensor belongs to it. Relations describe these connections
        in one place, so that they can be queried from any object without
        knowing the class of that object.

        A relation connects two objects and has a type (vHidd_Relation_#?).
        It is declared once, by either of the two objects, using
        moHidd_AddRelation. Both objects then report it, each from its own
        point of view: if a driver declares vHidd_Relation_Device to a PCI
        device object, that object reports the driver as vHidd_Relation_Driver.

        The target of a relation can also be named by its device path (see
        aoHidd_DevicePath) instead of by object pointer. Such a relation
        connects to the object carrying that path whenever one exists, in
        whatever order the two come to life, and it reports the bare path
        while there is none. This is how hardware that is described by another
        component, or not represented by an object at all (an ACPI namespace
        node, for example), is referred to.

        Relations are removed automatically when one of their objects is
        disposed. Subsystem objects (CLID_HW) are HIDD objects too, and every
        driver added with moHW_AddDriver is given a vHidd_Relation_Parent
        relation to its subsystem.

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_DevicePath

    SYNOPSIS
        [I.G], CONST_STRPTR

    LOCATION
        hiddclass

    FUNCTION
        A string identifying the piece of hardware the object represents,
        which is unique in the system and can be worked out by other components
        from what they know about the hardware.

        The path starts with the name of the namespace the hardware is
        enumerated in, followed by a slash and the address inside it:

            pci/0000:00:02.0        PCI segment:bus:device.function (hex). The
                                    segment is the number of the host bridge
                                    driver, in the order they were created.
            pci/0000                The host bridge driver of that segment
            acpi/\_SB_.PCI0.GFX0    Full ACPI namespace path, as returned by
                                    AcpiGetName()
            usb/1-2.3               USB bus, then the chain of ports
            dt/soc/serial@10000000  Device tree node, by its full path with
                                    unit addresses

        The supplied string is copied.

    NOTES
        Objects that do not stand for one addressable piece of hardware do not
        need a path.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_FindDevicePath, moHidd_AddRelation

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
        aoHidd_Parent

    SYNOPSIS
        [I.G], OOP_Object *

    LOCATION
        hiddclass

    FUNCTION
        The object this object belongs to: the subsystem a driver is registered
        in, the controller of a bus, the bus of a unit, the driver of a display.

        This is the vHidd_Relation_Parent relation of the object. Supplying
        the attribute on creation declares that relation, and reading it
        returns the first object the object has such a relation with.

    NOTES
        Objects created with moHW_AddDriver get this relation to their
        subsystem without asking for it.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_AddRelation, moHidd_FindRelated, aoHW_Device

    INTERNALS

*****************************************************************************************/

#define RELATION_INVERSE(type) \
    (((type) & vHidd_RelationF_Symmetric) ? (type) : ((type) ^ vHidd_RelationF_Inverse))

#define RELATION_EDGE(end) \
    (((end)->rn_Flags & RNF_TARGET) ? \
        (struct RelationEdge *)((UBYTE *)(end) - offsetof(struct RelationEdge, re_Target)) : \
        (struct RelationEdge *)(end))

/* Instance data of our class in an arbitrary object, NULL if it is not a HIDD object */
static inline struct HIDDData *relation_Data(struct class_static_data *csd, OOP_Object *o)
{
    OOP_Class *cl;

    for (cl = OOP_OCLASS(o); cl; cl = cl->superclass)
    {
        if (cl == csd->hiddclass)
            return OOP_INST_DATA(cl, o);
    }
    return NULL;
}

static inline CONST_STRPTR relation_PathOf(struct class_static_data *csd, OOP_Object *o)
{
    struct HIDDData *hd = relation_Data(csd, o);

    return (hd && hd->hd_Path) ? (CONST_STRPTR)hd->hd_Path->pn_Node.ln_Name : NULL;
}

static BOOL relation_IsA(OOP_Object *o, CONST_STRPTR classID)
{
    OOP_Class *cl;

    for (cl = OOP_OCLASS(o); cl; cl = cl->superclass)
    {
        if (cl->ClassNode.ln_Name && !strcmp(cl->ClassNode.ln_Name, classID))
            return TRUE;
    }
    return FALSE;
}

static void relation_Free(struct class_static_data *csd, struct RelationEdge *edge)
{
    Remove((struct Node *)&edge->re_Source.rn_Node);
    Remove((struct Node *)&edge->re_Target.rn_Node);
    FreeVecPooled(csd->MemPool, edge);
}

/* Find the end of a relation in an object's list. Called with cs_RelLock held. */
static struct RelationEnd *relation_FindEnd(struct HIDDData *hd, ULONG type,
                                            OOP_Object *other, CONST_STRPTR path)
{
    struct RelationEnd *end;

    ForeachNode(&hd->hd_Relations, end)
    {
        if (end->rn_Type != type)
            continue;

        if (other)
        {
            if (end->rn_Other == other)
                return end;
        }
        else if (!(end->rn_Flags & RNF_TARGET))
        {
            struct RelationEdge *edge = (struct RelationEdge *)end;

            if (edge->re_Path && !strcmp(edge->re_Path, path))
                return end;
        }
    }
    return NULL;
}

BOOL relation_Add(struct class_static_data *csd, OOP_Object *o, ULONG type,
                  OOP_Object *target, CONST_STRPTR path, CONST_STRPTR label)
{
    struct HIDDData *hd = relation_Data(csd, o);
    struct HIDDData *td = NULL;
    BOOL ok = FALSE;

    if (!hd || (type == vHidd_Relation_Any) || (target == o))
        return FALSE;

    if (target)
    {
        /* An object named directly has to be able to carry its end */
        path = NULL;
        td = relation_Data(csd, target);
        if (!td)
            return FALSE;
    }
    else if (!path)
        return FALSE;

    ObtainSemaphore(&csd->cs_RelLock);

    if (path)
    {
        struct PathNode *pn = (struct PathNode *)FindName(&csd->cs_Paths, path);

        if (pn)
        {
            target = pn->pn_Object;
            td = relation_Data(csd, target);
        }
    }

    if (target == o)
    {
        /* Our own path */
    }
    else if ((path && relation_FindEnd(hd, type, NULL, path)) ||
             (target && relation_FindEnd(hd, type, target, NULL)))
    {
        /* Declared before, possibly by the other object */
        ok = TRUE;
    }
    else
    {
        struct RelationEdge *edge;
        ULONG pathlen  = path  ? strlen(path)  + 1 : 0;
        ULONG labellen = label ? strlen(label) + 1 : 0;

        edge = AllocVecPooled(csd->MemPool, sizeof(struct RelationEdge) + pathlen + labellen);
        if (edge)
        {
            STRPTR str = (STRPTR)(edge + 1);

            if (path)
            {
                edge->re_Path = str;
                CopyMem(path, str, pathlen);
                str += pathlen;
            }
            if (label)
            {
                edge->re_Label = str;
                CopyMem(label, str, labellen);
            }

            edge->re_Source.rn_Other = target;
            edge->re_Source.rn_Type  = type;
            edge->re_Source.rn_Flags = 0;
            edge->re_Target.rn_Other = o;
            edge->re_Target.rn_Type  = RELATION_INVERSE(type);
            edge->re_Target.rn_Flags = RNF_TARGET;

            ADDTAIL(&hd->hd_Relations, &edge->re_Source.rn_Node);
            if (td)
                ADDTAIL(&td->hd_Relations, &edge->re_Target.rn_Node);
            else
                ADDTAIL(&csd->cs_Unresolved, &edge->re_Target.rn_Node);

            ok = TRUE;
        }
    }

    ReleaseSemaphore(&csd->cs_RelLock);

    D(bug("[HIDD] %s: 0x%p -[%08x]-> 0x%p '%s' %s\n", __func__, o, type, target,
          path ? path : (CONST_STRPTR)"", ok ? "ok" : "failed"));

    return ok;
}

/* Give an object its device path, and connect the relations that wait for it */
void relation_SetPath(struct class_static_data *csd, OOP_Object *o, struct HIDDData *hd,
                      CONST_STRPTR path)
{
    ULONG len = strlen(path) + 1;
    struct PathNode *pn = AllocVecPooled(csd->MemPool, sizeof(struct PathNode) + len);

    if (pn)
    {
        struct RelationEnd *end, *next;

        pn->pn_Node.ln_Name = (char *)(pn + 1);
        pn->pn_Object = o;
        CopyMem(path, pn->pn_Node.ln_Name, len);

        ObtainSemaphore(&csd->cs_RelLock);

        /* If the path is taken already, the first object keeps answering for it */
        if (!FindName(&csd->cs_Paths, path))
        {
            ForeachNodeSafe(&csd->cs_Unresolved, end, next)
            {
                struct RelationEdge *edge = RELATION_EDGE(end);

                if (!strcmp(edge->re_Path, path))
                {
                    Remove((struct Node *)&end->rn_Node);
                    ADDTAIL(&hd->hd_Relations, &end->rn_Node);
                    edge->re_Source.rn_Other = o;
                }
            }
        }
        ADDTAIL(&csd->cs_Paths, &pn->pn_Node);
        hd->hd_Path = pn;

        ReleaseSemaphore(&csd->cs_RelLock);
    }
}

/* Called when an object is disposed */
void relation_Dispose(struct class_static_data *csd, OOP_Object *o, struct HIDDData *hd)
{
    struct RelationEnd *end, *next;

    ObtainSemaphore(&csd->cs_RelLock);

    ForeachNodeSafe(&hd->hd_Relations, end, next)
    {
        struct RelationEdge *edge = RELATION_EDGE(end);

        if ((end->rn_Flags & RNF_TARGET) && edge->re_Path)
        {
            /* We were named by path. The declarer keeps its relation, without an object. */
            Remove((struct Node *)&end->rn_Node);
            ADDTAIL(&csd->cs_Unresolved, &end->rn_Node);
            edge->re_Source.rn_Other = NULL;
        }
        else
            relation_Free(csd, edge);
    }

    if (hd->hd_Path)
    {
        Remove(&hd->hd_Path->pn_Node);
        FreeVecPooled(csd->MemPool, hd->hd_Path);
        hd->hd_Path = NULL;
    }

    ReleaseSemaphore(&csd->cs_RelLock);
}

OOP_Object *relation_Find(struct class_static_data *csd, struct HIDDData *hd, ULONG type,
                          CONST_STRPTR classID)
{
    struct RelationEnd *end;
    OOP_Object *found = NULL;

    ObtainSemaphoreShared(&csd->cs_RelLock);

    ForeachNode(&hd->hd_Relations, end)
    {
        if (end->rn_Other &&
            ((type == vHidd_Relation_Any) || (end->rn_Type == type)) &&
            (!classID || relation_IsA(end->rn_Other, classID)))
        {
            found = end->rn_Other;
            break;
        }
    }

    ReleaseSemaphore(&csd->cs_RelLock);

    return found;
}

/*****************************************************************************************

    NAME
        moHidd_AddRelation

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_AddRelation *Msg);

        BOOL HIDD_AddRelation(OOP_Object *obj, ULONG type, OOP_Object *target,
                              struct TagItem *tags);

    LOCATION
        hiddclass

    FUNCTION
        Declares a relation between the object and another object.

    INPUTS
        obj    - The object declaring the relation.
        type   - What the target is to the object, one of vHidd_Relation_#?:

                    vHidd_Relation_Parent      - The object containing or managing it.
                    vHidd_Relation_Child       - An object it contains or manages.
                    vHidd_Relation_Device      - The bus device object it operates.
                    vHidd_Relation_Driver      - A driver object operating it.
                    vHidd_Relation_Uses        - A resource it depends on.
                    vHidd_Relation_UsedBy      - An object depending on it.
                    vHidd_Relation_Exposes     - An object it publishes, for example
                                                 a telemetry object or a display.
                    vHidd_Relation_ExposedBy   - The object publishing it.
                    vHidd_Relation_Firmware    - Its description in the firmware.
                    vHidd_Relation_FirmwareOf  - The hardware it describes.
                    vHidd_Relation_PoweredBy   - What supplies it with power.
                    vHidd_Relation_Powers      - What it supplies with power.
                    vHidd_Relation_Monitors    - The hardware its readings are about.
                    vHidd_Relation_MonitoredBy - An object reading its state.
                    vHidd_Relation_Companion   - Another function of the same device.

                 Types from vHidd_Relation_User on are free for private use.
        target - The other object, which has to be a HIDD object too, or NULL
                 if it is named with tHidd_Relation_Path.
        tags   - Optional additional information:

                    tHidd_Relation_Path (CONST_STRPTR)
                        Device path of the target. The relation connects to the
                        object with this aoHidd_DevicePath when there is one,
                        and follows it coming and going. Ignored when a target
                        object is given.
                    tHidd_Relation_Label (CONST_STRPTR)
                        A short description of what the target is used as,
                        such as "DDC bus" or "Core temperature".

                 The strings are copied.

    RESULT
        TRUE if the relation exists now, FALSE in case of failure.

    NOTES
        A relation needs to be declared by one of its two objects only. Declaring
        one that exists already, from either side, succeeds without effect.

        This method must not be called from within a moHidd_EnumRelations hook.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_RemoveRelation, moHidd_EnumRelations, moHidd_FindRelated, aoHidd_DevicePath

    INTERNALS

*****************************************************************************************/

BOOL HIDDCl__Hidd__AddRelation(OOP_Class *cl, OOP_Object *o, struct pHidd_AddRelation *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;

    return relation_Add(CSD(cl), o, msg->type, msg->target,
                        (CONST_STRPTR)GetTagData(tHidd_Relation_Path, 0, msg->tags),
                        (CONST_STRPTR)GetTagData(tHidd_Relation_Label, 0, msg->tags));
}

/*****************************************************************************************

    NAME
        moHidd_RemoveRelation

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_RemoveRelation *Msg);

        BOOL HIDD_RemoveRelation(OOP_Object *obj, ULONG type, OOP_Object *target,
                                 CONST_STRPTR path);

    LOCATION
        hiddclass

    FUNCTION
        Removes a relation of the object.

    INPUTS
        obj    - The object.
        type   - Type of the relation, as seen from the object.
        target - The related object, or NULL to select the relation by path.
        path   - The device path the relation was declared with. Only used if
                 target is NULL.

    RESULT
        TRUE if the relation was found and removed.

    NOTES
        There is no need to remove relations before disposing an object.

        This method must not be called from within a moHidd_EnumRelations hook.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_AddRelation

    INTERNALS

*****************************************************************************************/

BOOL HIDDCl__Hidd__RemoveRelation(OOP_Class *cl, OOP_Object *o, struct pHidd_RemoveRelation *msg)
{
    struct class_static_data *csd = CSD(cl);
    struct HIDDData *hd = OOP_INST_DATA(cl, o);
    struct RelationEnd *end = NULL;

    if (msg->target || msg->path)
    {
        ObtainSemaphore(&csd->cs_RelLock);

        end = relation_FindEnd(hd, msg->type, msg->target, msg->path);
        if (end)
            relation_Free(csd, RELATION_EDGE(end));

        ReleaseSemaphore(&csd->cs_RelLock);
    }

    return end ? TRUE : FALSE;
}

/*****************************************************************************************

    NAME
        moHidd_EnumRelations

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_EnumRelations *Msg);

        VOID HIDD_EnumRelations(OOP_Object *obj, ULONG type, struct Hook *callback,
                                APTR hookMsg);

    LOCATION
        hiddclass

    FUNCTION
        Enumerates the relations of the object, including those declared by
        other objects.

    INPUTS
        obj      - The object to query.
        type     - The type of relation to report, or vHidd_Relation_Any.
        callback - A user-supplied hook which will be called for every relation.
        hookMsg  - A user-defined data to be passed to the hook.

        The hook will be called with the following parameters:
            AROS_UFHA(struct Hook *         , hook    , A0)
                - A pointer to hook structure itself
            AROS_UFHA(struct HIDD_Relation *, relation, A2)
                - Description of the relation:
                    hr_Type   - Its type, as seen from the object queried.
                    hr_Object - The related object, or NULL if the relation
                                was declared by path and there is no object
                                with this path at the moment.
                    hr_Path   - Device path of the related object, or NULL.
                    hr_Label  - Description of the relation, or NULL.
            AROS_UFHA(APTR                  , message , A1)
                - User-defined data

        The hook should return FALSE in order to continue enumeration
        or TRUE in order to stop it.

    RESULT
        None.

    NOTES
        The structure and its strings are only valid during the call.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_FindRelated, moHidd_AddRelation

    INTERNALS
        The function uses internal semaphore locking. Because of this,
        it is illegal to attempt to add or remove relations, or to dispose
        HIDD objects, within the hook. Querying relations is allowed.

*****************************************************************************************/

VOID HIDDCl__Hidd__EnumRelations(OOP_Class *cl, OOP_Object *o, struct pHidd_EnumRelations *msg)
{
    struct class_static_data *csd = CSD(cl);
    struct HIDDData *hd = OOP_INST_DATA(cl, o);
    struct RelationEnd *end;

    ObtainSemaphoreShared(&csd->cs_RelLock);

    ForeachNode(&hd->hd_Relations, end)
    {
        struct HIDD_Relation rel;
        BOOL stop;

        if ((msg->type != vHidd_Relation_Any) && (end->rn_Type != msg->type))
            continue;

        rel.hr_Type   = end->rn_Type;
        rel.hr_Object = end->rn_Other;
        rel.hr_Label  = RELATION_EDGE(end)->re_Label;
        if (end->rn_Other)
            rel.hr_Path = relation_PathOf(csd, end->rn_Other);
        else
            rel.hr_Path = RELATION_EDGE(end)->re_Path;

        /* See moHW_EnumDrivers for the cast */
        stop = CALLHOOKPKT(msg->callback, &rel, msg->hookMsg);
        if (stop)
            break;
    }

    ReleaseSemaphore(&csd->cs_RelLock);
}

/*****************************************************************************************

    NAME
        moHidd_FindRelated

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_FindRelated *Msg);

        OOP_Object *HIDD_FindRelated(OOP_Object *obj, ULONG type, CONST_STRPTR classID);

    LOCATION
        hiddclass

    FUNCTION
        Returns the first object the object has a relation of the given kind with.

    INPUTS
        obj     - The object to query.
        type    - The type of relation, or vHidd_Relation_Any.
        classID - If not NULL, only objects of this public class, or of a
                  subclass of it, are considered.

    RESULT
        A pointer to the related object, or NULL if there is none.

    NOTES

    EXAMPLE
        // The subsystem a driver is registered in
        subsystem = HIDD_FindRelated(driver, vHidd_Relation_Parent, NULL);

        // The first sensor a driver publishes
        sensor = HIDD_FindRelated(driver, vHidd_Relation_Exposes, CLID_Hidd_Telemetry);

    BUGS

    SEE ALSO
        moHidd_EnumRelations

    INTERNALS

*****************************************************************************************/

OOP_Object *HIDDCl__Hidd__FindRelated(OOP_Class *cl, OOP_Object *o, struct pHidd_FindRelated *msg)
{
    return relation_Find(CSD(cl), OOP_INST_DATA(cl, o), msg->type, msg->classID);
}

/*****************************************************************************************

    NAME
        moHidd_FindDevicePath

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_FindDevicePath *Msg);

        OOP_Object *HIDD_FindDevicePath(OOP_Object *obj, CONST_STRPTR path);

    LOCATION
        hiddclass

    FUNCTION
        Finds the object representing a piece of hardware.

    INPUTS
        obj  - Any HIDD object. The result does not depend on it. The
               CLID_HW_Root object is the usual choice.
        path - The device path to look for.

    RESULT
        A pointer to the object which was created with this aoHidd_DevicePath,
        or NULL if there is none.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_DevicePath

    INTERNALS

*****************************************************************************************/

OOP_Object *HIDDCl__Hidd__FindDevicePath(OOP_Class *cl, OOP_Object *o, struct pHidd_FindDevicePath *msg)
{
    struct class_static_data *csd = CSD(cl);
    OOP_Object *found = NULL;

    if (msg->path)
    {
        struct PathNode *pn;

        ObtainSemaphoreShared(&csd->cs_RelLock);
        pn = (struct PathNode *)FindName(&csd->cs_Paths, msg->path);
        if (pn)
            found = pn->pn_Object;
        ReleaseSemaphore(&csd->cs_RelLock);
    }

    return found;
}

/* This has to be ready before the first object is created */
static int relation_Init(LIBBASETYPEPTR lh)
{
    struct class_static_data *csd = &lh->hd_csd;

    InitSemaphore(&csd->cs_RelLock);
    NEWLIST(&csd->cs_Unresolved);
    NEWLIST(&csd->cs_Paths);

    return TRUE;
}

ADD2INITLIB(relation_Init, -10)
