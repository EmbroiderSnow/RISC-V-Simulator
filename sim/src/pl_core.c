#include <pl_core.h>
#include <stdbool.h>
#include <cpu.h>
#include <disasm.h>

IF_ID_Reg if_id_reg = {0};
ID_EX_Reg id_ex_reg = {0};
EX_MEM_Reg ex_mem_reg = {0};
MEM_WB_Reg mem_wb_reg = {0};

bool PC_Write_Enable = 1;
bool IF_ID_Write_Enbale = 1;
bool ID_EX_Bubble_Insert = 0;
bool Predict_Right = 1;

int RAW_harzard_count = 0;
int control_harzard_count = 0;

extern CPU_state cpu;
extern int itrace_enabled;
extern int ftrace_enabled;
extern LLVMDisasmContextRef disasm_ctx;
extern int ninst;
extern int global_cycle_count;

static bool last_inst_is_div;

static inline void reg_use(uint64_t inst, int *use_rs1, int *use_rs2);
static inline bool check_read_after_write_hazard(
    bool is_writing, int write_dst,
    bool use_rs1, int rs1_idx,
    bool use_rs2, int rs2_idx);
static inline bool inst_fusion(Decode *s);

void init_pipeline() {
    memset(&if_id_reg, 0, sizeof(IF_ID_Reg));
    memset(&id_ex_reg, 0, sizeof(ID_EX_Reg));
    memset(&ex_mem_reg, 0, sizeof(EX_MEM_Reg));
    memset(&mem_wb_reg, 0, sizeof(MEM_WB_Reg));
    PC_Write_Enable = true;
    IF_ID_Write_Enbale = true;
    Predict_Right = true;
}

void pl_IF()
{
    if (!Predict_Right) {
        if_id_reg.valid = 0;
    }
    if (PC_Write_Enable && IF_ID_Write_Enbale)
    {
        // printf("IF: ");
        Decode *s = &if_id_reg.s;
        s->pc = cpu.pc;
        s->inst = inst_fetch(s->pc);
        s->snpc = s->pc + 4;
        s->dnpc = s->snpc;
        // if (itrace_enabled)
        //     handle_itrace(s);
        // printf("\n");
        cpu.pc = s->pc + 4;
        if_id_reg.predict_pc = s->pc + 4;
        if_id_reg.valid = 1;
    }
    else {
        // printf("IF: not fetch, state:\n PC_Write_Enable: %d, IF_ID_Write_Enable: %d, Predict_Right: %d\n\n", PC_Write_Enable, IF_ID_Write_Enbale, Predict_Right);
    }
}

