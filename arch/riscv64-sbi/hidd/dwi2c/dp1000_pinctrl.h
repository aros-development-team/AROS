/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Applying a device tree node's pin multiplexing.

    The DP1000 brings every peripheral out through a pad that has to be
    switched to it first, and the firmware only switches the ones it
    uses itself.

    This knows nothing about i2c and should move to a linklib when a
    second driver wants it.
*/

#ifndef DP1000_PINCTRL_H
#define DP1000_PINCTRL_H

#include <exec/types.h>
#include <fdt.h>

/*
 * Switch the pads named by a node's "pinctrl-0" to the functions it asks
 * for. FALSE when there is no pin controller or the node asks for
 * nothing; neither is worth failing a driver over. The register block is
 * mapped once and kept.
 */
BOOL DP1000_PinctrlApply(APTR KernelBase, fdt_node_t node);

#endif /* DP1000_PINCTRL_H */
