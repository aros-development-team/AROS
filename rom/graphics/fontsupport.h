#ifndef FONTSUPPORT_H
#define FONTSUPPORT_H
/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Misc definitions internal to fonts.
    Lang: english
*/

#define NUMCHARS(tf) ((tf->tf_HiChar - tf->tf_LoChar) + 2)
#define CTF(x) ((struct ColorTextFont *)x)

struct tfe_hashnode
{
    struct tfe_hashnode 	*next;
    struct TextFont		*back;
    struct TextFontExtension	*ext;
    
    /* A bitmap describing the font */
    OOP_Object *font_bitmap;
};

extern struct tfe_hashnode *tfe_hashlookup(struct TextFont *tf, struct GfxBase *GfxBase);

extern void tfe_hashadd(struct tfe_hashnode *hn
		, struct TextFont *tf
		, struct TextFontExtension 	*etf
		, struct GfxBase *GfxBase);


extern void tfe_hashdelete(struct TextFont *tf, struct GfxBase *GfxBase);
struct tfe_hashnode *tfe_hashnode_create(struct GfxBase *GfxBase);

OOP_Object *fontbm_to_hiddbm(struct TextFont *font, struct GfxBase *GfxBase);
	

#endif /* FONTSUPPORT_H */
