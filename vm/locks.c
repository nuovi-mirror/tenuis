#ifndef locks_c_
#define locks_c_
#include "vm.h"
#include "lockdefs.h"
#include <stdatomic.h>

/* memory locking structure; call rlockmem() before reading memory and
 * wlockmem() before writting it, calling frlockmem() and fwlockmem()
 * to unlock them respecivly */

/* counter readers shows the number of readers currently and one to
 * show if a writter is requesting. If a writter is requesting, a
 * reader must wait until they stop. When a writter is requesting, it
 * must wait until the reader counter hits zero. This makes it so there
 * can be many readers with only one writter, which is the most common
 * path here */

/* XXX C11 needed for this; could be replaced by a POSIX
 * backend, but this method is easier to reason about */

/* VM_MULTI(vm) says whether locking is on. By default it reads the flag at
 * runtime; vm_single.c and vm_multi.c define it as a constant before this
 * file is included, so the compiler deletes the checks entirely */
#ifndef VM_MULTI
#define VM_MULTI(vm) ((vm)->multi)
#endif

static inline void rlockmem (struct vm *vm) {
	if (VM_MULTI(vm) == 0) return;
	while (atomic_load(&writelock) == 1) ;	/* wait for no writers */
	atomic_fetch_add(&readers, 1);		/* add a reader */
}

static inline void frlockmem (struct vm *vm) {
	if (VM_MULTI(vm) == 0) return;
	atomic_fetch_sub(&readers, 1);		/* remove a reader */
}

static inline void wlockmem (struct vm *vm) {
	if (VM_MULTI(vm) == 0) return;
	while (atomic_load(&writelock) == 1) ;  /* wait for write lock */
	atomic_store(&writelock, 1);		/* lock the write lock */
	while (atomic_load(&readers) > 0) ;	/* wait for readers */
}

static inline void fwlockmem (struct vm *vm) {
	if (VM_MULTI(vm) == 0) return;
	atomic_store(&writelock, 0);		/* unlock write lock */
}
#endif
