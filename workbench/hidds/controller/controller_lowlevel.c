/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: lowlevel.library joyport bridge. Patches ReadJoyPort() and
          SetJoyPortAttrsA() with SetFunction() from this hidd's own
          initialisation, chaining to the previous vectors, and serves the
          four joyports from the subsystem's slot table.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/libraries.h>
#include <libraries/lowlevel.h>
#include <libraries/lowlevel_ext.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <hidd/controller.h>

#include "controller_intern.h"

/*
 * The patched entries run in the caller's context with lowlevel.library's
 * base in A6, so the hidd base is kept in statics filled at install time.
 * ReadJoyPort() is commonly called from interrupts (VBlank handlers of
 * games), so the read path only uses the seqlock and Disable().
 */
static struct controllerbase *ll_Base;
static struct ExecBase       *ll_SysBase;

#define SysBase     ll_SysBase
#define UtilityBase (ll_Base->csd.cs_UtilityBase)

#define LL_DIRECTION_THRESHOLD  16384   /* stick deflection that counts as a direction */

/* interrupt safe device lookup (Disable only) */
static struct ControllerDevice *ll_FindDevice(struct ControllerHWData *hw, UWORD id)
{
    struct ControllerDevice *dev, *found = NULL;

    Disable();
    ForeachNode(&hw->devices, dev)
    {
        if (dev->id == id)
        {
            found = dev;
            break;
        }
    }
    Enable();
    return found;
}

AROS_LD1(ULONG, ControllerReadJoyPort,
         AROS_LDA(ULONG, port, D0),
         struct Library *, LowLevelBase, 5, Controller);

AROS_LD2(ULONG, ControllerSetJoyPortAttrsA,
         AROS_LDA(ULONG, port, D0),
         AROS_LDA(struct TagItem *, tags, A1),
         struct Library *, LowLevelBase, 22, Controller);

/*****************************************************************************************
    encoding a reading into the JP_* bit layout
*****************************************************************************************/

static ULONG ll_Directions(const struct pHidd_Controller_Reading *rd, BOOL mapped)
{
    ULONG dir = 0;

    if (mapped)
    {
        ULONG gp = rd->gp_buttons;

        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_DpadUp))    dir |= JPF_JOY_UP;
        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_DpadDown))  dir |= JPF_JOY_DOWN;
        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_DpadLeft))  dir |= JPF_JOY_LEFT;
        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_DpadRight)) dir |= JPF_JOY_RIGHT;
        if (rd->gp_axes[vHidd_Controller_GPA_LeftX] <= -LL_DIRECTION_THRESHOLD) dir |= JPF_JOY_LEFT;
        if (rd->gp_axes[vHidd_Controller_GPA_LeftX] >=  LL_DIRECTION_THRESHOLD) dir |= JPF_JOY_RIGHT;
        if (rd->gp_axes[vHidd_Controller_GPA_LeftY] <= -LL_DIRECTION_THRESHOLD) dir |= JPF_JOY_UP;
        if (rd->gp_axes[vHidd_Controller_GPA_LeftY] >=  LL_DIRECTION_THRESHOLD) dir |= JPF_JOY_DOWN;
    }
    else
    {
        /* unmapped device: hat 0 and axes 0/1 as hid.class did */
        if (rd->hat_count)
        {
            UBYTE hat = rd->hats[0];
            if (hat & vHidd_Controller_Hat_Up)    dir |= JPF_JOY_UP;
            if (hat & vHidd_Controller_Hat_Down)  dir |= JPF_JOY_DOWN;
            if (hat & vHidd_Controller_Hat_Left)  dir |= JPF_JOY_LEFT;
            if (hat & vHidd_Controller_Hat_Right) dir |= JPF_JOY_RIGHT;
        }
        if (rd->axis_count > 0)
        {
            if (rd->axes[0] <= -LL_DIRECTION_THRESHOLD) dir |= JPF_JOY_LEFT;
            if (rd->axes[0] >=  LL_DIRECTION_THRESHOLD) dir |= JPF_JOY_RIGHT;
        }
        if (rd->axis_count > 1)
        {
            if (rd->axes[1] <= -LL_DIRECTION_THRESHOLD) dir |= JPF_JOY_UP;
            if (rd->axes[1] >=  LL_DIRECTION_THRESHOLD) dir |= JPF_JOY_DOWN;
        }
    }
    return dir;
}

