/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Basic help functions.
    Lang: English.
*/

#include "icon_intern.h"


/****************************************/
/* Copy the deficon name of type	*/
/* supplied into the mem supplied	*/
/* (GetDefDiskObject & PutDefDiskObject */
/****************************************/

VOID GetDefIconName (LONG def_type, UBYTE * deficonname)
{
    UBYTE * extname = NULL;

    strcpy (deficonname,"ENV:Sys/def_");

    switch(def_type)
    {
    case WBDISK   : extname = "Disk";     break;
    case WBDRAWER : extname = "Drawer";   break;
    case WBTOOL   : extname = "Tool";     break;
    case WBPROJECT: extname = "Project";  break;
    case WBGARBAGE: extname = "Trashcan"; break;
    case WBKICK   : extname = "Kick";     break;
    default: extname = "Unknown"; break; /* Avoid segmentation faults */
    }

    strcat (deficonname, extname);
} /* GetDefIconName */

