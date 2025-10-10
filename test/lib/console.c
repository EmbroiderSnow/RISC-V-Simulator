#include "syscall.h"
#include <stdint.h>
#include "stdio.h"

void putchar(int c) {
    // Implement putchar to output a character to the console
    // This is a stub implementation; replace with actual output code
    // For example, you might write to a memory-mapped I/O register
    // or use a system call to write to stdout.
    write(STDOUT_FILENO, &c, 1);
}

// int getchar(void) {
//     // Implement getchar to read a character from the console
//     // This is a stub implementation; replace with actual input code
//     // For example, you might read from a memory-mapped I/O register
//     // or use a system call to read from stdin.
// }