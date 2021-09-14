#include "timer.h"
#include "useomp.h"
#include "my_malloc.h"

#ifdef IRREGULAR
#define parallel_for parallel_for_irregular_tasking
#endif

#ifdef REGULAR
#define parallel_for parallel_for_tasking
#endif

