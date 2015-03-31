/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

struct ARM_Implementation
{
    IPTR        ARMI_Family;
    IPTR        ARMI_Platform;
    void (*ARMI_Delay) (IPTR);
    void (*ARMI_LED_Toggle) (IPTR, IPTR);
};

// values for arm_toggle_led

#define ARM_LED_ON              1
#define ARM_LED_OFF             0

#define ARM_LED_POWER           0
#define ARM_LED_ACTIVITY        1



