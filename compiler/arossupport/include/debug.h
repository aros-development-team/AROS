/*
    (C) 1995-96 AROS - The Amiga Research OS
    Debugging macros.

    This include file can be included several times !
*/
#ifndef CLIB_AROS_PROTOS_H
#   include <proto/arossupport.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h> /* For FindTask() */
#endif
#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif


#ifndef DEBUG
#   define DEBUG 0
#endif
#ifndef SDEBUG
#   define SDEBUG 0
#endif


/* Remove all macros. They get new values each time this file is
   included */
#undef D
#undef DB2
#undef ReturnVoid
#undef ReturnPtr
#undef ReturnStr
#undef ReturnInt
#undef ReturnXInt
#undef ReturnFloat
#undef ReturnSpecial
#undef ReturnBool

/* Sanity check macros */
#undef DBPRINTF
#undef THIS_FILE
#undef ASSERT(x)
#undef ASSERT_VALID_PTR(x)
#undef ASSERT_VALID_PTR_OR_NULL(x)
#undef ASSERT_VALID_TASK(t)
#undef ASSERT_VALID_PROCESS(p)

/*  Macros for "stair debugging" */
#undef SDInit
#undef EnterFunc
#undef Indent
#undef ExitFunc

#if SDEBUG

#   ifndef SDEBUG_INDENT
#	define SDEBUG_INDENT 2
#   endif

/* This is some new macros for making debug output more readable,
** by indenting for each functioncall made.
** Usage: Call the SDInit() macro before anything else in your main().
** Start the functions you want to debug with EnterFunc(bug("something"))
** and ALWAYS match these with a Returnxxxx type macro
** at the end of the func.
** Inside the func you can use the normal D(bug()) macro.
**
** To enable the macros, just add a #define SDEBUG 1
*/

/* User macro */
#define EnterFunc(x) {   			\
	struct Task *sd_task = FindTask(NULL);	\
   	int sd_spaceswritten;					\
   	for (sd_spaceswritten = 0; sd_spaceswritten < (ULONG)sd_task->tc_UserData; sd_spaceswritten ++) kprintf(" "); \
   	((ULONG)sd_task->tc_UserData) += SDEBUG_INDENT;		} \
	x

/* User macro. Add into start of your main() routine */
#   define SDInit()	\
	{ struct Task *sd_task = FindTask(NULL); sd_task->tc_UserData = NULL; }


/* Internal */
#   define Indent {   		\
	struct Task *sd_task = FindTask(NULL);	\
   	int sd_spaceswritten;					\
   	for (sd_spaceswritten = 0; sd_spaceswritten < (ULONG)sd_task->tc_UserData; sd_spaceswritten ++) kprintf(" "); }

/* Internal */
#define ExitFunc { 				\
	struct Task *sd_task = FindTask(NULL);	\
   	int sd_spaceswritten;					\
   	((ULONG)sd_task->tc_UserData) -= SDEBUG_INDENT;		\
   	for (sd_spaceswritten = 0; sd_spaceswritten < (ULONG)sd_task->tc_UserData; sd_spaceswritten ++) kprintf(" "); }


#else

#   define SDInit()
#   define Indent
#   define EnterFunc(x) D(x)
#   define ExitFunc

#endif

#if DEBUG
#   define D(x)     Indent x

#   if DEBUG > 1
#	define DB2(x)    x
#   else
#	define DB2(x)    /* eps */
#   endif


/* Sanity check macros
 *
 *	ASSERT(x)
 *		Do nothing if the expression <x> evalutates to a
 *		non-zero value, output a debug message otherwise.
 *
 *	ASSERT_VALID_PTR(x)
 *		Checks that the expression <x> points to a valid
 *		memory location, and outputs a debug message
 *		otherwise. A NULL pointer is considered VALID.
 *
 *	ASSERT_VALID_PTR_OR_NULL(x)
 *		Checks that the expression <x> points to a valid
 *		memory location, and outputs a debug message
 *		otherwise. A NULL pointer is considered NOT VALID.
 */

#define DBPRINTF kprintf

