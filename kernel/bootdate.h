#ifndef KERNEL_BOOTDATE_H
#define KERNEL_BOOTDATE_H

#include <stdint.h>

struct date {
    uint32_t year;
    uint32_t month;
    uint32_t day;
};

void timestamp_to_ymd(uint64_t ts, struct date *d);
#endif