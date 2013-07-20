/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <aros/debug.h>

#include <aros/libcall.h>
#include <exec/types.h>
#include <libraries/lowlevel.h>

#include <devices/timer.h>

#include <proto/potgo.h>

#include <hardware/cia.h>
#include <hardware/custom.h>

#include "lowlevel_intern.h"

/* See rom/lowlevel/readjoyport.c for documentation */

#define POTGO_GAMEPAD_PORT0 \
        (1 << 11) | (1 << 10) | (1 <<  9) | (1 <<  8)
#define POTGO_GAMEPAD_PORT1 \
        (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12)

/*
 * ciaapra - (1 << 7) is /FIR1 (port 1, pin 6)
 * ciaapra - (1 << 6) is /FIR0 (port 0, pin 6)
 * ciaaddra - 0x03
 * JOYnDAT:
 *     pin 1 (up) = ((JOYnDAT >> 0) ^ (JOYnDAT >> 1)) & 1
 *     pin 2 (dn) = ((JOYnDAT >> 8) ^ (JOYnDAT >> 9)) & 1
 *     pin 3 (lt) = (JOYnDAT >> 9) & 1
 *     pin 4 (rt) = (JOYnDAT >> 1) & 1
 * POTGO/POTINP:
 *     port 0: pin 9 (1 << 10), pin 5 (1 << 8)
 *     port 1: pin 9 (1 << 14), pin 5 (1 << 12)
 */
static inline ULONG llPollGameCtrl(int port)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    volatile struct CIA *cia = (struct CIA *)0xbfe001;
    UWORD pot;
    ULONG bits = 0;
    UWORD joydat;
    int i;
    UBYTE cmask = (port == 0) ? (1 << 6) : (1 << 7);

    /* Set Pin 5 as output, shift mode */
    pot = custom->potinp;
    pot &= ~((port == 0) ? (3 << 8) : (3 << 12));
    custom->potgo = pot | (port == 0) ? (2 << 8) : (2 << 12);
    cia->ciapra  &= ~cmask;
    cia->ciaddra |= cmask;

    /* Shift in the button values */
    D(bug("Sin: \n"));
    for (i = 0; i < 9; i++) {
        cia->ciapra |= cmask;
        cia->ciapra &= ~cmask;
        bits <<= 1;
        bits |= ((custom->potinp >> ((port == 0) ? 10 : 14)) & 1) ? 0 : 1;
        D(bug(" %d", bits & 1));
    }
    D(bug("\n"));

    cia->ciapra &= ~cmask;
    custom->potgo = pot;

    if ((bits & 3) != 2) {
        /* Stuck bits? Probably not a game controller */
        D(bug("%s: Stuck bits? (0x%04x)\n", __func__, bits));
        /* Revert to autosense */
        return 0;
    }
    bits &= ~3;
    bits <<= 15;

    /* Get the joypad bits */
    joydat = (port == 0) ? custom->joy0dat : custom->joy1dat;
   
    if ((joydat >> 1) & 1) {
        bits |= JPF_JOY_RIGHT;
    }
    if ((joydat >> 9) & 1) {
        bits |= JPF_JOY_LEFT;
    }
    if (((joydat >> 0) ^ (joydat >> 1)) & 1) {
        bits |= JPF_JOY_DOWN;
    }
    if (((joydat >> 8) ^ (joydat >> 9)) & 1) {
        bits |= JPF_JOY_UP;
    }

    return JP_TYPE_GAMECTLR | bits;
}

/*
 * ciaapra - (1 << 7) is /FIR1 (port 2, pin 6)
 * ciaapra - (1 << 6) is /FIR0 (port 1, pin 6)
 * ciaaddra - 0x03
 * JOYnDAT:
 *     pin 1 (up) = ((JOYnDAT >> 0) ^ (JOYnDAT >> 1)) & 1
 *     pin 2 (dn) = ((JOYnDAT >> 8) ^ (JOYnDAT >> 9)) & 1
 *     pin 3 (lt) = (JOYnDAT >> 9) & 1
 *     pin 4 (rt) = (JOYnDAT >> 1) & 1
 * POTGO/POTINP:
 *     port 1: pin 9 (1 << 10), pin 5 (1 << 8)
 *     port 2: pin 9 (1 << 14), pin 5 (1 << 12)
 */
