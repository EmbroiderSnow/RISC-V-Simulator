#include <common.h>
#include <decode.h>
#include <memory.h>

CPU_state cpu = {};
static int running = 1;
uint64_t global_cycle_count = 0;
uint64_t ninst = 0;

static inline void push_stage(Decode *s, CPU_Cycle_Stage *stage) {
    switch (*stage) {
        case STAGE_IF:
            *stage = STAGE_ID;
            break;
        case STAGE_ID:
            switch (s->type) 
            {
                case TYPE_R:
                case TYPE_I:
                case TYPE_S:
                case TYPE_B:
                case TYPE_U:
                    *stage = STAGE_EX;
                    break;
                case TYPE_J:
                    *stage = STAGE_WB;
                    break;
                default:
                    *stage = STAGE_DONE;
                    break; 
            }
            break;
        case STAGE_EX:
            switch (s->type)
            {
                case TYPE_R:
                    *stage = STAGE_WB;
                    break;
                case TYPE_I:
                    if (s->is_load) {
                        *stage = STAGE_MEM;
                    } else {
                        *stage = STAGE_WB;
                    }
                    break;
                case TYPE_S:
                    *stage = STAGE_MEM;
                    break;
                case TYPE_B:
                    *stage = STAGE_DONE;
                    break;
                case TYPE_U:
                    *stage = STAGE_WB;
                    break;
                default:
                    break;
            }
            break;
        case STAGE_MEM:
            switch (s->type)
            {
                case TYPE_I:
                    *stage = STAGE_WB;
                    break;
                case TYPE_S:
                    *stage = STAGE_DONE;
                    break;
                default:
                    break;
            }
            break;
        case STAGE_WB:
            *stage = STAGE_DONE;
            break;
        default:
            break;
    }
}

void init_cpu(){
    cpu.pc = MEM_BASE;
    memset(cpu.reg, 0, sizeof(cpu.reg));
}

void exec_once(){
    ++ninst;
    Decode s;
    CPU_Cycle_Stage stage = STAGE_IF;
    uint64_t alu_result = 0;
    uint64_t mem_result = 0;
    while (stage != STAGE_DONE) {
        switch (stage) {
            case STAGE_IF:
                s.pc = cpu.pc;
                s.inst = inst_fetch(s.pc);
                s.snpc = s.pc + 4;
                s.dnpc = s.snpc;
                push_stage(&s, &stage);
                global_cycle_count++;
                break;
            case STAGE_ID:
                // add cycle count in func decode_ID
                decode_ID(&s);
                push_stage(&s, &stage);
                break;
            case STAGE_EX:
                // alu_result used to pass result to MEM or WB stage
                // add cycle count in func decode_EX
                decode_EX(&s, &alu_result);
                push_stage(&s, &stage);
                break;
            case STAGE_MEM:
                // mem_result used to pass result to WB stage
                // add cycle count in func decode_MEM
                decode_MEM(&s, alu_result, &mem_result);
                push_stage(&s, &stage);
                break;
            case STAGE_WB:
                decode_WB(&s, alu_result, mem_result);
                push_stage(&s, &stage);
                break;
            case STAGE_DONE:
                printf("INST END\n");
                goto loop_end;
        }
    }
loop_end:
    cpu.pc = s.dnpc;
}

void exec_single_inst(uint32_t inst) {
    Decode s;
    s.inst = inst;
    decode_exec(&s);
}

void cpu_exec(){
    while(running){
        exec_once();
    }
}

void halt_trap(uint64_t pc, uint64_t code){
    if(code){
        printf(ANSI_FMT("HIT BAD TRAP!\n", ANSI_FG_RED));
    }else{
        printf(ANSI_FMT("HIT GOOD TRAP!\n", ANSI_FG_GREEN));
    }
    log_info("Program ended at pc %08lx, with exit code %ld.", pc, code);
    printf(ANSI_FMT("Performance: \n\tINST NUM  = %4ld\n\tCYCLE NUM = %4ld\n\tCPI       = %.3f\n", ANSI_FG_YELLOW), ninst, global_cycle_count, (float)global_cycle_count/(float)ninst);
    running = 0;
}
