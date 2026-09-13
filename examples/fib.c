#include "../include/simpro.h"
#include <stdio.h>

enum Profiling
{
    PROFILING_FIB,
};
char const *ProfilingToString(SIMPRO_Id id)
{
    switch ((enum Profiling)id)
    {
        case PROFILING_FIB: return "FIB";
    }
    return "<UNKNOWN>";
}

void Printer(char const *fmt, ...);

uint64_t Fib(uint8_t n)
{
    uint64_t res;
    SIMPRO_BLOCK(PROFILING_FIB)
    {
        SIMPRO_ADD_BANDWIDTH(1);
        res = (n <= 1) ? n : Fib(n - 1) + Fib(n - 2);
    }
    return res;
}

int main()
{
    SIMPRO_GlobalBegin();
    uint64_t const fibRes = Fib(30);

    printf("--------------------------------------------------------------------------------\n");
    SIMPRO_GlobalFinalizeAndPrint(&printf, &ProfilingToString);
    printf("--------------------------------------------------------------------------------\n");

    printf("fibRes: %lu\n", fibRes);
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
#include "../include/simpro_linux_x86_64.inc"
#define SIMPRO_IMPL
#include "../include/simpro.h"
