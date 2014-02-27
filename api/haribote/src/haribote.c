#ifndef HEADER_HARIBOTE
#define HEADER_HARIBOTE

#include <stdarg.h>

#define COL_BLACK       (0)
#define COL_RED         (1)
#define COL_GREEN       (2)
#define COL_YELLOW      (3)
#define COL_BLUE        (4)
#define COL_PURPLE      (5)
#define COL_CYAN        (6)
#define COL_WHITE       (7)
#define COL_GRAY        (8)
#define COL_DARK_RED    (9)
#define COL_DARK_GREEN  (10)
#define COL_DARK_YELLOW (11)
#define COL_DARK_BLUE   (12)
#define COL_DARK_PURPLE (13)
#define COL_DARK_CYAN   (14)
#define COL_DARK_GRAY   (15)


void api_putchar(char ch);
void api_putstr0(const char *s);
void api_putstr1(const char *s, int cnt);
void api_end(void);
int  api_openwin(char *buf, int w, int h, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
int  api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc(void);
char *api_malloc(int size);
void api_free(char *addr, int size);
void api_point(int win, int x, int y, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
int  api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_closewin(int win);
int  api_getkey(int mode);
int  api_alloctimer(void);
void api_inittimer(int timer, int data);
void api_settimer(int timer, int time);
void api_freetimer(int timer);

void api_putintx(int n);

int  s_len(const char *s);
void s_cpy(char *s, const char *t);
void s_cat(char *s, const char *t);
void s_reverse(char *s);
int  s_cmp(const char *s, const char *t);
int  s_ncmp(const char *s, const char *t, int n);

void s_to_upper(char *s);

int  s_atoi(const char *s);

int  sprintf(char *s, const char *fmt, ...);
int  snprintf(char *s, unsigned int n, const char *fmt, ...);
int  vsnprintf(char *s, unsigned int n, const char *fmt, va_list ap);

#endif