/* CD32 pad button order: RED BLUE GREEN YELLOW FORWARD REVERSE PLAY */
static ULONG ll_Buttons(const struct pHidd_Controller_Reading *rd, BOOL mapped)
{
    ULONG btn = 0;

    if (mapped)
    {
        ULONG gp = rd->gp_buttons;

        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_South))         btn |= JPF_BUTTON_RED;
        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_East))          btn |= JPF_BUTTON_BLUE;
        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_West))          btn |= JPF_BUTTON_GREEN;
        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_North))         btn |= JPF_BUTTON_YELLOW;
        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_RightShoulder)) btn |= JPF_BUTTON_FORWARD;
        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_LeftShoulder))  btn |= JPF_BUTTON_REVERSE;
        if (gp & vHidd_Controller_GPF(vHidd_Controller_GP_Start))         btn |= JPF_BUTTON_PLAY;
    }
    else
    {
        static const ULONG order[7] = { JPF_BUTTON_RED, JPF_BUTTON_BLUE, JPF_BUTTON_GREEN, JPF_BUTTON_YELLOW,
                                        JPF_BUTTON_FORWARD, JPF_BUTTON_REVERSE, JPF_BUTTON_PLAY };
        UWORD i;

        for (i = 0; i < 7 && i < rd->button_count; i++)
            if (rd->buttons[0] & (1UL << i))
                btn |= order[i];
    }
    return btn;
}

/* analogue X/Y as 0..255, 0 = left/up (the lowlevel_ext.h convention) */
static ULONG ll_Analogue(const struct pHidd_Controller_Reading *rd, BOOL mapped)
{
    WORD x = 0, y = 0;

    if (mapped)
    {
        x = rd->gp_axes[vHidd_Controller_GPA_LeftX];
        y = rd->gp_axes[vHidd_Controller_GPA_LeftY];
    }
    else
    {
        if (rd->axis_count > 0) x = rd->axes[0];
        if (rd->axis_count > 1) y = rd->axes[1];
    }
    return (((ULONG)(x + 32768) >> 8) & JP_XAXIS_MASK) | ((((ULONG)(y + 32768) >> 8) << 8) & JP_YAXIS_MASK);
}

/*
 * Encode the state of a slot. Returns FALSE when the slot has no device, in
 * which case the chained result is to be used.
 */
static BOOL ll_EncodePort(struct ControllerHWData *hw, ULONG slot, BOOL analogue, ULONG *result)
{
    struct ControllerDevice *dev;
    struct pHidd_Controller_Reading rd;
    BOOL mapped;
    ULONG dir, btn, type;

    if (slot >= HIDD_CONTROLLER_LEGACY_PORTS || !hw->slot[slot])
        return FALSE;
    if (hw->slot_flags[slot] & CTRL_SF_DISABLED)
    {
        *result = JP_TYPE_NOTAVAIL;
        return TRUE;
    }
    dev = ll_FindDevice(hw, hw->slot[slot]);
    if (!dev)
        return FALSE;

    rd.size = sizeof(rd);
    ctrl_CopyReading(ll_Base->csd.controllerClass, dev, &rd);
    mapped = (rd.flags & vHidd_Controller_RF_Mapped) != 0;
    dir = ll_Directions(&rd, mapped);
    btn = ll_Buttons(&rd, mapped);

    type = hw->slot_type[slot];
    if (type == SJA_TYPE_AUTOSENSE)
        type = SJA_TYPE_GAMECTLR;
    if (analogue || (hw->slot_flags[slot] & CTRL_SF_ANALOGUE))
        type = SJA_TYPE_ANALOGUE;

    switch (type)
    {
    case SJA_TYPE_JOYSTK:
        *result = JP_TYPE_JOYSTK | dir | (btn & (JPF_BUTTON_RED | JPF_BUTTON_BLUE));
        break;
    case SJA_TYPE_ANALOGUE:
    {
        /* directions only from the d-pad/hat, the stick goes into the axes */
        struct pHidd_Controller_Reading tmp = rd;
        tmp.gp_axes[vHidd_Controller_GPA_LeftX] = tmp.gp_axes[vHidd_Controller_GPA_LeftY] = 0;
        if (tmp.axis_count > 0) tmp.axes[0] = 0;
        if (tmp.axis_count > 1) tmp.axes[1] = 0;
        *result = JP_TYPE_ANALOGUE | ll_Directions(&tmp, mapped) | btn | ll_Analogue(&rd, mapped);
        break;
    }
    case SJA_TYPE_MOUSE:
        return FALSE;           /* not emulated: whatever the chain says */
    default:
        *result = JP_TYPE_GAMECTLR | dir | btn;
        break;
    }
    return TRUE;
}

