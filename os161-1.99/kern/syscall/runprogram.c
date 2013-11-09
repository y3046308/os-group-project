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

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include "opt-A2.h"
#include "limits.h"
#include "copyinout.h"

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
#if OPT_A2
#define NARG_MAX 1024

int
runprogram(char** argv, unsigned long argc)
{
#else
int
runprogram(char *progname)
{
#endif
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
#if OPT_A2
	result = vfs_open(argv[0], O_RDONLY, 0, &v);
#else
	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
#endif
	if (result) {
		return result;
	}

	/* We should be a new process. */
	KASSERT(curproc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}
#if OPT_A2

	size_t *offsets = kmalloc(NARG_MAX * sizeof(size_t)); //array to store offsets
	size_t *length = kmalloc(NARG_MAX * sizeof(size_t)); //store length of each string
	size_t offval = 0;	//accumulative offset values 
	userptr_t argbase = (userptr_t)stackptr; //points to the base of each argument
	
	for(unsigned int i = 0; i < argc ; i++){
		char *arg = argv[i]; //arguments
		size_t len = strlen(arg); //length of arguments
		if(len%4){	//make the length a multiple of 4
			len = len - len % 4 + 4;
		}
		offsets[i] = offval;
		offval += len;	//add new length to offset
		length[i] = len;
	}
	argbase -= offval;
	for(unsigned int i = 0; i < argc ; i++){
		char *arg = argv[i];
		copyoutstr(arg, argbase, length[i], NULL); //copy argument to user space
		argbase += length[i];
	}
	argbase -= offval;
	userptr_t uargs = argbase - (sizeof(userptr_t) * (argc + 1));
	userptr_t stack = uargs;
	userptr_t arg;
    for(unsigned int i = 0 ; i < argc ; i++) { // copy the elements
        arg = argbase + offsets[i];
        copyout(&arg, uargs, sizeof(userptr_t));
        uargs += sizeof(userptr_t); // 4
    }
    arg = NULL;
    copyout(&arg, uargs, sizeof(userptr_t)); // copy the NULL pointer.

    kfree(offsets);



    /* Warp to user mode. */
    enter_new_process((int)argc, stack, (vaddr_t)stack, entrypoint);
#else
	/* Warp to user mode. */
	enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
			  stackptr, entrypoint);
#endif
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

