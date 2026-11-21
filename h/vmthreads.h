#ifndef h_vmthreads_
#define h_vmthreads_

#include "vm.h"

struct vmtarg {
	struct vm *vm;
	word pc;
	word sp;
};

void vmtnew (struct vm *vm, word sp, word pc);

#endif
