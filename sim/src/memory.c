#include <common.h>
#include <gelf.h>
#include <fcntl.h>
#include <unistd.h>
#include "ftrace.h"

extern uint8_t* mem;
extern SymbolTable *sym_table;

uint8_t* guest_to_host(uint64_t addr) {return mem + addr - MEM_BASE;}

static inline uint64_t host_read(void *addr, int len){
    switch(len){
        case 1: return *(uint8_t  *)addr;
        case 2: return *(uint16_t *)addr;
	    case 4: return *(uint32_t *)addr;
        case 8: return *(uint64_t *)addr;
        default: sentinel("Invalid len for read.");
    }
error:
    return 0;
}

static inline void host_write(void *addr, int len, uint64_t data){
    switch(len){
        case 1: *(uint8_t  *)addr = data; return;
        case 2: *(uint16_t *)addr = data; return;
        case 4: *(uint32_t *)addr = data; return;
        case 8: *(uint64_t *)addr = data; return;
        default: sentinel("Invalid len for write.");
    }
error:
    return;
}

uint32_t inst_fetch(uint64_t pc){
    check(pc, "PC is zero.");
    return (*(uint32_t *)guest_to_host(pc));
error:
    return 0;
}

uint64_t mem_read(uint64_t addr, int len){
    check(addr - MEM_BASE < MEM_SIZE, "Read addr %016lx out of bound.", addr);
    uint64_t ret = host_read(guest_to_host(addr), len);
    return ret;
error:
    return 0;
}

void mem_write(uint64_t addr, int len, uint64_t data){
    check(addr - MEM_BASE < MEM_SIZE, "Write addr %016lx out of bound.", addr);
    host_write(guest_to_host(addr), len, data);
error:
    return;
}

void load_image(char *filepath){
    log_info("Physical Memory Range:[%016x, %016x].", MEM_BASE, MEM_BASE + MEM_SIZE - 1);
    check(filepath[0] != '\0', "IMAGE file path wrong.");
    FILE *fp = fopen(filepath, "rb");
    check(fp, "Failed to read %s.", filepath);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    log_info("The image is %s, size = %ld.", filepath, size);
    fseek(fp, 0, SEEK_SET);
    int ret = fread(guest_to_host(MEM_BASE), size, 1, fp);
    check(ret == 1, "Load image failed.");
    fclose(fp);
    return;

error:
    if(fp) fclose(fp);
    return;
}

void load_elf_symbols(const char *file_path) {
    int fd;
    Elf *elf;
    GElf_Ehdr ehdr;
    init_symbol_table(sym_table);

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library initialization failed: %s\n", elf_errmsg(-1));
        exit(1);
    }

    if ((fd = open(file_path, O_RDONLY, 0)) < 0) {
        perror("open");
        exit(1);
    }

    if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
        fprintf(stderr, "elf_begin() failed: %s\n", elf_errmsg(-1));
        exit(1);
    }

    if (gelf_getehdr(elf, &ehdr) == NULL) {
        fprintf(stderr, "gelf_getehdr() failed: %s\n", elf_errmsg(-1));
        exit(1);
    }

    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        gelf_getshdr(scn, &shdr);

        if (shdr.sh_type == SHT_SYMTAB) {
            Elf_Data *data = elf_getdata(scn, NULL);
            int count = shdr.sh_size / shdr.sh_entsize;

            for (int i = 0; i < count; ++i) {
                GElf_Sym sym;
                gelf_getsym(data, i, &sym);

                if (GELF_ST_TYPE(sym.st_info) == STT_FUNC) {
                    const char *name = elf_strptr(elf, shdr.sh_link, sym.st_name);
                    if (sym_table) {
                        add_func_symbol(sym_table, sym.st_value, name);
                    }
                }
            }
        }
    }

    elf_end(elf);
    close(fd);
}