#define	IMAGEMAX	6

#include "Toaster1.h"
#include "Toaster2.h"
#include "Toaster3.h"
#include "Toaster4.h"

struct Image *img[IMAGEMAX] = { &im0, &im1, &im2, &im3,  &im2, &im1, };

struct sequence {
	struct	sequence *next_sequence;
	int	img_count;
	struct	Image **img;
};

struct	sequence default_seq = { &default_seq, IMAGEMAX, img };
