#ifndef _THEMEPREVIEW_H_
#define _THEMEPREVIEW_H_

/*
    Copyright © 2013-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_ThemePreview                       (TAG_USER | 0x10000000)

/*** Attributes *************************************************************/
#define MUIA_ThemePreview_Theme                 (MUIB_ThemePreview | 1)
/* image size to show the theme in; 0 for its default size [IS.] */
#define MUIA_ThemePreview_Size                  (MUIB_ThemePreview | 2)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *ThemePreview_CLASS;
extern struct Library *DecoratorBase;
extern struct Library *DecorThemeBase;

/*** Functions **************************************************************/
/* decorator.library and decortheme.library are shared with the preview
   objects; each call to open them is matched by one to close them */
void ThemePreview_OpenLibs(void);
void ThemePreview_CloseLibs(void);

/*** Macros *****************************************************************/
#define ThemePreviewObject BOOPSIOBJMACRO_START(ThemePreview_CLASS->mcc_Class)

#endif /* _THEMEPREVIEW_H_ */
