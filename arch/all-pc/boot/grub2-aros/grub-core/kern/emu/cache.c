
#if defined(__ia64__)
#include <grub/cache.h>

void __clear_cache (char *beg, char *end);

void
grub_arch_sync_caches (void *address, grub_size_t len)
{
  __clear_cache (address, (char *) address + len);
}
#endif