/* Used by the subsystem method and the patch alike */
ULONG ctrl_LowLevelRead(struct ControllerHWData *hw, ULONG port, ULONG chained)
{
    ULONG result = chained;
    BOOL analogue = FALSE;

    if (port >= JP_ANALOGUE_PORT_MAGIC && port < (JP_ANALOGUE_PORT_MAGIC | HIDD_CONTROLLER_LEGACY_PORTS))
    {
        port &= 3;
        analogue = TRUE;
    }
    if (port < HIDD_CONTROLLER_LEGACY_PORTS)
        ll_EncodePort(hw, port, analogue, &result);
    return result;
}

BOOL ctrl_LowLevelSetAttrs(OOP_Class *cl, struct ControllerHWData *hw, ULONG port, struct TagItem *tags, BOOL chained)
{
    struct TagItem *tag, *tstate = tags;
    struct ControllerDevice *dev;
    struct pHidd_Controller_Rumble rumble;
    BOOL have_rumble = FALSE, result = chained;

    if (port >= HIDD_CONTROLLER_LEGACY_PORTS || !hw->slot[port])
        return result;
    dev = ctrl_FindDevice(cl, hw, hw->slot[port]);
    if (!dev)
        return result;
    result = TRUE;

    rumble = dev->rumble;
    rumble.duration_ms = 0;

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case SJA_Type:
            hw->slot_flags[port] &= ~CTRL_SF_ANALOGUE;
            if (tag->ti_Data == SJA_TYPE_ANALOGUE)
                hw->slot_flags[port] |= CTRL_SF_ANALOGUE;
            hw->slot_type[port] = (tag->ti_Data <= SJA_TYPE_JOYSTK) ? (UBYTE)tag->ti_Data : SJA_TYPE_AUTOSENSE;
            break;
        case SJA_Reinitialize:
            hw->slot_flags[port] &= ~CTRL_SF_ANALOGUE;
            hw->slot_type[port] = SJA_TYPE_AUTOSENSE;
            break;
        case SJA_RumbleSetSlowMotor:
            rumble.low = (UWORD)((tag->ti_Data & 0xFF) * 257);
            have_rumble = TRUE;
            break;
        case SJA_RumbleSetFastMotor:
            rumble.high = (UWORD)((tag->ti_Data & 0xFF) * 257);
            have_rumble = TRUE;
            break;
        case SJA_RumbleOff:
            if (tag->ti_Data)
            {
                rumble.low = rumble.high = rumble.left_trigger = rumble.right_trigger = 0;
                have_rumble = TRUE;
            }
            break;
        }
    }

    if (have_rumble)
        HIDD_Controller_SetRumble(dev->obj, &rumble);
    return result;
}

/*****************************************************************************************
    the patched lowlevel.library entries
*****************************************************************************************/

AROS_LH1(ULONG, ControllerReadJoyPort,
         AROS_LHA(ULONG, port, D0),
         struct Library *, LowLevelBase, 5, Controller)
{
    AROS_LIBFUNC_INIT

    struct controllerbase *base = ll_Base;
    ULONG result = JP_TYPE_NOTAVAIL;

    if (!base)
        return result;

    if (base->csd.cs_LLOldReadJoyPort)
        result = AROS_CALL1(ULONG, base->csd.cs_LLOldReadJoyPort,
                            AROS_LCA(ULONG, port, D0),
                            struct Library *, base->csd.cs_LowLevelBase);