void pl_ID()
{
    if (!Predict_Right) {
        // printf("ID: Clear\n");
        PC_Write_Enable = false;
        IF_ID_Write_Enbale = false;
        id_ex_reg.valid = 0;
        return;
    }
    // printf("ID: ");
    // if (if_id_reg.valid)
    //     handle_itrace(&if_id_reg.s);
    // else 
    //     printf("Bubble\n");
    // read rs1 & rs2 from inst
    uint64_t i = if_id_reg.s.inst;
    REG_NO rd = BITS(i, 11, 7);
    REG_NO rs1 = BITS(i, 19, 15);
    REG_NO rs2 = BITS(i, 24, 20);

    // does this instruction use rs1 / rs2?
    int use_rs1, use_rs2;
    reg_use(i, &use_rs1, &use_rs2);

    // harzard check

    bool ex_is_writing = ex_mem_reg.valid &&
                          ex_mem_reg.REG_write &&
                          (ex_mem_reg.REG_dst != 0);
    int ex_rd_dst = ex_mem_reg.REG_dst;

    bool mem_is_writing = mem_wb_reg.valid &&
                            mem_wb_reg.REG_write &&
                            (mem_wb_reg.REG_dst != 0);
    int mem_rd_dst = mem_wb_reg.REG_dst;

    bool hazard_EX = check_read_after_write_hazard(
        ex_is_writing, ex_rd_dst,
        use_rs1, rs1, use_rs2, rs2);

    bool hazard_MEM = check_read_after_write_hazard(
        mem_is_writing, mem_rd_dst,
        use_rs1, rs1, use_rs2, rs2);

    // harzard? lock PC and IF_ID_Reg
    if (hazard_EX || hazard_MEM)
    {
        ++RAW_harzard_count;
        // if (hazard_EX)
        //     printf("harzard: EX and ID\n");
        // if (hazard_MEM)
        //     printf("harzard: MEM and ID\n");
        PC_Write_Enable = false;
        IF_ID_Write_Enbale = false;
        // insert a bubble
        id_ex_reg.valid = 0;
    }
    // harzard resolved. exec normally
    else
    {
        PC_Write_Enable = true;
        IF_ID_Write_Enbale = true;
        // decode logic
        ID_EX_Reg *p = &id_ex_reg;
        p->s = if_id_reg.s;
        p->predict_pc = if_id_reg.predict_pc;
        p->valid = if_id_reg.valid;
        switch (get_inst_type(i))
        {
        case TYPE_R:
            p->ALU_use = USE_ALU;
            p->ALU_src1 = RS1;
            p->ALU_src2 = RS2;
            p->rs1 = rs1;
            p->rs2 = rs2;
            p->MEM_read = NOT_READ_MEM;
            p->MEM_write = NOT_WRITE_MEM;
            p->REG_write = WRITE_REG;
            p->REG_dst = rd;
            p->REG_src = ALU_RES;
            break;
        case TYPE_I:
            if (is_load(i))
            {
                p->ALU_use = USE_ALU;
                p->ALU_src1 = RS1;
                p->ALU_src2 = IMM;
                p->rs1 = rs1;
                p->rs2 = NOT_CARE;
                p->MEM_read = READ_MEM;
                p->MEM_write = NOT_WRITE_MEM;
                p->REG_write = WRITE_REG;
                p->REG_dst = rd;
                p->REG_src = MEM_RES;
            }
            else if ((i & 0x7F) == 0b1100111)
            { // jalr
                p->ALU_use = USE_ALU;
                p->ALU_src1 = RS1;
                p->ALU_src2 = IMM;
                p->rs1 = rs1;
                p->rs2 = NOT_CARE;
                p->MEM_read = 0;
                p->MEM_write = 0;
                p->REG_write = WRITE_REG;
                p->REG_dst = rd;
                p->REG_src = PC_PLUS_4;
            }
            else if ((i & 0x7F) == 0b1110011 || (i & 0x7F) == 0b0001111)
            { // SYSTEM (ecall/ebreak) or FENCE
                p->ALU_use = NOT_USE_ALU;
                p->ALU_src1 = NOT_CARE;
                p->ALU_src2 = NOT_CARE;
                p->rs1 = NOT_CARE;
                p->rs2 = NOT_CARE;
                p->MEM_read = NOT_READ_MEM;
                p->MEM_write = NOT_WRITE_MEM;
                p->REG_write = NOT_WRITE_REG;
                p->REG_dst = NOT_CARE;
                p->REG_src = NOT_CARE;
            }
            else
            {
                p->ALU_use = USE_ALU;
                p->ALU_src1 = RS1;
                p->ALU_src2 = IMM;
                p->rs1 = rs1;
                p->rs2 = NOT_CARE;
                p->MEM_read = NOT_READ_MEM;
                p->MEM_write = NOT_WRITE_MEM;
                p->REG_write = WRITE_REG;
                p->REG_dst = rd;
                p->REG_src = ALU_RES;
            }
            break;
        case TYPE_S:
            p->ALU_use = USE_ALU;
            p->ALU_src1 = RS1;
            p->ALU_src2 = IMM;
            p->rs1 = rs1;
            p->rs2 = rs2;
            p->MEM_read = NOT_READ_MEM;
            p->MEM_write = WRITE_MEM;
            p->REG_write = NOT_WRITE_REG;
            p->REG_dst = NOT_CARE;
            p->REG_src = NOT_CARE;
            break;
        case TYPE_B:
            p->ALU_use = USE_ALU;
            p->ALU_src1 = RS1;
            p->ALU_src2 = RS2;
            p->rs1 = rs1;
            p->rs2 = rs2;
            p->MEM_read = NOT_READ_MEM;
            p->MEM_write = NOT_WRITE_MEM;
            p->REG_write = NOT_WRITE_REG;
            p->REG_dst = NOT_CARE;
            p->REG_src = NOT_CARE;
            break;
        case TYPE_U:
            if ((i & 0x7F) == 0b0010111)
            { // auipc
                p->ALU_use = USE_ALU;
                p->ALU_src1 = SELECT_PC;
                p->ALU_src2 = IMM;
                p->rs1 = NOT_CARE;
                p->rs2 = NOT_CARE;
                p->MEM_read = NOT_READ_MEM;
                p->MEM_write = NOT_WRITE_MEM;
                p->REG_write = WRITE_REG;
                p->REG_dst = rd;
                p->REG_src = ALU_RES;
            }
            else
            { // lui
                p->ALU_use = USE_ALU;
                p->ALU_src1 = REG_ZERO;
                p->ALU_src2 = IMM;
                p->rs1 = NOT_CARE;
                p->rs2 = NOT_CARE;
                p->MEM_read = NOT_READ_MEM;
                p->MEM_write = NOT_WRITE_MEM;
                p->REG_write = WRITE_REG;
                p->REG_dst = rd;
                p->REG_src = ALU_RES;
            }
            break;
        case TYPE_J:
            p->ALU_use = USE_ALU;
            p->ALU_src1 = SELECT_PC;
            p->ALU_src2 = IMM;
            p->rs1 = NOT_CARE;
            p->rs2 = NOT_CARE;
            p->MEM_read = NOT_READ_MEM;
            p->MEM_write = NOT_WRITE_MEM;
            p->REG_write = WRITE_REG;
            p->REG_dst = rd;
            p->REG_src = PC_PLUS_4;
            break;
        default:
            break;
        }
    }
}

