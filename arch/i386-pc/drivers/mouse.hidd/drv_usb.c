/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: USB mouse driver.
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>
/* #include <hidd/usb.h> */
#include <devices/inputevent.h>

#include "mouse.h"

#define DEBUG 0
#include <aros/debug.h>

#ifdef HiddMouseAB
#undef HiddMouseAB
#endif
#define HiddMouseAB	(MSD(cl)->hiddMouseAB)

/* defines for buttonstate */

#define LEFT_BUTTON 	1
#define RIGHT_BUTTON 	2
#define MIDDLE_BUTTON	4

/***** Test procedure ***********************************************/

int test_mouse_usb(OOP_Class *cl, OOP_Object *o)
{
    return 0; /* Report no USB mouse */
}

void dispose_mouse_usb(OOP_Class *cl, OOP_Object *o)
{
    return;
}

/*****  *************************************************************/
