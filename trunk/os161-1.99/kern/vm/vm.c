
#ifdef UW
/* This was added just to see if we can get things to compile properly and
 * to provide a bit of guidance for assignment 3 */
#include "opt-vm.h"
#if OPT_VM
#include "opt-A3.h"

#include <types.h>
#include <lib.h>
#if OPT_A3
#define PAGEINLINE
#endif
#include <addrspace.h>
#include <vm.h>

#if OPT_A3
/* You will need to call this at some point */
//#include <coremap.h>
#include <mips/tlb.h>
#include <kern/errno.h>
#include <current.h>
#include <elf.h>
#include <spl.h>
#include <proc.h>
#include <uw-vmstats.h>
#include <coremap.h>
#include "pt.h"

#define DUMBVM_STACKPAGES    12
#define PAGE_FRAME 0xfffff000   /* mask for getting page number from addr */
/*
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

static
paddr_t
getppages(unsigned long npages)
{
   // Adapt code form dumbvm or implement something new 
	#if OPT_A3
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);
	
	spinlock_release(&stealmem_lock);
	return addr;
	#else
	 (void)npages;
	 panic("Not implemented yet.\n");
   return (paddr_t) NULL;
   #endif
}
*/
static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}
#endif

void
vm_bootstrap(void)
{
	#if OPT_A3
	
	int spl = splhigh();
	vmstats_init();
	splx(spl);
	/* initialize coremap */
/*	paddr_t first, last;
	ram_getsize(&first, &last); //get first and last address of ram
	pgnum = (last - first)/PAGE_SIZE; // number of pages that fit
	page_array = (struct page*)PADDR_TO_KVADDR(first); //convert paddr to kvaddr
	avail_addr = first + pgnum * sizeof(struct page); */
		
	#endif
	/* May need to add code. */
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
   /* Adapt code form dumbvm or implement something new */
	#if OPT_A3

	paddr_t pa;
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
	
	#else
	
	 (void)npages;
	 panic("Not implemented yet.\n");
   	return (vaddr_t) NULL;
   
   	#endif
}

void 
free_kpages(vaddr_t addr)
{
	/* nothing - leak the memory. */

	(void)addr;
}

void
vm_tlbshootdown_all(void)
{
	panic("Not implemented yet.\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("Not implemented yet.\n");
}

#if OPT_A3
static
int
tlb_get_rr_victim()
{
	int victim;
	static unsigned int next_victim = 0;

	victim = next_victim;
	next_victim = (next_victim + 1) % NUM_TLB;
	
	return victim;
}

void vm_shutdown() {
	vmstats_print();
}

#endif

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
  /* Adapt code form dumbvm or implement something new */
	#if OPT_A3

	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;
	// bool readonly = false;
	//bool write = false;
	bool cl;
	int flag;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
			return EINVAL;
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = curproc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	// KASSERT(as->as_pbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	// KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	// KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	// KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		flag = as->as_flag1;
		cl = as->as_complete_load1;
		
		as_zero_region(as->as_pbase1, as->as_npages1);
		
		//Get VPN from faultaddress
		int vpn = (faultaddress & PAGE_FRAME) >> 12;

		//find pte address for the VPN 
		struct pte* PTEAddr = as->pt1 + (vpn * sizeof(struct pte)); 
		
		//fetch the PTE
		struct pte entry = *PTEAddr; 
			
		//check page accessablilty 
		if(entry.valid == 0){ 
			paddr_t paddr = getppages(1); //create a page
			struct pte entry;
			if(flag & 2){ //write permitted
            entry = pte_create(paddr, 1, 1); 
			}
			else{
			entry = pte_create(paddr, 1, 0);
			}
			*PTEAddr = entry; 
		
			//fetch physical address
			int offset = faultaddress << 20;
			paddr = (PTEAddr->pfn << 12) | offset;
		}
		else if(entry.dirty == 0){ //segment is readonly
			//raise exception
		}
		else{
			// Access is allowed; fetch physical address
			int offset = faultaddress << 20; 
			paddr = (PTEAddr->pfn << 12) | offset; //concatenating offset to pfn
		}

	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
        flag = as->as_flag2;
        cl = as->as_complete_load2;

        as_zero_region(as->as_pbase2, as->as_npages2);

        //Get VPN from faultaddress
        int vpn = (faultaddress & PAGE_FRAME) >> 12;

        //find pte address for the VPN 
        struct pte *PTEAddr = as->pt2 + (vpn * sizeof(struct pte));

        //fetch the PTE
        struct pte entry = *PTEAddr;

        //check page accessablilty 
        if(entry.valid == 0){ 
            paddr_t paddr = getppages(1); //create a page
			struct pte entry;
			if(flag & 2){ //write permitted
            entry = pte_create(paddr, 1, 1);  
			}
			else{
			entry = pte_create(paddr, 1, 0);
			}
            *PTEAddr = entry;

			//fetch physical address
			int offset = faultaddress << 20;
			paddr = (PTEAddr->pfn << 12) | offset;
        }
        else if(entry.dirty == 0){ //segment is readonly
            //raise exception
        }
        else{
            // Access is allowed; fetch physical address
            int offset = faultaddress << 20;
            paddr = (PTEAddr->pfn << 12) | offset; //concatenating offset to pfn
        }

    }
	else if (faultaddress >= stackbase && faultaddress < stacktop) {

        //Get VPN from faultaddress
        int vpn = (faultaddress & PAGE_FRAME) >> 12;

        //find pte address for the VPN 
        struct pte *PTEAddr = as->pt3 + (vpn * sizeof(struct pte));

        //fetch the PTE
        struct pte entry = *PTEAddr;

        //check page accessablilty 
        if(entry.valid == 0){ 
            paddr_t paddr = getppages(1); //create a page
            struct pte entry = pte_create(paddr, 1, 1); 
            *PTEAddr = entry;
	
			//fetch physical address
			int offset = faultaddress << 20;
			paddr = (PTEAddr->pfn << 12) | offset;
        }
        else if(entry.dirty == 0){ //segment is readonly
            //raise exception
        }
        else{
            // Access is allowed; fetch physical address
            int offset = faultaddress << 20;
            paddr = (PTEAddr->pfn << 12) | offset; //concatenating offset to pfn
        }

    }
	else {
		return EFAULT;
	}

	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			if (ehi == faultaddress) {
				vmstats_inc(4); //TLB Faults with Replace
				return 0;
			}
			continue;
		}

		vmstats_inc(0); //TLB Faults
		vmstats_inc(1); //TLB Faults with Free

		ehi = faultaddress;
		if(!(flag & 2) && cl) { // no write flag (readonly) and code load done
			// kprintf("this TLB(%d) is set to readonly. (dirty==0)\n",flag);
			elo = paddr | TLBLO_VALID;	
		} else {
			elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		}
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}



	// kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");

	vmstats_inc(0); //TLB Faults
	vmstats_inc(2); //TLB Faults with Replace
	int victim = tlb_get_rr_victim();
	ehi = faultaddress;
	if(!(flag & 2) && cl) { // no write flag (readonly) and code load done
		// kprintf("this TLB(%d) is set to readonly. (dirty==0)\n",flag);
		elo = paddr | TLBLO_VALID;	
	} else {
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	}
	DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
	tlb_write(ehi, elo, victim);
	splx(spl);
	
	return 0;
		
	#else
	
	(void)faulttype;
	(void)faultaddress;
	panic("Not implemented yet.\n");
  	return 0;
  	
  	#endif
}
#endif /* OPT_VM */

#endif /* UW */

