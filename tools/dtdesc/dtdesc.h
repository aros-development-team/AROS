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
 uint8_t  *dth_Name;
 uint8_t  *dth_BaseName;
 uint8_t  *dth_Pattern;
 uint16_t *dth_Mask;
 uint32_t  dth_GroupID;
 uint32_t  dth_ID;
 uint16_t  dth_MaskLen;
 uint16_t  dth_Pad;
 uint16_t  dth_Flags;
 uint16_t  dth_Priority;
};

#endif /* DTDESC_H */
