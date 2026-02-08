#ifndef LIBKERNEL_STRING_H
#define LIBKERNEL_STRING_H

#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *s);
char *strcat(char *dest, const char *src);
void u64_to_hex(uint64_t value, char *buffer);
void u64_to_dec(uint64_t value, char *buffer);

#endif