static inline ULONG llPollJoystick(int port)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    volatile struct CIA *cia = (struct CIA *)0xbfe001;
    ULONG bits = 0;
    UWORD joydat;
    UBYTE cmask = (port == 0) ? (1 << 6) : (1 << 7);

    /* 'red' - /FIRn on CIA Port A */
    bits |= (cia->ciapra & cmask) ? 0 : JPF_BUTTON_RED;

    /* 'blue' - Pin 9 on POTINP */
    bits |= ((custom->potinp >> ((port == 0) ? 10 : 14)) & 1) ? 0 : JPF_BUTTON_BLUE;

    /* Get the joypad bits */
    joydat = (port == 0) ? custom->joy0dat : custom->joy1dat;
   
    if ((joydat >> 1) & 1) {
        bits |= JPF_JOY_RIGHT;
    }
    if ((joydat >> 9) & 1) {
        bits |= JPF_JOY_LEFT;
    }
    if (((joydat >> 0) ^ (joydat >> 1)) & 1) {
        bits |= JPF_JOY_DOWN;
    }
    if (((joydat >> 8) ^ (joydat >> 9)) & 1) {
        bits |= JPF_JOY_UP;
    }

    return JP_TYPE_JOYSTK | bits;
}


ULONG llPortOpen(struct LowLevelBase *LowLevelBase, int port, UWORD *bits)
{
    UWORD potbits, potres;
    ULONG type = LowLevelBase->ll_PortType[port];
    struct Library *PotgoBase = LowLevelBase->ll_PotgoBase;
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    if (type == 0 || type == JP_TYPE_GAMECTLR || type == JP_TYPE_JOYSTK) {
        if (port == 0)
            potbits = POTGO_GAMEPAD_PORT0;
        else 
            potbits = POTGO_GAMEPAD_PORT1;
        potres = AllocPotBits(potbits);
        if (potres != potbits) {
            D(bug("%s: Can't allocate PotGo bits 0x%04x\n", __func__));

            FreePotBits(potres);
            type = JP_TYPE_NOTAVAIL;
            PotgoBase = NULL;
        } else {
            /* Set Pin 5 as output, load mode */
            UWORD pot;
            pot = custom->potinp;
            pot &= ~((port == 0) ? (3 << 8) : (3 << 12));
            custom->potgo = pot | ((port == 0) ? (3 << 8) : (3 << 12));
        }
    } else {
        /* No Potgo bits allocated */
        potbits = 0;
    }

    /* Handle autosense */
    if (type == 0) {
        type = llPollGameCtrl(port);
        if (type == 0)
            type = JP_TYPE_JOYSTK;
        LowLevelBase->ll_PortType[port] = type & JP_TYPE_MASK;
        D(bug("%s: Autosense: 0x%08x\n", __func__, LowLevelBase->ll_PortType[port]));
    }

    *bits = potbits;
    return type;
}
    
static VOID llPortClose(struct LowLevelBase *LowLevelBase, int port, UWORD potbits)
{
    struct Library *PotgoBase = LowLevelBase->ll_PotgoBase;

    if (potbits) {
        FreePotBits(potbits);
    }
}


AROS_LH1(ULONG, ReadJoyPort,
    AROS_LHA(ULONG, port, D0),
    struct LowLevelBase *, LowLevelBase, 5, LowLevel)
{
  AROS_LIBFUNC_INIT

//  D(bug("%s: Port %d\n", __func__, port));

  if (port == 0 || port == 1) {
      ULONG state, type;
      UWORD potbits;

      type = llPortOpen(LowLevelBase, port, &potbits);
      switch (type) {
      case JP_TYPE_GAMECTLR:
          state = llPollGameCtrl(port);
          break;
      case JP_TYPE_JOYSTK:
          state = llPollJoystick(port);
          break;
      default:
          state = JP_TYPE_UNKNOWN;
          break;
      }
      llPortClose(LowLevelBase, port, potbits);

      return state;
  }

  return JP_TYPE_NOTAVAIL; // return failure until implemented

  AROS_LIBFUNC_EXIT
} /* ReadJoyPort */
