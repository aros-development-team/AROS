/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __DATASPACEDATA_H__
#define __DATASPACEDATA_H__

#include <uproperties.h>
#include <stringset.h>

struct MUI_DataspaceData
{
    UProperties *uprop;
    ZStringSet   *comments;
};


#endif
