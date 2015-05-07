#ifndef __KERNEL_ARM_H
#define __KERNEL_ARM_H

/*
    Copyright � 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

struct ARM_Implementation
{
    IPTR                ARMI_Family;
    IPTR                ARMI_Platform;
    APTR                ARMI_PeripheralBase;
    void                (*ARMI_Init) (APTR, APTR); // takes pointers to KernelBase & SysBase as input
    void                (*ARMI_InitCore) (APTR, APTR); // takes pointers to KernelBase & SysBase as input
    void                (*ARMI_SendIPI) (uint32_t, uint32_t, uint32_t); // Sends IPI msg to processors in mask
    APTR                (*ARMI_InitTimer) (APTR); // takes a pointer to KernelBase as input, and returns struct IntrNode
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
    void                (*ARMI_Save_VFP_State) (void *); // saves state of vfp to buffer
    void                (*ARMI_Restore_VFP_State) (void *); // restores state of vfp from buffer
    void                (*ARMI_Init_VFP_State) (void *); // Init VFP state in buffer
};

extern struct ARM_Implementation __arm_arosintern;

// values for arm_toggle_led

#define ARM_LED_ON              1
#define ARM_LED_OFF             0

#define ARM_LED_POWER           0
#define ARM_LED_ACTIVITY        1



#endif // __KERNEL_ARM_H
