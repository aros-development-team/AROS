/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi 5 RP1 Southbridge definitions
*/

#ifndef RP1_H
#define RP1_H

#include <exec/types.h>
#include <exec/libraries.h>

#define RP1_PCIE_VENDOR_ID      0x1DE4
#define RP1_PCIE_DEVICE_ID      0x0001
#define RP1_BAR1_SIZE           0x400000  /* 4MB peripheral space */

#define RP1_UART0_OFFSET        0x030000
#define RP1_UART1_OFFSET        0x034000
#define RP1_SPI0_OFFSET         0x050000
#define RP1_I2C0_OFFSET         0x070000
#define RP1_I2C1_OFFSET         0x074000
#define RP1_I2C2_OFFSET         0x078000
#define RP1_GPIO_OFFSET         0x0D0000
#define RP1_USB0_OFFSET         0x100000
#define RP1_USB1_OFFSET         0x110000
#define RP1_ETH_OFFSET          0x180000

struct RP1Base {
    struct Library  rp1_Lib;
    BOOL            rp1_Present;
    IPTR            rp1_BAR1;       /* PCIe BAR1 base address */

    /* Pre-computed peripheral base addresses */
    IPTR            rp1_USB0;
    IPTR            rp1_USB1;
    IPTR            rp1_ETH;
    IPTR            rp1_GPIO;
    IPTR            rp1_I2C0;
    IPTR            rp1_I2C1;
    IPTR            rp1_UART0;
};

#endif /* RP1_H */
