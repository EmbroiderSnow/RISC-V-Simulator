#include <common.h>
#include <iss_core.h>
#include <mc_core.h>
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
uint64_t global_cycle_count = 0;
uint64_t ninst = 0;

void init_cpu(){
    cpu.pc = MEM_BASE;
    memset(cpu.reg, 0, sizeof(cpu.reg));
    memset(cpu.csr, 0, sizeof(cpu.csr));
}

// ------------ ISS SIM ------------

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

// --------- Multi-cycle SIM ---------

void mc_exec_once() {
    // uint64_t record = global_cycle_count;
    ++ninst;
    Decode s;
    Multi_Cycle_Stage stage = STAGE_IF;
    uint64_t alu_result = 0;
    uint64_t mem_result = 0;
    while (stage != STAGE_DONE) {
        switch (stage) {
            case STAGE_IF:
                mc_IF(&s);
                if (itrace_enabled)
                    handle_itrace(&s);
                push_stage(&s, &stage);
                global_cycle_count++;
                break;
            case STAGE_ID:
                // add cycle count in func decode_ID
                mc_ID(&s);
                push_stage(&s, &stage);
                break;
            case STAGE_EX:
                // alu_result used to pass result to MEM or WB stage
                // add cycle count in func decode_EX
                mc_EX(&s, &alu_result);
                push_stage(&s, &stage);
                break;
            case STAGE_MEM:
                // mem_result used to pass result to WB stage
                // add cycle count in func decode_MEM
                mc_MEM(&s, alu_result, &mem_result);
                push_stage(&s, &stage);
                break;
            case STAGE_WB:
                mc_WB(&s, alu_result, mem_result);
                push_stage(&s, &stage);
                break;
            case STAGE_DONE:
                printf("INST END\n");
                goto loop_end;
        }
    }
loop_end:
    cpu.pc = s.dnpc;
    // printf("spend %ld cycle\n", global_cycle_count - record);
}

static inline void show_performance() {
    printf(ANSI_FMT("Performance: \n\tINST NUM  = %4ld\n\tCYCLE NUM = %4ld\n\tCPI       = %.3f\n", ANSI_FG_YELLOW), ninst, global_cycle_count, (float)global_cycle_count/(float)ninst);
}

void mc_cpu_exec() {
    while (running) {
        mc_exec_once();
    }
    show_performance();
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
