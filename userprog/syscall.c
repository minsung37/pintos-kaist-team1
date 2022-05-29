#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "filesys/filesys.h"
#include <console.h>

typedef int pid_t;

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
void halt(void);
void exit(int status);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
pid_t fork (const char *thread_name);
int exec (const char *file);
int wait (pid_t);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);


/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	check_address(f->rsp);
	uint64_t sys_num = f->R.rax;
	uint64_t arg1 = f->R.rdi;
	uint64_t arg2 = f->R.rsi;
	uint64_t arg3 = f->R.rdx;
	uint64_t arg4 = f->R.r10;
	uint64_t arg5 = f->R.r8;
	uint64_t arg6 = f->R.r9;

	// printf ("system call!\n");
	switch(sys_num){
		case 0:
			halt ();
			break;
		case 1:
			exit (arg1);
			break;
		// case 2:
		// 	fork (arg1);
		// 	break;
		// case 3:
		// 	exec (arg1);
		// 	break;
		// case 4:
		// 	wait (arg1);
		// 	break;
		case 5:
			create (arg1, arg2);
			break;
		case 6:
			remove (arg1);
			break;
		// case 7:
		// 	open (arg1);
		// 	break;
		// case 8:
		// 	file_size (arg1);
		// 	break;
		// case 9:
		// 	read (arg1, arg2, arg3);
		// 	break;
		// case 10:
		// 	write (arg1, arg2, arg3);
		// 	break;
		// case 11:
		// 	seek (arg1, arg2);
		// 	break;
		// case 12:
		// 	tell (arg1);
		// 	break;
		// case 13:
		// 	close (arg1);
		// 	break;
	}
	thread_exit ();
}


void halt(void) {
	power_off ();
}

void exit(int status){
	char *name_ptr = thread_current ()->name;
	status = 0;
	printf("%s: exit(%d)\n", name_ptr, status);
	thread_exit ();
}

bool create (const char *file, unsigned initial_size){
	bool success = filesys_create (file, (off_t) initial_size);
	if (!success)
		return false;
	return true; 
}

bool remove (const char *file){
	bool success = filesys_remove (file);
	if (!success)
		return false;
	return true; 
}

int write (int fd, const void *buffer, unsigned length) {
	if (fd == 1){
		putbuf (buffer, length); 
		return length;
	}
	return -1;
}