void pl_EX()
{
    // bubble
    if (!id_ex_reg.valid) {
        // printf("EX: Bubble\n");
        ex_mem_reg.valid = 0;
        return;
    }

    // printf("EX: ");
    // handle_itrace(&id_ex_reg.s);

    ++ninst;
    int rd = 0;
    uint64_t src1 = 0, src2 = 0, imm = 0;
    Decode *s = &id_ex_reg.s;
    uint64_t *alu_result = &ex_mem_reg.alu_result;


    INSTPAT_START();
    // RV64I
    INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, *alu_result = imm + 0);
    INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, *alu_result = s->pc + imm);
    INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, s->dnpc = (src1 + imm) & ~1);
    // BEQ, BNE, BLT, BGE, BLTU, BGEU
    INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B, if (src1 == src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne, B, if (src1 != src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B, if ((int64_t)src1 < (int64_t)src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, if ((int64_t)src1 >= (int64_t)src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu, B, if (src1 < src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, if (src1 >= src2) s->dnpc = s->pc + imm);
    // LB, LH, LW, LBU, LHU, LWU, LD
    INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 110 ????? 00000 11", lwu, I, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld, I, *alu_result = src1 + imm);
    // SB, SH, SW
    INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd, S, *alu_result = src1 + imm);
    INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, *alu_result = src1 + imm);
    // SLTI, SLTIU
    INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I, *alu_result = (int64_t)src1 < (int64_t)imm);
    INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, *alu_result = src1 < imm);
    // SLLI, SRLI, SRAI, SLLIW, SRLIW, SRAIW
    INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli, I, *alu_result = src1 << (imm & 0x3f));
    INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli, I, *alu_result = src1 >> (imm & 0x3f));
    INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai, I, *alu_result = (int64_t)src1 >> (imm & 0x3f));
    INSTPAT("0000000 ????? ????? 001 ????? 00110 11", slliw, I, *alu_result = SEXT((uint32_t)src1 << (imm & 0x1f), 32));
    INSTPAT("0000000 ????? ????? 101 ????? 00110 11", srliw, I, *alu_result = SEXT((uint32_t)src1 >> (imm & 0x1f), 32));
    INSTPAT("0100000 ????? ????? 101 ????? 00110 11", sraiw, I, *alu_result = SEXT((int32_t)src1 >> (imm & 0x1f), 32));
    // XORI, ORI, ANDI, ADDIW
    INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, *alu_result = src1 ^ imm);
    INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, *alu_result = src1 | imm);
    INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, *alu_result = src1 & imm);
    INSTPAT("??????? ????? ????? 000 ????? 00110 11", addiw, I, *alu_result = SEXT((int32_t)src1 + (int32_t)imm, 32));
    // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
    INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, *alu_result = src1 + src2);
    INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, *alu_result = src1 - src2);
    INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, R, *alu_result = src1 << (src2 & 0x3f));
    INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, R, *alu_result = (int64_t)src1 < (int64_t)src2);
    INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R, *alu_result = src1 < src2);
    INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R, *alu_result = src1 ^ src2);
    INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl, R, *alu_result = src1 >> (src2 & 0x3f));
    INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra, R, *alu_result = (int64_t)src1 >> (src2 & 0x3f));
    INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R, *alu_result = src1 | src2);
    INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, R, *alu_result = src1 & src2);
    // ADDW, SUBW, SLLW, SRLW, SRAW
    INSTPAT("0000000 ????? ????? 000 ????? 01110 11", addw, R, *alu_result = SEXT((uint32_t)src1 + (uint32_t)src2, 32));
    INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw, R, *alu_result = SEXT((uint32_t)src1 - (uint32_t)src2, 32));
    INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw, R, *alu_result = SEXT((uint32_t)src1 << (src2 & 0x1f), 32));
    INSTPAT("0000000 ????? ????? 101 ????? 01110 11", srlw, R, *alu_result = SEXT((uint32_t)src1 >> (src2 & 0x1f), 32));
    INSTPAT("0100000 ????? ????? 101 ????? 01110 11", sraw, R, *alu_result = SEXT((int32_t)src1 >> (src2 & 0x1f), 32));
    // FENCE, FENCE.I
    INSTPAT("0000??? ????? 00000 000 00000 00011 11", fence, I, NOP);
    INSTPAT("0000000 00000 00000 000 00000 00011 11", fencei, I, NOP);
    // ECALL
    INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, I, HALT(s->pc, R(10))); // R(10) is $a0
    // CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI

    // RV64M
    // MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU
    INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul, R, *alu_result = src1 * src2;);
    INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh, R, *alu_result = (int64_t)((__int128_t)(int64_t)src1 * (__int128_t)(int64_t)src2 >> 64));
    INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu, R, *alu_result = (int64_t)((__int128_t)(int64_t)src1 * (__int128_t)(uint64_t)src2 >> 64));
    INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu, R, *alu_result = (uint64_t)((__uint128_t)(uint64_t)src1 * (__uint128_t)(uint64_t)src2 >> 64));
    INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div, R, if (src2 == 0) *alu_result = -1; else *alu_result = (int64_t)src1 / (int64_t)src2; global_cycle_count += 39);
    INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu, R, if (src2 == 0) *alu_result = -1; else *alu_result = src1 / src2; global_cycle_count += 39);
    INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem, R, if (src2 == 0) *alu_result = src1; else *alu_result = (int64_t)src1 % (int64_t)src2; global_cycle_count += inst_fusion(s) ? -1 : 39);
    INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu, R, if (src2 == 0) *alu_result = src1; else *alu_result = src1 % src2; global_cycle_count += inst_fusion(s) ? -1 : 39);

    // MULW, DIVW, DIVUW, REMW, REMUW
    INSTPAT("0000001 ????? ????? 000 ????? 01110 11", mulw, R, *alu_result = SEXT((int32_t)src1 * (int32_t)src2, 32));
    INSTPAT("0000001 ????? ????? 100 ????? 01110 11", divw, R, if (src2 == 0) *alu_result = -1; else *alu_result = SEXT((int32_t)(int64_t)src1 / (int32_t)(int64_t)src2, 32); global_cycle_count += 39);
    INSTPAT("0000001 ????? ????? 101 ????? 01110 11", divuw, R, if (src2 == 0) *alu_result = -1; else *alu_result = SEXT((uint32_t)(uint64_t)src1 / (uint32_t)(uint64_t)src2, 32); global_cycle_count += 39);
    INSTPAT("0000001 ????? ????? 110 ????? 01110 11", remw, R, if (src2 == 0) *alu_result = SEXT((uint32_t)src1, 32); else *alu_result = SEXT((int32_t)(int64_t)src1 % (int32_t)(int64_t)src2, 32); global_cycle_count += inst_fusion(s) ? -1 : 39);
    INSTPAT("0000001 ????? ????? 111 ????? 01110 11", remuw, R, if (src2 == 0) *alu_result = SEXT((uint32_t)src1, 32); else *alu_result = SEXT((uint32_t)(uint64_t)src1 % (uint32_t)(uint64_t)src2, 32); global_cycle_count += inst_fusion(s) ? -1 : 39);

    // Invalid Opcode
    INSTPAT("??????? ????? ????? ??? ????? ????? ??", unk, N, printf(ANSI_FMT("[Stage EX]Unknown Inst!\n", ANSI_FG_RED)), HALT(s->pc, -1));
    INSTPAT_END();

    R(0) = 0;

    Predict_Right = (s->dnpc == id_ex_reg.predict_pc);
    if (!Predict_Right) {
        ++control_harzard_count;
        cpu.pc = s->dnpc;
        // printf("INST 0x%08x find mis-predict:\n Predict addr: 0x%08lx\n Actural addr: 0x%08lx\n", s->inst, id_ex_reg.predict_pc, s->dnpc);
    }
        


