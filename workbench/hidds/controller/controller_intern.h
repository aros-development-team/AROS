#ifndef HIDD_CONTROLLER_INTERN_H
#define HIDD_CONTROLLER_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Private structures of controller.hidd
*/

#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/ports.h>
#include <devices/timer.h>
#include <dos/bptr.h>
#include <hidd/input.h>
#include <hidd/controller.h>

/* Roles of a CLID_Hidd_Controller instance */
#define CTRL_ROLE_NONE      0
#define CTRL_ROLE_DEVICE    1   /* created through HW_AddDriver: a controller device */
#define CTRL_ROLE_CONSUMER  2   /* created by a client: receives events */

/* Consumer delivery modes */
#define CTRL_MODE_NONE      0
#define CTRL_MODE_CALLBACK  1
#define CTRL_MODE_SIGNAL    2
#define CTRL_MODE_PORT      3

#define CTRL_DEFAULT_QUEUE_DEPTH  64
#define CTRL_MAX_QUEUE_DEPTH      1024
#define CTRL_FANOUT_MAX           32    /* consumers collected per delivery pass */

/* Consumer flags */
#define CTRL_CF_COALESCE    0x0001
#define CTRL_CF_OVERFLOW    0x0002  /* overflow event pending */
#define CTRL_CF_ATTACHED    0x0004  /* linked into a list */

/* Device flags */
#define CTRL_DF_MAPPED      0x0001
#define CTRL_DF_TRUNCATED   0x0002
#define CTRL_DF_REGISTERED  0x0004  /* SetUpDriver completed, DeviceAdded sent */
#define CTRL_DF_GUIDGIVEN   0x0008  /* driver supplied the GUID */

#define CTRL_NUM_STD  (vHidd_Controller_GP_ButtonCount + vHidd_Controller_GPA_AxisCount + vHidd_Controller_ARCH_AxisCount)

struct ControllerHWData;

/* A mapping database record */
struct ControllerMapping
{
    struct MinNode  node;
    UBYTE           guid[16];
    UWORD           crc;            /* crc: field, 0 = not given */
    UBYTE           source;         /* vHidd_Controller_MapSrc_* */
    UBYTE           flags;
    UWORD           nbindings;
    STRPTR          name;           /* allocated */
    struct Hidd_Controller_Binding *bindings;   /* allocated, Std_None terminated */
};
#define CTRL_MF_STATIC   0x02       /* name/bindings are not allocated */

struct ControllerConsumer
{
    struct MinNode          node;
    OOP_Object              *obj;
    struct ControllerDevice *dev;       /* bound device, NULL = subsystem wide */
    UWORD                   device_id;  /* filter, 0 = all */
    UWORD                   mode;
    ULONG                   eventmask;
    UWORD                   flags;
    UWORD                   depth;
    InputIrqCallBack_t      cb;
    APTR                    cbdata;
    struct Task             *task;
    ULONG                   sigmask;
    struct MsgPort          *port;
    struct pHidd_Controller_Event *ring;    /* signal mode: depth entries */
    UWORD                   head;
    UWORD                   tail;
    UWORD                   count;
    UWORD                   pad;
    struct Hidd_Controller_EventMsg *msgs;  /* port mode: depth messages */
    ULONG                   dropped;
};

struct ControllerDevice
{
    struct MinNode          node;       /* ControllerHWData.devices */
    OOP_Object              *obj;
    struct ControllerHWData *hw;
    UWORD                   id;
    WORD                    legacy_port;
    WORD                    player_index;
    UBYTE                   connected;
    UBYTE                   role_pad;
    UWORD                   flags;      /* CTRL_DF_* */

    /* identity */
    CONST_STRPTR            name;       /* aHidd_Name */
    CONST_STRPTR            hwname;     /* aHidd_HardwareName */
    CONST_STRPTR            manufacturer;
    CONST_STRPTR            serial;
    CONST_STRPTR            path;
    UWORD                   vendor, product, version;
    UBYTE                   bus, type, style, family, connection;
    UBYTE                   guid[16];

    /* controls */
    struct Hidd_Controller_ControlDesc buttons[HIDD_CONTROLLER_MAX_BUTTONS];
    struct Hidd_Controller_ControlDesc axes[HIDD_CONTROLLER_MAX_AXES];
    struct Hidd_Controller_ControlDesc hats[HIDD_CONTROLLER_MAX_HATS];
    struct Hidd_Controller_ControlDesc touchpads[HIDD_CONTROLLER_MAX_TOUCHPADS];
    struct Hidd_Controller_ControlDesc sensors[HIDD_CONTROLLER_MAX_SENSORS];
    UWORD                   nbuttons, naxes, nhats, ntouchpads, nsensors;
    struct Hidd_Controller_OutputDesc outputs[HIDD_CONTROLLER_MAX_OUTPUTS];
    UWORD                   noutputs;
    ULONG                   caps;

