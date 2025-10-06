#ifndef CPU_H
#define CPU_H

void init_cpu();
void cpu_exec();
void exec_single_inst(uint32_t inst);
void halt_trap(uint64_t pc, uint64_t code);

#endif