/*
    Copyright © 2004, Martin Gierich. All rights reserved.
    Licensed under the terms of the AROS Public License (APL)
    $Id$

    Desc: HTML parser with syntax checker
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
//#include <string.h>

#include "common.h"
#include "parse.h"
#include "tables.h"

#ifdef __AROS__
int strcasecmp (
	const char * str1,
	const char * str2)
{
    int diff;
    while (!(diff = LOWERCASE (*str1) - LOWERCASE (*str2)) && *str1)
    {
	str1 ++;
	str2 ++;
    }
    return diff;
} /* strcasecmp */

int strcmp (
	const char * str1,
	const char * str2)
{
    int diff;
    while (!(diff = LOWERCASE (*str1) - LOWERCASE (*str2)) && *str1)
    {
	str1 ++;
	str2 ++;
    }
    return diff;
} /* strcasecmp */
#endif

extern string List_of_SegCmds[SEG_CMD_MAX];

/*******************************************************************************************/
/* Prototypes */

/* Output */
WARN( static void	warnlog( string text, ... ); )
static seg_struct *	seglist_store( u_char cmd );
static seg_struct *	seglist_check_last( u_char segcmd );
static void		seglist_delete_last( void );
static void		seglist_end( void );
static void		seglist_free( void );

/* String Parsing */
D( static void	debug_putc( char c ); )
static char	skip_spaces( char endchar, int stop );
static char	copy_text( char endchar );
static char	escape_to_char( void );
static int	get_number( int radix, char *lastchar );
static int	parse_tag( void );
static void	skip_to_tag_end( void );

/* Style Stack Handling */
static void	styles_reset( void );
static void	change_style( void );
static void	change_paragraph( para_struct *para );
static void	stack_push( int tag );
static int	stack_pop( int searchtag );
static int	stack_check( void );

/* Tag and Paragraph */
static void	block_start( void );
static void	block_end( void );
static void	handle_open_tag( int tag, int dofunc );
static void	handle_close_tag( int tag, int dofunc );

/*******************************************************************************************/
/* Globals */

/* page */
static page_struct	*page;

/* data arrays */
static int	MAX_Tag_Length;
static u_short	Hash_of_Tags[MAX_Tags];
static int	MAX_Esc_Length;
static u_short	Hash_of_Escs[MAX_Escs];

static string	inbuf;
static int	inbufbytes;

/* output buffers */
static string	strpool;
static string	strpoolpos;
static string	strpoolend;
static seg_struct *seglistpos;
int		seglistremain;

/* state tracking */
static int	linenum;
static int	level;
static int	block;
static int	skipspace;

/* state stack */
static stack_struct	stack[STACKSIZE];
static int		stackpos;
static stack_struct	style;

/*******************************************************************************************/
/* Main parse */

