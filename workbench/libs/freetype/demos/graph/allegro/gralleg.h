#ifndef GRALLEG_H
#define GRALLEG_H

#include "grobjs.h"

  extern
  grDevice  gr_alleg_device;

#ifdef GR_INIT_BUILD
  static
  grDeviceChain  gr_alleg_device_chain =
  {
    "Allegro",
    &gr_alleg_device,
    GR_INIT_DEVICE_CHAIN
  };

#undef GR_INIT_DEVICE_CHAIN
#define GR_INIT_DEVICE_CHAIN  &gr_alleg_device_chain

#endif  /* GR_INIT_BUILD */

#endif /* GRALLEG_H */
