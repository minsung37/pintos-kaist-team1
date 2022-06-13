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

void 
check_address(void *addr) {
	if (addr == NULL || is_kernel_vaddr(addr))
		exit(-1);
#ifdef VM	
	struct page *p = spt_find_page (&thread_current ()->spt, addr);
	if (p == NULL) {
		exit(-1);
	}
#endif
}

#ifdef VM
void 
check_valid_buffer(void *buffer, unsigned length, bool writable) {

	int va;
	va = pg_round_down(buffer);
	// >= ????
	while (length > 0) {
		struct page *p = spt_find_page (&thread_current ()->spt, (void *)(va));
		if (p == NULL) {
			exit(-1);
		}
		if (p->writable != writable) {
			exit(-1);
		}
		length -= PGSIZE;
		va += PGSIZE;
	}
}
#endif

void 
syscall_init(void) {

	lock_init(&filesys_lock);

	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
							((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			  FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

void 
syscall_handler(struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	thread_current ()->rsp = f->rsp;
	// printf("syscall rsp: %p\n", thread_current ()->rsp);
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
	default:
		exit(-1);
		break;
	}
}

void 
halt(void) {
	power_off();
}

void 
exit(int status) {
	struct thread *cur = thread_current();
	cur->exit_code = status;
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

int 
fork(const char *thread_name) {
	check_address(thread_name);
	int ret = process_fork(thread_name, &thread_current()->temp_tf);
	return ret;
}

int 
exec(const char *file) {
	check_address(file);

	char *fn_copy[64];

	memcpy(fn_copy, file, strlen(file) + 1);
	if (process_exec(fn_copy) == -1) {
		exit(-1);
	}
}

int 
wait(tid_t pid) {
	int ret = process_wait(pid);
	return ret;
}

bool 
create(const char *file, unsigned initial_size) {
	check_address(file);
	return filesys_create(file, initial_size);
}

bool 
remove(const char *file) {
	check_address(file);
	return filesys_remove(file);
}

int 
open(const char *file) {
	check_address(file);
	struct thread *current = thread_current();
	int i;

	if (current->next_fd < 128) {
		struct file *new_file = filesys_open(file);

		if (new_file != NULL) {
			current->fdt[current->next_fd] = new_file;
			return current->next_fd++;
		}
	}
	return -1;
}

int 
filesize(int fd) {
	struct file *file = thread_current()->fdt[fd];
	if (file == NULL)
		return -1;
	return file_length(file);
}

int 
read(int fd, void *buffer, unsigned length) {
	check_address(buffer);
	// check_valid_buffer(buffer, length, true || false);

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
write(int fd, const void *buffer, unsigned length) {
	check_address(buffer);
	// check_valid_buffer(buffer, length, true || false);

	if (fd == 1) {
		lock_acquire(&filesys_lock);
		putbuf(buffer, length);
		lock_release(&filesys_lock);

		return length;
	}

	if (fd >= 2) {
		lock_acquire(&filesys_lock);
		struct file *file = thread_current()->fdt[fd];

		if (file != NULL) {
			int bytes_written = file_write(file, buffer, length);
			lock_release(&filesys_lock);

			return bytes_written;
		}

		lock_release(&filesys_lock);
	}

	return -1;
}

void 
seek(int fd, unsigned position) {
	struct file *file = thread_current()->fdt[fd];
	if (file != NULL)
		file_seek(file, position);
}

unsigned 
tell(int fd) {
	struct file *file = thread_current()->fdt[fd];
	if (file != NULL)
		return file_tell(file);
}

void 
close(int fd) {
	struct file *file = thread_current()->fdt[fd];
	if (file != NULL) {
		lock_acquire(&filesys_lock);
		thread_current()->fdt[fd] = NULL;
		file_close(thread_current()->fdt[fd]);
		lock_release(&filesys_lock);
	}
}
