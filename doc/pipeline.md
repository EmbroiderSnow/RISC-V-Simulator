# 🧩 Pipelined RISC-V Simulator — Pipeline Register Design



This document details the structure and function of the **Pipeline Registers** defined in the `pl_core.h` source file. In the 5-stage pipeline design, these registers are positioned between the instruction stages, serving as temporary storage for data and control signals. They are crucial for data forwarding, handling pipeline hazards, and inserting "bubbles".

The design philosophy aligns with the multi-cycle simulator's stage breakdown:

| **Pipelined Stage**     | **Multi-Cycle Equivalent** |
| ----------------------- | -------------------------- |
| IF (Instruction Fetch)  | `STAGE_IF`                 |
| ID (Instruction Decode) | `STAGE_ID`                 |
| EX (Execute)            | `STAGE_EX`                 |
| MEM (Memory Access)     | `STAGE_MEM`                |
| WB (Write-Back)         | `STAGE_WB`                 |

------



## ⚙️ Pipeline Register Overview



The simulator utilizes four main pipeline registers, which separate the five stages:

| **Register** | **Separating Stages** | **Primary Function**                                         |
| ------------ | --------------------- | ------------------------------------------------------------ |
| `IF_ID_Reg`  | IF / ID               | Stores the fetched instruction and PC.                       |
| `ID_EX_Reg`  | ID / EX               | Passes operands, register indices, and all control signals.  |
| `EX_MEM_Reg` | EX / MEM              | Passes the ALU result and control signals for MEM/WB stages. |
| `MEM_WB_Reg` | MEM / WB              | Passes the final result (from ALU or Memory) for Write-Back. |

Control flow within the pipeline is governed by specific global flags:

- `Predict_Right`: Controls the flushing of the pipeline on a branch misprediction.
- `PC_Write_Enable` & `IF_ID_Write_Enbale`: Used to stall the IF/ID stages to resolve **RAW hazards**.

------



## 🧠 Register Details





### 🟦 1. IF/ID Register



**Structure:** `IF_ID_Reg`

**Purpose:** To store data passed from the **Instruction Fetch (IF)** stage to the **Instruction Decode (ID)** stage.

**Key Fields:**

| **Field**    | **Type**   | **Description**                                              |
| ------------ | ---------- | ------------------------------------------------------------ |
| `s`          | `Decode`   | Contains instruction's PC, the instruction (`inst`), and default next PC (`dnpc`, `snpc`). |
| `predict_pc` | `uint64_t` | The predicted next PC (usually `pc + 4`).                    |
| `valid`      | `int`      | Indicates if the register holds a valid instruction (1) or a bubble (0). Cleared on misprediction. |

------



### 🟩 2. ID/EX Register



**Structure:** `ID_EX_Reg`

**Purpose:** To store data and all generated control signals passed from **Instruction Decode (ID)** to the **Execute (EX)** stage.

**Key Features:**

- **Control Signals:** Contains all necessary control bits (e.g., `ALU_use`, `MEM_read/write`, `REG_write`) which will travel through the rest of the pipeline.
- **Operands:** Passes the source register *indices* (`rs1`, `rs2`).
- **Hazard Handling:** The `valid` field is set to 0 to insert a bubble when a **RAW hazard** is detected and a stall is required.

**Control Signal Enumerations:**

| **Signal**  | **Value** | **Description**                              |
| ----------- | --------- | -------------------------------------------- |
| `USE_ALU`   | 1         | Instruction requires the ALU                 |
| `RS1`       | 1         | ALU Source 1 is $\text{R[rs1]}$              |
| `IMM`       | 1         | ALU Source 2 is the immediate value          |
| `READ_MEM`  | 1         | Instruction is a load operation              |
| `WRITE_REG` | 1         | Instruction writes back to the register file |
| `ALU_RES`   | 0         | Write-back source is the ALU result          |

**Fields (Control & Data):**

C

