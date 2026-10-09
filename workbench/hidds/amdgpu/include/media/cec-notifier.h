/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _MEDIA_CEC_NOTIFIER_H_
#define _MEDIA_CEC_NOTIFIER_H_

#include <linux/types.h>

struct cec_notifier;
struct device;
struct edid;

static inline struct cec_notifier *cec_notifier_conn_register(struct device *hdmi_dev, const char *port_name, const void *conn_info) { return NULL; }
static inline void cec_notifier_conn_unregister(struct cec_notifier *n) { }
static inline void cec_notifier_set_phys_addr(struct cec_notifier *n, u16 pa) { }
static inline void cec_notifier_set_phys_addr_from_edid(struct cec_notifier *n, const struct edid *edid) { }
static inline void cec_notifier_phys_addr_invalidate(struct cec_notifier *n) { }

struct drm_connector;

struct cec_connector_info {
    u32 type;
    union {
        struct { u32 card_no; u32 connector_id; } drm;
        u32 raw[16];
    };
};

static inline void cec_fill_conn_info_from_drm(struct cec_connector_info *conn_info, const struct drm_connector *connector)
{
    memset(conn_info, 0, sizeof(*conn_info));
}

#endif
