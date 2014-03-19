/**
 * 文字列
 */


#ifndef HEADER_STRING
#define HEADER_STRING

#include <stdarg.h>
#include <stdbool.h>
#include "file.h"

void s_itob(unsigned int n, char *s, bool space);
void s_size(unsigned int size_B, char *s, int n);

int  strlen(const char *s);
char *strcpy(char *s, const char *t);
char *strncpy(char *s, const char *t, int n);
char *strcat(char *s, const char *t);
int  strcmp(const char *s, const char *t);
int  strncmp(const char *s, const char *t, int n);

int  atoi(const char *s);

int  printf(const char *fmt, ...);
int  fprintf(struct FILE_T *f, const char *fmt, ...);
int  snprintf(char *s, unsigned int n, const char *fmt, ...);
int  vfprintf(struct FILE_T *f, const char *fmt, va_list ap);
int  vsnprintf(char *s, unsigned int n, const char *fmt, va_list ap);

int   memcmp(const void *buf1, const void *buf2, unsigned int);
void *memcpy(void *dst, const void *src, unsigned int);
void *memmove(void *dst, const void *src, unsigned int);
void *memset(void *dst, int c, unsigned int count);

// 『エキスパートCプログラミング』p109
#define STRCMP(a, R, b)  (strcmp(a,b) R 0)

#define STRNCMP(a, R, b, n)  (strncmp(a,b,n) R 0)

#endif


#include "debug.h"


#define MAX(x,y)  (((x) > (y)) ? (x) : (y))
#define MIN(x,y)  (((x) < (y)) ? (x) : (y))


void s_itob(unsigned int n, char *s, bool space)
{
    int i;

    for (i = 31; i >= 0; i--) {
        if (n & (1 << i)) {
            *s++ = '1';
        } else {
            *s++ = '0';
        }

        if (space && (i & 0x7) == 0) {
            *s++ = ' ';
        }
    }

    *s = '\0';
}


static void s_itoa(int n, char *s);
static void reverse(char *s);

void s_size(unsigned int size_B, char *s, int n)
{
    char tmp[32];

    int d = 0;

    while (size_B >= 1024) {
        size_B /= 1024;
        d++;
    }

    s_itoa(size_B, tmp);

    switch (d) {
    case 0:
        strcat(tmp, " B");
        break;

    case 1:
        strcat(tmp, " KB");
        break;

    case 2:
        strcat(tmp, " MB");
        break;

    case 3:
        strcat(tmp, " GB");
        break;
    }

    strncpy(s, tmp, n);
}


int atoi(const char *s)
{
    int i;

    // 空白を飛ばす
    for (i = 0; s[i] == ' '; i++)
        ;

    int sign = (s[i] == '-') ? -1 : 1;

    // 符号を飛ばす
    if (s[i] == '+' || s[i] == '-')
        i++;

    int n;
    for (n = 0; '0' <= s[i] && s[i] <= '9'; i++)
        n = 10 * n + (s[i] - '0');

    return sign * n;
}


int strlen(const char *s)
{
    const char *p = s;

    while (*p++)
        ;

    return p - s - 1;
}

char *strcpy(char *s, const char *t)
{
    while ((*s++ = *t++) != 0)
        ;

    return s;
}

char *strncpy(char *s, const char *t, int n)
{
    while (n > 0 && (*s++ = *t++) != 0)
        n--;

    return s;
}

char *strcat(char *s, const char *t)
{
    while (*s++)
        ;
    s--;
    while ((*s++ = *t++) != 0)
        ;

    return s;
}

static void reverse(char *s)
{
    int c, i, j;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

// s<tなら<0, s==tなら0, s>tなら>0を返す
int strcmp(const char *s, const char *t)
{
    for ( ; *s == *t; s++, t++) {
        if (*s == '\0') {
            return 0;
        }
    }

    return *s - *t;
}

int strncmp(const char *s, const char *t, int n)
{
    int i;
    for (i = 0; i < n && *s == *t; i++, s++, t++) {
        if (*s == '\0') {
            return 0;
        }
    }

    if (i == n)
        return 0;

    return *s - *t;
}


static void itox(unsigned int n, char *s, bool capital);
static void s_uitoa(unsigned int n, char *s);


__attribute__((format (printf, 1, 2)))
int printf(const char *fmt, ...)
{
    char l_printf_buf[4096];

    struct FILE_T *f = f_get_file(STDOUT_FILENO);

    if (f == 0 || f->write == 0)
        f = f_debug;

    va_list ap;
    va_start(ap, fmt);
    int cnt = vsnprintf(l_printf_buf, 4096, fmt, ap);
    va_end(ap);

    f->write(f->self, l_printf_buf, cnt);

    return cnt;
}


__attribute__((format (printf, 2, 3)))
int fprintf(struct FILE_T *f, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int cnt = vfprintf(f, fmt, ap);
    va_end(ap);
    return cnt;
}


__attribute__((format (printf, 3, 4)))
int snprintf(char *s, unsigned int n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(s, n, fmt, ap);
    va_end(ap);

    return ret;
}


__attribute__((format (printf, 2, 0)))
int  vfprintf(struct FILE_T *f, const char *fmt, va_list ap)
{
    char l_printf_buf[4096];

    if (f == 0 || f->write == 0)
        f = f_debug;

    int cnt = vsnprintf(l_printf_buf, 4096, fmt, ap);

    f->write(f->self, l_printf_buf, cnt);

    return cnt;
}


#define FLG_HASH_SIGN   1
#define FLG_MINUS_SIGN  2

#define ADD_CHAR(ch)  do { \
    s[i++] = (ch);         \
    ret++;                 \
    if (i >= n-1)          \
        goto end_loop;     \
} while (0)

