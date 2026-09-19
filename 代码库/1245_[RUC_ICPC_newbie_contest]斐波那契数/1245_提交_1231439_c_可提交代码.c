#include <stdio.h>

static int avg_digits_gt4(unsigned long long x) {
    int sum = 0, len = 0;
    while (x > 0) {
        sum += (int)(x % 10);
        x /= 10;
        ++len;
    }
    return sum > 4 * len;
}

int main(void) {
    unsigned long long n;
    if (scanf("%llu", &n) != 1) return 0;

    // 生成序列：1, 2, 3, 5, ...
    unsigned long long a = 1, b = 1; // 当前项 a，上一个项 b
    int ans = 0;

    while (a < n) {
        if (avg_digits_gt4(a)) ++ans;
        unsigned long long next = a + b;
        b = a;
        a = next;
    }

    printf("%d\n", ans);
    return 0;
}