```c
typedef struct {
    Decode s;             // Instruction info (pc, inst)
    PL_SIGNAL ALU_use;    // USE_ALU / NOT_USE_ALU
    PL_SIGNAL ALU_src1;   // RS1 / SELECT_PC / REG_ZERO
    PL_SIGNAL ALU_src2;   // RS2 / IMM
    REG_NO rs1;           // Index of rs1 register
    REG_NO rs2;           // Index of rs2 register
    PL_SIGNAL MEM_read;   // READ_MEM / NOT_READ_MEM
    PL_SIGNAL MEM_write;  // WRITE_MEM / NOT_WRITE_MEM
    PL_SIGNAL REG_write;  // WRITE_REG / NOT_WRITE_REG
    REG_NO REG_dst;      // Destination register index (rd)
    PL_SIGNAL REG_src;    // ALU_RES / MEM_RES / PC_PLUS_4
    uint64_t predict_pc;  // Predicted PC from IF stage
    int valid;            // 1: Valid instruction, 0: Bubble
} ID_EX_Reg;
```

------



### 🟥 3. EX/MEM Register



**Structure:** `EX_MEM_Reg`

**Purpose:** To store data passed from the **Execute (EX)** stage to the **Memory Access (MEM)** stage.

**Key Tasks:**

- **Pass ALU Result:** The computed result (`alu_result`) which is either the final result (R/I-type) or the effective memory address (L/S-type).
- **Store Operand:** Passes the index of `rs2` (`rs2`) for Store instructions to access its *value* in the MEM stage.
- **Pass-Through Control:** Forwarding control signals for the WB stage.

**Fields (Control & Data):**

C

```c
typedef struct {
    Decode s;
    PL_SIGNAL MEM_read;
    PL_SIGNAL MEM_write;
    REG_NO rs2;           // rs2 index (for Store instructions)
    uint64_t alu_result;  // ALU computation result / Effective Address
    PL_SIGNAL REG_write;
    REG_NO REG_dst;
    PL_SIGNAL REG_src;
    int valid;
} EX_MEM_Reg;
```

------



### 🟨 4. MEM/WB Register



**Structure:** `MEM_WB_Reg`

**Purpose:** To store data passed from the **Memory Access (MEM)** stage to the final **Write-Back (WB)** stage.

**Key Tasks:**

- **Final Data:** Stores the final value to be written back to the register file, choosing between `alu_result` and `mem_result` based on the instruction type.
- **Load Result:** If a load instruction, `mem_result` holds the value read from data memory.

**Fields (Control & Data):**

C

```c
typedef struct {
    Decode s;
    uint64_t alu_result;  // Result from EX stage
    uint64_t mem_result;  // Data read from memory (for loads)
    PL_SIGNAL REG_write;
    REG_NO REG_dst;
    PL_SIGNAL REG_src;    // Selection: ALU_RES, MEM_RES, or PC_PLUS_4
    int valid;
} MEM_WB_Reg;
```



## 🚀 Execution Stage Details



The pipeline implementation separates the execution cycle into five sequential, non-overlapping stages. Each stage performs a specific set of operations and passes the result to the subsequent pipeline register.



### 1. Instruction Fetch (IF)



**Input:** `cpu.pc` (Program Counter)

**Output:** `IF_ID_Reg`

Description:

This stage is responsible for fetching the next instruction from the Instruction Memory and calculating the address of the next sequential instruction.

| **Operation**            | **Details**                                                  |
| ------------------------ | ------------------------------------------------------------ |
| **Fetch**                | Reads the 32-bit instruction (`inst`) from memory at the address specified by `cpu.pc`. |
| **PC Update**            | Calculates the predicted next PC (`cpu.pc + 4`).             |
| **Register Write**       | If the `PC_Write_Enable` control signal is set (i.e., no stall required), the instruction and PC are loaded into `IF_ID_Reg`. |
| **Control Hazard Check** | If a branch misprediction is detected (`Predict_Right == 0`), the pipeline is flushed by overriding `cpu.pc` with the correct target address, and the misprediction flag is reset. |



### 2. Instruction Decode (ID)



**Input:** `IF_ID_Reg`

**Output:** `ID_EX_Reg`

Description:

The instruction is decoded, register operands are read from the Register File, and control signals for the remaining stages are generated.

