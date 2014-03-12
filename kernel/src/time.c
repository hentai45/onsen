/**
 * 時間よ止まれ
 *
 * http://wiki.osdev.org/CMOS#Accessing_CMOS_Registers
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_TIME
#define HEADER_TIME

#define MAX_DT_STR 32

struct DATETIME {
    int year;
    int mon;
    int day;
    int hour;
    int min;
    int sec;
    char str[MAX_DT_STR];
};

#endif


//=============================================================================
// 非公開ヘッダ

#include <stdbool.h>

#include "asmfunc.h"
#include "str.h"

#define CMOS_REG_ADDR  0x70
#define CMOS_REG_DATA  0x71

#define CMOS_SEC   0x00
#define CMOS_MIN   0x02
#define CMOS_HOUR  0x04
#define CMOS_DAY   0x07
#define CMOS_MON   0x08
#define CMOS_YEAR  0x09

#define CMOS_STATE_A  0x0A
#define CMOS_STATE_B  0x0B

#define IS_BCD(state_b)  (((state_b) & 0x04) == 0)
#define BCD_TO_VAL(bcd)  ( ((bcd) >> 4) * 10 + ((bcd) & 0x0F) )

#define DT_IS_EQUAL(a,b)  ( ((a)->sec == (b)->sec) && ((a)->min == (b)->min) && ((a)->hour == (b)->hour) && ((a)->day == (b)->day) && ((a)->mon == (b)->mon) && ((a)->year == (b)->year) )

#define DT_IS_NOT_EQUAL(a, b) ( ! DT_IS_EQUAL(a, b))

//=============================================================================
// 関数

static unsigned char get_rtc(struct DATETIME *t);

void now(struct DATETIME *t)
{
    struct DATETIME tmp;
    unsigned char state_b;

    get_rtc(&tmp);

    do {
        *t = tmp;

        state_b = get_rtc(&tmp);
    } while (DT_IS_NOT_EQUAL(t, &tmp));

    bool hour24 = false;
    if (t->hour & 0x80) {
        t->hour &= 0x7F;
        hour24 = true;
    }

    // 値がBCD（二進化十進表現）なら数値に変換する
    if (IS_BCD(state_b)) {
        t->sec  = BCD_TO_VAL(t->sec);
        t->min  = BCD_TO_VAL(t->min);
        t->hour = BCD_TO_VAL(t->hour);
        t->day  = BCD_TO_VAL(t->day);
        t->mon  = BCD_TO_VAL(t->mon);
        t->year = BCD_TO_VAL(t->year);
    }

    // 12時間表示を24時間表示に変更
    if (!(state_b & 0x02) && hour24) {
        t->hour = (t->hour + 12) % 24;
    }

    t->year += 2000;

    snprintf(t->str, MAX_DT_STR, "%d-%02d-%02d %02d:%02d:%02d",
            t->year, t->mon, t->day, t->hour, t->min, t->sec);
}


static void wait_tm_ready(void);
static unsigned char get_cmos_register(char no);

static unsigned char get_rtc(struct DATETIME *t)
{
    cli();

    wait_tm_ready();

    t->sec  = get_cmos_register(CMOS_SEC);
    t->min  = get_cmos_register(CMOS_MIN);
    t->hour = get_cmos_register(CMOS_HOUR);
    t->day  = get_cmos_register(CMOS_DAY);
    t->mon  = get_cmos_register(CMOS_MON);
    t->year = get_cmos_register(CMOS_YEAR);

    unsigned char state_b = get_cmos_register(CMOS_STATE_B);

    sti();

    return state_b;
}

static void wait_tm_ready(void)
{
    while (get_cmos_register(CMOS_STATE_A) & 0x80)
        ;
}

static void select_cmos_register(char no);

static unsigned char get_cmos_register(char no)
{
    select_cmos_register(no);  // inb(0x71)後は選択レジスタ番号が0x0Dにリセットされるらしい
    return inb(CMOS_REG_DATA);
}


#define NMI_DISABLE  (0x80)

static void delay(void);

static void select_cmos_register(char no)
{
    outb(CMOS_REG_ADDR, NMI_DISABLE | no);

    delay();
}


#define DELAY_NUM  100

// どれぐらい待てばいいの？とりあえず適当にしておく
static void delay(void)
{
    for (int i = 0; i < DELAY_NUM; i++) {
        __asm__ __volatile__ ("nop");
    }
}
