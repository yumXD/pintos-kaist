#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/synch.h"

struct lock filesys_lock;

void syscall_entry(void);
void syscall_handler(struct intr_frame *);
void check_address(void *uaddr);
void exit(int status);

void halt(void);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file_name);
int filesize(int fd);
void seek(int fd, unsigned position);
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

void syscall_init(void)
{
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

/* The main system call interface */
void syscall_handler(struct intr_frame *f UNUSED)
{
	// TODO: Your implementation goes here.
	int syscall_number = f->R.rax; // 원하는 기능에 해당하는 시스템 콜 번호
	switch (syscall_number)
	{
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
		exit(f->R.rdi);
		break;
	case SYS_FORK:
		/* code */
		break;
	case SYS_EXEC:
		/* code */
		break;
	case SYS_WAIT:
		/* code */
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
		/* code */
		break;
	case SYS_WRITE:
		/* code */
		break;
	case SYS_SEEK:
		seek(f->R.rdi, f->R.rsi);
		break;
	case SYS_TELL:
		/* code */
		break;
	case SYS_CLOSE:
		/* code */
		break;
	default:
		exit(-1);
		break;
	}
	// printf("system call!\n");
	// thread_exit();
}

void check_address(void *uaddr)
{
	struct thread *cur = thread_current();
	if (uaddr == NULL || is_kernel_vaddr(uaddr) || pml4_get_page(cur->pml4, uaddr) == NULL)
	{
		exit(-1);
	}
}

void exit(int status)
{
	struct thread *cur = thread_current();
	cur->exit_status = status;
	printf("%s: exit(%d)\n", thread_name(), status);
	thread_exit();
}

void halt(void)
{
	power_off();
}

bool create(const char *file, unsigned initial_size)
{
	lock_acquire(&filesys_lock);
	check_address(file);
	bool success = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return success;
}

bool remove(const char *file)
{
	check_address(file);
	return filesys_remove(file);
}

int open(const char *file_name)
{
	check_address(file_name);
	lock_acquire(&filesys_lock);
	struct file *file = filesys_open(file_name);
	if (file == NULL)
	{
		lock_release(&filesys_lock);
		return -1;
	}
	int fd = process_add_file(file);
	if (fd == -1)
		file_close(file);
	lock_release(&filesys_lock);
	return fd;
}

int filesize(int fd)
{
	struct file *file = process_get_file(fd);
	if (file == NULL)
		return -1;
	return file_length(file);
}

void seek(int fd, unsigned position)
{
	struct file *file = process_get_file(fd);
	if (file == NULL)
		return;
	file_seek(file, position);
}