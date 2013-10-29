#include "opt-A2.h"
#if OPT_A2
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
#include <array.h>
#include <mips/trapframe.h>


pid_t sys_fork(struct trapframe *tf) {

	struct proc *newproc;
	(void)tf;
	newproc->p_name = curproc->p_name; // name
	newproc->p_threads = curproc->p_threads; // thread
	newproc->p_cwd = curproc->p_cwd; // vnode
	newproc->p_addrspace = curproc->p_addrspace; // addrspace



	//result = thread_fork(args[0] /* thread name */,
	//		proc /* new process */,
	//		cmd_progthread /* thread function */,
	//		args /* thread arg */, nargs /* thread arg */);
	return 1;
}

pid_t sys_getpid() {
	return curthread->t_proc->p_pid;
}

pid_t sys_waitpid(pid_t pid, int *status, int options) {
	(void)status;
	(void)options; // do nothing with it.

	struct exitc *c; 

	struct proc* p = find_proc(pid);
	if(p == NULL) {
		c = find_exitc(pid);
		*status = c->exitcode;
		return pid;
	}

	lock_acquire(p->p_lk);
	while(find_proc(pid) != NULL) {
		cv_wait(p->p_cv, p->p_lk);
	}
	lock_release(p->p_lk);

	c = find_exitc(pid);
	*status = c->exitcode;

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
