#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init(void);

/* project2 - System Call */

/* Process identifier. */
typedef int pid_t;

#include <stdbool.h>
void check_address(void *addr);
void halt(void);
void exit(int status);

pid_t fork(const char *thread_name);
int exec(const char *cmd_line);
int wait(pid_t);

bool create(const char *file, unsigned initial_size);
bool remove(const char *file);

int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned length);
int write(int fd, const void *buffer, unsigned length);
void seek(int fd, unsigned position);
int tell(int fd);
void close(int fd);

#endif /* userprog/syscall.h */
