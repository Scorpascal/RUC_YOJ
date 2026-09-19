#include <stdio.h>

int main(void) {
    int m, n;
    if (scanf("%d %d", &m, &n) != 2) return 0;

    long long sum = 0;
    for (int k = m; k <= n; ++k) {
        int block = (k - 1) / 2;            // 每两个数为一组
        int sign = (block % 2 == 0) ? 1 : -1; // 组号为偶数时为正，否则为负
        sum += sign * k;
    }

    printf("%lld\n", sum);
    return 0;
}