#ifndef ZUNE_ICONIMAGE_H
#define ZUNE_ICONIMAGE_H

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the IconImage class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_IconImage             "IconImage.mcc"

/*** Identifier base ********************************************************/
#define MUIB_IconImage             (MUIB_AROS | 0x00000300)

/*** Attributes *************************************************************/
#define MUIA_IconImage_DiskObject  (MUIB_IconImage | 0x00000000) /* I-- struct DiskObject * */
#define MUIA_IconImage_File        (MUIB_IconImage | 0x00000001) /* I-- CONST_STRPTR        */

/*** Macros *****************************************************************/
#define IconImageObject MUIOBJMACRO_START(MUIC_IconImage)

#endif /* ZUNE_ICONIMAGE_H */
