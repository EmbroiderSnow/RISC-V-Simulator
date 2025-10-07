#include <common.h>
#include <memory.h>
#include <cpu.h>

uint8_t *mem = NULL;

int main(int argc, char *argv[]){
    mem = (uint8_t *)malloc(MEM_SIZE);
    check_mem(mem);
    memset(mem, 0, MEM_SIZE);
    load_image(argv[1]);
    init_cpu();
    if (argc > 2 && strcmp(argv[2], "-d") == 0) {
        // TODO: debug mode
    }
    else if (argc > 2 && strcmp(argv[2], "-b") == 9) {
        // batch mode
        cpu_exec();
    }
    else {
        // TODO: help message
    }
    free(mem);
    return 0;

error:
    if(mem) free(mem);
    return -1;
}
