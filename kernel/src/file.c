/**
 * ファイル
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_FILE
#define HEADER_FILE

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


FILE_T *f_get_file(int fd);

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
#include "str.h"
#include "task.h"


//=============================================================================
// 公開関数


int f_open(const char *name, int flags)
{
    int fd = get_free_fd();

    if (fd == -1)
        return -1;

    if (strcmp(name, "/dev/tty") == 0 && flags == O_WRONLY){
        task_set_file(fd, f_console);
        return fd;
    } else if (strcmp(name, "/dev/keyboard") == 0 && flags == O_RDONLY) {
        task_set_file(fd, f_keyboard);
        return fd;
    } else if (strcmp(name, "/debug/temp") == 0) {
        task_set_file(fd, f_dbg_temp);
        return fd;
    }

    return -1;
}


int f_close(int fd)
{
    FILE_T *f = f_get_file(fd);

    if (f == 0)
        return -1;

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


FILE_T *f_get_file(int fd)
{
    return task_get_file(fd);
}


//=============================================================================
// 非公開関数


