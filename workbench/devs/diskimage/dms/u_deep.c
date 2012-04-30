/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Lempel-Ziv-DynamicHuffman decompression functions used in Deep
 *     mode.
 *     Most routines ripped from LZHUF written by  Haruyasu Yoshizaki
 *
 */

#include "xdms.h"

static UWORD DecodeChar (struct xdms_data *xdms);
static UWORD DecodePosition (struct xdms_data *xdms);
static void update (struct xdms_data *xdms, UWORD c);
static void reconst (struct xdms_data *xdms);

#define DBITMASK 0x3fff   /*  uses 16Kb dictionary  */

#define F 60 /* lookahead buffer size */
#define THRESHOLD 2
#define N_CHAR (256 - THRESHOLD + F) /* kinds of characters (character code = 0..N_CHAR-1) */
#define T (N_CHAR * 2 - 1) /* size of table */
#define R (T - 1) /* position of root */
#define MAX_FREQ 0x8000 /* updates tree when the */

#if 0
UWORD freq[T + 1]; /* frequency table */

UWORD prnt[T + N_CHAR]; /* pointers to parent nodes, except for the */
				/* elements [T..T + N_CHAR - 1] which are used to get */
				/* the positions of leaves corresponding to the codes. */

UWORD son[T];   /* pointers to child nodes (son[], son[] + 1) */
#endif

static void Init_DEEP_Tabs (struct xdms_data *xdms) {
	UWORD * const freq = xdms->u_deep.freq;
	UWORD * const prnt = xdms->u_deep.prnt;
	UWORD * const son = xdms->u_deep.son;
	UWORD i, j;

	for (i = 0; i < N_CHAR; i++) {
		freq[i] = 1;
		son[i] = (UWORD)(i + T);
		prnt[i + T] = i;
	}
	i = 0; j = N_CHAR;
	while (j <= R) {
		freq[j] = (UWORD)(freq[i] + freq[i + 1]);
		son[j] = i;
		prnt[i] = prnt[i + 1] = j;
		i += 2; j++;
	}
	freq[T] = 0xffff;
	prnt[R] = 0;

	xdms->init_deep_tabs = 0;
}

UWORD Unpack_DEEP (struct xdms_data *xdms, UBYTE *in, UBYTE *out, UWORD origsize) {
	UWORD deep_text_loc = xdms->deep_text_loc;
	UBYTE *text = xdms->text;
	UWORD i, j, c;
	UBYTE *outend;

	INITBITBUF(in);

	if (xdms->init_deep_tabs) Init_DEEP_Tabs(xdms);

	outend = out+origsize;
	while (out < outend) {
		c = DecodeChar(xdms);
		if (c < 256) {
			*out++ = text[deep_text_loc++ & DBITMASK] = (UBYTE)c;
		} else {
			j = (UWORD)(c - 255 + THRESHOLD);
			i = (UWORD)(deep_text_loc - DecodePosition(xdms) - 1);
			while (j--) *out++ = text[deep_text_loc++ & DBITMASK] = text[i++ & DBITMASK];
		}
	}

	xdms->deep_text_loc = (UWORD)((deep_text_loc+60) & DBITMASK);

	return 0;
}

static UWORD DecodeChar (struct xdms_data *xdms) {
	UWORD * const son = xdms->u_deep.son;
	UWORD c;

	c = son[R];

	/* travel from root to leaf, */
	/* choosing the smaller child node (son[]) if the read bit is 0, */
	/* the bigger (son[]+1} if 1 */
	while (c < T) {
		c = son[c + GETBITS(1)];
		DROPBITS(1);
	}
	c -= T;
	update(xdms, c);

	return c;
}

static UWORD DecodePosition (struct xdms_data *xdms) {
	UWORD i, j, c;

	i = GETBITS(8);  DROPBITS(8);
	c = (UWORD)(d_code[i] << 8);
	j = d_len[i];
	i = (UWORD)(((i << j) | GETBITS(j)) & 0xff);  DROPBITS(j);

	return (UWORD)(c | i);
}

/* reconstruction of tree */

static void reconst (struct xdms_data *xdms) {
	UWORD * const freq = xdms->u_deep.freq;
	UWORD * const prnt = xdms->u_deep.prnt;
	UWORD * const son = xdms->u_deep.son;
	UWORD i, j, k, f, l;

	/* collect leaf nodes in the first half of the table */
	/* and replace the freq by (freq + 1) / 2. */
	j = 0;
	for (i = 0; i < T; i++) {
		if (son[i] >= T) {
			freq[j] = (UWORD)((freq[i] + 1) / 2);
			son[j] = son[i];
			j++;
		}
	}
	/* begin constructing tree by connecting sons */
	for (i = 0, j = N_CHAR; j < T; i += 2, j++) {
		k = (UWORD)(i + 1);
		f = freq[j] = (UWORD)(freq[i] + freq[k]);
		for (k = (UWORD)(j - 1); f < freq[k]; k--);
		k++;
		l = (UWORD)((j - k) * 2);
		memmove(&freq[k + 1], &freq[k], (size_t)l);
		freq[k] = f;
		memmove(&son[k + 1], &son[k], (size_t)l);
		son[k] = i;
	}
	/* connect prnt */
	for (i = 0; i < T; i++) {
		if ((k = son[i]) >= T) {
			prnt[k] = i;
		} else {
			prnt[k] = prnt[k + 1] = i;
		}
	}
}

/* increment frequency of given code by one, and update tree */

static void update (struct xdms_data *xdms, UWORD c){
	UWORD * const freq = xdms->u_deep.freq;
	UWORD * const prnt = xdms->u_deep.prnt;
	UWORD * const son = xdms->u_deep.son;
	UWORD i, j, k, l;

	if (freq[R] == MAX_FREQ) {
		reconst(xdms);
	}
	c = prnt[c + T];
	do {
		k = ++freq[c];

		/* if the order is disturbed, exchange nodes */
		if (k > freq[l = (UWORD)(c + 1)]) {
			while (k > freq[++l]);
			l--;
			freq[c] = freq[l];
			freq[l] = k;

			i = son[c];
			prnt[i] = l;
			if (i < T) prnt[i + 1] = l;

			j = son[l];
			son[l] = i;

			prnt[j] = c;
			if (j < T) prnt[j + 1] = c;
			son[c] = j;

			c = l;
		}
	} while ((c = prnt[c]) != 0); /* repeat up to root */
}
