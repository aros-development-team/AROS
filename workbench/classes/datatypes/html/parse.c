/*
    Copyright © 2004, Martin Gierich. All rights reserved.
    Licensed under the terms of the AROS Public License (APL)
    $Id$

    Desc: HTML parser with syntax checker
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"
#include "parse.h"
#include "tables.h"

extern string List_of_SegCmds[SEG_CMD_MAX];

/*******************************************************************************************/
/* Prototypes */

int create_hash_table( u_short hashlist[], void *namelist, int num, int size, int havecase );

/* Output */
WARN( static void	warnlog( parse_struct *pdata, string text, ... ); )
WARN( static seg_struct*	seglist_store_text( parse_struct *pdata, string str ); )
static seg_struct *	seglist_store( parse_struct *pdata, u_char cmd );
static seg_struct *	seglist_check_last( parse_struct *pdata, u_char segcmd );
static void		seglist_delete_last( parse_struct *pdata );
static void		seglist_end( parse_struct *pdata );
static void		seglist_free( parse_struct *pdata );
static int		strpool_expand( parse_struct *pdata );

/* String Parsing */
D( static void	debug_putc( char c ); )
static char	skip_spaces( parse_struct *pdata, char endchar, int stop );
static char	copy_text( parse_struct *pdata, char endchar );
static char	escape_to_char( parse_struct *pdata );
static int	get_number( parse_struct *pdata, int radix, char *lastchar );
static int	parse_tag( parse_struct *pdata );
static void	skip_to_tag_end( parse_struct *pdata );

/* Style Stack Handling */
static void	styles_reset( parse_struct *pdata );
static void	change_style( parse_struct *pdata );
static void	change_paragraph( parse_struct *pdata, para_struct *para );
static void	stack_push( parse_struct *pdata, int tag );
static int	stack_pop( parse_struct *pdata, int searchtag );
static int	stack_check( parse_struct *pdata );

/* Tag and Paragraph */
static void	block_start( parse_struct *pdata );
static void	block_end( parse_struct *pdata );
static void	handle_open_tag( parse_struct *pdata, int tag, int dofunc );
static void	handle_close_tag( parse_struct *pdata, int tag, int dofunc );

/*******************************************************************************************/
/* Private Parse Data */
struct _parse_struct
{
	void		*userdata;

	/* page */
	page_struct	*page;

	/* data arrays */
	int		MAX_Tag_Length;
	u_short		Hash_of_Tags[MAX_Tags];
	int		MAX_Attr_Length;
	u_short		Hash_of_Attrs[MAX_Attrs];
	int		MAX_Esc_Length;
	u_short		Hash_of_Escs[MAX_Escs];

	string		inbuf;
	int		inbufbytes;

	/* output buffers */
	string		strpool;
	string		strpoolpos;
	string		strpoolend;
	seg_struct	*seglistpos;
	int		seglistremain;
WARN(
	string		warnpool;
	string		warnpoolpos;
	string		warnpoolend;
)

	/* state tracking */
	int		linenum;
	int		level;
	int		block;
	int		skipspace;

	/* state stack */
	stack_struct	stack[STACKSIZE];
	int		stackpos;
	stack_struct	style;
	style_flags	laststyle;
	para_struct	*lastpara;
};

/*******************************************************************************************/
/* Main parse */

