#include "vm.h"
#include "vmthreads.h"
#include <stdlib.h>
#include <pthread.h>

void *vmtnew_t (void *arg) {
	struct vmtarg *argst = (struct vmtarg *)arg;
	struct vm *vm = (struct vm *)argst->vm;
	struct vm *tvm = (struct vm *)malloc(sizeof(struct vm));

	tvm->pc = argst->pc;
	tvm->heap = vm->heap;
	tvm->registers[63] = argst->sp;
	tvm->kill = 0;
	tvm->halted = 0;
	tvm->multi = vm->multi;

	interpret(vm);
	return NULL;
}

void vmtnew (struct vm *vm, word sp, word pc) {
	struct vmtarg *args = malloc(sizeof(struct vmtarg));
	pthread_t thread;

	args->pc = pc;
	args->sp = sp;
	args->vm = vm;

	if (pthread_create(&thread, NULL, &vmtnew_t, &args) != 0) {
		vm->registers[0] = 1;
		free(args);
	}
		
	if (pthread_detach(thread) != 0) {
		vm->registers[0] = 2;

		if (pthread_join(thread, NULL) != 0) {
			vm->registers[0] = 3;
		}
	}
}
