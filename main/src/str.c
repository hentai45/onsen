/**
 * 文字列
 */


#ifndef HEADER_STRING
#define HEADER_STRING

#include <stdarg.h>

int  s_len(const char *s);
void s_cpy(char *s, const char *t);
void s_cat(char *s, const char *t);
void s_reverse(char *s);
int  s_cmp(const char *s, const char *t);
int  s_ncmp(const char *s, const char *t, int n);

void s_to_upper(char *s);

int  s_atoi(const char *s);

int  s_snprintf(char *s, unsigned int n, const char *fmt, ...);
int  s_snprintf2(char *s, unsigned int n, const char *fmt, va_list ap);

int   memcmp(const void *buf1, const void *buf2, unsigned int);
void *memcpy(void *dst, const void *src, unsigned int);
void *memmove(void *dst, const void *src, unsigned int);
void *memset(void *dst, int c, unsigned int count);

#endif


int s_len(const char *s)
{
    const char *p = s;

    while (*p++)
        ;

    return p - s - 1;
}

void s_cpy(char *s, const char *t)
{
    while ((*s++ = *t++) != 0)
        ;
}

void s_cat(char *s, const char *t)
{
    while (*s++)
        ;
    s--;
    while ((*s++ = *t++) != 0)
        ;
}

void s_reverse(char *s)
{
    int c, i, j;

    for (i = 0, j = s_len(s) - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

// s<tなら<0, s==tなら0, s>tなら>0を返す
int s_cmp(const char *s, const char *t)
{
    for ( ; *s == *t; s++, t++)
        if (*s == '\0')
            return 0;

    return *s - *t;
}

int s_ncmp(const char *s, const char *t, int n)
{
    int i;
    for (i = 0; i < n && *s == *t; i++, s++, t++)
        if (*s == '\n')
            return 0;

    if (i == n)
        return 0;

    return *s - *t;
}
static char l_tmp[256];

static void s_itoxs(unsigned int n, char *s, int digit);
static void s_itox(unsigned int n, char *s, int digit);
static void s_itoa(int n, char *s);
static void s_uitoa(unsigned int n, char *s);
static void s_itob(unsigned int n, char *s);
static void s_itoB(unsigned int n, char *s);
static void s_size(unsigned int size_B, char *s, int max);

#define ADD_CHAR(ch)  do { \
    s[i++] = (ch);         \
    ret++;                 \
    if (i >= n-1)          \
        break;             \
} while (0)

int s_snprintf(char *s, unsigned int n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = s_snprintf2(s, n, fmt, ap);
    va_end(ap);

    return ret;
}

int s_snprintf2(char *s, unsigned int n, const char *fmt, va_list ap)
{
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

    for (char *p = fmt; *p; p++) {
        if (*p != '%') {
            ADD_CHAR(*p);
            continue;
        }

        p++;

        if (*p == '%') {  // %
            ADD_CHAR('%');
        } else if (*p == 'c') {  // 1文字
            ch_val = (unsigned char) va_arg(ap, int);
            ADD_CHAR(ch_val);
        } else if (*p == 's') {  // 文字列
            for (s_val = va_arg(ap, char *); *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'd' || *p == 'i') {  // 整数
            s_itoa(va_arg(ap, int), l_tmp);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'u') {  // 符号なし整数
            s_uitoa(va_arg(ap, unsigned int), l_tmp);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'x') {  // 符号なし16進数(10以上はabcdef)
            s_itoxs(va_arg(ap, unsigned int), l_tmp, 8);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'X') {  // 符号なし16進数(10以上はABCDEF)
            s_itox(va_arg(ap, unsigned int), l_tmp, 8);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'p') {  // ポインタ
            ADD_CHAR('0');
            ADD_CHAR('x');
            s_itox((unsigned int) va_arg(ap, void *), l_tmp, 8);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'b') {  // ビット表示
            s_itob(va_arg(ap, unsigned int), l_tmp);
            for (s_val = l_tmp; *s_val; s_val++) {
                ADD_CHAR(*s_val);
            }
        } else if (*p == 'B') {  // ビット表示（8ビットごとにスペースが入る）
            s_itoB(va_arg(ap, unsigned int), l_tmp);
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

    s[i] = 0;

    return ret;
}


void s_itoxs(unsigned int n, char *s, int digit)
{
    int i, d;

    if (digit > 8)
        digit = 8;

    for (i = 0; i < digit; i++) {
        d = n & 0x0F;
        if (d < 10)
            s[i] = d + '0';
        else
            s[i] = d - 10 + 'a';
        n >>= 4;
    }

    s[i] = '\0';

    s_reverse(s);
}

void s_itox(unsigned int n, char *s, int digit)
{
    int i, d;

    if (digit > 8)
        digit = 8;

    for (i = 0; i < digit; i++) {
        d = n & 0x0F;
        if (d < 10)
            s[i] = d + '0';
        else
            s[i] = d - 10 + 'A';
        n >>= 4;
    }

    s[i] = '\0';

    s_reverse(s);
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

    s_reverse(s);
}


void s_uitoa(unsigned int n, char *s)
{
    int i;

    i = 0;

    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    s[i] = '\0';

    s_reverse(s);
}


void s_itob(unsigned int n, char *s)
{
    int i;

    for (i = 31; i >= 0; i--) {
        if (n & (1 << i)) {
            *s++ = '1';
        } else {
            *s++ = '0';
        }
    }

    *s = '\0';
}

void s_itoB(unsigned int n, char *s)
{
    int i;

    for (i = 31; i >= 0; i--) {
        if (n & (1 << i)) {
            *s++ = '1';
        } else {
            *s++ = '0';
        }

        if ((i & 0x7) == 0) {
            *s++ = ' ';
        }
    }

    *s = '\0';
}


int s_atoi(const char *s)
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
        s_cat(s, " B");
        break;

    case 1:
        s_cat(s, " KB");
        break;

    case 2:
        s_cat(s, " MB");
        break;

    case 3:
        s_cat(s, " GB");
        break;
    }
}
