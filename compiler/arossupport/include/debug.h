/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Debugging macros.
    This include file can be included several times!
*/

#ifndef CLIB_AROSSUPPORT_PROTOS_H
#   include <proto/arossupport.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h> /* For FindTask() */
#endif
#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif
#ifndef EXEC_ALERTS_H
#   include <exec/alerts.h>
#endif

#include <string.h>

#ifndef DEBUG
#   define DEBUG 0
#endif
#ifndef SDEBUG
#   define SDEBUG 0
#endif
#ifndef ADEBUG
#   define ADEBUG 0
#endif
#ifndef MDEBUG
#   define MDEBUG 0
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


/*  Macros for "stair debugging" */
#undef SDInit
#undef EnterFunc
#undef Indent
#undef ExitFunc

/* StegerG */
#undef SDEBUG
#define SDEBUG 0

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
#define EnterFunc(x) do {   			\
	struct Task *sd_task = FindTask(NULL);	\
   	int sd_spaceswritten;					\
   	for (sd_spaceswritten = 0; sd_spaceswritten < (ULONG)sd_task->tc_UserData; sd_spaceswritten ++) kprintf(" "); \
   	((ULONG)sd_task->tc_UserData) += SDEBUG_INDENT; } while(0); \
	x 
	

/* User macro. Add into start of your main() routine */
#   define SDInit()	\
	do { FindTask(NULL)->tc_UserData = NULL; } while(0)


/* Internal */
#   define Indent do {   		\
	struct Task *sd_task = FindTask(NULL);	\
   	int sd_spaceswritten;					\
   	for (sd_spaceswritten = 0; sd_spaceswritten < (ULONG)sd_task->tc_UserData; sd_spaceswritten ++) kprintf(" "); } while(0)

/* Internal */
#define ExitFunc do { 				\
	struct Task *sd_task = FindTask(NULL);	\
   	int sd_spaceswritten;					\
   	((ULONG)sd_task->tc_UserData) -= SDEBUG_INDENT;		\
   	for (sd_spaceswritten = 0; sd_spaceswritten < (ULONG)sd_task->tc_UserData; sd_spaceswritten ++) kprintf(" "); } while(0)

#else

#   define SDInit()
#   define Indent
#   define EnterFunc(x...) D(x)
#   define ExitFunc

#endif /* SDEBUG */



/* Sanity check macros
 *
 *	ASSERT(x)
 *		Do nothing if the expression <x> evalutates to a
 *		non-zero value, output a debug message otherwise.
 *
 *	ASSERT_VALID_PTR(x)
 *      Checks that the expression <x> points to a valid memory location, and 
 *      outputs a debug message otherwise. A NULL pointer is considered INVALID.
 * 
 *	ASSERT_VALID_PTR_OR_NULL(x)
 *      Checks that the expression <x> points to a valid memory location or that 
 *      it is NULL, and outputs a debug message otherwise. A NULL pointer is 
 *      considered VALID.
 *
 *	ASSERT_VALID_TASK(t)
 *		Checks that the pointer <t> points to a valid Task
 *		structure and outputs a debug message otherwise.
 *
 *	ASSERT_VALID_PROCESS(p)
 *		Checks that the pointer <p> points to a valid Process
 *		structure and outputs a debug message otherwise.
 *
 *	KASSERT(x)
 *		Do nothing if the expression <x> evalutates to a
 *		non-zero value, output a debug message, and cause an
 *		Alert() otherwise. This should only be used in kernel code.
 *
 */
#undef DBPRINTF
#undef THIS_FILE
#undef ASSERT
#undef ASSERT_VALID_PTR
#undef ASSERT_VALID_PTR_OR_NULL
#undef ASSERT_VALID_TASK
#undef ASSERT_VALID_PROCESS

#if ADEBUG

#define DBPRINTF kprintf

/* The trick with THIS_FILE allows us to reuse the same static string
 * instead of allocating a new copy for each invocation of these macros.
 */
#define THIS_FILE __FILE__

