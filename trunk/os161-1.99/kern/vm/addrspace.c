/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#ifdef UW
#include <proc.h>
#endif
#include "opt-A3.h"

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

#if OPT_A3
#include <spl.h>
#include <mips/tlb.h>
#include <coremap.h>
#include <uw-vmstats.h>
#include "pt.h"

#define DUMBVM_STACKPAGES    12


/*static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}*/

#endif

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */
	#if OPT_A3

	as->as_vbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_npages2 = 0;

	as->pt1 = NULL;
    as->pt2 = NULL;

    as->pt3 = kmalloc(STACKPAGES * sizeof(struct pte));
	for(int i = 0; i < STACKPAGES; i++){
		pte_create(&(as->pt3[i]),0,0,0);
	}

	#endif

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{

	/*
	 * Write this.
	 */
	#if OPT_A3

	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;

	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}


	
	*ret = new;

	#else

	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	(void)old;
	
	*ret = newas;

	#endif

	return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	
	kfree(as);
}

void
as_activate(void)
{
	#if OPT_A3
	
	int i, spl;
	struct addrspace *as;

	as = curproc_getas();
#ifdef UW
        /* Kernel threads don't have an address spaces to activate */
#endif
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();
	vmstats_inc(3);
	// kprintf("tlb invalid\n");
	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);

	#else
	
	struct addrspace *as;

	as = curproc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}
	
	#endif

	/*
	 * Write this.
	 */
}

void
#ifdef UW
as_deactivate(void)
#else
as_dectivate(void)
#endif
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything.
	 */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */
	#if OPT_A3

	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	// kprintf("npages: %d\n", npages);
	// kprintf("R: %d\nW: %d\nE: %d\n", readable, writeable, executable);

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		//kprintf("vaddr(1): 0x%08x\n", vaddr);
		as->as_npages1 = npages;
		as->as_flag1 = readable | writeable | executable;
		//kprintf("npages(1): %d\n", npages * sizeof(struct pte));
		//as->pt1 = kmalloc(npages * sizeof(struct pte));
		int pnum;
		if(npages * sizeof(struct pte) % PAGE_SIZE) {
			
		}
		as->pt1 = (void *)alloc_kpages(4);
		// kprintf("0x%08x\n",(unsigned int)as->pt1);
		for(unsigned int i = 0; i < npages; i++){ //initialize pt1
			pte_create(&(as->pt1[i]),0,0,0);
		}
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		//kprintf("vaddr(2): 0x%08x\n", vaddr);
		as->as_npages2 = npages;
		as->as_flag2 = readable | writeable | executable;
		//kprintf("npages(2): %d\n", npages * sizeof(struct pte));
        //as->pt2 = kmalloc(npages * sizeof(struct pte));
        as->pt2 = (void *)alloc_kpages(4);
        // kprintf("pt2: 0x%08x\n", (unsigned int)as->pt2);
		for(unsigned int i = 0; i < npages; i++){ //initialize pt2
			pte_create(&(as->pt2[i]),0,0,0);
		}
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;

	#else

	(void)as;
	(void)vaddr;
	(void)sz;
	(void)readable;
	(void)writeable;
	(void)executable;
	return EUNIMP;

	#endif
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;


}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	#if OPT_A3
//	as->as_complete_load1 = true;
//	as->as_complete_load2 = true;
	 (void) as;
	 reset_coremap();
	return 0;
	#else

	(void)as;
	return 0;
	#endif
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	//#if OPT_A3
	//KASSERT(as->as_stackpbase != 0);
	//#else
	(void)as;
	//#endif

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
}

