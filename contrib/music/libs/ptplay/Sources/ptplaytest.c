
/*
**	$Id: ptplaytest.c,v 1.1 2006/09/25 21:02:46 cvsu-ilkleht Exp $
**	Protracker 2.3a player - (C) 2001, 2003, 2004
**	Ronald Hof, Timm S. Mueller & Per Johansson ;)
**
*/

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

/*****************************************************************************/
/*
**	pt_donote(mod)
**		Interprete per new note
*/

void pt_donote2( pt_mod_s *mod)
{
    int ppos, spos;
	int i, j, k;
	pt_pattern_s *p;
	pt_patterndata_s *ptd;
	pt_channel_s *c;
	pt_sample_s *s;
	int newsp;
	 
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

		p = mod->pattern + mod->pos[spos];		/* pattern */

		ptd = p->data[ppos];							/* patterndata */
		for (i = 0; i < 4; ++i)
		{
			s=0;
			newsp = c->sp;

    		/* interprete effect command */

			j = ptd->efboth;


			switch (ptd->effect & 15)
			{
				/* Bxx - Position jump						Certified 100% */
				case 11:
				{
					if (spos >= j)
						mod->songloopcount++;

					spos=j-1;
					ppos=63;
				}
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
						mod->vbllen = PTV_TEST_FREQUENCY * 60 / (24 * j);
						break;								/* Bugfix (mod.stave 2 control / polka brothers) */
					}
					else
					if(j > 0)
					{
						mod->speed=j;
					}
					else
					{
						mod->songloopcount++;
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
**	Revision History
**	$Log: ptplaytest.c,v $
**	Revision 1.1  2006/09/25 21:02:46  cvsu-ilkleht
**	Imported
**
**	Revision 1.2  2004/03/11 09:33:53  iti
**	Implemented PtSeek(), song time calc done automatically now
**	
**	Revision 1.1.1.1  2004/03/11 00:13:33  iti
**	Initial import.
**	
**	Revision 1.3  2004/03/05 08:44:26  tmueller
**	Fixed __cplusplus, minor cleanup, added some CVS headers
**	
*/
