/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

struct ARM_Implementation
{
    IPTR        ARMI_Family;
    IPTR        ARMI_Platform;
    APTR        (*ARMI_InitTimer) (APTR); // takes a pointer to KernelBase as input, and returns struct IntrNode
    void        (*ARMI_Delay) (int);
    void        (*ARMI_LED_Toggle) (int, int);
};

// values for arm_toggle_led

#define ARM_LED_ON              1
#define ARM_LED_OFF             0

#define ARM_LED_POWER           0
#define ARM_LED_ACTIVITY        1