page_struct * parse_init( void *mempool )
{
	int		i, j;
	char		c;
	string		str;
	u_short		hash;
	page_struct	*page;

	MAX_Tag_Length = 0;
	for( i=0; i<MAX_Tags; i++ )
	{
		str = List_of_Tags[i].name;
		j = strlen( str );
		if( j > MAX_Tag_Length )
			MAX_Tag_Length = j;
		hash = 0;
		while( (c = *str++) )
		{
			hash = HASHNOCASE( hash, c );
		}
		Hash_of_Tags[i] = hash;
/*		D( printf("Tag %2d %s:\t hash 0x%04x len %2d prevl %d nextl %d flags %02x\n",
			i, List_of_Tags[i].name, (int)Hash_of_Tags[i], j, (int)List_of_Tags[i].prevlevel,
			(int)List_of_Tags[i].nextlevel, (int)List_of_Tags[i].flags); )
*/	}
	D(
		for( i=0; i<MAX_Tags-1; i++ )
		{
			for( j=i+1; j<MAX_Tags; j++ )
			{
				// printf("Tags %2d=%04x %2d=%04x\n", i, Hash_of_Tags[i], j, Hash_of_Tags[j]);
				if( Hash_of_Tags[i] == Hash_of_Tags[j] )
					printf("Duplicate Tag Hash at %d <-> %d !!!\n", i, j);
			}
		}
	)
	D( printf("Max Tag len %d Num Tags %d\n", MAX_Tag_Length, MAX_Tags); )
	D( printf("structsize tag %d esc %d stack %d page %d seg %d style %d\n", sizeof(tag_struct),
		sizeof(esc_struct), sizeof(stack_struct), sizeof(page_struct), sizeof(seg_struct), sizeof(style_flags)); )

	MAX_Esc_Length = 0;
	for( i=0; i<MAX_Escs; i++ )
	{
		str = List_of_Escs[i].name;
		j = strlen( str );
		if( j > MAX_Esc_Length )
			MAX_Esc_Length = j;
		hash = 0;
		while( (c = *str++) )
		{
			hash = HASHCASE( hash, c );
		}
		Hash_of_Escs[i] = hash;
//		D( printf("Esc %2d %s:\t hash 0x%04x\n", i, List_of_Escs[i].name, (int)Hash_of_Escs[i]); )
	}
	D(
		for( i=0; i<MAX_Escs-1; i++ )
		{
			for( j=i+1; j<MAX_Escs; j++ )
			{
				// printf("Escs %2d=%04x %2d=%04x\n", i, Hash_of_Escs[i], j, Hash_of_Escs[j]);
				if( Hash_of_Escs[i] == Hash_of_Escs[j] )
					printf("Duplicate Esc Hash at %d <-> %d !!!\n", i, j);
			}
		}
	)

	page = MALLOC( mempool, sizeof( page_struct ) );
	if( !page )
		return NULL;
	D( printf("page: %p\n", page); )
	page->mempool = mempool;
	page->title = NULL;
	page->seglist = NULL;
	seglistremain = 0;

	strpool = strpoolpos = MALLOC( mempool, STRINGPOOLSIZE );
	if( !strpool )
		return NULL;
	strpoolend = strpool + STRINGPOOLSIZE;
	D( printf("strpool: %p < %p < %p\n", strpool, strpoolpos, strpoolend); )

	return page;
}

int parse_do( page_struct *mypage, string myinbuf, int myinbufbytes )
{
	int	tag;
	char	c;

	// global hack
	page = mypage;
	inbuf = myinbuf;
	inbufbytes = myinbufbytes;

	linenum = 1;
	level = LEVEL_top;
	block = FALSE;
	skipspace = TRUE;
	stackpos = 0;
	styles_reset();
	do
	{
		D( printf("\n"); )
		if( style.hasnotext )
			c = skip_spaces( CHR_TAGSTART, FALSE );
		else
		{
			if( level == LEVEL_inline )
			{
				c = copy_text( CHR_TAGSTART );
			}
			else
			{
				c = skip_spaces( CHR_TAGSTART, TRUE );
				if( c && c != CHR_TAGSTART)
				{
					WARN( warnlog("implicit <p> (inline text)"); )
					UNGETCHAR( c )
					handle_open_tag( TAG_p, FALSE );
					skipspace = TRUE;
					c = copy_text( CHR_TAGSTART );
				}
			}
		}
		if( !c )
			break;
		GETCHAR( c, break )
		if( c != '/' )
		{
			UNGETCHAR( c )
			tag = parse_tag();
			D( printf("%d: Tag <%s> (%d) opening level {%s} stack %d\n",
				linenum, tag>=0 ? List_of_Tags[tag].name:"?",
				tag, List_of_Levels[level], stackpos); )
			if( tag >= 0 )
				handle_open_tag( tag, TRUE );
		}
		else
		{
			tag = parse_tag();
			D( printf("%d: Tag </%s> (%d) closing level {%s} stack %d\n",
				linenum, tag>=0 ? List_of_Tags[tag].name:"?",
				tag, List_of_Levels[level], stackpos); )
			if( tag >= 0 )
				handle_close_tag( tag, TRUE );
		}
		D( printf("%d: New level {%s} (%d) hasnotext %d\n", linenum, List_of_Levels[level], level, style.hasnotext); )
	}
	while( inbufbytes > 0 );
	return TRUE;
}

