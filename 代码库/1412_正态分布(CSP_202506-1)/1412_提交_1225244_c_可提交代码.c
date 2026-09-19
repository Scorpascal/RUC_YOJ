#include <stdio.h>

int main(void) {
    int k;
    if (scanf("%d", &k) != 1) return 0;
    for (int t = 0; t < k; ++t) {
        long long mu, sigma, n;
        scanf("%lld %lld %lld", &mu, &sigma, &n);
        long long delta = n - mu;
        long long factor = 100 / sigma;   // 题目保证 sigma 是 100 的因子
        long long m100 = delta * factor;  // 等价于 z * 100，且为整数

        long long i = m100 / 10 + 1;      // 行：0.0 为第 1 行
        long long j = m100 % 10 + 1;      // 列：0.00 为第 1 列

        printf("%lld %lld\n", i, j);
    }
    return 0;
}