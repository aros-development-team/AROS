#ifndef AROS_RT_H
#define AROS_RT_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    Resource Tracking

*/
#ifndef ENABLE_RT
#   define ENABLE_RT	0
#endif

#define RTT_MEMORY	0

void RT_Init (void);
void RT_IntAdd (int rtt, char * file, int line, ...); /* Add a resource for tracking */
void RT_IntCheck (int rtt, char * file, int line, ...); /* Check a resource before use */
void RT_IntFree (int rtt, char * file, int line, ...); /* Stop tracking of a resource */
void RT_IntEnter (char * functionname, char * filename, int line);
void RT_Leave (void);

#if ENABLE_RT
#   define RT_Add(rtt, args...)    RT_IntAdd (rtt, __FILE__, __LINE__, ##args)
#   define RT_Check(rtt, args...)  RT_IntCheck (rtt, __FILE__, __LINE__, ##args)
#   define RT_Free(rtt, args...)   RT_IntFree (rtt, __FILE__, __LINE__, ##args)
#   define RT_Enter(fn)            RT_IntEnter (fn,__FILE__, __LINE__)
#else
#   define RT_Add(rtt, args...)    /* eps */
#   define RT_Check(rtt, args...)  /* eps */
#   define RT_Free(rtt, args...)   /* eps */
#   define RT_Enter()              /* eps */
#endif

#endif /* AROS_RT_H */
