/**
 * スタックトレース
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_STACKTRACE
#define HEADER_STACKTRACE

#include "file.h"


void init_func_names(void);
void stacktrace(unsigned int max_frames, struct FILE_T *f);
void stacktrace2(unsigned int max_frames, struct FILE_T *f, unsigned int *ebp);
char *get_func_name(unsigned int addr);


#endif


//=============================================================================
// 非公開ヘッダ

#include "debug.h"
#include "ext2fs.h"
#include "memory.h"
#include "str.h"

#define FNAMES_FILE_NAME  "fnames.dat"

struct FUNC_NAME {
    unsigned int addr;
    char name[32];
};


static struct FUNC_NAME *l_func_names = 0;
static int l_num_fnames = 0;


//=============================================================================
// 公開関数

void init_func_names(void)
{
    int inode_i = ext2_get_inode_i(EXT2_ROOT_INODE, FNAMES_FILE_NAME);

    if (inode_i == 0) {
        dbgf("File not found. %s\n", FNAMES_FILE_NAME);
        return;
    }

    int size_B;
    char *p = (char *) ext2_open(inode_i, &size_B);

    if (p == 0) {
        dbgf("Could not open file. %s\n", FNAMES_FILE_NAME);
        return;
    }

    l_num_fnames = size_B / sizeof(struct FUNC_NAME);
    l_func_names = (struct FUNC_NAME *) p;
}


// http://wiki.osdev.org/Stack_Trace
void stacktrace(unsigned int max_frames, struct FILE_T *f)
{
    unsigned int *ebp = &max_frames - 2;
    stacktrace2(max_frames, f, ebp);
}


void stacktrace2(unsigned int max_frames, struct FILE_T *f, unsigned int *ebp)
{
    fprintf(f, "stack trace:\n");
    for(unsigned int frame = 0; frame < max_frames; ++frame)
    {
        unsigned int eip = ebp[1];
        if(eip == 0)
            break;

        ebp = (unsigned int *) ebp[0];
        //unsigned int *arguments = &ebp[2];
        char *name = get_func_name(eip);
        if (name) {
            fprintf(f, "  %p: %.32s\n", eip, name);
        } else {
            fprintf(f, "  %p: %.32s\n", eip, "unknown func");
            return;
        }
    }
}


char *get_func_name(unsigned int addr)
{
    char *name = 0;

    if (l_func_names == 0)
        return name;

    for (int i = 0; i < l_num_fnames; i++) {
        if (l_func_names[i].addr >= addr)
            break;

        name = l_func_names[i].name;
    }

    return name;
}
