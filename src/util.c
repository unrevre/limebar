#include "util.h"

uint32_t hash(const char* string) {
    uint32_t key = 5381;

    char c;

    while ((c = *string++) != '\0')
        key += (key << 5) + c;

    return key;
}
