#ifndef ZUNE_ABOUTWINDOW_H
#define ZUNE_ABOUTWINDOW_H

#include <utility/tagitem.h>
#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_AboutWindow          "AboutWindow.mcc"

/*** Identifier base ********************************************************/
#define MUIB_AboutWindow          (MUIB_AROS | 0x00000400)

/*** Attributes *************************************************************/
#define MUIA_AboutWindow_Image    (MUIB_AboutWindow | 0x00000000) /* I-- Object *         */
#define MUIA_AboutWindow_Authors  (MUIB_AboutWindow | 0x00000001) /* I-- struct TagItem * */
#define MUIA_AboutWindow_Sponsors (MUIB_AboutWindow | 0x00000002) /* I-- struct TagItem * */

/*
    Defines and macros for creating taglists for MUIA_AboutWindow_Authors
    and MUIA_AboutWindow_Sponsors. Using these, you can create an 
    apropriate taglist like this::
    
        struct TagItem *AUTHORS = TAGLIST
        (
            SECTION
            (
                SID_PROGRAMMING,
                NAME, "Marvin",
                NAME, "Ford Prefect"
            ),
            SECTION
            (
                SID_TRANSLATING,
                NAME, "Zaphod Beeblebrox"
            )
        );
*/
#define SECTION_ID                (MUIB_AboutWindow | 0x00000010)
#define NAME                      (MUIB_AboutWindow | 0x00000011)

#define SID_NONE                  (0)
#define SID_PROGRAMMING           (1)
#define SID_TRANSLATING           (2)

#define SECTION(id, args...) SECTION_ID, id, args

/*** Macros *****************************************************************/
#define AboutWindowObject MUIOBJMACRO_START(MUIC_AboutWindow)

#endif /* ZUNE_ABOUTWINDOW_H */