page_struct * parse_init( void *mempool )
{
	page_struct	*page;
	parse_struct	*pdata;

	D( printf("structsize tag %d esc %d stack %d page %d seg %d style %d\n", sizeof(tag_struct),
		sizeof(esc_struct), sizeof(stack_struct), sizeof(page_struct), sizeof(seg_struct), sizeof(style_flags)); )

	page = MALLOC( mempool, sizeof( page_struct ) );
	if( !page )
		return NULL;
	D( printf("page: %p\n", page); )
	page->mempool = mempool;
	page->title = NULL;
	page->seglist = NULL;

	pdata = page->pdata = MALLOC( mempool, sizeof( parse_struct ) );
	if( !pdata )
		return NULL;
	pdata->page = page;
	pdata->seglistpos = NULL;
	pdata->seglistremain = 0;
	pdata->strpool = NULL;
	pdata->strpoolpos = NULL;
	pdata->strpoolend = NULL;
	WARN(
		pdata->warnpool = pdata->warnpoolpos = MALLOC( mempool, WARNPOOLSIZE );
		if( !pdata->warnpool )
			return NULL;
		pdata->warnpoolend = pdata->warnpool + WARNPOOLSIZE;
	)

	pdata->MAX_Tag_Length = create_hash_table(
		pdata->Hash_of_Tags, List_of_Tags, MAX_Tags, sizeof(tag_struct), FALSE );
	pdata->MAX_Attr_Length = create_hash_table(
		pdata->Hash_of_Attrs, List_of_Attrs, MAX_Attrs, sizeof(attr_struct), FALSE );
	pdata->MAX_Esc_Length = create_hash_table(
		pdata->Hash_of_Escs, List_of_Escs, MAX_Escs, sizeof(esc_struct), TRUE );

	pdata->linenum = 1;
	pdata->level = LEVEL_top;
	pdata->block = FALSE;
	pdata->skipspace = TRUE;
	pdata->stackpos = 0;

	styles_reset( pdata );

	return page;
}

int parse_do( page_struct *page, string myinbuf, int myinbufbytes )
{
	parse_struct	*pdata;
	int	tag;
	char	c;

	pdata = page->pdata;
	pdata->inbuf = myinbuf;
	pdata->inbufbytes = myinbufbytes;
	do
	{
		D( printf("\n"); )
		if( pdata->style.hasnotext )
			c = skip_spaces( pdata, CHR_TAGSTART, FALSE );
		else
		{
			if( pdata->level == LEVEL_inline )
			{
				c = copy_text( pdata, CHR_TAGSTART );
			}
			else
			{
				c = skip_spaces( pdata, CHR_TAGSTART, TRUE );
				if( c && c != CHR_TAGSTART)
				{
					WARN( warnlog( pdata, "implicit <p> (inline text)" ); )
					UNGETCHAR( c )
					handle_open_tag( pdata, TAG_p, FALSE );
					pdata->skipspace = TRUE;
					c = copy_text( pdata, CHR_TAGSTART );
				}
			}
		}
		if( !c )
			break;
		GETCHAR( c, break )
		if( c != '/' )
		{
			UNGETCHAR( c )
			tag = parse_tag( pdata );
			D( printf("%d: Tag <%s> (%d) opening level {%s} stack %d\n",
				pdata->linenum, tag>=0 ? List_of_Tags[tag].name:"?",
				tag, List_of_Levels[pdata->level], pdata->stackpos); )
			if( tag >= 0 )
				handle_open_tag( pdata, tag, TRUE );
		}
		else
		{
			tag = parse_tag( pdata );
			D( printf("%d: Tag </%s> (%d) closing level {%s} stack %d\n",
				pdata->linenum, tag>=0 ? List_of_Tags[tag].name:"?",
				tag, List_of_Levels[pdata->level], pdata->stackpos); )
			if( tag >= 0 )
				handle_close_tag( pdata, tag, TRUE );
		}
		D( printf("%d: New level {%s} (%d) hasnotext %d\n",
			pdata->linenum, List_of_Levels[pdata->level], pdata->level, pdata->style.hasnotext); )
	}
	while( pdata->inbufbytes > 0 );
	return TRUE;
}

