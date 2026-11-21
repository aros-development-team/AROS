#pragma once
#include <exec/types.h>
struct Task;
struct SignalSemaphore { char pad[48]; struct Task *ss_Owner; };
