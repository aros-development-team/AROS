/* 
    Copyright © 2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef MUI_LISTIMAGE_H
#define MUI_LISTIMAGE_H

/* Structure to display images in the List class using \33O[%lx]
 * Included both by the text engine (the one parsing List entries)
 * and the List class.
 */

/* Created/returned by List_CreateImage */
struct ListImage
{
    struct MinNode node;
    Object        *obj;
};

#endif /* MUI_LISTIMAGE_H */

