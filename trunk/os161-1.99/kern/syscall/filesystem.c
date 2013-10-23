
#include <types.h>
#include <kern/errno.h>
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

/*
int sys_open(const char* filename, int flags) {
	(void)filename;
	(void)flags;
	return 1;
}

int sys_close(int fd){
  vfs_close(...);  
}*/

int sys_read(int fd, void *buf, size_t buflen) {
	(void)fd;
	(void)buf;
	(void)buflen;


	return 1;
}
int sys_write(int fd, const void *buf, size_t nbytes) {

	(void)fd;

	struct vnode *vn;
	char *console = NULL;
	console = kstrdup("con:");
	int result = vfs_open(console,O_WRONLY,0,&vn);
	kfree(console);

	struct uio u;
	struct iovec iov;
	struct addrspace *as;

	as = as_create();

	iov.iov_ubase = (void *)buf;
	iov.iov_len = nbytes;

//	u = uio_kinit(&iov, &u, (void *)buf, nbytes, 0, UIO_WRITE );
// uio_iovec describes the location and length of the buffer that is being transferred to or from. This buffer can be in the application part of the kernel address space or in the kernel's part.
// uio_resid describes the amount of data to transfer
// uio_rw identifies whether the transfer is from the file/device or to the file/device,
// uio_segflag indicates whether the buffer is in the user (application) part of the address space or the kernel's part of the address space.
// uio_space points to the addrspace object of the process that is doing the VOP_WRITE or VOP_READ. This is only used if the buffer is is in the user part of the address space - otherwise, it should be set to NULL.
	u.uio_iov = &iov;
	u.uio_resid = nbytes;
	u.uio_rw = UIO_WRITE;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_space = curproc_getas();

	VOP_WRITE(vn,&u);

	// SIMPLE IMPLEMENTATION
	//kprintf("asd");

	return result;
}