| **Operation**                          | **Details**                                                  |
| -------------------------------------- | ------------------------------------------------------------ |
| **Decode**                             | The instruction's fields (`opcode`, `funct3`, `rd`, `rs1`, `rs2`) are extracted, and the immediate value is generated (`Decode` structure). |
| **Control Signal Generation**          | All necessary control signals (`ALU_use`, `ALU_src1/src2`, `MEM_read/write`, `REG_write`, `REG_src`) are determined based on the instruction type. |
| **Operand Read**                       | The *indices* (`rs1`, `rs2`) are passed to the next stage. The *values* are generally handled later via forwarding or the EX stage. |
| **Data Hazard Check (Stall)**          | The stage checks for **RAW hazards** (Read After Write) where the current instruction requires a register (`rs1` or `rs2`) that is a destination (`REG_dst`) in the subsequent `EX_MEM_Reg` (Load-Use) or `MEM_WB_Reg`. If a *Load-Use* hazard is detected, the pipeline is stalled (`PC_Write_Enable = 0`, `IF_ID_Write_Enbale = 0`) and a **bubble** is inserted into `ID_EX_Reg` (`ID_EX_Bubble_Insert = 1`). |
| **Control Hazard Check (Jump/Branch)** | Preliminary check for Jumps and Branches to prepare for target calculation in EX. |



### 3. Execute (EX)



**Input:** `ID_EX_Reg`

**Output:** `EX_MEM_Reg`

Description:

The ALU performs the required operation, and branch/jump targets are calculated and resolved.

| **Operation**         | **Details**                                                  |
| --------------------- | ------------------------------------------------------------ |
| **Data Forwarding**   | This critical step occurs here: the actual operands (`op1`, `op2`) for the ALU are selected. If a register value is being written by an instruction in the MEM or WB stages, the result is *forwarded* directly from `EX_MEM_Reg` or `MEM_WB_Reg` instead of waiting for the Register File write. |
| **ALU Calculation**   | The ALU computes the result (`alu_result`), which could be:  |
|                       | - Arithmetic/Logic result (R-type, I-type)                   |
|                       | - Effective memory address (L-type, S-type)                  |
|                       | - Branch target address (B-type)                             |
| **Branch Resolution** | For branch instructions, the condition is evaluated. If the branch is taken and this contradicts the `predict_pc` from the IF stage, the `Predict_Right` flag is set to `0`, triggering a pipeline flush in the next IF stage. |
| **Pass Data**         | The `alu_result` and control signals are passed to `EX_MEM_Reg`. |



### 4. Memory Access (MEM)



**Input:** `EX_MEM_Reg`

**Output:** `MEM_WB_Reg`

Description:

This stage performs data memory access for load and store instructions.

| **Operation**                     | **Details**                                                  |
| --------------------------------- | ------------------------------------------------------------ |
| **Load Operation (`MEM_read`)**   | If the instruction is a Load, the stage reads data from the Data Memory using the effective address (`alu_result`) and stores the result in `mem_result`. |
| **Store Operation (`MEM_write`)** | If the instruction is a Store, the stage writes the value of `R[rs2]` (which was forwarded from the register file or a forwarding path) to the Data Memory at the effective address (`alu_result`). |
| **Pass Data**                     | Both `alu_result` and `mem_result` (if applicable) are passed along with the final write-back control signals to `MEM_WB_Reg`. |



### 5. Write-Back (WB)



**Input:** `MEM_WB_Reg`

**Output:** Register File

Description:

The final result is written back to the Register File, completing the instruction's execution.

| **Operation**          | **Details**                                                  |
| ---------------------- | ------------------------------------------------------------ |
| **Write Enable Check** | If `REG_write` is true (i.e., not a Store, Branch, or a Bubble), the write-back occurs. |
| **Data Selection**     | The value to be written is selected based on `REG_src`:      |
|                        | - `ALU_RES`: Uses `alu_result` (R-type, I-type arithmetic).  |
|                        | - `MEM_RES`: Uses `mem_result` (Load instructions).          |
|                        | - `PC_PLUS_4`: Uses `s.pc + 4` (JAL, JALR link address).     |
| **Register Write**     | The selected value is written to the destination register `R[REG_dst]`. The value of the zero register `R[0]` is explicitly reset to 0 after every cycle to maintain integrity. |

------