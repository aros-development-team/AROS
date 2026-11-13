/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/symbolsets.h>
#include <hidd/vulkan_hidd.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "lavapipe_intern.h"

static int HiddLavapipe_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.UtilityBase)
        CloseLibrary(LIBBASE->sd.UtilityBase);

    if (LIBBASE->sd.hiddVulkanAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Vulkan);

    return TRUE;
}

static int HiddLavapipe_InitLib(LIBBASETYPEPTR LIBBASE)
{
    if ((LIBBASE->sd.UtilityBase = OpenLibrary((STRPTR)"utility.library",0)))
    {
        if ((LIBBASE->sd.hiddVulkanAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Vulkan)))
        {
            Lavapipe_ForceLLVMPipeRTTI();
            return TRUE;
        }
        CloseLibrary(LIBBASE->sd.UtilityBase);
    }
    return FALSE;
}

ADD2INITLIB(HiddLavapipe_InitLib, 0)
ADD2EXPUNGELIB(HiddLavapipe_ExpungeLib, 0)

ADD2LIBS((STRPTR)"vulkan.hidd", 8, static struct Library *, VulkanHiddBase);
