/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$
    
    Desc: Library calls.
    Lang: English
*/

#ifndef INLINE_MACROS_H
#define INLINE_MACROS_H

/*
    LibBase is stored as the last argument. It's up to you whether
    you want to use it or no.
    This macros may be compiled with -O1 parameter. Do not use -Wall
    switch or you'll see some warnings (compiler thinks, that %esp
    isn't initialized.
*/


#define LP0(offs, rt, name, bt, bn) 					\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)(bn); \
    _##name##_sp-=sizeof(struct Library*); 				\
    *(struct Library**)_##name##_sp=_##name##_bn; 			\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)"			\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*);				\
    _##name##_re;							\
})

#define LP0NR(offs, name, bt, bn) 					\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)(bn); \
    _##name##_sp-=sizeof(struct Library*); 				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)"			\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*);				\
})

#define LP1(offs, rt, name, t1, v1, bt, bn) 				\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1);			\
    _##name##_re;							\
})

#define LP1NR(offs, name, t1, v1, bt, bn) 				\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1);			\
})

#define LP2(offs, rt, name, t1, v1, t2, v2, bt, bn)			\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2);	\
    _##name##_re;							\
})

#define LP2NR(offs, name, t1, v1, t2, v2, bt, bn)			\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2);	\
})

#define LP3(offs, rt, name, t1, v1, t2, v2, t3, v3, bt, bn)		\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3);							\
    _##name##_re;							\
})

#define LP3NR(offs, name, t1, v1, t2, v2, t3, v3, bt, bn)		\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("edx") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%edx)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3);							\
})

#define LP4(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4);						\
    _##name##_re;							\
})

#define LP4NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4);						\
})

#define LP5(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5);				\
    _##name##_re;							\
})

#define LP5FP(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, bt, bn, fpt)	\
({									\
    typedef fpt;							\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5);				\
    _##name##_re;							\
})

#define LP5NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5);				\
})

#define LP6(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6);			\
    _##name##_re;							\
})

#define LP6NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6);			\
})

#define LP7(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t7);						\
    *(t7*)_##name##_sp=v7;						\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6)+sizeof(t7);	\
    _##name##_re;							\
})

#define LP7NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t7);						\
    *(t7*)_##name##_sp=v7;						\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6)+sizeof(t7);	\
})

#define LP8(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t8);						\
    *(t8*)_##name##_sp=v8;						\
    _##name##_sp-=sizeof(t7);						\
    *(t7*)_##name##_sp=v7;						\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6)+sizeof(t7)		\
	+sizeof(t8);							\
    _##name##_re;							\
})

#define LP8NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t8);						\
    *(t8*)_##name##_sp=v8;						\
    _##name##_sp-=sizeof(t7);						\
    *(t7*)_##name##_sp=v7;						\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6)+sizeof(t7)		\
	+sizeof(t8);							\
})

#define LP9(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, t9, v9, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t9);						\
    *(t9*)_##name##_sp=v9;						\
    _##name##_sp-=sizeof(t8);						\
    *(t8*)_##name##_sp=v8;						\
    _##name##_sp-=sizeof(t7);						\
    *(t7*)_##name##_sp=v7;						\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6)+sizeof(t7)		\
	+sizeof(t8)+sizeof(t9);						\
    _##name##_re;							\
})

#define LP9NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, t9, v9, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t9);						\
    *(t9*)_##name##_sp=v9;						\
    _##name##_sp-=sizeof(t8);						\
    *(t8*)_##name##_sp=v8;						\
    _##name##_sp-=sizeof(t7);						\
    *(t7*)_##name##_sp=v7;						\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6)+sizeof(t7)		\
	+sizeof(t8)+sizeof(t9);						\
})

#define LP10NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, t9, v9, t10, v10, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t10);						\
    *(t10*)_##name##_sp=v10;						\
    _##name##_sp-=sizeof(t9);						\
    *(t9*)_##name##_sp=v9;						\
    _##name##_sp-=sizeof(t8);						\
    *(t8*)_##name##_sp=v8;						\
    _##name##_sp-=sizeof(t7);						\
    *(t7*)_##name##_sp=v7;						\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :/* no output */							\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6)+sizeof(t7)		\
	+sizeof(t8)+sizeof(t9)+sizeof(t10);				\
})

#define LP11(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, t8, v8, t9, v9, t10, v10, t11, v11, bt, bn)	\
({									\
    register void * _##name##_sp asm("esp"); 				\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *const _##name##_bn asm ("eax") = (struct Library*)bn; \
    _##name##_sp-=sizeof(struct Library*);				\
    *(struct Library**)_##name##_sp=_##name##_bn;			\
    _##name##_sp-=sizeof(t11);						\
    *(t11*)_##name##_sp=v11;						\
    _##name##_sp-=sizeof(t10);						\
    *(t10*)_##name##_sp=v10;						\
    _##name##_sp-=sizeof(t9);						\
    *(t9*)_##name##_sp=v9;						\
    _##name##_sp-=sizeof(t8);						\
    *(t8*)_##name##_sp=v8;						\
    _##name##_sp-=sizeof(t7);						\
    *(t7*)_##name##_sp=v7;						\
    _##name##_sp-=sizeof(t6);						\
    *(t6*)_##name##_sp=v6;						\
    _##name##_sp-=sizeof(t5);						\
    *(t5*)_##name##_sp=v5;						\
    _##name##_sp-=sizeof(t4);						\
    *(t4*)_##name##_sp=v4;						\
    _##name##_sp-=sizeof(t3);						\
    *(t3*)_##name##_sp=v3;						\
    _##name##_sp-=sizeof(t2);						\
    *(t2*)_##name##_sp=v2;						\
    _##name##_sp-=sizeof(t1);						\
    *(t1*)_##name##_sp=v1;						\
    asm volatile ("call *-"#offs"(%%eax)"				\
    :"=r" (_##name##_re)						\
    :"r"(_##name##_bn)							\
    :"eax", "ecx", "edx", "cc", "memory");				\
    _##name##_sp+=sizeof(struct Library*)+sizeof(t1)+sizeof(t2)		\
	+sizeof(t3)+sizeof(t4)+sizeof(t5)+sizeof(t6)+sizeof(t7)		\
	+sizeof(t8)+sizeof(t9)+sizeof(t10)+sizeof(t11);			\
    _##name##_re;							\
})


#endif /* INLINE_MACROS_H */
