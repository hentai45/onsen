/**
 * ファイル
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_FILE
#define HEADER_FILE

#define FILE_TABLE_SIZE  (16)

#define STDIN_FILENO   (0)
#define STDOUT_FILENO  (1)
#define STDERR_FILENO  (2)

#define FILE_FLG_FREE  (0)
#define FILE_FLG_USED  (1)

#define O_RDONLY  (0)
#define O_WRONLY  (1)
#define O_RDWR    (2)

typedef struct _FILE_T {
    void *self;
    int refs;
    int (*close)(void *self);
    int (*read)(void *self, void *buf, int cnt);
    int (*write)(void *self, const void *buf, int cnt);
} FILE_T;

typedef struct _FILE_TABLE_ENTRY {
    int flags;
    FILE_T *file;
} FTE;


FTE *create_file_tbl(void);
int free_file_tbl(FTE *tbl);
FILE_T *f_get_file(int fd);
int f_set_file(int fd, FILE_T *f);

int f_open(const char *name, int flags);
int f_close(int fd);
int f_read(int fd, void *buf, int cnt);
int f_write(int fd, const void *buf, int cnt);

extern FILE_T *f_keyboard;

#endif

FILE_T *f_keyboard;

//=============================================================================
// 非公開ヘッダ

#include "console.h"
#include "debug.h"
#include "memory.h"
#include "str.h"
#include "task.h"

static int get_free_fd(void);


//=============================================================================
// 公開関数


int f_open(const char *name, int flags)
{
    int fd = get_free_fd();

    if (fd == -1)
        return -1;

    if (STRCMP(name, ==, "/dev/tty") && flags == O_WRONLY){
        f_set_file(fd, f_console);
        return fd;
    } else if (STRCMP(name, ==, "/dev/keyboard") && flags == O_RDONLY) {
        f_set_file(fd, f_keyboard);
        return fd;
    } else if (STRCMP(name, ==, "/debug/temp")) {
        f_set_file(fd, f_dbg_temp);
        return fd;
    }

    return -1;
}


int f_close(int fd)
{
    FILE_T *f = f_get_file(fd);

    if (f == 0)
        return -1;

    g_cur->file_tbl[fd].flags = FILE_FLG_FREE;

    if (f->close == 0)
        return 0;

    return f->close(f->self);
}


int f_read(int fd, void *buf, int cnt)
{
    FILE_T *f = f_get_file(fd);

    if (f == 0)
        return -1;

    if (f->read == 0)
        return 0;

    return f->read(f->self, buf, cnt);
}


int f_write(int fd, const void *buf, int cnt)
{
    FILE_T *f = f_get_file(fd);

    if (f == 0)
        return -1;

    if (f->write == 0)
        return 0;

    return f->write(f->self, buf, cnt);
}


FTE *create_file_tbl(void)
{
    int size = sizeof(FTE) * FILE_TABLE_SIZE;

    FTE *tbl = (FTE *) mem_alloc(size);

    memset(tbl, 0, size);

    return tbl;
}


int free_file_tbl(FTE *tbl)
{
    return mem_free(tbl);
}


FILE_T *f_get_file(int fd)
{
    if (g_cur->file_tbl == 0)
        return 0;

    if (fd < 0 || FILE_TABLE_SIZE <= fd) {
        DBGF("invalid fd (%d)\n", fd);
        return 0;
    }

    return g_cur->file_tbl[fd].file;
}


int f_set_file(int fd, FILE_T *f)
{
    if (g_cur->file_tbl == 0)
        return -1;

    if (fd < 0 || FILE_TABLE_SIZE <= fd) {
        DBGF("invalid fd (%d)\n", fd);
        return -1;
    }

    if (g_cur->file_tbl[fd].flags == FILE_FLG_USED) {
        DBGF("used fd (%d)\n", fd);
        return -1;
    }

    g_cur->file_tbl[fd].flags = FILE_FLG_USED;
    g_cur->file_tbl[fd].file = f;

    return 0;
}

//=============================================================================
// 非公開関数


static int get_free_fd(void)
{
    if (g_cur->file_tbl == 0)
        return -1;

    for (int fd = 0; fd < FILE_TABLE_SIZE; fd++) {
        if (g_cur->file_tbl[fd].flags == FILE_FLG_FREE) {
            return fd;
        }
    }

    return -1;
}

