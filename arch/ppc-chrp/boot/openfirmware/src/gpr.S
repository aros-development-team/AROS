/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#define GLOBAL(x)   .global x;x:

GLOBAL(_savegpr_14)   stw     14,-72(11)      /* save gp registers */
GLOBAL(_savegpr_15)   stw     15,-68(11)
GLOBAL(_savegpr_16)   stw     16,-64(11)
GLOBAL(_savegpr_17)   stw     17,-60(11)
GLOBAL(_savegpr_18)   stw     18,-56(11)
GLOBAL(_savegpr_19)   stw     19,-52(11)
GLOBAL(_savegpr_20)   stw     20,-48(11)
GLOBAL(_savegpr_21)   stw     21,-44(11)
GLOBAL(_savegpr_22)   stw     22,-40(11)
GLOBAL(_savegpr_23)   stw     23,-36(11)
GLOBAL(_savegpr_24)   stw     24,-32(11)
GLOBAL(_savegpr_25)   stw     25,-28(11)
GLOBAL(_savegpr_26)   stw     26,-24(11)
GLOBAL(_savegpr_27)   stw     27,-20(11)
GLOBAL(_savegpr_28)   stw     28,-16(11)
GLOBAL(_savegpr_29)   stw     29,-12(11)
GLOBAL(_savegpr_30)   stw     30,-8(11)
GLOBAL(_savegpr_31)   stw     31,-4(11)
                      blr

GLOBAL(_restgpr_14)   lwz     14,-72(11)      /* restore gp registers */
GLOBAL(_restgpr_15)   lwz     15,-68(11)
GLOBAL(_restgpr_16)   lwz     16,-64(11)
GLOBAL(_restgpr_17)   lwz     17,-60(11)
GLOBAL(_restgpr_18)   lwz     18,-56(11)
GLOBAL(_restgpr_19)   lwz     19,-52(11)
GLOBAL(_restgpr_20)   lwz     20,-48(11)
GLOBAL(_restgpr_21)   lwz     21,-44(11)
GLOBAL(_restgpr_22)   lwz     22,-40(11)
GLOBAL(_restgpr_23)   lwz     23,-36(11)
GLOBAL(_restgpr_24)   lwz     24,-32(11)
GLOBAL(_restgpr_25)   lwz     25,-28(11)
GLOBAL(_restgpr_26)   lwz     26,-24(11)
GLOBAL(_restgpr_27)   lwz     27,-20(11)
GLOBAL(_restgpr_28)   lwz     28,-16(11)
GLOBAL(_restgpr_29)   lwz     29,-12(11)
GLOBAL(_restgpr_30)   lwz     30,-8(11)
GLOBAL(_restgpr_31)   lwz     31,-4(11)
                      blr

GLOBAL(_restgpr_14_x) lwz     14,-72(11)      /* restore gp registers */
GLOBAL(_restgpr_15_x) lwz     15,-68(11)
GLOBAL(_restgpr_16_x) lwz     16,-64(11)
GLOBAL(_restgpr_17_x) lwz     17,-60(11)
GLOBAL(_restgpr_18_x) lwz     18,-56(11)
GLOBAL(_restgpr_19_x) lwz     19,-52(11)
GLOBAL(_restgpr_20_x) lwz     20,-48(11)
GLOBAL(_restgpr_21_x) lwz     21,-44(11)
GLOBAL(_restgpr_22_x) lwz     22,-40(11)
GLOBAL(_restgpr_23_x) lwz     23,-36(11)
GLOBAL(_restgpr_24_x) lwz     24,-32(11)
GLOBAL(_restgpr_25_x) lwz     25,-28(11)
GLOBAL(_restgpr_26_x) lwz     26,-24(11)
GLOBAL(_restgpr_27_x) lwz     27,-20(11)
GLOBAL(_restgpr_28_x) lwz     28,-16(11)
GLOBAL(_restgpr_29_x) lwz     29,-12(11)
GLOBAL(_restgpr_30_x) lwz     30,-8(11)
GLOBAL(_restgpr_31_x) lwz     0,4(11)
                      lwz     31,-4(11)
                      mtlr    0
                      mr      1,11
                      blr
