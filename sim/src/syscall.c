#include <cpu.h>
#include "syscall.h"
#include <decode.h>
#include <stdio.h>
#include <memory.h>

extern CPU_state cpu;

void handle_syscall(Decode *s) {
    uint64_t syscall_no = cpu.reg[17]; // a7 register
    switch (syscall_no) {
        case SYSCALL_EXIT:
            printf("Syscall: exit with code %lu\n", cpu.reg[10]); // a0 register
            halt_trap(s->pc, cpu.reg[10]);
            break;
        
        case SYSCALL_WRITE:
            {
                uint64_t fd = cpu.reg[10]; // a0
                uint64_t buf = cpu.reg[11]; // a1
                uint64_t count = cpu.reg[12]; // a2
                if (fd == 1) { // stdout
                    for (uint64_t i = 0; i < count; i++) {
                        // printf("Syscall: write byte %02x from addr %016lx\n", mem_read(buf + i, 1), buf + i);
                        uint8_t byte_to_write = mem_read(buf + i, 1);
                        putchar(byte_to_write);
                    }
                    cpu.reg[10] = count; // return value in a0
                } else {
                    printf("Syscall: write to unsupported fd %lu\n", fd);
                    cpu.reg[10] = -1; // error
                }
            }
            break;
        
        default:
            printf("Unknown syscall: %lu\n", syscall_no);
            halt_trap(s->pc, -1);
            break;
    }
}