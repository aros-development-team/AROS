#ifndef _HASH_H
#define _HASH_H

#ifndef _TOOLLIB_H
#   include <toollib.h>
#endif
#ifndef _CALLBACK_H
#   include <callback.h>
#endif

typedef struct _Hash Hash;

Hash * Hash_New      (void);
void   Hash_Delete   (Hash *, CB delNode, CBD userdata);
void   Hash_Store    (Hash *, const char * key, const void * data);
void * Hash_Find     (Hash *, const char * key);
void   Hash_Traverse (Hash *, CB traverseProc, CBD userdata);

#endif /* _HASH_H */
