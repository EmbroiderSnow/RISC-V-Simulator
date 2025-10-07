#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct {
    uint64_t reg[32];
    uint64_t pc;
} CPU_state;

void init_cpu();
void cpu_exec();
void exec_once();
void exec_single_inst(uint32_t inst);
void halt_trap(uint64_t pc, uint64_t code);

#endif