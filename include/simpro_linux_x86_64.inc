#include "simpro.h"

#include <sys/time.h>
#include <x86intrin.h>

uint64_t SIMPRO_ReadSystemTime_us(void)
{
    struct timeval Value;
    gettimeofday(&Value, 0);

    uint64_t const microSec = 1000000llu * (uint64_t)Value.tv_sec + (uint64_t)Value.tv_usec;
    return microSec;
}
uint64_t SIMPRO_ReadCPUTimer_x()
{
    return (uint64_t)__rdtsc();
}

/*
    MIT License

    Copyright (c) 2026 Lukas Kerkemeier

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
