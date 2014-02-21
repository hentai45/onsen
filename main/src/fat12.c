/**
 * FAT12
 *
 * @file fat12.c
 * @author Ivan Ivanovich Ivanov
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_FAT12
#define HEADER_FAT12

typedef struct FILEINFO {
    char name[8];
    char ext[3];
    unsigned char type;
    char reserve[10];
    unsigned short time;
    unsigned short data;
    unsigned short clustno;
    unsigned int size;
} FILEINFO;


void fat12_init(void);
FILEINFO *fat12_get_file_info(void);
void fat12_load_file(int clustno, int size, char *buf);
int fat12_search_file(FILEINFO *fi, char *fname);


#endif


//=============================================================================
// 非公開ヘッダ

#include "memory.h"
#include "str.h"


static int fat[4 * 2880];


static void fat12_read_fat(int *fat);


//=============================================================================
// 公開関数

void fat12_init(void)
{
    fat12_read_fat(fat);
}


FILEINFO *fat12_get_file_info(void)
{
    return (FILEINFO *) (VADDR_DISK_IMG + 0x2600);
}


void fat12_load_file(int clustno, int size, char *buf)
{
    unsigned char *img = (unsigned char *) (VADDR_DISK_IMG + 0x3E00);

    for (;;) {
        if (size <= 512) {
            for (int i = 0; i < size; i++) {
                buf[i] = img[clustno * 512 + i];
            }
            break;
        }

        for (int i = 0; i < 512; i++) {
            buf[i] = img[clustno * 512 + i];
        }

        size -= 512;
        buf += 512;
        clustno = fat[clustno];
    }
}


int fat12_search_file(FILEINFO *fi, char *fname)
{
    // **** ファイル名を正規化する
    // ファイル名大文字で８文字 + 拡張子大文字で３文字
    // 空いた領域は空白で埋める

    char s[12];
    for (int i = 0; i < 11; i++) {
        s[i] = ' ';
    }

    int j = 0;
    for (int i = 0; fname[i] != 0; i++) {
        if (j >= 11)
            return -1;

        if (fname[i] == '.' && j <= 8) {
            j = 8;
        } else {
            s[j] = fname[i];
            if ('a' <= s[j] && s[j] <= 'z') {
                s[j] -= 0x20;
            }
            j++;
        }
    }

    // **** 検索

    for (int i = 0; i < 224; i++) {
        // ここで終了か？
        if (fi[i].name[0] == 0x00)
            return -1;

        // 削除されたファイルは飛ばす
        if (fi[i].name[0] == 0xE5)
            continue;

        // 隠しファイルかディレクトリなら次へ
        if ((fi[i].type & 0x18) != 0)
            continue;

        if (s_ncmp((char *) fi[i].name, s, 11) == 0)
            // 見つかった
            return i;
    }

    return -1;
}


//=============================================================================
// 非公開関数

/// FAT12のFAT領域を読み込む
static void fat12_read_fat(int *fat)
{
    unsigned char *img = (unsigned char *) (VADDR_DISK_IMG + 0x0200);

    for (int i = 0, j = 0; i < 2880; i += 2, j += 3) {
        fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xFFF;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xFFF;
    }
}