    /* per axis normalisation state */
    LONG                    axis_raw[HIDD_CONTROLLER_MAX_AXES];     /* last accepted raw value */
    LONG                    axis_flat[HIDD_CONTROLLER_MAX_AXES];    /* raw units */
    LONG                    axis_fuzz[HIDD_CONTROLLER_MAX_AXES];    /* raw units */

    /* standard mapping */
    struct Hidd_Controller_Binding bind[CTRL_NUM_STD];              /* by std slot */
    UWORD                   raw_btn_std[HIDD_CONTROLLER_MAX_BUTTONS];
    UWORD                   raw_axis_std[HIDD_CONTROLLER_MAX_AXES];
    UWORD                   raw_hat_std[HIDD_CONTROLLER_MAX_HATS][4];
    UBYTE                   mapsource;
    UBYTE                   map_pad[3];
    const struct Hidd_Controller_Binding *drv_bindings;  /* driver supplied table (aHidd_Controller_BindingTable) */

    /* reading cache (seqlock: seq odd while writing) */
    volatile ULONG          seq;
    struct pHidd_Controller_Reading rd;

    /* extended state */
    struct pHidd_Controller_Finger fingers[HIDD_CONTROLLER_MAX_TOUCHPADS][HIDD_CONTROLLER_MAX_FINGERS];
    struct pHidd_Controller_SensorSample sensor_ring[HIDD_CONTROLLER_MAX_SENSORS][HIDD_CONTROLLER_SENSOR_RING];
    UBYTE                   sensor_head[HIDD_CONTROLLER_MAX_SENSORS];
    UBYTE                   sensor_count[HIDD_CONTROLLER_MAX_SENSORS];
    struct pHidd_Controller_PowerInfo power;
    BOOL                    sensors_enabled;

    /* last output requests */
    struct pHidd_Controller_Rumble rumble;
    UBYTE                   led[3];
    UBYTE                   led_pad;

    struct MinList          consumers;  /* device bound consumers */
};

struct ControllerInstData
{
    UBYTE   role;
    UBYTE   pad[3];
    union {
        struct ControllerDevice   dev;
        struct ControllerConsumer con;
    } u;
};

struct ControllerHWData
{
    struct MinList          devices;
    struct MinList          consumers;      /* subsystem wide consumers */
    struct MinList          mappings;
    struct SignalSemaphore  maplock;
    UWORD                   next_id;
    UWORD                   devcount;
    UBYTE                   slot_policy;
    UBYTE                   slot_pad[3];
    UWORD                   slot[HIDD_CONTROLLER_LEGACY_PORTS];   /* device ids, 0 = empty */
    UBYTE                   slot_flags[HIDD_CONTROLLER_LEGACY_PORTS];
};
#define CTRL_SF_PINNED   0x01

struct controller_staticdata
{
    OOP_AttrBase            hiddAB;
    OOP_AttrBase            hwAttrBase;
    OOP_AttrBase            hiddInputAB;
    OOP_AttrBase            hwInputAB;
    OOP_AttrBase            hiddControllerAB;
    OOP_AttrBase            hwControllerAB;
    OOP_MethodID            hwMethodBase;
    OOP_MethodID            hwInputMethodBase;
    OOP_MethodID            hiddControllerMethodBase;
    OOP_MethodID            hwControllerMethodBase;
    OOP_Class               *controllerClass;
    OOP_Class               *hwClass;
    OOP_Object              *hwObject;

    struct Library          *cs_SysBase;
    struct Library          *cs_OOPBase;
    struct Library          *cs_UtilityBase;

    struct timerequest      cs_TimerReq;
    struct Device           *cs_TimerBase;
    ULONG                   cs_EClockFreq;
};

struct controllerbase
{
    struct Library               LibNode;
    struct controller_staticdata csd;
};

#define CSD(cl) (&((struct controllerbase *)cl->UserData)->csd)

#undef HiddAttrBase
#undef HWAttrBase
#undef HiddInputAB
#undef HWInputAB
#undef HiddControllerAB
#undef HWControllerAB
#undef HWBase
#undef HWInputBase
#undef HiddControllerBase
#undef HWControllerBase
#define HiddAttrBase            (CSD(cl)->hiddAB)
#define HWAttrBase              (CSD(cl)->hwAttrBase)
#define HiddInputAB             (CSD(cl)->hiddInputAB)
#define HWInputAB               (CSD(cl)->hwInputAB)
#define HiddControllerAB        (CSD(cl)->hiddControllerAB)
#define HWControllerAB          (CSD(cl)->hwControllerAB)
#define HWBase                  (CSD(cl)->hwMethodBase)
#define HWInputBase             (CSD(cl)->hwInputMethodBase)
#define HiddControllerBase      (CSD(cl)->hiddControllerMethodBase)
#define HWControllerBase        (CSD(cl)->hwControllerMethodBase)

