#ifndef STDIO_H
#define STDIO_H

#include "stdarg.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

void putchar(int c);
int getchar(void);

void printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list ap);

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);

#endif