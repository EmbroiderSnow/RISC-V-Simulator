#include <cpu.h>
#include <trap.h>
#include <decode.h>

extern CPU_state cpu;

void handle_syscall(Decode *s) {
    uint64_t syscall_no = cpu.reg[17]; // a7 register
    switch (syscall_no) {
        case SYSCALL_EXIT:
            Log("Syscall: exit with code %lu", cpu.reg[10]); // a0 register
            halt_trap(s->pc, cpu.reg[10]);
            break;
        
        case SYSCALL_WRITE:
            {
                uint64_t fd = cpu.reg[10]; // a0
                uint64_t buf = cpu.reg[11]; // a1
                uint64_t count = cpu.reg[12]; // a2
                if (fd == 1) { // stdout
                    for (uint64_t i = 0; i < count; i++) {
                        putchar(*(uint8_t *)(buf + i));
                    }
                    cpu.reg[10] = count; // return value in a0
                } else {
                    Log("Syscall: write to unsupported fd %lu", fd);
                    cpu.reg[10] = -1; // error
                }
            }
            break;
        
        default:
            Log("Unknown syscall: %lu", syscall_no);
            halt_trap(s->pc, -1);
            break;
    }
}