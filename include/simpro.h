#ifndef SIMPRO_H_
#define SIMPRO_H_

//
// You typically only need to do something like this:
// SIMPRO_BLOCK(42) // 42 <- the index for this specific profile "range"
// {
//     // your code under test
// }
// This will time the scope and add it into the global data.
// Only do this
// - _after_ SIMPRO_GlobalBegin(), and
// - _before_ SIMPRO_GlobalFinalizeAndPrint() was called.
//
// If SIMPRO_ENABLE is not defined (or SIMPRO_DISABLE is defined), the profiling
// calls above will be effectively no-ops. However, your code will still be
// executed. So if you want to check how much the profiling is changing your
// global run time, you can disable it for a given translation unit.
//

#include <stdint.h>

typedef uint8_t SIMPRO_Id;

//
// User provided
//

/// Monotonic and possibly slow clock that returns some system clock in microseconds.
/// It is used to map the fast and possibly unknown CPU timer into something similar
/// to a wall clock.
uint64_t SIMPRO_ReadSystemTime_us(void);
/// Fast clock that returns some unit of time. It is only relevant that overflows of
/// the specific timer are correctly mapped onto the uint64_t overflows. This means
/// if you have a time A, then an overflow, and then a time B, that
/// (uint64_t)B-(uint64_t)A
/// is still the difference between the two times (caution: _one_ overflow).
uint64_t SIMPRO_ReadCPUTimer_x(void); // _x -> The exact unit of the timer is irrelevant!

//
// Library code
//

///
/// Call this to initialize the global data
///
void SIMPRO_GlobalBegin(void);

/// Function used to print the results, imilar to printf().
/// `int`: only to support direct use of `printf`.
typedef int(SIMPRO_printf)(char const *fmt, ...);
/// Function used to convert indices of ranges to readable names. NULL is a valid value.
typedef char const *(SIMPRO_IdLookup)(SIMPRO_Id id);
///
/// Call this to finalize all the profiling and printing the results.
///
void SIMPRO_GlobalFinalizeAndPrint(SIMPRO_printf *p, SIMPRO_IdLookup *lookup);

#if defined(SIMPRO_ENABLE) && !defined(SIMPRO_DISABLE)
#define SIMPRO_LocalRangeData SIMPRO_INTERNAL_LocalRangeData
#define SIMPRO_RangeBegin(ID) SIMPRO_INTERNAL_RangeBegin(ID)
#define SIMPRO_RangeEnd(PTR) SIMPRO_INTERNAL_RangeEnd(PTR)
#define SIMPRO_BLOCK(ID)                                                               \
    for (SIMPRO_LocalRangeData SIMPRO_internally_used_range__ = SIMPRO_RangeBegin(ID); \
         SIMPRO_internally_used_range__.id;                                            \
         SIMPRO_RangeEnd(&SIMPRO_internally_used_range__))
#define SIMPRO_ADD_BANDWIDTH(PROCESSED_BYTES) SIMPRO_INTERNAL_AddBandwidth(PROCESSED_BYTES)
#else
#define SIMPRO_LocalRangeData (void *)
#define SIMPRO_RangeBegin(ID) ((void)ID, NULL)
#define SIMPRO_RangeEnd(PTR) ((void)(PTR))
#define SIMPRO_BLOCK(ID) for (int i = 0; i < 1; ++i)
#define SIMPRO_ADD_BANDWIDTH(PROCESSED_BYTES) ((void)(PROCESSED_BYTES))
#endif

//
// DO NOT USE these "_INTERNAL_" types/functions directly.
// Use the provided macros below so the macros can be disabled.
//
typedef uint16_t SIMPRO_INTERNAL_Id;
typedef struct
{
    SIMPRO_INTERNAL_Id id;
    SIMPRO_INTERNAL_Id parentId;
    uint64_t startTime_x;
    uint64_t previousTime_x;
} SIMPRO_INTERNAL_LocalRangeData;
SIMPRO_INTERNAL_LocalRangeData SIMPRO_INTERNAL_RangeBegin(SIMPRO_Id id);
void SIMPRO_INTERNAL_RangeEnd(SIMPRO_INTERNAL_LocalRangeData *range);
void SIMPRO_INTERNAL_AddBandwidth(uint64_t processedBytes);

//*
#endif
#ifdef SIMPRO_IMPL
// */

// we know that the time is in us
#define SIMPRO_LEN(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

static uint64_t SIMPRO_GuessCPUTimerFreqHz(uint64_t const toWait_ms)
{
    uint64_t const toWait_us = 1000 * toWait_ms;
    // These are all volatile because the relative order of them needs to be this
    // exact way. The "speed penalty" is irrelevant in this function because it is
    // only called once.
    volatile uint64_t const cpuStart_x = SIMPRO_ReadCPUTimer_x();
    volatile uint64_t const systemStart_us = SIMPRO_ReadSystemTime_us();
    volatile uint64_t cpuEnd_x = 0;
    volatile uint64_t systemEnd_us = 0;
    volatile uint64_t elapsedTime_us = 0;
    do
    {
        cpuEnd_x = SIMPRO_ReadCPUTimer_x();
        systemEnd_us = SIMPRO_ReadSystemTime_us();
        elapsedTime_us = systemEnd_us - systemStart_us;
    } while (elapsedTime_us < toWait_us);

    uint64_t const cpuHz = (1000 * (cpuEnd_x - cpuStart_x)) / toWait_ms;
    return cpuHz;
}

//--------------------------------------------------------------------------------
typedef struct
{
    uint64_t callnumber;
    uint64_t processedBytes;
    uint64_t timeInclusiveChildren_x;
    uint64_t timeExclusiveChildren_x;
} SIMPRO_Range;

