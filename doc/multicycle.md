# 🧩 Multi-Cycle RISC-V Simulator — Stage Design Overview

This document explains the functional design of each stage in the **multi-cycle RISC-V simulator** implemented in `mc_core.c`.
 The design divides instruction execution into **five sequential stages**, each representing a major phase of instruction processing.
 The overall control flow is managed by a simple **finite-state machine (FSM)** that transitions between these stages according to the instruction type.

------

## ⚙️ Overview of the Multi-Cycle FSM

Each instruction goes through the following pipeline-like stages:

| Stage        | Name               | Description                                                  |
| ------------ | ------------------ | ------------------------------------------------------------ |
| `STAGE_IF`   | Instruction Fetch  | Fetch instruction from memory using the current PC           |
| `STAGE_ID`   | Instruction Decode | Decode instruction, identify type, extract operands and immediates |
| `STAGE_EX`   | Execute            | Perform ALU computation or branch/jump target calculation    |
| `STAGE_MEM`  | Memory Access      | Perform load or store operations                             |
| `STAGE_WB`   | Write-Back         | Write results to register file                               |
| `STAGE_DONE` | Completion         | Instruction execution finished                               |

The FSM transitions between these stages using the helper function `push_stage()`:

- It examines the **instruction type** (`TYPE_R`, `TYPE_I`, `TYPE_S`, `TYPE_B`, `TYPE_U`, `TYPE_J`) and decides which stage should follow.
- For example, R-type instructions skip `MEM`, while load/store instructions include it.

------

## 🧠 Stage Details

### 🟦 1. Instruction Fetch (IF)

**Function:** `void mc_IF(Decode *s)`

**Main Tasks:**

- Read the instruction from instruction memory using the current **program counter (PC)**.
- Compute the sequential next PC (`snpc = PC + 4`).
- Set both `snpc` and `dnpc` (default next PC) to this value.

**Pseudo-code:**

```c
s->pc = cpu.pc;
s->inst = inst_fetch(s->pc);
s->snpc = s->pc + 4;
s->dnpc = s->snpc;
```

**Key Notes:**

- This stage consumes one cycle for memory access.
- No register or memory modification occurs.
- The output is a decoded `inst` that will be passed to the next stage.

------

### 🟩 2. Instruction Decode (ID)

**Function:** `void mc_ID(Decode *s)`

**Main Tasks:**

- Decode the fetched instruction into **operation type**, **register indices**, and **immediate values**.
- Identify instruction category (`TYPE_R`, `TYPE_I`, `TYPE_S`, etc.).
- For load/store instructions, set the flag `s->is_load = 1` or `0`.
- For `jal` and `jalr`, compute preliminary `dnpc` (jump target).

**Key Features:**

- Uses the `INSTPAT` macro pattern system to match binary opcodes.
- Initializes control information that determines later behavior.
- Increments `global_cycle_count` to represent the decode phase delay.

**Example:**

```c
INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, s->is_load = 0);
INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, s->is_load = 1);
```

**Outputs:**

- Instruction classification (`s->type`)
- Immediate value and register IDs
- Branch or jump control signals (`dnpc`)

------

### 🟥 3. Execute (EX)

**Function:** `void mc_EX(Decode *s, uint64_t *alu_result)`

**Main Tasks:**

- Perform arithmetic, logic, shift, and comparison operations via ALU.
- Compute branch or jump target addresses.
- Update `dnpc` if a branch is taken.
- For load/store instructions, compute the **effective address**.

**Example Behaviors:**

- `add` / `sub`: perform integer arithmetic.
- `sll`, `sra`, `and`, `or`, `xor`: perform bitwise operations.
- `beq`, `bne`, `blt`, etc.: compare operands and update `s->dnpc`.
- `jalr`: set jump target to `(src1 + imm) & ~1`.

**Performance modeling:**

- Arithmetic and logical ops add `+1` to `global_cycle_count`.
- Multiplication/division instructions add more (up to +39 cycles) to reflect realistic multi-cycle latency.

**Output:**

- `*alu_result` contains the computed result or memory address.

------

### 🟨 4. Memory Access (MEM)

**Function:** `void mc_MEM(Decode *s, uint64_t alu_result, uint64_t *mem_result)`

**Main Tasks:**

- For **load** instructions: read from memory at address `alu_result`.
- For **store** instructions: write to memory at address `alu_result` using `src2`.
- Pass the loaded data to the next stage (`mem_result`).

**Implementation Highlights:**

```c
// Load
INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld, I, *mem_result = Mr(alu_result, 8));
// Store
INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(alu_result, 4, src2));
```

**Cycle accounting:**

- Each load/store operation increments the global cycle counter by 1.

**Output:**

- `mem_result` holds the loaded value (for loads).
- Stores do not produce results to write back.

------

### 🟧 5. Write-Back (WB)

**Function:** `void mc_WB(Decode *s, uint64_t alu_result, uint64_t mem_result)`

**Main Tasks:**

- Write results back into the register file (`R(rd)`).
- Choose between `alu_result` (for arithmetic ops) or `mem_result` (for loads).
- Handle jump instructions (`jal`, `jalr`) by writing return address (`pc + 4`) to the destination register.

**Examples:**

```c
INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, R(rd) = imm);
INSTPAT("??????? ????? ????? 000 ????? 00000 11", lw, I, R(rd) = mem_result);
INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R(rd) = alu_result);
```

**Key Points:**

- Enforces `R(0) = 0` (as required by RISC-V).
- Marks the completion of the instruction (`STAGE_DONE`).

------

## 🔁 Stage Transition Summary

The **`push_stage()`** function governs how each instruction advances through the stages:

| Current Stage | Next Stage             | Condition                                              |
| ------------- | ---------------------- | ------------------------------------------------------ |
| `IF`          | `ID`                   | Always                                                 |
| `ID`          | `EX` or `WB`           | Depends on instruction type (`J` jumps directly to WB) |
| `EX`          | `MEM`, `WB`, or `DONE` | Loads/stores go to `MEM`; others to `WB`               |
| `MEM`         | `WB` or `DONE`         | Loads → `WB`; stores → `DONE`                          |
| `WB`          | `DONE`                 | Always                                                 |

This ensures that:

- R-type instructions: **IF → ID → EX → WB → DONE**
- I-type arithmetic: **IF → ID → EX → WB → DONE**
- Loads: **IF → ID → EX → MEM → WB → DONE**
- Stores: **IF → ID → EX → MEM → DONE**
- Branches: **IF → ID → EX → DONE**
- Jumps: **IF → ID → WB → DONE**

------

## 🧩 Cycle Count Modeling

Each stage contributes to the global cycle count:

| Stage | Typical Cycle Cost | Notes                                  |
| ----- | ------------------ | -------------------------------------- |
| IF    | +1                 | Fetch from memory                      |
| ID    | +1                 | Decode delay                           |
| EX    | +1                 | ALU op; multiply/divide adds up to +39 |
| MEM   | +1                 | Memory access                          |
| WB    | +1                 | Register write delay                   |

Thus, a typical **R-type** instruction takes ~5 cycles, while a **load** instruction takes ~6 cycles, and a **DIV** may take ~40 cycles.