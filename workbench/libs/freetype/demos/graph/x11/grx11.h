/*******************************************************************
 *
 *  grx11.h  graphics driver for X11 (header)
 *
 *  This is the driver for displaying inside a window under X11,
 *  used by the graphics utility of the FreeType test suite.
 *
 *  Copyright 1999-2000, 2001, 2002 by Antoine Leca, David Turner
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT. By continuing to use, modify or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

#ifndef GRX11_H
#define GRX11_H

#ifdef __cplusplus
#define class  c_class
#endif

#include "grobjs.h"
#include "grdevice.h"

  extern
  grDevice  gr_x11_device;

#ifdef GR_INIT_BUILD
  static
  grDeviceChain  gr_x11_device_chain =
  {
    "x11",
    &gr_x11_device,
    GR_INIT_DEVICE_CHAIN
  };

#undef GR_INIT_DEVICE_CHAIN
#define GR_INIT_DEVICE_CHAIN  &gr_x11_device_chain

#endif  /* GR_INIT_BUILD */

#endif /* GRX11_H */
