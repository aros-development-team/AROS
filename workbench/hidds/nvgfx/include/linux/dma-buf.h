/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_DMA_BUF_H_
#define _LINUX_DMA_BUF_H_

#include <linux/types.h>
#include <linux/list.h>
#include <linux/dma-resv.h>
#include <linux/scatterlist.h>
#include <linux/dma-direction.h>
#include <linux/iosys-map.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>

/* buffer sharing across drivers does not exist here; the types are kept
   so that the gem/ttm structures keep their shape */
struct dma_buf_attachment;
struct dma_buf_ops {
    bool cache_sgt_mapping;
    int (*attach)(struct dma_buf *, struct dma_buf_attachment *);
    void (*detach)(struct dma_buf *, struct dma_buf_attachment *);
    int (*pin)(struct dma_buf_attachment *attach);
    void (*unpin)(struct dma_buf_attachment *attach);
    struct sg_table *(*map_dma_buf)(struct dma_buf_attachment *, enum dma_data_direction);
    void (*unmap_dma_buf)(struct dma_buf_attachment *, struct sg_table *, enum dma_data_direction);
    void (*release)(struct dma_buf *);
    int (*begin_cpu_access)(struct dma_buf *, enum dma_data_direction);
    int (*end_cpu_access)(struct dma_buf *, enum dma_data_direction);
    int (*mmap)(struct dma_buf *, struct vm_area_struct *vma);
    int (*vmap)(struct dma_buf *dmabuf, struct iosys_map *map);
    void (*vunmap)(struct dma_buf *dmabuf, struct iosys_map *map);
};
struct dma_buf {
    size_t size;
    struct file *file;
    const struct dma_buf_ops *ops;
    void *priv;
    struct dma_resv *resv;
    struct module *owner;
    const char *exp_name;
    struct list_head attachments;
    struct list_head list_node;
};
struct dma_buf_attachment {
    struct dma_buf *dmabuf;
    struct device *dev;
    struct list_head node;
    struct sg_table *sgt;
    enum dma_data_direction dir;
    bool peer2peer;
    const struct dma_buf_attach_ops *importer_ops;
    void *importer_priv;
    void *priv;
};
struct dma_buf_attach_ops {
    bool allow_peer2peer;
    void (*move_notify)(struct dma_buf_attachment *attach);
};
struct dma_buf_export_info {
    const char *exp_name;
    struct module *owner;
    const struct dma_buf_ops *ops;
    size_t size;
    int flags;
    struct dma_resv *resv;
    void *priv;
};
#define DEFINE_DMA_BUF_EXPORT_INFO(name) struct dma_buf_export_info name = { .exp_name = "nouveau", .owner = NULL }
static inline struct dma_buf *dma_buf_export(const struct dma_buf_export_info *e) { return ERR_PTR(-ENOSYS); }
static inline void dma_buf_put(struct dma_buf *dmabuf) { }
static inline void get_dma_buf(struct dma_buf *dmabuf) { }
static inline int dma_buf_fd(struct dma_buf *dmabuf, int flags) { return -ENOSYS; }
static inline struct dma_buf *dma_buf_get(int fd) { return ERR_PTR(-ENOSYS); }
static inline struct dma_buf_attachment *dma_buf_attach(struct dma_buf *d, struct device *dev) { return ERR_PTR(-ENOSYS); }
static inline struct dma_buf_attachment *dma_buf_dynamic_attach(struct dma_buf *d, struct device *dev, const struct dma_buf_attach_ops *o, void *p) { return ERR_PTR(-ENOSYS); }
static inline void dma_buf_detach(struct dma_buf *d, struct dma_buf_attachment *a) { }
static inline struct sg_table *dma_buf_map_attachment(struct dma_buf_attachment *a, enum dma_data_direction d) { return ERR_PTR(-ENOSYS); }
static inline void dma_buf_unmap_attachment(struct dma_buf_attachment *a, struct sg_table *s, enum dma_data_direction d) { }
static inline struct sg_table *dma_buf_map_attachment_unlocked(struct dma_buf_attachment *a, enum dma_data_direction d) { return ERR_PTR(-ENOSYS); }
static inline void dma_buf_unmap_attachment_unlocked(struct dma_buf_attachment *a, struct sg_table *s, enum dma_data_direction d) { }
static inline int dma_buf_pin(struct dma_buf_attachment *a) { return -ENOSYS; }
static inline void dma_buf_unpin(struct dma_buf_attachment *a) { }
static inline int dma_buf_vmap(struct dma_buf *d, struct iosys_map *m) { return -ENOSYS; }
static inline void dma_buf_vunmap(struct dma_buf *d, struct iosys_map *m) { }
static inline int dma_buf_vmap_unlocked(struct dma_buf *d, struct iosys_map *m) { return -ENOSYS; }
static inline void dma_buf_vunmap_unlocked(struct dma_buf *d, struct iosys_map *m) { }
static inline int dma_buf_mmap(struct dma_buf *d, struct vm_area_struct *v, unsigned long p) { return -ENOSYS; }
static inline void dma_buf_move_notify(struct dma_buf *d) { }
static inline bool dma_buf_is_dynamic(struct dma_buf *d) { return false; }
static inline int dma_buf_begin_cpu_access(struct dma_buf *d, enum dma_data_direction dir) { return 0; }
static inline int dma_buf_end_cpu_access(struct dma_buf *d, enum dma_data_direction dir) { return 0; }

#endif /* _LINUX_DMA_BUF_H_ */
