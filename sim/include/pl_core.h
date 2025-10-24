#ifndef PL_CORE_H
#define PL_CORE_H

#include <isa_decode.h>

// Defination of pipeline registers
typedef struct {
    Decode s;
    uint64_t predict_pc;
    int valid;
} IF_ID_Reg;

typedef int PL_SIGNAL;
typedef int REG_NO;

#define USE_ALU 1
#define NOT_USE_ALU 0
#define SELECT_PC 0
#define RS1 1
#define REG_ZERO 3
#define RS2 0
#define IMM 1
#define READ_MEM 1
#define NOT_READ_MEM 0
#define WRITE_MEM 1
#define NOT_WRITE_MEM 0
#define WRITE_REG 1
#define NOT_WRITE_REG 0
#define ALU_RES 0
#define MEM_RES 1
#define PC_PLUS_4 2
#define NOT_CARE -1

typedef struct {
    Decode s;
    PL_SIGNAL ALU_use; // Does this instruction use ALU? no: 0, yes: 1
    PL_SIGNAL ALU_src1; // PC: 0, rs1: 1
    PL_SIGNAL ALU_src2; // rs2: 0, imm: 1
    REG_NO rs1;
    REG_NO rs2;
    // ALU_op: use MARCO INSTPAT to hard encode
    // ---- not used in stage EX but need to pass to MEM and WB ----
    PL_SIGNAL MEM_read; // Does this instruction read memory? no: 0, yes: 1
    PL_SIGNAL MEM_write; // Does this instruction write memory? no: 0, yes: 1
    // store instruction read rs2 and write the value in rs2 to memory, so pass rs2 to EX_MEM_reg
    PL_SIGNAL REG_write; // Does this instruction write register? no: 0, yes: 1
    REG_NO REG_dst; // Which register does this instruction write to? (equals to rd, maybe)
    PL_SIGNAL REG_src; // Where is the value written to the register read from? alu_result: 0, mem_result: 1, pc+4: 2
    uint64_t predict_pc;
    int valid;
} ID_EX_Reg;

typedef struct {
    Decode s;
    PL_SIGNAL MEM_read; // Does this instruction read memory? no: 0, yes: 1
    PL_SIGNAL MEM_write; // Does this instruction write memory? no: 0, yes: 1
    REG_NO rs2;
    uint64_t alu_result;
    PL_SIGNAL REG_write; // Does this instruction write register? no: 0, yes: 1
    REG_NO REG_dst; // Which register does this instruction write to? (equals to rd, maybe)
    PL_SIGNAL REG_src; // Where is the value written to the register read from? alu_result: 0, mem_result: 1, pc+4: 2
    int valid;
} EX_MEM_Reg;

typedef struct {
    Decode s;
    uint64_t alu_result;
    uint64_t mem_result;
    PL_SIGNAL REG_write; // Does this instruction write register? no: 0, yes: 1
    REG_NO REG_dst; // Which register does this instruction write to? (equals to rd, maybe)
    PL_SIGNAL REG_src; // Where is the value written to the register read from? alu_result: 0, mem_result: 1, pc+4: 2
    int valid;
} MEM_WB_Reg;

void init_pipeline();
void pl_IF();
void pl_ID();
void pl_EX();
void pl_MEM();
void pl_WB();

#endif