/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Regression test for the relations of HIDD objects (moHidd_AddRelation and
    friends, aoHidd_DevicePath), and for the vHidd_Relation_Parent relations
    the subsystem tree is given by moHW_AddDriver.

    With the argument DUMP it also prints the relations of every object in
    the subsystem tree.
*/

#include <aros/asmcall.h>
#include <dos/dos.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <stdio.h>
#include <string.h>

OOP_AttrBase HiddAttrBase;
OOP_AttrBase HWAttrBase;

static const struct OOP_ABDescr abd[] =
{
    {IID_Hidd, &HiddAttrBase},
    {IID_HW  , &HWAttrBase  },
    {NULL    , NULL         }
};

static ULONG checks, failures;

#define CHECK(cond, name) \
    do { checks++; if (!(cond)) { failures++; printf("FAIL: %s (line %d)\n", name, __LINE__); } } while (0)

/* What an enumeration saw */
struct Seen
{
    ULONG        count;
    ULONG        stopAfter;     /* Stop the enumeration after this many, 0 = never */
    ULONG        type;
    OOP_Object  *object;
    char         path[64];
    char         label[64];
    BOOL         hasPath, hasLabel;
};

AROS_UFH3S(BOOL, seenFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct HIDD_Relation *, rel, A2),
    AROS_UFHA(struct Seen *, seen, A1))
{
    AROS_USERFUNC_INIT

    seen->count++;
    seen->type   = rel->hr_Type;
    seen->object = rel->hr_Object;
    seen->hasPath = seen->hasLabel = FALSE;
    if (rel->hr_Path)
    {
        strncpy(seen->path, rel->hr_Path, sizeof(seen->path) - 1);
        seen->hasPath = TRUE;
    }
    if (rel->hr_Label)
    {
        strncpy(seen->label, rel->hr_Label, sizeof(seen->label) - 1);
        seen->hasLabel = TRUE;
    }

    return (seen->stopAfter && (seen->count >= seen->stopAfter)) ? TRUE : FALSE;

    AROS_USERFUNC_EXIT
}

static struct Hook seenHook = { .h_Entry = (APTR)seenFunc };

static ULONG countRelations(OOP_Object *o, ULONG type, struct Seen *seen)
{
    struct Seen local;

    if (!seen)
        seen = &local;
    memset(seen, 0, sizeof(*seen));
    HIDD_EnumRelations(o, type, &seenHook, seen);

    return seen->count;
}

static OOP_Object *newObject(CONST_STRPTR name, CONST_STRPTR path)
{
    struct TagItem tags[] =
    {
        {aHidd_Name                            , (IPTR)name},
        {path ? aHidd_DevicePath : TAG_IGNORE  , (IPTR)path},
        {TAG_DONE                              , 0         }
    };

    return OOP_NewObject(NULL, CLID_Hidd, tags);
}

static BOOL addRelation(OOP_Object *o, ULONG type, OOP_Object *target, CONST_STRPTR path, CONST_STRPTR label)
{
    struct TagItem tags[] =
    {
        {path  ? tHidd_Relation_Path  : TAG_IGNORE, (IPTR)path },
        {label ? tHidd_Relation_Label : TAG_IGNORE, (IPTR)label},
        {TAG_DONE                                 , 0          }
    };

    return HIDD_AddRelation(o, type, target, tags);
}

/*** The subsystem tree ******************************************************/

/* Method IDs only mean something to the classes which have the interface, so check before calling */
static BOOL isA(OOP_Object *o, CONST_STRPTR classID)
{
    OOP_Class *cl;

    for (cl = OOP_OCLASS(o); cl; cl = cl->superclass)
    {
        if (cl->ClassNode.ln_Name && !strcmp(cl->ClassNode.ln_Name, classID))
            return TRUE;
    }
    return FALSE;
}

struct TreeMsg
{
    OOP_Object *parent;
    ULONG       drivers;
};

static struct Hook treeHook;

/* Every driver a subsystem lists has to have the subsystem as its parent */
static void checkTree(OOP_Object *o)
{
    struct TreeMsg msg = { o, 0 };

    if (isA(o, CLID_HW))
    {
        HW_EnumDrivers(o, &treeHook, &msg);
        /* There can be more children than drivers */
        CHECK(countRelations(o, vHidd_Relation_Child, NULL) >= msg.drivers, "subsystem reports its drivers as children");
    }
}