#define ASSERT(x) do {						\
	(x) ? 0 :						\
	( DBPRINTF("\x07%s:%ld: assertion failed: %s\n",	\
	THIS_FILE, __LINE__, #x) );				\
} while(0)

#define ASSERT_VALID_PTR(x) do {				\
	(((IPTR)(x) > 1024) &&					\
	TypeOfMem((APTR)(x))) ? 0 :				\
	( DBPRINTF("\x07%s, %ld: bad pointer: %s = $%lx\n",	\
	THIS_FILE, __LINE__, #x, (APTR)(x)) );			\
} while(0)

#define ASSERT_VALID_PTR_OR_NULL(x) do {			\
	((((APTR)(x)) == NULL) ||				\
	(((IPTR)(x) > 1024) &&	TypeOfMem((APTR)(x)))) ? 0 :	\
	( DBPRINTF("\x07%s:%ld: bad pointer: %s = $%lx\n",	\
	THIS_FILE, __LINE__, #x, (APTR)(x)) );			\
} while(0)

#define ASSERT_VALID_TASK(t) do {				\
	ASSERT_VALID_PTR(t);					\
	ASSERT((((t)->tc_Node.ln_Type == NT_TASK) ||			\
	(t)->tc_Node.ln_Type == NT_PROCESS));			\
} while(0)

#define ASSERT_VALID_PROCESS(p) do {				\
	ASSERT_VALID_PTR(p);					\
	ASSERT((p)->pr_Task.tc_Node.ln_Type == NT_PROCESS);	\
} while(0)

#define KASSERT(x) do {						\
	(x) ? 0 :						\
	( DBPRINTF("\x07%s:%ld: assertion failed: %s\n",	\
	THIS_FILE, __LINE__, #x), Alert(AG_BadParm) );		\
} while(0)

#else /* !ADEBUG */

#   define ASSERT(x)
#   define ASSERT_VALID_PTR(x)
#   define ASSERT_VALID_PTR_OR_NULL(x)
#   define ASSERT_VALID_TASK(t)
#   define ASSERT_VALID_PROCESS(p)
#   define KASSERT(x)

#endif /* ADEBUG */


/* Memory munging macros
 */
 
/* MUNGWALL_SIZE must be a multiple of MEMCHUNK_TOTAL, otherwise for example
   rom/exec/allocate complains, because the return value (memory pointer) of
   AllocMem would not be a multiple of MEMCHUNK_TOTAL anymore.
   
   See rom/exec/allocmem and rom/exec/freemem for more info. 
   
   The "32 *" probably makes sure that you get a multiple of MEMCHUNK_TOTAL on
   every machine, because on 32 bit machines MEMCHUNK_TOTAL will be 8 and
   on 64 bit machines it will be 16, and it would even work on 128 bit machines
   because I guess there MEMCHUNK_TOTAL will be 32 */
   
#define MUNGWALL_SIZE (32 * 1)

#define MUNGWALLHEADER_SIZE 32

#if AROS_SIZEOFULONG == 4
#    define MEMFILL_FREE	0xDEADBEEFL
#    define MEMFILL_ALLOC	0xC0DEDBADL
#    define MEMFILL_WALL	0xABADC0DEL
#elif AROS_SIZEOFULONG == 8
#    define MEMFILL_FREE	0xDEADBEEFDEADBEEFL
#    define MEMFILL_ALLOC	0xC0DEDBADC0DEDBADL
#    define MEMFILL_WALL	0xABADC0DEABADC0DEL
#else
#    error sizeof ULONG is neither 4 nor 8 in this architecture
#endif

#undef MUNGE_BLOCK

#if MDEBUG
/* Fill the memory block pointed by <ptr> of size <size> with <fill>
 */
#   define MUNGE_BLOCK(ptr, fill, size) do {		\
	ULONG *__p = (ULONG *)(ptr);			\
	ULONG __s = (size) / sizeof(ULONG);		\
	while (__s--) *__p++ = (fill);			\
} while(0)

/* Build a wall over the memory block <ptr> of size <size> with <fill> bricks.
 */
#   define BUILD_WALL(ptr, fill, size) do {		\
	memset((ptr), (fill), (size));			\
} while(0)

/* Check the integrity of the wall <ptr> of size <size> bytes containing <fill>.
 */
#   define CHECK_WALL(ptr, fill, size) do {		\
	UBYTE *__p = (UBYTE *)(ptr);			\
	size_t __s = (size);				\
	while (__s--)					\
	{						\
	    if(*__p != (fill))				\
	    {						\
		struct Task *__t = FindTask(NULL);	\
		kprintf("\x07" "Broken wall detected at %s:%d at 0x%x, " \
			"Task: 0x%x, Name: %s\n",       \
                        __FUNCTION__, __LINE__,         \
			__p, __t, __t->tc_Node.ln_Name);\
	    }						\
	    __p++;					\
	}						\
} while(0)

#    define MungWallCheck() AvailMem(MEMF_CLEAR)
#else

#    define MUNGE_BLOCK(ptr, size, fill)
#    define CHECK_WALL(ptr, fill, size)
#    define MungWallCheck()

#endif /* MDEBUG */


#if DEBUG
#   define D(x...)     Indent x

#   if DEBUG > 1
#	define DB2(x...)    x
#   else
#	define DB2(x...)    /* eps */
#   endif

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
#   define D(x...)     /* eps */
#   define DB2(x...)     /* eps */

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

#define bug	kprintf
#define rbug(main,sub,lvl,fmt,args...) \
		rkprintf (DBG_MAINSYSTEM_ ## main, \
			DBG_ ## main ## _SUBSYSTEM_ ## sub, \
			lvl, fmt, ##args)

/* Debugging constants. These should be defined outside and this
   part should be generated. */
#define DBG_MAINSYSTEM_INTUITION "intuition"
#define DBG_INTUITION_SUBSYSTEM_INPUTHANDLER "inputhandler"
		
#define AROS_FUNCTION_NOT_IMPLEMENTED(library) \
    kprintf("The function %s/%s() is not implemented.\n", (library), __FUNCTION__)

#define AROS_METHOD_NOT_IMPLEMENTED(CLASS, name) \
    kprintf("The method %s::%s() is not implemented.\n", (CLASS), (name))

#define aros_print_not_implemented(name) \
    kprintf("The function %s() is not implemented.\n", (name))

#define ALIVE kprintf("%s - %s line %d\n",__FILE__,__FUNCTION__,__LINE__);

#endif /* AROS_DEBUG_H */
