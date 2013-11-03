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
#include <limits.h>

int errno;

pid_t sys_fork() {

	if(procarray_num(procarr) == PID_MAX - PID_MIN) {
		errno = EMPROC;
		return 0;
	}

	struct proc *newproc = kmalloc(sizeof(*newproc));

	// copy
	newproc->p_name = curthread->t_proc->p_name;  // name
	newproc->p_cwd = curproc->p_cwd; // vnode
	newproc->p_addrspace = curproc->p_addrspace; // addrspace
	// create new
	spinlock_init(&newproc->p_lock);
	newproc->p_cv = cv_create("proc cv");
	newproc->p_lk = lock_create("proc lock");
	newproc->fd_table = NULL;
	// pid creation
	for(pid_t curid = PID_MIN ; curid <= PID_MAX ; curid++) { // loop through available pid range.
		if(find_proc(curid) == NULL) { // first available pid found.
			newproc->p_pid = curid; // set pid.
			break; // break loop.
		}
	}

	// copy threads
	struct threadarray oldthreads = curproc->p_threads;
	pid_t result;

	// fork threads
	for(unsigned i = 0 ; i < threadarray_num(&oldthreads) ; i++) {
		struct thread *t = threadarray_get(&oldthreads,i);
		result = thread_fork(t->t_name,
				newproc,
				t->entrypoint,
				t->data1,
				t->data2);
	}

	// return values
	if(result) { // parent
		return newproc->p_pid;
	} else { // child
		return 0;
	}
}

pid_t sys_getpid() {
	return curthread->t_proc->p_pid;
}

pid_t sys_waitpid(pid_t pid, int *status, int options) {
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
	//thread_exit();
	proc_destroy(curthread->t_proc);
	// curthread->t_proc = NULL; //what is this??? you removed it at the bottom and then added it here?
	// code below is removed. Since the process is destroyed, there is no "curthread" now.
	// We cannot access to it.
	// curthread->t_proc = NULL;
}

#endif