int parse_end( page_struct *mypage )
{
	if( block )
	{
		WARN( warnlog("implicit block end at end"); )
		block_end();
	}
	if( stackpos != 0 )
	{
		WARN( warnlog("stack not empty at end"); )
		stack_pop( TAG_html );
	}
	seglist_end();
	D( printf("strpool: %p < %p < %p\n", strpool, strpoolpos, strpoolend); )
	D( printf("page: %p title %p seglist %p\n", page, page->title, page->seglist); )
	return TRUE;
}

void parse_free( page_struct *mypage )
{
	seglist_free();
	D( printf("freed seglist\n"); )
	MFREE( mypage, strpool );
	D( printf("freed strpool\n"); )
}

/*******************************************************************************************/
/* Output */

WARN(
static void warnlog( string format, ... )
{
	va_list	args;

	va_start (args, format);
	fprintf( stderr, "*** line %2d: ", linenum);
	vfprintf( stderr, format, args );
	fprintf( stderr, "\n");
	va_end (args);
}
)

static seg_struct* seglist_store( u_char cmd )
{
	seg_struct	*seg;

	//D( printf("---1 cmd %d seglistpos %p remain %d [%s]\n", cmd, seglistpos, seglistremain, List_of_SegCmds[cmd]); )
	seglistremain --;	/* make sure a last one remains */
	if( seglistremain <= 0 )
	{
		seg = MALLOC( page->mempool, SEGLISTSIZE * sizeof(seg_struct) );
		if( !seg )
		{
			seglistremain = 0;	/* keep error state */
			return NULL;
		}
		if( !page->seglist )
		{	/* initial */
			D(printf("initial seglist alloc %p size 0x%x\n", seg, SEGLISTSIZE * sizeof(seg_struct));)
			page->seglist = seg;
		}
		else
		{	/* add another list */
			D(printf("more seglist alloc %p\n", seg);)
			seglistpos->cmd = SEG_CMD_Next;
			seglistpos->next = seg;
		}
		seglistremain = SEGLISTSIZE - 1;	/* subtract this one */
		seglistpos = seg;
		D( printf("seglist: %p < %p < %d\n", page->seglist, seglistpos, seglistremain); )
	}
	seg = seglistpos ++;
	seg->cmd = cmd;
	return seg;
}

static seg_struct* seglist_check_last( u_char segcmd )
{
	seg_struct	*seg;

	seg = seglistpos;
	seg --;
	if( seg->cmd == segcmd )
		return seg;
	else
		return NULL;
}

static void seglist_delete_last( void )
{
	seglistpos --;
	seglistremain ++;
}

static void seglist_end( void )
{
	seg_struct	*seg;

	seg = seglistpos;
	seg --;
	seg->cmd |= SEG_CMD_LAST;
	D( printf("seglist: %p < %p < %d\n", page->seglist, seglistpos, seglistremain); )
}

static void seglist_free( void )
{
	MFREE( page, page->seglist );
}

/*******************************************************************************************/
/* String Parsing */

D(
static void debug_putc( char c )
{
	if( c >= ' ' )
		printf("%c", c);
	else
		printf("[%d]", (int)c);
}
)

static char skip_spaces( char endchar, int stop )
{
	char	c;
	char	str[20];
	int	i, code;

	D( printf("%d: Skip Spaces to %c: [", linenum, endchar); )
	i = 0;
	do
	{
		GETCHAR( c, return 0 )
		D( debug_putc( c ); )
		if( c == endchar )
			break;
		if( c == CHR_NEWLINE )
			linenum++;
		code = charlist[(u_char)c];
		if( !ISSPACE( code ) )
		{
			if( stop )
				return c;
			else if( i<19 )
				str[i++] = c;
		}
	}
	while( 1 );
	D( printf("]\n"); )
	str[i] = '\0';
	WARN( if( i ) warnlog("unexpected chars: %s", str); )
	return c;
}

