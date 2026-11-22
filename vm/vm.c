#include "vm.h"

/* pick the interpreter variant once; threads spawned later copy the
 * flag and come back through here */
void interpret (struct vm *vm) {
	if (vm->multi == 0) interpret_single(vm);
	else interpret_multi(vm);
}
