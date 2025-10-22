#ifndef __FTRACE_H__
#define __FTRACE_H__

#include <stdint.h>
#include <isa_decode.h>

typedef struct {
    uint64_t address; 
    char* name;       
} FuncSymbol;

typedef struct {
    FuncSymbol* symbols; 
    int count;           
    int capacity;        
} SymbolTable;

void init_symbol_table(SymbolTable *table);

void add_func_symbol(SymbolTable *table, uint64_t addr, const char *name);

void sort_symbols_by_address(SymbolTable *table);

const FuncSymbol* find_func(SymbolTable *table, uint64_t addr);

void free_symbol_table(SymbolTable *table);

void handle_ftrace(Decode *s);

#endif