#include <common.h>
#include <macro.h>
#include <pattern.h>
#include <cpu.h>
#include <memory.h>
#include <isa_decode.h>

void decode_operand(Decode *s, int *rd, uint64_t *src1, uint64_t *src2, uint64_t *imm, DecodeType type){
    uint32_t i = s->inst;
    int rs1 = BITS(i, 19, 15);
    int rs2 = BITS(i, 24, 20);
    *rd     = BITS(i, 11,  7);
    switch(type) {
        case TYPE_R: src1R(); src2R();         break;
        case TYPE_I: src1R();          immI(); break;
        case TYPE_S: src1R(); src2R(); immS(); break;
        case TYPE_B: src1R(); src2R(); immB(); break;
        case TYPE_U:                   immU(); break;
        case TYPE_J:                   immJ(); break;
        case TYPE_N:                           break;
    }
}