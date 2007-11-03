#include <math.h>

#include	<proto/exec.h>
#include	<proto/utility.h>

#include	"declgate.h"
#include	"LibHeader.h"
#include "ptplay_priv.h"

#define	SysBase		LibBase->MySysBase
#define	UtilityBase	LibBase->MyUtilBase

#ifdef	__MORPHOS__
#if 0
UWORD __dputch[5] = {0xCD4B, 0x4EAE, 0xFDFC, 0xCD4B, 0x4E75};
#define kprintf(fmt, tags...)	({ULONG _tags[] = { 0 , ## tags }; RawDoFmt(fmt, (APTR)&_tags[1], (void (*)(void)) __dputch, (APTR) *((APTR *)4));})
#else
# ifndef __VBCC__
#  define kprintf(fmt, tags...)
# else
#  define kprintf(fmt,tags)
# endif
#endif
#endif

/**************************************************************************
**
**	ULONG = pt_test(data, size)
**		Test if the mod data is protracker module.
**		of the required mod data structure. Otherwise, the return value is
**		boolean and indicates whether initialization succeeded.
*/

ULONG NATDECLFUNC_3(PtTest, a0, STRPTR, filename, a1, UBYTE *, buf, d0, LONG, bufsize)
{
	DECLARG_4(a0, STRPTR, filename, a1, UBYTE *, buf, d0, LONG, bufsize, a6, struct PtPlayLibrary *, LibBase)

	ULONG	result;

	result	= PT_MOD_UNKNOWN;

	if (buf != NULL)
	if (bufsize >= 1084)
	{
		ULONG	*tmp;

		/* M.K. signature */

		tmp		= (ULONG *)buf;
		result	= PT_MOD_PROTRACKER;

		if (tmp[ 1080/4 ] != MAKE_ID('M','.','K','.'))
		if (tmp[ 1080/4 ] != MAKE_ID('M','!','K','!'))
		if (tmp[ 1080/4 ] != MAKE_ID('F','L','T','4'))
		{
//			result	= PT_MOD_SOUNDFX;

			if (tmp[ 60/4 ] != MAKE_ID('S','O','N','G'))		/* SoundFX 1.3	*/
			{
				UBYTE	songlen;

				songlen	= buf[ 470 ];
				result	= PT_MOD_UNKNOWN;

				if (buf[ 470 ] > 0 && buf[ 470 ] < 129)		/* songlen	*/
				if (buf[ 471 ] > 0 && buf[ 471 ] < 128)		/* patterns	*/
				{
					LONG	i = songlen;

					do
					{
						if (buf[ 472 + i ] > songlen)
							goto pois;

						i--;
					}
					while (i >= 0);

					buf	+= 44;

					for (i = 0; i < 15; i++)
					{
						if ((buf[ 0 ] & 0x0f) != 0)
						{
							goto pois;
						}

						if (buf[ 1 ] > 64)
						{
							goto pois;
						}

						buf	+= 30;
					}

					result	= PT_MOD_SOUNDTRACKER;
				}
			}
		}
	}

pois:
	return result;
}

static inline VOID clr_l(ULONG *src, ULONG longs)
{
	do
	{
		*src++	= 0;
		longs--;
	}
	while (longs > 0);
}

/**********************************************************************
	Reset
**********************************************************************/

static VOID pt_reset(pt_mod_s *mod)
{
	mod->speed	= 6;

	if (mod->modformat == PT_MOD_SOUNDFX)
		mod->speed	= 5;

	mod->ciaspeed			= 125;
	mod->vbllen				= mod->freq / 50;
	mod->spos				= 0;
	mod->ppos				= 0;
	mod->bpos				= 0;
	mod->tick				= 0;
	mod->patdelay			= 0;
	mod->skiptopos			= 0;
	mod->filter				= FALSE;
	mod->songloopcount	= 0;
}

/**********************************************************************
	pt_getlen
**********************************************************************/

