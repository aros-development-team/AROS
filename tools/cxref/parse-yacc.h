#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

# ifndef YYSTYPE
#  define YYSTYPE int
#  define YYSTYPE_IS_TRIVIAL 1
# endif
# define	IDENTIFIER	257
# define	TYPE_NAME	258
# define	LITERAL	259
# define	STRING_LITERAL	260
# define	ELLIPSES	261
# define	MUL_ASSIGN	262
# define	DIV_ASSIGN	263
# define	MOD_ASSIGN	264
# define	ADD_ASSIGN	265
# define	SUB_ASSIGN	266
# define	LEFT_ASSIGN	267
# define	RIGHT_ASSIGN	268
# define	AND_ASSIGN	269
# define	XOR_ASSIGN	270
# define	OR_ASSIGN	271
# define	EQ_OP	272
# define	NE_OP	273
# define	PTR_OP	274
# define	AND_OP	275
# define	OR_OP	276
# define	DEC_OP	277
# define	INC_OP	278
# define	LE_OP	279
# define	GE_OP	280
# define	LEFT_SHIFT	281
# define	RIGHT_SHIFT	282
# define	SIZEOF	283
# define	TYPEDEF	284
# define	EXTERN	285
# define	STATIC	286
# define	AUTO	287
# define	REGISTER	288
# define	CONST	289
# define	VOLATILE	290
# define	VOID	291
# define	INLINE	292
# define	CHAR	293
# define	SHORT	294
# define	INT	295
# define	LONG	296
# define	SIGNED	297
# define	UNSIGNED	298
# define	FLOAT	299
# define	DOUBLE	300
# define	STRUCT	301
# define	UNION	302
# define	ENUM	303
# define	CASE	304
# define	DEFAULT	305
# define	IF	306
# define	ELSE	307
# define	SWITCH	308
# define	WHILE	309
# define	DO	310
# define	FOR	311
# define	GOTO	312
# define	CONTINUE	313
# define	BREAK	314
# define	RETURN	315
# define	ASM	316


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
