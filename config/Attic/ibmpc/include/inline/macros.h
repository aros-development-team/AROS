/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$
    
    Desc: Library calls.
    Lang: English
*/

#ifndef INLINE_MACROS_H
#define INLINE_MACROS_H

/* LibBase is stored as the last argument. It's up to you whether
you want to use it or no. */

#define LP0(offs, rt, name, bt, bn) 					\
({									\
    register rt _##name##_re __asm__("eax");				\
    register struct Library *_##name##_bn __asm__("eax");		\
    _##name##_bn=(struct Library*)(bn);					\
    __asm__ ("pushl %%eax"::"r"(_##name##_bn));				\
    __asm__ __volatile__("call *-"#offs"(%%eax)\n\taddl $4,%%esp"	\
    : "=r" (_##name##_re)						\
    : "r" (_##name##_bn)						\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");			\
    _##name##_re;							\
})

#define LP0NR(offs, name, bt, bn) 					\
({									\
    register struct Library *_##name##_bn __asm__("eax");		\
    _##name##_bn=(struct Library*)(bn);					\
    __asm__("pushl %%eax"::"r"(_##name##_bn));				\
    __asm__ __volatile__("call *-"#offs"(%%eax)\n\taddl $4,%%esp"	\
    : /* no output */ 							\
    : "r" (_##name##_bn)						\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");			\
})

#define LP1(offs, rt, name, t1, v1, bt, bn) \
({\
    register rt _##name##_re __asm__("eax");\
    register t1 _n1 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n1 = v1;\
    __asm__("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $8,%%esp"\
    : "=r" (_##name##_re)\
    : "r" (_##name##_bn), "r" (_n1)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
    _##name##_re;\
})

#define LP1NR(offs, name, t1, v1, bt, bn) \
({\
    register t1 _n1 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n1 = v1;\
    __asm__("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $8,%%esp"\
    : /* no output */ \
    : "r" (_##name##_bn), "r" (_n1)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
})

#define LP2(offs, rt, name, t1, v1, t2, v2, bt, bn) \
({\
    register rt _##name##_re __asm__("eax");\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $12,%%esp"\
    : "=r" (_##name##_re)\
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
    _##name##_re;\
})

#define LP2NR(offs, name, t1, v1, t2, v2, bt, bn) \
({\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $12,%%esp"\
    : /* no output */ \
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
})

#define LP3(offs, rt, name, t1, v1, t2, v2, t3, v3, bt, bn) \
({\
    register rt _##name##_re __asm__("eax");\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register t3 _n3 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n3 = v3;\
    __asm__("pushl %%eax"::"r"(_n3));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $16,%%esp"\
    : "=r" (_##name##_re)\
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2), "r" (_n3)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
    _##name##_re;\
})

#define LP3NR(offs, name, t1, v1, t2, v2, t3, v3, bt, bn) \
({\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register t3 _n3 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n3 = v3;\
    __asm__("pushl %%eax"::"r"(_n3));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $16,%%esp"\
    : /* no output */ \
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2), "r" (_n3)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
})

#define LP4(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, bt, bn) \
({\
    register rt _##name##_re __asm__("eax");\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register t3 _n3 __asm__("eax");\
    register t4 _n4 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n4 = v4;\
    __asm__("pushl %%eax"::"r"(_n4));\
    _n3 = v3;\
    __asm__("pushl %%eax"::"r"(_n3));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $20,%%esp"\
    : "=r" (_##name##_re)\
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2), "r" (_n3), "r" (_n4)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
    _##name##_re;\
})

#define LP4NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, bt, bn) \
({\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register t3 _n3 __asm__("eax");\
    register t4 _n4 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n4 = v4;\
    __asm__("pushl %%eax"::"r"(_n4));\
    _n3 = v3;\
    __asm__("pushl %%eax"::"r"(_n3));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $20,%%esp"\
    : /* no output */ \
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2), "r" (_n3), "r" (_n4)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
})

#define LP5(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, bt, bn) \
({\
    register rt _##name##_re __asm__("eax");\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register t3 _n3 __asm__("eax");\
    register t4 _n4 __asm__("eax");\
    register t5 _n5 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n5 = v5;\
    __asm__("pushl %%eax"::"r"(_n5));\
    _n4 = v4;\
    __asm__("pushl %%eax"::"r"(_n4));\
    _n3 = v3;\
    __asm__("pushl %%eax"::"r"(_n3));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $24,%%esp"\
    : "=r" (_##name##_re)\
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2), "r" (_n3), "r" (_n4), "r" (_n5)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
    _##name##_re;\
})

#define LP5FP(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, bt, bn, fpt) \
({\
    typedef fpt;\
    register rt _##name##_re __asm__("eax");\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register t3 _n3 __asm__("eax");\
    register t4 _n4 __asm__("eax");\
    register t5 _n5 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n5 = v5;\
    __asm__("pushl %%eax"::"r"(_n5));\
    _n4 = v4;\
    __asm__("pushl %%eax"::"r"(_n4));\
    _n3 = v3;\
    __asm__("pushl %%eax"::"r"(_n3));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $24,%%esp"\
    : "=r" (_##name##_re)\
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2), "r" (_n3), "r" (_n4), "r" (_n5)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
    _##name##_re;\
})

#define LP5NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, bt, bn) \
({\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register t3 _n3 __asm__("eax");\
    register t4 _n4 __asm__("eax");\
    register t5 _n5 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n5 = v5;\
    __asm__("pushl %%eax"::"r"(_n5));\
    _n4 = v4;\
    __asm__("pushl %%eax"::"r"(_n4));\
    _n3 = v3;\
    __asm__("pushl %%eax"::"r"(_n3));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $24,%%esp"\
    : /* no output */ \
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2), "r" (_n3), "r" (_n4), "r" (_n5)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
})

#define LP6(offs, rt, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, bt, bn) \
({\
    register rt _##name##_re __asm__("eax");\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register t3 _n3 __asm__("eax");\
    register t4 _n4 __asm__("eax");\
    register t5 _n5 __asm__("eax");\
    register t6 _n6 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n6 = v6;\
    __asm__("pushl %%eax"::"r"(_n6));\
    _n5 = v5;\
    __asm__("pushl %%eax"::"r"(_n5));\
    _n4 = v4;\
    __asm__("pushl %%eax"::"r"(_n4));\
    _n3 = v3;\
    __asm__("pushl %%eax"::"r"(_n3));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $28,%%esp"\
    : "=r" (_##name##_re)\
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2), "r" (_n3), "r" (_n4), "r" (_n5), "r" (_n6)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
    _##name##_re;\
})

#define LP6NR(offs, name, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, bt, bn) \
({\
    register t1 _n1 __asm__("eax");\
    register t2 _n2 __asm__("eax");\
    register t3 _n3 __asm__("eax");\
    register t4 _n4 __asm__("eax");\
    register t5 _n5 __asm__("eax");\
    register t6 _n6 __asm__("eax");\
    register struct Library *_##name##_bn __asm__("eax");\
    _##name##_bn=(struct Library*)(bn);\
    __asm__("pushl %%eax"::"r"(_##name##_bn));\
    _n6 = v6;\
    __asm__("pushl %%eax"::"r"(_n6));\
    _n5 = v5;\
    __asm__("pushl %%eax"::"r"(_n5));\
    _n4 = v4;\
    __asm__("pushl %%eax"::"r"(_n4));\
    _n3 = v3;\
    __asm__("pushl %%eax"::"r"(_n3));\
    _n2 = v2;\
    __asm__("pushl %%eax"::"r"(_n2));\
    _n1 = v1;\
    __asm__ ("pushl %%eax"::"r"(_n1));\
    _##name##_bn=(struct Library*)(bn);\
    __asm__ __volatile__ ("call *-"#offs"(%%eax)\n\taddl $28,%%esp"\
    : /* no output */ \
    : "r" (_##name##_bn), "r" (_n1), "r" (_n2), "r" (_n3), "r" (_n4), "r" (_n5), "r" (_n6)\
    : "eax", "ebx", "ecx", "edx", "cc", "memory");\
})

#endif /* INLINE_MACROS_H */