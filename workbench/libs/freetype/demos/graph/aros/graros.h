#ifndef GRAROS_H
#define GRAROS_H

#include "grobjs.h"
#include "grdevice.h"

  extern
  grDevice  gr_aros_device;

#ifdef GR_INIT_BUILD
  static
  grDeviceChain  gr_aros_device_chain =
  {
    "aros",
    &gr_aros_device,
    GR_INIT_DEVICE_CHAIN
  };

#undef GR_INIT_DEVICE_CHAIN
#define GR_INIT_DEVICE_CHAIN  &gr_aros_device_chain

#endif  /* GR_INIT_BUILD */

#endif /* GRAROS_H */
