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
#include <limits.h>
#include "opt-A2.h"
#include "synch.h"
#include "array.h"

#if OPT_A2

int errno;

static struct fd* create_fd(int flag, int handle, const char* filename, struct vnode* vn){
	struct fd* file_descriptor;
	file_descriptor = kmalloc(sizeof(struct fd));
	file_descriptor->file_flag = flag;
	file_descriptor->file_handle = handle;
	file_descriptor->filename = (char *)filename;
	KASSERT(vn != NULL);
	file_descriptor->file = vn;

	return file_descriptor;
}

static void add_fd(struct fd* file, int filehandle){            // add new file descriptor to fd_table
        curproc->fd_table[filehandle] = file;
}

int sys_close(int fd){
  struct vnode* tmp = curproc->fd_table[fd]->file;
  if (tmp != NULL) {
    vfs_close(tmp); 
    return 0;      //successfully closed.
  } else{
    errno = EBADF;    // fd is not a valid file handle
  }
  return -1;   // error found
}

int sys_open(const char *filename, int file_flag, mode_t mode){
	if(filename == NULL){ //bad memory reference
		errno = EFAULT;
		return -1;
	}
	if(file_flag & O_APPEND){ //flags contained invalid values
		errno = EINVAL;
		return -2;
	}
	
	struct vnode** new_file; //passed into vfs_lookup function
	int ret; //return value
	char* filenamecast = kmalloc(sizeof((char *) NAME_MAX));
	strcpy(filenamecast, filename); //cast constant filename -> non-constant
	
	ret = vfs_lookup(filenamecast, new_file); //takes care of EISDIR and ENOTDIR hopefully
	if(file_flag & O_CREAT && ret >= 0 && file_flag & O_EXCL){
		errno = EEXIST; //file creation failed because it already exists
		return -3;
	}
	
	//new_file = NULL; //passed into vfs_open function
	if(curproc->open_num < MAX_fd_table){  //fd table is not full, open allowed 
		ret = vfs_open(filenamecast, file_flag, mode , new_file);
		curproc->open_num++;
		if(ret < 0){
			return -1;
		}
	}else{ //fd table is full, open disallowed
		errno = EMFILE; 
		return -4;
	}
	
	//still need to check errors: ENOSPC and EIO********************************************************************	

	int file_handle = 3; //file handle is the index at which the fd is located

	while(curproc->fd_table[file_handle] != NULL) { //find empty slot in fd_table
		file_handle++;
	}

	struct fd* f = create_fd(file_flag, file_handle, filename, *new_file); //add fd to the fd table
  add_fd(f, file_handle);

	return file_handle;  //index of the fd in the fd_fd_table
}


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


#endif
