#include <common.h>
#include <macro.h>
#include <pattern.h>
#include <dbg.h>
#include <cpu.h>
#include <memory.h>
#include <isa_decode.h>

void decode_operand(Decode *s, int *rd, uint64_t *src1, uint64_t *src2, uint64_t *imm, DecodeType type){
    uint32_t i = s->inst;
    int rs1 = BITS(i, 19, 15);
    int rs2 = BITS(i, 24, 20);
    *rd     = BITS(i, 11,  7);
    s->type = type;
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

int is_load(uint64_t inst) {
    uint32_t opcode = inst & 0x7F;
    return opcode == 0b0000011;
}

DecodeType get_inst_type(uint64_t inst) {
    uint32_t opcode = inst & 0x7F;

    switch (opcode) {
        // R-Type
        case 0b0110011: // OP (add, sub, sll, slt, sltu, xor, srl, sra, or, and)
        case 0b0111011: // OP-32 (addw, subw, sllw, srlw, sraw)
            return TYPE_R;

        // I-Type
        case 0b0000011: // LOAD (lb, lh, lw, lbu, lhu, lwu, ld)
        case 0b0010011: // OP-IMM (addi, slti, sltiu, xori, ori, andi, slli, srli, srai)
        case 0b0011011: // OP-IMM-32 (addiw, slliw, srliw, sraiw)
        case 0b1100111: // JALR
        case 0b1110011: // SYSTEM (ecall, ebreak, csr*)
        case 0b0001111: // FENCE, FENCE.I
            return TYPE_I;

        // S-Type
        case 0b0100011: // STORE (sb, sh, sw, sd)
            return TYPE_S;

        // B-Type
        case 0b1100011: // BRANCH (beq, bne, blt, bge, bltu, bgeu)
            return TYPE_B;

        // U-Type
        case 0b0110111: // LUI
        case 0b0010111: // AUIPC
            return TYPE_U;

        // J-Type
        case 0b1101111: // JAL
            return TYPE_J;

        // Unknown
        default:
            return TYPE_N;
    }
}