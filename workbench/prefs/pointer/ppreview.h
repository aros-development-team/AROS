#ifndef _PPREVIEW_H_
#define _PPREVIEW_H_

/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_PPreview                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *PPreview_CLASS;

/*** Macros *****************************************************************/
#define PPreviewObject BOOPSIOBJMACRO_START(PPreview_CLASS->mcc_Class)

/*** Attributes *************************************************************/
#define MUIA_PPreview_Alpha     (MUIB_PPreview | 0x00000000) /* ISG  UWORD  */
#define MUIA_PPreview_HSpotX    (MUIB_PPreview | 0x00000001) /* ISG  UWORD  */
#define MUIA_PPreview_HSpotY    (MUIB_PPreview | 0x00000002) /* ISG  UWORD  */
#define MUIA_PPreview_FileName  (MUIB_PPreview | 0x00000003) /* ISG  STRPTR */

#endif /* _PPREVIEW_H_ */