    if (base->csd.hwObject)
    {
        struct ControllerHWData *hw = OOP_INST_DATA(base->csd.hwClass, base->csd.hwObject);
        result = ctrl_LowLevelRead(hw, port, result);
    }
    return result;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, ControllerSetJoyPortAttrsA,
         AROS_LHA(ULONG, port, D0),
         AROS_LHA(struct TagItem *, tags, A1),
         struct Library *, LowLevelBase, 22, Controller)
{
    AROS_LIBFUNC_INIT

    struct controllerbase *base = ll_Base;
    ULONG result = FALSE;

    if (!base)
        return result;

    if (base->csd.cs_LLOldSetJoyPortAttrsA)
        result = AROS_CALL2(ULONG, base->csd.cs_LLOldSetJoyPortAttrsA,
                            AROS_LCA(ULONG, port, D0),
                            AROS_LCA(struct TagItem *, tags, A1),
                            struct Library *, base->csd.cs_LowLevelBase);

    if (base->csd.hwObject && tags)
    {
        struct ControllerHWData *hw = OOP_INST_DATA(base->csd.hwClass, base->csd.hwObject);
        result = ctrl_LowLevelSetAttrs(base->csd.hwClass, hw, port, tags, result ? TRUE : FALSE);
    }
    return result;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************************
    install / remove (task context)
*****************************************************************************************/

BOOL ctrl_LowLevelInstall(struct controllerbase *base)
{
    /* SysBase below expands to the static; fill it before anything uses it */
    ll_SysBase = (struct ExecBase *)base->csd.cs_SysBase;

    if (base->csd.cs_LowLevelBase)
        return TRUE;

    base->csd.cs_LowLevelBase = OpenLibrary("lowlevel.library", 40);
    if (!base->csd.cs_LowLevelBase)
    {
        D(bug("[Controller] lowlevel.library not available yet\n"));
        return FALSE;
    }

    ll_Base = base;

    Disable();
    base->csd.cs_LLOldReadJoyPort = SetFunction(base->csd.cs_LowLevelBase, -5 * LIB_VECTSIZE,
                                                AROS_SLIB_ENTRY(ControllerReadJoyPort, Controller, 5));
    base->csd.cs_LLOldSetJoyPortAttrsA = SetFunction(base->csd.cs_LowLevelBase, -22 * LIB_VECTSIZE,
                                                     AROS_SLIB_ENTRY(ControllerSetJoyPortAttrsA, Controller, 22));
    Enable();

    D(bug("[Controller] lowlevel.library joyport bridge installed\n"));
    return TRUE;
}

/* Returns FALSE (and leaves the patch in place) when someone patched over us */
BOOL ctrl_LowLevelRemove(struct controllerbase *base)
{
    APTR vec;
    BOOL ok = TRUE;

    if (!base->csd.cs_LowLevelBase)
        return TRUE;

    Disable();
    vec = SetFunction(base->csd.cs_LowLevelBase, -5 * LIB_VECTSIZE, base->csd.cs_LLOldReadJoyPort);
    if (vec != AROS_SLIB_ENTRY(ControllerReadJoyPort, Controller, 5))
    {
        SetFunction(base->csd.cs_LowLevelBase, -5 * LIB_VECTSIZE, vec);
        ok = FALSE;
    }
    else
    {
        vec = SetFunction(base->csd.cs_LowLevelBase, -22 * LIB_VECTSIZE, base->csd.cs_LLOldSetJoyPortAttrsA);
        if (vec != AROS_SLIB_ENTRY(ControllerSetJoyPortAttrsA, Controller, 22))
        {
            SetFunction(base->csd.cs_LowLevelBase, -22 * LIB_VECTSIZE, vec);
            SetFunction(base->csd.cs_LowLevelBase, -5 * LIB_VECTSIZE, AROS_SLIB_ENTRY(ControllerReadJoyPort, Controller, 5));
            ok = FALSE;
        }
    }
    Enable();

    if (ok)
    {
        CloseLibrary(base->csd.cs_LowLevelBase);
        base->csd.cs_LowLevelBase = NULL;
        ll_Base = NULL;
        ll_SysBase = NULL;
    }
    return ok;
}
