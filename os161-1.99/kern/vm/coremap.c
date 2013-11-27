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
        core_table = kmalloc(sizeof(struct coremap*) * coremap_size);
//        core_table = (struct page*)PADDR_TO_KVADDR(a1);
        
        for (int i = 0 ; i < coremap_size ; i++){       // assign right value to each entry in page table
	        core_table[i] = kmalloc(sizeof(struct coremap));
//               core_table[i] = (struct page)PADDR_TO_KVADDR(a1 + i * page_size);
                core_table[i]->pa = a1 + i * PAGE_SIZE;
                core_table[i]->state = FREE;    // state of page initially free
		core_table[i]->size = -1;
        }
}

// get next available physical page and return it
paddr_t getppages(unsigned long npages){
	paddr_t a;
	unsigned long count = 0;					// we use count to count number of frames that are available to us
	for (int i = 0 ; i < coremap_size ; i++){
		if (core_table[i]->state == FREE){	// if if frame is available to use
			count += 1;			// add counts
		}
		if (count == npages){			// if we have enough free frames to use
			
		}
	}
	
 	return a;	// temporaily set
}

void free_pages(paddr_t paddr){
	for (int i = 0 ; i < coremap_size ; i++){
		if (core_table[i]->pa == paddr){
//			 core_table[i]->state = FREE;
		}
	}
}

#endif
