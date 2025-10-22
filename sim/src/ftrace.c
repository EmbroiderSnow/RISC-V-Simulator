#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpu.h>
#include <macro.h>
#include "ftrace.h" 

void init_symbol_table(SymbolTable *table) {
    if (!table) return;
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

void add_func_symbol(SymbolTable *table, uint64_t addr, const char *name) {
    if (!table || !name) return;

    if (table->count >= table->capacity) {
        int new_capacity = (table->capacity == 0) ? 16 : table->capacity * 2;
        FuncSymbol* new_symbols = realloc(table->symbols, new_capacity * sizeof(FuncSymbol));
        if (!new_symbols) {
            perror("Failed to reallocate symbol table");
            return;
        }
        table->symbols = new_symbols;
        table->capacity = new_capacity;
    }

    char *name_copy = strdup(name);
    if (!name_copy) {
        perror("Failed to duplicate symbol name");
        return;
    }

    table->symbols[table->count].address = addr;
    table->symbols[table->count].name = name_copy;
    table->count++;
}

static int compare_symbols(const void *a, const void *b) {
    FuncSymbol *sym_a = (FuncSymbol *)a;
    FuncSymbol *sym_b = (FuncSymbol *)b;
    if (sym_a->address < sym_b->address) return -1;
    if (sym_a->address > sym_b->address) return 1;
    return 0;
}

void sort_symbols_by_address(SymbolTable *table) {
    if (!table || table->count == 0) return;
    qsort(table->symbols, table->count, sizeof(FuncSymbol), compare_symbols);
}

const FuncSymbol* find_func(SymbolTable *table, uint64_t addr) {
    if (!table || table->count == 0) return NULL;

    int low = 0, high = table->count - 1;
    int best_match_idx = -1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (table->symbols[mid].address <= addr) {
            best_match_idx = mid; 
            low = mid + 1;        
        } else {
            high = mid - 1;       
        }
    }
    
    if (best_match_idx != -1) {
        return &(table->symbols[best_match_idx]);
    }

    return NULL;
}


void free_symbol_table(SymbolTable *table) {
    if (!table) return;
    for (int i = 0; i < table->count; ++i) {
        free(table->symbols[i].name); 
    }
    free(table->symbols); 
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

static int is_call(uint32_t inst) {
    uint32_t opcode = inst & 0x7f;
    uint32_t rd = (inst >> 7) & 0x1f;
    if (opcode == 0x6f && rd == 1) { // JAL && rd == x1 (ra)
        return 1;
    }
    if (opcode == 0x67 && rd == 1) { // JALR && rd == x1 (ra)
        return 1;
    }
    return 0;
}

static int is_ret(uint32_t inst) {
    uint32_t opcode = inst & 0x7f;
    uint32_t rs1 = (inst >> 15) & 0x1f;
    uint32_t rd = (inst >> 7) & 0x1f;
    uint32_t imm = (inst >> 20) & 0xfff; 
    if (opcode == 0x67 && rs1 == 1 && rd == 0 && imm == 0) { // JALR x0, 0(x1)
        return 1;
    }
    return 0;
}

extern CPU_state cpu;
extern int indentaton_level;
extern SymbolTable *sym_table;

static uint64_t get_call_target_addr(Decode *s) {
    uint32_t inst = s->inst;
    uint32_t opcode = inst & 0x7f;

    // J-type (jal)
    if (opcode == 0b1101111) {
        int32_t imm_j = SEXT(BITS(inst, 31, 31) << 20 | \
                                BITS(inst, 19, 12) << 12 | \
                                BITS(inst, 20, 20) << 11 | \
                                BITS(inst, 30, 21) << 1, 21);
        return s->pc + imm_j;
    }
    // I-type (jalr)
    else if (opcode == 0b1100111) {
        int rs1_idx = (inst >> 15) & 0x1f;
        int32_t imm_i = (int32_t)inst >> 20;
        return cpu.reg[rs1_idx] + imm_i;
    }

    return 0;
}

void handle_ftrace(Decode *s) {
    if (is_call(s->inst)) {
        uint64_t target_addr = get_call_target_addr(s);
        const FuncSymbol *func_symbol = find_func(sym_table, target_addr);
        const char *func_name = func_symbol ? func_symbol->name : "unknown_function";
        const uint64_t func_addr = func_symbol ? func_symbol->address : 0;
        printf("\33[1;34m0x%08lx\33[1;0m: ", s->pc);
        for (int i = 0; i < indentaton_level; i++) {
            printf("  ");
        }
        printf("call [%s@%08lx]\n", func_name, func_addr);
        indentaton_level++;
    } else if (is_ret(s->inst)) {
        indentaton_level--;
        if (indentaton_level < 0) indentaton_level = 0;
        const FuncSymbol *func_symbol = find_func(sym_table, s->pc);
        const char *func_name = func_symbol ? func_symbol->name : "unknown_function";
        printf("\33[1;34m0x%08lx\33[1;0m: ", s->pc);
        for (int i = 0; i < indentaton_level; i++) {
            printf("  ");
        }
        printf("ret  [%s]\n", func_name);
    }
}