static struct
{
    uint64_t globalStartTime_x;
    SIMPRO_INTERNAL_Id activeRange;
    // 0 -> global range, other: x -> range for id x-1
    SIMPRO_Range ranges[1 << (sizeof(SIMPRO_Id) * 8)];
} SIMPRO_GlobalProfileState = { 0 };

void SIMPRO_GlobalBegin(void)
{
    SIMPRO_GlobalProfileState.globalStartTime_x = SIMPRO_ReadCPUTimer_x();
    SIMPRO_GlobalProfileState.activeRange = 0;
}

static char const *SIMPRO_FallbackConverter(SIMPRO_Id id)
{
    static const char lut[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    static char buffer[2 + 2 * sizeof(id) + 1] = { '0', 'x' };
    {
        _Static_assert(sizeof(id) == 1, "Update this scope");
        buffer[2] = lut[(id & 0xF0) >> 4];
        buffer[3] = lut[(id & 0x0F) >> 0];
        buffer[4] = '\0';
    }
    return buffer;
}

void SIMPRO_GlobalFinalizeAndPrint(SIMPRO_printf *p, SIMPRO_IdLookup *lookup)
{
    if (!p)
    {
        return;
    }
    if (!lookup)
    {
        lookup = &SIMPRO_FallbackConverter;
    }

    // index 0 is the full program
    {
        SIMPRO_Range *root = SIMPRO_GlobalProfileState.ranges + 0;
        root->callnumber = 1;
        root->timeInclusiveChildren_x = SIMPRO_ReadCPUTimer_x() - SIMPRO_GlobalProfileState.globalStartTime_x;
        root->timeExclusiveChildren_x += root->timeInclusiveChildren_x;
    }

    uint64_t const cpuFreq_hz = SIMPRO_GuessCPUTimerFreqHz(100);
    double const fullRuntime_x = (double)SIMPRO_GlobalProfileState.ranges[0].timeInclusiveChildren_x;

    (void)p(
        "Time measurements: (based on guessed CPU frequency of %f GHz)\n"
        "names;calls;"
        "incl. children .1ms;incl. children %%;"
        "excl. children .1ms;excl children %%;"
        "data MiB;bandwidth MiB/s\n"
        /**/,
        (double)cpuFreq_hz / 1000000000.0
    );
    for (unsigned i = 0; i < SIMPRO_LEN(SIMPRO_GlobalProfileState.ranges); ++i)
    {
        SIMPRO_Range const *data = SIMPRO_GlobalProfileState.ranges + i;
        if (0 < data->callnumber)
        {
            char const *name = (0 == i) ? "root" : lookup((SIMPRO_Id)(i - 1));

            double const bandwidthMiB =
                ((double)cpuFreq_hz * (double)data->processedBytes) /
                ((double)data->timeInclusiveChildren_x * 1024lu * 1024lu);
            (void)p(
                "%s;%lu;%lu;%f;%lu;%f;%lu;%f\n"
                /**/,
                name,
                data->callnumber,
                (10000 * data->timeInclusiveChildren_x) / cpuFreq_hz,
                (100 * (double)data->timeInclusiveChildren_x) / fullRuntime_x,
                (10000 * data->timeExclusiveChildren_x) / cpuFreq_hz,
                (100 * (double)data->timeExclusiveChildren_x) / fullRuntime_x,
                data->processedBytes,
                bandwidthMiB
            );
        }
    }
}

SIMPRO_INTERNAL_LocalRangeData SIMPRO_INTERNAL_RangeBegin(SIMPRO_Id id)
{
    // 0 -> global, so we simply add one to it.
    SIMPRO_INTERNAL_Id const workingId = id + 1;
    SIMPRO_INTERNAL_LocalRangeData range = {
        .id = workingId,
        .parentId = SIMPRO_GlobalProfileState.activeRange,
        .previousTime_x = SIMPRO_GlobalProfileState.ranges[workingId].timeInclusiveChildren_x,
    };
    range.startTime_x = SIMPRO_ReadCPUTimer_x();

    SIMPRO_GlobalProfileState.activeRange = range.id;

    return range;
}

void SIMPRO_INTERNAL_RangeEnd(SIMPRO_INTERNAL_LocalRangeData *range)
{
    if(!range)
    {
        return;
    }

    uint64_t const elapsed_x = SIMPRO_ReadCPUTimer_x() - range->startTime_x;

    SIMPRO_Range *myOwn = SIMPRO_GlobalProfileState.ranges + range->id;
    SIMPRO_Range *parent = SIMPRO_GlobalProfileState.ranges + range->parentId;

    //
    // Idea:
    // 1) If I have any children, they will have reduced my "EXCLUSIVE" time already.
    //    So if i just add the full range, their time will be exactly excluded.
    // 2) Because I am a child I need to reduces the "EXCLUSIVE" time of my parent.
    // 3) If I want the time including my children, I can just use the time of the full range.
    //
    parent->timeExclusiveChildren_x -= elapsed_x;
    myOwn->timeExclusiveChildren_x += elapsed_x;
    myOwn->timeInclusiveChildren_x = range->previousTime_x + elapsed_x;
    myOwn->callnumber += 1;

    SIMPRO_GlobalProfileState.activeRange = range->parentId;
    *range = (SIMPRO_INTERNAL_LocalRangeData){ 0 };
}

void SIMPRO_INTERNAL_AddBandwidth(uint64_t processedBytes)
{
    SIMPRO_GlobalProfileState.ranges[SIMPRO_GlobalProfileState.activeRange].processedBytes += processedBytes;
}

#endif

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
