#ifndef HEADER_HARIBOTE
#define HEADER_HARIBOTE

void api_putchar(char ch);
void api_putstr0(const char *s);
void api_putstr1(const char *s, int cnt);
void api_end(void);
int  api_openwin(char *buf, int w, int h, int col_inv, char *title);

void api_putintx(int n);

#endif