#define INCREMENT_P do { \
    if (*++p == 0)       \
        goto end_loop;   \
} while (0)

__attribute__((format (printf, 3, 0)))
int vsnprintf(char *s, unsigned int n, const char *fmt, va_list ap)
{
    char l_tmp[256];

    if (n == 0)
        return 0;

    if (n == 1) {
        s[0] = 0;
        return 0;
    }

    int i = 0;
    int ret = 0;

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            ADD_CHAR(*p);
            continue;
        }

        INCREMENT_P;

        /* flags */

        int flags = 0;

        if (*p == '#') {
            flags |= FLG_HASH_SIGN;

            INCREMENT_P;
        }

        if (*p == '-') {
            flags |= FLG_MINUS_SIGN;

            INCREMENT_P;
        }

        /* width */

        int width = 0;
        bool pad0 = false;

        if (*p == '0') {
            pad0 = true;
            INCREMENT_P;
        }

        while ('0' <= *p && *p <= '9') {
            width *= 10;
            width += *p - '0';

            INCREMENT_P;
        }

        if (*p == '*') {
           width = va_arg(ap, int);

           INCREMENT_P;
        }

        /* precision */

        unsigned int precision = 0;

        if (*p == '.') {
            INCREMENT_P;

            if (*p == '*') {
               precision = va_arg(ap, int);

               INCREMENT_P;
            } else {
                while ('0' <= *p && *p <= '9') {
                    precision *= 10;
                    precision += *p - '0';

                    INCREMENT_P;
                }
            }
        }

        if (*p == '%') {  // %
            ADD_CHAR('%');
        } else if (*p == 'c') {  // 1文字
            unsigned char ch_val = (unsigned char) va_arg(ap, int);
            ADD_CHAR(ch_val);
        } else if (*p == 's') {  // 文字列
            char *s_val = va_arg(ap, char *);

            int len;

            if (precision == 0) {
                len = strlen(s_val);
            } else {
                len = precision;
            }

            if ((flags & FLG_MINUS_SIGN) == 0) {
                for (int k = width - len; k > 0; k--) {
                    ADD_CHAR(' ');
                }
            }

            if (precision == 0)
                precision = 0xFFFFFFFF;

            for (; s_val && *s_val && precision > 0; s_val++, precision--) {
                ADD_CHAR(*s_val);
            }

            if (flags & FLG_MINUS_SIGN) {
                for (int k = width - len; k > 0; k--) {
                    ADD_CHAR(' ');
                }
            }
        } else if (*p == 'd' || *p == 'i') {  // 整数
            s_itoa(va_arg(ap, int), l_tmp);
            int len = strlen(l_tmp);

            if (pad0) {
                if (precision == 0) {
                    precision = width;
                }
            } else {
                for (int k = width - MAX(len, precision); k > 0; k--) {
                    ADD_CHAR(' ');
                }
            }

            for (int k = precision - len; k > 0; k--) {
                ADD_CHAR('0');
            }

            for (char *s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'u') {  // 符号なし整数
            s_uitoa(va_arg(ap, unsigned int), l_tmp);
            for (char *s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'x' || *p == 'X') {  // 符号なし16進数
            if (flags == FLG_HASH_SIGN) {
                width -= 2;
            }

            bool capital = (*p == 'X') ? true : false;
            itox(va_arg(ap, unsigned int), l_tmp, capital);
            int len = strlen(l_tmp);

            if (pad0) {
                if (precision == 0) {
                    precision = width;
                }
            } else {
                for (int k = width - MAX(len, precision); k > 0; k--) {
                    ADD_CHAR(' ');
                }
            }

            if (flags == FLG_HASH_SIGN) {
                ADD_CHAR('0');
                ADD_CHAR(*p);

                width -= 2;
            }

            for (int k = precision - len; k > 0; k--) {
                ADD_CHAR('0');
            }

            for (char *s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'p') {  // ポインタ
            ADD_CHAR('0');
            ADD_CHAR('x');

            unsigned int val = (unsigned int) va_arg(ap, void *);
            itox(val, l_tmp, true);
            int len = strlen(l_tmp);

            for (int k = 8 - len; k > 0; k--) {
                ADD_CHAR('0');
            }

            for (char *s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else {
            ADD_CHAR(*p);
        }
    }

end_loop:

    s[i] = 0;

    return ret;
}


static void itox(unsigned int n, char *s, bool capital)
{
    if (n == 0) {
        s[0] = '0';
        s[1] = '\0';
        return;
    }

    char char_a;

    if (capital)
        char_a = 'A';
    else
        char_a = 'a';

    int i = 0;

    while (n > 0) {
        int d = n & 0x0F;

        if (d < 10)
            s[i] = d + '0';
        else
            s[i] = d - 10 + char_a;

        n >>= 4;
        i++;
    }

    s[i] = '\0';

    reverse(s);
}


static void s_itoa(int n, char *s)
{
    int i, sign;

    if ((sign = n) < 0)
        n = -n;

    i = 0;

    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0)
        s[i++] = '-';

    s[i] = '\0';

    reverse(s);
}


static void s_uitoa(unsigned int n, char *s)
{
    int i;

    i = 0;

    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    s[i] = '\0';

    reverse(s);
}
