#ifndef DEBUG_H
#define DEBUG_H

/*
** $PROJECT: C debugging macros
**
** $VER: debug.h 1.7 (04.09.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
*/

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __SASC
#define DEBUGFUNC      __FUNC__
#elif __GNUC__
#define DEBUGFUNC      __FUNCTION__
#else
#define DEBUGFUNC
#endif

#ifdef DEBUG

#define KPrintF kprintf
#ifdef __AROS__
#include <aros/debug.h>
#else
void KPrintF(const char *fmt, ...);
#endif

#define D(x)                  { x; }
#define bug                   kprintf
#define fbug                  kprintf
#define DEBUG_TAGLIST(x)      { struct TagItem *tstate = x; \
				struct TagItem *tag; \
				bug("TagList : \n"); \
				while((tag = NextTagItem(&tstate))) \
				  bug("{0x%08lx,0x%08lx}\n",tag->ti_Tag,tag->ti_Data); \
			      }
#define DEBUG_EXECLIST(x)     { struct Node *node; bug("ExecList : \n"); \
				for(node = (x)->lh_Head; node->ln_Succ != NULL; node = node->ln_Succ) \
				   bug("0x%08lx : %s\n",node,node->ln_Name); \
			      }

#define DBLINE    bug(PROJECTNAME " " __FILE__ "(%4ld):" DEBUGFUNC "() :",__LINE__)

#define DC(x)
#define DB(x)     { DBLINE; bug x; }
#define DBL(l,x)  { DBLINE; bug x; }
#define ENTERING  D(fbug("enter " DEBUGFUNC "()\n"))
#define LEAVING   D(fbug("leave " DEBUGFUNC "()\n"))
#define ENTERLVL(l) ENTERING
#define LEAVELVL(l) LEAVING
#define DTL(x)    D({ bug(__FILE__ "(%4ld):" DEBUGFUNC "() ",__LINE__); \
		    DEBUG_TAGLIST(x); \
		  })
#define DDL(x)    D({ bug(__FILE__ "(%4ld):" DEBUGFUNC "() ",__LINE__); \
		    DEBUG_EXECLIST(x); \
		  })
#define DA(e, x)   D({ if((e)) { bug("Fault: "); bug x; } })
#define DIA(e, x)
#define DELAY(x)  {if(FindTask(NULL)->tc_Node.ln_Type == NT_PROCESS) Delay(x);}

#ifndef PROJECTNAME
#define PROJECTNAME
#endif

#else

#define bug
#define DC(x)
#define D(x)
#define DB(x)
#define DBL(l,x)
#define DA(x,e)
#define DIA(x,expr)
#define DELAY(x)
#define DTL(x)
#define DDL(x)
#define ENTERING
#define LEAVING
#define ENTERLVL(l)
#define LEAVELVL(l)

#endif

#ifdef __cplusplus
};
#endif

#endif   /* DEBUG_H */


