
#ifndef _MACROS_H
#define _MACROS_H

/****************************************************************************/

#define SAVEDS
#define ASM
#define REGARGS
#define STDARGS
#define INLINE          inline
#define VARARGS68k  __stackparm
#define REG(x,p)        p
//#define LIBCALL

#define M_DISP(n) static BOOPSI_DISPATCHER(ULONG,n,cl,obj,msg)

//static ULONG SAVEDS ASM n(REG(a0,struct IClass *cl),REG(a2,Object *obj),REG(a1,Msg msg))

#define M_DISPSTART
#define M_DISPEND(n) BOOPSI_DISPATCHER_END
#define DISP(n) (n)


#undef NODE
#define NODE(a) ((struct Node *)(a))

#undef MINNODE
#define MINNODE(a) ((struct MinNode *)(a))

#undef LIST
#define LIST(a) ((struct List *)(a))

#undef MINLIST
#define MINLIST(a) ((struct MinList *)(a))

#undef MESSAGE
#define MESSAGE(m) ((struct Message *)(m))

#undef NEWLIST
#define NEWLIST(l) (LIST(l)->lh_Head = NODE(&LIST(l)->lh_Tail), \
                    LIST(l)->lh_Tail = NULL, \
                    LIST(l)->lh_TailPred = NODE(&LIST(l)->lh_Head))

#undef QUICKNEWLIST
#define QUICKNEWLIST(l) (LIST(l)->lh_Head = NODE(&LIST(l)->lh_Tail), \
                         LIST(l)->lh_TailPred = NODE(&LIST(l)->lh_Head))

#undef ADDTAIL
#define ADDTAIL(l,n) AddTail(LIST(l),NODE(n))

#undef PORT
#define PORT(p) ((struct MsgPort *)(p))

#undef INITPORT
#define INITPORT(p,s) (PORT(p)->mp_Flags = PA_SIGNAL, \
                       PORT(p)->mp_SigBit = (UBYTE)(s), \
                       PORT(p)->mp_SigTask = FindTask(NULL), \
                       NEWLIST(&(PORT(p)->mp_MsgList)))

#undef QUICKINITPORT
#define QUICKINITPORT(p,s,t) (PORT(p)->mp_Flags = PA_SIGNAL, \
                              PORT(p)->mp_SigBit = (UBYTE)(s), \
                              PORT(p)->mp_SigTask = (t), \
                              QUICKNEWLIST(&(PORT(p)->mp_MsgList)))

#undef INITMESSAGE
#define INITMESSAGE(m,p,l) (MESSAGE(m)->mn_Node.ln_Type = NT_MESSAGE, \
                            MESSAGE(m)->mn_ReplyPort = PORT(p), \
                            MESSAGE(m)->mn_Length = ((UWORD)l))

#undef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))

#undef MIN
#define MIN(a,b) ((a<b) ? (a) : (b))

#undef MAX
#define MAX(a,b) ((a>b) ? (a) : (b))

#undef ABS
#define ABS(a) (((a)>0) ? (a) : -(a))

#undef BOOLSAME
#define BOOLSAME(a,b) (((a) ? TRUE : FALSE)==((b) ? TRUE : FALSE))

/****************************************************************************/

#endif /* _MACROS_H */
