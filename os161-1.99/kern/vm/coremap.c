#include <types.h>
#include <lib.h>
#include "opt-vm.h"
#include "opt-A3.h"
#include <coremap.h>
#include <addrspace.h>
#include <vm.h>

#if OPT_A3

void init_coremap(){
        // initialize coremap
        paddr_t a1, a2;
        ram_getsize(&a1, &a2);   // get number of physical pages
        coremap_size = (a2 - a1) / PAGE_SIZE;
        coremap = kmalloc(sizeof(struct entry*) * coremap_size);
//        core_table = (struct page*)PADDR_TO_KVADDR(a1);
        
        for (int i = 0 ; i < coremap_size ; i++){       // assign right value to each entry in page table
	        coremap[i] = kmalloc(sizeof(struct entry));
//               coremap[i] = (struct page)PADDR_TO_KVADDR(a1 + i * page_size);
                coremap[i]->pa = a1 + i * PAGE_SIZE;
                coremap[i]->state = FREE;    // state of page initially free
		coremap[i]->size = -1;
        }
}

// checks if there is availble physical memory of size 'npages' available in coremap
static bool free_space_check(unsigned long npages, int i){
	for (int j = i ; j < coremap_size; j++){
		if (coremap[j]->state == USED) return false;	
		if (npages == 1) return true;
		npages -= 1;
	}
	return false;	// not enough available frames
}

// get next available physical page and return it
paddr_t getppages(unsigned long npages){
	paddr_t a = 0;				// initially set to zero for case when there is no available physical addr found
	for (int i = 0 ; i < coremap_size ; i++){
		if (coremap[i]->state == FREE && free_space_check(npages - 1, i + 1)){	// if if enough frames are available to use
			a = coremap[i]->pa;
			coremap[i]->size = npages;
			for (unsigned int j = i ; j < i + npages ; j++){
				coremap[j]->state = USED;
			}
		}
	}
 	return a;
}

void free_pages(paddr_t paddr){
	for (int i = 0 ; i < coremap_size ; i++){
		if (coremap[i]->pa == paddr && coremap[i]->state == USED){	// if correct physical address is found
			int size = coremap[i]->size;
			for (int j = i ; j < size + i ; j++){
				coremap[j]->state = FREE;	// free entries that is no longer be used
				coremap[j]->size = -1;	// and set their lengths to -1
			}
		}
	}
}

#endif
