#ifndef DISASM_H
#define DISASM_H

#include <llvm-c/Target.h>
#include <llvm-c/Disassembler.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <isa_decode.h>

LLVMDisasmContextRef init_disasm(const char *triple);
void disassemble_inst(LLVMDisasmContextRef disasm_ctx, uint8_t *bytes, int len, uint64_t pc, char *out, size_t out_len);
void cleanup_disasm(LLVMDisasmContextRef disasm_ctx);

void init_llvm_disassembler();
void cleanup_llvm_disassembler();

void handle_itrace(Decode *s);

#endif