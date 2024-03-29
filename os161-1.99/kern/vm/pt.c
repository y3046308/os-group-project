#include "pt.h"
#include <addrspace.h>
#include <coremap.h>
#include "opt-A3.h"
#include <lib.h>

#if OPT_A3

void
pte_create(struct pte *p, paddr_t pfn, int valid, int dirty){
        p->pfn = pfn;
        p->valid = valid;
        p->dirty = dirty;
        p->ref = 0;    
	p->sw = -1;
}

int find_pte(struct addrspace *a, paddr_t pa, int segment){
        struct pte *ptTemp = NULL;
        if (segment == 1){      // if adding it to pt1
                ptTemp = a->pt1;
        }
        else if (segment == 2){ // else if adding it to pt2
                ptTemp = a->pt2;
        }
        else{                   // else adding it to pt3
                ptTemp = a->pt3;
        }
	int i = 0;
	while (ptTemp != NULL){
		if (ptTemp[i].pfn == pa){
			break;
		}
		i++;
	}
	return i;
}
/*
// add page table entry into page table. If table is full, apply page replacement policy(FIFO)
void pte_add(struct addrspace *a, vaddr_t va, int segment, int tableSize){
        struct pte *ptTemp;
        if (segment == 1){      // if adding it to pt1
                ptTemp = a->pt1;
        }
        else if (segment == 2){ // else if adding it to pt2
                ptTemp = a->pt2;
        }
        else{                   // else adding it to pt3
                ptTemp = a->pt3;
        }
        int i, ref = -1;
        for (i = 0 ; i < tableSize ; i++){
                if (ptTemp[i].pfn == 0){        // if we found empty slot
                        ptTemp[i].pfn = getppages(1, USER);   // load available physical addr from coremap(<---- need to be adjusted)
                        ptTemp[i].valid = 1;    // ready to be traslated
                        ptTemp[i].dirty = 0;    // 0 since not modified ; just loaded into pt
                        ptTemp[i].ref = 1;      
                }
                if (ref == -1 && ptTemp[i].ref == 0){
                        ref = i;        // save the location of reference bit
                }
        }
        if (i == tableSize && ref != -1){       // if page is full, use replacement algorithm
                ptTemp[ref].pfn = getppages(1, USER); // again, this needs to be fixed
                ptTemp[ref].valid = 1;
                ptTemp[ref].dirty = 0;
                ptTemp[ref].ref = 0;
                
                ptTemp[(ref + 1) % tableSize].ref = 1;  // find next (first in) and change its reference bit
        }
        (void)va;
}
*/
#endif

