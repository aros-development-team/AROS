#include <config.h>
#include <grub/emu/misc.h>

#ifndef GRUB_MACHINE_EMU
#error "This source is only meant for grub-emu platform"
#endif

#if defined(__i386__)
#include "../i386/dl.c"
#elif defined(__x86_64__)
#include "../x86_64/dl.c"
#elif defined(__sparc__)
#include "../sparc64/dl.c"
#elif defined(__mips__)
#include "../mips/dl.c"
#elif defined(__powerpc__)
#include "../powerpc/dl.c"
#elif defined(__ia64__)
#include "../ia64/dl.c"
#else
#error "No target cpu type is defined"
#endif

/* grub-emu-lite supports dynamic module loading, so it won't have any
   embedded modules.  */
void
grub_init_all (void)
{
  return;
}

void
grub_fini_all (void)
{
  return;
}

void
grub_emu_init (void)
{
  return;
}

void
grub_emu_post_init (void)
{
}
