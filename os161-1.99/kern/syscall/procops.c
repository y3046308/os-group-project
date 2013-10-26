#include <types.h>
#include <kern/errno.h>
#include <../../user/include/errno.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <addrspace.h>
#include <vnode.h>
#include <syscall.h>
#include <vfs.h>
#include <vnode.h>
#include <current.h>
#include <kern/fcntl.h>
#include "opt-A2.h"
#if OPT_A2

void sys__exit(int exitcode) {
	threadarray_remove(&curproc->p_threads, 0);
	proc_destroy(curthread->t_proc);
	curthread->t_proc = NULL;
	thread_exit();
	// code below is removed. Since the process is destroyed, there is no "curthread" now.
	// We cannot access to it.
	// curthread->t_proc = NULL;
	(void)exitcode;
}

#endif
