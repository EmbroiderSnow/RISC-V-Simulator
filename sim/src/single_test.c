#include <common.h>
#include <memory.h>
#include <cpu.h>
#include <inst_test.h>

uint8_t *mem = NULL;

int main(int argc, char *argv[]) {
    mem = (uint8_t *)malloc(MEM_SIZE);
    // check_mem(mem);
    memset(mem, 0, MEM_SIZE);
    test_lui();
    return 0;
}