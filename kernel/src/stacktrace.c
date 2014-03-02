/**
 * スタックトレース
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_STACKTRACE
#define HEADER_STACKTRACE

#include "file.h"


void init_func_names(void);
void stack_trace(unsigned int max_frames, FILE_T *f);


#endif


//=============================================================================
// 非公開ヘッダ

#include "fat12.h"
#include "memory.h"
#include "str.h"

#define FNAMES_FILE_NAME  "fnames.bin"

typedef struct _FUNC_NAME {
    unsigned int addr;
    char name[32];
} FUNC_NAME;


static char *get_func_name(unsigned int addr);

static FUNC_NAME *l_func_names = 0;
static int l_num_fnames = 0;


//=============================================================================
// 公開関数

void init_func_names(void)
{
    FILEINFO *finfo = fat12_get_file_info();
    int i = fat12_search_file(finfo, FNAMES_FILE_NAME);

    if (i < 0)
        return;

    l_num_fnames = finfo[i].size / sizeof(FUNC_NAME);
    l_func_names = (FUNC_NAME *) mem_alloc(finfo[i].size);
    fat12_load_file(finfo[i].clustno, finfo[i].size, (char *) l_func_names);
}


// http://wiki.osdev.org/Stack_Trace
void stack_trace(unsigned int max_frames, FILE_T *f)
{
    unsigned int *ebp = &max_frames - 2;
    s_fprintf(f, "stack trace:\n");
    for(unsigned int frame = 0; frame < max_frames; ++frame)
    {
        unsigned int eip = ebp[1];
        if(eip == 0)
            break;

        ebp = (unsigned int *) ebp[0];
        //unsigned int *arguments = &ebp[2];
        char *name = get_func_name(eip);
        s_fprintf(f, "  %#X: %.32s\n", eip, name);
    }
}


static char *get_func_name(unsigned int addr)
{
    char *name = "unknown func";

    if (l_func_names == 0)
        return name;

    for (int i = 0; i < l_num_fnames; i++) {
        name = l_func_names[i].name;

        if (l_func_names[i].addr >= addr)
            break;
    }

    return name;
}
