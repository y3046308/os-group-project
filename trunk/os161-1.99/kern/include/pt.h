#ifndef _PT_H_
#define _PT_H_
#include "opt-A3.h"
#include <types.h>
#include <addrspace.h>

#if OPT_A3
enum _pstate_t {
    free,
    dirty,
    fixed,
    clean
};
typedef enum _pstate_t pstate_t;


struct page{
    struct addrspace* as;
    vaddr_t vaddr;
    pstate_t state;
};

struct pte{
    paddr_t pfn;
    int valid;	// used to track which page is in memory
    int dirty;  // 1 if it has been changed since it's loaded
    int ref;	// reference bit that is used for FIFO;	0 -> first in 1-> not first
    int sw;	// index to swapfile
};

struct pte pte_create(paddr_t pfn, int valid, int dirty);
void pte_add(struct addrspace *a, vaddr_t va, int segment, int tableSize);
#endif

#endif /* PT.H */
