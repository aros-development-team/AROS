#ifndef OBJCACHE_H
#define OBJCACHE_H

typedef struct
{
    int just_for_type_checking;
} ObjectCache;

ObjectCache *create_object_cache(OOP_Class *classPtr, STRPTR classID, struct TagItem *createTags, struct GfxBase *GfxBase);
VOID delete_object_cache(ObjectCache *objectCache, struct GfxBase *GfxBase);
OOP_Object *obtain_cache_object(ObjectCache *objectCache, struct GfxBase *GfxBase);
VOID release_cache_object(ObjectCache *objectCache, OOP_Object *object, struct GfxBase *GfxBase);

#endif
