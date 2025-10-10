#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYSCALL_EXIT 93
#define SYSCALL_WRITE 64

static inline int64_t syscall(int64_t no, int64_t a0, int64_t a1, int64_t a2) {
    register int64_t a7 asm("a7") = no;
    register int64_t a0_ asm("a0") = a0;
    register int64_t a1_ asm("a1") = a1;
    register int64_t a2_ asm("a2") = a2;
    asm volatile("ecall"
                 : "+r"(a0_)
                 : "r"(a7), "r"(a1_), "r"(a2_)
                 : "memory");
    return a0_;
}

static inline void exit(int64_t code) {
    syscall(SYSCALL_EXIT, code, 0, 0);
}

static inline int64_t write(int64_t fd, const void *buf, int64_t count) {
    return syscall(SYSCALL_WRITE, fd, (int64_t)buf, count);
}


#endif