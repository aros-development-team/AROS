#ifndef DEVICES_KEYBOARD_H
#define DEVICES_KEYBOARD_H

/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Important defines and structures for keyboard.device
    Lang: english
*/

#include <exec/io.h>

/**********************************************************************
 ********************** Keyboard Device Commands **********************
 **********************************************************************/

#define KBD_READEVENT        (CMD_NONSTD + 0)
#define KBD_READMATRIX       (CMD_NONSTD + 1)
#define KBD_ADDRESETHANDLER  (CMD_NONSTD + 2)
#define KBD_REMRESETHANDLER  (CMD_NONSTD + 3)
#define KBD_RESETHANDLERDONE (CMD_NONSTD + 4)

#endif /* DEVICES_KEYBOARD_H */
