#include <clib/alib_protos.h>
#define CallHook(MyHook, MyObject, tags...) \
	({ULONG _tags[] = { tags }; CallHookA((MyHook), (MyObject), (APTR)_tags);})