int parse_end( page_struct *page )
{
	parse_struct	*pdata;

	pdata = page->pdata;
	if( pdata->block )
	{
		WARN( warnlog( pdata, "implicit block end at end" ); )
		block_end( pdata );
	}
	if( pdata->stackpos != 0 )
	{
		WARN( warnlog( pdata, "stack not empty at end" ); )
		stack_pop( pdata, TAG_html );
	}
	WARN({
		string		str;
		
		pdata->warnpoolpos[0] = '\0';	/* empty string at end */
		str=pdata->warnpool;
		if( str[0] )
		{
			seglist_store( pdata, SEG_CMD_Linebreak );
			seglist_store( pdata, SEG_CMD_Blockstart );
			seglist_store( pdata, SEG_CMD_Ruler );
			seglist_store( pdata, SEG_CMD_Blockend );
			seglist_store_text( pdata, "Warnings:" );
			while( str[0] )
			{
				seglist_store_text( pdata, str );
				str += strlen( str ) + 1;
			}
		}
		D(
			printf("\nWarnings:\n");
			for(str=pdata->warnpool; str<pdata->warnpoolpos; str++)
			{
				if(*str >= ' ') printf("%c", *str);
				else printf("[%d]\n", *str);
			}
			printf("\n\n");
		)
	})
	seglist_end( pdata );
	D( printf("strpool: %p < %p < %p\n", pdata->strpool, pdata->strpoolpos, pdata->strpoolend); )
	D( printf("page: %p title %p seglist %p\n", page, page->title, page->seglist); )
	return TRUE;
}

void parse_free( page_struct *page )
{
	parse_struct	*pdata;

	pdata = page->pdata;
	seglist_free( pdata );
	D( printf("freed seglist\n"); )
	WARN( MFREE( page->mempool, pdata->warnpool ); )
	MFREE( page->mempool, pdata->strpool );
	D( printf("freed strpool\n"); )
}

/*******************************************************************************************/
/* Hash Handling */

int create_hash_table( u_short hashlist[], void *namelist, int num, int size, int havecase )
{
	int		i, j, maxlen;
	char		c;
	string		str;
	string		*strptr;
	u_short		hash;

	maxlen = 0;
	for( i=0; i<num; i++ )
	{
		strptr = (string *)namelist;
		str = *strptr;
		j = strlen( str );
		if( j > maxlen )
			maxlen = j;
		hash = 0;
		if( havecase )
			while( (c = *str++) )
			{
				hash = HASHCASE( hash, c );
			}
		else
			while( (c = *str++) )
			{
				hash = HASHNOCASE( hash, c );
			}
		hashlist[i] = hash;
//		D( str = *strptr; printf("Hash of %2d %p %p list %p [%s]:\t 0x%04x\n", i, strptr, str, &hashlist[i], str /*(string)namelist*/, (int)hashlist[i]); )
		namelist += size;
	}
	D(
		for( i=0; i<num-1; i++ )
		{
			for( j=i+1; j<num; j++ )
			{
			        /* printf("Hash of %2d=%04x %2d=%04x\n", i, hashlist[i], j, hashlist[j]); */
				if( hashlist[i] == hashlist[j] )
					printf("Duplicate Tag Hash at %d <-> %d !!!\n", i, j);
			}
		}
	)
	D( printf("maxlen %d num %d\n", maxlen, num); )
	return maxlen;
}

/*******************************************************************************************/
/* Output */

WARN(
static void warnlog( parse_struct *pdata, string format, ... )
{
	va_list		args;
	string		poolpos;
	int		poolsize, i;

	va_start( args, format );
	D( printf("*** line %2d: %s\n", pdata->linenum, format); )
	/*fprintf( stderr, "*** line %2d: ", pdata->linenum );
	vfprintf( stderr, format, args );
	fprintf( stderr, "\n");*/

	poolpos = pdata->warnpoolpos;
	poolsize = pdata->warnpoolend - poolpos;
	if( poolsize > 100 )
	{
		i = snprintf( poolpos, poolsize, "line %2d: ", pdata->linenum );
		poolpos += i;
		poolsize -= i;
		i = vsnprintf( poolpos, poolsize, format, args );
		i ++;	/* leading zero */
		poolpos += i;
		poolsize -= i;
		pdata->warnpoolpos = poolpos;
		if( poolsize <= 100 )
		{
			D( printf("*** warnlog overflow\n"); )
			i = snprintf( poolpos, poolsize, "..." );
			i ++;
			pdata->warnpoolpos += i;
		}
	}
	va_end( args );
}
)

WARN(
static seg_struct* seglist_store_text( parse_struct *pdata, string str )
{
	seg_struct	*seg;

	seg = seglist_store( pdata, SEG_CMD_Text );
	if( !seg )
		return NULL;
	seg->textlen = strlen( str );
	seg->textseg = str;
	seglist_store( pdata, SEG_CMD_Linebreak );
	return seg;
}
)

