#ifndef TOOLLIB_HASH_H
#define TOOLLIB_HASH_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif
#ifndef TOOLLIB_CALLBACK_H
#   include <toollib/callback.h>
#endif

typedef struct _Hash Hash;

Hash * Hash_New      (void);
void   Hash_Delete   (Hash *, CB delNode, CBD userdata);
void   Hash_Store    (Hash *, const char * key, const void * data);
void   Hash_StoreNC  (Hash *, const char * key, const void * data);
void * Hash_Find     (Hash *, const char * key);
void * Hash_FindNC   (Hash *, const char * key);
void   Hash_Traverse (Hash *, CB traverseProc, CBD userdata);

#endif /* TOOLLIB_HASH_H */
