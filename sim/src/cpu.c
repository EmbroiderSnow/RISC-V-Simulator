#include <common.h>
#include <iss_core.h>
#include <memory.h>
#include <disasm.h>
#include <macro.h>
#include "ftrace.h"

extern int itrace_enabled;
extern int ftrace_enabled;
extern LLVMDisasmContextRef disasm_ctx;

int indentaton_level = 0;
CPU_state cpu = {};
static int running = 1;

void init_cpu(){
    cpu.pc = MEM_BASE;
    memset(cpu.reg, 0, sizeof(cpu.reg));
    memset(cpu.csr, 0, sizeof(cpu.csr));
}

void iss_exec_once() {
    Decode s;
    s.pc   = cpu.pc;
    s.inst = inst_fetch(s.pc);
    s.snpc = s.pc + 4;
    if (itrace_enabled && disasm_ctx) {
        handle_itrace(&s);
    }
    if (ftrace_enabled) {
        handle_ftrace(&s);
    }
    decode_exec(&s);
    cpu.pc = s.dnpc;
}

void iss_cpu_exec() {
    while (running) {
        iss_exec_once();
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
