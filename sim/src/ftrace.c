#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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