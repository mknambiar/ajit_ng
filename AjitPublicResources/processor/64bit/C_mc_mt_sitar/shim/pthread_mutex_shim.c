#include <errno.h>
#include <pthread.h>
#include <stdint.h>

// Sitar mutex wait hook. Real scheduler-integrated yield can override this.
void __attribute__((weak)) sitar_mutex_wait_yield(void)
{
}

typedef struct {
	uintptr_t key;
	volatile int taken;
} mutex_slot_t;

#define MUTEX_SLOTS 4096

static mutex_slot_t g_slots[MUTEX_SLOTS];

static inline uint32_t slot_hash(uintptr_t key)
{
	return (uint32_t) ((key >> 3) % MUTEX_SLOTS);
}

static mutex_slot_t* lookup_or_create_slot(uintptr_t key)
{
	uint32_t h = slot_hash(key);
	uint32_t i;
	for (i = 0; i < MUTEX_SLOTS; i++) {
		mutex_slot_t* s = &g_slots[(h + i) % MUTEX_SLOTS];
		uintptr_t cur = s->key;
		if (cur == key) {
			return s;
		}
		if (cur == 0 && __sync_bool_compare_and_swap(&s->key, 0, key)) {
			s->taken = 0;
			return s;
		}
		if (s->key == key) {
			return s;
		}
	}
	return 0;
}

int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t* attr)
{
	(void) attr;
	if (!m) return EINVAL;

	mutex_slot_t* s = lookup_or_create_slot((uintptr_t) m);
	return (s ? 0 : ENOMEM);
}

int pthread_mutex_trylock(pthread_mutex_t* m)
{
	if (!m) return EINVAL;

	mutex_slot_t* s = lookup_or_create_slot((uintptr_t) m);
	if (!s) return ENOMEM;

	return (__sync_bool_compare_and_swap(&s->taken, 0, 1) ? 0 : EBUSY);
}

int pthread_mutex_lock(pthread_mutex_t* m)
{
	if (!m) return EINVAL;

	// Preserve pthread semantics for legacy C paths:
	// keep trying until lock is acquired, but invoke the Sitar wait hook
	// between attempts so coroutine-aware builds can yield at this point.
	while (1) {
		int rc = pthread_mutex_trylock(m);
		if (rc == 0) return 0;
		if (rc != EBUSY) return rc;
		if (sitar_mutex_wait_yield) {
			sitar_mutex_wait_yield();
		}
	}
}

int pthread_mutex_unlock(pthread_mutex_t* m)
{
	if (!m) return EINVAL;

	mutex_slot_t* s = lookup_or_create_slot((uintptr_t) m);
	if (!s) return ENOMEM;

	s->taken = 0;
	return 0;
}
