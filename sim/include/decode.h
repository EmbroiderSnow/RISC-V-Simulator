#ifndef DECODE_H
#define DECODE_H

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

// This is a DECLARATION
extern const char* riscv_abi_names[32];

void decode_exec(Decode *s);
void decode_ID(Decode *s);
void decode_EX(Decode *s, uint64_t *alu_result);
void decode_MEM(Decode *s, uint64_t alu_result, uint64_t *mem_result);
void decode_WB(Decode *s, uint64_t alu_result, uint64_t mem_result);
void disassemble(Decode *s, char *asm_buf);

#endif