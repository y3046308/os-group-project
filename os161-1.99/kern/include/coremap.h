#include "opt-A3.h"
#include <addrspace.h>
#if OPT_A3

enum page_state{
	FREE, DIRTY, FIXED, CLEAN,
};

struct page{
	vaddr_t va;
	struct addrspace *as;
	enum page_state state;	// dirty, valid, use bit etc
};

int coremap_size;
struct page **page_table;	// table of pages

#endif
