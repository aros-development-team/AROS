#include <grub/crypto.h>
extern void grub_gcry_arcfour_init (void);
extern void grub_gcry_arcfour_fini (void);
extern void grub_gcry_blowfish_init (void);
extern void grub_gcry_blowfish_fini (void);
extern void grub_gcry_camellia_init (void);
extern void grub_gcry_camellia_fini (void);
extern void grub_gcry_cast5_init (void);
extern void grub_gcry_cast5_fini (void);
extern void grub_gcry_crc_init (void);
extern void grub_gcry_crc_fini (void);
extern void grub_gcry_des_init (void);
extern void grub_gcry_des_fini (void);
extern void grub_gcry_rfc2268_init (void);
extern void grub_gcry_rfc2268_fini (void);
extern void grub_gcry_seed_init (void);
extern void grub_gcry_seed_fini (void);
extern void grub_gcry_sha1_init (void);
extern void grub_gcry_sha1_fini (void);
extern void grub_gcry_sha256_init (void);
extern void grub_gcry_sha256_fini (void);
extern void grub_gcry_sha512_init (void);
extern void grub_gcry_sha512_fini (void);
extern void grub_gcry_tiger_init (void);
extern void grub_gcry_tiger_fini (void);
extern void grub_gcry_twofish_init (void);
extern void grub_gcry_twofish_fini (void);
extern void grub_gcry_whirlpool_init (void);
extern void grub_gcry_whirlpool_fini (void);
extern void grub_gcry_md4_init (void);
extern void grub_gcry_md4_fini (void);
extern void grub_gcry_md5_init (void);
extern void grub_gcry_md5_fini (void);
extern void grub_gcry_rmd160_init (void);
extern void grub_gcry_rmd160_fini (void);
extern void grub_gcry_rijndael_init (void);
extern void grub_gcry_rijndael_fini (void);
extern void grub_gcry_serpent_init (void);
extern void grub_gcry_serpent_fini (void);

void
grub_gcry_init_all (void)
{
  grub_gcry_arcfour_init ();
  grub_gcry_blowfish_init ();
  grub_gcry_camellia_init ();
  grub_gcry_cast5_init ();
  grub_gcry_crc_init ();
  grub_gcry_des_init ();
  grub_gcry_rfc2268_init ();
  grub_gcry_seed_init ();
  grub_gcry_sha1_init ();
  grub_gcry_sha256_init ();
  grub_gcry_sha512_init ();
  grub_gcry_tiger_init ();
  grub_gcry_twofish_init ();
  grub_gcry_whirlpool_init ();
  grub_gcry_md4_init ();
  grub_gcry_md5_init ();
  grub_gcry_rmd160_init ();
  grub_gcry_rijndael_init ();
  grub_gcry_serpent_init ();
}

void
grub_gcry_fini_all (void)
{
  grub_gcry_arcfour_fini ();
  grub_gcry_blowfish_fini ();
  grub_gcry_camellia_fini ();
  grub_gcry_cast5_fini ();
  grub_gcry_crc_fini ();
  grub_gcry_des_fini ();
  grub_gcry_rfc2268_fini ();
  grub_gcry_seed_fini ();
  grub_gcry_sha1_fini ();
  grub_gcry_sha256_fini ();
  grub_gcry_sha512_fini ();
  grub_gcry_tiger_fini ();
  grub_gcry_twofish_fini ();
  grub_gcry_whirlpool_fini ();
  grub_gcry_md4_fini ();
  grub_gcry_md5_fini ();
  grub_gcry_rmd160_fini ();
  grub_gcry_rijndael_fini ();
  grub_gcry_serpent_fini ();
}
