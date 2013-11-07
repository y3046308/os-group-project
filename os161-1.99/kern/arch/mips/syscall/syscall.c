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
#include <../../user/include/errno.h>
#include <kern/syscall.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include "opt-A2.h"
#if OPT_A2
#include <addrspace.h>
#include <proc.h>
#include <synch.h>
#endif

int errno;

/*
 * System call dispatcher.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. 64-bit arguments are passed in *aligned*
 * pairs of registers, that is, either a0/a1 or a2/a3. This means that
 * if the first argument is 32-bit and the second is 64-bit, a1 is
 * unused.
 *
 * This much is the same as the calling conventions for ordinary
 * function calls. In addition, the system call number is passed in
 * the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, or v0 and v1 if 64-bit. This is also like an ordinary
 * function call, and additionally the a3 register is also set to 0 to
 * indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/user/lib/libc/arch/mips/syscalls-mips.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * If you run out of registers (which happens quickly with 64-bit
 * values) further arguments must be fetched from the user-level
 * stack, starting at sp+16 to skip over the slots for the
 * registerized values, with copyin().
 */
void
syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	KASSERT(curthread != NULL);
	KASSERT(curthread->t_curspl == 0);
	KASSERT(curthread->t_iplhigh_count == 0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;

	switch (callno) {
	    case SYS_reboot:
		err = sys_reboot(tf->tf_a0);
		break;

	    case SYS___time:
		err = sys___time((userptr_t)tf->tf_a0,
				 (userptr_t)tf->tf_a1);
		break;

	    /* Add stuff here */
#if OPT_A2
		case SYS_open:
			retval = sys_open((char *)tf->tf_a0,tf->tf_a1,tf->tf_a2);
			if (retval < 0)	err = errno;
			break;
		case SYS_close:
			retval = sys_close(tf->tf_a0);
			if (retval < 0) err = errno;
			break;
		case SYS_read:
	    		retval = sys_read(tf->tf_a0,(void *)tf->tf_a1,tf->tf_a2);
			if (retval < 0) err = errno;
			break;
		case SYS_write:
			retval = sys_write(tf->tf_a0,(void *)tf->tf_a1,tf->tf_a2);
			if (retval < 0) err = errno;
			break;
		case SYS_fork:
			retval = sys_fork(tf); // actually, do not need to set retval. it always return 0.
			break;
		case SYS_waitpid:
			retval = sys_waitpid(tf->tf_a0, (int *)tf->tf_a1, tf->tf_a2);
			break;
		case SYS_getpid:
			retval = sys_getpid();
			break;
		case SYS__exit:
			sys__exit(1);
			break;
#endif

	    default:
		kprintf("Unknown syscall %d\n", callno);
		err = ENOSYS;
		break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	KASSERT(curthread->t_curspl == 0);
	/* ...or leak any spinlocks */
	KASSERT(curthread->t_iplhigh_count == 0);
}

/*
 * Enter user mode for a newly forked process.
 *
 * This function is provided as a reminder. You need to write
 * both it and the code that calls it.
 *
 * Thus, you can trash it and do things another way if you prefer.
 */

void enter_forked_process(void *tf, unsigned long unused) {

	// tf - trapframe
	// unused - addrspace

	lock_acquire(curproc->p_lk);

	struct trapframe *ptf = tf;

	// address space
	struct addrspace * newas;
	as_copy((struct addrspace *)unused, &newas); // copy old addrspace to new one
	curproc_setas(newas); // set the curproc's addrspace
	as_activate(); // activate it

	/*ptf->tf_v0 = 0;
	ptf->tf_a3 = 0;*/

	// trapframe copy.
	struct trapframe ctf;
	ctf.tf_vaddr = ptf->tf_vaddr;
	ctf.tf_status = ptf->tf_status;
	ctf.tf_cause = ptf->tf_cause;
	ctf.tf_lo = ptf->tf_lo;
	ctf.tf_hi = ptf->tf_hi;
	ctf.tf_ra = ptf->tf_ra;
	ctf.tf_at = ptf->tf_at;
	ctf.tf_v0 = 0;
	ctf.tf_v1 = ptf->tf_v1;
	ctf.tf_a0 = ptf->tf_a0;
	ctf.tf_a1 = ptf->tf_a1;
	ctf.tf_a2 = ptf->tf_a2;
	ctf.tf_a3 = 0;
	ctf.tf_t0 = ptf->tf_t0;
	ctf.tf_t1 = ptf->tf_t1;
	ctf.tf_t2 = ptf->tf_t2;
	ctf.tf_t3 = ptf->tf_t3;
	ctf.tf_t4 = ptf->tf_t4;
	ctf.tf_t5 = ptf->tf_t5;
	ctf.tf_t6 = ptf->tf_t6;
	ctf.tf_t7 = ptf->tf_t7;
	ctf.tf_s0 = ptf->tf_s0;
	ctf.tf_s1 = ptf->tf_s1;
	ctf.tf_s2 = ptf->tf_s2;
	ctf.tf_s3 = ptf->tf_s3;
	ctf.tf_s4 = ptf->tf_s4;
	ctf.tf_s5 = ptf->tf_s5;
	ctf.tf_s6 = ptf->tf_s6;
	ctf.tf_s7 = ptf->tf_s7;
	ctf.tf_t8 = ptf->tf_t8;
	ctf.tf_t9 = ptf->tf_t9;
	ctf.tf_k0 = ptf->tf_k0;
	ctf.tf_k1 = ptf->tf_k1;
	ctf.tf_gp = ptf->tf_gp;
	ctf.tf_sp = ptf->tf_sp;
	ctf.tf_s8 = ptf->tf_s8;
	ctf.tf_epc = ptf->tf_epc + 4;

	lock_release(curproc->p_lk);

	mips_usermode(&ctf);
}