static char copy_text( char endchar )
{
	char	c;
	string	strstart;
	int	space, i, code;
	int	prelayout;

	space = skipspace;
	prelayout = style.prelayout;
	do
	{
		D( printf("%d: Copy Text (sp %d) to %c: [", linenum, space, endchar); )
		strstart = strpoolpos;
		do
		{
			GETCHAR( c, return 0 )
			code = charlist[(u_char)c];
			if( ISNORMAL( code ) )
			{
				space = FALSE;
				PUTCHAR( c )
			}
			else if( ISSPACE( code ) )
			{
				if( c == CHR_NEWLINE )
				{
					linenum++;
					if( prelayout )
						break;
				}
				if( prelayout )
				{
					if( c == '\t' )
						for(i=0; i<8; i++)
						{
							PUTCHAR( ' ' );
						}
					else
					{
						PUTCHAR( ' ' );
					}
				}
				else if( !space )
				{
					space = TRUE;
					PUTCHAR( ' ' );
				}
			}
			else
			{
				space = FALSE;
				if( c == CHR_ESCAPE )
				{
					c = escape_to_char();
					D( printf("&"); )
					PUTCHAR( c )
					D( printf("&"); )
				}
				else if( c == endchar )
				{
					break;
				}
				else
				{
					WARN( warnlog("char %c needs escape", c); )
					PUTCHAR( c )
				}
			}
		}
		while( 1 );
		D( printf("]\n"); )
		if( strpoolpos > strstart )
		{
			seg_struct *seg;
			int length;
			
			if( skipspace && (strpoolpos - strstart) == 1 && strstart[0] == ' ' && !prelayout )
				return c;
			length = strpoolpos - strstart;
			PUTCHAR( '\0' )
			D( printf("String %2d: [%s]\n", length, strstart); )
			skipspace = FALSE;
			seg = seglist_store( SEG_CMD_Text );
			if( !seg )
				return 0;
			seg->textlen = length;
			seg->textseg = strstart;
		}
		if( prelayout && (c == CHR_NEWLINE) )
			seglist_store( SEG_CMD_Linebreak );
	}
	while( c != endchar );
	return c;
}

static char escape_to_char( void )
{
	char	c;
	int	i, res, code;
	char	str[MAX_Esc_Length+1];
	u_short	hash;

	GETCHAR( c, return '?' )
	if( c == '#' )
	{
		GETCHAR( c, return '?' )
		if( LOWERCASE(c)=='x' )
		{
			i = get_number( 16, &c );
		}
		else if( c>='0' && c<='9' )
		{
			UNGETCHAR( c )
			i = get_number( 10, &c );
		}
	}
	else
	{
		UNGETCHAR( c )
		hash = 0;
		for( i=0; i<=MAX_Esc_Length; i++ )
		{
			GETCHAR( c, return '?' )
			code = charlist[(u_char)c];
			if( c==';' || !ISNORMAL( code ) )
				break;
			hash = HASHCASE( hash, c );
			str[i] = c;
		}
		str[i] = '\0';
		if( c==';' )
		{
			for( i=0; i<MAX_Escs; i++ )
			{
				if( Hash_of_Escs[i] == hash )
				{
					res = strcmp( str, List_of_Escs[i].name );
					if( res==0 )
						return List_of_Escs[i].value;
					D( printf("Hash failure\n"); )
				}
			}
			WARN( warnlog("unknown escape %s", str); )
			return '?';
		}
	}
	if( c==';' )
		return (char)i;
	WARN( warnlog("bad end of escape (%c)", c); )
	UNGETCHAR( c )
	return '?';
}

static int get_number( int radix, char *lastchar )
{
	char	c;
	int	num;

	num = 0;
	do
	{
		GETCHAR( c, return 0 )
		if( c>='0' && c<='9' )
		{
			num = num*radix + c - '0';
		}
		else if( radix==16 )
		{
			c = LOWERCASE(c);
			if( c>='a' && c<='f' )
				num = num*16 + c - 'a' + 10;
			else
				break;
		}
		else
			break;
	}
	while( 1 );
	D( printf("[Num %d 0x%x]", num, num); )
	*lastchar = c;
	return num;
}

