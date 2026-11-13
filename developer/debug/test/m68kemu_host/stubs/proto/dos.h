#pragma once
#include <exec/libraries.h>
struct DosLibrary { struct Library lib; };
#include <utility/tagitem.h>
#define DOS_FIB 0
#define GetSegListInfo(seg, tags) 0
#define MatchEnd(ap) ((void)0)
#define FreeDosObject(type, ptr) ((void)0)
