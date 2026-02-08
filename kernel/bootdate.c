#include "bootdate.h"

void timestamp_to_ymd(uint64_t ts, struct date *d) {
    uint64_t days = ts / 86400;
    uint64_t z = days + 719468;
    uint64_t era = (z >= 0 ? z : z - 146096) / 146097;
    uint64_t doe = z - era * 146097;
    uint64_t yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    d->year = (uint32_t)(yoe + era * 400);
    uint64_t doy = doe - (365*yoe + yoe/4 - yoe/100);
    uint64_t mp = (5*doy + 2)/153;
    d->day = (uint32_t)(doy - (153*mp+2)/5 + 1);
    d->month = (uint32_t)(mp < 10 ? mp + 3 : mp - 9);
    if (d->month <= 2) d->year += 1;
}