static int parse_tag( void )
{
	char	c;
	int	tag;
	int	i, res, code;
	char	str[MAX_Tag_Length+1];
	u_short	hash;

	D( printf("%d: Parse Tag: ", linenum); )
	hash = 0;
	for( i=0; i<=MAX_Tag_Length; i++ )
	{
		GETCHAR( c, return -1 )
		D( debug_putc( c ); )
		code = charlist[(u_char)c];
		if( !ISNORMAL( code ) )
			break;
		hash = HASHNOCASE( hash, c );
		str[i] = c;
	}
	str[i] = '\0';
	D( printf("\n"); )
	UNGETCHAR( c )
	for( tag=0; tag<MAX_Tags; tag++ )
	{
		if( Hash_of_Tags[tag] == hash )
		{
			res = strcasecmp( str, List_of_Tags[tag].name );
//			D( printf("tag: %d res: %d\n", tag, res); )
			if( res==0 )
				return tag;
			D( printf("Hash failure\n"); )
		}
	}
	WARN( warnlog("skipping unknown tag <%s>", str); )
	skip_to_tag_end();
	return -1;
}

static void skip_to_tag_end( void )
{
	char	c;
	int	quote, code;

	D( printf("%d: Skip to Tag End: ", linenum); )
	quote = FALSE;
	do
	{
		GETCHAR( c, return )
		D( debug_putc( c ); )
		code = charlist[(u_char)c];
		if( ISNORMAL( code ) )
			continue;
		if( c == CHR_NEWLINE )
			linenum++;
		if( c == CHR_QUOTE )
		{
			quote = !quote;
			D( printf("#"); )
			continue;
		}
	}
	while( c != CHR_TAGEND );
	WARN( if( quote ) warnlog("unmatched quote"); )
	D( printf("\n"); )
	return;
}

/*******************************************************************************************/
/* Style Stack Handling */

static void styles_reset( void )
{
	style.hasnotext = TRUE;
	style.prelayout = FALSE;
	style.styleflags.value = 0;
	style.styleflags.fl.fontsize = 8;
}

static void change_style( void )
{
	static style_flags	laststyle = {0};
	seg_struct	*seg;

	if( block && laststyle.value != style.styleflags.value )
	{
		laststyle = style.styleflags;
		seg = seglist_check_last( SEG_CMD_Softstyle );
		if( seg )
		{
//			D( printf("*****\t\t\texisting style change to %04x\n", style.styleflags.value); )
			seg->styleflags = style.styleflags;
		}
		else
		{
//			D( printf("*****\t\t\tstyle change to %04x\n", style.styleflags.value); )
			seg = seglist_store( SEG_CMD_Softstyle );
			if( seg )
				seg->styleflags = style.styleflags;
		}
	}
}

static void change_paragraph( para_struct *para )
{
	static para_struct	*lastpara;
	seg_struct		*seg;

	style.styleflags = para->styleflags;
	change_style();
	if( lastpara != para )
	{
		lastpara = para;
//		D( printf("*****\t\t\tpara change to %02x\n", para->paraflags.value); )
		seg = seglist_store( SEG_CMD_Parastyle );
		if( seg )
		{
			seg->paraflags = para->paraflags;
		}
	}
}

static void stack_push( int tag )
{
	if( stackpos < STACKSIZE )
	{
		stack[stackpos]			= style;
		stack[stackpos].tag		= tag;
		stack[stackpos].line		= linenum;
		stackpos++;
//		D( printf("*****\t\t\tpush tag <%s> style %04x pos %d\n", List_of_Tags[tag].name, style.styleflags.value, stackpos); )
	}
	else
	{
		WARN( warnlog("stack overflow"); )
	}
}

