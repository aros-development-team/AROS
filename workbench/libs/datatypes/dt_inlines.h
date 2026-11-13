
#define setattrs(object, ...) \
({ \
    const IPTR setattrs_args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };\
    SetAttrsA(object, (struct TagItem *)(setattrs_args)); \
})

#define Do_OM_NOTIFY(object,  ginfo,  flags, ...) \
({ \
    const IPTR doomnotify_args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };\
    struct opUpdate doomnotifymsg; \
    doomnotifymsg.MethodID = OM_NOTIFY; \
    doomnotifymsg.opu_AttrList = (struct TagItem *)&doomnotify_args; \
    doomnotifymsg.opu_GInfo = ginfo; \
    doomnotifymsg.opu_Flags = flags; \
    DoMethodA(object, (Msg)(&doomnotifymsg)); \
})

#define DoGad_OM_NOTIFY(object, win, req, flags, ...) \
({ \
    const IPTR dogomnotify_args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };\
    struct opUpdate dogomnotifymsg; \
    dogomnotifymsg.MethodID = OM_NOTIFY; \
    dogomnotifymsg.opu_AttrList = (struct TagItem *)&dogomnotify_args; \
    dogomnotifymsg.opu_GInfo = NULL; \
    dogomnotifymsg.opu_Flags = flags; \
    DoGadgetMethodA((struct Gadget*)object, win, req, (Msg)(&dogomnotifymsg)); \
})

#define opencatalog(locale, name, ...) \
({ \
    const IPTR opencat_args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };\
    OpenCatalogA(locale, name, (struct TagItem *)(opencat_args)); \
})
