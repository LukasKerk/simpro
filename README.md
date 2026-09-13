# SIMPRO

SIMPRO ("SIMple PROfiler") is a [STB-style](https://github.com/nothings/stb) library for measuring the time your code takes.
It is _heavily_ inspired by a design of [Casey Muratori](https://caseymuratori.com/).

SIMPRO is really light on its dependencies.
It only requires the `C99` standard and the `stdint.h` header.
To achieve this, you as the user are tasked to provide two timing functions:
```
uint64_t SIMPRO_ReadSystemTime_us(void);
uint64_t SIMPRO_ReadCPUTimer_x(void);
```

The minimal usage looks something like this:
```
#define SIMPRO_ENABLE
#define SIMPRO_IMPL
#include "simpro.h"

#include <stdio.h>

int main()
{
    SIMPRO_GlobalBegin();

    SIMPRO_BLOCK(42) // <- Id of this "range"
    {
        // your code to test
    }

    SIMPRO_GlobalFinalizeAndPrint(&printf, NULL);
}
```


## How to Integrate

If you want to use SIMPRO in your own code, you can just copy `include/simpro.h` into your code base.
If you do not want to provide your own timing functions, you might be able to find a default implementation in `include`.
For example for Linux on an X86-64 machine you can take the implementations from `include/simpro_x86_64.inc`.


## How to Build

The examples are build with [nob](https://github.com/tsoding/nob.h).
To build the examples on a Linux machine you can run
```
cc nob.c -o nob && ./nob
```
