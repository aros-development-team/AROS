#include "ati.h"
#include "radeon.h"
#include "radeon_reg.h"
#include "radeon_macros.h"

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/exec.h>

void METHOD(ATII2C, Hidd_I2C, PutBits)
{
    struct ati_staticdata *sd = SD(cl);
    ULONG val;
    
    val = INREG(sd->Card.DDCReg) & (ULONG)~(RADEON_GPIO_EN_0 | RADEON_GPIO_EN_1);
    val |= (msg->scl ? 0:RADEON_GPIO_EN_1);
    val |= (msg->sda ? 0:RADEON_GPIO_EN_0);
    OUTREG(sd->Card.DDCReg, val);
    val = INREG(sd->Card.DDCReg);
}

void METHOD(ATII2C, Hidd_I2C, GetBits)
{
    struct ati_staticdata *sd = SD(cl);

    ULONG val = INREG(sd->Card.DDCReg);
    *msg->sda = (val & RADEON_GPIO_Y_0) != 0;
    *msg->scl = (val & RADEON_GPIO_Y_1) != 0;
}

ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, I2CBase);
