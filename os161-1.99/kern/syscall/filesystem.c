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
#include "opt-A2.h"
#if OPT_A2

struct fd{       // file descriptor indicating each individual file
	int file_flag;
	char *filename;
	struct vnode* file;
};

struct fd* table;  // collection of file descriptor

struct fd* create_fd(int flag, const char* filename, struct vnode* vn){
	struct fd* file_descriptor;
	file_descriptor = kmalloc(sizeof(struct fd));
	file_descriptor->file_flag = flag;
	file_descriptor->filename = (char *)filename;
	file_descriptor->file = vn;
	return file_descriptor;
}

struct vnode* find_flag(int fd){    // find f.d with given fd
  struct fd* copy = table;
  while (copy != NULL){
    if (copy->file_flag == fd){
      return copy->file;
    }
    copy = copy + 1;
  }
  return NULL;           // fd not found
}

int sys_open(const char* filename, int flags) {
	(void)filename;
	(void)flags;
  	struct fd* tmp = create_fd(flags, (char*)filename, NULL);
  	(void)tmp;
  	//int vfs_open(char *path, int openflags, mode_t mode, struct vnode **ret);
  	//int cur = vfs_open(
	return 1;
}

int sys_close(int fd){
  struct vnode* tmp = find_flag(fd);
  if (tmp != NULL){
    vfs_close(tmp); 
    return 0;      //successfully closed.
  }
  else{
    return EBADF;    // fd is not a valid file handle
  }
  return -1;   // error found
}

/*int sys_open(char *filename, int file_flag){
	KASSERT(filename != NULL);
	KASSERT(file_flag == O_RDONLY || file_flag == O_RDWR || file_flag == O_WRONLY);
	struct fd f;
	f.file_flag = file_flag;
	f.filename = filename;
	vfs_open(filename, file_flag
	
	
}*/

int sys_read(int fd, void *buf, size_t buflen) {
	(void)fd;
	(void)buf;
	(void)buflen;


	return 1;
}

int sys_write(int fd, const void *buf, size_t nbytes) {

	(void)fd;

	struct vnode *vn; // creating vnode (temp)
	char *console = NULL; // console string ("con:")
	console = kstrdup("con:"); // set to console
	int result = vfs_open(console,O_WRONLY,0,&vn); // open the console vnode
	kfree(console); // free the console

	struct uio u;
	struct iovec iov;
	struct addrspace *as;

	as = as_create();

	iov.iov_ubase = (void *)buf;
	iov.iov_len = nbytes;

	u.uio_iov = &iov;
	u.uio_resid = nbytes;
	u.uio_rw = UIO_WRITE;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_space = curproc_getas();

	VOP_WRITE(vn,&u);

	// SIMPLE IMPLEMENTATION
	// kprintf("asd");

	return result;
}

pid_t sys_getpid(){
    return curthread->t_proc->t_pid;
}

void sys__exit(int exitcode) {
	threadarray_remove(&curproc->p_threads, 0);
	proc_destroy(curthread->t_proc);
	// code below is removed. Since the process is destroyed, there is no "curthread" now.
	// We cannot access to it.
	// curthread->t_proc = NULL;
	(void)exitcode;
}
#endif
