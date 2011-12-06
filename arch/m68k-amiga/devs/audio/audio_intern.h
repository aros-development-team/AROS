#ifndef AUDIO_DEVICE_H
#define AUDIO_DEVICE_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <devices/audio.h>

#define NR_CH 4
#define CH_MASK ((1 << NR_CH) - 1)

struct AudioInterrupt
{
	struct Interrupt audint;
	UWORD ch;
	struct AudioBase *ab;
};

struct AudioBase
{
    struct Device td_device;
    struct AudioInterrupt audint[NR_CH];
    UWORD *zerosample;
    WORD key[NR_CH];
    BYTE  pri[NR_CH];
    WORD keygen;
    struct MinList writelist[NR_CH];
    struct MinList misclist;
    UWORD cycles[NR_CH];
    UWORD initialcyclemask;
    UWORD initialdmamask;
    UWORD stopmask;
};

extern void audiohw_init(struct AudioBase *ab);
extern void audiohw_reset(struct AudioBase *ab, UWORD mask);
extern void audiohw_start(struct AudioBase *ab, UWORD mask);
extern void audiohw_stop(struct AudioBase *ab, UWORD mask);
extern void audiohw_prepareptlen(struct AudioBase *ab, struct IOAudio *io, UBYTE ch);
extern void audiohw_preparepervol(struct AudioBase *ab, struct IOAudio *io, UBYTE ch);

extern struct IOAudio *getnextwrite(struct AudioBase *ab, UBYTE ch, BOOL second);


#endif
