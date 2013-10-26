#include <types.h>
#include <kern/errno.h>
#include <../../user/include/errno.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <addrspace.h>
#include <vnode.h>
#include <syscall.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <current.h>
#include <kern/fcntl.h>
#include "opt-A2.h"
#if OPT_A2
#include <array.h>


pid_t sys_getpid() {
	return curthread->t_proc->p_pid;
}

pid_t sys_waitpid(pid_t pid, int *status, int options) {
	(void)status;
	(void)options;

	struct proc* p = find_proc(pid);
	if(p == NULL) return pid;

	lock_acquire(p->p_lk);
	while(find_proc(pid) != NULL) {
		cv_wait(p->p_cv, p->p_lk);
	}
	lock_release(p->p_lk);

	return pid;
}

void sys__exit(int exitcode) {

	if(codes == NULL) {
		codes = exitcarray_create();
		exitcarray_init(codes);
	}

	struct exitc *c;
	c->exitcode = exitcode;
	c->pid = curthread->t_proc->p_pid;

	exitcarray_add(codes, c, NULL);

	threadarray_remove(&curproc->p_threads, 0);
	proc_destroy(curthread->t_proc);
	curthread->t_proc = NULL;
	thread_exit();
	// code below is removed. Since the process is destroyed, there is no "curthread" now.
	// We cannot access to it.
	// curthread->t_proc = NULL;
}

#endif
