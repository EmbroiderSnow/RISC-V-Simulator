#include <cpu.h>
#include <memory.h>
#include <common.h>
#include <dbg.h>
#include <decode.h>
#include <disasm.h>
#include "ftrace.h"

extern SymbolTable *sym_table;
extern LLVMDisasmContextRef disasm_ctx;

const char* riscv_abi_names[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10","s11","t3","t4","t5","t6"
};

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
    cleanup_llvm_disassembler();
    exit(0);
}

static void cmd_step(int steps) {
    char asm_buf[128];
    for (int i = 0; i < steps; ++i) {
        uint64_t current_pc = cpu.pc;
        uint32_t inst_code = mem_read(current_pc, 4);

        uint8_t bytes[4];
        bytes[0] = inst_code & 0xFF;
        bytes[1] = (inst_code >> 8) & 0xFF;
        bytes[2] = (inst_code >> 16) & 0xFF;
        bytes[3] = (inst_code >> 24) & 0xFF;

        disassemble_inst(disasm_ctx, bytes, 4, current_pc, asm_buf, sizeof(asm_buf));

        // Print the address and the disassembled instruction
        printf("\33[1;34m=> 0x%016lx\33[1;0m: \t%s\n", current_pc, asm_buf);

        // Look up and print the function name if available
        if (sym_table) {
            const FuncSymbol *func_symbol = find_func(sym_table, current_pc);
            if (func_symbol) {
                printf("\33[1;34m0x%08lx\33[1;0m in \33[1;33m%s\33[1;0m ()\n", func_symbol->address, func_symbol->name);
            }
        }

        // Execute the instruction
        exec_once(); 
    }
}

static void cmd_info(char arg) {
    if (arg == 'r') {
        for (int i = 0; i < 32; ++i) {
            char reg_id[5];
            sprintf(reg_id, "x%d", i);
            char reg_name[6];
            sprintf(reg_name, "(%s)", riscv_abi_names[i]);
            printf("\33[1;34m%-4s %-6s\33[1;0m : 0x%016lx\n", reg_id, reg_name, cpu.reg[i]);
        }
        printf("\33[1;34mpc\33[1;0m          : 0x%016lx\n", cpu.pc);
    } else {
        printf("Unknown info command '%c'\n", arg);
    }
}

static void cmd_examine(int len, uint64_t addr) {
    for (int i = 0; i < len; ++i) {
        uint32_t data = mem_read(addr + i * 4, 4);
        if (i % 4 == 0) {
            printf("\33[1;34m0x%016lx\33[1;0m: ", addr + i * 4);
        }
        printf("0x%08x ", data);
        if (i % 4 == 3 || i == len - 1) {
            printf("\n");
        }
    }
}

void debug_loop() {
    init_llvm_disassembler();
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