/*
typedef struct {
    Decode s;
    PL_SIGNAL MEM_read; // Does this instruction read memory? no: 0, yes: 1
    PL_SIGNAL MEM_write; // Does this instruction write memory? no: 0, yes: 1
    REG_NO rs2;
    uint64_t alu_result;
    PL_SIGNAL REG_write; // Does this instruction write register? no: 0, yes: 1
    REG_NO REG_dst; // Which register does this instruction write to? (equals to rd, maybe)
    PL_SIGNAL REG_src; // Where is the value written to the register read from? alu_result: 0, mem_result: 1, pc+4: 2
    int valid;
} EX_MEM_Reg;
*/
    ex_mem_reg.s = id_ex_reg.s;
    ex_mem_reg.MEM_read = id_ex_reg.MEM_read;
    ex_mem_reg.MEM_write = id_ex_reg.MEM_write;
    ex_mem_reg.rs2 = id_ex_reg.rs2;
    ex_mem_reg.REG_write = id_ex_reg.REG_write;
    ex_mem_reg.REG_dst = id_ex_reg.REG_dst;
    ex_mem_reg.REG_src = id_ex_reg.REG_src;
    ex_mem_reg.valid = id_ex_reg.valid;

    return;
}

void pl_MEM()
{
    if (!ex_mem_reg.valid) {
        // printf("MEM: Bubble\n");
        mem_wb_reg.valid = 0;
        return;
    }
    // printf("MEM: ");
    // handle_itrace(&ex_mem_reg.s);
    Predict_Right = true;
    last_inst_is_div = false;
    uint64_t *mem_result = &mem_wb_reg.mem_result;
    Decode *s = &ex_mem_reg.s;
    if (ex_mem_reg.MEM_read == READ_MEM || ex_mem_reg.MEM_write == WRITE_MEM)
    {
        int rd = 0;
        uint64_t src1 = 0, src2 = 0, imm = 0;
        uint64_t alu_result = ex_mem_reg.alu_result;

        INSTPAT_START();
        // RV64I
        //  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);
        //  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
        //  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->pc + 4, s->dnpc = s->pc + imm);
        //  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->pc + 4, s->dnpc = (src1 + imm) & ~1);
        //  BEQ, BNE, BLT, BGE, BLTU, BGEU
        //  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, if (src1 == src2) s->dnpc = s->pc + imm);
        //  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, if (src1 != src2) s->dnpc = s->pc + imm);
        //  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, if ((int64_t)src1 < (int64_t)src2) s->dnpc = s->pc + imm);
        //  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, if ((int64_t)src1 >= (int64_t)src2) s->dnpc = s->pc + imm);
        //  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, if (src1 < src2) s->dnpc = s->pc + imm);
        //  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, if (src1 >= src2) s->dnpc = s->pc + imm);
        //  LB, LH, LW, LBU, LHU, LWU, LD
        INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, *mem_result = SEXT(Mr(alu_result, 1), 8));
        INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, *mem_result = SEXT(Mr(alu_result, 2), 16));
        INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, *mem_result = SEXT(Mr(alu_result, 4), 32));
        INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, *mem_result = Mr(alu_result, 1));
        INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, *mem_result = Mr(alu_result, 2));
        INSTPAT("??????? ????? ????? 110 ????? 00000 11", lwu, I, *mem_result = Mr(alu_result, 4));
        INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld, I, *mem_result = Mr(alu_result, 8));
        // SB, SH, SW
        INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(alu_result, 1, src2));
        INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, Mw(alu_result, 2, src2));
        INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(alu_result, 4, src2));
        INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd, S, Mw(alu_result, 8, src2));
        // INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
        // SLTI, SLTIU
        // INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = (int64_t)src1 < (int64_t)imm);
        // INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = src1 < imm);
        // SLLI, SRLI, SRAI, SLLIW, SRLIW, SRAIW
        // INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << (imm & 0x3f));
        // INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = src1 >> (imm & 0x3f));
        // INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (int64_t)src1 >> (imm & 0x3f));
        // INSTPAT("0000000 ????? ????? 001 ????? 00110 11", slliw  , I, R(rd) = SEXT((uint32_t)src1 << (imm & 0x1f), 32));
        // INSTPAT("0000000 ????? ????? 101 ????? 00110 11", srliw  , I, R(rd) = SEXT((uint32_t)src1 >> (imm & 0x1f), 32));
        // INSTPAT("0100000 ????? ????? 101 ????? 00110 11", sraiw  , I, R(rd) = SEXT((int32_t)src1 >> (imm & 0x1f), 32));
        // XORI, ORI, ANDI, ADDIW
        // INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);
        // INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
        // INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);
        // INSTPAT("??????? ????? ????? 000 ????? 00110 11", addiw  , I, R(rd) = SEXT((int32_t)src1 + (int32_t)imm, 32));
        // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
        // INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
        // INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
        // INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << (src2 & 0x3f));
        // INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = (int64_t)src1 < (int64_t)src2);
        // INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = src1 < src2);
        // INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);
        // INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = src1 >> (src2 & 0x3f));
        // INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (int64_t)src1 >> (src2 & 0x3f));
        // INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
        // INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);
        // ADDW, SUBW, SLLW, SRLW, SRAW
        // INSTPAT("0000000 ????? ????? 000 ????? 01110 11", addw   , R, R(rd) = SEXT((uint32_t)src1 + (uint32_t)src2, 32));
        // INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw   , R, R(rd) = SEXT((uint32_t)src1 - (uint32_t)src2, 32));
        // INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw   , R, R(rd) = SEXT((uint32_t)src1 << (src2 & 0x1f), 32));
        // INSTPAT("0000000 ????? ????? 101 ????? 01110 11", srlw   , R, R(rd) = SEXT((uint32_t)src1 >> (src2 & 0x1f), 32));
        // INSTPAT("0100000 ????? ????? 101 ????? 01110 11", sraw   , R, R(rd) = SEXT((int32_t)src1 >> (src2 & 0x1f), 32));
        // FENCE, FENCE.I
        // INSTPAT("0000??? ????? 00000 000 00000 00011 11", fence  , N, NOP);
        // INSTPAT("0000000 00000 00000 000 00000 00011 11", fencei , N, NOP);
        // ECALL
        // INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, HALT(s->pc, R(10))); // R(10) is $a0
        // CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI

        // RV64M
        // MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU
        // INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1 * src2);
        // INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = (int64_t)((__int128_t)(int64_t)src1 * (__int128_t)(int64_t)src2 >> 64));
        // INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu , R, R(rd) = (int64_t)((__int128_t)(int64_t)src1 * (__int128_t)(uint64_t)src2 >> 64));
        // INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R, R(rd) = (uint64_t)((__uint128_t)(uint64_t)src1 * (__uint128_t)(uint64_t)src2 >> 64));
        INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, last_inst_is_div = true);
        INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, last_inst_is_div = true);
        // INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, if (src2 == 0) R(rd) = src1; else R(rd) = (int64_t)src1 % (int64_t)src2);
        // INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, if (src2 == 0) R(rd) = src1; else R(rd) = src1 % src2);

        // MULW, DIVW, DIVUW, REMW, REMUW
        // INSTPAT("0000001 ????? ????? 000 ????? 01110 11", mulw   , R, R(rd) = SEXT((int32_t)src1 * (int32_t)src2, 32));
        INSTPAT("0000001 ????? ????? 100 ????? 01110 11", divw   , R, last_inst_is_div = true);
        INSTPAT("0000001 ????? ????? 101 ????? 01110 11", divuw  , R, last_inst_is_div = true);
        // INSTPAT("0000001 ????? ????? 110 ????? 01110 11", remw   , R, if (src2 == 0) R(rd) = SEXT((uint32_t)src1, 32); else R(rd) = SEXT((int32_t)(int64_t)src1 % (int32_t)(int64_t)src2, 32));
        // INSTPAT("0000001 ????? ????? 111 ????? 01110 11", remuw  , R, if (src2 == 0) R(rd) = SEXT((uint32_t)src1, 32); else R(rd) = SEXT((uint32_t)(uint64_t)src1 % (uint32_t)(uint64_t)src2, 32));

        // Invalid Opcode
        INSTPAT("??????? ????? ????? ??? ????? ????? ??", unk, N, printf(ANSI_FMT("[Stage MEM]Unknown Inst!\n", ANSI_FG_RED)), HALT(s->pc, -1));
        INSTPAT_END();

        R(0) = 0;
    }