void pt_donote2(pt_mod_s *mod);
void pt_dotick(pt_mod_s *mod);

static ULONG pt_getlen(pt_mod_s *mod)
{
	ULONG	length, frames, freq, max;
	LONG	j;

	freq			= mod->freq;
	mod->freq	= PTV_TEST_FREQUENCY;

	pt_reset(mod);

	frames	= PTV_TEST_FREQUENCY;
	length	= 1;

	max		= 30 * 60 * PTV_TEST_FREQUENCY;	// max 30 mins!

	for (;;)
	{
		max--;

		if (mod->songloopcount || max == 0)
			break;

		/* vbllen = freq / 50	*/

		j = mod->bpos - mod->vbllen;	/* mod->bpos - mod->vbllen	*/

		if (j >= 0)
		{
			mod->bpos = j;

			if (mod->tick == 0)
				pt_donote2(mod);

			mod->tick--;
		}
	
		mod->bpos++;
		frames--;

		if (!frames)
		{
			length++;
			frames	= PTV_TEST_FREQUENCY;
		}
	}

	mod->freq	= freq;

	return length;
}

/**************************************************************************
	PtSeek
**************************************************************************/

ULONG NATDECLFUNC_2(PtSeek, a0, pt_mod_s *, mod, d0, ULONG, time)
{
	DECLARG_3(a0, pt_mod_s *, mod, d0, ULONG, time, a6, struct PtPlayLibrary *, LibBase)
	pt_channel_s	*c;
	ULONG	i, length, freq, frames;

	time	%= mod->PlayLength * 2;

	clr_l((ULONG *)&mod->chan[0], sizeof(pt_channel_s) / 4 * 4);

	for (i = 0; i < 4; i++)
		mod->chan[0].sp = -1;

	mod->chan[1].pan	= 255;
	mod->chan[2].pan	= 255;
	freq					= mod->freq;
	mod->freq			= PTV_TEST_FREQUENCY;

	pt_reset(mod);

	frames	= PTV_TEST_FREQUENCY;
	length	= 0;

	for (;;)
	{
		pt_sample_s		*s;
		LONG	j, k;

		if (length == time)
			break;

		j	= mod->bpos - mod->vbllen;
		c	= &mod->chan[0];

		if (j >= 0)
		{
			mod->bpos = j;
			pt_dotick(mod);
		}
	
		mod->bpos++;

		for (i = 0; i < 4; ++i)
		{
			j	= c->sp;

			if (j >= 0)
			{
				s	= &mod->sample[c->sample];
				k	= s->replen;

				if (k > 32768)
				{
					if (j >= s->repend)
					{
						j	-= k;
						k	 = c->delaysample;

						if (k)
							c->sample	= k;
					}
				}
				else if (j >= s->length)
				{
					c->sp	= -1;
					continue;
				}

				c->sp	= j + c->freq;
			}

			c++;
		}

		frames--;

		if (!frames)
		{
			length++;
			frames	= PTV_TEST_FREQUENCY;
		}
	}

	for (i = 0; i < 4; ++i)
	{
		c	= &mod->chan[i];
		c->freq	= c->per ? (1172874197 / (freq * c->per / 50)) : 0;
	}

	if (mod->ciaspeed)
		mod->vbllen	= freq * 60 / (24 * mod->ciaspeed);

	mod->freq	= freq;

	return time;
}

/**************************************************************************
**
**	ULONG = AllocMod(patterns, type)
*/

