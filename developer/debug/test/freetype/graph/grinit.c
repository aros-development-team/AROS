#include "grobjs.h"
#include "grdevice.h"
#include <stdio.h>

#define GR_INIT_DEVICE_CHAIN   ((grDeviceChain*)0)
#define GR_INIT_BUILD

#ifdef DEVICE_X11
#ifndef VMS
#include "x11/grx11.h"
#else
#include "grx11.h"
#endif
#endif

#ifdef DEVICE_OS2_PM
#include "os2/gros2pm.h"
#endif

#ifdef DEVICE_WIN32
#include "win32/grwin32.h"
#endif

#ifdef macintosh
#include "mac/grmac.h"
#endif

#ifdef DEVICE_ALLEGRO
#include "allegro/gralleg.h"
#endif

#ifdef DEVICE_BEOS
#include "beos/grbeos.h"
#endif


 /**********************************************************************
  *
  * <Function>
  *    grInitDevices
  *
  * <Description>
  *    This function is in charge of initialising all system-specific
  *    devices. A device is responsible for creating and managing one
  *    or more "surfaces". A surface is either a window or a screen,
  *    depending on the system.
  *
  * <Return>
  *    a pointer to the first element of a device chain. The chain can
  *    be parsed to find the available devices on the current system
  *
  * <Note>
  *    If a device cannot be initialised correctly, it is not part of
  *    the device chain returned by this function. For example, if an
  *    X11 device was compiled in the library, it will be part of
  *    the returned device chain only if a connection to the display
  *    could be established
  *
  *    If no driver could be initialised, this function returns NULL.
  *
  **********************************************************************/

  extern
  grDeviceChain*  grInitDevices( void )
  {
    grDeviceChain*  chain = GR_INIT_DEVICE_CHAIN;
    grDeviceChain*  cur   = gr_device_chain;


    while (chain)
    {
      /* initialize the device */
      grDevice*  device;

      device = chain->device;
      if ( device->init() == 0             &&
           gr_num_devices < GR_MAX_DEVICES )

      {
        /* successful device initialisation - add it to our chain */
        cur->next   = 0;
        cur->device = device;
        cur->name   = device->device_name;

        if (cur > gr_device_chain)
          cur[-1].next = cur;

        cur++;
        gr_num_devices++;
      }
      chain = chain->next;
    }

    return (gr_num_devices > 0 ? gr_device_chain : 0 );
  }


  extern
  void  grDoneDevices( void )
  {
    int             i;
    grDeviceChain*  chain = gr_device_chain;


    for ( i = 0; i < gr_num_devices; i++ )
    {
      chain->device->done();

      chain->next   = 0;
      chain->device = 0;
      chain->name   = 0;

      chain++;
    }

    gr_num_devices = 0;
  }
