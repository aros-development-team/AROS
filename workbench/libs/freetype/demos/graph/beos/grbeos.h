#ifndef GRBEOS_H
#define GRBEOS_H

#include "grobjs.h"

  extern
  grDevice  gr_beos_device;

#ifdef GR_INIT_BUILD
  static
  grDeviceChain  gr_beos_device_chain =
  {
    "beos",
    &gr_beos_device,
    GR_INIT_DEVICE_CHAIN
  };

#undef GR_INIT_DEVICE_CHAIN
#define GR_INIT_DEVICE_CHAIN  &gr_beos_device_chain

#endif  /* GR_INIT_BUILD */

#endif /* GRBEOS_H */

