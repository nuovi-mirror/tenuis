#ifndef h_lockdefs_
#define h_lockdefs_
#include <stdatomic.h>
extern volatile atomic_uint readers;
extern volatile atomic_uint writelock;
#endif
