#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define CSR_MEPC 0x341
#define CSR_MCAUSE 0x342
#define CSR_MSTATUS 0x300
#define CSR_MTVEC 0x305

typedef struct {
    uint64_t reg[32];
    uint64_t pc;
    uint64_t csr[4096];
} CPU_state;

void init_cpu();
void halt_trap(uint64_t pc, uint64_t code);

// ------------ ISS SIM ------------

void iss_cpu_exec();
void iss_exec_once();

// --------- Multi-cycle SIM ---------

void mc_cpu_exec();
void mc_exec_once();

// ---------- Pipeline SIM -----------

void pl_cpu_exec();
void pl_exec_once();

#endif