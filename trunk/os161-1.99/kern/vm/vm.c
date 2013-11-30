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
#include <coremap.h>
#include <uw-vmstats.h>
#include "pt.h"
#include <kern/iovec.h>
#include <uio.h>

#define DUMBVM_STACKPAGES    12
#define PAGE_FRAME 0xfffff000   /* mask for getting page number from addr */
//#define OFFSET_MASK 0x00000fff  /* mask for getting offset */

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

static
int
load_segment(struct addrspace *as, struct vnode *v,
         off_t offset, vaddr_t vaddr,
         size_t memsize, size_t filesize,
         int is_executable)
{
    struct iovec iov;
    struct uio u;
    int result;



    if (filesize > memsize) {
        kprintf("ELF: warning: segment filesize > segment memsize\n");
        filesize = memsize;
    }

    DEBUG(DB_EXEC, "ELF: Loading %lu bytes to 0x%lx\n",
          (unsigned long) filesize, (unsigned long) vaddr);

    iov.iov_ubase = (userptr_t)vaddr;
    iov.iov_len = memsize;       // length of the memory space
    u.uio_iov = &iov;
    u.uio_iovcnt = 1;
    u.uio_resid = filesize;          // amount to read from the file
    u.uio_offset = offset;
    u.uio_segflg = is_executable ? UIO_USERISPACE : UIO_USERSPACE;
    u.uio_rw = UIO_READ;
    u.uio_space = as;

    result = VOP_READ(v, &u);
    if (result) {
        return result;
    }

    if (u.uio_resid != 0) {
        /* short read; problem with executable? */
        kprintf("ELF: short read on segment - file truncated?\n");
        return ENOEXEC;
    }
	return result;
}
#endif

static
paddr_t
getppages2(unsigned long npages)
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

/*static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
} */