/* Access to the device/consumer data of any CLID_Hidd_Controller derived object */
static inline struct ControllerInstData *ctrl_InstData(OOP_Class *cl, OOP_Object *o)
{
    return (struct ControllerInstData *)OOP_INST_DATA(CSD(cl)->controllerClass, o);
}
static inline struct ControllerDevice *ctrl_Device(OOP_Class *cl, OOP_Object *o)
{
    struct ControllerInstData *d = ctrl_InstData(cl, o);
    return (d->role == CTRL_ROLE_DEVICE) ? &d->u.dev : NULL;
}
static inline struct ControllerConsumer *ctrl_Consumer(OOP_Class *cl, OOP_Object *o)
{
    struct ControllerInstData *d = ctrl_InstData(cl, o);
    return (d->role == CTRL_ROLE_CONSUMER) ? &d->u.con : NULL;
}

/* controller_controls.c */
BOOL  ctrl_ImportControls(OOP_Class *cl, struct ControllerDevice *dev,
                          const struct Hidd_Controller_ControlDesc *table,
                          const struct Hidd_Controller_OutputDesc *outputs);
WORD  ctrl_NormaliseAxis(const struct ControllerDevice *dev, UWORD idx, LONG raw);
UBYTE ctrl_DecodeHat(const struct Hidd_Controller_ControlDesc *desc, LONG raw);
BOOL  ctrl_AxisChanged(struct ControllerDevice *dev, UWORD idx, LONG raw);

/* controller_reading.c */
UQUAD ctrl_Now(OOP_Class *cl);
void  ctrl_ReadingInit(struct ControllerDevice *dev);
ULONG ctrl_CopyReading(OOP_Class *cl, const struct ControllerDevice *dev, struct pHidd_Controller_Reading *dst);
void  ctrl_PushReport(OOP_Class *cl, struct ControllerDevice *dev, const struct pHidd_Controller_RawReport *r);
void  ctrl_PushValue(OOP_Class *cl, struct ControllerDevice *dev, UBYTE kind, UBYTE index, LONG value, UQUAD ts);

/* controller_notify.c */
void  ctrl_ConsumerAttach(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerConsumer *con);
void  ctrl_ConsumerDetach(OOP_Class *cl, struct ControllerConsumer *con);
void  ctrl_DeliverEvent(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerDevice *dev,
                        const struct pHidd_Controller_Event *ev);
BOOL  ctrl_ConsumerGetEvent(OOP_Class *cl, struct ControllerConsumer *con, struct pHidd_Controller_Event *dst);
void  ctrl_DetachDeviceConsumers(OOP_Class *cl, struct ControllerDevice *dev);
void  ctrl_SendDeviceEvent(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerDevice *dev, UWORD type, LONG value);

/* controller_mapping.c */
void  ctrl_ComputeGUID(struct ControllerDevice *dev);
void  ctrl_MappingInitDB(OOP_Class *cl, struct ControllerHWData *hw);
void  ctrl_MappingFreeDB(OOP_Class *cl, struct ControllerHWData *hw);
BOOL  ctrl_MappingAdd(OOP_Class *cl, struct ControllerHWData *hw, const struct Hidd_Controller_MappingDesc *desc, UBYTE source);
BOOL  ctrl_MappingRemove(OOP_Class *cl, struct ControllerHWData *hw, const UBYTE *guid, UBYTE source);
void  ctrl_ApplyMapping(OOP_Class *cl, struct ControllerDevice *dev);
void  ctrl_FreeMapping(OOP_Class *cl, struct ControllerDevice *dev);
void  ctrl_ComputeStandardView(struct ControllerDevice *dev);
UWORD ctrl_LabelFor(const struct ControllerDevice *dev, UWORD stdid);
ULONG ctrl_CopyBindings(struct ControllerDevice *dev, struct Hidd_Controller_Binding *buf, ULONG max);
BOOL  ctrl_GUIDFromString(const char *s, UBYTE *guid);
void  ctrl_GUIDToString(const UBYTE *guid, char *s);

/* controllersubsystem.c */
struct ControllerDevice *ctrl_FindDevice(OOP_Class *cl, struct ControllerHWData *hw, UWORD id);
void  ctrl_SlotAssignAuto(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerDevice *dev);
void  ctrl_SlotRelease(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerDevice *dev);

#endif /* HIDD_CONTROLLER_INTERN_H */
