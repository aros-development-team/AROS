/*
    Copyright © 2004, Martin Gierich. All rights reserved.
    Licensed under the terms of the AROS Public License (APL)
    $Id$

    Desc: Structs for HTML parser with syntax checker
*/

#define USE_WARN 1
#define DEBUG_PARSE 1

#ifndef WARN
#if USE_WARN
#define WARN(x...) x
#else
#define WARN(x...)
#endif
#endif

#ifndef D
#if DEBUG_PARSE
#define D(x...) x
#else
#define D(x...)
#endif
#endif

typedef void (*tagfunc)( parse_struct *pdata );

/*******************************************************************************************/
/* Prototypes */

/* Tag Open/Close */
void		tag_html_open( parse_struct *pdata );
void		tag_title_close( parse_struct *pdata );
void		tag_comment( parse_struct *pdata );
void		tag_h1_open( parse_struct *pdata );
void		tag_h2_open( parse_struct *pdata );
void		tag_p_open( parse_struct *pdata );
void		tag_pre_open( parse_struct *pdata );
void		tag_b_open( parse_struct *pdata );
void		tag_big_open( parse_struct *pdata );
void		tag_i_open( parse_struct *pdata );
void		tag_small_open( parse_struct *pdata );
void		tag_tt_open( parse_struct *pdata );
void		tag_a_open( parse_struct *pdata );
void		tag_br( parse_struct *pdata );
void		tag_hr( parse_struct *pdata );
void		tag_img( parse_struct *pdata );

/*******************************************************************************************/

#define STACKSIZE 100
#define STRINGPOOLSIZE 1024
#define WARNPOOLSIZE 1024
#define SEGLISTSIZE 100

/*******************************************************************************************/

#define CHRGROUP_SPACE	1
#define CHRGROUP_CTRL	2
#define CHR_TAGSTART	('<')
#define CHR_TAGEND	('>')
#define CHR_ESCAPE	('&')
#define CHR_QUOTE	('"')
#define CHR_NEWLINE	('\n')

#define ISNORMAL(code)	(!code)
#define ISSPACE(code)	((code)==CHRGROUP_SPACE)
#define ISCTRL(code)	((code)==CHRGROUP_CTRL)

#define LOWERCASE(c)		((c) | 0x20)
#define HASHNOCASE(hash, c)	(((hash)<<2) ^ LOWERCASE((u_char)c))
#define HASHCASE(hash, c)	(((hash)<<2) ^ ((u_char)c) )

/*******************************************************************************************/
/* Style Stack */

typedef struct
{
	int		tag;
	int		line;
	u_char		hasnotext;
	u_char		prelayout;
	style_flags	styleflags;
} stack_struct __attribute__((packed));

/*******************************************************************************************/
/* Inline Functions */

#define GETCHAR( c, ret )			\
	if( !pdata->inbufbytes-- )			\
	{					\
		D(printf("Buffer empty\n");)	\
		ret;				\
	}					\
	c = *pdata->inbuf++;

#define UNGETCHAR( c )				\
	pdata->inbuf--;				\
	pdata->inbufbytes++;

#define PUTCHAR( c )				\
	D( if( c ) printf("%c", c); )		\
	*strpos++ = c;				\
	maxlen--;

