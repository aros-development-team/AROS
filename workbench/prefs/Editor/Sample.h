/** Simple constants definition **/

#ifndef	SAMPLE_H
#define	SAMPLE_H

#define	SAMPLE_HEI	50
#define	EXTEND_RIG	30

#define	LINE1			"/**********************************************************"
#define	LINE2			"** jed.c : An simple, fast and efficient text editor     **"
#define	LINE3			"**         Written by T.Pierron and C.Guillaume.         **"
#define	LINE4			"**         Started on august 1998.                       **"
#define	LINE5			"**-------------------------------------------------------**"

/** Setup mini-gui for sample **/
void init_sample(struct Window *, PREFS *, WORD top);

/** Render piece of JanoEditor **/
void render_sample(struct Window *, UBYTE what);

/** Free allocated things **/
void free_sample(void);

#endif
