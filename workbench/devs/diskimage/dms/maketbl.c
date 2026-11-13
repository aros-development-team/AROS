/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Makes decoding table for Heavy LZH decompression
 *     From  UNIX LHA made by Masaru Oki
 *
 */

#include "xdms.h"

struct mktbl_data {
	WORD c;
	UWORD n, tblsiz, len, depth, maxdepth, avail;
	UWORD codeword, bit, *tbl, TabErr;
	UBYTE *blen;
};

static UWORD mktbl (struct xdms_data *xdms, struct mktbl_data *mt);

UWORD make_table (struct xdms_data *xdms, UWORD nchar, UBYTE bitlen[],UWORD tablebits, UWORD table[]) {
	struct mktbl_data mt_s, *mt = &mt_s;
	memset(mt, 0, sizeof(*mt));
	mt->n = mt->avail = nchar;
	mt->blen = bitlen;
	mt->tbl = table;
	mt->tblsiz = (UWORD)(1U << tablebits);
	mt->bit = (UWORD)(mt->tblsiz / 2);
	mt->maxdepth = (UWORD)(tablebits + 1);
	mt->depth = mt->len = 1;
	mt->c = -1;
	mt->codeword = 0;
	mt->TabErr = 0;
	mktbl(xdms, mt);	/* left subtree */
	if (mt->TabErr) return mt->TabErr;
	mktbl(xdms, mt);	/* right subtree */
	if (mt->TabErr) return mt->TabErr;
	if (mt->codeword != mt->tblsiz) return 5;
	return 0;
}

static UWORD mktbl (struct xdms_data *xdms, struct mktbl_data *mt) {
	UWORD i = 0;

	if (mt->TabErr) return 0;

	if (mt->len == mt->depth) {
		while (++mt->c < mt->n)
			if (mt->blen[mt->c] == mt->len) {
				i = mt->codeword;
				mt->codeword += mt->bit;
				if (mt->codeword > mt->tblsiz) {
					mt->TabErr=1;
					return 0;
				}
				while (i < mt->codeword) mt->tbl[i++] = (UWORD)mt->c;
				return (UWORD)mt->c;
			}
		mt->c = -1;
		mt->len++;
		mt->bit >>= 1;
	}
	mt->depth++;
	if (mt->depth < mt->maxdepth) {
		mktbl(xdms, mt);
		mktbl(xdms, mt);
	} else if (mt->depth > 32) {
		mt->TabErr = 2;
		return 0;
	} else {
		if ((i = mt->avail++) >= 2 * mt->n - 1) {
			mt->TabErr = 3;
			return 0;
		}
		xdms->u_heavy.left[i] = mktbl(xdms, mt);
		xdms->u_heavy.right[i] = mktbl(xdms, mt);
		if (mt->codeword >= mt->tblsiz) {
			mt->TabErr = 4;
			return 0;
		}
		if (mt->depth == mt->maxdepth) mt->tbl[mt->codeword++] = i;
	}
	mt->depth--;
	return i;
}
