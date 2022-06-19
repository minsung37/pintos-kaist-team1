#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

// * USERPROG 추가
#include "threads/palloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

/* VM */
#include "vm/vm.h"

void syscall_entry(void);
void syscall_handler(struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081			/* Segment selector msr */
#define MSR_LSTAR 0xc0000082		/* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

// void *mmap (void *addr, size_t length, int writable, int fd, off_t offset);
// void munmap (void *addr);

void 
check_address(void *addr) {
	if (addr == NULL || is_kernel_vaddr(addr))
		exit(-1);
#ifdef VM	
	struct page *p = spt_find_page (&thread_current ()->spt, addr);
	// printf("check_address %p, page? %p\n", addr, p);
	if (p == NULL) {
		exit(-1);
	}
#endif
}

#ifdef VM
void 
check_valid_buffer(void *buffer, unsigned length) {
	uint64_t size;
	for (size = 0; size <= length; size += PGSIZE) {
		void *addr = buffer + size;
		if (addr == NULL || is_kernel_vaddr(addr))
			exit(-1);

		struct page *p = spt_find_page (&thread_current ()->spt, addr);
		// printf("ADDR %p writable? %d\n", addr, p->writable);
		if (p == NULL || !p->writable) 
			exit(-1);
	}
}
#endif

void 
syscall_init(void) {

	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
							((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			  FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	lock_init(&filesys_lock);
	
}

void 
syscall_handler(struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	thread_current ()->rsp = f->rsp;
	switch (f->R.rax)
	{
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
		exit(f->R.rdi);
		break;
	case SYS_FORK:
		memcpy(&thread_current()->temp_tf, f, sizeof(struct intr_frame));
		f->R.rax = fork(f->R.rdi);
		break;
	case SYS_CREATE:
		f->R.rax = create(f->R.rdi, f->R.rsi);
		break;
	case SYS_REMOVE:
		f->R.rax = remove(f->R.rdi);
		break;
	case SYS_OPEN:
		f->R.rax = open(f->R.rdi);
		break;
	case SYS_FILESIZE:
		f->R.rax = filesize(f->R.rdi);
		break;
	case SYS_READ:
		f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_WRITE:
		f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_EXEC:
		exec(f->R.rdi);
		break;
	case SYS_WAIT:
		f->R.rax = wait(f->R.rdi);
		break;
	case SYS_SEEK:
		seek(f->R.rdi, f->R.rsi);
		break;
	case SYS_TELL:
		f->R.rax = tell(f->R.rdi);
		break;
	case SYS_CLOSE:
		close(f->R.rdi);
		break;

	case SYS_MMAP:
		f->R.rax = mmap(f->R.rdi, f->R.rsi, f->R.rdx, f->R.r10, f->R.r8);
		break;
	case SYS_MUNMAP:
		munmap(f->R.rdi);
		break;
	default:
		exit(-1);
		break;
	}	

	// case SYS_CHDIR:                  /* Change the current directory. */
	// 	break;
	// case SYS_MKDIR:                  /* Create a directory. */
	// 	break;
	// case SYS_READDIR:                /* Reads a directory entry. */
	// 	break;
	// case SYS_ISDIR:                  /* Tests if a fd represents a directory. */
	// 	break;
	// case SYS_INUMBER:                /* Returns the inode number for a fd. */
	// 	break;
	// case SYS_SYMLINK:                /* Returns the inode number for a fd. */
	// 	break;

}

void 
halt (void) {
	power_off();
}

void 
exit (int status) {
	struct thread *cur = thread_current();
	cur->exit_code = status;
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

int 
fork (const char *thread_name) {
	check_address(thread_name);
	lock_acquire(&filesys_lock);
	int ret = process_fork(thread_name, &thread_current()->temp_tf);
	lock_release(&filesys_lock);
	return ret;
}

int 
exec (const char *file) {
	check_address(file);

	char *fn_copy[64];

	memcpy(fn_copy, file, strlen(file) + 1);
	if (process_exec(fn_copy) == -1) {
		exit(-1);
	}
}

int 
wait (tid_t pid) {
	int ret = process_wait(pid);
	return ret;
}

bool 
create (const char *file, unsigned initial_size) {
	check_address(file);
	lock_acquire(&filesys_lock);
	bool success = filesys_create(file, initial_size);
	lock_release(&filesys_lock);

	return success;
}

bool 
remove (const char *file) {
	check_address(file);
	lock_acquire(&filesys_lock);
	bool success = filesys_remove(file);
	lock_release(&filesys_lock);
	return success;
}

int 
open (const char *file) {
	check_address(file);

	struct thread *current = thread_current();
	int i;
	if (current->next_fd < 128) {
		lock_acquire(&filesys_lock);
		struct file *new_file = filesys_open(file);
		lock_release(&filesys_lock);

		if (new_file != NULL) {
			current->fdt[current->next_fd] = new_file;

			return current->next_fd++;
		}
	}

	return -1;
}

int 
filesize (int fd) {
	struct file *file = thread_current()->fdt[fd];
	if (file == NULL)
		return -1;
	return file_length(file);
}

int 
read (int fd, void *buffer, unsigned length) {
	// check_address(buffer);
	check_valid_buffer(buffer, length);

	struct thread *curr = thread_current();
	int bytes_read;

	if (fd == 0) {
		lock_acquire(&filesys_lock);
		bytes_read = input_getc();
		lock_release(&filesys_lock);

		return bytes_read;
	}
	else if (fd >= 2) {
		lock_acquire(&filesys_lock);
		bytes_read = file_read(curr->fdt[fd], buffer, length);
		lock_release(&filesys_lock);

		return bytes_read;
	}
	return -1;
}

int
write (int fd, const void *buffer, unsigned length) {
	check_address(buffer);
	// check_valid_buffer(buffer, length);

	if (fd == 1) {
		lock_acquire(&filesys_lock);
		putbuf(buffer, length);
		lock_release(&filesys_lock);

		return length;
	}

	if (fd >= 2) {
		struct file *file = thread_current()->fdt[fd];

		if (file != NULL) {
			lock_acquire(&filesys_lock);
			int bytes_written = file_write(file, buffer, length);
			lock_release(&filesys_lock);

			return bytes_written;
		}

		// lock_release(&filesys_lock);
	}

	return -1;
}

void 
seek (int fd, unsigned position) {
	struct file *file = thread_current ()->fdt[fd];
	if (file != NULL)
		file_seek (file, position);
}

unsigned 
tell (int fd) {
	struct file *file = thread_current ()->fdt[fd];
	if (file != NULL)
		return file_tell(file);
}

void 
close (int fd) {
	struct file *file = thread_current ()->fdt[fd];
	if (file != NULL) {
		lock_acquire (&filesys_lock);
		file_close (file);
		lock_release (&filesys_lock);
		thread_current ()->fdt[fd] = NULL;
	}
}


void *
mmap (void *addr, size_t length, int writable, int fd, off_t offset) {

	if (addr ==  NULL || is_kernel_vaddr(addr) || pg_ofs(addr))
		return NULL;

	if ((int64_t)length <= 0 || (int64_t)length < (int64_t)offset)
		return NULL;

	if (fd < 2)
		return NULL;

	/* Use the file_reopen function to obtain a separate and independent reference 
	 * to the file for each of its mappings. */

	struct file *ofile = file_reopen (thread_current ()->fdt[fd]);
	lock_acquire (&filesys_lock);
	void *success = do_mmap (addr, length, writable, ofile, offset);
	lock_release (&filesys_lock);
	return success;
}

void 
munmap (void *addr) {
	check_address (addr);
	do_munmap (addr);
	return;
}