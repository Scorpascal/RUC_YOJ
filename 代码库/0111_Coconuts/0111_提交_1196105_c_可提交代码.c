#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int cmp_desc(const void *a, const void *b) {
    long long x = *(const long long*)a;
    long long y = *(const long long*)b;
    if (x < y) return 1;
    if (x > y) return -1;
    return 0;
}

int valid(long long n, long long m) {
    long long cur = n;
    for (long long i = 0; i < m; ++i) {
        if (cur % m != 1) return 0;
        // cur = cur - 1 - (cur-1)/m  => (m-1)*(cur-1)/m
        cur = (m - 1) * ((cur - 1) / m);
    }
    return (cur % m) == 0;
}

int main(void) {
    long long n;
    while (scanf("%lld", &n) == 1 && n != 0) {
        if (n < 2) {
            printf("no solution\n");
            continue;
        }
        long long t = n - 1;
        long long limit = (long long)floor(sqrt((double)t));
        long long *divs = NULL;
        size_t cap = 0, cnt = 0;
        for (long long i = 1; i <= limit; ++i) {
            if (t % i == 0) {
                long long d1 = i;
                long long d2 = t / i;
                if (d1 > 1) {
                    if (cnt + 1 > cap) { cap = cap ? cap*2 : 8; divs = realloc(divs, cap * sizeof(long long)); }
                    divs[cnt++] = d1;
                }
                if (d2 > 1 && d2 != d1) {
                    if (cnt + 1 > cap) { cap = cap ? cap*2 : 8; divs = realloc(divs, cap * sizeof(long long)); }
                    divs[cnt++] = d2;
                }
            }
        }
        if (cnt == 0) {
            printf("no solution\n");
            free(divs);
            continue;
        }
        qsort(divs, cnt, sizeof(long long), cmp_desc);
        int found = 0;
        for (size_t i = 0; i < cnt; ++i) {
            long long m = divs[i];
            if (m <= 1) continue;
            if (valid(n, m)) {
                printf("%lld\n", m);
                found = 1;
                break;
            }
        }
        if (!found) printf("no solution\n");
        free(divs);
    }
    return 0;
}