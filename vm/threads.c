#include "vm.h"
#include "vmthreads.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void *vmtnew_t (void *arg) {
	struct vmtarg *argst = (struct vmtarg *)arg;
	struct vm *vm = (struct vm *)argst->vm;
	struct vm *tvm = (struct vm *)malloc(sizeof(struct vm));

	memcpy(tvm, vm, sizeof(struct vm));

	tvm->pc = argst->pc;
	tvm->registers[63] = argst->sp;

	interpret(tvm);
	return NULL;
}

void vmtnew (struct vm *vm, word sp, word pc) {
	struct vmtarg *args = malloc(sizeof(struct vmtarg));
	pthread_t thread;

	args->pc = pc;
	args->sp = sp;
	args->vm = vm;

	if (pthread_create(&thread, NULL, &vmtnew_t, args) != 0) {
		vm->registers[0] = 1;
		free(args);
		return;
	}
	
	if (pthread_detach(thread) != 0) {
		vm->registers[0] = 2;

		if (pthread_join(thread, NULL) != 0) {
			vm->registers[0] = 3;
		}
	}
}