static int stack_pop( int searchtag )
{
	int	tag, pos, count;

	pos = stackpos;
	count = 0;
	while( pos > 0 )
	{
		pos--;
		tag = stack[pos].tag;
//		D( printf("*****\t\t\tSearch tag <%s> pos %d\n", List_of_Tags[tag].name, pos); )
		if( tag == searchtag )
		{
			stackpos = pos;
			style		= stack[stackpos];
//			D( printf("*****\t\t\tpop tag <%s> style %04x pos %d\n", List_of_Tags[tag].name, style.styleflags.value, stackpos); )
			return tag;
		}
		else
		{
			WARN( warnlog("skipped tag <%s> from line %d in stack trying to find </%s>",
				List_of_Tags[tag].name, stack[pos].line, List_of_Tags[searchtag].name); )
		}
		count++;
	}
	WARN(	if( !count )
			warnlog("stack underflow");
		else
			warnlog("missing opening tag for </%s>", List_of_Tags[searchtag].name);
	)
	return -1;
}

static int stack_check( void )
{
	if( stackpos > 0 )
		return stack[stackpos-1].tag;
	else
		return -1;
}

/*******************************************************************************************/
/* Tag Open/Close */

static void block_start( void )
{
	if( block )
	{
		WARN( warnlog("implicit block end"); )
		block_end();
	}
	D( printf("------- start -------\n"); )
	seglist_store( SEG_CMD_Blockstart );
	block = TRUE;
}

static void block_end( void )
{
	seg_struct *seg;

	if( block )
	{
		D( printf("-------  end  -------\n"); )
		/* remove single space at end */
		seg = seglist_check_last( SEG_CMD_Text );
		if( seg )
		{
			if( seg->textlen == 1 && seg->textseg[0] == ' ' )
				seglist_delete_last();
		}
		seglist_store( SEG_CMD_Blockend );
		block = FALSE;
	}
	else
	{
		WARN( warnlog("block end without start"); )
	}
}

static void handle_open_tag( int tag, int dofunc )
{
	int	oldtag;
	string	name;
	short	flags;
	int	prevlevel;
	int	nextlevel;
	tagfunc	func;

	name = List_of_Tags[tag].name;
	flags = List_of_Tags[tag].flags;
	prevlevel = List_of_Tags[tag].prevlevel;
	nextlevel = List_of_Tags[tag].nextlevel;
	oldtag = stack_check();
	if( (flags & TSF_PARAGRAPH) && block && oldtag > 0 ) /* already in block context before opening new block ? */
	{
		WARN( warnlog("implicit </%s> (block end before new block)", List_of_Tags[oldtag].name); )
		handle_close_tag( oldtag, FALSE );
	}
	else if( !(flags & TSF_NESTING) && tag == oldtag )	/* trying to nest directly ? */
	{
		WARN( warnlog("implicit </%s> (cannot nest)", name); )
		handle_close_tag( tag, FALSE );
	}
	else if( (flags & TSF_INLINE) && !block )	/* inline tag outside inline context ? */
	{
		WARN( warnlog("implicit <p> (inline tag)", List_of_Tags[tag].name); )
		handle_open_tag( TAG_p, FALSE );	/* create new paragraph, *uh* recursively */
	}
	if( !(flags & TSF_NOCLOSETAG) )
	{
		stack_push( tag );
		style.hasnotext = flags & TSF_HASNOTEXT;
	}
	if( (flags & TSF_PARAGRAPH) || (!block && (flags & TSF_INLINE)) )
	{
		block_start();
		skipspace = TRUE;
	}
	
	if( dofunc )
	{
		func = List_of_Tags[tag].openfunc;
		if( func )
			func();
		else
			skip_to_tag_end();
	}
	
	if( (flags & TSF_PARAGRAPH) && (flags & TSF_NOCLOSETAG) )
		block_end();
	WARN( if( prevlevel && prevlevel != level )
			warnlog("tag <%s> in level {%s} instead of {%s}",
				name, List_of_Levels[level], List_of_Levels[prevlevel]); )
	if( nextlevel )
		level = nextlevel;
	change_style();
}

