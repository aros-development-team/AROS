#ifndef _MUI_CLASSES_RADIO_H
#define _MUI_CLASSES_RADIO_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Radio         "Radio.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Radio         (MUIB_ZUNE | 0x00002a00)  

/*** Attributes *************************************************************/
#define MUIA_Radio_Active  (MUIB_MUI|0x00429b41) /* MUI:V4  isg LONG      */
#define MUIA_Radio_Entries (MUIB_MUI|0x0042b6a1) /* MUI:V4  i.. STRPTR *  */
/* MUIA_Radio_CustomChilds: 
 * if you supply this arg with a NULL-terminated array of objects, they will be
 * used instead of the labels created with MUIA_Radio_Entries.
 * These objects must be subclasses of Area, but not necessarily Text.
 * See Zune Prefs for examples.
 */ 
#define MUIA_Radio_CustomChilds  (MUIB_Radio | 0x00000000) /* Zune:20030606  i.. Object*[] */
/* MUIA_Radio_HorizEntries:
 * by default it's TRUE, meaning that each individual radio entry (an entry
 * being a radio image and a labelling object) will be a horiz group.
 * To layout all entries within the gadget, use MUIA_Group_Horiz instead.
 */
#define MUIA_Radio_HorizEntries  (MUIB_Radio | 0x00000001) /* Zune:20030606  i.. BOOL */


extern const struct __MUIBuiltinClass _MUI_Radio_desc; /* PRIV */

#endif /* _MUI_CLASSES_RADIO_H */
