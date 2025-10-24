#include <common.h>
#include <memory.h>
#include <cpu.h>
#include <disasm.h>
#include "ftrace.h"

uint8_t *mem = NULL;
int itrace_enabled = 0;
int ftrace_enabled = 0;
SymbolTable *sym_table = NULL;
LLVMDisasmContextRef disasm_ctx;

const char *help_string = "Usage: Simulator <model> <img_file> [options]\n"
                                  "Models:\n"
                                  "  iss    Instruction Set Simulator\n"
                                  "  mc     Multi-cycle Performance Simulator\n"
                                  "  pl     Pipeline Performance Simulator\n";

int run_iss_model(int argc, char *argv[]);
int run_mc_model(int argc, char *argv[]);
int run_pl_model(int argc, char *argv[]);

int main(int argc, char *argv[]){
    if (argc < 3) {
        printf("%s\n", help_string);
    }

    char *model = argv[1];

    if (strcmp(model, "iss") == 0) {
        run_iss_model(argc - 1, argv + 1);
    }
    else if (strcmp(model, "mc") == 0) {
        run_mc_model(argc - 1, argv + 1);
    }
    else if (strcmp(model, "pl") == 0) {
        run_pl_model(argc - 1, argv + 1);
    }
    else {
        printf("Error: Unknown model '%s'.\n", model);
        return -1;
    }

    return 0;
}

int run_iss_model(int argc, char *argv[]) {
    mem = (uint8_t *)malloc(MEM_SIZE);
    check_mem(mem);
    memset(mem, 0, MEM_SIZE);
    
    char image_file[128] = "";
    sprintf(image_file, "test/build/%s.bin", argv[1]);
    load_image(image_file);

    char elf_file[128] = "";
    sprintf(elf_file, "test/build/%s.elf", argv[1]);
    sym_table = malloc(sizeof(SymbolTable));
    load_elf_symbols(elf_file);
    sort_symbols_by_address(sym_table);

    init_cpu();
    if (argc > 2 && strcmp(argv[2], "--debug") == 0) {
        init_llvm_disassembler();
        debug_loop();
    }
    else if (argc > 2 && strcmp(argv[2], "--batch") == 0) {
        iss_cpu_exec();
    }
    else if (argc > 2 && strcmp(argv[2], "--itrace") == 0) {
        itrace_enabled = 1;
        init_llvm_disassembler();
        iss_cpu_exec();
    }
    else if (argc > 2 && strcmp(argv[2], "--ftrace") == 0) {
        ftrace_enabled = 1;
        iss_cpu_exec();
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

int run_mc_model(int argc, char *argv[]) {
    mem = (uint8_t *)malloc(MEM_SIZE);
    check_mem(mem);
    memset(mem, 0, MEM_SIZE);
    
    char image_file[128] = "";
    sprintf(image_file, "test/build/%s.bin", argv[1]);
    load_image(image_file);
 
    if (argc > 2 && strcmp(argv[2], "--itrace") == 0) {
        itrace_enabled = 1;
        init_llvm_disassembler();
    }

    init_cpu();
    mc_cpu_exec();

    free(mem);
    return 0;

error:
    if(mem) free(mem);
    return -1;
}

int run_pl_model(int argc, char *argv[]) {
    mem = (uint8_t *)malloc(MEM_SIZE);
    check_mem(mem);
    memset(mem, 0, MEM_SIZE);
    
    char image_file[128] = "";
    sprintf(image_file, "test/build/%s.bin", argv[1]);
    load_image(image_file);
 
    if (argc > 2 && strcmp(argv[2], "--itrace") == 0) {
        itrace_enabled = 1;
        init_llvm_disassembler();
    }

    init_cpu();
    pl_cpu_exec();

    free(mem);
    return 0;

error:
    if(mem) free(mem);
    return -1;
}