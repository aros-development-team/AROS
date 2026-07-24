#ifndef __KERNEL_ARM_H
#define __KERNEL_ARM_H

#include <asm/cpu.h>

/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    AArch64 platform implementation interface.
    Named kernel_arm.h for compatibility with shared code.
*/

struct ARM_Implementation
{
    IPTR                ARMI_Family;
    IPTR                ARMI_Platform;
    APTR                ARMI_PeripheralBase;
    cpumask_t           ARMI_AffinityMask;
    void                (*ARMI_Init) (APTR, APTR);
    void                (*ARMI_InitCore) (APTR, APTR);
    void                (*ARMI_SendIPI) (uint32_t, uint32_t, uint32_t);
    APTR                (*ARMI_InitTimer) (APTR);
    void                (*ARMI_Delay) (int);
    unsigned int        (*ARMI_GetTime) (void);
    void                (*ARMI_PutChar) (int);
    void                (*ARMI_SerPutChar) (uint8_t);
    int                 (*ARMI_SerGetChar) (void);
    void                (*ARMI_IRQInit) ();
    void                (*ARMI_IRQEnable) (int);
    void                (*ARMI_IRQDisable) (int);
    void                (*ARMI_IRQProcess) (void);
    void                (*ARMI_FIQProcess) (void);
    void                (*ARMI_LED_Toggle) (int, int);
    void                (*ARMI_Save_VFP_State) (void *);
    void                (*ARMI_Restore_VFP_State) (void *);
    void                (*ARMI_Init_VFP_State) (void *);
};

extern struct ARM_Implementation __arm_arosintern;

#define ARM_LED_ON              1
#define ARM_LED_OFF             0

#define ARM_LED_POWER           0
#define ARM_LED_ACTIVITY        1

#endif /* __KERNEL_ARM_H */
