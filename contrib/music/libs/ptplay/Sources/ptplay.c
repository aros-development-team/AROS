
/*
**	$Id: ptplay.c,v 1.2 2007/04/13 20:17:37 cvsu-ilkleht Exp $
**	Protracker 2.3a player - (C) 2001, 2003, 2004
**	Ronald Hof, Timm S. Mueller & Per Johansson ;)
**
**	TODO: check exact clipping behavior at places
**	where finetuning is involved
*/

#if 0
#include <proto/exec.h>
#include	"LibHeader.h"
#define	SysBase		LibBase->MySysBase
UWORD __dputch[5] = {0xCD4B, 0x4EAE, 0xFDFC, 0xCD4B, 0x4E75};
#define kprintf(fmt, tags...)	({ULONG _tags[] = { 0 , ## tags }; RawDoFmt(fmt, (APTR)&_tags[1], (void (*)(void)) __dputch, (APTR) *((APTR *)4));})
#else
# ifndef __VBCC__
#  define kprintf(fmt, tags...)
# else
#  define kprintf(fmt,tags)
# endif
#endif

/*****************************************************************************/

/*
 Bugfixes so far: 	6xx command
						4xx command
						E6x command
						Fxx command (when setting F0x and Fxx on the same row)
						E1x command
						E2x command
						E5x command
 There are more fixes than these, but i forgot to write them here. ;-/ , especially under pt_dotick()

 Finally also supports EFx command. :)


 The "Certified 100%" means that the effect works 100% when used in a single use mode. However
 it´s not 100% sure it works in all conditions with combined effects.


 \Per Johansson

*/

/* changed some stuff, and cleaned a few things up so it compiles with visual studio 
    lots more to come "soon"
    
    wrecK
    
 */

/* 
**	some source cleanup... shouldn't we find a better place for all these
**	comments? for a start, i added a history section at the end of this
**	file, where all commit messages will be collected :) timm
*/

/*****************************************************************************/

#include	"LibHeader.h"
#include "ptplay_priv.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define DO_LOWPASS						/* emulate $E0 */
#define DO_SONGEND						/* handle song end */
#define DO_MASTERVOLUME					/* use mastervolume */
#define DO_CLAMPING						/* clamp output */
#undef DO_PANNING						/* use $8 for panning */

#define CHNF_SLIDESKIP		0x0001		/* skip slide fx in first tick */
#define CHNF_APPLYTONEP		0x0002		/* tone-portamento is active */

#define	PER_LOW				113			/* Lowest allowable period */
#define	PER_HI				856			/* Highest allowable period */



static const unsigned char tab_funk[16]=
{
	0,5,6,7,8,10,11,13,16,19,22,26,32,43,64,128

};


static const unsigned char tab_vib[32] =
{
	0,24,49,74,97,120,141,161,
	180,197,212,224,235,244,250,253,
	255,253,250,244,235,224,212,197,
	180,161,141,120,97,74,49,24
};


static const short tab_tunes[16][36] =
{
	{856,808,762,720,678,640,604,570,538,508,480,453,
	428,404,381,360,339,320,302,285,269,254,240,226,
	214,202,190,180,170,160,151,143,135,127,120,113},

	{850,802,757,715,674,637,601,567,535,505,477,450,
	425,401,379,357,337,318,300,284,268,253,239,225,
	213,201,189,179,169,159,150,142,134,126,119,113},

	{844,796,752,709,670,632,597,563,532,502,474,447,
	422,398,376,355,335,316,298,282,266,251,237,224,
	211,199,188,177,167,158,149,141,133,125,118,112},

	{838,791,746,704,665,628,592,559,528,498,470,444,
	419,395,373,352,332,314,296,280,264,249,235,222,
	209,198,187,176,166,157,148,140,132,125,118,111},

	{832,785,741,699,660,623,588,555,524,495,467,441,
	416,392,370,350,330,312,294,278,262,247,233,220,
	208,196,185,175,165,156,147,139,131,124,117,110},

	{826,779,736,694,655,619,584,551,520,491,463,437,
	413,390,368,347,328,309,292,276,260,245,232,219,
	206,195,184,174,164,155,146,138,130,123,116,109},

	{820,774,730,689,651,614,580,547,516,487,460,434,
	410,387,365,345,325,307,290,274,258,244,230,217,
	205,193,183,172,163,154,145,137,129,122,115,109},

	{814,768,725,684,646,610,575,543,513,484,457,431,
	407,384,363,342,323,305,288,272,256,242,228,216,
	204,192,181,171,161,152,144,136,128,121,114,108},

	{907,856,808,762,720,678,640,604,570,538,508,480,
	453,428,404,381,360,339,320,302,285,269,254,240,
	226,214,202,190,180,170,160,151,143,135,127,120},

	{900,850,802,757,715,675,636,601,567,535,505,477,
	450,425,401,379,357,337,318,300,284,268,253,238,
	225,212,200,189,179,169,159,150,142,134,126,119},

	{894,844,796,752,709,670,632,597,563,532,502,474,
	447,422,398,376,355,335,316,298,282,266,251,237,
	223,211,199,188,177,167,158,149,141,133,125,118},

	{887,838,791,746,704,665,628,592,559,528,498,470,
	444,419,395,373,352,332,314,296,280,264,249,235,
	222,209,198,187,176,166,157,148,140,132,125,118},

	{881,832,785,741,699,660,623,588,555,524,494,467,
	441,416,392,370,350,330,312,294,278,262,247,233,
	220,208,196,185,175,165,156,147,139,131,123,117},

	{875,826,779,736,694,655,619,584,551,520,491,463,
	437,413,390,368,347,328,309,292,276,260,245,232,
	219,206,195,184,174,164,155,146,138,130,123,116},

	{868,820,774,730,689,651,614,580,547,516,487,460,
	434,410,387,365,345,325,307,290,274,258,244,230,
	217,205,193,183,172,163,154,145,137,129,122,115},

	{862,814,768,725,684,646,610,575,543,513,484,457,
	431,407,384,363,342,323,305,288,272,256,242,228,
	216,203,192,181,171,161,152,144,136,128,121,114},
};


