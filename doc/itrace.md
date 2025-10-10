# itrace

Implementing the **itrace** feature is relatively straightforward — the main task is to **disassemble RISC-V machine code**.
 Initially, I wrote a custom disassembly function, but the documentation suggested that the **LLVM library** could be linked to handle this task. Therefore, I ultimately chose to integrate **LLVM** for the implementation.

This approach has an additional benefit: in **debug mode**, the `si` (step instruction) command can now print the **current instruction being executed**, making debugging much more convenient.

The disassembly implementation is defined in **`sim/include/disasm.h`** and **`sim/src/disasm.c`**.