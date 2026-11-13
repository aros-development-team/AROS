#ifndef _ICONIMAGE_PRIVATE_H_
#define _ICONIMAGE_PRIVATE_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the IconImage class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#include <workbench/icon.h>

/*** Instance data **********************************************************/
struct IconImage_DATA
{
    struct DiskObject *iid_DiskObject;
};

#endif /* _ICONIMAGE_PRIVATE_H_ */
