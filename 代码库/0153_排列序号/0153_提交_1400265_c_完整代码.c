#include <stdio.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    int a[25];
    for (int i = 0; i < n; ++i) {
        if (scanf("%d", &a[i]) != 1) return 0;
    }

    // 预计算阶乘
    unsigned long long fact[21];
    fact[0] = 1ULL;
    for (int i = 1; i <= n; ++i) fact[i] = fact[i - 1] * (unsigned long long)i;

    // 计算字典序排名（从1开始）
    unsigned long long rank = 1ULL;
    for (int i = 0; i < n; ++i) {
        int cnt_smaller_right = 0;
        for (int j = i + 1; j < n; ++j) {
            if (a[j] < a[i]) ++cnt_smaller_right;
        }
        rank += (unsigned long long)cnt_smaller_right * fact[n - i - 1];
    }

    printf("%llu\n", rank);
    return 0;
}