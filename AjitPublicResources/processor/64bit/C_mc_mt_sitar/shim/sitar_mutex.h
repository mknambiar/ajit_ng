#ifndef SITAR_MUTEX_H
#define SITAR_MUTEX_H

#include <errno.h>
#include <pthread.h>

// Weak hook: can be overridden by a Sitar-aware yield implementation.
void __attribute__((weak)) sitar_mutex_wait_yield(void);

// Exactly one trylock attempt per call.
// Returns 1 if lock acquired, 0 otherwise.
static inline int sitar_mutex_lock(pthread_mutex_t* m)
{
	int rc = pthread_mutex_trylock(m);
	if (rc == 0) {
		return 1;
	}
	if (rc == EBUSY) {
		if (sitar_mutex_wait_yield) {
			sitar_mutex_wait_yield();
		}
		return 0;
	}
	return 0;
}

static inline void sitar_mutex_unlock(pthread_mutex_t* m)
{
	(void) pthread_mutex_unlock(m);
}

#endif
