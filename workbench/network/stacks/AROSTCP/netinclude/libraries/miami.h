#ifndef  LIBRARIES_MIAMI_H
#define  LIBRARIES_MIAMI_H

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*
**	$VER: libraries/miami.h 2.11 (25.12.97)
**
**	C prototypes. For use with 32 bit integers only.
**
**	(C) Copyright 1997 Nordic Global Inc.
**	    All Rights Reserved
*/

struct MiamiPFBuffer {
	unsigned long flags;			/* currently unused */
	unsigned char *data;
	unsigned long length;
	unsigned char *name;
	unsigned char itype;			/* interface type MIAMIPFBIT_... */
	unsigned char ptype;			/* packet type MIAMIPFBPT_... */
	unsigned char pad[2];
};

#define MIAMIPFBIT_LOOP		0
#define MIAMIPFBIT_BUILTIN	1

#define MIAMIPFBPT_IP		0
#define MIAMIPFBPT_ARP		1


#define MIAMICPU_M68KREG	1
#define MIAMICPU_PPCV4		2

#define MIAMITAG_DUMMY		(TAG_USER+0x570000)
#define MIAMITAG_HOOKTYPE	(MIAMITAG_DUMMY+1)

#endif
