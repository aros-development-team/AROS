#ifndef _POSINFO_H
#define _POSINFO_H

extern int Purify_Lineno;
extern const char * Purify_Filename;
extern const char * Purify_Functionname;

typedef struct
{
    const char * filename;
    const char * functionname;
    int 	 lineno;
}
PosInfo;

typedef struct
{
    PosInfo	 caller;
    PosInfo	 called;
    const void * fp;
}
CallStackEntry;

typedef struct _CallStackNode CallStackNode;

#define PURIFY_CSNE		64
#define PURIFY_RememberDepth	4	/* Remember the 4 last functions */

struct _CallStackNode
{
    CallStackNode * next;
    CallStackEntry  entries[PURIFY_CSNE];
};

typedef struct
{
    int     nstack;
    PosInfo current;
    PosInfo stack[PURIFY_RememberDepth];
}
RememberData;

extern CallStackNode  * Purify_CallStack;
extern CallStackEntry * Purify_CurrentFrame;

#define SETPOS(pi)  ((pi)->filename     = Purify_Filename, \
		     (pi)->functionname = Purify_Functionname, \
		     (pi)->lineno       = Purify_Lineno)

void Purify_CallFunction (void);
void Purify_EnterFunction (const char * filename, const char * functionname,
			int lineno, const void * fp);
void Purify_LeaveFunction (void);
void Purify_RememberCallers (RememberData * rd);
void Purify_PrintCallers (RememberData * rd);

#endif /* _POSINFO_H */
