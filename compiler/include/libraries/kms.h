/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef LIBRARIES_KMS_H
#define LIBRARIES_KMS_H 1

#include <devices/keymap.h>
#include <exec/libraries.h>

#define	KMSNAME "kms.library"

struct KMSLibrary
{
    struct Library kms_Lib;
    UWORD          kms_SwitchQual;	/* Switch hotkey qualifier  */
    UWORD	   kms_SwitchCode;	/* Switch hotkey code	    */
    struct KeyMap *kms_AltKeymap;	/* Alternate keymap pointer */
};

#define KMS_DISABLE 0xFFFF	/* Set kms_SwitchCode to this in order to turn off switcher */

#endif
