#include <libraries/realtime.h>
#include <aros/symbolsets.h>

ADD2LIBS("realtime.library", 39, LIBSET_REALTIME_PRI, struct RealTimeBase *, RealTimeBase, NULL, NULL);
