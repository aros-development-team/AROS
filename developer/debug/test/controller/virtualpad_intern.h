#ifndef VIRTUALPAD_INTERN_H
#define VIRTUALPAD_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: private definitions of virtualpad.hidd
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <hidd/controller.h>
#include "virtualpad.h"

#define VPAD_LOG_SIZE 32

struct VirtualPad_Profile
{
    CONST_STRPTR name;
    CONST_STRPTR description;
    CONST_STRPTR hwname;
    CONST_STRPTR manufacturer;
    CONST_STRPTR serial;
    UWORD        vendor, product, version;
    UBYTE        bus, family, type, connection;
    const struct Hidd_Controller_ControlDesc *controls;
    const struct Hidd_Controller_OutputDesc  *outputs;
    const struct Hidd_Controller_Binding     *bindings;   /* NULL = let the subsystem decide */
};

struct VirtualPadData
{
    const struct VirtualPad_Profile *profile;
    struct VirtualPad_Output log[VPAD_LOG_SIZE];
    UWORD  log_head;
    UWORD  log_count;
    LONG   next_effect_id;
    struct pHidd_Controller_RawReport state;    /* last fed raw state */
};

struct virtualpad_staticdata
{
    OOP_AttrBase    hiddAB;
    OOP_AttrBase    hiddControllerAB;
    OOP_AttrBase    hiddVirtualPadAB;
    OOP_MethodID    hiddControllerMB;
    OOP_MethodID    hiddVirtualPadMB;
    OOP_Class       *vpadClass;
    struct Library  *cs_SysBase;
    struct Library  *cs_OOPBase;
    struct Library  *cs_UtilityBase;
};

struct virtualpadbase
{
    struct Library                LibNode;
    struct virtualpad_staticdata  vsd;
};

#define VSD(cl) (&((struct virtualpadbase *)cl->UserData)->vsd)

#undef HiddAttrBase
#undef HiddControllerAB
#undef HiddVirtualPadAB
#undef HiddControllerBase
#undef HiddVirtualPadBase
#define HiddAttrBase            (VSD(cl)->hiddAB)
#define HiddControllerAB        (VSD(cl)->hiddControllerAB)
#define HiddVirtualPadAB        (VSD(cl)->hiddVirtualPadAB)
#define HiddControllerBase      (VSD(cl)->hiddControllerMB)
#define HiddVirtualPadBase      (VSD(cl)->hiddVirtualPadMB)

const struct VirtualPad_Profile *vpad_FindProfile(CONST_STRPTR name);
extern const struct VirtualPad_Profile vpad_profiles[];

#endif
