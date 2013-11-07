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
#include <spl.h>
#include <kern/wait.h>

int errno;

pid_t sys_fork(struct trapframe *tf) {


	if(procarray_num(procarr) == PID_MAX - PID_MIN) {
		errno = EMPROC;
		return 0;
	}

	struct proc *newproc = kmalloc(sizeof(*newproc));

	// normal field
	newproc->p_name = kstrdup(curthread->t_proc->p_name);  // name
	newproc->p_cwd = curproc->p_cwd; // vnode
	
	// synch field
	spinlock_init(&newproc->p_lock);
	newproc->p_cv = cv_create("proc cv");
	newproc->p_lk = lock_create("proc lock");
	newproc->open_num = 0;

	// files field
	newproc->fd_table = kmalloc(sizeof(struct fd) * MAX_fd_table);
	for(unsigned i = 0 ; i < MAX_fd_table ; i++) {
		newproc->fd_table[i] = curproc->fd_table[i];
	}

	// pid creation
	for(pid_t curid = PID_MIN ; curid <= PID_MAX ; curid++) { // loop through available pid range.
		if(find_proc(curid) == NULL) { // first available pid found.
			newproc->p_pid = curid; // set pid.
			break; // break loop.
		}
	}

	threadarray_init(&newproc->p_threads);
	spinlock_init(&newproc->p_lock);

	// copy threads
	// struct threadarray oldthreads = curproc->p_threads;
	pid_t result;

	procarray_add(procarr,newproc,NULL);

	// fork thread
	// struct thread *t = threadarray_get(&oldthreads,0);
	int spl = splhigh();
	result = thread_fork(curthread->t_proc->p_name,
			newproc,	
			enter_forked_process, tf, (unsigned long)curproc->p_addrspace
			);

	// parent return.
	splx(spl);

	return newproc->p_pid;
}

pid_t sys_getpid() {
	return curproc->p_pid;
}

pid_t sys_waitpid(pid_t pid, int *status, int options) {
	(void)options; // do nothing with it.

	struct exitc *c; 

	struct proc* p = find_proc(pid); // search for process
	if(p != NULL) {
		lock_acquire(p->p_lk);
		while(find_proc(pid) != NULL) {
			cv_wait(p->p_cv, p->p_lk);
		}
		lock_release(p->p_lk);
	}

	c = find_exitc(pid);
	*status = c->exitcode;

	/*unsigned num = exitcarray_num(codes);
	for (unsigned i = 0 ; i < num ; i++) {
		if (exitcarray_get(codes, i) == c) {
			exitcarray_remove(codes,i);
		}
	}*/

	return pid;
}

void sys__exit(int exitcode) {

	// array init
	if(codes == NULL) {
		codes = exitcarray_create();
		exitcarray_init(codes);
	}

	exitcode = _MKWAIT_EXIT(exitcode);

	// add to exitcode array
	struct exitc *c;
	c->exitcode = exitcode;
	c->pid = curthread->t_proc->p_pid;
	exitcarray_add(codes, c, NULL);

	// remove from procarray.
	unsigned num = procarray_num(procarr);
	for (unsigned i = 0 ; i < num ; i++) {
		if ((procarray_get(procarr, i)) == curproc) {
			procarray_remove(procarr,i);
			break;
		}
	}
	// pid_list[curproc->p_pid] = false;

	cv_signal(curproc->p_cv,curproc->p_lk);

	proc_remthread(curthread);
	thread_exit();
	// curthread->t_proc = NULL; //what is this??? you removed it at the bottom and then added it here?
	// code below is removed. Since the process is destroyed, there is no "curthread" now.
	// We cannot access to it.
	// curthread->t_proc = NULL;
}

#endif