static pt_mod_s *AllocMod(ULONG patterns, ULONG type, ULONG freq, struct PtPlayLibrary *LibBase)
{
	pt_mod_s	*mod;

	mod	= AllocVecTaskPooled(sizeof(*mod) + patterns * sizeof(pt_pattern_s));

	if (mod != NULL)
	{
		DOUBLE	tempf;
		ULONG	i;

		clr_l((ULONG *)mod, sizeof(*mod) / 4);

#if 0
		clr_l((ULONG *)&mod->fltpi[0], 8);
		clr_l((ULONG *)&mod->chan[0], sizeof(pt_channel_s) / 4 * 4);
#endif

		mod->name[20]		= '\0';
		mod->modformat		= type;
		mod->numpat			= patterns;
		mod->pattern		= (pt_pattern_s *) (mod + 1);

		for (i = 0; i < 4; i++)
			mod->chan[0].sp = -1;

		mod->chan[1].pan	= 255;
		mod->chan[2].pan	= 255;

		mod->mastervolume	= 256;
		mod->freq			= freq * 2;

		/* init lowpass filter */

		mod->flags	= MODF_ALLOWFILTER;	// MODF_ALLOWPANNING;
		tempf = M_PI * 4000 / freq;
		tempf = atan(tempf);
		tempf = -(tempf-1)/(1+tempf);
		mod->fltb = (FLOAT) tempf;
		mod->flta = (FLOAT) ((1 - tempf) / 2);
	}

	return mod;
}

/**************************************************************************
**
**	ULONG = GetPattsAndPtrs(mod, bp, endptr, instruments)
*/

static VOID GetPattsAndPtrs(pt_mod_s *mod, UBYTE *bp, UBYTE *endptr, struct PtPlayLibrary *LibBase)
{
	ULONG	instruments, modtype;
	ULONG	i;

	modtype		= mod->modformat;
	instruments	= 32;

	if (modtype != PT_MOD_PROTRACKER)
		instruments	= 16;

	/* ptrs to sampledata */

	for (i = 1; i < instruments; ++i)
	{
		pt_sample_s *s = &mod->sample[i];

		s->data = bp;
		bp += s->length;

#ifdef	__MORPHOS__
//		if (bp - buf > bufsize)

		if (bp > endptr)
		{
			kprintf("Sample ptr past end of file\n");

			s->length	-= bp - endptr;
		}
#endif
		
		s->length <<= 14;			/* include oversampling precision */
		s->repeat <<= 14;
		s->replen <<= 14;
		s->repend = s->repeat + s->replen;
	}	
}

/**************************************************************************
**
**	APTR = PtInit(buf, bufsize)
*/

static pt_mod_s *PtInit(UBYTE *buf, ULONG bufsize, ULONG freq, struct PtPlayLibrary *LibBase)
{
	pt_mod_s	*mod;
	UBYTE *bp;
	ULONG	l;
	LONG i, j, k;

	/* determine number of patterns */

	mod	= NULL;
	k		= 0;	
	bp		= buf + 952;

	for (i = 0; i < 128; ++i)
	{
		j = *bp++;
		if (j > k) k = j;
	}
	
	k++;

	if (bufsize >= 1084 + (k << 8))
	{
		mod	= AllocMod(k, PT_MOD_PROTRACKER, freq, LibBase);

		if (mod != NULL)
		{
			for (i = 0; i < 20; ++i)
				mod->name[i] = buf[i];

			/* samples */

			bp = buf + 20;

			for (i = 1; i < 32; ++i)
			{
				pt_sample_s *s = &mod->sample[i];

				for (l = 0; l < 22; ++l)
					s->name[l]	= bp[l];

				j = bp[22];
				k = bp[23];
				s->length = ((j << 8) + k) << 1;
				j = bp[24];
				s->ft = j & 15;
				j = bp[25];
				s->volume = j;
				j = bp[26];
				k = bp[27];
				s->repeat = ((j << 8) + k) << 1;
				j = bp[28];
				k = bp[29];
				s->replen = ((j << 8) + k) << 1;
				bp += 30;
			}
	
			/* mod length */

			j	= buf[950];
			mod->length = j;

			/* positions */

			bp = buf + 952;

			for (i = 0; i < 128; ++i)
			{
				j = *bp++;
				mod->pos[i] = j;
			}

			bp	= buf + 1084;

			for (i = 0; i < mod->numpat; ++i)
			{
				ULONG i2, i3;
				pt_pattern_s *pat = mod->pattern + i;

				for (i2 = 0; i2 < 64; ++i2)
				{
					for (i3 = 0; i3 < 4; ++i3)
					{
						pt_patterndata_s *p = &pat->data[i2][i3];

						j = *bp++;
						k = *bp++;
						p->period = ((j & 15) << 8) + k;
						k	= *bp++;
						p->sample = (j & 240) + (k >> 4);
						p->effect = k & 15;
						j = *bp++;
						p->efd1 = j >> 4;
						p->efd2 = j & 15;
						p->efboth = j;
					}
				}
			}

			/* patterns */

			GetPattsAndPtrs(mod, bp, buf + bufsize, LibBase);
		}
	}

	return mod;
}

