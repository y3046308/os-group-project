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
#include "synch.h"
#include "array.h"

#if OPT_A2

#define MAX_fd_table 64	// only allow 64 files in fd_table (size of TLB)

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

static void add_fd(struct fd* file){		// add new file descriptor to fd_table
	if(curproc->fd_table == NULL) {
		curproc->fd_table = kmalloc(sizeof(struct fd*)*MAX_fd_table);
	}
	int i = 0;
	while(curproc->fd_table[i] != NULL){
		i++;
	}	
	curproc->fd_table[i] = file;
}

int sys_close(int fd){
  struct vnode* tmp = curproc->fd_table[fd]->file;
  if (tmp != NULL){
    vfs_close(tmp); 
    return 0;      //successfully closed.
  }
  else{
    errno = EBADF;    // fd is not a valid file handle
  }
  return -1;   // error found
}

int sys_open(const char *filename, int file_flag, mode_t mode){
	if(filename != NULL){ //bad memory reference
		errno = EFAULT;
		return -1;
	}
	if(file_flag & O_APPEND){ //flags contained invalid values
		errno = EINVAL;
		return -1;
	}
	
	struct vnode** new_file = NULL; //passed into vfs_lookup function
	int ret; //return value
	char* filenamecast = strcpy(filenamecast, filename); //cast constant filename -> non-constant
	
	ret = vfs_lookup(filenamecast, new_file); //takes care of EISDIR and ENOTDIR hopefully
	if(file_flag & O_CREAT && ret >= 0 && file_flag & O_EXCL){
		errno = EEXIST; //file creation failed because it already exists
		return -1;
	}
	
	new_file = NULL; //passed into vfs_open function
	if(curproc->open_num < MAX_fd_table){  //fd table is not full, open allowed 
		ret = vfs_open(filenamecast, file_flag, mode , new_file);
		curproc->open_num++;
		if(ret < 0){
			return -1;
		}		
	}else{ //fd table is full, open disallowed
		errno = EMFILE; 
		return -1;
	}
	
	//still need to check errors: ENOSPC and EIO********************************************************************	

	int file_handle = 0; //file handle is the index at which the fd is located
	while(curproc->fd_table[file_handle] != NULL) { //find empty slot in fd_table
		file_handle++;
	}

	struct fd* f = create_fd(file_flag, file_handle, filename, *new_file); //add fd to the fd table
	add_fd(f);

	return 0;  //index of the fd in the fd_fd_table
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


/*struct fd* find_fd_name(const char* name){    // find f.d with given file name
  struct fd* copy = fd_table;
  while (copy != NULL){
    if (copy->filename == name){
      return copy;
    }
    copy = copy + 1;
  }
  return NULL;           // fd not found
}*/

/*int sys_open(const char* filename, int flags) {
    int open = -1;
    if (filename  == NULL) errno = EFAULT; // filename was an invalid pointer.

    else if (flags != O_RDONLY || flags != O_WRONLY || flags != O_RDWR || flags != O_CREAT || 
            flags != O_EXCL || flags != O_TRUNC || flags != O_APPEND) errno = EINVAL; //        flags contained invalid values.
        else{    
        struct fd* tmp = find_fd_name(filename);             // find a file from file description fd_table
        if (tmp == NULL){
            if(flags != O_CREAT) errno = ENOENT;  // The named file does not exist, and O_CREAT was not specified.
            else{                   // if flag is O_CREAT, create a file
                int newFlag;
                for (newFlag = 3; newFlag < MAX_fd_table ; newFlag++){
                    if ((fd_table + newFlag) == NULL){
                        break;      // if fd_table at [newFlag] does not exit, it will be used to store new f.d
                    }
                } 
                add_fd(create_fd(newFlag, (char*)filename, NULL));  // create a new fd and add it into fd_table
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
}*/

// find_fd function is commented out because we have no use for it so far. Uncomment and use function if necessary

/*static struct fd* find_fd(char* filename){    // find f.d with given fh
  struct fd** copy = curproc->fd_table;
  for(int i =0; i < MAX_fd_table; i++){
    if (strcmp((copy[i])->filename, filename) == 0){
      return copy[i];
    }
  }
  return NULL;           // fd not found
}*/

#endif
