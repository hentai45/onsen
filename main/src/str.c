#ifndef HEADER_STRING
#define HEADER_STRING

int s_len(const char *s);
void s_cpy(char *s, const char *t);
void s_cat(char *s, const char *t);
void s_reverse(char *s);
int s_cmp(const char *s, const char *t);
int s_ncmp(const char *s, const char *t, int n);

void s_to_upper(char *s);

void s_itox(int n, char *s, int digit);
void s_itoa(int n, char *s);
int  s_atoi(const char *s);
void s_size(unsigned int size_B, char *s);

void s_dbg_int(char *s, const char *name, unsigned int var);
void s_dbg_intx(char *s, const char *name, unsigned int var);
void s_dbg_shortx(char *s, const char *name, unsigned short var);

int memcmp(const void *buf1, const void *buf2, unsigned int);
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

void s_itox(int n, char *s, int digit)
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

    //s[i++] = 'x';
    //s[i++] = '0';
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


void s_size(unsigned int size_B, char *s)
{
    int d = 0;

    while (size_B >= 1024) {
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


void s_dbg_int(char *s, const char *name, unsigned int var)
{
    s_cpy(s, name);

    char varx[16];
    s_itoa(var, varx);
    s_cat(s, varx);
}


void s_dbg_intx(char *s, const char *name, unsigned int var)
{
    s_cpy(s, name);

    char varx[9];
    s_itox(var, varx, 8);
    s_cat(s, varx);
}


void s_dbg_shortx(char *s, const char *name, unsigned short var)
{
    s_cpy(s, name);

    char varx[5];
    s_itox(var, varx, 4);
    s_cat(s, varx);
}