static seg_struct* seglist_store( parse_struct *pdata, u_char cmd )
{
	seg_struct	*seg;

//	D( printf("---1 cmd %d seglistpos %p remain %d [%s]\n", cmd,
//		pdata->seglistpos, pdata->seglistremain, List_of_SegCmds[cmd & SEG_CMD_MASK]); )
	pdata->seglistremain --;	/* make sure a last one remains */
	if( pdata->seglistremain <= 0 )
	{
		seg = MALLOC( pdata->page->mempool, SEGLISTSIZE * sizeof(seg_struct) );
		if( !seg )
		{
			pdata->seglistremain = 0;	/* keep error state */
			return NULL;
		}
		if( !pdata->page->seglist )
		{	/* initial */
			D(printf("initial seglist alloc %p size 0x%x\n", seg, SEGLISTSIZE * sizeof(seg_struct));)
			pdata->page->seglist = seg;
		}
		else
		{	/* add another list */
			D(printf("more seglist alloc %p\n", seg);)
			pdata->seglistpos->cmd = SEG_CMD_Next;
			pdata->seglistpos->next = seg;
		}
		pdata->seglistremain = SEGLISTSIZE - 1;	/* subtract this one */
		pdata->seglistpos = seg;
		D( printf("seglist: %p < %p < %d\n", pdata->page->seglist, pdata->seglistpos, pdata->seglistremain); )
	}
	seg = pdata->seglistpos ++;
	seg->cmd = cmd;
	return seg;
}

static seg_struct* seglist_check_last( parse_struct *pdata, u_char segcmd )
{
	seg_struct	*seg;

	seg = pdata->seglistpos;
	seg --;
	if( seg->cmd == segcmd )
		return seg;
	else
		return NULL;
}

static void seglist_delete_last( parse_struct *pdata )
{
	pdata->seglistpos --;
	pdata->seglistremain ++;
}

static void seglist_end( parse_struct *pdata )
{
	seg_struct	*seg;

	seg = pdata->seglistpos;
	seg --;
	seg->cmd |= SEG_CMD_LAST;
	D( printf("seglist: %p < %p < %d\n", pdata->page->seglist, pdata->seglistpos, pdata->seglistremain); )
}

static void seglist_free( parse_struct *pdata )
{
	MFREE( page, pdata->page->seglist );
}


static int strpool_expand( parse_struct *pdata )
{
	pdata->strpool = pdata->strpoolpos = MALLOC( pdata->page->mempool, STRINGPOOLSIZE );
	if( !pdata->strpool )
		return FALSE;
	pdata->strpoolend = pdata->strpool + STRINGPOOLSIZE;
	D( printf("(strpool %p)", pdata->strpool); )
	return TRUE;
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

static char skip_spaces( parse_struct *pdata, char endchar, int stop )
{
	char	c;
	char	str[20];
	int	i, code;

	D( printf("%d: Skip Spaces to %c: [", pdata->linenum, endchar); )
	i = 0;
	do
	{
		GETCHAR( c, return 0 )
		D( debug_putc( c ); )
		if( c == endchar )
			break;
		if( c == CHR_NEWLINE )
			pdata->linenum++;
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
	WARN( if( i ) warnlog( pdata, "unexpected chars: %s", str ); )
	return c;
}

static char copy_text( parse_struct *pdata, char endchar )
{
	char	c;
	string	strstart, strpos;
	int	space, i, code, maxlen;
	int	prelayout;

	space = pdata->skipspace;
	prelayout = pdata->style.prelayout;
	do
	{
		D( printf("%d: Copy Text (sp %d) to %c: [", pdata->linenum, space, endchar); )
		strstart = strpos = pdata->strpoolpos;
		maxlen = pdata->strpoolend - strstart;
		if( maxlen < 10 )
		{
			i = strpool_expand( pdata );
			if( !i )
				return 0;
			strstart = strpos = pdata->strpoolpos;
			maxlen = pdata->strpoolend - strstart;
		}
		c = 0;
		while( maxlen > 0 )
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
					pdata->linenum++;
					if( prelayout )
						break;
				}
				if( prelayout )
				{
					if( c == '\t' && maxlen >= 8 )
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
					c = escape_to_char( pdata );
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
					WARN( warnlog( pdata, "char %c needs escape", c ); )
					PUTCHAR( c )
				}
			}
		}
		D( printf("]\n"); )
		pdata->strpoolpos = strpos;
		if( strpos > strstart )
		{
			seg_struct *seg;
			int length;
			
			if( pdata->skipspace && (strpos - strstart) == 1 && strstart[0] == ' ' && !prelayout )
				return c;
			length = strpos - strstart;
			D(	printf("String %2d (max %d): [", length, maxlen);
				for(i=0; i<length; i++) printf("%c", strstart[i]);
				printf("]\n"); )
			pdata->skipspace = FALSE;
			seg = seglist_store( pdata, SEG_CMD_Text );
			if( !seg )
				return 0;
			seg->textlen = length;
			seg->textseg = strstart;
		}
		if( prelayout && (c == CHR_NEWLINE) )
			seglist_store( pdata, SEG_CMD_Linebreak );
	}
	while( c != endchar );
	return c;
}

