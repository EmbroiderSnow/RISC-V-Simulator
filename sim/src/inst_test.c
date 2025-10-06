#include <common.h>
#include <memory.h>
#include <cpu.h>
#include <assert.h>
extern CPU_state cpu;

void test_lui() {
    uint32_t inst = 0x123453b7; // lui x7 0x12345
    init_cpu();
    exec_single_inst(inst);
    assert(cpu.reg[7] == 0x12345000);
    printf("LUI test passed!\n");
}
