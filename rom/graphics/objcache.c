/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/arossupport.h>

#include <oop/oop.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

#include <string.h>

#include "graphics_intern.h"
#include "objcache.h"

#define CACHE_INCREMENT 4

struct cacheitem
{
    OOP_Object *obj;
    BOOL used;
};

struct objcache
{
    struct TagItem  	    *create_tags;
    struct SignalSemaphore  lock;
    struct cacheitem 	    *cache;
    ULONG   	    	    cachesize;
    ULONG   	    	    num_objects;
    STRPTR  	    	    class_id;
    OOP_Class 	    	    *class_ptr;
};


ObjectCache *create_object_cache(OOP_Class *classPtr, STRPTR classID, struct TagItem *createTags, struct GfxBase *GfxBase)
{
    struct objcache *oc;
    
    
    if (   (NULL == classPtr && NULL == classID)
        || (NULL != classPtr && NULL != classID) )
    	return NULL;
	
     
    oc = AllocMem(sizeof (*oc), MEMF_PUBLIC|MEMF_CLEAR);
    if (NULL != oc) {
    
    	InitSemaphore(&oc->lock);
	oc->cachesize = CACHE_INCREMENT;
	oc->num_objects = 0;
	
     	oc->create_tags = CloneTagItems(createTags);
	if (NULL != oc->create_tags) {
	    BOOL got_class = FALSE;
	
	    if (NULL != classID) {
		oc->class_id = AllocVec(strlen(classID) + 1, MEMF_ANY);
	    	if (NULL != oc->class_id) {
	    	    strcpy(oc->class_id, classID);
		    got_class = TRUE;
		}
	    } else {
	    	oc->class_ptr = classPtr;
		got_class = TRUE;
	    }
	    
	    if (got_class) {
		
	    	oc->cache = AllocMem(sizeof (*oc->cache) * oc->cachesize, MEMF_CLEAR);
	    	if (NULL != oc->cache) {
	    	    return (ObjectCache *)oc;
	    	}
		
		FreeVec(oc->class_id);
		    
	    } /* if (got class) */
		
	    FreeTagItems(oc->create_tags);
	    
	} /* if (tagitems copied) */
	
	FreeMem(oc, sizeof (*oc));
    } /* if (objcache struct allocated) */
     
    return NULL;
}

VOID delete_object_cache(ObjectCache *objectCache, struct GfxBase *GfxBase)
{
    struct objcache *oc;
    
    ULONG i;

    oc = (struct objcache *)objectCache;
    
    ObtainSemaphore(&oc->lock);
    
    /* Check if all elements in the object cache are unused */
    for (i = 0; i < oc->num_objects; i ++) {
	    if (oc->cache[i].used) {
		D(bug("!!!! TRYING TO DELETE AN OBJECT CACHE WITH USED OBJECTS !!!!\n"));
    		ReleaseSemaphore(&oc->lock);
		return;

	
	}
    
    }
    
    
    for (i = 0; i < oc->num_objects; i ++) {
    	if (NULL != oc->cache[i].obj)
	    OOP_DisposeObject(oc->cache[i].obj);
	else
	    break;
    }
    
    FreeMem(oc->cache, sizeof (*oc->cache) * oc->cachesize);
    
    FreeTagItems(oc->create_tags);
    
    FreeVec(oc->class_id);
    
    ReleaseSemaphore(&oc->lock);

    FreeMem(oc, sizeof (*oc));
    
    return;
    
}


OOP_Object *obtain_cache_object(ObjectCache *objectCache, struct GfxBase *GfxBase)
{
    struct objcache *oc;
    ULONG i;
    OOP_Object *obj = NULL;
    
    oc = (struct objcache *)objectCache;

/*    bug("obtain_cache_object(classID=%s, classPtr=%p)\n"
    	, oc->class_id, oc->class_ptr);
*/    
    ObtainSemaphore(&oc->lock);
    
    
    /* Look to see if we can find a free object */
    for (i = 0; i < oc->cachesize; i ++) {
    	struct cacheitem *ci;
	
	ci = &oc->cache[i];
/* bug("cache[%d]=%p, %d\n", i, ci->obj, ci->used);
*/    	if (NULL == ci->obj) {
	    break;
	} else {
	    if (!ci->used) {
	  	obj = ci->obj;
		ci->used = TRUE;
		break;
	    }
	}
    }
    
/* bug("obj found in cache: %p\n", obj);    
*/    if (NULL == obj) {
    	struct cacheitem *ci;
    	/* No object free, so we try to create a new one.
	But first we see if the cache can hold it */

	if (oc->num_objects == oc->cachesize) {
	     /* Not more space in the cache, try to expand it */
	     struct cacheitem *newcache;
	     
	     newcache = AllocMem(sizeof (*oc->cache) * (oc->cachesize + CACHE_INCREMENT), MEMF_CLEAR);
	     if (NULL == newcache)
	     	goto exit;
		
	     /* Copy the old cache data */
	     memcpy(newcache, oc->cache, sizeof (*oc->cache) * oc->cachesize);
	     
	     /* Free old cache */
	     FreeMem(oc->cache, sizeof (*oc->cache) * oc->cachesize);

	     /* Use new cache */
	     oc->cache = newcache;
	     oc->cachesize +=CACHE_INCREMENT;
		
	}
	
	/* Try to create a new object */
/* bug("Trying to create new object\n");
*/	ci = &oc->cache[oc->num_objects];
	
	if (oc->class_id)
	    ci->obj = OOP_NewObject(NULL, oc->class_id, oc->create_tags);
	else
	    ci->obj = OOP_NewObject(oc->class_ptr, NULL, oc->create_tags);
	
	if (NULL == ci->obj)
	    goto exit;
	
	oc->num_objects ++;
	ci->used = TRUE;
	
	obj = ci->obj;
    
    }
    
exit:
    ReleaseSemaphore(&oc->lock);
    
    return obj;
    
    
}

VOID release_cache_object(ObjectCache *objectCache, OOP_Object *object, struct GfxBase *GfxBase)
{
    struct objcache *oc;
    ULONG i;
    D(BOOL found = FALSE);
    
    oc = (struct objcache *)objectCache;
    
    ObtainSemaphore(&oc->lock);
    
    for (i = 0; i < oc->cachesize; i ++) {
    	struct cacheitem *ci;
	
	ci = &oc->cache[i];
    	if (NULL == ci->obj) {
	    break;
	} else {
	    if (ci->obj == object) {
	    	/* Object found */
		ci->used = FALSE;
		D(found = TRUE);
	    }
	}
    }

#if DEBUG
    if (!found)
	bug("!!!! TRYING TO RELEASE OBJECT CACHE ELEMENT WHICH WAS NOT PRESENT IN CACHE\n");
#endif

    ReleaseSemaphore(&oc->lock);
    
    return;
    
}
