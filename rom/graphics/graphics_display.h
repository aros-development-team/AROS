
#define DEBUG_LOADVIEW(x)

/* display functions */
extern struct gfxdisplay_data *display_Setup(OOP_Object *gfxhidd, struct GfxBase *GfxBase);
extern void display_Register(OOP_Object *display, struct gfxdriver_data *cfg, BOOL force, struct GfxBase *GfxBase);
extern void display_Queue(struct gfxdisplay_data *mdd, struct gfxdisplay_data *last, struct GfxBase *GfxBase);
extern void display_Expunge(struct monitor_displaydata *mdd, struct GfxBase *GfxBase);
extern void display_Enable(struct gfxdisplay_data *mdd, struct GfxBase *GfxBase);

/* framebuffer functions */
extern OOP_Object *display_FBCreate(struct monitor_displaydata *mdd, struct GfxBase *GfxBase);
extern void display_FBInstall(struct monitor_displaydata *mdd, struct GfxBase *GfxBase);
extern void display_FBUninstall(struct monitor_displaydata *mdd, struct GfxBase *GfxBase);

/* viewport functions */
extern struct HIDD_ViewPortData *display_FindViewPorts(struct monitor_displaydata *mdd, struct View *view, struct GfxBase *GfxBase);
extern ULONG display_PrepareViewPorts(struct monitor_displaydata *mdd, struct HIDD_ViewPortData *vpd, struct View *v, struct GfxBase *GfxBase);
extern ULONG display_LoadViewPorts(struct monitor_displaydata *mdd, struct HIDD_ViewPortData *vpd, struct View *v, struct GfxBase *GfxBase);
