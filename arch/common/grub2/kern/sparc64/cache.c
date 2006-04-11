#include <grub/types.h>

void grub_arch_sync_caches(void *address __attribute__((unused)),
                           grub_size_t len __attribute__((unused)));
void grub_arch_sync_caches(void *address __attribute__((unused)),
                           grub_size_t len __attribute__((unused)))
{
  return;
}
