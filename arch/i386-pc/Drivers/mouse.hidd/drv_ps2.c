/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: PS/2 mouse driver.
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
#include <devices/inputevent.h>

#include "mouse.h"

#define DEBUG 0
#include <aros/debug.h>

#define HiddMouseAB	(MSD(cl)->hiddMouseAB)

/* defines for buttonstate */

#define LEFT_BUTTON 	1
#define RIGHT_BUTTON 	2
#define MIDDLE_BUTTON	4

/***** Test procedure ***********************************************/

int test_mouse_ps2(OOP_Class *cl, OOP_Object *o)
{
    return 0; /* Report no PS/2 mouse */
}

/*****  *************************************************************/