static char escape_to_char( parse_struct *pdata )
{
	char	c;
	int	i=0, res, code;
	char	str[pdata->MAX_Esc_Length+1];
	u_short	hash;
	u_short	*hashlist;

	GETCHAR( c, return '?' )
	if( c == '#' )
	{
		GETCHAR( c, return '?' )
		if( LOWERCASE(c)=='x' )
		{
			i = get_number( pdata, 16, &c );
		}
		else if( c>='0' && c<='9' )
		{
			UNGETCHAR( c )
			i = get_number( pdata, 10, &c );
		}
	}
	else
	{
		UNGETCHAR( c )
		hash = 0;
		for( i=0; i<=pdata->MAX_Esc_Length; i++ )
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
			hashlist = pdata->Hash_of_Escs;
			for( i=0; i<MAX_Escs; i++ )
			{
				if( hashlist[i] == hash )
				{
					res = strcmp( str, List_of_Escs[i].name );
					if( res==0 )
						return List_of_Escs[i].value;
					D( printf("Hash failure\n"); )
				}
			}
			WARN( warnlog( pdata, "unknown escape %s", str ); )
			return '?';
		}
	}
	if( c==';' )
		return (char)i;
	WARN( warnlog( pdata, "bad end of escape (%c)", c ); )
	UNGETCHAR( c )
	return '?';
}

static int get_number( parse_struct *pdata, int radix, char *lastchar )
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

static int parse_tag( parse_struct *pdata )
{
	char	c;
	int	tag;
	int	i, res, code;
	char	str[pdata->MAX_Tag_Length+1];
	u_short	hash;
	u_short	*hashlist;

	D( printf("%d: Parse Tag: ", pdata->linenum); )
	hash = 0;
	for( i=0; i<=pdata->MAX_Tag_Length; i++ )
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
	hashlist = pdata->Hash_of_Tags;
	for( tag=0; tag<MAX_Tags; tag++ )
	{
		if( hashlist[tag] == hash )
		{
			res = strcasecmp( str, List_of_Tags[tag].name );
//			D( printf("tag: %d res: %d\n", tag, res); )
			if( res==0 )
				return tag;
			D( printf("Hash failure\n"); )
		}
	}
	WARN( warnlog( pdata, "skipping unknown tag <%s>", str ); )
	skip_to_tag_end( pdata );
	return -1;
}

static void skip_to_tag_end( parse_struct *pdata )
{
	char	c;
	int	quote, code;

	D( printf("%d: Skip to Tag End: ", pdata->linenum); )
	quote = FALSE;
	do
	{
		GETCHAR( c, return )
		D( debug_putc( c ); )
		code = charlist[(u_char)c];
		if( ISNORMAL( code ) )
			continue;
		if( c == CHR_NEWLINE )
			pdata->linenum++;
		if( c == CHR_QUOTE )
		{
			quote = !quote;
			D( printf("#"); )
			continue;
		}
	}
	while( c != CHR_TAGEND );
	WARN( if( quote ) warnlog( pdata, "unmatched quote" ); )
	D( printf("\n"); )
	return;
}

