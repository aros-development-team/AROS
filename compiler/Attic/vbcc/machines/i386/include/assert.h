#ifndef __ASSERT_H
#define __ASSERT_H 1
#endif

#undef assert

#ifndef NDEBUG
#define assert(exp) ((void)((exp)?0:fprintf(stderr,"Assertion failed: %d, file %s, line %d\n",(exp),__FILE__,__LINE__)))
#else
#define assert(exp)
#endif