/*
typedef struct {
    Decode s;
    uint64_t alu_result;
    uint64_t mem_result;
    PL_SIGNAL REG_write; // Does this instruction write register? no: 0, yes: 1
    REG_NO REG_dst; // Which register does this instruction write to? (equals to rd, maybe)
    PL_SIGNAL REG_src; // Where is the value written to the register read from? alu_result: 0, mem_result: 1, pc+4: 2
    int valid;
} MEM_WB_Reg;
*/
    mem_wb_reg.alu_result = ex_mem_reg.alu_result;
    mem_wb_reg.s = ex_mem_reg.s;
    mem_wb_reg.REG_write = ex_mem_reg.REG_write;
    mem_wb_reg.REG_dst = ex_mem_reg.REG_dst;
    mem_wb_reg.REG_src = ex_mem_reg.REG_src;
    mem_wb_reg.valid = ex_mem_reg.valid;

    return;
}

void pl_WB()
{
    if (!mem_wb_reg.valid) {
        // printf("WB: Bubble\n");
        R(0) = 0;
        return;
    }
    // printf("WB: ");
    // handle_itrace(&mem_wb_reg.s);
    if (mem_wb_reg.REG_write)
    {
        switch (mem_wb_reg.REG_src)
        {
        case ALU_RES:
            R(mem_wb_reg.REG_dst) = mem_wb_reg.alu_result;
            break;
        case MEM_RES:
            R(mem_wb_reg.REG_dst) = mem_wb_reg.mem_result;
            break;
        case PC_PLUS_4:
            R(mem_wb_reg.REG_dst) = mem_wb_reg.s.pc + 4;
            break;
        default:
            break;
        }
    }

    R(0) = 0;
    return;
}

