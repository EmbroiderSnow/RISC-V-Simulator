#include <common.h>
#include <decode.h>
#include <memory.h>
#include <disasm.h>
#include <macro.h>
#include "ftrace.h"

extern int itrace_enabled;
extern int ftrace_enabled;
extern SymbolTable *sym_table;
extern LLVMDisasmContextRef disasm_ctx;

int indentaton_level = 0;
CPU_state cpu = {};
static int running = 1;

void init_cpu(){
    cpu.pc = MEM_BASE;
    memset(cpu.reg, 0, sizeof(cpu.reg));
    memset(cpu.csr, 0, sizeof(cpu.csr));
}

static int is_call(uint32_t inst) {
    uint32_t opcode = inst & 0x7f;
    uint32_t rd = (inst >> 7) & 0x1f;
    if (opcode == 0x6f && rd == 1) { // JAL && rd == x1 (ra)
        return 1;
    }
    if (opcode == 0x67 && rd == 1) { // JALR && rd == x1 (ra)
        return 1;
    }
    return 0;
}

uint64_t get_call_target_addr(Decode *s) {
    uint32_t inst = s->inst;
    uint32_t opcode = inst & 0x7f;

    // J-type (jal)
    if (opcode == 0b1101111) {
        int32_t imm_j = SEXT(BITS(inst, 31, 31) << 20 | \
                                BITS(inst, 19, 12) << 12 | \
                                BITS(inst, 20, 20) << 11 | \
                                BITS(inst, 30, 21) << 1, 21);
        return s->pc + imm_j;
    }
    // I-type (jalr)
    else if (opcode == 0b1100111) {
        int rs1_idx = (inst >> 15) & 0x1f;
        int32_t imm_i = (int32_t)inst >> 20;
        return cpu.reg[rs1_idx] + imm_i;
    }

    return 0;
}

static int is_ret(uint32_t inst) {
    uint32_t opcode = inst & 0x7f;
    uint32_t rs1 = (inst >> 15) & 0x1f;
    uint32_t rd = (inst >> 7) & 0x1f;
    uint32_t imm = (inst >> 20) & 0xfff; 
    if (opcode == 0x67 && rs1 == 1 && rd == 0 && imm == 0) { // JALR x0, 0(x1)
        return 1;
    }
    return 0;
}

void exec_once(){
    Decode s;
    s.pc   = cpu.pc;
    s.inst = inst_fetch(s.pc);
    s.snpc = s.pc + 4;
    if (itrace_enabled && disasm_ctx) {
        char asm_buf[128];
        uint8_t bytes[4];
        bytes[0] = s.inst & 0xFF;
        bytes[1] = (s.inst >> 8) & 0xFF;
        bytes[2] = (s.inst >> 16) & 0xFF;
        bytes[3] = (s.inst >> 24) & 0xFF;
        disassemble_inst(disasm_ctx, bytes, 4, s.pc, asm_buf, sizeof(asm_buf));
        printf("\33[1;34m0x%016lx\33[1;0m: %08x          %s\n", s.pc, s.inst, asm_buf);
    }
    if (ftrace_enabled) {
        if (is_call(s.inst)) {
            uint64_t target_addr = get_call_target_addr(&s);
            printf("Call target address: 0x%lx\n", target_addr);
            const FuncSymbol *func_symbol = find_func(sym_table, target_addr);
            const char *func_name = func_symbol ? func_symbol->name : "unknown_function";
            const uint64_t func_addr = func_symbol ? func_symbol->address : 0;
            printf("\33[1;34m0x%08lx\33[1;0m: ", s.pc);
            for (int i = 0; i < indentaton_level; i++) {
                printf("  ");
            }
            printf("call [%s@%08lx]\n", func_name, func_addr);
            indentaton_level++;
        } else if (is_ret(s.inst)) {
            indentaton_level--;
            if (indentaton_level < 0) indentaton_level = 0;
            const FuncSymbol *func_symbol = find_func(sym_table, s.pc);
            const char *func_name = func_symbol ? func_symbol->name : "unknown_function";
            printf("\33[1;34m0x%08lx\33[1;0m: ", s.pc);
            for (int i = 0; i < indentaton_level; i++) {
                printf("  ");
            }
            printf("ret  [%s]\n", func_name);
        }
    }
    decode_exec(&s);
    cpu.pc = s.dnpc;
}

void exec_single_inst(uint32_t inst) {
    Decode s;
    s.inst = inst;
    decode_exec(&s);
}

void cpu_exec(){
    while(running){
        exec_once();
    }
}

void halt_trap(uint64_t pc, uint64_t code){
    if(code){
        printf(ANSI_FMT("HIT BAD TRAP!\n", ANSI_FG_RED));
    }else{
        printf(ANSI_FMT("HIT GOOD TRAP!\n", ANSI_FG_GREEN));
    }
    log_info("Program ended at pc %08lx, with exit code %ld.", pc, code);
    running = 0;
}
