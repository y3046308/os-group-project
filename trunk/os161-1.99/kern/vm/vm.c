
#ifdef UW
/* This was added just to see if we can get things to compile properly and
 * to provide a bit of guidance for assignment 3 */

#include "opt-vm.h"
#if OPT_VM

#include <types.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include "opt-A3.h"

#if OPT_A3
/* You will need to call this at some point */
#include <mips/tlb.h>
#include <kern/errno.h>
#include <current.h>
#include <elf.h>
#include <spl.h>
#include <proc.h>
#include <coremap.h>
#include <uw-vmstats.h>

#define DUMBVM_STACKPAGES    12
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

static
paddr_t
getppages(unsigned long npages)
{
   /* Adapt code form dumbvm or implement something new */
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
#endif

void
vm_bootstrap(void)
{
	#if OPT_A3
	int spl = splhigh();
	vmstats_init();
/*
	// initialize coremap
        paddr_t a1 = 0, a2 = 0;
        ram_getsize(&a1, &a2);   // get number of physical pages
	coremap_size = ROUNDDOWN(a2, PAGE_SIZE) / PAGE_SIZE;
	//page_table = kmalloc(sizeof(struct page*) * coremap_size);
	page_table = (struct page*)PADDR_TO_KVADDR(a1);
	
	for (int i = 0 ; i < coremap_size ; i++){	// assign right value to each entry in page table
//		page_table[i] = kmalloc(sizeof(struct page));
		page_table[i] = (struct page)PADDR_TO_KVADDR(a1 + i * page_size);
		page_table[i]->va = a1 + i * PAGE_SIZE;
		page_table[i]->state = FREE;	// state of page initially free
	}
*/
	splx(spl);
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
	KASSERT(as->as_pbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
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
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		flag = as->as_flag2;
		cl = as->as_complete_load2;
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
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
				vmstats_inc(4);
				return 0;
			}
			continue;
		}

		vmstats_inc(0);
		vmstats_inc(1);

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

	vmstats_inc(0);
	vmstats_inc(2);
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

