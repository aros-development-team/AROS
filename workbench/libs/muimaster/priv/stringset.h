/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __Z_STRING_SET_H__
#define __Z_STRING_SET_H__

//#ifndef _AROS
//#include <glib.h>
//#endif

#include <stdio.h>

struct ZStringSet;
typedef struct ZStringSet ZStringSet;

ZStringSet *z_string_set_new(void);
void z_string_set_destroy(ZStringSet *set);
void z_string_set_add(ZStringSet *set, const char *str);
void z_string_set_dump(ZStringSet *set, FILE *out);

#endif
