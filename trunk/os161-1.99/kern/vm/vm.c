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

// static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

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
    u.uio_segflg = UIO_SYSSPACE;
    //u.uio_segflg = is_executable ? UIO_USERISPACE : UIO_USERSPACE;
    u.uio_rw = UIO_READ;
    //u.uio_space = as;
    u.uio_space = NULL;
    (void)as;
    (void)is_executable;

    result = VOP_READ(v, &u);
    if (result) {
    	kprintf("result(load_segment): %d\n",result);
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

/*static
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
}*/

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
	paddr_t lo, hi;
    ram_getsize(&lo,&hi);
    kprintf("top: %d\n",hi);
    kprintf("bot: %d\n",lo);
    freeaddr = lo;
    paddr_max = hi;
    int page_num = (hi - lo) / PAGE_SIZE;
    page_num -= 2;
    coremap_size = page_num;
    coremaps = (struct coremap*)PADDR_TO_KVADDR(lo);
    int remainder = (page_num * sizeof(struct coremap)) % PAGE_SIZE;
    if(remainder) {
    	freeaddr += PAGE_SIZE + ((page_num * sizeof(struct coremap)) & 0xfffff000);
    } else {
    	freeaddr += ((page_num * sizeof(struct coremap)) & 0xfffff000);
    }
    init_coremap(freeaddr);
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
	//kprintf("d");
	pa = getppages(npages, KERNEL);
	if (pa==0) {
		return 0;
	}
	//kprintf("KMALLOC: 0x%08x\n", PADDR_TO_KVADDR(pa));
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

	//kprintf("free address: 0x%08x\n", addr);

	/*int vpn;
	//paddr_t pa = 0;
	vaddr_t vbase1, vbase2, vtop1, vtop2, stackbase, stacktop;
	struct pte* PTEAddr;

	struct addrspace *as = curthread->t_proc->p_addrspace;

	if(as == NULL) {
		vpn = 1;
	}

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if(vbase1 <= addr && addr < vtop1) {
        vpn = (addr - vbase1) / PAGE_SIZE;
        PTEAddr = as->pt1 + (vpn * sizeof(struct pte)); 
	} else if(vbase2 <= addr && addr < vtop2) {
        vpn = (addr - vbase2) / PAGE_SIZE;
        PTEAddr = as->pt2 + (vpn * sizeof(struct pte)); 
	} else if (addr >= stackbase && addr < stacktop) {
        vpn = (addr - stackbase) / PAGE_SIZE;
        PTEAddr = as->pt3 + (vpn * sizeof(struct pte)); 
	} else {
		return;
	}
	//fetch the PTE
	struct pte entry = *PTEAddr; 
	
	freeppages(entry.pfn);*/
	paddr_t paddr = addr - 0x80000000;
	kprintf("freeing paddr: 0x%08x\n",paddr);
	freeppages(paddr);


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

static unsigned int counter = 0;

#endif

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
  /* Adapt code form dumbvm or implement something new */
	#if OPT_A3
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i, vpn;
	uint32_t ehi = 0, elo = 0;
	struct addrspace *as;
	int spl;
	int seg = 0;
	int flag;
	struct pte* PTEAddr;
	bool load = false;

	kprintf("before align: 0x%08x\n", faultaddress);
	faultaddress &= PAGE_FRAME;

	// kprintf("faultaddr: 0x%08x\n", faultaddress);

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
		seg = 1;
	
		//Get VPN from faultaddress
		vpn = (faultaddress - vbase1) / PAGE_SIZE;
		
		//kprintf("fault on segment 1");	
		//kprintf("vpn: %d\n", vpn);

		//find pte address for the VPN 
		PTEAddr = &(as->pt1[vpn]);
	
		
		//fetch the PTE
		struct pte entry = *PTEAddr; 
		
		//check page accessablilty 
		if(entry.valid == 0){ 
			//kprintf("a");
			paddr = getppages(1, USER); //create a page
			if(paddr == 0) {
				return ENOMEM;
			}
			//kprintf("paddr: 0x%08x\n", paddr);
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
			load = true;
			//kprintf("pa: 0x%08x\n", PTEAddr->pfn); 
		
		}
		else if(entry.dirty == 0 && faulttype == VM_FAULT_WRITE){ //segment is readonly
			//exception
			//kprintf("dirty write");
			return EPERM;
		}
		else{
			//Access is allowed; fetch physical address
			paddr = entry.pfn;
		}

	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
        
		//kprintf("fault on segment 2");
		
		flag = as->as_flag2;
		seg = 2;

        //Get VPN from faultaddress
        vpn = (faultaddress - vbase2) / PAGE_SIZE;
		//kprintf("vpn: %d\n", vpn);

        //find pte address for the VPN 
        PTEAddr = &(as->pt2[vpn]);

        //fetch the PTE
        struct pte entry = *PTEAddr;

        //check page accessablilty 
        if(entry.valid == 0){ 
            paddr = getppages(1, USER); //create a page
            if(paddr == 0) {
            	//kprintf("out of memory. %d\n",counter);
				return ENOMEM;
			}
            //kprintf("paddr: 0x%08x\nindex:%d\n", paddr,vpn);
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
			load = true;

        }
        else if(entry.dirty == 0 && faulttype == VM_FAULT_WRITE){ //segment is readonly
            //raise exception
			return EPERM;
        }
        else{
            // Access is allowed; fetch physical address
			paddr = entry.pfn;
			//kprintf("paddr(pfn): 0x%08x\n", paddr);
        }

    }
	else if (faultaddress >= stackbase && faultaddress < stacktop) {

		flag = PF_R | PF_W;

		//kprintf("fault on segment 3\n");
		seg = 3;
        //Get VPN from faultaddress
        vpn = (faultaddress - stackbase) / PAGE_SIZE;

		//kprintf("vpn: %d\n", vpn);
	
        //find pte address for the VPN 
        PTEAddr = &(as->pt3[vpn]);

        //fetch the PTE
        struct pte entry = *PTEAddr;

        //check page accessablilty 
        if(entry.valid == 0){ 
            paddr = getppages(1, USER); //create a page
            if(paddr == 0) {
				return ENOMEM;
			}
            // kprintf("page fault");
            PTEAddr->pfn = paddr;
            PTEAddr->valid = 1;
            PTEAddr->dirty = 1;
        }
        else{
            // Access is allowed; fetch physical address
  			paddr = entry.pfn;
        }
        load = true;

    }
	else {
		kprintf("0x%08x\n", faultaddress);
		kprintf("???\n");
		panic("NOOO");
		return EFAULT;
	}

	/* make sure it's page-aligned */
	//kprintf("0x%08x\n0x%08x\nseg:%d\n", faultaddress, paddr,seg);
	KASSERT(paddr != 0);
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

		// spl = splhigh();
		vmstats_inc(0); //TLB Faults
		vmstats_inc(1); //TLB Faults with Free
		// splx(spl);

		ehi = faultaddress;
		if(!(flag & 2)) { // no write flag (readonly) and code load done
			// kprintf("this TLB(%d) is set to readonly. (dirty==0)\n",flag);
			elo = paddr | TLBLO_VALID;	
		} else {
			elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		}
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);

		
		//kprintf("(seg %d) TLB write on %d. ehi: 0x%08x, elo: 0x%08x\n", seg, i, ehi, elo);
		/* load segment from elf */
		if(load) {
			tlb_write(ehi, paddr | TLBLO_DIRTY | TLBLO_VALID, i);
			if(seg == 1){
				int d = PTEAddr->dirty;
				int result = 0;
				PTEAddr->dirty = 1;
				off_t off = (off_t)faultaddress - (off_t)vbase1 + as->offset1;
				//kprintf("off: %d, faultaddress: 0x%08x, vbase: 0x%08x\n", (int)off, faultaddress, vbase1);
				int index = as->filesz1 / PAGE_SIZE;
				size_t r = as->filesz1 % PAGE_SIZE;
				if(index > vpn){
					//kprintf("mid\n");
					result = load_segment(as, as->vn, off, PADDR_TO_KVADDR(paddr), PAGE_SIZE, PAGE_SIZE, as->is_exec1);
				}
				else if(index == vpn && r != 0){
					//kprintf("end\n");
					result = load_segment(as, as->vn, off, PADDR_TO_KVADDR(paddr), PAGE_SIZE, r, as->is_exec1);
				}
				else{
					// kprintf("LOLa");
					bzero((void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE);
				}
	        	PTEAddr->dirty = d;
	        	//kprintf("result: %d\n", result);
				if (result) {
	            	return result;
	        	}
			}
			if(seg == 2){
				int d = PTEAddr->dirty;
				int result = 0;
				PTEAddr->dirty = 1;
				off_t off = (off_t)faultaddress - (off_t)vbase2 + as->offset2;
				int index = as->filesz2 / PAGE_SIZE;
				size_t r = as->filesz2 % PAGE_SIZE;
	            if(index > vpn){
	                result = load_segment(as, as->vn, off, PADDR_TO_KVADDR(paddr), PAGE_SIZE, PAGE_SIZE, as->is_exec2);
	            }
	            else if(index == vpn && r != 0){
	                result = load_segment(as, as->vn, off, PADDR_TO_KVADDR(paddr), PAGE_SIZE, r, as->is_exec2);
	            }
				else{
					// kprintf("LOLb");
					bzero((void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE);
					counter++;
				}
	            PTEAddr->dirty = d;
				if (result) {
	                return result;
	            }
			}
			if(seg == 3) {
				// kprintf("LOLc");
				bzero((void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE);
				//kprintf("asd\n");
			}
			tlb_write(ehi, elo, i);
		} else {
			tlb_write(ehi, elo, i);
		}
		splx(spl);
		//kprintf("faultaddr: 0x%08x\n", faultaddress);
		//kprintf("ehi: 0x%08x, elo: 0x%08x\n", ehi, elo);
		return 0;
	}



	// kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	vmstats_inc(0); //TLB Faults
	vmstats_inc(2); //TLB Faults with Replace
	int victim = tlb_get_rr_victim();
	ehi = faultaddress;
	if(!(flag & 2)) { // no write flag (readonly) and code load done
		// kprintf("this TLB(%d) is set to readonly. (dirty==0)\n",flag);
		elo = paddr | TLBLO_VALID;	
	} else {
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	}
	DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
	if (load) {
		tlb_write(ehi, paddr | TLBLO_DIRTY | TLBLO_VALID, victim);
		/* load segment from elf */
	    if(seg == 1){
			int d = PTEAddr->dirty;
			PTEAddr->dirty = 1;
			int result = 0;
	    	off_t off = (off_t)faultaddress - (off_t)vbase1 + as->offset1;
	    	int index = as->filesz1 / PAGE_SIZE;
			size_t r = as->filesz1 % PAGE_SIZE;
			//kprintf("off: %d, faultaddress: 0x%08x, vbase: 0x%08x\n", (int)off, faultaddress, vbase1);
			//kprintf("physical address: 0x%08x\n", paddr);
	        if(index > vpn){
	            result = load_segment(as, as->vn, off, PADDR_TO_KVADDR(paddr), PAGE_SIZE, PAGE_SIZE, as->is_exec1);
	        }
	        else if(index == vpn && r != 0){
	            result = load_segment(as, as->vn, off, PADDR_TO_KVADDR(paddr), PAGE_SIZE, r, as->is_exec1);
	        }
			else{
				bzero((void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE);
			}
	    	PTEAddr->dirty = d;
			//kprintf("result: %di\n", result);
			if (result) {
	        	return result;
	        }
	    }
	   	if(seg == 2){
			int d = PTEAddr->dirty;
			PTEAddr->dirty = 1;
			int result = 0;
	    	off_t off = (off_t)faultaddress - (off_t)vbase2 + as->offset2;
	    	int index = as->filesz2 / PAGE_SIZE;
			size_t r = as->filesz2 % PAGE_SIZE;
	        if(index > vpn){
	            result = load_segment(as, as->vn, off, PADDR_TO_KVADDR(paddr), PAGE_SIZE, PAGE_SIZE, as->is_exec2);
	        }
	        else if(index == vpn && r != 0){
	            result = load_segment(as, as->vn, off, PADDR_TO_KVADDR(paddr), PAGE_SIZE, r, as->is_exec2);
	        }
			else{
				//kprintf("LOLe: ");
				// kprintf("paddr: 0x%08x\n", paddr);
				counter++;
				bzero((void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE);
			}
	      	PTEAddr->dirty = d;
			if (result) {
	       		return result;
	     	}
	   	}
		if(seg == 3) {
			//kprintf("LOLf");
			bzero((void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE);
		}
		tlb_write(ehi, elo, victim);
	} else {
		tlb_write(ehi, elo, victim);
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