static inline void reg_use(uint64_t inst, int *use_rs1, int *use_rs2)
{
    switch (get_inst_type(inst))
    {
    case TYPE_R:
    case TYPE_S:
    case TYPE_B:
        *use_rs1 = 1;
        *use_rs2 = 1;
        break;
    case TYPE_I:
        *use_rs1 = 1;
        *use_rs2 = 0;
        break;
    case TYPE_U:
    case TYPE_J:
    default:
        *use_rs1 = 0;
        *use_rs2 = 0;
        break;
    }
}

static inline bool check_read_after_write_hazard(
    bool is_writing, int write_dst,
    bool use_rs1, int rs1_idx,
    bool use_rs2, int rs2_idx)
{
    if (!is_writing)
    {
        return false; 
    }

    bool conflict_rs1 = use_rs1 && (write_dst == rs1_idx);
    bool conflict_rs2 = use_rs2 && (write_dst == rs2_idx);

    return (conflict_rs1 || conflict_rs2);
}

static inline bool inst_fusion(Decode *s) {
    uint32_t i = s->inst;
    int rs1 = BITS(i, 19, 15);
    int rs2 = BITS(i, 24, 20);
    int rd  = BITS(i, 11,  7);
    i = mem_wb_reg.s.inst;
    int p_rs1 = BITS(i, 19, 15);
    int p_rs2 = BITS(i, 24, 20);
    int p_rd  = BITS(i, 11,  7);
    if (last_inst_is_div && (rs1 == p_rs1) && (rs2 == p_rs2) && (rd == p_rd))
        return true;
    return false;
}