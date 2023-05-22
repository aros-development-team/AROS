#ifndef IA_OPTION_H
#define IA_OPTION_H

#include "ia_install.h"

/* ************************************************
        Install Option Class Methods/Attribs
 * ************************************************/
#define MUIM_InstallOption_Update               (MUIM_IO_BASE + 0x1)

#define MUIA_InstallOption_List                 (MUIA_IO_BASE + 0x1)            /* Install GUI Object representing the object   */
#define MUIA_InstallOption_Obj                  (MUIA_IO_BASE + 0x2)            /* Install GUI Object representing the object   */
#define MUIA_InstallOption_ID                   (MUIA_IO_BASE + 0x3)            /* Install 'unique' Option ID                   */
#define MUIA_InstallOption_ValueTag             (MUIA_IO_BASE + 0x4)            /* TAG used to access the GUI objects value     */
#define MUIA_InstallOption_Value                (MUIA_IO_BASE + 0x5)            /* The current value for the option, at this
                                                                                   stage.
                                                                                   N.B. this value only changes when the stage
                                                                                   changes                                      */

#define MUIV_InstallOptionID_Source             (-2L)                           /* Generic attrib for an install source path    */
#define MUIV_InstallOptionID_Dest               (-3L)                           /* Generic attrib for an install target path    */
#define MUIV_InstallOptionID_StorageAvail       (-4L)                           /* Generic attrib for storage device (maybe)    */

static void OPTOSET(Object *optObj, IPTR optTag, IPTR optVal)
{
    Object *optObjS;
    GET(optObj, MUIA_InstallOption_Obj, &optObjS);
    SET(optObjS, optTag, optVal);
}

static void OPTONNSET(Object *optObj, IPTR optTag, IPTR optVal)
{
    Object *optObjS;
    GET(optObj, MUIA_InstallOption_Obj, &optObjS);
    NNSET(optObjS, optTag, optVal);
}

static void OPTOGET(Object *optObj, IPTR optTag, IPTR *optstorage)
{
    Object *optObjS;
    GET(optObj, MUIA_InstallOption_Obj, &optObjS);
    GET(optObjS, optTag, optstorage);
}

static IPTR XOPTOGET(Object *optObj, IPTR optTag)
{
    Object *optObjS;
    GET(optObj, MUIA_InstallOption_Obj, &optObjS);
    return XGET(optObjS, optTag);
}

#endif /* IA_OPTION_H */