#pragma once
#include <exec/types.h>
struct NotifyRequest { STRPTR nr_Name; char pad[60]; };
#define EndNotify(nr) ((void)0)