/**************************************************************************
**
**	APTR = StInit(buf, bufsize)
*/

static pt_mod_s *StInit(UBYTE *buf, ULONG bufsize, ULONG freq, struct PtPlayLibrary *LibBase)
{
	pt_mod_s	*mod;
	UBYTE *bp;
	LONG i, j, k;

	/* determine number of patterns */

	mod	= NULL;
	k		= 0;
	bp		= buf + 472;

	for (i = 0; i < 128; ++i)
	{
		j = *bp++;
		if (j > k) k = j;
	}
	
	k++;

	if (bufsize >= (1084 - 480) + (k << 8))
	{
		mod	= AllocMod(k, PT_MOD_SOUNDTRACKER, freq, LibBase);

		if (mod != NULL)
		{
			for (i = 0; i < 20; ++i) mod->name[i] = buf[i];

			/* samples */

			bp = buf + 20;

			for (i = 1; i < 16; ++i)
			{
				pt_sample_s *s = &mod->sample[i];
				j = bp[22];
				k = bp[23];
				s->length = ((j << 8) + k) << 1;
				s->ft = 0;			/* SoundTracker ei tue finetunea	*/
				j = bp[25];
				s->volume = j;
				j = bp[26];
				k = bp[27];
				s->repeat = ((j << 8) + k) << 1;
				j = bp[28];
				k = bp[29];
				s->replen = ((j << 8) + k) << 1;
				bp += 30;
			}
	
			/* mod length */

			j	= buf[470];
			mod->length = j;

			/* positions */

			bp = buf + 472;

			for (i = 0; i < 128; ++i)
			{
				j = *bp++;
				mod->pos[i] = j;
			}

			bp	= buf + 1084 - 480;

			for (i = 0; i < mod->numpat; ++i)
			{
				ULONG i2, i3;
				pt_pattern_s *pat = mod->pattern + i;

				for (i2 = 0; i2 < 64; ++i2)
				{
					for (i3 = 0; i3 < 4; ++i3)
					{
						pt_patterndata_s *p = &pat->data[i2][i3];

						j = *bp++;
						k = *bp++;
						p->period = ((j & 15) << 8) + k;
						k	= *bp++;
						p->sample = k >> 4;

						k	&= 15;
						j	 = *bp++;

//						if (k >= 3 && k <= 0xb)
//							k	= j	= 0;

						p->effect = k;
						p->efd1	= j >> 4;
						p->efd2	= j & 15;
						p->efboth = j;
					}
				}
			}

			/* patterns */

			GetPattsAndPtrs(mod, bp, buf + bufsize, LibBase);
		}
	}

	return mod;
}

/**************************************************************************
**
**	APTR = SFXInit(buf, bufsize)
*/

