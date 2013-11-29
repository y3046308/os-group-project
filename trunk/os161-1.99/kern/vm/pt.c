#include "pt.h"
#include "opt-A3.h"
#include <lib.h>

#if OPT_A3

struct pte 
pte_create(paddr_t pfn, int valid, int dirty){
	struct pte *p = kmalloc(sizeof(struct pte));
/*	if (p == NULL){
		return NULL;
	} */
	
	p->pfn = pfn;
	p->valid = valid;
	p->dirty = dirty;

	return *p;
}

#endif
