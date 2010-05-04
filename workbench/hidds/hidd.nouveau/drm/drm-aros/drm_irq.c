/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drmP.h"
#include "drm_aros.h"

#include <proto/oop.h>
#include <hidd/pci.h>

#if !defined(HOSTED_BUILD)
static void interrupt_handler(HIDDT_IRQ_Handler * irq, HIDDT_IRQ_HwInfo *hw)
{
    struct drm_device *dev = (struct drm_device*)irq->h_Data;
    
    /* FIXME: What if INT is shared between devices? */
    if (dev->driver->irq_handler)
        dev->driver->irq_handler(dev);
}
#endif

int drm_irq_install(struct drm_device *dev)
{
#if defined(HOSTED_BUILD)
    return 0;
#else    
    struct OOP_Object *o = NULL;
    IPTR INTLine = 0;
    int retval = -EINVAL;
    
    ObtainSemaphore(&dev->struct_mutex.semaphore);
    if (dev->irq_enabled) {
        return -EBUSY;
    }
    dev->irq_enabled = 1;
    ReleaseSemaphore(&dev->struct_mutex.semaphore);
    
    if (dev->driver->irq_preinstall)
        dev->driver->irq_preinstall(dev);

    dev->IntHandler = (HIDDT_IRQ_Handler *)AllocVec(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC | MEMF_CLEAR);
    
    if (dev->IntHandler)
    {
        dev->IntHandler->h_Node.ln_Pri = 10;
        dev->IntHandler->h_Node.ln_Name = "Gallium3D INT Handler";
        dev->IntHandler->h_Code = interrupt_handler;
        dev->IntHandler->h_Data = dev;

        OOP_GetAttr(dev->pdev, aHidd_PCIDevice_INTLine, &INTLine);
        DRM_DEBUG("INTLine: %d\n", INTLine);
        
        o = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
        
        if (o)
        {
            struct pHidd_IRQ_AddHandler __msg__ = {
                mID:            OOP_GetMethodID(IID_Hidd_IRQ, moHidd_IRQ_AddHandler),
                handlerinfo:    dev->IntHandler,
                id:             INTLine,
            }, *msg = &__msg__;

            if (OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg))
                retval = 0;

            OOP_DisposeObject((OOP_Object *)o);
        }
    }

    if (retval != 0)
    {
        ObtainSemaphore(&dev->struct_mutex.semaphore);
        dev->irq_enabled = 0;
        ReleaseSemaphore(&dev->struct_mutex.semaphore);
        return retval;
    }
    
    if (dev->driver->irq_postinstall)
    {
        retval = dev->driver->irq_postinstall(dev);
        if (retval < 0)
        {
            ObtainSemaphore(&dev->struct_mutex.semaphore);
            dev->irq_enabled = 0;
            ReleaseSemaphore(&dev->struct_mutex.semaphore);            
        }
    }
    
    return retval;
#endif    
}

int drm_irq_uninstall(struct drm_device *dev)
{
#if defined(HOSTED_BUILD)
    return 0;
#else      
    int irq_enabled;
    struct OOP_Object *o = NULL;
    int retval = -EINVAL;

    ObtainSemaphore(&dev->struct_mutex.semaphore);
    irq_enabled = dev->irq_enabled;
    dev->irq_enabled = 0;
    ReleaseSemaphore(&dev->struct_mutex.semaphore);

    if (!irq_enabled)
        return retval;

    if (dev->driver->irq_uninstall)
        dev->driver->irq_uninstall(dev);

    o = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
    
    if (o)
    {
        struct pHidd_IRQ_RemHandler __msg__ = {
            mID:            OOP_GetMethodID(IID_Hidd_IRQ, moHidd_IRQ_RemHandler),
            handlerinfo:    dev->IntHandler,
        }, *msg = &__msg__;

        if (OOP_DoMethod((OOP_Object *)o, (OOP_Msg)msg))
        {
            FreeVec(dev->IntHandler);
            dev->IntHandler = NULL;
            retval = 0;
        }

        OOP_DisposeObject((OOP_Object *)o);
    }

    return retval;
#endif
}
