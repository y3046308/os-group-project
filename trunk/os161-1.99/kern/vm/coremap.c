#include <types.h>
#include <lib.h>
#include "opt-vm.h"
#include "opt-A3.h"
#include <coremap.h>
#include <spinlock.h>
#include <addrspace.h>
#include <vm.h>

#if OPT_A3

static paddr_t page_start = 0;

void init_coremap(paddr_t freeaddr){
	// kprintf("initializing...\n");
	page_start = freeaddr;
    for (int i = 0 ; i < coremap_size ; i++){
    	// kprintf(" %d", i);
        coremaps[i].pa = freeaddr + i * PAGE_SIZE;
        coremaps[i].state = FREE;    // state of page initially free
        coremaps[i].size = 0;
        coremaps[i].page_num = 0;
    }
    // kprintf("\n");

    kprintf("start addr: 0x%08x\n", page_start);
}

// checks if there is availble physical memory of size 'npages' available in coremap

static
paddr_t
find_free_frame(int npages) {
	int current_size = 0;
	paddr_t rpa = 0;
	for(int i = 0 ; i < coremap_size ; i++) { // loop through 
		kprintf("checking: %d\n", i);
		if(coremaps[i].state == FREE) {
			if (current_size == 0) {
				rpa = coremaps[i].pa;
			}
			current_size++;
			if(current_size == npages) {	
				for(int j = 0 ; j < npages ; j++) {
					coremaps[i - j].state = USED;
					coremaps[i - j].size = npages * PAGE_SIZE;
					coremaps[i].page_num = npages;
				}
				kprintf("page id: %d ~ %d\n", (i-npages+1), i);
				kprintf("paddr: 0x%08x\n\n", rpa);
				return rpa;
			}
		} else {
			current_size = 0;
		}
	}

	// not found:
	return 0;
}

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;


// free the physical address.
void
freeppages(paddr_t paddr) {
	int index = (paddr - page_start) / PAGE_SIZE; // index calc.
	int psize = coremaps[index].page_num; // get pagesize.
	for(int i = 0 ; i < psize ; i++) { // loop through pagesize

		coremaps[i+index].state = FREE;
		coremaps[i+index].size = 0;
		coremaps[i+index].page_num = 0;
	}
}

// get next available physical page and return it
paddr_t
getppages(unsigned long npages)
{
	// Adapt code form dumbvm or implement something new 
	#if OPT_A3
	kprintf("requested %d pages\n",(int)npages);
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);
	if(npages > 0) {
		addr = find_free_frame(npages);
	} else {
		addr = 0;
	}
	if(addr == 0) {
		addr = ram_stealmem(npages);
	}
	spinlock_release(&stealmem_lock);
	return addr;
	#else
	 (void)npages;
	 panic("Not implemented yet.\n");
   return (paddr_t) NULL;
   #endif
}

/*void free_pages(paddr_t paddr){
	for (int i = 0 ; i < coremap_size ; i++){
		if (coremap[i]->pa == paddr && coremap[i]->state == USED){	// if correct physical address is found
			int size = coremap[i]->size;
			for (int j = i ; j < size + i ; j++){
				coremap[j]->state = FREE;	// free entries that is no longer be used
				coremap[j]->size = -1;	// and set their lengths to -1
			}
		}
	}
}*/

#endif
