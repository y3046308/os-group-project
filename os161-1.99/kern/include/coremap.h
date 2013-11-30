
#ifndef _COREMAP_H_
#define _COREMAP_H_
#include "opt-A3.h"
#include <addrspace.h>
#if OPT_A3

typedef enum _frame_state{
//	FREE, DIRTY, FIXED, CLEAN,
	FREE, USED,
} frame_state;

struct coremap {
	paddr_t pa; // physical address
	size_t size;	// size
	frame_state state;
	int page_num;
};

int coremap_size;
struct coremap *coremaps;

void init_coremap(paddr_t freeaddr);
paddr_t getppages(unsigned long npages);
void freeppages(paddr_t paddr);
#endif
#endif
