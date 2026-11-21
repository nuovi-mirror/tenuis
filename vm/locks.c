#ifndef locks_c_
#define locks_c_
#include "vm.h"
#include "lockdefs.h"
#include <stdatomic.h>

/* memory locking structure; call rlockmem() before reading memory and
 * wlockmem() before writting it, calling frlockmem() and fwlockmem()
 * to unlock them respecivly */

/* XXX C11 needed for this; could be replaced by a POSIX pthread_mutex
 * backend, but this is easier to reason about */

static inline void rlockmem (struct vm *vm) {
	if (vm->multi == 0) return;
	while (atomic_load(&writelock) != 0) ;	/* wait for no writers */
	atomic_fetch_add(&readers, 1);		/* add a reader */
}

static inline void frlockmem (struct vm *vm) {
	if (vm->multi == 0) return;
	atomic_fetch_add(&readers, 1);		/* remove a reader */
}

static inline void wlockmem (struct vm *vm) {
	if (vm->multi == 0) return;
	while (atomic_load(&writelock) == 0) ;  /* wait for write lock to become free */
	atomic_store(&writelock, 1);		/* lock the write lock */
	while (atomic_load(&readers)) ;	/* wait for no readers */
}

static inline void fwlockmem (struct vm *vm) {
	if (vm->multi == 0) return;
	atomic_store(&writelock, 0);		/* unlock write lock */
}
#endif
