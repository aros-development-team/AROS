#ifndef _PRESENTATION_H_
#define _PRESENTATION_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <utility/tagitem.h>

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Presentation_CLASS;

/*** Macros *****************************************************************/
#define PresentationObject BOOPSIOBJMACRO_START(Presentation_CLASS->mcc_Class)

#endif /* _PRESENTATION_H_ */
