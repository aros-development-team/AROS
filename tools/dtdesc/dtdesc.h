#ifndef DTDESC_H
#define DTDESC_H 1

/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: DataTypesDescriptorCreator
    Lang: English.
*/

/*
 *  includes
 */

#include <c_iff.h>

/*
 *  structs
 */

struct DataTypeHeader
{
 CARD8  *dth_Name;
 CARD8  *dth_BaseName;
 CARD8  *dth_Pattern;
 CARD16 *dth_Mask;
 CARD32  dth_GroupID;
 CARD32  dth_ID;
 CARD16  dth_MaskLen;
 CARD16  dth_Pad;
 CARD16  dth_Flags;
 CARD16  dth_Priority;
};

#endif /* DTDESC_H */

