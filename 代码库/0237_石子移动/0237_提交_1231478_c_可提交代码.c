#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static int cmp_ll(const void *a, const void *b) {
    long long va = *(const long long*)a;
    long long vb = *(const long long*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

int main(void) {
    int N;
    if (scanf("%d", &N) != 1) return 0;
    long long *a = malloc(sizeof(long long) * N);
    for (int i = 0; i < N; ++i) {
        if (scanf("%lld", &a[i]) != 1) a[i] = 0;
    }
    long long sum = 0;
    for (int i = 0; i < N; ++i) sum += a[i];
    long long avg = sum / N;

    long long *c = malloc(sizeof(long long) * N);
    c[0] = 0;
    for (int i = 1; i < N; ++i) {
        c[i] = c[i-1] + (a[i-1] - avg);
    }

    qsort(c, N, sizeof(long long), cmp_ll);
    long long median = c[N/2];
    long long ans = 0;
    for (int i = 0; i < N; ++i) ans += llabs(c[i] - median);

    printf("%lld\n", ans);

    free(a);
    free(c);
    return 0;
}