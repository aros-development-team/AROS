/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Opens libraries for use by linklibs within kernel.
*/

#include <proto/exec.h>
#include <proto/aros.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/partition.h>
#include <proto/utility.h>
#include <proto/workbench.h>

void InitKernelBases(void)
{
//    ArosBase = OpenLibrary("aros.library", 0);
//    CyberGfxBase = OpenLibrary("cybergraphics.library", 0);
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
//    ExpansionBase = OpenLibrary("expansion.library", 0);
//    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0);
//    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0);
//    KeymapBase = OpenLibrary("keymap.library", 0);
//    LayersBase = OpenLibrary("layers.library", 0);
//    PartitionBase = OpenLibrary("partition.library", 0);
//    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 0);
//    WorkbenchBase = OpenLibrary("workbench.library", 0);
}

