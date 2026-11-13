#ifdef __MORPHOS__
#define CGXSHOWHIDESUPPORT

#define CGXSYSTEM_BASE_NAME CGXSystemBase

#define CGXShowWindow(Window_) \
LP1(0x66, ULONG, CGXShowWindow, struct Window *, Window_ , a0, \
, CGXSYSTEM_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)
#define CGXHideWindow(Window_) \
LP1(0x60, ULONG, CGXHideWindow, struct Window *, Window_ , a0, \
, CGXSYSTEM_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)
#define IsWindowHidden(Window_) \
LP1(0x6C, ULONG, IsWindowHidden, struct Window *, Window_ , a0, \
, CGXSYSTEM_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)
#endif
