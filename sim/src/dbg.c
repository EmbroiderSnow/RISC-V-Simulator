#include <cpu.h>
#include <memory.h>
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

void debug_loop() {
    char line[256];
    while (1) {
        printf("(simdb) ");
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        char *cmd = strtok(line, " \n");
        if (!cmd) continue;

        if (strcmp(cmd, "help") == 0) {
            cmd_help();
        } else if (strcmp(cmd, "c") == 0) {
            cmd_continue();
        } else if (strcmp(cmd, "q") == 0) {
            cmd_quit();
        } else if (strcmp(cmd, "si") == 0) {
            char *arg = strtok(NULL, " \n");
            int steps = arg ? atoi(arg) : 1;
            cmd_step(steps);
        } else if (strcmp(cmd, "info") == 0) {
            char *arg = strtok(NULL, " \n");
            if (arg) {
                cmd_info(arg[0]);
            } else {
                printf("Usage: info r\n");
            }
        } else if (strcmp(cmd, "x") == 0) {
            char *len_str = strtok(NULL, " \n");
            char *addr_str = strtok(NULL, " \n");
            if (len_str && addr_str) {
                int len = atoi(len_str);
                uint64_t addr = strtoull(addr_str, NULL, 0);
                cmd_examine(len, addr);
            } else {
                printf("Usage: x <len> <addr>\n");
            }
        } else {
            printf("Unknown command '%s'. Type 'help' for a list of commands.\n", cmd);
        }
    }
}