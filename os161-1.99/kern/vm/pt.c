#include "pt.h"

struct pte 
pte_create(paddr_t pfn, int valid, int dirty){
	struct pte pte = kmalloc(sizeof(struct pte));
	if (pte == NULL){
		return NULL;
	}
	
	pte->pfn = pfn;
	pte->valid = valid;
	pte->dirty = dirty;

	return pte;
}

