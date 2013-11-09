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
#include <kern/unistd.h>

#if OPT_A2
#define DUMBVM_STACKPAGES 12
int errno;

static struct fd* create_fd(int flag, const char* filename, struct vnode* vn){
	struct fd* file_descriptor;
	file_descriptor = kmalloc(sizeof(struct fd));
	file_descriptor->file_flag = flag;
	file_descriptor->filename = (char *)filename;
	file_descriptor->lock = lock_create(filename);
	file_descriptor->offset = 0;
	file_descriptor->buflen = 0;
	KASSERT(vn != NULL);
	file_descriptor->file = vn;

	return file_descriptor;
}

static void add_fd(struct fd* file, int file_handle){		// add new file descriptor to fd_table
	curproc->fd_table[file_handle] = file;
}

static bool valid_address_check(struct addrspace *as, vaddr_t addr){
	if (addr < USERSTACK){
		if ((as->as_vbase1 <= addr && addr < (as->as_vbase1 + as->as_npages1 * PAGE_SIZE)) ||
		    (as->as_vbase2 <= addr && addr < (as->as_vbase2 + as->as_npages2 * PAGE_SIZE)) ||
		    (USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE <= addr)){
			return true;
		}
	}
	return false;
}
int sys_close(int fd){
        int retval = -1;
        if (fd < 0 || fd >= MAX_fd_table){
                errno = EBADF;
        }
	else if (curproc->fd_table[fd] != NULL) {
		vfs_close(curproc->fd_table[fd]->file);
		kfree(curproc->fd_table[fd]);
		curproc->fd_table[fd] = NULL;
		retval = 0;
	}
	else{
		errno = EIO;
	}
	return retval; 
}

int sys_open(const char *filename, int file_flag, mode_t mode){
	bool create = false, full = true;
        if(filename == NULL || !(valid_address_check(curproc->p_addrspace, (vaddr_t)filename))){ //bad memory reference
                errno = EFAULT;
                return -1;
        }
		if(file_flag > 94 || file_flag % 4 == 3 || file_flag & O_APPEND){
			errno = EINVAL;
			return -1;
		}
		/*
       if(file_flag & O_APPEND){ //flags contained invalid values
                errno = EINVAL;
                return -1;
        }*/
	struct vnode* new_file;
	int ret;
	if (curproc->open_num < MAX_fd_table){	// fd table is avilable
		full = false;
		ret = vfs_open((char *)filename, file_flag, mode , &new_file);	// open file when table has free space
		curproc->open_num++;
		if (ret == 0){
			if ((file_flag & O_CREAT) && (file_flag & O_EXCL)){
				errno = EEXIST;
				return -1;
			}
		}
		else{
			create = true;
			if (file_flag & ~O_CREAT){
				errno = ENOENT;
				return -1;
			}
		}
	}
	if (full){	// if table is full
		if (create) errno = ENOSPC;
		else errno = EMFILE;
		return -1;
	}
        
        int file_handle = 3; //file handle is the index at which the fd is located
        
        while(curproc->fd_table[file_handle] != NULL) { //find empty slot in fd_table
                file_handle++;
        }
	struct fd* f = create_fd(file_flag, filename, new_file);
        add_fd(f,file_handle);

        return file_handle;  //index of the fd in the fd_fd_table
}


int sys_read(int fd, void *buf, size_t buflen) {
    struct fd* tmp;
    if (fd < 0 || fd >= MAX_fd_table){       // if fd < 0 || fd > MAX_fd_table or 
        errno = EBADF;
        return -1;
    }
	else if (fd == STDOUT_FILENO || fd == STDERR_FILENO){      // fd == STDOUT_FILENO || STDERR_FILENO
            errno = EIO;
            return -1;
        }
        else if (fd >= 3 && fd < MAX_fd_table){
            tmp = curproc->fd_table[fd];
            if (tmp == NULL || (tmp->file_flag & O_WRONLY)){        // or if file is not readable
                errno = EBADF;  // error
                return -1;
            }
        }
        if (!buf || buf == NULL || !valid_address_check(curproc->p_addrspace, (vaddr_t)buf)){      // else if invalid buffer
            errno = EFAULT;
            return -1;
        }

        struct uio u;
        struct iovec iov;
        struct addrspace *as ;
        as = as_create();

        iov.iov_ubase = buf;
        iov.iov_len = buflen;

        u.uio_iov = &iov;
        u.uio_iovcnt = 1;
        u.uio_offset = tmp->offset;
        u.uio_resid = buflen;
        u.uio_segflg = UIO_USERSPACE;
        u.uio_rw = UIO_READ;
        u.uio_space = curproc_getas();
	
	if (fd == STDIN_FILENO){
		struct vnode *vn;
		char *console = NULL; // console string ("con:")
            console = kstrdup("con:"); // set to console
            vfs_open(console,O_RDONLY,0,&vn); // open the console vnode
            kfree(console); // free the console
            int result = VOP_READ(vn,&u);
            if(result < 0){
                    errno = EIO; //A hardware I/O error occurred writing the data
                    return -1;
            }
	} else{
	        int retval = VOP_READ(tmp->file, &u);
		if (retval < 0){
			errno = EIO;
			return -1;
		}
	}
	return buflen - u.uio_resid ;
}

int sys_write(int fd, const void *buf, size_t nbytes) {
	if (!buf || buf == NULL || !valid_address_check(curproc->p_addrspace, (vaddr_t)buf)){      // invalid buffer OR buffer out of range
	        errno = EFAULT;
	        return -1;
	}
	if(fd < 0 || fd >= MAX_fd_table){
	        errno = EBADF;
	        return -1;
	} else if (fd == STDIN_FILENO){	// fd == STDIN_FILENO
		errno = EIO;
		return -1;
	} else if (fd >= 3 && fd < MAX_fd_table){		// have valid fd
		struct fd* tmp = curproc->fd_table[fd];
//                if (tmp == NULL || (tmp->file_flag & O_RDONLY)){	// but have no file at fd OR file at fd is RDONLY
		if (tmp == NULL){ 
			errno = EBADF;
			return -1;
		}
		switch(tmp->file_flag & O_ACCMODE){
			case O_WRONLY: break;
			case O_RDWR:   break;
			default:
				errno = EBADF;
				return -1;
		}
	}

	struct vnode *vn; // creating vnode (temp)
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

	if(fd == STDOUT_FILENO || fd == STDERR_FILENO){
		char *console = NULL; // console string ("con:")
		console = kstrdup("con:"); // set to console
		vfs_open(console,O_WRONLY,0,&vn); // open the console vnode
		kfree(console); // free the console
		int result = VOP_WRITE(vn,&u);
		if(result < 0){
			errno = EIO; //A hardware I/O error occurred writing the data
			return -1;
		}
	}
	else{
		u.uio_offset = curproc->fd_table[fd]->offset;
		VOP_WRITE(curproc->fd_table[fd]->file, &u);
		if(u.uio_resid){
			errno = ENOSPC; //There is no free space remaining on the filesystem containing the file
			return -1;
		}
		curproc->fd_table[fd]->offset += nbytes - u.uio_resid;
		curproc->fd_table[fd]->buflen += nbytes - u.uio_resid; //update buflength of fd
	}
	
	return nbytes - u.uio_resid;
}
#endif