static void handle_close_tag( int tag, int dofunc )
{
	string	name;
	short	flags;
	int	prevlevel;
	int	nextlevel;
	tagfunc	func;

	name = List_of_Tags[tag].name;
	flags = List_of_Tags[tag].flags;
	prevlevel = List_of_Tags[tag].prevlevel;
	nextlevel = List_of_Tags[tag].nextlevel;
	if( dofunc )
	{
		func = List_of_Tags[tag].closefunc;
		if( func )
			func();
		else
			skip_to_tag_end();
	}
	
	if( !(flags & TSF_NOCLOSETAG) )
	{
		WARN( if( nextlevel && nextlevel != level )
				warnlog("tag </%s> in level {%s} instead of {%s}",
					name, List_of_Levels[level], List_of_Levels[nextlevel]); )
		if( flags & TSF_PARAGRAPH )
		{
			block_end();
			skipspace = TRUE;
		}
		if( prevlevel )
			level = prevlevel;
		stack_pop( tag );
	}
	else
	{
		WARN( warnlog("<%s> should have no closing tag", name); )
	}
	change_style();
}

/*******************************************************************************************/
/* Tag Open/Close */

void tag_html_open( void )
{
	D( printf("opened html\n"); )
	skip_to_tag_end();
}

void tag_title_close( void )
{
	seg_struct *seg;
	
	seg = seglist_check_last( SEG_CMD_Text );
	if( seg )
	{
		page->title = seg->textseg;
		seglist_delete_last();
	}
	skip_to_tag_end();
}

void tag_comment( void )
{
	char	c, c1, c2;

	D( printf("%d: Skipping comment: ", linenum); )
	c = c1 = ' ';
	do
	{
		c2 = c1;
		c1 = c;
		GETCHAR( c, return )
		D( debug_putc( c ); )
		if( c == CHR_NEWLINE )
			linenum++;
	}
	while( c != CHR_TAGEND || c1 != '-' || c2 != '-' );
	D( printf("\n"); )
}

void tag_h1_open( void )
{
	para_struct	*para;

	D( printf("opened h1\n"); )
	para = &List_of_Paras[PARA_h1];
	change_paragraph( para );
	skip_to_tag_end();
}

void tag_h2_open( void )
{
	para_struct	*para;

	D( printf("opened h2\n"); )
	para = &List_of_Paras[PARA_h2];
	change_paragraph( para );
	skip_to_tag_end();
}

void tag_p_open( void )
{
	para_struct	*para;

	D( printf("opened p\n"); )
	para = &List_of_Paras[PARA_p];
	change_paragraph( para );
	skip_to_tag_end();
}

void tag_pre_open( void )
{
	para_struct	*para;

	D( printf("opened pre\n"); )
	para = &List_of_Paras[PARA_pre];
	change_paragraph( para );
	style.prelayout = TRUE;
	skip_to_tag_end();
}

void tag_b_open( void )
{
	D( printf("opened b\n"); )
	style.styleflags.fl.bold = TRUE;
	skip_to_tag_end();
}

void tag_big_open( void )
{
	D( printf("opened big\n"); )
	style.styleflags.fl.fontsize++;
	skip_to_tag_end();
}

void tag_i_open( void )
{
	D( printf("opened i\n"); )
	style.styleflags.fl.italics = TRUE;
	skip_to_tag_end();
}

void tag_small_open( void )
{
	D( printf("opened small\n"); )
	style.styleflags.fl.fontsize--;
	skip_to_tag_end();
}

void tag_tt_open( void )
{
	D( printf("opened tt\n"); )
	style.styleflags.fl.fixedwidth = TRUE;
	skip_to_tag_end();
}

void tag_a_open( void )
{
	D( printf("opened a\n"); )
	style.styleflags.fl.underlined = TRUE;
	skip_to_tag_end();
}

void tag_br( void )
{
	seglist_store( SEG_CMD_Linebreak );
	skipspace = TRUE;
	skip_to_tag_end();
}

void tag_hr( void )
{
	seglist_store( SEG_CMD_Ruler );
	skip_to_tag_end();
}

void tag_img( void )
{
	seg_struct *seg;

	seg = seglist_store( SEG_CMD_Image );
	if( seg )
	{
		seg->image = NULL;
	}
	skip_to_tag_end();
}

