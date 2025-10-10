# Syscall

The implementation of system calls involves two key aspects:
 (1) triggering system calls within the **RISC-V environment**, and
 (2) handling those system calls inside the **simulator** itself.

Since the provided **RISC-V cross toolchain** targets a **bare-metal environment**, it does not support **glibc** and thus lacks C function definitions for system calls. Therefore, we need to implement them ourselves. In the **`test`** section, I implemented **`syscall.h`** to encapsulate system calls, providing good extensibility for future additions.

```c
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
```

In the RISC-V environment, system calls are invoked using the **`ecall`** instruction, which differs from the **`trap`** mechanism used in x86. This means that the simulator’s **instruction decoding logic** must handle `ecall` explicitly. Within the `ecall` handler, the simulator interprets the system call according to the **RISC-V ABI specification** and the **system call number**, simulating the corresponding effect on the host machine. (in `sim/src/syscall.c`)

```c
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
```

Some system calls involve memory operations, which would require support for **paging**, an **MMU**, and related mechanisms within the simulator. Since that would move the focus toward operating system–level topics—beyond the scope of this course—we only implemented **`exit`** and **`write`**.
 Other system calls can be easily added by extending the existing framework.