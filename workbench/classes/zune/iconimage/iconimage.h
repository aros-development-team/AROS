#ifndef ZUNE_ICONIMAGE_H
#define ZUNE_ICONIMAGE_H

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_IconImage             "IconImage.mcc"

/*** Identifier base ********************************************************/
#define MUIB_IconImage             (MUIB_AROS | 0x00000300)

/*** Attributes *************************************************************/
#define MUIA_IconImage_DiskObject  (MUIB_IconImage | 0x00000000) /* I-- struct DiskObject * */

/*** Macros *****************************************************************/
#define IconImageObject MUIOBJMACRO_START(MUIC_IconImage)

#endif /* ZUNE_ICONIMAGE_H */
