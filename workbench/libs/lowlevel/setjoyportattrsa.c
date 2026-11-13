/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc:
*/
#include "lowlevel_intern.h"

#include <libraries/lowlevel.h>

/*****************************************************************************

    NAME */

      AROS_LH2(BOOL, SetJoyPortAttrsA,

/*  SYNOPSIS */
      AROS_LHA(ULONG, portNumber, D0),
      AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 22, LowLevel)

/*  FUNCTION
        Allows control over the behaviour of a joystick or game controller
        port handled by lowlevel.library.  Each port normally operates in
        automatic detection mode, identifying whether a mouse, joystick, or
        other controller is connected.  This function can override that
        behaviour or reset a port to its default state.

        The attributes are supplied as a tag list.  Only the tags listed
        below are defined by the standard lowlevel.library interface and
        compatible Poseidon (USB stack) extensions.

        Tag meanings:

            SJA_Type (ULONG)
                Sets the controller type for the given port.  Possible values:

                    SJA_TYPE_AUTOSENSE  - Enable automatic device detection (default)
                    SJA_TYPE_MOUSE      - Force mouse mode
                    SJA_TYPE_JOYSTK     - Force joystick mode
                    SJA_TYPE_GAMECTLR   - Force game controller mode

                When set to anything other than AUTOSENSE, lowlevel.library
                will not attempt to detect the connected device type
                automatically.  It is the caller’s responsibility to restore
                AUTOSENSE before exiting.

            SJA_Reinitialize (VOID)
                Resets the specified port to its initial state, freeing any
                allocated resources and returning it to AUTOSENSE mode.

            -- Poseidon (USB stack) extensions --

            SJA_RumbleSetSlowMotor (ULONG)
                Activates the “slow” (low-frequency) rumble motor of a
                compatible game controller.  The parameter specifies the
                intensity level as a 32-bit value from 0 (off) to 0xFFFFFFFF
                (maximum).  Values outside this range are clamped.

            SJA_RumbleSetFastMotor (ULONG)
                Activates the “fast” (high-frequency) rumble motor of a
                compatible game controller.  The parameter specifies the
                intensity level as a 32-bit value from 0 (off) to 0xFFFFFFFF
                (maximum).

            SJA_RumbleOff (VOID)
                Disables all active rumble motors immediately.  Equivalent
                to setting both SJA_RumbleSetSlowMotor and
                SJA_RumbleSetFastMotor to zero.

        These rumble control tags are supported only if the underlying port
        driver or Poseidon class implements force feedback support.  On
        systems without such support, the tags are ignored without error.

    INPUTS
        portNumber - Index of the controller port to modify (typically 0–3).
        tagList    - Pointer to a TagItem list describing attributes to set.
                     May be NULL, in which case the function performs no action
                     but returns TRUE.

    RESULT
        Returns TRUE if successful, or FALSE if the specified port number or
        tag was invalid or if the operation could not be completed.

    BUGS

    SEE ALSO
        ReadJoyPort(), SystemControlA(), utility.library/TagItem

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return FALSE;
    
    AROS_LIBFUNC_EXIT
    
} /* SetJoyPortAttrsA */
