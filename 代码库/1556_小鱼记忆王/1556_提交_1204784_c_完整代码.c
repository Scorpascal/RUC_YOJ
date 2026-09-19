#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef getchar_unlocked
#define getchar_unlocked getchar
#endif

static int read_int_ll(long long *out) {
    int c = getchar_unlocked();
    if (c == EOF) return 0;
    while (c != '-' && (c < '0' || c > '9')) {
        c = getchar_unlocked();
        if (c == EOF) return 0;
    }
    int sign = 1;
    if (c == '-') {
        sign = -1;
        c = getchar_unlocked();
    }
    long long x = 0;
    while (c >= '0' && c <= '9') {
        x = x * 10 + (c - '0');
        c = getchar_unlocked();
    }
    *out = x * sign;
    return 1;
}

int main(void) {
    int capacity = 1024;
    int size = 0;
    int *arr = malloc(capacity * sizeof(int));
    if (!arr) return 0;

    long long v;
    while (read_int_ll(&v)) {
        if (v == 0) break;
        if (size == capacity) {
            capacity <<= 1;
            int *tmp = realloc(arr, capacity * sizeof(int));
            if (!tmp) { free(arr); return 0; }
            arr = tmp;
        }
        arr[size++] = (int)v;
    }

    if (size > 0) {
        for (int i = size - 1; i >= 0; --i) {
            if (i != size - 1) putchar(' ');
            printf("%d", arr[i]);
        }
    }
    putchar('\n');

    free(arr);
    return 0;
}