#include <llvm-c/Target.h>
#include <llvm-c/Disassembler.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <disasm.h>

extern LLVMDisasmContextRef disasm_ctx;

LLVMDisasmContextRef init_disasm(const char *triple) {
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllDisassemblers();

    LLVMDisasmContextRef disasm_ctx = LLVMCreateDisasm(triple, NULL, 0, NULL, NULL);
    if (!disasm_ctx) {
        fprintf(stderr, "Failed to create LLVM disassembler for triple %s\n", triple);
    }
    return disasm_ctx;
}

void disassemble_inst(LLVMDisasmContextRef disasm_ctx, uint8_t *instruction_bytes, int num_bytes, uint64_t pc, char *out_buffer, size_t out_buffer_size) {
    if (!disasm_ctx) {
        snprintf(out_buffer, out_buffer_size, "Disassembler not initialized");
        return;
    }

    size_t bytes_read = LLVMDisasmInstruction(
        disasm_ctx,
        instruction_bytes,
        num_bytes,
        pc,
        out_buffer,
        out_buffer_size
    );

    if (bytes_read == 0) {
        snprintf(out_buffer, out_buffer_size, "unknown instruction");
    }
}

void cleanup_disasm(LLVMDisasmContextRef disasm_ctx) {
    if (disasm_ctx) {
        LLVMDisasmDispose(disasm_ctx);
    }
}

void init_llvm_disassembler() {
    disasm_ctx = init_disasm("riscv64-unknown-elf");
}

void cleanup_llvm_disassembler() {
    cleanup_disasm(disasm_ctx);
}

void handle_itrace(Decode *s) {
    char asm_buf[128];
    uint8_t bytes[4];
    bytes[0] = s->inst & 0xFF;
    bytes[1] = (s->inst >> 8) & 0xFF;
    bytes[2] = (s->inst >> 16) & 0xFF;
    bytes[3] = (s->inst >> 24) & 0xFF;
    disassemble_inst(disasm_ctx, bytes, 4, s->pc, asm_buf, sizeof(asm_buf));
    printf("\33[1;34m0x%016lx\33[1;0m: %08x          %s\n", s->pc, s->inst, asm_buf);
}