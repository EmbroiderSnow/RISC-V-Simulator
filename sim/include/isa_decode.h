#ifndef ISA_DECODE_H
#define ISA_DECODE_H

#include <stdint.h>
#include <pattern.h>
#include <macro.h>
#include <dbg.h>
#include <memory.h>

typedef enum {
    TYPE_R, TYPE_I, TYPE_S, TYPE_B, TYPE_U, TYPE_J, TYPE_N
} DecodeType;

typedef struct {
  uint64_t pc;
  uint64_t snpc; // static next pc
  uint64_t dnpc; // dynamic next pc
  uint32_t inst;
  DecodeType type;
  uint8_t is_load;
} Decode;

void decode_operand(Decode *s, int *rd, uint64_t *src1, uint64_t *src2, uint64_t *imm, DecodeType type);

#define R(i) (cpu.reg[i])
#define Mr mem_read
#define Mw mem_write
#define HALT(thispc, code) halt_trap(thispc, code)
#define NOP do {} while(0)

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immJ() do { *imm = SEXT(BITS(i, 31, 31) << 20 | \
                                BITS(i, 19, 12) << 12 | \
                                BITS(i, 20, 20) << 11 | \
                                BITS(i, 30, 21) << 1, 21);} while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immB() do { *imm = SEXT(BITS(i, 31, 31) << 12 | \
                                BITS(i, 7, 7) << 11 | \
                                BITS(i, 30, 25) << 5 | \
                                BITS(i, 11, 8) << 1, 13);} while(0)

#define INSTPAT_INST(s) ((s)->inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

#endif