/* The trick with THIS_FILE allows us to reuse the same static string
 * instead of allocating a new copy for each invocation of these macros.
 */
#define THIS_FILE __FILE__

#define ASSERT(x) ( (x) ? 0 :					\
	( DBPRINTF("\x07%s:%ld: assertion failed: %s\n",	\
	THIS_FILE, __LINE__, #x) ) );

#define ASSERT_VALID_PTR(x) ( (((LONG)(x) > 1024) &&		\
	TypeOfMem((APTR)(x))) ? 0 :				\
	( DBPRINTF("\x07%s, %ld: bad pointer: %s = $%lx\n",	\
	THIS_FILE, __LINE__, #x, (APTR)(x)) ) );

#define ASSERT_VALID_PTR_OR_NULL(x) ( ((((APTR)(x)) == NULL) ||	\
	(((LONG)(x) > 1024) &&	TypeOfMem((APTR)(x)))) ? 0 :	\
	( DBPRINTF("\x07%s:%ld: bad pointer: %s = $%lx\n",	\
	THIS_FILE, __LINE__, #x, (APTR)(x)) ) );

#define ASSERT_VALID_TASK(t) ( ASSERT_VALID_PTR(t) &&		\
	(((t)->tc_Node.ln_Type == NT_TASK) ||			\
	(t)->tc_Node.ln_Type == NT_PROCESS) );

#define ASSERT_VALID_PROCESS(p) ( ASSERT_VALID_PTR(p) &&	\
	(((p)->pr_Task.tc_Node.ln_Type == NT_PROCESS) );


    /* return-macros. NOTE: I make a copy of the value in __aros_val, because
       the return-value might have side effects (like return x++;). */
#   define ReturnVoid(name)         { ExitFunc kprintf ("Exit " name "()\n"); return; }
#   define ReturnPtr(name,type,val) {  type __aros_val = (type)val; \
				    ExitFunc kprintf ("Exit " name "=%08lx\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnStr(name,type,val) { type __aros_val = (type)val; \
				    ExitFunc kprintf ("Exit " name "=\"%s\"\n", \
				    __aros_val); return __aros_val; }
#   define ReturnInt(name,type,val) { type __aros_val = (type)val; \
				    ExitFunc kprintf ("Exit " name "=%ld\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnXInt(name,type,val) { type __aros_val = (type)val; \
				    ExitFunc kprintf ("Exit " name "=%lx\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnFloat(name,type,val) { type __aros_val = (type)val; \
				    ExitFunc kprintf ("Exit " name "=%g\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnSpecial(name,type,val,fmt) { type __aros_val = (type)val; \
				    ExitFunc kprintf ("Exit " name "=" fmt "\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnBool(name,val)     { BOOL __aros_val = (val != 0); \
				    ExitFunc kprintf ("Exit " name "=%s\n", \
				    __aros_val ? "TRUE" : "FALSE"); \
				    return __aros_val; }
#else /* !DEBUG */
#   define D(x)     /* eps */
#   define DB2(x)     /* eps */

#   define ASSERT(x)
#   define ASSERT_VALID_PTR(x)
#   define ASSERT_VALID_PTR_OR_NULL(x)
#   define ASSERT_VALID_TASK(t)
#   define ASSERT_VALID_PROCESS(p)

#   define ReturnVoid(name)                 return
#   define ReturnPtr(name,type,val)         return val
#   define ReturnStr(name,type,val)         return val
#   define ReturnInt(name,type,val)         return val
#   define ReturnXInt(name,type,val)        return val
#   define ReturnFloat(name,type,val)       return val
#   define ReturnSpecial(name,type,val,fmt) return val
#   define ReturnBool(name,val)             return val
#endif /* DEBUG */

#ifndef AROS_DEBUG_H
#define AROS_DEBUG_H

#define bug	 kprintf

#define aros_print_not_implemented(name) \
	kprintf("The function %s is not implemented.\n", name);

#define ALIVE kprintf("%s - %s line %d\n",__FILE__,__FUNCTION__,__LINE__);

#endif /* AROS_DEBUG_H */