/*******************************************************************************************/
/* Style Stack Handling */

static void styles_reset( parse_struct *pdata )
{
	seg_struct		*seg;

	pdata->style.hasnotext = TRUE;
	pdata->style.prelayout = FALSE;
	pdata->style.styleflags.value = 0;
	pdata->laststyle.value = 0;
	pdata->style.styleflags.fl.fontsize = 8;
	pdata->laststyle.fl.fontsize = 8;
	seg = seglist_store( pdata, SEG_CMD_Softstyle );
	if( seg )
		seg->styleflags = pdata->style.styleflags;

	pdata->lastpara = NULL;
	change_paragraph( pdata, &List_of_Paras[PARA_p] );
}

static void change_style( parse_struct *pdata )
{
	seg_struct		*seg;

	if( pdata->block && pdata->laststyle.value != pdata->style.styleflags.value )
	{
		pdata->laststyle = pdata->style.styleflags;
		seg = seglist_check_last( pdata, SEG_CMD_Softstyle );
		if( seg )
		{
//			D( printf("*****\t\t\texisting style change to %04x\n", pdata->style.styleflags.value); )
			seg->styleflags = pdata->style.styleflags;
		}
		else
		{
//			D( printf("*****\t\t\tstyle change to %04x\n", pdata->style.styleflags.value); )
			seg = seglist_store( pdata, SEG_CMD_Softstyle );
			if( seg )
				seg->styleflags = pdata->style.styleflags;
		}
	}
}

static void change_paragraph( parse_struct *pdata, para_struct *para )
{
	seg_struct		*seg;

	pdata->style.styleflags = para->styleflags;
	change_style( pdata );
	if( pdata->lastpara != para )
	{
		pdata->lastpara = para;
//		D( printf("*****\t\t\tpara change to %02x\n", para->paraflags.value); )
		seg = seglist_store( pdata, SEG_CMD_Parastyle );
		if( seg )
		{
			seg->paraflags = para->paraflags;
		}
	}
}

static void stack_push( parse_struct *pdata, int tag )
{
	int		pos;
	stack_struct	*stack;

	pos = pdata->stackpos;
	stack = &pdata->stack[0];
	if( pos < STACKSIZE )
	{
		stack[pos]	= pdata->style;
		stack[pos].tag	= tag;
		stack[pos].line	= pdata->linenum;
		pdata->stackpos++;
//		D( printf("*****\t\t\tpush tag <%s> style %04x pos %d\n", List_of_Tags[tag].name, pdata->style.styleflags.value, pdata->stackpos); )
	}
	else
	{
		WARN( warnlog( pdata, "stack overflow" ); )
	}
}

static int stack_pop( parse_struct *pdata, int searchtag )
{
	int	tag, pos, count;
	stack_struct	*stack;

	pos = pdata->stackpos;
	stack = &pdata->stack[0];
	count = 0;
	while( pos > 0 )
	{
		pos--;
		tag = stack[pos].tag;
//		D( printf("*****\t\t\tSearch tag <%s> pos %d\n", List_of_Tags[tag].name, pos); )
		if( tag == searchtag )
		{
			pdata->stackpos = pos;
			pdata->style = stack[pos];
//			D( printf("*****\t\t\tpop tag <%s> style %04x pos %d\n", List_of_Tags[tag].name, pdata->style.styleflags.value, pdata->stackpos); )
			return tag;
		}
		else
		{
			WARN( warnlog( pdata, "skipped tag <%s> from line %d in stack trying to find </%s>",
				List_of_Tags[tag].name, stack[pos].line, List_of_Tags[searchtag].name ); )
		}
		count++;
	}
	WARN(	if( !count )
			warnlog( pdata, "stack underflow" );
		else
			warnlog( pdata, "missing opening tag for </%s>", List_of_Tags[searchtag].name );
	)
	return -1;
}

