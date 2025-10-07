#include <cpu.h>
#include <common.h>

static void cmd_help() {
    printf("Available commands:\n");
    printf("  help - Display this help message\n");
    printf("  c    - Continue execution\n");
    printf("  q    - Quit the simulator\n");
    printf("  si   - Step execution by specified number of instructions\n");
    printf("  info - Display information about registers\n");
    printf("  x    - Examine memory\n");
}

static void cmd_continue() {
    cpu_exec();
}

static void cmd_quit() {
    exit(0);
}

static void cmd_step(int stemps) {
    for (int i = 0; i < stemps; ++i) {
        exec_once();
    }
}

static void cmd_info(char arg) {
    if (arg == 'r') {
        for (int i = 0; i < 32; ++i) {
            printf("x%d: 0x%016lx\n", i, cpu.reg[i]);
        }
        printf("pc: 0x%016lx\n", cpu.pc);
    } else {
        printf("Unknown info command '%c'\n", arg);
    }
}

static void cmd_examine(int len, uint64_t addr) {
    check_mem(addr)
    for (int i = 0; i < len; ++i) {
        uint32_t data = mem_read(addr + i * 4, 4);
        printf("0x%016lx: 0x%08x\n", addr + i * 4, data);
    }
error:
    printf("Invalid memory address 0x%lx\n", addr);
    return;
}