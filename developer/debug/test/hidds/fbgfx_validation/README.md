<!--
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
-->

# Linear framebuffer validation regression

The test includes the same `fbgfx_validation.h` that the production driver
uses. It checks buffer geometry, the 32-bit allocation limit, pointer spans
and palette intervals against separate wide-arithmetic expectations. Invalid
geometry must leave the output size unchanged. No source extraction, display,
framebuffer mapping or hardware access is required.

The native target is part of the ordinary `test` and `test-quick` dependencies:

```sh
make test-hidd-fbgfx-validation
```

Run `hidds/gfx/fbgfx_validation` from the test installation, either from Shell
or with `Run`. Success returns 0; failed assertions return 10.

For a supplemental host check, from the source root:

```sh
cc -std=c99 -Wall -Wextra -Werror \
  -Iarch/aarch64-raspi/hidd/fbgfx \
  developer/debug/test/hidds/fbgfx_validation/fbgfx_validation.c \
  -o /tmp/fbgfx_validation
/tmp/fbgfx_validation
```

Add `-fsanitize=address,undefined -fno-omit-frame-pointer` for a sanitizer run.
The same C source builds for AROS; its extra attribute-width checks are
conditional on the pointer width. The 64-bit configuration performs 8,634
checks. Removing the allocation cap, pointer span guard or wide palette end
calculation separately produces failures.

These tests cover arithmetic used by the driver. They do not exercise OOP
constructor disposal, boot-console detachment, physical framebuffer writes,
compositor operation, scheduling or screen compatibility. A driver build is
also distinct from executing that driver in AROS.
