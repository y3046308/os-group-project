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
#include <copyinout.h>

#define DUMBVM_STACKPAGES 12
#define NARG_MAX 1024

int errno;

static bool valid_address_check(struct addrspace *as, vaddr_t addr){
	vaddr_t stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	vaddr_t stacktop = USERSTACK;
	if ((as->as_vbase1 <= addr && addr < (as->as_vbase1 + as->as_npages1 * PAGE_SIZE)) ||
	    (as->as_vbase2 <= addr && addr < (as->as_vbase2 + as->as_npages2 * PAGE_SIZE)) ||
	    (stackbase <= addr && addr < stacktop)) {
		return true;
	}
	return false;
}

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
	newproc->fd_table = kmalloc(sizeof(struct fd *) * MAX_fd_table);
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

	struct exitc *c = kmalloc(sizeof(struct exitc));
	c->exitcode = -1;
	c->pid = newproc->p_pid;
	exitcarray_add(codes,c,NULL);

	// fork thread
	// struct thread *t = threadarray_get(&oldthreads,0);

	// copy trapframe to heap.
	struct trapframe *ctf = kmalloc(sizeof(struct trapframe));
	ctf->tf_vaddr = tf->tf_vaddr;
	ctf->tf_status = tf->tf_status;
	ctf->tf_cause = tf->tf_cause;
	ctf->tf_lo = tf->tf_lo;
	ctf->tf_hi = tf->tf_hi;
	ctf->tf_ra = tf->tf_ra;
	ctf->tf_at = tf->tf_at;
	ctf->tf_v0 = tf->tf_v0;
	ctf->tf_v1 = tf->tf_v1;
	ctf->tf_a0 = tf->tf_a0;
	ctf->tf_a1 = tf->tf_a1;
	ctf->tf_a2 = tf->tf_a2;
	ctf->tf_a3 = tf->tf_a3;
	ctf->tf_t0 = tf->tf_t0;
	ctf->tf_t1 = tf->tf_t1;
	ctf->tf_t2 = tf->tf_t2;
	ctf->tf_t3 = tf->tf_t3;
	ctf->tf_t4 = tf->tf_t4;
	ctf->tf_t5 = tf->tf_t5;
	ctf->tf_t6 = tf->tf_t6;
	ctf->tf_t7 = tf->tf_t7;
	ctf->tf_s0 = tf->tf_s0;
	ctf->tf_s1 = tf->tf_s1;
	ctf->tf_s2 = tf->tf_s2;
	ctf->tf_s3 = tf->tf_s3;
	ctf->tf_s4 = tf->tf_s4;
	ctf->tf_s5 = tf->tf_s5;
	ctf->tf_s6 = tf->tf_s6;
	ctf->tf_s7 = tf->tf_s7;
	ctf->tf_t8 = tf->tf_t8;
	ctf->tf_t9 = tf->tf_t9;
	ctf->tf_k0 = tf->tf_k0;
	ctf->tf_k1 = tf->tf_k1;
	ctf->tf_gp = tf->tf_gp;
	ctf->tf_sp = tf->tf_sp;
	ctf->tf_s8 = tf->tf_s8;
	ctf->tf_epc = tf->tf_epc;

	//int spl = splhigh();
	result = thread_fork(curthread->t_proc->p_name,
			newproc,	
			enter_forked_process, ctf, (unsigned long)curproc->p_addrspace
			);

	// parent return.
	//splx(spl);

	return newproc->p_pid;
}

pid_t sys_getpid() {
	return curproc->p_pid;
}