AROS_UFH3S(BOOL, treeFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(OOP_Object *, driver, A2),
    AROS_UFHA(struct TreeMsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    if (isA(driver, CLID_Hidd))
    {
        IPTR parent = 0;

        msg->drivers++;
        CHECK(HIDD_FindRelated(driver, vHidd_Relation_Parent, NULL) == msg->parent, "driver finds its subsystem");
        OOP_GetAttr(driver, aHidd_Parent, &parent);
        CHECK((OOP_Object *)parent == msg->parent, "driver reports its subsystem as aHidd_Parent");
        checkTree(driver);
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}

/*** DUMP: everything reachable from the hardware root, by relations alone ***/

static const char *typeName(ULONG type)
{
    static char buf[24];

    switch (type)
    {
    case vHidd_Relation_Child:          return "Child";
    case vHidd_Relation_Parent:         return "Parent";
    case vHidd_Relation_Driver:         return "Driver";
    case vHidd_Relation_Device:         return "Device";
    case vHidd_Relation_UsedBy:         return "UsedBy";
    case vHidd_Relation_Uses:           return "Uses";
    case vHidd_Relation_Exposes:        return "Exposes";
    case vHidd_Relation_ExposedBy:      return "ExposedBy";
    case vHidd_Relation_FirmwareOf:     return "FirmwareOf";
    case vHidd_Relation_Firmware:       return "Firmware";
    case vHidd_Relation_Powers:         return "Powers";
    case vHidd_Relation_PoweredBy:      return "PoweredBy";
    case vHidd_Relation_Monitors:       return "Monitors";
    case vHidd_Relation_MonitoredBy:    return "MonitoredBy";
    case vHidd_Relation_Companion:      return "Companion";
    }
    snprintf(buf, sizeof(buf), "0x%08x", (unsigned)type);
    return buf;
}

static const char *objectName(OOP_Object *o)
{
    CONST_STRPTR name = NULL;

    if (!o)
        return "(no object)";

    OOP_GetAttr(o, aHW_ClassName, (IPTR *)&name);
    if (!name)
        OOP_GetAttr(o, aHidd_HardwareName, (IPTR *)&name);
    if (!name)
        OOP_GetAttr(o, aHidd_Name, (IPTR *)&name);

    return name ? (const char *)name : "(unnamed)";
}

static struct Hook dumpHook;

static void dumpObject(OOP_Object *o, IPTR depth)
{
    CONST_STRPTR path = NULL;

    OOP_GetAttr(o, aHidd_DevicePath, (IPTR *)&path);
    printf("%*s%s [%s]", (int)(depth * 2), "", objectName(o), OOP_OCLASS(o)->ClassNode.ln_Name);
    if (path)
        printf(" <%s>", path);
    printf("\n");

    /* First what it is connected to, then what it contains */
    HIDD_EnumRelations(o, vHidd_Relation_Any, &dumpHook, (APTR)(depth | 0x10000));
    HIDD_EnumRelations(o, vHidd_Relation_Child, &dumpHook, (APTR)depth);
}

AROS_UFH3S(BOOL, dumpFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct HIDD_Relation *, rel, A2),
    AROS_UFHA(IPTR, depth, A1))
{
    AROS_USERFUNC_INIT

    if (!(depth & 0x10000))
        dumpObject(rel->hr_Object, depth + 1);
    else if ((rel->hr_Type != vHidd_Relation_Child) && (rel->hr_Type != vHidd_Relation_Parent))
    {
        printf("%*s  -> %s: %s", (int)((depth & 0xffff) * 2), "", typeName(rel->hr_Type), objectName(rel->hr_Object));
        if (rel->hr_Path)
            printf(" <%s>", rel->hr_Path);
        if (rel->hr_Label)
            printf(" \"%s\"", rel->hr_Label);
        printf("\n");
    }
    return FALSE;

    AROS_USERFUNC_EXIT
}

/*** main ********************************************************************/

int main(int argc, char **argv)
{
    OOP_Object *root, *a, *b, *c, *d, *plain;
    BOOL dump = (argc > 1) && !strcasecmp(argv[1], "DUMP");
    struct Seen seen;
    IPTR val;

    if (!OOP_ObtainAttrBases(abd))
    {
        printf("Failed to obtain attribute bases\n");
        return RETURN_FAIL;
    }

    treeHook.h_Entry = (APTR)treeFunc;
    dumpHook.h_Entry = (APTR)dumpFunc;

    root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    CHECK(root != NULL, "hardware root exists");
    if (root)
    {
        /* Subsystems are HIDD objects */
        val = 0;
        OOP_GetAttr(root, aHidd_Name, &val);
        CHECK(val != 0, "hardware root has HIDD attributes");
        CHECK(HIDD_FindRelated(root, vHidd_Relation_Parent, NULL) == NULL, "hardware root has no parent");

        checkTree(root);
        if (dump)
            dumpObject(root, 0);
    }

    a = newObject("relation.a", "test/a");
    b = newObject("relation.b", NULL);
    c = newObject("relation.c", NULL);
    plain = OOP_NewObject(NULL, CLID_Root, NULL);
    CHECK(a && b && c, "test objects created");
    if (!(a && b && c))
        goto out;

    /* Device paths */
    val = 0;
    OOP_GetAttr(a, aHidd_DevicePath, &val);
    CHECK(val && !strcmp((char *)val, "test/a"), "device path is readable");
    val = 1;
    OOP_GetAttr(b, aHidd_DevicePath, &val);
    CHECK(val == 0, "object without device path reports none");
    CHECK(HIDD_FindDevicePath(b, "test/a") == a, "object is found by device path");
    CHECK(HIDD_FindDevicePath(a, "test/nothing") == NULL, "unknown device path is not found");
    CHECK(HIDD_FindDevicePath(a, NULL) == NULL, "NULL device path is not found");

    /* Refused declarations */
    CHECK(!addRelation(a, vHidd_Relation_Uses, a, NULL, NULL), "relation to oneself is refused");
    CHECK(!addRelation(a, vHidd_Relation_Any, b, NULL, NULL), "relation without type is refused");
    CHECK(!addRelation(a, vHidd_Relation_Uses, NULL, NULL, NULL), "relation without target is refused");
    CHECK(!addRelation(a, vHidd_Relation_Uses, NULL, "test/a", NULL), "relation to the own path is refused");
    if (plain)
        CHECK(!addRelation(a, vHidd_Relation_Uses, plain, NULL, NULL), "relation to a non-HIDD object is refused");
    CHECK(countRelations(a, vHidd_Relation_Any, NULL) == 0, "refused relations leave nothing behind");

    /* A relation and its inverse */
    CHECK(addRelation(a, vHidd_Relation_Uses, b, NULL, "the label"), "relation is declared");
    CHECK(countRelations(a, vHidd_Relation_Any, &seen) == 1, "declarer has one relation");
    CHECK(seen.type == vHidd_Relation_Uses && seen.object == b, "declarer sees the declared type");
    CHECK(seen.hasLabel && !strcmp(seen.label, "the label"), "declarer sees the label");
    CHECK(!seen.hasPath, "target without device path reports none");
    CHECK(countRelations(b, vHidd_Relation_Any, &seen) == 1, "target has one relation");
    CHECK(seen.type == vHidd_Relation_UsedBy && seen.object == a, "target sees the inverse type");
    CHECK(seen.hasLabel && !strcmp(seen.label, "the label"), "target sees the label");
    CHECK(seen.hasPath && !strcmp(seen.path, "test/a"), "device path of the related object is reported");
    CHECK(HIDD_FindRelated(a, vHidd_Relation_Uses, NULL) == b, "related object is found by type");
    CHECK(HIDD_FindRelated(b, vHidd_Relation_UsedBy, NULL) == a, "related object is found by inverse type");
    CHECK(HIDD_FindRelated(a, vHidd_Relation_UsedBy, NULL) == NULL, "inverse type does not match on the declarer");
    CHECK(HIDD_FindRelated(a, vHidd_Relation_Any, CLID_Hidd) == b, "related object is found by class");
    CHECK(HIDD_FindRelated(a, vHidd_Relation_Any, CLID_HW) == NULL, "class filter rejects other classes");

    /* Declaring it again changes nothing, from either side */
    CHECK(addRelation(a, vHidd_Relation_Uses, b, NULL, NULL), "repeated declaration succeeds");
    CHECK(addRelation(b, vHidd_Relation_UsedBy, a, NULL, NULL), "declaration from the other side succeeds");
    CHECK(countRelations(a, vHidd_Relation_Any, NULL) == 1 && countRelations(b, vHidd_Relation_Any, NULL) == 1,
          "repeated declarations add nothing");

    /* Types are told apart */
    CHECK(addRelation(a, vHidd_Relation_Exposes, b, NULL, NULL), "second relation to the same object");
    CHECK(addRelation(a, vHidd_Relation_Companion, c, NULL, NULL), "symmetric relation is declared");
    CHECK(addRelation(a, vHidd_Relation_User + 1, c, NULL, NULL), "private relation is declared");
    CHECK(countRelations(a, vHidd_Relation_Any, NULL) == 4, "all relations are enumerated");
    CHECK(countRelations(a, vHidd_Relation_Exposes, &seen) == 1 && seen.object == b, "enumeration filters by type");
    CHECK(countRelations(b, vHidd_Relation_ExposedBy, &seen) == 1 && seen.object == a, "enumeration filters by inverse type");
    CHECK(countRelations(c, vHidd_Relation_Companion, &seen) == 1 && seen.object == a, "symmetric relation reads the same from the other end");
    CHECK(addRelation(c, vHidd_Relation_Companion, a, NULL, NULL) && countRelations(c, vHidd_Relation_Companion, NULL) == 1,
          "symmetric relation is not duplicated from the other end");
    CHECK(countRelations(c, (vHidd_Relation_User + 1) | vHidd_RelationF_Inverse, NULL) == 1, "private relation is inverted");

    /* The hook can stop the enumeration */
    memset(&seen, 0, sizeof(seen));
    seen.stopAfter = 2;
    HIDD_EnumRelations(a, vHidd_Relation_Any, &seenHook, &seen);
    CHECK(seen.count == 2, "hook stops the enumeration");

    /* Removal */
    CHECK(HIDD_RemoveRelation(b, vHidd_Relation_ExposedBy, a, NULL), "relation is removed from the target side");
    CHECK(!HIDD_RemoveRelation(a, vHidd_Relation_Exposes, b, NULL), "removed relation is gone");
    CHECK(!HIDD_RemoveRelation(a, vHidd_Relation_Uses, NULL, NULL), "removal needs a target");
    CHECK(countRelations(a, vHidd_Relation_Any, NULL) == 3 && countRelations(b, vHidd_Relation_Any, NULL) == 1, "removal takes one relation");

    /* aHW_Device is answered from the relations */
    val = 1;
    OOP_GetAttr(a, aHW_Device, &val);
    CHECK(val == 0, "no device relation, no aHW_Device");
    CHECK(addRelation(a, vHidd_Relation_Device, c, NULL, NULL), "device relation is declared");
    val = 0;
    OOP_GetAttr(a, aHW_Device, &val);
    CHECK((OOP_Object *)val == c, "aHW_Device reports the device relation");
    CHECK(HIDD_FindRelated(c, vHidd_Relation_Driver, NULL) == a, "device finds its driver");

    /* Relations by device path: to an object which exists */
    CHECK(addRelation(c, vHidd_Relation_Monitors, NULL, "test/a", NULL), "relation by path to an existing object");
    CHECK(countRelations(c, vHidd_Relation_Monitors, &seen) == 1 && seen.object == a, "path is resolved at once");
    CHECK(HIDD_FindRelated(a, vHidd_Relation_MonitoredBy, NULL) == c, "object named by path sees the relation");
    CHECK(addRelation(c, vHidd_Relation_Monitors, NULL, "test/a", NULL) && countRelations(c, vHidd_Relation_Monitors, NULL) == 1,
          "relation by path is not duplicated");

    /* ... and to one which does not exist yet */
    CHECK(addRelation(b, vHidd_Relation_Firmware, NULL, "test/late", "late"), "relation by path to a missing object");
    CHECK(countRelations(b, vHidd_Relation_Firmware, &seen) == 1, "unresolved relation is enumerated");
    CHECK(seen.object == NULL && seen.hasPath && !strcmp(seen.path, "test/late"), "unresolved relation reports its path");
    CHECK(HIDD_FindRelated(b, vHidd_Relation_Firmware, NULL) == NULL, "unresolved relation has no object to find");

    d = newObject("relation.d", "test/late");
    CHECK(d != NULL, "late object created");
    if (d)
    {
        CHECK(HIDD_FindRelated(b, vHidd_Relation_Firmware, NULL) == d, "relation connects when the object appears");
        CHECK(countRelations(d, vHidd_Relation_FirmwareOf, &seen) == 1 && seen.object == b, "late object sees the relation");
        CHECK(seen.hasLabel && !strcmp(seen.label, "late"), "late object sees the label");

        OOP_DisposeObject(d);
        CHECK(HIDD_FindDevicePath(a, "test/late") == NULL, "disposed object is not found by path");
        CHECK(countRelations(b, vHidd_Relation_Firmware, &seen) == 1 && seen.object == NULL,
              "relation by path outlives its target");

        d = newObject("relation.d2", "test/late");
        CHECK(d && HIDD_FindRelated(b, vHidd_Relation_Firmware, NULL) == d, "relation connects again");
        CHECK(HIDD_RemoveRelation(b, vHidd_Relation_Firmware, NULL, "test/late"), "relation is removed by path");
        CHECK(d && countRelations(d, vHidd_Relation_Any, NULL) == 0, "removal by path disconnects the target");
        if (d)
            OOP_DisposeObject(d);
    }

    CHECK(addRelation(b, vHidd_Relation_Firmware, NULL, "test/never", NULL), "relation to a path that never appears");

    /* Disposal takes the relations with it */
    OOP_DisposeObject(a);
    a = NULL;
    CHECK(countRelations(c, vHidd_Relation_Monitors, &seen) == 1 && seen.object == NULL,
          "relation by path stays when its target is disposed");
    CHECK(countRelations(c, vHidd_Relation_Any, NULL) == 1, "relations to a disposed object are gone");
    CHECK(countRelations(b, vHidd_Relation_Any, NULL) == 1, "relations of a disposed declarer are gone");

    /*
     * What a driver publishing a sensor does, and what a monitoring tool does
     * to find it. Only if the telemetry base class is on this system.
     */
    {
        OOP_Object *sensor = OOP_NewObject(NULL, "hidd.telemetry", NULL);

        if (sensor)
        {
            CHECK(addRelation(b, vHidd_Relation_Exposes, sensor, NULL, "sensor"), "driver publishes a telemetry object");
            CHECK(HIDD_FindRelated(b, vHidd_Relation_Exposes, "hidd.telemetry") == sensor, "telemetry object is found by class");
            CHECK(HIDD_FindRelated(b, vHidd_Relation_Exposes, CLID_HW) == NULL, "telemetry object is not mistaken for another class");
            CHECK(HIDD_FindRelated(sensor, vHidd_Relation_ExposedBy, NULL) == b, "telemetry object finds its publisher");
            OOP_DisposeObject(sensor);
            CHECK(countRelations(b, vHidd_Relation_Exposes, NULL) == 0, "disposed telemetry object is no longer published");
        }
        else
            printf("(no telemetry class here, its checks are skipped)\n");
    }

    /* The two relations which can be declared on creation */
    {
        struct TagItem tags[] =
        {
            {aHidd_Name     , (IPTR)"relation.e"},
            {aHidd_Parent   , (IPTR)b           },
            {aHW_Device     , (IPTR)c           },
            {TAG_DONE       , 0                 }
        };
        OOP_Object *e = OOP_NewObject(NULL, CLID_Hidd, tags);

        CHECK(e != NULL, "object with parent and device created");
        if (e)
        {
            val = 0;
            OOP_GetAttr(e, aHidd_Parent, &val);
            CHECK((OOP_Object *)val == b, "aHidd_Parent declares and reports the parent");
            CHECK(HIDD_FindRelated(b, vHidd_Relation_Child, NULL) == e, "parent finds its child");
            val = 0;
            OOP_GetAttr(e, aHW_Device, &val);
            CHECK((OOP_Object *)val == c, "aHW_Device declares and reports the device");
            CHECK(HIDD_FindRelated(c, vHidd_Relation_Driver, NULL) == e, "device finds the driver created for it");

            OOP_DisposeObject(e);
            CHECK(countRelations(b, vHidd_Relation_Any, NULL) == 1 && countRelations(c, vHidd_Relation_Any, NULL) == 1,
                  "relations declared on creation go with the object");
        }
    }
    val = 1;
    OOP_GetAttr(b, aHidd_Parent, &val);
    CHECK(val == 0, "object without parent reports none");

    /*
     * The PCI subsystem numbers its hardware drivers, and everything on the
     * bus of one carries that number in its device path. Only where there is
     * PCI.
     */
    {
        OOP_Object *host = HIDD_FindDevicePath(b, "pci/0000");

        if (host)
        {
            CHECK(isA(host, "hidd.pci.driver"), "first PCI segment is a PCI hardware driver");
            if (countRelations(host, vHidd_Relation_Child, &seen))
            {
                CHECK(seen.hasPath && !strncmp(seen.path, "pci/0000:", 9), "PCI device carries the segment of its bus");
                CHECK(seen.object && HIDD_FindRelated(seen.object, vHidd_Relation_Parent, NULL) == host,
                      "PCI device finds the driver of its bus");
            }
        }
        else
            printf("(no PCI here, its checks are skipped)\n");
    }

out:
    if (a) OOP_DisposeObject(a);
    if (b) OOP_DisposeObject(b);
    if (c) OOP_DisposeObject(c);
    if (plain) OOP_DisposeObject(plain);

    OOP_ReleaseAttrBases(abd);

    printf("relation: %u checks, %u failures\n", (unsigned)checks, (unsigned)failures);
    printf(failures ? "RESULT: FAIL\n" : "RESULT: PASS\n");

    return failures ? RETURN_FAIL : RETURN_OK;
}
