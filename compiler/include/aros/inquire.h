#ifndef AROS_INQUIRE_H
#define AROS_INQUIRE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: aros.library ArosInquire
    Lang: english
*/

#include <utility/tagitem.h>

#define AI_Base			(TAG_USER)

/* If you use any of these tags, the tag's ti_Data field should point to the
   location where the result is stored. */

/* General tags */
#define AI_ArosVersion		(AI_Base + 1)
	/* ULONG: Major AROS version number, e.g. 41 */

#define AI_ArosReleaseMajor	(AI_Base + 2)
	/* ULONG: Major AROS release version, e.g. 1 */

#define AI_ArosReleaseMinor	(AI_Base + 3)
	/* ULONG: Minor AROS release version, e.g. 11 */

#define AI_ArosReleaseDate	(AI_Base + 4)
	/* LONG: Days since 1978-01-01 */

#define AI_ArosBuildDate	(AI_Base + 5)
	/* STRPTR */

#define AI_ArosVariant      (AI_Base + 6)
    /* STRPTR */

/* Architecture specific tags */

/* Native Amiga */
#define AI_BaseA		(AI_Base + 0x10000)

#define AI_KickstartBase	(AI_BaseA + 1)
	/* IPTR: Kickstart base address */

#define AI_KickstartSize	(AI_BaseA + 2)
	/* IPTR: Kickstart size */

#define AI_KickstartVersion	(AI_BaseA + 3)
	/* UWORD: Major Kickstart version */

#define AI_KickstartRevision	(AI_BaseA + 4)
	/* UWORD: Minor Kickstart revision */


/* Linux */
#define AI_BaseL		(AI_Base + 0x20000)



#endif /* AROS_AROSSUPPORTBASE_H */
