#ifndef SYSCALL_H
#define SYSCALL_H

#include <isa_decode.h>

#define SYSCALL_EXIT 93
#define SYSCALL_WRITE 64

void handle_syscall(Decode *s);

#endif