#if 0
pt_mod_s * pt_load(char *filename, int freq)
{
  FILE *fp;
  int len;
  pt_mod_s *mod;

  char *buf=0;

	if (fp=fopen(filename,"rb"))
		{
			fseek(fp,0,SEEK_END);
			len=ftell(fp);
			fseek(fp,0,SEEK_SET);
			buf=(char *)malloc(len);
			fread(buf,len,1,fp);
			fclose(fp);
		}
	if(buf==0) return 0;

  mod=pt_init((unsigned char *)buf,len,freq);
  if(mod==0) free(buf);
  mod->buf=buf;
  return mod;
}

void pt_free(pt_mod_s * mod)
{
  if(mod==0) return;
  if(mod->buf!=0) free(mod->buf);
  free(mod);
}

/*****************************************************************************/
/*
**	numbytes/success = pt_init(mod, data, size, freq)
**		Init module ure from data. If mod is NULL, returns the size
**		of the required mod data ure. Otherwise, the return value is
**		boolean and indicates whether initialization succeeded.
*/

pt_mod_s *pt_init(unsigned char *buf, int bufsize, int freq)
{
	unsigned char *bp;
	int i, j, k,l;
	double tempf;
	pt_mod_s *mod;
    pt_sample_s *s;
    pt_pattern_s *pat;
    int i2, i3;
    pt_patterndata_s *p;

	if (buf == NULL) return NULL;
	if (bufsize < 1084) return NULL;
	
	/* M.K. signature */

	bp = buf + 0x438;
	if (!(bp[0] == 'M' && bp[1] == '.' && bp[2] == 'K' && bp[3] == '.')) return NULL;

	/* determine number of patterns */

	k = 0;	
	bp = buf + 952;
	
	for (i = 0; i < 128; ++i)
	{
		j = *bp++;
		if (j > k) k = j;
	}
	
	k++;
	if (bufsize < 1084 + (k << 8))
	{
		return NULL;		/* incomplete */
	}

	//if (mod == NULL)
	//{
		/* return the required size in bytes */
	//	return sizeof(*mod) + k * sizeof( pt_pattern_s);
	//}

	mod=(pt_mod_s *)calloc(1,sizeof(*mod) + k * sizeof( pt_pattern_s));
	
	memset(mod, 0, sizeof(*mod));
	mod->numpat = k;
	mod->pattern = ( pt_pattern_s *) (mod + 1);
	
	for (i = 0; i < 4; ++i)
	{
		mod->chan[i].sp = -1;
	}

	mod->chan[0].pan = 0;
	mod->chan[1].pan = 255;
	mod->chan[2].pan = 255;
	mod->chan[3].pan = 0;
	
	mod->mastervolume = 256;
//	mod->flags = 0;										//MODF_ALLOWPANNING;
	mod->flags = MODF_ALLOWFILTER;		//MODF_ALLOWPANNING;

	mod->speed = 6;
	mod->ciaspeed = 125;
	mod->freq = freq;
	mod->vbllen = freq / 50;
	
	for(i = 0; i < 20; ++i)
		mod->name[i] = buf[i];



	/* samples */

	bp = buf + 20;
	for (i = 1; i < 32; ++i)
	{
		s = &mod->sample[i];

		for(l = 0; l < 22; ++l)
			s->name[l] = bp[l];

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

	j = buf[950];
	mod->length = j;
	
	/* positions */

	bp = buf + 952;
	for (i = 0; i < 128; ++i)
	{
		j = *bp++;
		mod->pos[i] = j;
	}

	/* patterns */

	bp = buf + 1084;
	for (i = 0; i < mod->numpat; ++i)
	{
		pat = mod->pattern + i;
		for (i2 = 0; i2 < 64; ++i2)
		{
			for (i3 = 0; i3 < 4; ++i3)
			{
				p = &pat->data[i2][i3];
				j = *bp++;
				k = *bp++;
				p->period = ((j & 15) << 8) + k;
				k = *bp++;
				p->sample = (j & 240) + (k >> 4);
				p->effect = k & 15;
				j = *bp++;
				p->efd1 = j >> 4;
				p->efd2 = j & 15;
				p->efboth = j;
			}
		}
	}
	
	/* ptrs to sampledata */

	for (i = 1; i < 32; ++i)
	{
		s = &mod->sample[i];

		s->data = (char *)bp;
		bp += s->length;

		if (bp - buf > bufsize)
		{
			return FALSE;			/* sample ptr past end of file */
		}
		
		s->length <<= 14;			/* include oversampling precision */
		s->repeat <<= 14;
		s->replen <<= 14;
		s->repend = s->repeat + s->replen;
	}	


	/* init lowpass filter */

	mod->flags |= MODF_ALLOWFILTER;
	mod->filter =FALSE;

	tempf = 3.14159265358979 * 4000 / freq;
	tempf = atan(tempf);
	tempf = -(tempf-1)/(1+tempf);
	mod->fltb = (float) tempf;
	mod->flta = (float) ((1 - tempf) / 2);
	return mod;
}
#endif

static int getperiod(int note, int finetune, int delta)
{
	int i = 0;
	while (tab_tunes[0][i] != note)
		i++;

	i += delta;
	if (i > 35)
		i = 35;

	return tab_tunes[finetune][i];
}

/*****************************************************************************/
/*
**	pt_donote(mod)
**		Interprete per new note
*/

static void pt_donote( pt_mod_s *mod)
{
    int ppos, spos;
	int i, j, k;
	char UsePreviousVolume;
	pt_pattern_s *p;
	pt_patterndata_s *ptd;
	pt_channel_s *c;
	pt_sample_s *s;
	int newsp;
	int repeat;
	int replen;
	
		 
	if (mod->patdelay)
	{
		mod->patdelay--;
	}
	else
	{
		
		c = mod->chan;

		mod->skiptopos = 0;

		ppos = mod->ppos;
		spos = mod->spos;

		p = mod->pattern + mod->pos[spos];				/* pattern */

		ptd = p->data[ppos];							/* patterndata */
		for (i = 0; i < 4; ++i)
		{
			s=0;
			newsp = c->sp;

			/* next period and sample */

			k = ptd->period;
			j = ptd->sample;


			UsePreviousVolume=FALSE;

			c->prevvol=c->basevol;						/* Keep previous volume. */
			if(j!=0)
			{
				c->prevsample=j;						/* Keep previous sample nr. */
			}


			/*
			  If someone does like Jogeir and sets the sample
			  once and then uses notes without a sample-nr, Protracker 3.xx does not support his
			  behaviour, but we should for compability as alot of trackers used this bug / feature.
			*/

			if ( k != 0 && j==0 )						/* Period is set, and sample nr is 0 */
				if( ptd->effect == 0 || ptd->effect == 12 || ptd->effect == 15 )
				{
					j=c->prevsample;
					c->basevol = c->prevvol;
					UsePreviousVolume=TRUE;
				}



			/*	Reset period to last base note if vibrato was on
			**	and no new period is given */

			if( k != 0 && ptd->effect != 3 )
			{
				c->tpsrc=c->per;
			}


			/* Bugfix, for combined use of vibrato and volume sliding. */
			if ( k == 0 && c->avibspeed && ptd->effect != 4 && ptd->effect !=6 )
			{
				k = c->per;
				c->tpsrc=c->per;
			}


			/* Temporary fix for vibrato problem. 4xx (now behaves like Protracker 2.3d) */
			if( c->avibspeed != 0 )
			{
				k = ptd->period;
			}


		
			c->flags &= ~CHNF_APPLYTONEP;
			c->avibspeed = 0;
			c->atrespeed = 0;
			c->dvol = 0;
			c->dp = 0;
			c->arp = 0;
			c->delay = 0;
			c->retrig = 0;
			


			if (j > 0 && j < 32)
			{

				/* new sample */

				if (k)	
				{
					/* period given, ok */
					c->delaysample = 0;
					c->sample = j;
				}
				else
				{
					/* no period: delay this sample, but don't change it now */
					c->delaysample = j;
				}

				s = &mod->sample[j];


				j = s->volume;

				if (j > 63)
					j = 63;

				if( UsePreviousVolume == FALSE )
				{
					c->basevol = j;
					c->vol = j;
				}
			

				if (k == 0)
					goto noper;						/* no sample restart if no period */
				
				c->ft = s->ft;						/* set finetune */
				newsp = 0;							/* restart sample */
			}
			else
			{		
				if (k == 0)
					goto noper;
			}
			
			/* new period */
			
			c->arpbase = k;							/* ...is also new arpeggio baseperiod */
			c->avibindex = 0;
			c->atreindex = 0;

			j = ptd->effect;						/* get effect */


			if (j == 3 || j == 5)
				goto noper;							/* don't override active tonesliding */

			
			newsp = 0;								/* restart sample */
			
			k = getperiod(k, c->ft, 0);

			c->per = k;								/* new period */
			c->tptarget = k;						/* is also toneportamento target */
			c->tpsrc=k;




			/* new period */
			//c->freq = (16383 / (50 * 0.000000279365)) / (mod->freq / 50 * k);

			c->freq = k ? (1172874197 / (mod->freq * k / 50)) : 0;

noper:


			/* Do Funkit here */

			s = &mod->sample[c->sample];
			j=c->glissando >> 4;

			if(j != 0)
			{
				j=tab_funk[j];
				c->funkoffset+=j;

				if( !(c->funkoffset & 7))
				{
					repeat=s->repeat >> 14;
					replen=s->replen >> 14;

					c->funkoffset=0;
					c->funkcnt+=1;

					if( !(c->funkcnt <= replen) )
					{
						c->funkcnt=0;
					}

					/* Only modify sample if it has a loop, important */
					if(repeat !=0 )
					{
						/* Manipulates sample-data, nasty. ;) */
						j=-1;
						j=j-s->data[repeat+c->funkcnt];
						s->data[repeat+c->funkcnt]=j;
						c->sp=newsp=c->oldsp=c->sp+c->funkcnt;
					}
				}
			}
			/* End of Funkit */




    		/* interprete effect command */


			j = ptd->efboth;


			switch (ptd->effect & 15)
			{
				/* 0xx - Arpeggio							Certified 100% */
				case 0:
					if(j)
						c->arp=j;
				break;

				/* 1xx - Portamento Up						Certified 100% */
				case 1:
					c->dp = -j;
					c->flags |= CHNF_SLIDESKIP;
				break;

				/* 2xx - Portamento Down					Certified 100% */
				case 2:
					c->dp = j;
					c->flags |= CHNF_SLIDESKIP;
				break;

				/* 3xx - Toneportamento						Certified 100% */
				case 3:
					if (j)
						c->dtp=j;

					j = ptd->period;
					if(j)
					{
						c->tptarget = getperiod(j, c->ft, 0);
					}
					if (c->dtp)
					{
						c->flags |= CHNF_APPLYTONEP | CHNF_SLIDESKIP;
					}
					newsp = c->sp;					// no NOT reset samplepos
				break;

				/* 4xx - Vibrato							Certified 100% */
				case 4:
					j=ptd->efd2;
					if(j)
						c->vibdepth=j;
					j=ptd->efd1;
					if (j)
						c->vibspeed=j;
					c->avibspeed=c->vibspeed;
					c->flags |= CHNF_SLIDESKIP;
				break;


				/* 5xx - Toneportamento + Volslide			Certified 100% */
				case 5:
					c->dvol=-(j & 15);
					j >>= 4;
					if(j)
						c->dvol=j;

					j = ptd->period;						/* new target period? */
					if(j)
					{
						c->tptarget = getperiod(j, c->ft, 0);
					}
					if (c->dtp)
					{
						c->flags |= CHNF_APPLYTONEP | CHNF_SLIDESKIP;
					}
					newsp = c->sp;							/* no NOT reset samplepos */
				break;


				/* 6xx - Vibrato + Volslide					Certified 100% */
				case 6:
					c->dvol=-(j & 15);
					j >>=4;
					if(j)
						c->dvol=j;

					c->avibspeed=c->vibspeed;
					c->flags |= CHNF_SLIDESKIP;
				break;


				/* 7xx - Tremolo							Certified 100% */
				case 7:
					j=ptd->efd2;	
					if(j)
						c->tredepth=j << 1;
					j=ptd->efd1;
					if(j)
						c->trespeed=j;
					c->atrespeed=c->trespeed;
				break;


				/* 9xx - pt_sample_soffset						Certified 100% */
				case 9:
					if(ptd->period)
					{
						j <<= 22;
						if(j == 0)
							j=c->oldsp;
						if(s==NULL)
							j+=c->oldsp;
						c->oldsp=j;
						newsp=j;
					}
				break;


				/* Axx - Volumeslide						Certified 100% */
				case 10:
					c->dvol=-(j & 15);
					j >>= 4;
					if(j)
						c->dvol=j;
				break;


				/* Bxx - Position jump						Certified 100% */
				case 11:
					if (spos >= j)
					{
						mod->flags |= MODF_SONGEND;

						if(mod->flags & MODF_DOSONGEND)
							mod->songloopcount++;
					}

					spos=j-1;
					ppos=63;
				break;


				/* Cxx - Set volume							Certified 100% */
				case 12:
					if(j > 63)
						j = 63;
					c->basevol=j;
				break;


				/* Dxx - pt_pattern_sbreak						Certified 100% */
				case 13:
					if(j)
					{
						j=(j >> 4) * 10 + (j & 15);
						if (j < 64)
						{
							mod->skiptopos=j;
							break;
						}
					}
					ppos=63;
				break;



				/* Exx */
				case 14:
					k=ptd->efd1;
					j=ptd->efd2;

					switch (k & 15)
					{

						/* E0x - Filter On / Off			Certified 100% */
						case 0:
							if(mod->flags & MODF_ALLOWFILTER)
							{
								mod->filter = (j & 1) ^ 1;
							}
						break;


						/* E1x - Fineslide up				Certified 100% */
						case 1:
							if(j)
							{
								j=-j;
								j += c->per;
								c->tptarget=j;
								c->per=j;
								c->freq = j ? (1172874197 / (mod->freq * j / 50)) : 0;
							}
						break;

						/* E2x - Fineslide down				Certified 100% */
						case 2:
							if(j)
							{
								j += c->per;
								c->tptarget=j;
								c->per=j;
								c->freq = j ? (1172874197 / (mod->freq * j / 50)) : 0;
							}
						break;

						/* E3x - Glissando control			Certified 100% */
						case 3:
							c->glissando=j;
						break;
									
						/* E4x - Vibrato control			Certified 100% */
						case 4:
							c->vibwave=j;
						break;


						/* E5x = Set Finetune				Certified 100% */
						case 5:
							j=ptd->period;
							if(j)
							{
								newsp=0;
								k=ptd->efd2;
								c->ft=k;
								j=getperiod(j,c->ft,0);
								c->per=j;
								c->tptarget=j;
								c->freq = j ? (1172874197 / (mod->freq * j / 50)) : 0;
							}
						break;


						/* E6x - pt_pattern_sloop				Certified 100% */
						case 6:
							if(c->loopflg==0)
								if(j==0)
								{
									c->loopstart=ppos;
									c->loopflg=1;
									break;
								}

							/* Countdown */
							if (j != 0)
							{
								if(c->loopflg!=2)
									c->loopcount=j;

								c->loopflg=2;
								if(c->loopcount <=0)
								{
									c->loopcount=0;
									c->loopflg=0;
									break;
								}
								else
								{
									ppos=c->loopstart-1;
									spos--;
									c->loopcount--;
									break;
								}
							}
						break;

						/* E7x - Tremolo control			Certified 100% */
						case 7:
							c->tremwave=j;
						break;

						/* E9x - Retrig note				Certified 100% */
						case 9:
							newsp=0;
							c->retrig=j;
							c->delay=j+1;
						break;

						/* EAx - Finevol up					Certified 100% */
						case 10:
							j+=c->basevol;
							if(j > 63)
								j=63;
							c->basevol=j;
							c->vol=j;
						break;

						/* EBx - Finevol down				Certified 100% */
						case 11:
							j=c->basevol-j;
							if(j < 0)
								j=0;
							c->basevol=j;
							c->vol=j;
						break;

						/* ECx - Notecut					Certified 100% */
						case 12:
							c->cutoff=j+1;
						break;
									
						/* EDx - Notedelay					Certified 100% */
						case 13:
							if(ptd->period)
							{
								c->delay=j+1;
								newsp=c->sp;
							}
						break;

						/* EEx - pt_pattern_sdelay				Certified 100% */
						case 14:
							mod->patdelay=j;
						break;
								

						/* EFx - Invert loop				Certified 100% */
						case 15:
						{
							c->glissando=j << 4;
						}
						break;


					} /* End switch Exx commands */
				break;

				/* Fxx - Set speed							Certified 100% */
				case 0xF:
					if(j >= 32)
					{
						mod->ciaspeed=j;
						mod->vbllen=mod->freq * 60 / (24 * j);
						break;								/* Bugfix (mod.stave 2 control / polka brothers) */
					}
					else
					if(j > 0)
					{
						mod->speed=j;
					}
					else
					{
						mod->flags |= MODF_SONGEND;

						if(mod->flags & MODF_DOSONGEND)
						{
							mod->songloopcount++;
						}
					}
				break;
			}
			c->sp=newsp;
			c++;
			ptd++;
		}


		/* handle pattern and song position and skipping */
		j = mod->skiptopos;
		if(j)
		{
			ppos=j;
		}
		else
		{
			ppos=(ppos+1) & 63;
			if(ppos)
				goto nopwrap;
		}

		spos++;

		if (spos >= mod->length)
		{
			spos=0;
			mod->flags |= MODF_SONGEND;
			mod->songloopcount++;
		}
		mod->spos=spos;
nopwrap:
		mod->ppos=ppos;

	}
	mod->tick=mod->speed;
}



/*****************************************************************************/
/*
**	pt_dotick(mod)
**		Interprete per tick
*/

void pt_dotick( pt_mod_s *mod)
{

	int i, j, k;
	pt_channel_s *c = &mod->chan[0];



	if (mod->tick == 0)
		pt_donote(mod);

	mod->tick--;
	
	for (i = 0; i < 4; ++i)
	{


		if (c->flags & CHNF_SLIDESKIP)
		{
			c->flags &= ~CHNF_SLIDESKIP;					/* no slide fx in first tick */
			goto chperdone;
		}


		/* persistent changes to base period (slide, tone portamento) */

		j = c->dp;											/* delta pitch (slide) */
		if (j)
			goto chdodelta;


		if (c->flags & CHNF_APPLYTONEP)
			goto chtonep;									/* apply tone portamento */


		/* nonpersistent changes to base period (arpeggio, vibrato) */

		k = c->arp;											/* arpeggio */
		if (k)
		{
			j = mod->tick % 3;
			k = (k >> ((2 - j) << 2)) & 15;					/* get arpeggio nibble */
			j = c->arpbase;									/* arpeggio base period */
			j = getperiod(j, c->ft, k);						/* find finetuned + arpeggio offset */
			//if (j < 113) j = 113;
			goto newperiod;
		}


		k = c->avibspeed;

		if (k == 0)
			goto chperdone;


		j = c->avibindex;
		k += j;
		c->avibindex = k & 63;
		

		switch (c->vibwave & 3)
		{
			default:
				k = j << 3;
				if (j >= 32) k -= 512;
				break;
			
			case 2:
				k = 255;
				goto vibchk;
		
			case 0:
				k = tab_vib[j & 31];
		
			vibchk:
				if (j >= 32) k = -k;
				break;
		}


		j = (c->vibdepth * k) >> 7;
		j += c->per;


		if (j < PER_LOW)
			j = PER_LOW;
		if (j > PER_HI)
			j = PER_HI;

		c->freq = j ? (1172874197 / (mod->freq * j / 50)) : 0;

		c->tpsrc=j;

		goto chperdone;

chtonep:
		j = c->tptarget;								/* target period */
		j -= c->tpsrc;									/* - temporary source period */

		if(j == 0)
			goto chperdone;								/* Reached */

		k = c->dtp;

		if (j < 0)
		{
			if (-k > j)
				j = -k;									/* period goes down (slide up) */
		}
		else
		{
			if (k < j)
				j = k;									/* period goes up (slide down) */
		}


chdodelta:
		j += c->tpsrc;

		if(j < PER_LOW)
			j = PER_LOW;
		if(j > PER_HI)
			j = PER_HI;

		c->per = j;
		c->tpsrc=j;

		
		if(c->glissando)
		{
			// find closest _finetuned_ note

			k = 0;
			while (tab_tunes[c->ft][k] > j) k++;
			j = tab_tunes[c->ft][k];
		}


newperiod:

		c->freq = j ? (1172874197 / (mod->freq * j / 50)) : 0;


chperdone:

		j = c->basevol;
		
		k = c->dvol;	
		if (k)												/* apply volslide */
		{
			j += k;
			if (j > 63) j = 63;
			if (j < 0) j = 0;
			c->basevol = j;
		}
		
		k = c->atrespeed;

		if (k)												/* apply tremolo */
		{
			j = c->atreindex;
			k += j;
			c->atreindex = k & 63;
			switch (c->tremwave & 3)
			{
				default:
					k = j << 3;
					if (j >= 32) k -= 512;
					break;
				case 2:
					k = 255;
					goto tremchk;

				case 0:
					k = tab_vib[j & 31];
				tremchk:
					if (j >= 32) k = -k;
					break;
			}
			j = (k * c->tredepth) >> 8;
			j += c->basevol;
			if (j > 63) j = 63;
			if (j < 0) j = 0;
		}


		if (c->cutoff)
		{
			if (--c->cutoff == 0)
			{
				j = c->basevol = 0;							/* turn off volume */
			}
		}
		
		if (c->delay)
		{
			if (--c->delay == 0)
			{
				c->sp = 0;
				c->delay = c->retrig;
			}
		}
		
		c->vol = j;
		c++;
	}
}

/*****************************************************************************/
/*
**	pt_render(mod, destbuf1, destbuf2, bufmodulo, numsmp, freqscale, depth, channels)
**	Render protracker to sample buffer
**
**		destbuf1	destptr for channel 1
**		destbuf2	destptr for channel 2 (may be NULL)
**		bufmodulo	number of bytes to proceed from one sample to the next
**		freqscale:	1, 2	oversampling factor
**		depth:		8, 16	number of bits per output sample
**		channels:	1, 2	mono or stereo
*/

VOID NATDECLFUNC_8(PtRender, a0, pt_mod_s *, mod, a1, BYTE *, buf, a2, BYTE *, buf2, d0, LONG, bufmodulo, d1, LONG, numsmp, d2, LONG, scale, d3, LONG, depth, d4, LONG, channels)
{
	DECLARG_8(a0, pt_mod_s *, mod, a1, BYTE *, buf, a2, BYTE *, buf2, d0, LONG, bufmodulo, d1, LONG, numsmp, d2, LONG, scale, d3, LONG, depth, d4, LONG, channels)

	int writeselect = 0;
	int accul = 0;
	int accur = 0;
	int i, j,k,sr, sl;
	pt_channel_s *c;
	pt_sample_s* s;
	float inp, outp;
	float flta;
	float fltb;
	float *fltpip;
	float *fltpop;

	scale	= 2;
	numsmp *= scale;

	while ((scale >>= 1)) writeselect += 4;

	writeselect += ((depth >> 3) - 1) << 1;
	writeselect += channels - 1;

	while (numsmp--)
	{
	
	#ifdef DO_SONGEND
		if (mod->songloopcount && (mod->flags & MODF_DOSONGEND))
		{
			sr = 0;					/* mute */
			sl = 0;
		}
		else
	#endif
		{
			 c = &mod->chan[0];

			j = mod->bpos - mod->vbllen;
			if (j >= 0)
			{
				mod->bpos = j;
				pt_dotick(mod);		/* apply changes per tick */
			}
	
			mod->bpos++;
	
			/* mix */

			sl = 0;

			if (writeselect & 1)
			{
				/* stereo */
				
				sr = 0;
				for (i = 0; i < 4; ++i)
				{
					j = c->sp;
					if (j >= 0)
					{
						s = &mod->sample[c->sample];
						k = s->replen;

						if (k > 32768)
						{
							if (j >= s->repend)
							{
								j -= k;
								k = c->delaysample;
								if (k) c->sample = k;
							}
						}
						else
						{
							if (j >= s->length)
							{
								c->sp = -1;
								continue;	
							}
						}
						
						c->sp = j + c->freq;
						j = c->vol * (s->data[j >> 14]);

						k = c->pan;
						sr += j * k;
						sl += j * (255 - k);
					}
					c++;
				}

				/* <<8(8->16bit) >>2(numchannels) >>6(volume) >>7(panning) */

//				sr >>= 8;				// Seem to improve audio, however if this is wrong, please restore previous 7
//				sl >>= 8;
				sr >>= 7;
				sl >>= 7;

			#ifdef DO_LOWPASS
				if (mod->filter)
				{
				    flta = mod->flta;
	                fltb = mod->fltb;
	                fltpip = mod->fltpi;
	                fltpop = mod->fltpo;	

					inp = sl;
					outp = flta * (inp + *fltpip) + fltb * *fltpop;
					*fltpip++ = inp;
					*fltpop++ = outp;
					inp = outp;
					outp = flta * (inp + *fltpip) + fltb * *fltpop;
					*fltpip++ = inp;
					*fltpop++ = outp;
					sl = outp;

					inp = sr;
					outp = flta * (inp + *fltpip) + fltb * *fltpop;
					*fltpip++ = inp;
					*fltpop++ = outp;
					inp = outp;
					outp = flta * (inp + *fltpip) + fltb * *fltpop;
					*fltpip = inp;
					*fltpop = outp;
					sr = outp;
				}
			#endif
			

			#ifdef DO_CLAMPING
				if (sl < -32768) sl = -32768;
				if (sl > 32767) sl = 32767;
				if (sr < -32768) sr = -32768;
				if (sr > 32767) sr = 32767;
			#endif


			}
			else
			{
				/* mono */

				for (i = 0; i < 4; ++i)
				{
					j = c->sp;
					if (j >= 0)
					{
						s = &mod->sample[c->sample];
						k = s->replen;

						if (k > 32768)
						{
							if (j >= s->repend)
							{
								j -= k;
								k = c->delaysample;
								if (k) c->sample = k;
							}
						}
						else
						{
							if (j >= s->length)
							{
								c->sp = -1;
								continue;	
							}
						}
						
						c->sp = j + c->freq;
						sl += (short) c->vol * (short) (s->data[j >> 14]);
					}
					c++;
				}

			#ifdef DO_LOWPASS
				if (mod->filter)
				{
					
					flta = mod->flta;
					fltb = mod->fltb;
					fltpip = mod->fltpi;
					fltpop = mod->fltpo;

					inp = sl;
					outp = flta * (inp + *fltpip) + fltb * *fltpop;
					*fltpip++ = inp;
					*fltpop++ = outp;
					inp = outp;
					outp = flta * (inp + *fltpip) + fltb * *fltpop;
					*fltpip = inp;
					*fltpop = outp;
					sl = outp;
				}
			#endif
			
			#ifdef DO_CLAMPING
				if (sl < -32768) sl = -32768;
				if (sl > 32767) sl = 32767;
			#endif

				sr = sl;
			}
		}
		
		/* write to destination */

	#ifdef DO_MASTERVOLUME
		j = mod->mastervolume;		

		switch (writeselect & 7)
		{
			case 0:		*buf = (j * ((sr + sl) >> 1)) >> 16;	/* 1-8-mono */
						break;

			case 1:		*buf = (j * sr) >> 16;					/* 1-8-stereo */
						*buf2 = (j * sl) >> 16;
						break;

			case 2:		*(short *) buf = (j * (sr + sl)) >> 9;	/* 1-16-mono */
						break;

			case 3:
						*(short *) buf = (j * sr) >> 8;			/* 1-16-stereo */
						*(short *) buf2 = (j * sl) >> 8;
					break;

			case 4:		accul += sr + sl;						/* 2-8-mono */
						if (numsmp & 1) continue;
						*buf = (accul * j) >> 18;
						accul = 0;
						break;

			case 5:		accur += sr;							/* 2-8-stereo */
						accul += sl;
						if (numsmp & 1) continue;
						*buf = (accur * j) >> 17;
						*buf2 = (accul * j) >> 17;
						accur = 0;
						accul = 0;
						break;

			case 6:		accul += sr + sl;						/* 2-16-mono */
						if (numsmp & 1) continue;
						*(short *) buf = (accul * j) >> 10;
						accul = 0;
						break;

			case 7:		accur += sr;							/* 2-16-stereo */
						accul += sl;
						if (numsmp & 1) continue;
						*(short *) buf = (accur * j) >> 9;
						*(short *) buf2 = (accul * j) >> 9;
						accur = 0;
						accul = 0;
						break;
		}
	#else
		switch (writeselect & 7)
		{
			case 0:		*buf = ((sr + sl) >> 1) >> 8;			/* 1-8-mono */
						break;

			case 1:		*buf = (j * sr) >> 8;					/* 1-8-stereo */
						*buf2 = (j * sl) >> 8;
						break;

			case 2:		*(short *) buf = (sr + sl) >> 1;			/* 1-16-mono */
						break;

			case 3:		*(short *) buf = sr;						/* 1-16-stereo */
						*(short *) buf2 = sl;
						break;

			case 4:		accul += sr + sl;						/* 2-8-mono */
						if (numsmp & 1) continue;
						*buf = accul >> 10;
						accul = 0;
						break;

			case 5:		accur += sr;							/* 2-8-stereo */
						accul += sl;
						if (numsmp & 1) continue;
						*buf = accur >> 9;
						*buf2 = accul >> 9;
						accur = 0;
						accul = 0;
						break;

			case 6:		accul += sr + sl;						/* 2-16-mono */
						if (numsmp & 1) continue;
						*(short *) buf = accul >> 2;
						accul = 0;
						break;

			case 7:		accur += sr;							/* 2-16-stereo */
						accul += sl;
						if (numsmp & 1) continue;
						*(short *) buf = accur >> 1;
						*(short *) buf2 = accul >> 1;
						accur = 0;
						accul = 0;
						break;
		}
	#endif

		buf += bufmodulo;
		buf2 += bufmodulo;
	}
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: ptplay.c,v $
**	Revision 1.2  2007/04/13 20:17:37  cvsu-ilkleht
**	Updated to 2.6
**
**	Revision 1.1  2006/09/25 21:02:46  cvsu-ilkleht
**	Imported
**
**	Revision 1.3  2004/05/08 18:56:37  iti
**	Fixed sources for VBCC/68k
**	
**	Revision 1.2  2004/04/12 23:30:45  iti
**	Fixed small mod ending bug
**	
**	Revision 1.1.1.1  2004/03/11 00:13:31  iti
**	Initial import.
**	
**	Revision 1.3  2004/03/05 08:44:26  tmueller
**	Fixed __cplusplus, minor cleanup, added some CVS headers
**	
*/
