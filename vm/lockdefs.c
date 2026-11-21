#include "vm.h"
#include <stdatomic.h>

volatile atomic_uint readers = 0;
volatile atomic_uint writelock = 0;
