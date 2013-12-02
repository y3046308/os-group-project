#include <types.h>
#include <lib.h>
#include "opt-vm.h"
#include "opt-A3.h"
#include <coremap.h>
#include <spinlock.h>
#include <addrspace.h>
#include <spl.h>
#include <vm.h>

#if OPT_A3

static paddr_t page_start = 0;
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

void init_coremap(paddr_t freeaddr){
	kprintf("initializing %d coremaps...\n", coremap_size);
	page_start = freeaddr;
    for (int i = 0 ; i < coremap_size ; i++){
    	// kprintf(" %d", i);
        coremaps[i].pa = freeaddr + i * PAGE_SIZE;
        coremaps[i].state = FREE;    // state of page initially free
        coremaps[i].size = 0;
        coremaps[i].page_num = 0;
        coremaps[i].owner = NO;
    }
    // kprintf("\n");

    kprintf("start addr: 0x%08x\n", page_start);
}

void reset_coremap() {
	spinlock_acquire(&stealmem_lock);
	for(int i = 0 ; i < coremap_size ; i++) {
		if(coremaps[i].owner == USER) {
			coremaps[i].state = FREE;
			coremaps[i].size = 0;
			coremaps[i].page_num = 0;
			coremaps[i].owner = NO;
			bzero((void*)PADDR_TO_KVADDR(coremaps[i].pa),PAGE_SIZE);
		}
	}
	spinlock_release(&stealmem_lock);
}

// checks if there is availble physical memory of size 'npages' available in coremap

static
paddr_t
find_free_frame(int npages, frame_owner owner) {
	int current_size = 0;
	paddr_t rpa = 0;
	for(int i = 0 ; i < coremap_size ; i++) { // loop through 
		// kprintf("checking: %d\n", i);
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
					coremaps[i].owner = owner;
				}
				//kprintf("page id: %d ~ %d\n", (i-npages+1), i);
				//kprintf("paddr: 0x%08x\n", rpa);
				return rpa;
			}
		} else {
			current_size = 0;
		}
	}

	// not found:
	return 0;
}


// free the physical address.
void
freeppages(paddr_t paddr) {
	int index = (paddr - page_start) / PAGE_SIZE; // index calc.
	int psize = coremaps[index].page_num; // get pagesize.
	spinlock_acquire(&stealmem_lock);
	for(int i = 0 ; i < psize ; i++) { // loop through pagesize
		coremaps[i+index].state = FREE;
		coremaps[i+index].size = 0;
		coremaps[i+index].page_num = 0;
		coremaps[i+index].owner = NO;
		//kprintf("freed %dth coremap: 0x%08x\n", i+index,coremaps[i+index].pa);
		bzero((void*)PADDR_TO_KVADDR(coremaps[i+index].pa),PAGE_SIZE);
	}
	spinlock_release(&stealmem_lock);
}

// get next available physical page and return it
paddr_t
getppages(unsigned long npages, frame_owner owner)
{
	//kprintf("requested %d pages\n",(int)npages);
	//int spl = splhigh();
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);
	if(npages > 0) {
		addr = find_free_frame(npages, owner);
		if(addr >= paddr_max - 4096){
			//splx(spl);
			return 0;
		}
	} else {
		addr = 0;
	}
	if(addr == 0) {
		addr = ram_stealmem(npages);
	}
	spinlock_release(&stealmem_lock);
	////kprintf("return addr: 0x%08x\n", addr);
	//splx(spl);
	return addr;
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
