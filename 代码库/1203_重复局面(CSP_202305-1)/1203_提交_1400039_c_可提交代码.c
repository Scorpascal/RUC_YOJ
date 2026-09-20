#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t used;
    uint64_t hash;
    char key[64];
    int count;
} Entry;

static uint64_t fnv1a_64(const char *data) {
    const uint64_t FNV_OFFSET = 1469598103934665603ULL;
    const uint64_t FNV_PRIME  = 1099511628211ULL;
    uint64_t h = FNV_OFFSET;
    for (int i = 0; i < 64; ++i) {
        h ^= (unsigned char)data[i];
        h *= FNV_PRIME;
    }
    return h;
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    // 哈希表容量取>=4n的2次幂，降低冲突
    size_t cap = 1;
    size_t need = (size_t)n * 4u;
    while (cap < need) cap <<= 1;
    if (cap < 8) cap = 8;

    Entry *tab = (Entry *)calloc(cap, sizeof(Entry));
    if (!tab) return 0;

    char row[9];
    char key[64];

    for (int pos = 0; pos < n; ++pos) {
        // 读取并拼接成64字节键
        for (int r = 0; r < 8; ++r) {
            scanf("%8s", row);
            memcpy(key + r * 8, row, 8);
        }

        uint64_t h = fnv1a_64(key);
        size_t idx = (size_t)(h & (cap - 1));

        for (;;) {
            Entry *e = &tab[idx];
            if (!e->used) {
                e->used = 1;
                e->hash = h;
                memcpy(e->key, key, 64);
                e->count = 1;
                printf("%d\n", e->count);
                break;
            } else if (e->hash == h && memcmp(e->key, key, 64) == 0) {
                e->count += 1;
                printf("%d\n", e->count);
                break;
            }
            idx = (idx + 1) & (cap - 1);
        }
    }

    free(tab);
    return 0;
}