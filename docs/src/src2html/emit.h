#include <stdio.h>

FILE * out;
char * outname;
FILE * toc;

void emit_init (void);
void emit_exit (void);
void emit (int token, ...);
extern int lasttoken;
extern int pendingspace;
extern int emitcount;
extern int appendix;
extern int isnewtext;

extern int chapter, section, subsection;

enum emitmode
{
    em_html,
};

enum listmode
{
    lm_none,
    lm_description,
    lm_itemize,
    lm_enumeration,
    lm_emph,
    lm_new,
    lm_methods,
    lm_tags,
    lm_indent
};

enum lastemittype
{
    let_char,
    let_space,
    let_nl,
    let_par,
    let_special
};

extern enum emitmode emode;
extern enum listmode lmode;
extern enum lastemittype lastemit;

const char * getlref (const char * name);
void deflref (const char * name, const char * str);
extern void emit_char (int c);
extern void emit_char_always (int c);
extern void emit_string (const char * str);
extern void emit_special (const char * fmt, ...);
extern void emit_nl (void);
extern void emit_par (void);
extern void emit_space (void);