void
vm_bootstrap(void)
{
	#if OPT_A3
	
	int spl = splhigh();
	vmstats_init();
	paddr_t lo, hi, freeaddr;
    ram_getsize(&lo,&hi);
    freeaddr = lo;
    int page_num = hi / PAGE_SIZE;
    coremap_size = page_num;
    coremaps = (struct coremap*)PADDR_TO_KVADDR(lo);
    freeaddr += PAGE_SIZE + ((page_num * sizeof(struct coremap)) & 0xfffff000);
    init_coremap(freeaddr);
    alloc_kpages(freeaddr / PAGE_SIZE);
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
	kprintf("kvaddr: 0x%08x\n", PADDR_TO_KVADDR(pa));
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
	int i, vpn;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;
	bool cl;
	int seg = 0;
	int flag;
	struct pte* PTEAddr;
	
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
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
	
		flag = as->as_flag1;
		cl = as->as_complete_load1;
		seg = 1;
	
		//Get VPN from faultaddress
		vpn = (faultaddress - vbase1) / PAGE_SIZE;
		
		kprintf("fault on segment 1");	
		kprintf("vpn: %d\n", vpn);

		//find pte address for the VPN 
		PTEAddr = as->pt1 + (vpn * sizeof(struct pte)); 
	
		
		//fetch the PTE
		struct pte entry = *PTEAddr; 
		
		//check page accessablilty 
		if(entry.valid == 0){ 
			paddr = getppages(1); //create a page
			kprintf("paddr: 0x%08x\n", paddr);
			if(flag & 2){ //write permitted
				PTEAddr->pfn = paddr;
				PTEAddr->valid = 1;
				PTEAddr->dirty = 1; 
			}
			else{
				PTEAddr->pfn = paddr;
                PTEAddr->valid = 1;
                PTEAddr->dirty = 0;
			}
			kprintf("pa: 0x%08x\n", PTEAddr->pfn); 
		
		}
		else if(entry.dirty == 0 && faulttype == VM_FAULT_WRITE){ //segment is readonly
			//exception
			kprintf("dirty write");
			return EPERM;
		}
		else{
			//Access is allowed; fetch physical address
			paddr = entry.pfn;
		}

	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
        
		kprintf("fault on segment 2");
		
		flag = as->as_flag2;
        cl = as->as_complete_load2;
		seg = 2;

        //Get VPN from faultaddress
        vpn = (faultaddress - vbase2) / PAGE_SIZE;
	
		kprintf("fault on segment 2");
		kprintf("vpn: %d\n", vpn);

        //find pte address for the VPN 
        PTEAddr = as->pt2 + (vpn * sizeof(struct pte));

        //fetch the PTE
        struct pte entry = *PTEAddr;

        //check page accessablilty 
        if(entry.valid == 0){ 
            paddr = getppages(1); //create a page
			if(flag & 2){ //write permitted
				PTEAddr->pfn = paddr;
                PTEAddr->valid = 1;
                PTEAddr->dirty = 1;
			}
			else{
				PTEAddr->pfn = paddr;
                PTEAddr->valid = 1;
                PTEAddr->dirty = 0;
			}

        }
        else if(entry.dirty == 0 && faulttype == VM_FAULT_WRITE){ //segment is readonly
            //raise exception
			return EPERM;
        }
        else{
            // Access is allowed; fetch physical address
			paddr = entry.pfn;
        }

    }
	else if (faultaddress >= stackbase && faultaddress < stacktop) {

		kprintf("fault on segment 3");
		seg = 3;
        //Get VPN from faultaddress
        vpn = (faultaddress - stackbase) / PAGE_SIZE;

		kprintf("vpn: %d\n", vpn);
	
        //find pte address for the VPN 
        PTEAddr = as->pt3 + (vpn * sizeof(struct pte));

        //fetch the PTE
        struct pte entry = *PTEAddr;

        //check page accessablilty 
        if(entry.valid == 0){ 
            paddr = getppages2(1); //create a page
            PTEAddr->pfn = paddr;
            PTEAddr->valid = 1;
            PTEAddr->dirty = 1;
	
        }
        else{
            // Access is allowed; fetch physical address
  			paddr = entry.pfn;
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
		/* load segment from elf */
		if(seg == 1){
			int d = PTEAddr->dirty;
			PTEAddr->dirty = 1;
			off_t off = (off_t)faultaddress - (off_t)vbase1 + as->offset1;
			kprintf("off: %d, faultaddress: 0x%08x, vbase: 0x%08x\n", (int)off, faultaddress, vbase1);
			if(as->filesz1 / PAGE_SIZE < (unsigned int)vpn){
            	bzero((void *) paddr, PAGE_SIZE);
        	}
			int result = load_segment(as, as->vn, off, faultaddress, PAGE_SIZE, PAGE_SIZE, as->is_exec1);
        	PTEAddr->dirty = d;
			if (result) {
            	return result;
        	}
		}
		if(seg == 2){
			int d = PTEAddr->dirty;
			PTEAddr->dirty = 1;
			off_t off = (off_t)faultaddress - (off_t)vbase2 + as->offset2;
			if(as->filesz2 / PAGE_SIZE < (unsigned int)vpn){
            	bzero((void *) paddr, PAGE_SIZE);
        	}
            int result = load_segment(as, as->vn, off, faultaddress, PAGE_SIZE, PAGE_SIZE, as->is_exec2);
            PTEAddr->dirty = d;
			if (result) {
                return result;
            }
		}
		if(seg == 3) {
			bzero((void *)faultaddress, PAGE_SIZE);
		}
		splx(spl);
		kprintf("faultaddr: 0x%08x\n", faultaddress);
		kprintf("ehi: 0x%08x, elo: 0x%08x\n", ehi, elo);
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
	/* load segment from elf */
    if(seg == 1){
		int d = PTEAddr->dirty;
		PTEAddr->dirty = 1;
    	off_t off = (off_t)faultaddress - (off_t)vbase1 + as->offset1;
    	int result = load_segment(as, as->vn, off, faultaddress, PAGE_SIZE, PAGE_SIZE, as->is_exec1);
    	PTEAddr->dirty = d;
		kprintf("result: %di\n", result);
		if (result) {
        	return result;
        }
    }
   	if(seg == 2){
		int d = PTEAddr->dirty;
		PTEAddr->dirty = 1;
    	off_t off = (off_t)faultaddress - (off_t)vbase2 + as->offset2;
    	int result = load_segment(as, as->vn, off, faultaddress, PAGE_SIZE, PAGE_SIZE, as->is_exec2);
      	PTEAddr->dirty = d;
		if (result) {
       		return result;
     	}
   	}
	if(seg == 3) {
		bzero((void *)faultaddress, PAGE_SIZE);
	}
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

