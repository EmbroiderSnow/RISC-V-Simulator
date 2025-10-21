#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <decode.h>

typedef struct {
    uint64_t reg[32];
    uint64_t pc;
} CPU_state;

typedef enum {
    STAGE_IF,
    STAGE_ID,
    STAGE_EX,
    STAGE_MEM,
    STAGE_WB,
    STAGE_DONE // check inst finish
} CPU_Cycle_Stage;

void init_cpu();
void cpu_exec();
void exec_once();
void exec_single_inst(uint32_t inst);
void halt_trap(uint64_t pc, uint64_t code);

#endif