pid_t sys_waitpid(pid_t pid, int *status, int options) {
	
	if(options != 0) { // only support option 0.
		errno = EINVAL;
		return -1;
	}
	if(pid == curproc->p_pid) {
		return pid;
	}

	struct exitc *c; 

	// status pointer validation
	vaddr_t sp = (vaddr_t)status;
	struct addrspace *as = curproc->p_addrspace;
	if (as != NULL && !valid_address_check(as, sp)) { // out of vaddr boundary for this proc
		errno = EFAULT;
		return -1;
	}

	struct proc* p = find_proc(pid); // search for process

	if(p != NULL) {
		lock_acquire(p->p_lk);
		while(find_proc(pid) != NULL) {
			cv_wait(p->p_cv, p->p_lk);
		}
		lock_release(p->p_lk);
	}

	c = find_exitc(pid);
	if(c) {
		*status = c->exitcode;
	} else {
		errno = ESRCH;
		return -1;
	}

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

	exitcode = _MKWAIT_EXIT(exitcode);

	// add to exitcode array
	struct exitc *c = find_exitc(curproc->p_pid);
	if(c) {
		c->exitcode = exitcode;
	}

	// remove from procarray.
	unsigned num = procarray_num(procarr);
	for (unsigned i = 0 ; i < num ; i++) {
		if ((procarray_get(procarr, i)) == curproc) {
			procarray_remove(procarr,i);
			break;
		}
	}
	// pid_list[curproc->p_pid] = false;

	cv_broadcast(curproc->p_cv,curproc->p_lk);

	proc_remthread(curthread);
	thread_exit();
	// curthread->t_proc = NULL; //what is this??? you removed it at the bottom and then added it here?
	// code below is removed. Since the process is destroyed, there is no "curthread" now.
	// We cannot access to it.
	// curthread->t_proc = NULL;
}

int sys_execv(userptr_t progname, userptr_t args) {
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	
	char * path = kmalloc(PATH_MAX);

	as = curproc_getas();

	if (as != NULL && !valid_address_check(as, (vaddr_t)args)) { // out of vaddr boundary for this proc
		errno = EFAULT;
		return -1;
	}
	if (as != NULL && !valid_address_check(as, (vaddr_t)progname)) { // out of vaddr boundary for this proc
		errno = EFAULT;
		return -1;
	}

	copyinstr(progname, path, PATH_MAX, NULL);

	// Open the executable, create a new address space and load the elf into it

	result = vfs_open((char *)path, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}
	// KASSERT(curproc_getas() == NULL);

	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	curproc_setas(as);
	as_activate();

	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}
	vfs_close(v);

	result = as_define_stack(as, &stackptr); // stackptr set
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}
	kfree(path);

	userptr_t karg, uargs, argbase;
	char * buffer, * bufend;
	buffer = kmalloc(sizeof(ARG_MAX));
	bufend = buffer;
	size_t resid, bufsize;
	size_t alen;
	size_t *offsets = kmalloc(NARG_MAX * sizeof(size_t));
	int size = 0;
	userptr_t arg;


	resid = bufsize = ARG_MAX;

	for(int i = 0 ; i < NARG_MAX ; i++) { // copyin from user.
		copyin(args, &karg, sizeof(userptr_t)); // copy the pointer.
		if(karg == NULL) break; // if NULL, break.
		copyinstr(karg, bufend, resid, &alen);
		offsets[i] = bufsize - resid;
		bufend += alen;
		resid -= alen;
		args += sizeof(userptr_t);
		size++;
	}

	size_t buflen = bufend - buffer; // length of buffer.
	vaddr_t stack = stackptr - buflen; // current stack position.
	stack -= (stack & (sizeof(void *) - 1)); // alignment
	argbase = (userptr_t)stack; // argbase.

	copyout(buffer, argbase, buflen); // copy the arguments into stack.

	stack -= (size + 1)*sizeof(userptr_t); // go to array pointer (bottom of stack).
	uargs = (userptr_t)stack; // got stack.

	for(int i = 0 ; i < size ; i++) { // copy the elements
		arg = argbase + offsets[i];
		copyout(&arg, uargs, sizeof(userptr_t));
		uargs += sizeof(userptr_t); // 4
	}
	arg = NULL;
	copyout(&arg, uargs, sizeof(userptr_t)); // copy the NULL pointer.



	kfree(buffer);
	kfree(offsets);

	enter_new_process(size /*argc*/, (userptr_t)stack /*userspace addr of argv*/,
			  stack, entrypoint);

	return 0;

}

#endif