static pt_mod_s *SFXInit(UBYTE *buf, ULONG bufsize, ULONG freq, struct PtPlayLibrary *LibBase)
{
	pt_mod_s	*mod;
	ULONG	*lTmp;
	UBYTE *bp;
	LONG i, j, k;

	/* determine number of patterns */

	lTmp	 = (ULONG *)buf;
	buf	+= 60;
	mod	 = NULL;
	k		 = 0;
	bp		 = buf + 472;

	for (i = 0; i < 128; ++i)
	{
		j = *bp++;
		if (j > k) k = j;
	}
	
	k++;

	if (bufsize >= (1084 - 480) + (k << 8))
	{
		mod	= AllocMod(k, PT_MOD_SOUNDFX, freq, LibBase);

		if (mod != NULL)
		{
			mod->name[0]	= '\0';

			/* samples */

			bp = buf + 20;

			for (i = 1; i < 16; ++i)
			{
				pt_sample_s *s = &mod->sample[i];

#if 0
				j = bp[22];
				k = bp[23];
				s->length = ((j << 8) + k) << 1;
#endif

				j	= bp[24];
				s->ft = j & 15;			/* SoundFX ei tue finetunea, tai?	*/
				j = bp[25];
				s->volume = j;

				s->repeat	= 0;
#if 0
				j = bp[26];
				k = bp[27];
				s->repeat = ((j << 8) + k) << 1;
#endif
				j = bp[28];
				k = bp[29];
				s->replen = ((j << 8) + k) << 1;
				bp += 30;
			}
	
			/* mod length */

			j	= buf[470];
			mod->length = j;

			/* positions */

			bp = buf + 472;

			for (i = 0; i < 128; ++i)
			{
				j = *bp++;
				mod->pos[i] = j;
			}

			bp	= buf + 1084 - 480;

			for (i = 0; i < mod->numpat; ++i)
			{
				ULONG i2, i3;
				pt_pattern_s *pat = mod->pattern + i;

				for (i2 = 0; i2 < 64; ++i2)
				{
					for (i3 = 0; i3 < 4; ++i3)
					{
						pt_patterndata_s *p = &pat->data[i2][i3];

						j = *bp++;
						k = *bp++;

						p->period = ((j & 15) << 8) + k;
						k	= *bp++;
						p->sample = k >> 4;
						k	&= 15;
						j	 = *bp++;

						switch (k)
						{
							case	1: k	= 0; break;		/* arpeggiato	*/
							case	2:							/* pitchbend	*/
							{
								if ((j & 0xf0) == 0)		/* PortaUp		*/
								{
									j	= j >> 4;
									k	= 1;
								}
								break;
							}

							case	3: k = 0x0e; j = 0x01; break;	/* filter on	*/
							case	4: k = 0x0e; j = 0x00; break;	/* filter off	*/

							case	6: k = 0x0a; j = j >> 4; break;
							case	5: k = 0x0a; j	= (j >> 4) << 4; break;
							case	7: k = 0x01; break;
							case	8: k = 0x02; break;

							/* 7 == NoteStepUp, 8 == NoteStepDown	*/
							/* 5 == VolumeUp	, 6 == VolumeDown		*/
						}

						p->effect = k;
						p->efd1 = j >> 4;
						p->efd2 = j & 15;
						p->efboth = j;
					}
				}
			}

			/* patterns */

			/* ptrs to sampledata */

			bp	= buf + 60 + 600 + mod->numpat * 1024;

			for (i = 1; i < 16; ++i)
			{
				pt_sample_s *s = &mod->sample[i];

				s->data = bp;
				s->length	= *lTmp++;
				bp += s->length;

				s->length <<= 14;			/* include oversampling precision */
				s->repeat <<= 14;
				s->replen <<= 14;
				s->repend = s->repeat + s->replen;
			}	

//			GetPattsAndPtrs(mod, bp, buf + bufsize, LibBase);
		}
	}

	return mod;
}

/**************************************************************************
**
**	numbytes/success = pt_init(mod, data, size, freq)
**		Init module structure from data. If mod is NULL, returns the size
**		of the required mod data structure. Otherwise, the return value is
**		boolean and indicates whether initialization succeeded.
*/