static int stack_check( parse_struct *pdata )
{
	int	pos;
	stack_struct	*stack;

	pos = pdata->stackpos;
	stack = &pdata->stack[0];
	if( pos > 0 )
		return stack[pos-1].tag;
	else
		return -1;
}

/*******************************************************************************************/
/* Tag Open/Close */

static void block_start( parse_struct *pdata )
{
	if( pdata->block )
	{
		WARN( warnlog( pdata, "implicit block end" ); )
		block_end( pdata );
	}
	D( printf("------- start -------\n"); )
	seglist_store( pdata, SEG_CMD_Blockstart );
	pdata->block = TRUE;
}

static void block_end( parse_struct *pdata )
{
	seg_struct *seg;

	if( pdata->block )
	{
		D( printf("-------  end  -------\n"); )
		/* remove single space at end */
		seg = seglist_check_last( pdata, SEG_CMD_Text );
		if( seg )
		{
			if( seg->textlen == 1 && seg->textseg[0] == ' ' )
				seglist_delete_last( pdata );
		}
		seglist_store( pdata, SEG_CMD_Blockend );
		pdata->block = FALSE;
	}
	else
	{
		WARN( warnlog( pdata, "block end without start" ); )
	}
}

static void handle_open_tag( parse_struct *pdata, int tag, int dofunc )
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
	oldtag = stack_check( pdata );
	if( (flags & TSF_PARAGRAPH) && pdata->block && oldtag > 0 ) /* already in block context before opening new block ? */
	{
		WARN( warnlog( pdata, "implicit </%s> (block end before new block)", List_of_Tags[oldtag].name ); )
		handle_close_tag( pdata, oldtag, FALSE );
	}
	else if( !(flags & TSF_NESTING) && tag == oldtag )	/* trying to nest directly ? */
	{
		WARN( warnlog( pdata, "implicit </%s> (cannot nest)", name ); )
		handle_close_tag( pdata, tag, FALSE );
	}
	else if( (flags & TSF_INLINE) && !pdata->block )	/* inline tag outside inline context ? */
	{
		WARN( warnlog( pdata, "implicit <p> (inline tag)", List_of_Tags[tag].name ); )
		handle_open_tag( pdata, TAG_p, FALSE );	/* create new paragraph, *uh* recursively */
	}
	if( !(flags & TSF_NOCLOSETAG) )
	{
		stack_push( pdata, tag );
		pdata->style.hasnotext = flags & TSF_HASNOTEXT;
	}
	if( (flags & TSF_PARAGRAPH) || (!pdata->block && (flags & TSF_INLINE)) )
	{
		block_start( pdata );
		pdata->skipspace = TRUE;
	}
	
	if( dofunc )
	{
		func = List_of_Tags[tag].openfunc;
		if( func )
			func( pdata );
		else
			skip_to_tag_end( pdata );
	}
	
	if( (flags & TSF_PARAGRAPH) && (flags & TSF_NOCLOSETAG) )
		block_end( pdata );
	WARN( if( prevlevel && prevlevel != pdata->level )
			warnlog( pdata, "tag <%s> in level {%s} instead of {%s}",
				name, List_of_Levels[pdata->level], List_of_Levels[prevlevel] ); )
	if( nextlevel )
		pdata->level = nextlevel;
	change_style( pdata );
}

static void handle_close_tag( parse_struct *pdata, int tag, int dofunc )
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
			func( pdata );
		else
			skip_to_tag_end( pdata );
	}
	
	if( !(flags & TSF_NOCLOSETAG) )
	{
		WARN( if( nextlevel && nextlevel != pdata->level )
				warnlog( pdata, "tag </%s> in level {%s} instead of {%s}",
					name, List_of_Levels[pdata->level], List_of_Levels[nextlevel] ); )
		if( flags & TSF_PARAGRAPH )
		{
			block_end( pdata );
			pdata->skipspace = TRUE;
		}
		if( prevlevel )
			pdata->level = prevlevel;
		stack_pop( pdata, tag );
	}
	else
	{
		WARN( warnlog( pdata, "<%s> should have no closing tag", name ); )
	}
	change_style( pdata );
}

