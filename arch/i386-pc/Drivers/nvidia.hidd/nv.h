#ifndef HIDD_NV_H
#define HIDD_NV_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Nvidia header file.
    Lang: English.
*/


#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef HIDD_PCI_H
#	include <hidd/pci.h>
#endif

//#include "bitmap.h"

/***** nVidia gfx HIDD *******************/

/* IDs */
#define IID_Hidd_NVgfx		"hidd.gfx.nvidia"
#define CLID_Hidd_NVgfx		"hidd.gfx.nvidia"

/* misc */

struct nv_staticdata
{
	struct Library		*oopbase;
	struct Library		*utilitybase;
	struct ExecBase		*sysbase;

	HIDDT_PCI_Device	*card;

	OOP_Class			*nvclass;

	VOID				(*activecallback)(APTR, OOP_Object *, BOOL);
	APTR				callbackdata;
};

#define NSD(cl) ((struct nv_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)NSD(cl)->oopbase)
#define UtilityBase	((struct Library *)NSD(cl)->utilitybase)
#define SysBase		(NSD(cl)->sysbase)

#endif /* HIDD_NV_H */

