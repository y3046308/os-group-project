
#ifndef _PT_H_
#define _PT_H_
#include "opt-A3.h"

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
    int valid;
    int dirty;
};

struct pte pte_create(paddr_t pfn, int valid, int dirty);

#endif

#endif /* PT.H */
