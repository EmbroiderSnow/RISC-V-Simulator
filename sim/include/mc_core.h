#ifndef MC_CORE_H
#define MC_CORE_H

#include <isa_decode.h>

typedef enum {
    STAGE_IF,
    STAGE_ID,
    STAGE_EX,
    STAGE_MEM,
    STAGE_WB,
    STAGE_DONE // check inst finish
} Multi_Cycle_Stage;

void mc_IF(Decode *s);
void mc_ID(Decode *s);
void mc_EX(Decode *s, uint64_t *alu_result);
void mc_MEM(Decode *s, uint64_t alu_result, uint64_t *mem_result);
void mc_WB(Decode *s, uint64_t alu_result, uint64_t mem_result);

void push_stage(Decode *s, Multi_Cycle_Stage *stage);

#endif