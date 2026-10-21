/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
   Licensed under the AROS Public License (APL), Version 1.1. */
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include "unavailable.c"
static atomic_int start;
static void *worker(void *arg)
{
    (void)arg;
    while (!atomic_load_explicit(&start, memory_order_acquire)) { }
    for (int i = 0; i < 512; ++i) {
        char name[] = "RAM:unchanged-XXXXXX";
        errno = EINVAL;
        assert(aros_rs_poll(NULL, 0, 0) == -1 && errno == ENOSYS);
        errno = EINVAL;
        assert(aros_rs_fork() == (intptr_t)-1 && errno == ENOSYS);
        errno = EINVAL;
        assert(aros_rs_if_nametoindex("unavailable") == 0 && errno == ENOSYS);
        errno = EINVAL;
        assert(aros_rs_mkdtemp(name) == NULL && errno == ENOSYS);
        assert(strcmp(name, "RAM:unchanged-XXXXXX") == 0);
        errno = EINVAL;
        assert(aros_rs_gethostname(name, sizeof(name)) == -1 && errno == ENOSYS);
        assert(strcmp(name, "RAM:unchanged-XXXXXX") == 0);
        errno = EINVAL;
        assert(aros_rs_ttyname_r(-1, name, sizeof(name)) == ENOSYS && errno == EINVAL);
        assert(strcmp(name, "RAM:unchanged-XXXXXX") == 0);
        assert(aros_rs_ttyname_r(-1, NULL, 0) == ENOSYS && errno == EINVAL);
        errno = EINVAL;
        assert(aros_rs_mmap(NULL, 1, 0, 0, -1, 0) == (void *)(intptr_t)-1 && errno == ENOSYS);
    }
    return NULL;
}
int main(void)
{
    _Static_assert(sizeof(intptr_t) == 8, "fixture models native LP64 return widths");
    pthread_t threads[4];
    for (int i = 0; i < 4; ++i) assert(pthread_create(&threads[i], NULL, worker, NULL) == 0);
    errno = ERANGE;
    atomic_store_explicit(&start, 1, memory_order_release);
    for (int i = 0; i < 4; ++i) assert(pthread_join(threads[i], NULL) == 0);
    assert(errno == ERANGE);
    assert(aros_rs_hstrerror(0) != NULL && errno == ERANGE);
    return 0;
}
