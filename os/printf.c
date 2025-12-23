// 依赖 console_putchar 函数输出字符
// Log 打印
#include <stdarg.h>

void console_putchar(int c);

void printstr(char *s) {
    while (*s) console_putchar(*s++);
}

void printint(int xx, int base, int sign) {
    static char digits[] = "0123456789abcdef";
    char buf[16];
    int i;
    unsigned int x;

    if (sign && (sign = xx < 0)) x = -xx;
    else x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign) buf[i++] = '-';

    while (--i >= 0) console_putchar(buf[i]);
}

void printf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (int i = 0; fmt[i]; i++) {
        char c = fmt[i];
        if (c == '%') {
            c = fmt[++i];
            switch (c) {
            case 'd': printint(va_arg(ap, int), 10, 1); break;
            case 'x': printint(va_arg(ap, int), 16, 0); break;
            case 's': printstr(va_arg(ap, char*)); break;
            case '%': console_putchar('%'); break;
            default: console_putchar(c);
            }
        } else {
            console_putchar(c);
        }
    }
    va_end(ap);
}