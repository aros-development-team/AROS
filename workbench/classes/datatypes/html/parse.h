/*
    Copyright © 2004, Martin Gierich. All rights reserved.
    Licensed under the terms of the AROS Public License (APL)
    $Id$

    Desc: Structs for HTML parser with syntax checker
*/

#define USE_WARN 0
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

typedef void (*tagfunc)( void );

/*******************************************************************************************/
/* Prototypes */

/* Tag Open/Close */
void		tag_html_open( void );
void		tag_title_close( void );
void		tag_comment( void );
void		tag_h1_open( void );
void		tag_h2_open( void );
void		tag_p_open( void );
void		tag_pre_open( void );
void		tag_b_open( void );
void		tag_big_open( void );
void		tag_i_open( void );
void		tag_small_open( void );
void		tag_tt_open( void );
void		tag_a_open( void );
void		tag_br( void );
void		tag_hr( void );
void		tag_img( void );

/*******************************************************************************************/

#define STACKSIZE 100
#define STRINGPOOLSIZE 10000
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
	if( !inbufbytes-- )			\
	{					\
		D(printf("Buffer empty\n");)	\
		ret;				\
	}					\
	c = *inbuf++;

#define UNGETCHAR( c )				\
	inbuf--;				\
	inbufbytes++;

#define PUTCHAR( c )				\
	D( if( c ) printf("%c", c); )		\
	if( strpoolpos < strpoolend )		\
	{					\
		*strpoolpos++ = c;		\
	}

