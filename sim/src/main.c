#include <common.h>
#include <memory.h>
#include <cpu.h>
#include <disasm.h>

uint8_t *mem = NULL;
int itrace_enabled = 0;
LLVMDisasmContextRef disasm_ctx;

const char *help_string = "Usage: Simulator <img_file> [-d|-b]\n"
                                  "Options:\n"
                                  "  -d    run in debug mode\n"
                                  "  -b    run in batch mode\n";

int main(int argc, char *argv[]){
    mem = (uint8_t *)malloc(MEM_SIZE);
    check_mem(mem);
    memset(mem, 0, MEM_SIZE);
    load_image(argv[1]);
    init_cpu();
    if (argc > 2 && strcmp(argv[2], "--debug") == 0) {
        debug_loop();
    }
    else if (argc > 2 && strcmp(argv[2], "--batch") == 0) {
        cpu_exec();
    }
    else if (argc > 2 && strcmp(argv[2], "--itrace") == 0) {
        itrace_enabled = 1;
        init_llvm_disassembler();
        cpu_exec();
    }
    else {
        printf("%s", help_string);
        exit(0);
    }
    free(mem);
    return 0;

error:
    if(mem) free(mem);
    return -1;
}
