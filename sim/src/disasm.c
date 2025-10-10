#include <llvm-c/Target.h>
#include <llvm-c/Disassembler.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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