#pragma once
#include <exec/types.h>
struct Library { char pad[20]; UWORD lib_Version, lib_Revision; char pad2[10]; };
#define NT_LIBRARY 9
