/**
 * 文字列
 */


#ifndef HEADER_STRING
#define HEADER_STRING

#include <stdarg.h>
#include "file.h"

int  strlen(const char *s);
char *strcpy(char *s, const char *t);
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


#include <stdbool.h>
#include "debug.h"

static void reverse(char *s);


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
        if (*s == '\0' || *t == '\0') {
            if (*s == '\0' && *t == '\0')
                return 0;
            else
                break;
        }
    }

    return *s - *t;
}

int strncmp(const char *s, const char *t, int n)
{
    int i;
    for (i = 0; i < n && *s == *t; i++, s++, t++) {
        if (*s == '\0' || *t == '\0') {
            if (*s == '\0' && *t == '\0')
                return 0;
            else
                break;
        }
    }

    if (i == n)
        return 0;

    return *s - *t;
}


static void s_itox(unsigned int n, char *s, int digit, bool capital);
static void s_itoa(int n, char *s);
static void s_uitoa(unsigned int n, char *s);
static void s_itob(unsigned int n, char *s, bool space);
static void s_size(unsigned int size_B, char *s, int max);


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


int fprintf(struct FILE_T *f, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int cnt = vfprintf(f, fmt, ap);
    va_end(ap);
    return cnt;
}


int snprintf(char *s, unsigned int n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(s, n, fmt, ap);
    va_end(ap);

    return ret;
}


int  vfprintf(struct FILE_T *f, const char *fmt, va_list ap)
{
    char l_printf_buf[4096];

    if (f == 0 || f->write == 0)
        f = f_debug;

    int cnt = vsnprintf(l_printf_buf, 4096, fmt, ap);

    f->write(f->self, l_printf_buf, cnt);

    return cnt;
}


#define FLG_HASH_SIGN (1)

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

int vsnprintf(char *s, unsigned int n, const char *fmt, va_list ap)
{
    char l_tmp[256];

    if (n == 0)
        return 0;

    if (n == 1) {
        s[0] = 0;
        return 0;
    }

    char *s_val;
    unsigned char ch_val;

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
            flags = FLG_HASH_SIGN;

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
            ch_val = (unsigned char) va_arg(ap, int);
            ADD_CHAR(ch_val);
        } else if (*p == 's') {  // 文字列
            if (precision == 0)
                precision = 0xFFFFFFFF;

            for (s_val = va_arg(ap, char *); s_val && *s_val && precision > 0; s_val++, precision--) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'd' || *p == 'i') {  // 整数
            s_itoa(va_arg(ap, int), l_tmp);

            for (int k = width - strlen(l_tmp); k > 0; k--) {
                if (pad0) {
                    ADD_CHAR('0');
                } else {
                    ADD_CHAR(' ');
                }
            }

            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'u') {  // 符号なし整数
            s_uitoa(va_arg(ap, unsigned int), l_tmp);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'x') {  // 符号なし16進数(10以上はabcdef)
            if (flags == FLG_HASH_SIGN) {
                ADD_CHAR('0');
                ADD_CHAR('x');
            }

            s_itox(va_arg(ap, unsigned int), l_tmp, 8, false);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'X') {  // 符号なし16進数(10以上はABCDEF)
            if (flags == FLG_HASH_SIGN) {
                ADD_CHAR('0');
                ADD_CHAR('x');
            }

            s_itox(va_arg(ap, unsigned int), l_tmp, 8, true);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'p') {  // ポインタ
            ADD_CHAR('0');
            ADD_CHAR('x');
            s_itox((unsigned int) va_arg(ap, void *), l_tmp, 8, true);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'b') {  // ビット表示
            s_itob(va_arg(ap, unsigned int), l_tmp, false);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'B') {  // ビット表示（8ビットごとにスペースが入る）
            s_itob(va_arg(ap, unsigned int), l_tmp, true);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'z') {  // サイズ（ex. 32 MB）
            s_size(va_arg(ap, unsigned int), l_tmp, 1024);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'Z') {  // サイズ (ex. 32768 KB)
            s_size(va_arg(ap, unsigned int), l_tmp, 1024 * 1024);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else {
            ADD_CHAR(*s_val);
        }
    }

end_loop:

    s[i] = 0;

    return ret;
}


void s_itox(unsigned int n, char *s, int digit, bool capital)
{
    int i, d;
    char char_a;

    if (capital)
        char_a = 'A';
    else
        char_a = 'a';

    if (digit > 8)
        digit = 8;

    for (i = 0; i < digit; i++) {
        d = n & 0x0F;
        if (d < 10)
            s[i] = d + '0';
        else
            s[i] = d - 10 + char_a;
        n >>= 4;
    }

    s[i] = '\0';

    reverse(s);
}


void s_itoa(int n, char *s)
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


void s_uitoa(unsigned int n, char *s)
{
    int i;

    i = 0;

    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    s[i] = '\0';

    reverse(s);
}


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


void s_size(unsigned int size_B, char *s, int max)
{
    int d = 0;

    while (size_B >= max) {
        size_B /= 1024;
        d++;
    }

    s_itoa(size_B, s);

    switch (d) {
    case 0:
        strcat(s, " B");
        break;

    case 1:
        strcat(s, " KB");
        break;

    case 2:
        strcat(s, " MB");
        break;

    case 3:
        strcat(s, " GB");
        break;
    }
}
