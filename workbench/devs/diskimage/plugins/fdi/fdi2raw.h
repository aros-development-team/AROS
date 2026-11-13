#ifndef FDI2RAW_H
#define FDI2RAW_H

#include <stdint.h>

struct fdi;
typedef struct fdi FDI;

typedef uint8_t uae_u8;
typedef uint16_t uae_u16;
typedef uint32_t uae_u32;

void fdi2raw_header_free (FDI *fdi);
int fdi2raw_get_last_track (FDI	*fdi);
int fdi2raw_get_num_sector (FDI	*fdi);
unsigned int fdi2raw_get_last_head (FDI *fdi);
FDI *fdi2raw_header(BPTR f);
int fdi2raw_loadtrack (FDI *fdi, uae_u16 *mfmbuf, uae_u16 *tracktiming,
	unsigned int track, unsigned int *tracklength, unsigned int *indexoffsetp,
	int *multirev, int mfm);

#endif
