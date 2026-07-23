/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Display driver / view handling prototypes.
*/

/* Driver prototypes */

typedef ULONG (*VIEW_FUNC)(struct monitor_displaydata *mdd, struct HIDD_ViewPortData *vpd, struct View *v, struct GfxBase *GfxBase);
extern ULONG DoViewFunction(struct View *view, VIEW_FUNC fn, struct GfxBase *GfxBase);
extern BOOL driver_Setup(struct gfxdriver_data *cfg, struct TagItem *attrs, BOOL force, struct GfxBase * GfxBase);
extern void driver_Queue(struct gfxboot_entry *boote, struct GfxBase * GfxBase);
extern void driver_ReplayBootQueue(struct GfxBase *GfxBase);
extern int driver_init(struct GfxBase * GfxBase);
extern void driver_expunge(struct GfxBase * GfxBase);
