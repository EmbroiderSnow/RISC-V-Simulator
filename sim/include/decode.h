#ifndef DECODE_H
#define DECODE_H

typedef struct {
  uint64_t pc;
  uint64_t snpc; // static next pc
  uint64_t dnpc; // dynamic next pc
  uint32_t inst;
} Decode;

// This is a DECLARATION
extern const char* riscv_abi_names[32];

void decode_exec(Decode *s);
void disassemble(Decode *s, char *asm_buf);

#endif