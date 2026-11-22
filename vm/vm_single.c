/* interpreter instance with memory locking compiled out */
#define VM_MULTI(vm) ((void)(vm), 0)
#define INTERP interpret_single
#include "interp.c"
