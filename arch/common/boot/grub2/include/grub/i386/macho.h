#define GRUB_MACHO_CPUTYPE_IS_HOST32(x) ((x)==0x00000007)
#define GRUB_MACHO_CPUTYPE_IS_HOST64(x) ((x)==0x01000007)

struct grub_macho_thread32
{
  grub_uint32_t cmd;
  grub_uint32_t cmdsize;
  grub_uint8_t unknown1[48];
  grub_uint32_t entry_point;
  grub_uint8_t unknown2[20];
} __attribute__ ((packed));
