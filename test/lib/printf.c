#include "stdio.h"
#include "stdarg.h"

static void putch(int ch, int *cnt) {
    putchar(ch);
    *cnt += 1;
}

int vprintf(const char *fmt, va_list ap) {
    int cnt = 0;
    vprintfmt((void *)putch, &cnt, fmt, ap);
    return cnt;
}

int printf(const char *fmt, ...) {
    va_list ap;
    int cnt;

    va_start(ap, fmt);
    cnt = vprintf(fmt, ap);
    va_end(ap);

    return cnt;
}