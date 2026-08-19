/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Synopsys DesignWare I2C HIDD for RP1 (Raspberry Pi 5)
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <hidd/i2c.h>

#include "i2c_dw.h"

#include LC_LIBDEFS_FILE

APTR KernelBase __attribute__((used)) = NULL;

static void dw_i2c_disable(IPTR base)
{
    int tries = 1000;
    dw_i2c_wr(base, DW_IC_ENABLE, 0);
    while ((dw_i2c_rd(base, DW_IC_ENABLE_STATUS) & IC_ENABLE_ENABLE) && --tries)
        dw_i2c_udelay(10);
}

static void dw_i2c_enable(IPTR base)
{
    dw_i2c_wr(base, DW_IC_ENABLE, IC_ENABLE_ENABLE);
}

static BOOL dw_i2c_wait_tx_empty(IPTR base)
{
    int tries = 10000;
    while (!(dw_i2c_rd(base, DW_IC_STATUS) & IC_STATUS_TFE) && --tries)
        dw_i2c_udelay(10);
    return tries > 0;
}

static BOOL dw_i2c_wait_rx_data(IPTR base)
{
    int tries = 10000;
    while (!(dw_i2c_rd(base, DW_IC_STATUS) & IC_STATUS_RFNE) && --tries)
        dw_i2c_udelay(10);
    return tries > 0;
}

void METHOD(I2CDW, Hidd_I2C, PutByte)
{
    struct I2CDWBase *LIBBASE = (struct I2CDWBase *)cl->UserData;
    IPTR base = LIBBASE->i2c_RegBase;

    dw_i2c_wr(base, DW_IC_DATA_CMD, msg->data | IC_DATA_CMD_STOP);
    dw_i2c_wait_tx_empty(base);
}

void METHOD(I2CDW, Hidd_I2C, GetByte)
{
    struct I2CDWBase *LIBBASE = (struct I2CDWBase *)cl->UserData;
    IPTR base = LIBBASE->i2c_RegBase;

    dw_i2c_wr(base, DW_IC_DATA_CMD, IC_DATA_CMD_READ | IC_DATA_CMD_STOP);
    if (dw_i2c_wait_rx_data(base))
        *(msg->data) = (UBYTE)(dw_i2c_rd(base, DW_IC_DATA_CMD) & 0xFF);
}

void METHOD(I2CDW, Hidd_I2C, WriteRead)
{
    struct I2CDWBase *LIBBASE = (struct I2CDWBase *)cl->UserData;
    IPTR base = LIBBASE->i2c_RegBase;
    ULONG i;

    /* Write phase (no STOP — restart follows) */
    for (i = 0; i < msg->writeLength; i++)
        dw_i2c_wr(base, DW_IC_DATA_CMD, msg->writeBuffer[i]);
    dw_i2c_wait_tx_empty(base);

    /* Read phase */
    for (i = 0; i < msg->readLength; i++) {
        ULONG cmd = IC_DATA_CMD_READ;
        if (i == msg->readLength - 1)
            cmd |= IC_DATA_CMD_STOP;
        dw_i2c_wr(base, DW_IC_DATA_CMD, cmd);
    }

    /* Collect read data */
    for (i = 0; i < msg->readLength; i++) {
        if (dw_i2c_wait_rx_data(base))
            msg->readBuffer[i] = (UBYTE)(dw_i2c_rd(base, DW_IC_DATA_CMD) & 0xFF);
    }
}

static int I2CDW_Init(LIBBASETYPEPTR LIBBASE)
{
    IPTR base;
    struct Library *rp1base;
    IPTR *fields;

    D(bug("[I2C-DW] Init\n"));

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return FALSE;

    /* Only on RPi5 */
    if ((IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) == 0xFE000000) {
        D(bug("[I2C-DW] RPi4 — using BCM BSC instead\n"));
        return FALSE;
    }

    rp1base = OpenResource("rp1.resource");
    if (!rp1base)
        return FALSE;

    fields = (IPTR *)((UBYTE *)rp1base + sizeof(struct Library));
    if (!fields[0])
        return FALSE;

    /* Use I2C1 (GPIO 2/3 on header) at BAR1 + 0x074000 */
    base = fields[1] + 0x074000;
    LIBBASE->i2c_RegBase = base;

    D(bug("[I2C-DW] DesignWare I2C at 0x%p\n", base));

    /* Configure: master mode, standard speed, 7-bit addr, restart enable */
    dw_i2c_disable(base);
    dw_i2c_wr(base, DW_IC_CON,
              IC_CON_MASTER | IC_CON_SPEED_STD | IC_CON_RESTART_EN | IC_CON_SLAVE_DIS);

    /* SCL timing for 100kHz at 200MHz input clock */
    dw_i2c_wr(base, DW_IC_SS_SCL_HCNT, 1000);
    dw_i2c_wr(base, DW_IC_SS_SCL_LCNT, 1000);

    /* Disable all interrupts (polling mode) */
    dw_i2c_wr(base, DW_IC_INTR_MASK, 0);

    dw_i2c_enable(base);

    D(bug("[I2C-DW] Initialized at 100kHz\n"));

    return TRUE;
}

ADD2INITLIB(I2CDW_Init, 0)
