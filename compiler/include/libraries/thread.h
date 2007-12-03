#ifndef LIBRARIES_THREAD_H
#define LIBRARIES_THREAD_H 1

#include <stdint.h>

typedef void *      (*ThreadFunction)(void *);
typedef uint32_t    ThreadIdentifier;
typedef void *      Mutex;
typedef void *      Condition;

#endif
