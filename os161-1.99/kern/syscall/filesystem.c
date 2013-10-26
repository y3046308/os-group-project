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
#include "opt-A2.h"
#if OPT_A2
#define MAX_TABLE 10	// only allow 10 files in table

int errno;

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
	if (vn != NULL){
		file_descriptor->file = vn;
	}
	else{
		file_descriptor->file = kmalloc(sizeof(struct vnode));
	}
	return file_descriptor;
}

void add_fd(struct fd* file){		// add new file descriptor to table
  struct fd* copy = table;
  while (copy != NULL){
    copy = copy + 1;
  }
  copy = kmalloc(sizeof(struct fd));
  copy = file;
}

struct fd* find_fd_flag(int fd){    // find f.d with given fd
  struct fd* copy = table;
  while (copy != NULL){
    if (copy->file_flag == fd){
      return copy;
    }
    copy = copy + 1;
  }
  return NULL;           // fd not found
}

struct fd* find_fd_name(const char* name){    // find f.d with given file name
  struct fd* copy = table;
  while (copy != NULL){
    if (copy->filename == name){
      return copy;
    }
    copy = copy + 1;
  }
  return NULL;           // fd not found
}

int sys_open(const char* filename, int flags) {
	int open = -1;
	if (filename  == NULL) errno = EFAULT; // filename was an invalid pointer.

	else if (flags != O_RDONLY || flags != O_WRONLY || flags != O_RDWR || flags != O_CREAT || 
            flags != O_EXCL || flags != O_TRUNC || flags != O_APPEND) errno = EINVAL; //		flags contained invalid values.
        else{    
		struct fd* tmp = find_fd_name(filename);             // find a file from file description table
		if (tmp == NULL){
			if(flags != O_CREAT) errno = ENOENT;  // The named file does not exist, and O_CREAT was not specified.
			else{					// if flag is O_CREAT, create a file
				int newFlag;
				for (newFlag = 3; newFlag < MAX_TABLE ; newFlag++){
					if ((table + newFlag) == NULL){
						break;		// if table at [newFlag] does not exit, it will be used to store new f.d
					}
				} 
				add_fd(create_fd(newFlag, (char*)filename, NULL));	// create a new fd and add it into table
				open = newFlag;
			}
		}
		else{
			if(flags == O_EXCL) errno = EEXIST; // The named file exists, and O_EXCL was specified.
			else{
		  		open = vfs_open((char*)filename, flags, 0, &tmp->file);        // actually open a file
			}
		}
	}
	return open;
}

int sys_close(int fd){
  struct vnode* tmp = find_fd_flag(fd)->file;
  if (tmp != NULL){
    vfs_close(tmp); 
    return 0;      //successfully closed.
  }
  else{
    errno = EBADF;    // fd is not a valid file handle
  }
  return -1;   // error found
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
