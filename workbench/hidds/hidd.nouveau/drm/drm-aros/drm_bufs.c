/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drmP.h"

resource_size_t drm_get_resource_len(struct drm_device *dev, unsigned int resource)
{
    return (resource_size_t)pci_resource_len(dev->pdev, resource);
}

resource_size_t drm_get_resource_start(struct drm_device *dev,
                        unsigned int resource)
{
    return (resource_size_t)pci_resource_start(dev->pdev, resource);
}

// static int drm_addmap_core(struct drm_device *dev, unsigned int offset,
//                unsigned int size, enum drm_map_type type,
//                enum drm_map_flags flags,
//                struct drm_map_list **maplist)
// {
//     struct drm_map *map;
//     struct drm_map_list *list;
//     unsigned long user_token;
//     int ret;    
// 
//     /* HACK to get unique user tokens */
//     static uint64_t map_counter = 0;
//     
//     /* FIXME: Add support for other types */
//     if ((type != _DRM_REGISTERS) && (type != _DRM_FRAME_BUFFER) && (type != _DRM_SCATTER_GATHER))
//     {
//         DRM_ERROR("Type %d UNHANDLED\n", type);
//         return -EINVAL;
//     }
// 
//     map = drm_alloc(sizeof(*map), DRM_MEM_MAPS);
//     if (!map)
//         return -ENOMEM;
// 
//     map->offset = offset;
//     map->size = size;
//     map->flags = flags;
//     map->type = type;
// 
//     switch (map->type) {
//     case _DRM_REGISTERS:
//     case _DRM_FRAME_BUFFER:
//         /* Some drivers preinitialize some maps, Therefore, we just return 
//          * success when duplicate map is tried to be created
//          */
//         list = drm_find_matching_map(dev, map);
//         if (list != NULL) {
//             if (list->map->size != map->size) {
//                 DRM_DEBUG("Matching maps of type %d with "
//                       "mismatched sizes, (%ld vs %ld)\n",
//                       map->type, map->size,
//                       list->map->size);
//                 list->map->size = map->size;
//             }
// 
//             drm_free(map, sizeof(*map), DRM_MEM_MAPS);
//             *maplist = list;
//             return 0;
//         }
// 
//         if (drm_core_has_MTRR(dev)) {
//             if (map->type == _DRM_FRAME_BUFFER ||
//                 (map->flags & _DRM_WRITE_COMBINING)) {
//                 map->mtrr = mtrr_add(map->offset, map->size,
//                              MTRR_TYPE_WRCOMB, 1);
//             }
//         }
// 
//         if (map->type == _DRM_REGISTERS) {
//             map->handle = drm_aros_pci_ioremap(dev->pcidriver, (APTR)map->offset, (IPTR)map->size);
//             if (!map->handle) {
//                 drm_free(map, sizeof(*map), DRM_MEM_MAPS);
//                 return -ENOMEM;
//             }
//         }
//         break;
//     case _DRM_SCATTER_GATHER:
//         if (!dev->sg) {
//             drm_free(map, sizeof(*map), DRM_MEM_MAPS);
//             return -EINVAL;
//         }
//         map->offset += (unsigned long)dev->sg->virtual;
//         break;
//     default:
//         drm_free(map, sizeof(*map), DRM_MEM_MAPS);
//         return -EINVAL;
//     }
// 
// 
//     list = drm_alloc(sizeof(*list), DRM_MEM_MAPS);
//     if (!list) {
//         if (map->type == _DRM_REGISTERS)
//             drm_aros_pci_iounmap(dev->pcidriver, map->handle, map->size);
//         drm_free(map, sizeof(*map), DRM_MEM_MAPS);
//         return -EINVAL;
//     }
//     memset(list, 0, sizeof(*list));
//     list->map = map;
// 
//     ObtainSemaphore(&dev->struct_semaphore);
//     list_add(&list->head, &dev->maplist);
// 
//     /* Assign a 32-bit handle */
//     /* ORIGINAL CODE */
//     /*user_token = (map->type == _DRM_SHM) ? (unsigned long) map->handle :
//         map->offset;
//     ret = drm_map_handle(dev, &list->hash, user_token, 0);
// 
//     if (ret) {
//         if (map->type == _DRM_REGISTERS)
//             drm_aros_pci_iounmap(dev->pcidriver, map->handle, map->size);
//         drm_free(map, sizeof(*map), DRM_MEM_MAPS);
//         drm_free(list, sizeof(*list), DRM_MEM_MAPS);
//         return ret;
//     }
// 
//     list->user_token = list->hash.key << PAGE_SHIFT;*/
// 
//     list->user_token = ++map_counter;
//     ReleaseSemaphore(&dev->struct_semaphore);
// 
//     *maplist = list;
//     return 0;
// }
// 
// int drm_addmap(struct drm_device *dev, unsigned int offset,
//            unsigned int size, enum drm_map_type type,
//            enum drm_map_flags flags, drm_local_map_t ** map_ptr)
// {
//     struct drm_map_list *list;
//     int rc;
// 
//     rc = drm_addmap_core(dev, offset, size, type, flags, &list);
//     if (!rc)
//         *map_ptr = list->map;
//     return rc;
// }
// 
// /**
//  * Remove a map private from list and deallocate resources if the mapping
//  * isn't in use.
//  *
//  * \param inode device inode.
//  * \param file_priv DRM file private.
//  * \param cmd command.
//  * \param arg pointer to a struct drm_map structure.
//  * \return zero on success or a negative value on error.
//  *
//  * Searches the map on drm_device::maplist, removes it from the list, see if
//  * its being used, and free any associate resource (such as MTRR's) if it's not
//  * being on use.
//  *
//  * \sa drm_addmap
//  */
// int drm_rmmap_locked(struct drm_device *dev, drm_local_map_t *map)
// {
//      struct drm_map_list *r_list = NULL, *list_t;
//      int found = 0;
// 
//     /* Find the list entry for the map and remove it */
// //    list_for_each_entry_safe(r_list, list_t, &dev->maplist, head) {
//     for (r_list = (struct drm_map_list *)dev->maplist.next; 
//     r_list != (struct drm_map_list *)&dev->maplist; 
//     r_list = (struct drm_map_list *)r_list->head.next){
//         if (r_list->map == map) {
//             list_del(&r_list->head);
// //            drm_ht_remove_key(&dev->map_hash,
// //                      r_list->user_token >> PAGE_SHIFT);
//             drm_free(r_list, sizeof(*r_list), DRM_MEM_MAPS);
//             found = 1;
//             break;
//         }
//     }
// 
//     if (!found)
//         return -EINVAL;
// 
//     /* List has wrapped around to the head pointer, or it's empty and we
//      * didn't find anything.
//      */
// 
//     switch (map->type) {
//     case _DRM_REGISTERS:
//         drm_aros_pci_iounmap(dev->pcidriver, map->handle, map->size);
//         /* FALLTHROUGH */
//     case _DRM_FRAME_BUFFER:
//         if (drm_core_has_MTRR(dev) && map->mtrr >= 0) {
//             int retcode;
//             retcode = mtrr_del(map->mtrr, map->offset, map->size);
//             DRM_DEBUG("mtrr_del=%d\n", retcode);
//         }
//         break;
//     case _DRM_SHM:
//         DRM_IMPL("handle _DRM_SHM\n");
//         break;
//     case _DRM_AGP:
//     case _DRM_SCATTER_GATHER:
//         break;
//     case _DRM_CONSISTENT:
//         DRM_IMPL("handle _DRM_CONSISTENT\n");
//         break;
//     case _DRM_TTM:
//         DRM_IMPL("handle _DRM_TTM\n");
//         break;
//     }
//     drm_free(map, sizeof(*map), DRM_MEM_MAPS);
// 
//     return 0;
// }
// 
// int drm_rmmap(struct drm_device *dev, drm_local_map_t *map)
// {
//     int ret;
// 
//     ObtainSemaphore(&dev->struct_semaphore);
//     ret = drm_rmmap_locked(dev, map);
//     ReleaseSemaphore(&dev->struct_semaphore);
// 
//     return ret;
// }

int drm_order(unsigned long size)
{
    int order;
    unsigned long tmp;

    for (order = 0, tmp = size >> 1; tmp; tmp >>= 1, order++) ;

    if (size & (size - 1))
        ++order;

    return order;
}

// struct drm_map_list *drm_find_matching_map(struct drm_device *dev, drm_local_map_t *map)
// {
//     struct drm_map_list *entry;
// //     list_for_each_entry(entry, &dev->maplist, head) {
//     for (entry = (struct drm_map_list *)dev->maplist.next; 
//     entry != (struct drm_map_list *)&dev->maplist; 
//     entry = (struct drm_map_list *)entry->head.next){
//         if (entry->map && map->type == entry->map->type &&
//             ((entry->map->offset == map->offset) ||
//              (map->type == _DRM_SHM && map->flags==_DRM_CONTAINS_LOCK))) {
//             return entry;
//         }
//     }
// 
//     return NULL;
// }
