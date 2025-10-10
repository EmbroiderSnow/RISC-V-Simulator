#include "syscall.h"
#include "stdio.h"

int main() {
    char ch = 'A';
    char *str = "Hello, World!";
    char *null_str = NULL;
    int pos_int = 12345;
    int neg_int = -54321;
    unsigned int uns_int = 40000;
    void *ptr = (void*)0x80001234;

    printf("--- Comprehensive printf Test Suite ---\n\n");

    // 1. Basic Conversions
    printf("1. Basic Conversion Specifiers:\n");
    printf("   - Character '%%c': %c\n", ch);
    printf("   - String '%%s': %s\n", str);
    printf("   - Signed Decimal '%%d': %d\n", pos_int);
    printf("   - Another Signed Decimal '%%d': %d\n", neg_int);
    printf("   - Unsigned Decimal '%%u': %u\n", uns_int);
    printf("   - Octal '%%o': %o\n", 0123);
    printf("   - Hexadecimal '%%x': %x\n", 0xABCD);
    printf("   - Pointer '%%p': %p\n", ptr);
    printf("   - Escaped '%%%%': %%\n\n");

    // 2. Width and Padding Flags ('0' and '-')
    printf("2. Width and Padding Flags:\n");
    printf("   - Padded Int [\"%%5d\"]: [%5d]\n", 123);
    printf("   - Zero-Padded Int [\"%%05d\"]: [%05d]\n", 123);
    printf("   - Left-Aligned Int [\"%%-5d\"]: [%-5d]\n", 123);
    printf("   - Padded String [\"%%15s\"]: [%15s]\n", "test");
    printf("   - Left-Aligned String [\"%%-15s\"]: [%-15s]\n\n", "test");

    // 3. Precision Specifier ('.')
    printf("3. Precision Specifier:\n");
    printf("   - Limited String [\"%%.5s\"]: [%.5s]\n", "longstring");
    printf("   - Limited String with width [\"%%10.5s\"]: [%10.5s]\n", "longstring");
    printf("   - Limited String left-aligned [\"%%-10.5s\"]: [%-10.5s]\n\n", "longstring");

    // 4. Long Specifiers ('l' and 'll')
    printf("4. Long Specifiers:\n");
    long long_num = 9876543210L;
    unsigned long long ull_num = 12345678901234567890ULL;
    printf("   - Long Decimal '%%ld': %ld\n", long_num);
    printf("   - Long Long Unsigned '%%llu': %llu\n", ull_num);
    printf("   - Long Long Hex '%%llx': %llx\n\n", ull_num);

    // 5. Edge Cases
    printf("5. Edge Cases:\n");
    printf("   - Printing Zero: %d\n", 0);
    printf("   - Printing NULL string: %s\n", null_str);
    printf("   - Printing pointer to Zero: %p\n", (void*)0);
    printf("   - Unrecognized Specifier '%%z': %z\n\n");
    
    // 6. Combination Test
    printf("6. Combination Test:\n");
    printf("   - Result: [%-15.8s] dec: [%08d] hex: [%08x]\n", "a very long string", -42, 42);

    printf("\n--- Test Suite Finished ---\n");

    // Signal that the test completed successfully.
    exit(0);
}