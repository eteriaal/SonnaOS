#include <libkernel/string.h>
#include <stdint.h>

size_t strlen(const char *s) {
    size_t len = 0;

    if (!s) {
        return 0;
    }

    while (s[len]) {
        len++;
    }

    return len;
}

char *strcat(char *dest, const char *src)
{
    char *end = dest;
    while (*end != '\0')
        end++;
    while (*src != '\0')
    {
        *end = *src;
        end++;
        src++;
    }
    *end = '\0';
    return dest;
}

void u64_to_hex(uint64_t value, char *buffer) {
    static const char digits[] = "0123456789ABCDEF";
    buffer[0] = '0';
    buffer[1] = 'x';

    for (int i = 0; i < 16; i++) {
        int shift = (15 - i) * 4;
        buffer[2 + i] = digits[(value >> shift) & 0xF];
    }
    buffer[18] = '\0';
}

void u64_to_dec(uint64_t value, char *buffer) {
    char temp[21];
    size_t idx = 0;

    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    while (value > 0 && idx < sizeof(temp)) {
        temp[idx++] = (char)('0' + (value % 10));
        value /= 10;
    }

    size_t out = 0;
    while (idx > 0) {
        buffer[out++] = temp[--idx];
    }
    buffer[out] = '\0';
}
