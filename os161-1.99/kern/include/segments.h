#if OPT_A3
#include "opt-A3.h"

struct segment{
	vaddr_t start;
	size_t size;	
	int flag;	// read-only for text-segment
};
#endif
