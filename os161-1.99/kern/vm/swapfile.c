#include "opt-A3.h"
#include <types.h>
#include <vfs.h>
#include <vnode.h>
#include <uio.h>
#include <synch.h>
#include <vm.h>
#include <kern/fcntl.h>
#include <swapfile.h>
#include <uw-vmstats.h>
#if OPT_A3

void swap_initialize(){
	char *filename = kstrdup("swapfile");
	int ret = vfs_open(filename, O_RDWR|O_CREAT|O_TRUNC, 0,  &swapfile);	// open swapfile
	(void)ret;	// ret just used for open
	kfree(filename);
	
	swap_num = MAX_SIZE / PAGE_SIZE;	// define number of pages that would fit in swapfile
	swapLock = lock_create("swapLock");
//	swap_partition = kmalloc(sizeof(struct swap*) * swap_num);
	swap_partition = kmalloc(sizeof(bool) * swap_num);
	for (int i = 0 ; i < swap_num ; i++){	// initialize swap_parition(initially empty)
		swap_partition[i] = false;	
//		swap_partition[i]->entry = NULL;
	}
}

void read_from_swap(paddr_t pfn, int index){
        struct uio u;
        struct iovec iov;

        iov.iov_ubase = (void*)PADDR_TO_KVADDR(pfn * PAGE_SIZE);
//	iov.iov_ubase = NULL;
        iov.iov_len = PAGE_SIZE;

        u.uio_iov = &iov;
        u.uio_resid = PAGE_SIZE;
        u.uio_rw = UIO_WRITE;
        u.uio_segflg = UIO_USERSPACE;
        u.uio_space = NULL;
	VOP_READ(swapfile, &u);

        // free for somebody else to use it
//        swap_partition[index]->occupied = false;
	swap_partition[index] = false;
//        swap_partition[index]->entry = NULL;
	vmstats_inc(8);	// "Page Faults from Swapfile"
}

//int write_to_swap(struct pte* entry){
int write_to_swap(){
	int i;
	for (i = 0 ; i < swap_num ; i++){
		if (!swap_partition[i]){
			swap_partition[i] = true;
	//		swap_partition[i]->entry = entry;

			struct uio u;
			struct iovec iov;

			iov.iov_ubase = (void*)PADDR_TO_KVADDR(i * PAGE_SIZE);
//			iov.iov_ubase = NULL;
		        iov.iov_len = PAGE_SIZE;

		        u.uio_iov = &iov;
		        u.uio_resid = PAGE_SIZE;
	    	        u.uio_rw = UIO_WRITE;
			u.uio_segflg = UIO_USERSPACE;
		        u.uio_space = NULL;

			int result = VOP_WRITE(swapfile,&u);
			if (result < 0){		// possibly swapfile is full
				panic("Out of swap space");
			}
			break;		// leave for-loop
		}
	}
	vmstats_inc(9);		// swapfile_writes
	return i;
}

#endif
