#ifndef _PRESENTER_H_
#define _PRESENTER_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <utility/tagitem.h>

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Presenter_CLASS;

/*** Macros *****************************************************************/
#define PresenterObject BOOPSIOBJMACRO_START(Presenter_CLASS->mcc_Class)

#endif /* _PRESENTER_H_ */