/*******************************************************************************************/
/* Tag Open/Close */

void tag_html_open( parse_struct *pdata )
{
	D( printf("opened html\n"); )
	skip_to_tag_end( pdata );
}

void tag_title_close( parse_struct *pdata )
{
	seg_struct *seg;
	
	seg = seglist_check_last( pdata, SEG_CMD_Text );
	if( seg )
	{
		if( pdata->strpoolpos < pdata->strpoolend )
			*pdata->strpoolpos++ = '\0';
		pdata->page->title = seg->textseg;
		seglist_delete_last( pdata );
	}
	skip_to_tag_end( pdata );
}

void tag_comment( parse_struct *pdata )
{
	char	c, c1, c2;

	D( printf("%d: Skipping comment: ", pdata->linenum); )
	c = c1 = ' ';
	do
	{
		c2 = c1;
		c1 = c;
		GETCHAR( c, return )
		D( debug_putc( c ); )
		if( c == CHR_NEWLINE )
			pdata->linenum++;
	}
	while( c != CHR_TAGEND || c1 != '-' || c2 != '-' );
	D( printf("\n"); )
}

void tag_h1_open( parse_struct *pdata )
{
	para_struct	*para;

	D( printf("opened h1\n"); )
	para = &List_of_Paras[PARA_h1];
	change_paragraph( pdata, para );
	skip_to_tag_end( pdata );
}

void tag_h2_open( parse_struct *pdata )
{
	para_struct	*para;

	D( printf("opened h2\n"); )
	para = &List_of_Paras[PARA_h2];
	change_paragraph( pdata, para );
	skip_to_tag_end( pdata );
}

void tag_p_open( parse_struct *pdata )
{
	para_struct	*para;

	D( printf("opened p\n"); )
	para = &List_of_Paras[PARA_p];
	change_paragraph( pdata, para );
	skip_to_tag_end( pdata );
}

void tag_pre_open( parse_struct *pdata )
{
	para_struct	*para;

	D( printf("opened pre\n"); )
	para = &List_of_Paras[PARA_pre];
	change_paragraph( pdata, para );
	pdata->style.prelayout = TRUE;
	skip_to_tag_end( pdata );
}

void tag_b_open( parse_struct *pdata )
{
	D( printf("opened b\n"); )
	pdata->style.styleflags.fl.bold = TRUE;
	skip_to_tag_end( pdata );
}

void tag_big_open( parse_struct *pdata )
{
	D( printf("opened big\n"); )
	pdata->style.styleflags.fl.fontsize++;
	skip_to_tag_end( pdata );
}

void tag_i_open( parse_struct *pdata )
{
	D( printf("opened i\n"); )
	pdata->style.styleflags.fl.italics = TRUE;
	skip_to_tag_end( pdata );
}

void tag_small_open( parse_struct *pdata )
{
	D( printf("opened small\n"); )
	pdata->style.styleflags.fl.fontsize--;
	skip_to_tag_end( pdata );
}

void tag_tt_open( parse_struct *pdata )
{
	D( printf("opened tt\n"); )
	pdata->style.styleflags.fl.fixedwidth = TRUE;
	skip_to_tag_end( pdata );
}

void tag_a_open( parse_struct *pdata )
{
	D( printf("opened a\n"); )
	pdata->style.styleflags.fl.underlined = TRUE;
	skip_to_tag_end( pdata );
}

void tag_br( parse_struct *pdata )
{
	seglist_store( pdata, SEG_CMD_Linebreak );
	pdata->skipspace = TRUE;
	skip_to_tag_end( pdata );
}

void tag_hr( parse_struct *pdata )
{
	seglist_store( pdata, SEG_CMD_Ruler );
	skip_to_tag_end( pdata );
}

void tag_img( parse_struct *pdata )
{
	seg_struct *seg;

	seg = seglist_store( pdata, SEG_CMD_Image );
	if( seg )
	{
		seg->image = NULL;
	}
	skip_to_tag_end( pdata );
}

