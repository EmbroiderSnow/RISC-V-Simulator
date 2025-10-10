# ftrace

The implementation of **ftrace** involves two key components:
 (1) **loading and parsing the symbol table**, and
 (2) **identifying `call` and `ret` instructions** — which, unlike in x86, are not represented by explicit opcodes in RISC-V.

I used the **libelf** library to parse the **ELF file’s symbol table** and designed a data structure to store function **(name, address)** tuples. After loading, the addresses are **sorted** to enable efficient lookup during execution.

The **ftrace** implementation is defined in **`sim/include/ftrace.h`** and **`sim/src/ftrace.c`**, where you can check the detailed logic.

Similar to **itrace**, **ftrace** enhances the **debug mode**: the `si` (step instruction) command now supports displaying **which function the current instruction belongs to**, offering improved traceability during debugging.