#ifndef HEADER_UTILS
#define HEADER_UTILS

//-----------------------------------------------------------------------------
// API 作成補助関数
//-----------------------------------------------------------------------------


// 引数なしの API 呼び出し
inline __attribute__ ((always_inline))
static int api0(int api_no)
{
    int ret = 0;

    __asm__ __volatile__ (
        "int $0x44"

        : "=a" (ret)
        : "0" (api_no)
    );

    return ret;
}


// 引数が１つの API 呼び出し
inline __attribute__ ((always_inline))
static int api1(int api_no, int arg1)
{
    int ret = 0;

    __asm__ __volatile__ (
        "int $0x44"

        : "=a" (ret)
        : "0" (api_no), "b" (arg1)
    );

    return ret;
}


// 引数が２つの API 呼び出し
inline __attribute__ ((always_inline))
int api2(int api_no, int arg1, int arg2)
{
    int ret = 0;

    __asm__ __volatile__ (
        "int $0x44"

        : "=a" (ret)
        : "0" (api_no), "b" (arg1), "c" (arg2)
    );

    return ret;
}


// 引数が3つの API 呼び出し
inline __attribute__ ((always_inline))
int api3(int api_no, int arg1, int arg2, int arg3)
{
    int ret = 0;

    __asm__ __volatile__ (
        "int $0x44"

        : "=a" (ret)
        : "0" (api_no), "b" (arg1), "c" (arg2), "d" (arg3)
    );

    return ret;
}


// 引数が4つの API 呼び出し
inline __attribute__ ((always_inline))
int api4(int api_no, int arg1, int arg2, int arg3, int arg4)
{
    int ret = 0;

    __asm__ __volatile__ (
        "int $0x44"

        : "=a" (ret)
        : "0" (api_no), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4)
    );

    return ret;
}


// 引数が5つの API 呼び出し
inline __attribute__ ((always_inline))
int api5(int api_no, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    int ret = 0;

    __asm__ __volatile__ (
        "int $0x44"

        : "=a" (ret)
        : "0" (api_no), "b" (arg1), "c" (arg2), "d" (arg3),
          "S" (arg4), "D" (arg5)
    );

    return ret;
}

#endif

