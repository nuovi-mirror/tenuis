/* interpreter instance with memory locking always on */
#define VM_MULTI(vm) ((void)(vm), 1)
#define INTERP interpret_multi
#include "interp.c"