pt_mod_s *NATDECLFUNC_5(Init, a1, UBYTE *, buf, d0, LONG, bufsize, d1, LONG, freq, d2, ULONG, modtype, a6, struct PtPlayLibrary *, LibBase)
{
	DECLARG_5(a1, UBYTE *, buf, d0, LONG, bufsize, d1, LONG, freq, d2, ULONG, modtype, a6, struct PtPlayLibrary *, LibBase)
	pt_mod_s	*mod	= NULL;

	switch (modtype)
	{
		case PT_MOD_PROTRACKER		: mod	= PtInit(buf, bufsize, freq, LibBase); break;
		case PT_MOD_SOUNDTRACKER	: mod	= StInit(buf, bufsize, freq, LibBase); break;
		case PT_MOD_SOUNDFX			: mod	= SFXInit(buf, bufsize, freq, LibBase); break;
	}

	if (mod)
	{
		mod->PlayLength	= pt_getlen(mod);
		pt_reset(mod);
	}

	return mod;
}

/**************************************************************************
**
**	ULONG = pt_cleanup(mod)
**		Disposes the mod structure
*/

VOID NATDECLFUNC_2(PtCleanup, a0, pt_mod_s *, mod, a6, struct PtPlayLibrary *, LibBase)
{
	DECLARG_2(a0, pt_mod_s *, mod, a6, struct PtPlayLibrary *, LibBase)

	if (mod != NULL)
		FreeVecTaskPooled(mod);
}

/**********************************************************************
	PtSetAttrs
**********************************************************************/

VOID NATDECLFUNC_3(PtSetAttrs, a0, pt_mod_s *, mod, a1, struct TagItem *, taglist, a6, struct PtPlayLibrary *, LibBase)
{
	DECLARG_3(a0, pt_mod_s *, mod, a1, struct TagItem *, taglist, a6, struct PtPlayLibrary *, LibBase)

	struct TagItem *tags, *tag;

	tags = taglist;

	while (tag = NextTagItem(&tags))
	{
		switch (tag->ti_Tag)
		{
			case	PTPLAY_CIAspeed			: mod->ciaspeed		= tag->ti_Data; break;
			case	PTPLAY_Flags				: mod->flags			= tag->ti_Data; break;
			case	PTPLAY_MasterVolume		: mod->mastervolume	= tag->ti_Data; break;
			case	PTPLAY_PatternPosition	: mod->ppos				= tag->ti_Data; break;
			case	PTPLAY_SongPosition		: mod->spos				= tag->ti_Data; break;
		}
	}
}

ULONG NATDECLFUNC_3(PtGetAttr, a0, pt_mod_s *, mod, d0, ULONG, tagitem, a1, ULONG *, StoragePtr)
{
	DECLARG_3(a0, pt_mod_s *, mod, d0, ULONG, tagitem, a1, ULONG *, StoragePtr)

	ULONG	store, result	= FALSE;

	switch (tagitem)
	{
		case	PTPLAY_CIAspeed			: store	= mod->ciaspeed; break;
		case	PTPLAY_Flags				: store	= mod->flags; break;
		case	PTPLAY_MasterVolume		: store	= mod->mastervolume; break;
		case	PTPLAY_PatternData		: store	= (ULONG)mod->pattern; break;
		case	PTPLAY_PatternPosition	: store	= mod->ppos; break;
		case	PTPLAY_Patterns			: store	= (ULONG)mod->numpat; break;
		case	PTPLAY_Positions			: store	= (ULONG)&mod->pos[0];
		case	PTPLAY_SongLength			: store	= mod->length; break;
		case	PTPLAY_SongLoopCount		: store	= mod->songloopcount; break;
		case	PTPLAY_SongPosition		: store	= mod->spos; break;
		case	PTPLAY_SongTitle			: store	= (ULONG)mod->name; break;
		case	PTPLAY_TotalTime			: store	= mod->PlayLength; break;

		default	: goto pois;

	}

	*StoragePtr	= store;
	result		= TRUE;

pois:
	return result;
}
