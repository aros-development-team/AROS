/*******************************************************************
 *
 *  grwin32.h  graphics driver for Win32 platform (header)
 *
 *  This is the driver for displaying inside a window under Win32,
 *  used by the graphics utility of the FreeType test suite.
 *
 *  Written by Antoine Leca.
 *  Copyright 1999-2000, 2001, 2002 by Antoine Leca, David Turner
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  Borrowing liberally from the other FreeType drivers.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT. By continuing to use, modify or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

#ifndef GRWIN32_H
#define GRWIN32_H

#include "grobjs.h"

  extern
  grDevice  gr_win32_device;

#ifdef GR_INIT_BUILD
  static
  grDeviceChain  gr_win32_device_chain =
  {
    "win32",
    &gr_win32_device,
    GR_INIT_DEVICE_CHAIN
  };

#undef GR_INIT_DEVICE_CHAIN
#define GR_INIT_DEVICE_CHAIN  &gr_win32_device_chain

#endif  /* GR_INIT_BUILD */

#endif /* GRWIN32_H */
