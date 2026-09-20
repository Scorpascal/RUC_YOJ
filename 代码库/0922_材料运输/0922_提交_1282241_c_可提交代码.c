#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static inline long long llabsll(long long x) { return x >= 0 ? x : -x; }

int cmp_ll(const void *a, const void *b) {
    long long x = *(const long long*)a;
    long long y = *(const long long*)b;
    return (x > y) - (x < y);
}

int main(void) {
    // 加速IO（在某些平台可选）
    // 但标准IO足够，避免复杂性

    int n;
    if (scanf("%d", &n) != 1) return 0;
    if (n <= 0) { printf("0\n"); return 0; }

    long long *a = (long long*)malloc(sizeof(long long) * (size_t)n);
    if (!a) return 0;

    long long sum = 0;
    for (int i = 0; i < n; ++i) {
        long long v;
        if (scanf("%lld", &v) != 1) { free(a); return 0; }
        a[i] = v;
        sum += v;
    }

    if (sum % n != 0) {
        // 无法达到整数均分
        printf("-1\n");
        free(a);
        return 0;
    }

    long long avg = sum / n;

    if (n == 1) {
        printf("0\n");
        free(a);
        return 0;
    }

    // 为环路构造前缀和 S[0..n-1]，包含 S[0]=0
    long long *S = (long long*)malloc(sizeof(long long) * (size_t)n);
    if (!S) { free(a); return 0; }

    S[0] = 0;
    long long pref = 0;
    for (int i = 1; i < n; ++i) {
        pref += (a[i - 1] - avg);
        S[i] = pref;
    }

    // 排序并取中位数（环路需包含 S[0]）
    qsort(S, (size_t)n, sizeof(long long), cmp_ll);
    long long m = S[n / 2];

    // 计算总代价（包含 i=0..n-1）
    long long cost = 0;
    for (int i = 0; i < n; ++i) {
        cost += llabsll(S[i] - m);
    }

    printf("%lld\n", cost);

    free(S);
    free(a);
    return 0;
}