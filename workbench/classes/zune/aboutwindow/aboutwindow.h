#ifndef ZUNE_ABOUTWINDOW_H
#define ZUNE_ABOUTWINDOW_H

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the AboutWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#include <utility/tagitem.h>
#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_AboutWindow                 "AboutWindow.mcc"

/*** Identifier base ********************************************************/
#define MUIB_AboutWindow                 (MUIB_AROS | 0x00000400)

/*** Public attributes ******************************************************/
/*+ 
    [I--] Object *
    Image object to display as a logotype. Defaults to NULL.
+*/
#define MUIA_AboutWindow_Image           (MUIB_AboutWindow | 0x00000000)

/*+
    [I--] CONST_STRPTR
    Application title. Defaults to the value of MUIA_Application_Title if 
    unspecified. If set to NULL, it will be omitted. Example: "Wanderer".         
+*/
#define MUIA_AboutWindow_Title           (MUIB_AboutWindow | 0x00000001) 

/*+
    [I--] CONST_STRPTR
    Version number. Defaults to the value of MUIA_Application_Version_Number
    if unspecified. If set to NULL, it will be omitted. Example: "1.5"
+*/
#define MUIA_AboutWindow_Version_Number  (MUIB_AboutWindow | 0x00000002)

/*+
    [I--] CONST_STRPTR
    Date information on the standard international YYYY-MM-DD format. Defaults
    to the value of MUIA_Application_Version_Date if unspecified. If set to 
    NULL, it will be omitted.
+*/
#define MUIA_AboutWindow_Version_Date    (MUIB_AboutWindow | 0x00000003)

/*+
    [I--] CONST_STRPTR
    Arbitrary extra version information. Defaults to the value of 
    MUIA_Application_Version_Extra if not specified. If set to NULL, it will
    be omitted. Examples: "nightly build", "demo version"
+*/
#define MUIA_AboutWindow_Version_Extra   (MUIB_AboutWindow | 0x00000004)

/*+ 
    [I--] CONST_STRPTR
    Copyright information. Defaults to the value of MUIA_Application_Copyright
    if not specified. If set to NULL, it will be omitted. Example:
    "Copyright © 2003, The AROS Development Team. All rights reserved."
+*/
#define MUIA_AboutWindow_Copyright       (MUIB_AboutWindow | 0x00000005)

/*+ 
    [I--] CONST_STRPTR 
    Short description, of around 40 characters. Defaults to the value of 
    MUIA_Application_Description if unspecified. If set to NULL, it will be
    omitted. Example: "Allows you to change the font preferences."
+*/
#define MUIA_AboutWindow_Description     (MUIB_AboutWindow | 0x00000006)

/*+ 
    [I--] struct TagItem *
    List of authors (see below how to create it).
+*/
#define MUIA_AboutWindow_Authors         (MUIB_AboutWindow | 0x00000007)

/*+ 
    [I--] struct TagItem *
    List of sponsors (see below how to create it). 
+*/
#define MUIA_AboutWindow_Sponsors        (MUIB_AboutWindow | 0x00000008)


/*+
    Defines and macros for creating taglists for MUIA_AboutWindow_Authors
    and MUIA_AboutWindow_Sponsors. Using these, you can create an 
    apropriate taglist like this::
    
        struct TagItem *AUTHORS = TAGLIST
        (
            SECTION
            (
                SID_PROGRAMMING,
                NAME("Marvin"),
                NAME("Ford Prefect")
            ),
            SECTION
            (
                SID_TRANSLATING,
                NAME("Zaphod Beeblebrox")
            )
        );
+*/
#define SECTION_ID              (MUIB_AboutWindow | 0x000000F0)
#define NAME_STRING             (MUIB_AboutWindow | 0x000000F1)

#define SID_NONE                (0)
#define SID_PROGRAMMING         (1)
#define SID_TRANSLATING         (2)

#define SECTION(id, args...)    SECTION_ID, id, args
#define NAME(name)              NAME_STRING, ((IPTR) (name))

/*** Protected attributes ***************************************************/
#if 0 /* FIXME: Implement */
#define MUIA_AboutWindow_RootGroup         (MUIB_AboutWindow | 0x00000009)
#define MUIA_AboutWindow_ImageGroup        (MUIB_AboutWindow | 0x0000000a)
#define MUIA_AboutWindow_VersionObject     (MUIB_AboutWindow | 0x0000000b)
#define MUIA_AboutWindow_CopyrightObject   (MUIB_AboutWindow | 0x0000000c)
#define MUIA_AboutWindow_DescriptionGroup  (MUIB_AboutWindow | 0x0000000d)
#define MUIA_AboutWindow_DescriptionObject (MUIB_AboutWindow | 0x0000000e)
#endif

/*** Macros *****************************************************************/
#define AboutWindowObject MUIOBJMACRO_START(MUIC_AboutWindow)

#endif /* ZUNE_ABOUTWINDOW_H */
