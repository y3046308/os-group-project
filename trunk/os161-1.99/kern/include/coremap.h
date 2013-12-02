
#ifndef _COREMAP_H_
#define _COREMAP_H_
#include "opt-A3.h"
#include <addrspace.h>
#if OPT_A3

typedef enum _frame_state{
//	FREE, DIRTY, FIXED, CLEAN,
	FREE, USED,
} frame_state;
typedef enum _frame_owner{
	NO, KERNEL, USER,
} frame_owner;

struct coremap {
	vaddr_t va; // victim
	paddr_t pa; // physical address
	frame_state state;
	frame_owner owner;
	int page_num;
};

int coremap_size;
paddr_t paddr_max, freeaddr;
struct coremap *coremaps;
bool replaced;

void init_coremap(paddr_t freeaddr);
void reset_coremap(void);
paddr_t getppages(unsigned long npages, frame_owner owner);
void freeppages(paddr_t paddr);
#endif
#endif
