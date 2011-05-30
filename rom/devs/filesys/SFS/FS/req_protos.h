#ifndef __AROS__
#define AROS_PP_VARIADIC_CAST2IPTR(x) x
#endif

LONG requestArgs(UBYTE *title, UBYTE *fmt, UBYTE *gads, APTR params);
LONG reqArgs(UBYTE *fmt, UBYTE *gads, APTR params);
LONG req_unusualArgs(UBYTE *fmt, APTR params);
void dreqArgs(UBYTE *fmt, APTR params);

#define request(title, fmt, gads, ...)					\
({									\
    IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };	\
    requestArgs(title, fmt, gads, __args);				\
})

#define req(fmt, gads, ...)						\
({									\
    IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };	\
    reqArgs(fmt, gads, __args);						\
})

#define req_unusual(fmt, ...)						\
({									\
    IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };	\
    req_unusualArgs(fmt, __args);					\
})

#define dreq(fmt, ...)							\
({									\
    IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };	\
    dreqArgs(fmt, __args);						\
})
